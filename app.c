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
#include <stdio.h>

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
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
    round_test_parm.txpwr = 0;          // enum_txpower(0)
    round_test_parm.count_idx = 0;      // enum_totalnum_idx(0)
    round_test_parm.interval_idx = 0;   // enum_adv_interval_idx(0)
    
    /* Abort callbacks */
    round_test_parm.envmon_abort = tst_envmon_abort;
    round_test_parm.sender_abort = tst_sender_abort;
    round_test_parm.scanner_abort = tst_scanner_abort;
    round_test_parm.numcast_abort = tst_numcast_abort;
    
    /* PHY selection - default to all enabled */
    round_test_parm.phy_2m = true;      // get_cfg_phy_sel(0)
    round_test_parm.phy_1m = true;      // get_cfg_phy_sel(1)
    round_test_parm.phy_s8 = true;      // get_cfg_phy_sel(2)
    round_test_parm.phy_ble4 = true;    // get_cfg_phy_sel(3)
    
    /* Channel selection - default all channels enabled */
    round_test_parm.inhibit_ch37 = false;  // !get_cfg_ch37()
    round_test_parm.inhibit_ch38 = false;  // !get_cfg_ch38()
    round_test_parm.inhibit_ch39 = false;  // !get_cfg_ch39()
    
    /* Other settings */
    round_test_parm.non_ANONYMOUS = false; // get_cfg_NON_ANONYMOUS()
    round_test_parm.ignore_rcv_resp = false; // get_uni_cast_method()
}

void app_init(void)
{
    int err;
    
    /* Initialize BLE loss test service */
    err = losstst_init();
    if (err) {
        printf("ERROR: losstst_init failed: %d\n", err);
        /* Continue anyway - some features may still work */
    }
    
    /* Initialize external peripherals if needed */
    // extscr_init();  // External screen/UART interface
    
    /* Load default parameters */
    load_parm_cfg();

    printf("=== Application Ready ===\n");
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
    static bool initialized = false;
    static uint32_t uptime_barrier_ms = 0;
    bool re_sche = false;
    
    /* One-time initialization delay */
    if (!initialized) {
        uptime_barrier_ms = sl_sleeptimer_get_tick_count() + 2000;
        
        /* Wait for initial startup delay */
        if (sl_sleeptimer_get_tick_count() < uptime_barrier_ms) {
            return;
        }
        initialized = true;
    }
    
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
            load_parm_cfg();
            task_delay = false;
        }
        else if (scanner_task_tgr(0)) {
            task_ENVMON = false;
            task_SENDER = false;
            task_SCANNER = true;
            task_NUMCAST = false;
            load_parm_cfg();
            task_delay = false;
        }
        else if (numcst_task_tgr(0)) {
            task_ENVMON = false;
            task_SENDER = false;
            task_SCANNER = false;
            task_NUMCAST = true;
            load_parm_cfg();
            task_delay = false;
        }
        else if (envmon_task_tgr(0)) {
            task_ENVMON = true;
            task_SENDER = false;
            task_SCANNER = false;
            task_NUMCAST = false;
            load_parm_cfg();
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
            sl_sleeptimer_delay_millisecond(10);
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
            sl_sleeptimer_delay_millisecond(10);
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
            numcst_task_tgr(-numcst_task_tgr(0));
        }
    }
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
      // Create an advertising set.
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
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
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
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

