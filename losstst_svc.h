/**
 * @file update_adv_port.h
 * @brief Portable BLE Advertisement Update Module for Silicon Labs
 * 
 * This module provides portable BLE extended advertising functionality
 * that can be adapted from Nordic nRF52 to Silicon Labs platforms.
 */

#ifndef __UPDATE_ADV_PORT_H__
#define __UPDATE_ADV_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "sl_bt_api.h"

/* ================== Type Definitions ================== */

/**
 * @brief Test parameter structure
 * 
 * This structure contains all parameters needed to configure
 * the BLE test modes (sender, scanner, numcast, envmon).
 */
typedef struct {
    int8_t txpwr;              /**< TX power level configuration index */
    uint8_t interval_idx;      /**< Advertising interval index */
    uint8_t count_idx;         /**< Total count index for sender mode */
    bool phy_2m;               /**< Enable 2M PHY */
    bool phy_1m;               /**< Enable 1M PHY */
    bool phy_s8;               /**< Enable Coded PHY (S=8) */
    bool phy_ble4;             /**< Enable BLE 4.x legacy mode */
    bool ignore_rcv_resp;      /**< Ignore received responses */
    bool inhibit_ch37;         /**< Disable advertising channel 37 */
    bool inhibit_ch38;         /**< Disable advertising channel 38 */
    bool inhibit_ch39;         /**< Disable advertising channel 39 */
    bool non_ANONYMOUS;        /**< Use non-anonymous advertising */
    void *envmon_abort;        /**< Environment monitor abort callback */
    void *sender_abort;        /**< Sender abort callback */
    void *scanner_abort;       /**< Scanner abort callback */
    void *numcast_abort;       /**< Number cast abort callback */
} test_param_t;

/* ================== Platform Abstraction Layer ================== */

/**
 * @brief BLE advertising parameter structures for Silicon Labs BG/MG series
 */

/* TODO: Include appropriate Silicon Labs headers */
/* #include "sl_bt_api.h" */

typedef struct {
    uint8_t  id;            /* Reserved for compatibility (unused) */
    uint8_t  sid;           /* Reserved for periodic advertising (unused) */
    uint8_t  secondary_max_skip; /* Reserved for extended adv optimization (unused) */
    uint32_t interval_min;  /* Advertising interval minimum (0.625ms units) */
    uint32_t interval_max;  /* Advertising interval maximum (0.625ms units) */
    uint8_t  primary_phy;   /* Primary PHY: SL_BT_GAP_PHY_1M or SL_BT_GAP_PHY_CODED */
    uint8_t  secondary_phy; /* Secondary PHY: SL_BT_GAP_PHY_1M, 2M, or CODED */
    uint16_t options;       /* ⭐ KEY FIELD - Controls all advertising behavior:
                             *   BT_LE_ADV_OPT_USE_TX_POWER: Include TX power in adv
                             *   BT_LE_ADV_OPT_ANONYMOUS: Anonymous advertising
                             *   BT_LE_ADV_OPT_EXT_ADV: Use extended advertising
                             *   BT_LE_ADV_OPT_NO_2M: Don't use 2M PHY
                             *   BT_LE_ADV_OPT_CODED: Use Coded PHY (Long Range)
                             *   BT_LE_ADV_OPT_USE_IDENTITY: Use identity address
                             *   BT_LE_ADV_OPT_CONNECTABLE: Connectable advertising
                             */
    void    *peer;          /* Reserved for directed advertising (unused) */
} adv_param_t;

typedef struct {
    uint16_t timeout;       /* Duration in 10ms units (0=continuous) */
    uint16_t num_events;    /* Max number of events (0=no limit) */
} adv_start_param_t;

typedef struct {
    uint8_t type;          /* AD type (flags, name, manufacturer, etc.) */
    uint8_t data_len;      /* Data length */
    const uint8_t *data;   /* Data pointer */
} adv_data_t;

/* ================== Initialization Functions ================== */

/**
 * @brief Initialize BLE loss test service
 * 
 * Must be called once at startup before any other functions.
 * 
 * @return 0 on success, negative error code on failure
 */
int losstst_init(void);

/* ================== Advertising Control Functions ================== */

/**
 * @brief Update advertising parameters and data
 * 
 * @param index Advertising set index (0-4)
 * @param adv_param Advertising parameters (NULL to keep current)
 * @param adv_data Advertising data (NULL to use default)
 * @param adv_start_param Start parameters (NULL to use default)
 * @return 0 on success, negative error code on failure
 */
