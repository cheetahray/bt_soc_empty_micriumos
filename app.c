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
#include "lcd_ui.h"
#include "sl_simple_button_instances.h"
#include "cmsis_os2.h"
#include <stdio.h>

/* Debug print macro - outputs to BLE if connected, otherwise to UART */
#define DEBUG_PRINT(fmt, ...) do { \
    if (ble_log_is_connected()) { \
        BLE_PRINTF(fmt, ##__VA_ARGS__); \
    } else { \
        printf(fmt, ##__VA_ARGS__); \
    } \
} while(0)

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// Event flags for button handling (to avoid LCD operations in interrupt context)
static osEventFlagsId_t button_event_flags = NULL;
#define BTN0_PRESSED_FLAG  (1U << 0)  // Button 0 pressed event
#define BTN1_PRESSED_FLAG  (1U << 1)  // Button 1 pressed event

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
    
    if (extscr_tgr_stamp == 0 && tgr_val != 0) {
        /* Task trigger activated */
        result = 2;
        if (update) {
            extscr_tgr_stamp = tgr_val;
        }
    } 
    else if (extscr_tgr_stamp != 0 && tgr_val == 0) {
        /* Task trigger cleared */
        result = -2;
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
    /* These functions should be implemented based on your config system */
    /* Example placeholder values: */
    round_test_parm.txpwr = 0;  // Default: 0 dBm (safe and moderate power)
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
    
    /* Create event flags for button handling */
    button_event_flags = osEventFlagsNew(NULL);
    if (button_event_flags == NULL) {
        DEBUG_PRINT("[BTN] ERROR: Failed to create event flags\n");
    }
    
    /* Initialize LCD UI */
    if (lcd_ui_init() == 0) {
        DEBUG_PRINT("[LCD] Display initialized\n");
    } else {
        DEBUG_PRINT("[LCD] Failed to initialize display\n");
    }
    
    /* Initialize BLE loss test service */
    err = losstst_init();
    if (err) {
        DEBUG_PRINT("ERROR: losstst_init failed: %d\n", err);
        /* Continue anyway - some features may still work */
    }
    
    /* Initialize external peripherals if needed */
    // extscr_init();  // External screen/UART interface
    
    /* Load default parameters */
    load_parm_cfg();
    
    /* Show startup screen with loaded configuration */
    lcd_ui_show_startup(&round_test_parm);

    DEBUG_PRINT("=== Application Ready ===\n");
    DEBUG_PRINT("[BLE LOG] Log characteristic handle: %d\n", BLE_LOG_CHARACTERISTIC_HANDLE);
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
    
    /* Handle button events from interrupt context */
    if (button_event_flags != NULL) {
        uint32_t flags = osEventFlagsGet(button_event_flags);
        
        if (flags & BTN0_PRESSED_FLAG) {
            osEventFlagsClear(button_event_flags, BTN0_PRESSED_FLAG);
            DEBUG_PRINT("[BTN] Processing Button 0 - expand/select\n");
            lcd_ui_expand_selection();
        }
        
        if (flags & BTN1_PRESSED_FLAG) {
            osEventFlagsClear(button_event_flags, BTN1_PRESSED_FLAG);
            DEBUG_PRINT("[BTN] Processing Button 1 - next selection\n");
            lcd_ui_next_selection();
        }
    }
    
    static uint32_t uptime_barrier_ms = 0;
    bool re_sche = false;
    
    /* One-time initialization delay */
    // if (!initialized) {
    //     uptime_barrier_ms = sl_sleeptimer_get_tick_count() + 2000;
        
    //     /* Wait for initial startup delay */
    //     if (sl_sleeptimer_get_tick_count() < uptime_barrier_ms) {
    //         return;
    //     }
    //     initialized = true;
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
            // 不调用 load_parm_cfg()，保留用户在 LCD 上的设置
            task_delay = false;
        }
        else if (scanner_task_tgr(0)) {
            task_ENVMON = false;
            task_SENDER = false;
            task_SCANNER = true;
            task_NUMCAST = false;
            // 不调用 load_parm_cfg()，保留用户在 LCD 上的设置
            task_delay = false;
        }
        else if (numcst_task_tgr(0)) {
            task_ENVMON = false;
            task_SENDER = false;
            task_SCANNER = false;
            task_NUMCAST = true;
            // 不调用 load_parm_cfg()，保留用户在 LCD 上的设置
            task_delay = false;
        }
        else if (envmon_task_tgr(0)) {
            task_ENVMON = true;
            task_SENDER = false;
            task_SCANNER = false;
            task_NUMCAST = false;
            // 不调用 load_parm_cfg()，保留用户在 LCD 上的设置
            task_delay = false;
        }
        else {
            task_ENVMON = false;
            task_SENDER = false;
            task_SCANNER = false;
            task_NUMCAST = false;
        }
        
        /* ========== Task Setup Phase ========== */
        if (task_SCANNER || task_SENDER || task_NUMCAST || task_ENVMON) {
            // Range test 使用 advertising sets 0-4
            // Connection advertising (set 5) 继续运行，允许 BLE log 访问
            // Silicon Labs BLE stack 支持多个 advertising sets 同时运行
            DEBUG_PRINT("[ADV] Range test starting with sets 0-4 (connection set 5 remains active)\n");
        }
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
            sl_sleeptimer_delay_millisecond(10);
            re_sche = is_re_sche(false);
            if (re_sche) break;
        } while (sl_sleeptimer_get_tick_count() < uptime_barrier_ms);
        
        if (re_sche) {
            task_SCANNER = false;
            task_SENDER = false;
            task_NUMCAST = false;
            DEBUG_PRINT("[ADV] Range test interrupted during setup\n");
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
            sl_sleeptimer_delay_millisecond(10);
            re_sche = is_re_sche(false);
            if (re_sche) break;
        } while (sl_sleeptimer_get_tick_count() < uptime_barrier_ms);
        
        if (re_sche) {
            task_SCANNER = false;
            task_SENDER = false;
            task_NUMCAST = false;
            DEBUG_PRINT("[ADV] Range test interrupted\n");
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
            numcst_task_tgr(-numcst_task_tgr(0));
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

    switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
        case sl_bt_evt_system_boot_id:
            DEBUG_PRINT("[ADV] System boot - initializing\n");
            // 建立手机连接用的 advertising set (set 5)
            // losstst_svc 使用 sets 0-4 for range test
            sc = sl_bt_advertiser_create_set(&advertising_set_handle);
            app_assert_status(sc);

            // Generate data for advertising
            sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                                                                 sl_bt_advertiser_general_discoverable);
            app_assert_status(sc);

            // Set advertising interval to 100ms.
            sc = sl_bt_advertiser_set_timing(
                advertising_set_handle,
                160, // min. adv. interval (milliseconds * 1.6)
                160, // max. adv. interval (milliseconds * 1.6)
                0,   // adv. duration
                0);  // max. num. adv. events
            app_assert_status(sc);
            // Start advertising and enable connections.
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
      
      DEBUG_PRINT("[ADV] Connection advertising restarted\n");
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

/**************************************************************************//**
 * Button callback handler.
 * This is called in INTERRUPT CONTEXT - keep it fast!
 * Do NOT call LCD functions directly - use event flags instead.
 *****************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  // Button 0: Expand current item / Select sub-option
  if (handle == &sl_button_btn0) {
    if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
      // Set flag for processing in task context
      if (button_event_flags != NULL) {
        osEventFlagsSet(button_event_flags, BTN0_PRESSED_FLAG);
      }
    }
  }
  
  // Button 1: Navigate to next item (in main menu or sub-menu)
  if (handle == &sl_button_btn1) {
    if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
      // Set flag for processing in task context
      if (button_event_flags != NULL) {
        osEventFlagsSet(button_event_flags, BTN1_PRESSED_FLAG);
      }
    }
  }
}

