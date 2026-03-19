/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "sl_bt_api.h"
#include "sl_main_init.h"
#include "app_assert.h"
#include "app.h"
#include "losstst_svc.h"
#include "ble_log.h"
#include "sl_iostream.h"
#include "sl_iostream_init_usart_instances.h"
#include "silabs_spec_uart_shim.h"
#include "cmsis_os2.h"
#include <stdio.h>

extern void extscr_init(void);

/* EXP_PRINTF: print directly to expansion header UART (always) */
#define EXP_PRINTF(fmt, ...) do { \
    char _exp_buf[128]; \
    int _exp_len = snprintf(_exp_buf, sizeof(_exp_buf), fmt, ##__VA_ARGS__); \
    if (_exp_len > 0) { \
        sl_iostream_write(sl_iostream_exp_handle, _exp_buf, (size_t)_exp_len); \
    } \
} while(0)

/* Debug print macro - outputs to BLE if connected, otherwise to UART */
#define DEBUG_PRINT(fmt, ...) do { \
    if (ble_log_is_connected()) { \
        BLE_PRINTF(fmt, ##__VA_ARGS__); \
    } else { \
        printf(fmt, ##__VA_ARGS__); \
    } \
} while(0)

/* is_re_sche() verbose debug log: 1=on, 0=off (produces high-frequency output) */
#ifndef IS_RE_SCHE_DEBUG_VERBOSE
#define IS_RE_SCHE_DEBUG_VERBOSE 0
#endif

/* Scan RX verbose log switch: 1=on, 0=off */
#ifndef SCAN_RX_VERBOSE_LOG
#define SCAN_RX_VERBOSE_LOG 0
#endif

#if SCAN_RX_VERBOSE_LOG
#define SCAN_LOG(fmt, ...) DEBUG_PRINT(fmt, ##__VA_ARGS__)
#else
#define SCAN_LOG(fmt, ...) do { } while (0)
#endif

/* Optional scan log target address filter (printed order: addr[5]:...:addr[0]) */
#ifndef SCAN_LOG_TARGET_FILTER
#define SCAN_LOG_TARGET_FILTER 0
#endif

#define SCAN_LOG_TARGET_ADDR_5 0xC3
#define SCAN_LOG_TARGET_ADDR_4 0xD7
#define SCAN_LOG_TARGET_ADDR_3 0x89
#define SCAN_LOG_TARGET_ADDR_2 0x36
#define SCAN_LOG_TARGET_ADDR_1 0xDA
#define SCAN_LOG_TARGET_ADDR_0 0x9E

static bool scan_log_addr_match(const bd_addr *addr)
{
#if SCAN_LOG_TARGET_FILTER
    if (addr == NULL) {
        return false;
    }
    return (addr->addr[5] == SCAN_LOG_TARGET_ADDR_5
            && addr->addr[4] == SCAN_LOG_TARGET_ADDR_4
            && addr->addr[3] == SCAN_LOG_TARGET_ADDR_3
            && addr->addr[2] == SCAN_LOG_TARGET_ADDR_2
            && addr->addr[1] == SCAN_LOG_TARGET_ADDR_1
            && addr->addr[0] == SCAN_LOG_TARGET_ADDR_0);
#else
    (void)addr;
    return true;
#endif
}

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// BLE Log characteristic handle
// Log Output characteristic value handle (0x1b = 27) from gatt_db.c
#define BLE_LOG_CHARACTERISTIC_HANDLE  27  
static uint8_t current_connection = 0xFF;
/* ================== Global Variables ================== */

/* Task state flags */
bool task_ENVMON = false;
bool task_SENDER = false;
bool task_SCANNER = false;
bool task_NUMCAST = false;
bool task_delay = false;

/* Test parameters */
test_param_t round_test_parm;

/* ================== Helper Functions ================== */

/**
 * @brief Check if reschedule is needed
 * 
 * Monitors task trigger changes from external sources (UART commands, etc.)
 * 
 * @param update If true, update internal state after detecting change
 * @return 1: config changed, 2: task trigger set, -2: task trigger cleared, 0: no change
 */
static int is_re_sche(int update)
{
    int result = 0;
    static int16_t extscr_tgr_stamp = 0;
    
    /* Get maximum of all task triggers */
    int16_t tgr_val = sender_task_tgr(0);
    if (scanner_task_tgr(0) > tgr_val) tgr_val = scanner_task_tgr(0);
    if (numcst_task_tgr(0) > tgr_val) tgr_val = numcst_task_tgr(0);
    if (envmon_task_tgr(0) > tgr_val) tgr_val = envmon_task_tgr(0);
    
#if IS_RE_SCHE_DEBUG_VERBOSE
    /* Debug only when values change */
    if (last_logged_stamp != extscr_tgr_stamp || last_logged_tgr_val != tgr_val) {
        DEBUG_PRINT("is_re_sche: update=%d stamp=%d tgr_val=%d numcst=%d\n",
                   update, extscr_tgr_stamp, tgr_val, numcst_task_tgr(0));
        last_logged_stamp = extscr_tgr_stamp;
        last_logged_tgr_val = tgr_val;
    }
#endif

    if (extscr_tgr_stamp == 0 && tgr_val != 0) {
        /* Task trigger activated */
        result = 2;
#if IS_RE_SCHE_DEBUG_VERBOSE
        DEBUG_PRINT("is_re_sche: Task activated! stamp=0->%d, result=%d, update=%d\n",
                   tgr_val, result, update);
#endif
        if (update) {
            extscr_tgr_stamp = tgr_val;
        }
    }
    else if (extscr_tgr_stamp != 0 && tgr_val == 0) {
        /* Task trigger cleared */
        result = -2;
#if IS_RE_SCHE_DEBUG_VERBOSE
        DEBUG_PRINT("is_re_sche: Task STOPPED! stamp=%d->0, result=%d, update=%d\n",
                   extscr_tgr_stamp, result, update);
#endif
        if (update) {
            extscr_tgr_stamp = tgr_val;
        }
    } 
    /* Note: Silicon Labs version doesn't have DIP switch support */
    /* Nordic's poll_cfg_switch() and load_parm_dipswitch() are not ported */
    
    return result;
}

/**
 * @brief Abort callbacks for each test mode
 */
static bool tst_sender_abort(void)
{
    return is_re_sche(0);
}

static bool tst_scanner_abort(void)
{
    return is_re_sche(0);
}

static bool tst_numcast_abort(void)
{
    return is_re_sche(0);
}

static bool tst_envmon_abort(void)
{
    return is_re_sche(0);
}

/**
 * @brief Load test parameters from configuration
 * 
 * This loads parameters from external configuration sources (UART, settings, etc.)
 * For Silicon Labs, this should be adapted to your config system.
 */
static void load_parm_cfg(void)
{
    round_test_parm.txpwr = enum_txpower(0);
    round_test_parm.count_idx = enum_totalnum_idx(0);      // enum_totalnum_idx(0)
    round_test_parm.interval_idx = enum_adv_interval_idx(0);   // enum_adv_interval_idx(0)
    
    /* Abort callbacks */
    round_test_parm.envmon_abort = tst_envmon_abort;
    round_test_parm.sender_abort = tst_sender_abort;
    round_test_parm.scanner_abort = tst_scanner_abort;
    round_test_parm.numcast_abort = tst_numcast_abort;
    
    /* PHY selection - default to all enabled */
    round_test_parm.phy_2m = get_cfg_phy_sel(0);      // get_cfg_phy_sel(0)
    round_test_parm.phy_1m = get_cfg_phy_sel(1);      // get_cfg_phy_sel(1)
    round_test_parm.phy_s8 = get_cfg_phy_sel(2);      // get_cfg_phy_sel(2)
    round_test_parm.phy_ble4 = get_cfg_phy_sel(3);    // get_cfg_phy_sel(3)
    
    /* Channel selection - default all channels enabled */
    round_test_parm.inhibit_ch37 = !get_cfg_ch37();  // !get_cfg_ch37()
    round_test_parm.inhibit_ch38 = !get_cfg_ch38();  // !get_cfg_ch38()
    round_test_parm.inhibit_ch39 = !get_cfg_ch39();  // !get_cfg_ch39()
    
    /* Other settings */
    round_test_parm.non_ANONYMOUS = get_cfg_NON_ANONYMOUS(); // get_cfg_NON_ANONYMOUS()
    round_test_parm.ignore_rcv_resp = get_uni_cast_method(); // get_uni_cast_method()
}

void app_init(void)
{
    int err;
    
    /* Initialize BLE log service */
    ble_log_init();
    
    /* Initialize BLE loss test service */
    err = losstst_init();
    if (err) {
        /* Continue anyway - some features may still work */
    }
    
    /* Initialize external peripherals if needed */
    spec_uart_shim_init();  /* Must come before extscr_init */
    extscr_init();  /* Start external screen/UART interface thread */
    
    /* Load default parameters */
    load_parm_cfg();
    
    //EXP_PRINTF("=== EXP UART OK ===\r\n");

}

// Application Process Action.
void app_process_action(void)
{
    if (app_is_process_required()) {
    /////////////////////////////////////////////////////////////////////////////
    // Put your additional application code here!                              //
    // This is will run each time app_proceed() is called.                     //
    // Do not call blocking functions from here!                               //
    /////////////////////////////////////////////////////////////////////////////
    
    static uint32_t uptime_barrier_ms = 0;
    bool re_sche = false;
    
    /* One-time initialization delay */
    // uptime_barrier_ms = sl_sleeptimer_get_tick_count() + 2000;
    
    // /* Wait for initial startup delay */
    // while (sl_sleeptimer_get_tick_count() < uptime_barrier_ms) {
    //     if(platform_can_yield()) {
    //         platform_yield();
    //     } 
    // }
    
    /* ========== Task Selection Phase ========== */
    if (!task_ENVMON && !task_SCANNER && !task_SENDER && !task_NUMCAST) {
        /* No task active - check for task triggers */
        
        /* Note: DIP switch logic (poll_cfg_switch, load_parm_dipswitch) not ported */
        /* Silicon Labs version uses UART/external commands only */
        
        if (sender_task_tgr(0)) {
            task_ENVMON = false;
            task_SENDER = true;
            task_SCANNER = false;
            task_NUMCAST = false;
            load_parm_cfg();  // 讀取最新的 cfg_phy_sel[] 等 LCD 設定
            task_delay = false;
        }
        else if (scanner_task_tgr(0)) {
            task_ENVMON = false;
            task_SENDER = false;
            task_SCANNER = true;
            task_NUMCAST = false;
            load_parm_cfg();  // 讀取最新的 cfg_phy_sel[] 等 LCD 設定
            task_delay = false;
        }
        else if (numcst_task_tgr(0)) {
            task_ENVMON = false;
            task_SENDER = false;
            task_SCANNER = false;
            task_NUMCAST = true;
            load_parm_cfg();  // 讀取最新的 cfg_phy_sel[] 等 LCD 設定
            task_delay = false;
        }
        else if (envmon_task_tgr(0)) {
            task_ENVMON = true;
            task_SENDER = false;
            task_SCANNER = false;
            task_NUMCAST = false;
            load_parm_cfg();  // 讀取最新的 cfg_phy_sel[] 等 LCD 設定
            task_delay = false;
        }
        else {
            task_ENVMON = false;
            task_SENDER = false;
            task_SCANNER = false;
            task_NUMCAST = false;
        }
        
        /* ========== Task Setup Phase ========== */
        if (task_SCANNER) {
            blocking_adv(0);
            blocking_adv(1);
            blocking_adv(2);
            blocking_adv(3);
            scanner_setup(&round_test_parm);
        }
        else if (task_SENDER) {
            blocking_adv(0);
            blocking_adv(1);
            blocking_adv(2);
            blocking_adv(3);
            sender_setup(&round_test_parm);
        }
        else if (task_NUMCAST) {
            blocking_adv(0);
            blocking_adv(1);
            blocking_adv(2);
            blocking_adv(3);
            numcast_setup(&round_test_parm);
            is_re_sche(true);
            return;
        }
        else if (task_ENVMON) {
            blocking_adv(0);
            blocking_adv(1);
            blocking_adv(2);
            blocking_adv(3);
            envmon_setup(&round_test_parm);
            is_re_sche(true);
            return;
        }
        
        /* Wait period after setup */
        is_re_sche(true);
        uptime_barrier_ms = sl_sleeptimer_get_tick_count() + 1000;
        
        do {
            osDelay(10);  /* proper RTOS sleep — yields CPU to other tasks */
            re_sche = is_re_sche(false);
            if (re_sche) break;
        } while (sl_sleeptimer_get_tick_count() < uptime_barrier_ms);
        
        if (re_sche) {
            task_SCANNER = false;
            task_SENDER = false;
            task_NUMCAST = false;
            return;
        }
        
        /* Skip to execution if no task selected */
        if (!task_SCANNER && !task_SENDER) {
            return;
        }
        
        /* Stop BLE4 advertising before test */
        update_adv(3, NULL, NULL, NULL);
        
        /* Additional delay before starting test */
        if (task_delay) {
            uptime_barrier_ms = sl_sleeptimer_get_tick_count() + 
                               (task_SCANNER ? 1000 : 20000);
        }
        else {
            uptime_barrier_ms = sl_sleeptimer_get_tick_count() + 
                               (task_SCANNER ? 1000 : 3000);
        }
        
        do {
            osDelay(10);  /* proper RTOS sleep — yields CPU to other tasks */
            re_sche = is_re_sche(false);
            if (re_sche) break;
        } while (sl_sleeptimer_get_tick_count() < uptime_barrier_ms);
        
        if (re_sche) {
            task_SCANNER = false;
            task_SENDER = false;
            task_NUMCAST = false;
            return;
        }
    }
    
    /* ========== Task Execution Phase ========== */
    if (task_ENVMON) {
        int err = losstst_envmon();
        if (err <= 0) {
            task_ENVMON = false;
            envmon_task_tgr(-envmon_task_tgr(0));
        }
    }
    else if (task_SENDER) {
        int err = losstst_sender();
        if (err <= 0) {
            task_SENDER = false;
            sender_task_tgr(-sender_task_tgr(0));
        }
    }
    else if (task_SCANNER) {
        int err = losstst_scanner();
        if (err <= 0) {
            task_SCANNER = false;
            scanner_task_tgr(-scanner_task_tgr(0));
        }
    }
    else if (task_NUMCAST) {
        int err = losstst_numcast();
        if (err <= 0) {
            task_NUMCAST = false;
            sender_task_tgr(-sender_task_tgr(0));
        }
    }
    // 若所有 range test 任务都结束
    // Connection advertising (set 5) 继续运行，无需额外操作
  }
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the default weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
        uint32_t evt_id = SL_BT_MSG_ID(evt->header);
    static uint32_t legacy_scan_evt_count = 0;
    static uint32_t ext_scan_evt_count = 0;

        switch (evt_id) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
        DEBUG_PRINT("[ADV] System boot - initializing\n");
        /* Create connection advertising set (set 5); losstst_svc uses sets 0-4 */
        sc = sl_bt_advertiser_create_set(&advertising_set_handle);
        app_assert_status(sc);
        sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                   sl_bt_advertiser_general_discoverable);
        app_assert_status(sc);
        sc = sl_bt_advertiser_set_timing(advertising_set_handle,
                                         160, /* min interval 100 ms */
                                         160, /* max interval 100 ms */
                                         0,   /* adv duration: continuous */
                                         0);  /* max adv events: unlimited */
        app_assert_status(sc);
        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                           sl_bt_legacy_advertiser_connectable);
        app_assert_status(sc);
        DEBUG_PRINT("[ADV] Connection advertising started\n");
        break;

        // -------------------------------
        // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
        current_connection = evt->data.evt_connection_opened.connection;
        
        // Enable BLE log output to connected device
    #if BLE_LOG_CHARACTERISTIC_HANDLE != 0
        ble_log_set_connection(current_connection, BLE_LOG_CHARACTERISTIC_HANDLE);
        BLE_PRINTF("[BLE] Connection established\n");
    #endif
        break;

        // -------------------------------
        // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
        // Clear BLE log connection
    #if BLE_LOG_CHARACTERISTIC_HANDLE != 0
        ble_log_clear_connection();
    #endif
        current_connection = 0xFF;
        
        // 重启 connection advertising（range test 期间也可重连查看 BLE log）
        sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                    sl_bt_advertiser_general_discoverable);
        app_assert_status(sc);

        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                            sl_bt_legacy_advertiser_connectable);
        app_assert_status(sc);
        
        break;

        ///////////////////////////////////////////////////////////////////////////
        // Add additional event handlers here as your application requires!      //
        ///////////////////////////////////////////////////////////////////////////
    case sl_bt_evt_advertiser_timeout_id:
        // 手动调用我们的处理函数
        losstst_adv_sent_handler(evt->data.evt_advertiser_timeout.handle);
        break;    
    case sl_bt_evt_scanner_legacy_advertisement_report_id: {
        sl_bt_evt_scanner_legacy_advertisement_report_t *scan_evt = 
            &evt->data.evt_scanner_legacy_advertisement_report;
        bool log_this = scan_log_addr_match(&scan_evt->address);

        legacy_scan_evt_count++;
        /* Address-filtered detail log */
        if (log_this && (legacy_scan_evt_count <= 5 || (legacy_scan_evt_count % 200U) == 0U)) {
            SCAN_LOG("[SCAN_EVT][LEGACY] evt=0x%08lX cnt=%lu len=%u rssi=%d addr=%02X:%02X:%02X:%02X:%02X:%02X\n",
                     (unsigned long)evt_id,
                     (unsigned long)legacy_scan_evt_count,
                     scan_evt->data.len,
                     scan_evt->rssi,
                     scan_evt->address.addr[5], scan_evt->address.addr[4],
                     scan_evt->address.addr[3], scan_evt->address.addr[2],
                     scan_evt->address.addr[1], scan_evt->address.addr[0]);
        }
        /* Heartbeat: only for our test packets (man=0xFFFF form=0xBAAB) */
        bool is_test_pkt = losstst_check_form_id(scan_evt->data.data, scan_evt->data.len);
        if (log_this && is_test_pkt && (legacy_scan_evt_count <= 3 || (legacy_scan_evt_count % 500U) == 0U)) {
            DEBUG_PRINT("[SCAN_EVT][LEGACY] total=%lu rssi=%d len=%u addr=%02X:%02X:%02X:%02X:%02X:%02X\n",
                        (unsigned long)legacy_scan_evt_count,
                        scan_evt->rssi,
                        scan_evt->data.len,
                        scan_evt->address.addr[5], scan_evt->address.addr[4],
                        scan_evt->address.addr[3], scan_evt->address.addr[2],
                        scan_evt->address.addr[1], scan_evt->address.addr[0]);
        }
        
        sl_bt_scanner_process_legacy_report(
            &scan_evt->address,
            scan_evt->rssi,
            scan_evt->data.data,
            scan_evt->data.len
        );
        break;
        }
    case sl_bt_evt_scanner_extended_advertisement_report_id: {
        sl_bt_evt_scanner_extended_advertisement_report_t *scan_evt = 
            &evt->data.evt_scanner_extended_advertisement_report;
        bool log_this = scan_log_addr_match(&scan_evt->address);

        ext_scan_evt_count++;
        bool ext_is_test_pkt = losstst_check_form_id(scan_evt->data.data, scan_evt->data.len);
        if (log_this && ext_is_test_pkt && (ext_scan_evt_count <= 5 || (ext_scan_evt_count % 100U) == 0U)) {
            SCAN_LOG("[SCAN_EVT][EXT] evt=0x%08lX cnt=%lu len=%u rssi=%d txpwr=%d phy=%u/%u addr=%02X:%02X:%02X:%02X:%02X:%02X\n",
                     (unsigned long)evt_id,
                     (unsigned long)ext_scan_evt_count,
                     scan_evt->data.len,
                     scan_evt->rssi,
                     scan_evt->tx_power,
                     scan_evt->primary_phy,
                     scan_evt->secondary_phy,
                     scan_evt->address.addr[5], scan_evt->address.addr[4],
                     scan_evt->address.addr[3], scan_evt->address.addr[2],
                     scan_evt->address.addr[1], scan_evt->address.addr[0]);
        }
        
        sl_bt_scanner_process_extended_report(
            &scan_evt->address,
            scan_evt->rssi,
            scan_evt->tx_power,
            scan_evt->primary_phy,
            scan_evt->secondary_phy,
            scan_evt->data.data,
            scan_evt->data.len
        );
        break;
        }
        // -------------------------------
        // Default event handler.
    default:
        break;
    }
  
  // Signal the application task to proceed after BLE event processing
  app_proceed();
}

