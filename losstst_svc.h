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
#include "silabs_nordic_compat.h"

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

/**
 * @brief Quick check if an AD payload contains MANUFACTURER_ID + LOSS_TEST_FORM_ID.
 *
 * Used to filter heartbeat logs so only our own test packets are printed.
 *
 * @param ad_data Pointer to raw advertising data
 * @param ad_len  Length of advertising data
 * @return true if a matching manufacturer-specific record is found
 */
bool losstst_check_form_id(const uint8_t *ad_data, uint16_t ad_len);

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

/**
 * @brief Get task running status (used for UI indicators)
 * @return 0=idle, 1=running (this task), 2=blocked (other task running)
 */
int8_t envmon_task_status(void);
int8_t sender_task_status(void);
int8_t scanner_task_status(void);
int8_t numcst_task_status(void);

/* ================== Configuration Enumeration Functions ================== */

/**
 * @brief Enumerate TX power values
 * 
 * @param dir Direction: 0=current, >0=next (higher power), <0=previous (lower power)
 *            INT8_MAX=highest, INT8_MIN=lowest
 * @return Current TX power value in dBm
 */
int8_t enum_txpower(int8_t dir);

/**
 * @brief Enumerate advertising interval index
 * 
 * @param dir Direction: 0=current, 1=next, -1=previous
 *            INT8_MAX=max index, INT8_MIN=0
 * @return Current interval index (0-based)
 */
uint8_t enum_adv_interval_idx(int8_t dir);

/**
 * @brief Enumerate total packet number index
 * 
 * @param dir Direction: 0=current, 1=next, -1=previous
 *            INT8_MAX=max index, INT8_MIN=0
 * @return Current count index (0-based)
 */
uint8_t enum_totalnum_idx(int8_t dir);

/* ================== Configuration Getter Functions ================== */

/**
 * @brief Get PHY type selection status
 * 
 * @param idx PHY index: 0=2M, 1=1M, 2=Coded(S8), 3=BLE4.x
 * @return true if selected, false otherwise
 */
bool get_cfg_phy_sel(uint8_t idx);

/**
 * @brief Get channel 37 enable status
 * 
 * @return true if channel 37 is enabled
 */
bool get_cfg_ch37(void);

/**
 * @brief Get channel 38 enable status
 * 
 * @return true if channel 38 is enabled
 */
bool get_cfg_ch38(void);

/**
 * @brief Get channel 39 enable status
 * 
 * @return true if channel 39 is enabled
 */
bool get_cfg_ch39(void);

/**
 * @brief Get non-anonymous advertising mode status
 * 
 * @return true if non-anonymous mode is enabled
 */
bool get_cfg_NON_ANONYMOUS(void);

/**
 * @brief Get anonymous advertising mode status
 * 
 * @return true if anonymous mode is active (i.e. non_ANONYMOUS == false)
 */
bool get_cfg_ANONYMOUS(void);

/**
 * @brief Get SoC DC-DC converter power mode
 * 
 * @return 0=LN (LDO only), 1=SW (DC-DC switching), 2=Unknown
 */
int8_t get_soc_dcdc(void);

/**
 * @brief Get 2M PHY selection status
 * @return true if 2M PHY is selected
 */
bool get_cfg_phy2m(void);

/**
 * @brief Get 1M PHY selection status
 * @return true if 1M PHY is selected
 */
bool get_cfg_phy1m(void);

/**
 * @brief Get Coded PHY S=8 selection status
 * @return true if Coded S8 PHY is selected
 */
bool get_cfg_phy8s(void);

/**
 * @brief Get BLE 4.x legacy PHY selection status
 * @return true if BLE4 legacy PHY is selected
 */
bool get_cfg_phyBLEv4(void);

/**
 * @brief Get unicast method status
 * 
 * @return true if unicast method is enabled
 */
bool get_uni_cast_method(void);

/**
 * @brief Get number cast auto mode status
 * 
 * @return 0 = manual mode, 1 = auto mode
 */
bool get_number_cast_auto(void);

/* ================== Resource Accessor Functions ================== */
/* Used by silabs_ext_scr_svc.c as function pointers in resource arrays */

uint8_t sender_id_upper(void);     /**< High byte of sender ID */
uint8_t sender_id_lower(void);     /**< Low byte of sender ID */

uint8_t node_id_upper(void);       /**< High byte of node device address */
uint8_t node_id_lower(void);       /**< Low byte of node device address */

uint8_t numcst_src_id_upper(void); /**< High byte of numcast source node ID */
uint8_t numcst_src_id_lower(void); /**< Low byte of numcast source node ID */

int8_t numcst_rssi_lower(void);    /**< Numcast RSSI lower bound */
int8_t numcst_rssi_upper(void);    /**< Numcast RSSI upper bound */
int8_t numcst_rssi_average(void);  /**< Numcast RSSI average */

/* ================== Configuration Toggle Functions ================== */

bool chg_cfg_phy2m(void);
bool chg_cfg_phy1m(void);
bool chg_cfg_phy8s(void);
bool chg_cfg_phyBLEv4(void);
bool chg_cfg_ANONYMOUS(void);
void chg_uni_cast_method(void);
int chg_soc_dcdc(void);
bool chg_number_cast_auto(void);
int8_t rcv_state_progress(uint8_t idx);
int16_t numcst_setval(uint8_t field, int16_t setval);
int16_t numcst_rxval(uint8_t field);

/* ================== RC Protocol Functions (stubs) ================== */
/* bt_addr_le_t is defined in silabs_nordic_compat.h */
bool rc_msg_outgoing(void *msg_p, size_t sz);
bool rc_rush_msg_outgoing(void *msg_p, size_t sz);
bool rm_msg_outgoing(void *msg_p, size_t sz);
bool rm_rush_msg_outgoing(void *msg_p, size_t sz);
bool set_rc_party(bt_addr_le_t *addr_p);
void clr_rc_party(void);
bool chk_rc_party(void *ptr);

/* ================== Advertising Interval Accessor Functions ================== */
uint16_t adv_interval_lower(uint8_t idx);
uint16_t adv_interval_upper(uint8_t idx);
uint16_t enum_totalnum(int8_t dir);
int8_t   sender_txpower(void);

/* ================== Channel Configuration Toggle Functions ================== */
bool chg_cfg_ch37(void);
bool chg_cfg_ch38(void);
bool chg_cfg_ch39(void);

/* ================== TX/RX Ratio Accessor Functions ================== */
uint16_t xmt_ratio_lower(int idx);
uint16_t xmt_ratio_upper(int idx);
uint16_t rcv_ratio_lower(int idx);
uint16_t rcv_ratio_upper(int idx);

/* ================== RX/Envmon RSSI Accessor Functions ================== */
int8_t rcv_rssi_lower(int idx);
int8_t rcv_rssi_upper(int idx);
int8_t rcv_rssi_average(int idx);
int8_t envmon_rssi_lower(int idx);
int8_t envmon_rssi_upper(int idx);
int8_t envmon_rssi_average(int idx);

/* ================== Statistics Accessor Functions ================== */
uint32_t env_stats_val(int idx);
uint32_t rcv_stats_val(int idx);

/* ================== State Mark Functions ================== */
uint8_t snd_state_mark(int idx);
uint8_t rcv_state_mark(int idx);
bool numcast_phy_mark(uint8_t idx);

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

bool platform_can_yield(void);
void platform_yield(void);                                         

#ifdef __cplusplus
}
#endif

#endif /* __UPDATE_ADV_PORT_H__ */