int update_adv(uint8_t index, 
               const adv_param_t *adv_param,
               adv_data_t *adv_data,
               const adv_start_param_t *adv_start_param);

/**
 * @brief Stop advertising on specified set
 * 
 * @param index Advertising set index (0-3)
 */
void blocking_adv(uint8_t index);

/* ================== Test Mode Setup Functions ================== */

/**
 * @brief Setup sender test mode
 * 
 * @param param Test parameters
 * @return 0 on success, negative error code on failure
 */
int sender_setup(const test_param_t *param);

/**
 * @brief Setup scanner test mode
 * 
 * @param param Test parameters
 * @return 0 on success, negative error code on failure
 */
int scanner_setup(const test_param_t *param);

/**
 * @brief Setup number cast test mode
 * 
 * @param param Test parameters
 * @return 0 on success, negative error code on failure
 */
int numcast_setup(const test_param_t *param);

/**
 * @brief Setup environment monitor test mode
 * 
 * @param param Test parameters
 * @return 0 on success, negative error code on failure
 */
int envmon_setup(const test_param_t *param);

/* ================== Test Mode Execution Functions ================== */

/**
 * @brief Execute sender test iteration
 * 
 * Should be called repeatedly in main loop when sender task is active.
 * 
 * @return >0: continue, 0: finished, <0: error/aborted
 */
int losstst_sender(void);

/**
 * @brief Execute scanner test iteration
 * 
 * Should be called repeatedly in main loop when scanner task is active.
 * 
 * @return >0: continue, 0: finished, <0: error/aborted
 */
int losstst_scanner(void);

/**
 * @brief Execute number cast test iteration
 * 
 * Should be called repeatedly in main loop when numcast task is active.
 * 
 * @return >0: continue, 0: finished, <0: error/aborted
 */
int losstst_numcast(void);

/**
 * @brief Execute environment monitor test iteration
 * 
 * Should be called repeatedly in main loop when envmon task is active.
 * 
 * @return >0: continue, 0: finished, <0: error/aborted
 */
int losstst_envmon(void);

/* ================== Task Trigger Functions ================== */

/**
 * @brief Get/Set sender task trigger
 * 
 * @param set If 0: get current value, if >0: set trigger, if <0: clear trigger
 * @return Current trigger value
 */
int8_t sender_task_tgr(int8_t set);

/**
 * @brief Get/Set scanner task trigger
 * 
 * @param set If 0: get current value, if >0: set trigger, if <0: clear trigger
 * @return Current trigger value
 */
int8_t scanner_task_tgr(int8_t set);

/**
 * @brief Get/Set number cast task trigger
 * 
 * @param set If 0: get current value, if >0: set trigger, if <0: clear trigger
 * @return Current trigger value
 */
int8_t numcst_task_tgr(int8_t set);

/**
 * @brief Get/Set environment monitor task trigger
 * 
 * @param set If 0: get current value, if >0: set trigger, if <0: clear trigger
 * @return Current trigger value
 */
int8_t envmon_task_tgr(int8_t set);

/* ================== Utility Functions ================== */

/**
 * @brief Advertising sent event handler
 * 
 * This function should be called from your BLE event handler when
 * an advertising timeout/completion event occurs.
 * 
 * In Silicon Labs, call this from sl_bt_on_event() when receiving:
 * - sl_bt_evt_advertiser_timeout_id
 * 
 * Nordic equivalent: loss_tst_sent_cb (automatic callback)
 * 
 * @param adv_handle Advertising handle that completed sending
 */
void losstst_adv_sent_handler(uint8_t adv_handle);

/**
 * @brief Finalize sender (application-specific)
 * 
 * Application-defined function for sender finalization.
 */
void sender_finit(void);

void sl_bt_scanner_process_legacy_report(const bd_addr *addr, int8_t rssi,
                                        const uint8_t *ad_data, uint16_t ad_len);

void sl_bt_scanner_process_extended_report(const bd_addr *addr, int8_t rssi,
                                         int8_t tx_power, uint8_t primary_phy,
                                         uint8_t secondary_phy,
                                         const uint8_t *ad_data, uint16_t ad_len);

#ifdef __cplusplus
}
#endif

#endif /* __UPDATE_ADV_PORT_H__ */