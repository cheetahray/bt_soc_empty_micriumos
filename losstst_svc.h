/**
 * @file losstst_svc.h
 * @brief Portable BLE Advertisement Update Module for Silicon Labs
 * 
 * This module provides portable BLE extended advertising functionality
 * that can be adapted from Nordic nRF52 to Silicon Labs platforms.
 */

#ifndef __losstst_svc_H__
#define __losstst_svc_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Silicon Labs SDK headers */
#include "sl_status.h"
#include "sl_bt_api.h"

/* ================== Configuration Constants ================== */

/**
 * Maximum number of advertising sets supported.
 * On Nordic: CONFIG_BT_EXT_ADV_MAX_ADV_SET
 * On Silicon Labs: Adjust according to your configuration
 */
#ifndef MAX_ADV_SETS
#define MAX_ADV_SETS 5
#endif

/**
 * Maximum device name length
 * On Nordic: CONFIG_BT_DEVICE_NAME_MAX
 * On Silicon Labs: Adjust according to your configuration
 */
#ifndef MAX_DEVICE_NAME_LEN
#define MAX_DEVICE_NAME_LEN 30
#endif

/**
 * Default device name
 * On Nordic: CONFIG_BT_DEVICE_NAME
 * On Silicon Labs: Adjust according to your configuration
 */
#ifndef DEFAULT_DEVICE_NAME
#define DEFAULT_DEVICE_NAME "Turnkey LossTest"
#endif

/* ================== Advertising Interval Definitions ================== */

/* Convert milliseconds to BLE interval units (0.625ms per unit) */
#define MS_TO_BLE_INTERVAL(ms) (((unsigned int)(ms) * 16) / 10)

/* Pre-defined advertising intervals from Bluetooth Core Spec 5.4 */
#define VALUE_ADV_INT_MIN_0  30   /* TGAP(adv_fast_interval1) */
#define VALUE_ADV_INT_MAX_0  60
#define PARAM_ADV_INT_MIN_0  MS_TO_BLE_INTERVAL(VALUE_ADV_INT_MIN_0)
#define PARAM_ADV_INT_MAX_0  MS_TO_BLE_INTERVAL(VALUE_ADV_INT_MAX_0)

#define VALUE_ADV_INT_MIN_1  60
#define VALUE_ADV_INT_MAX_1  120
#define PARAM_ADV_INT_MIN_1  MS_TO_BLE_INTERVAL(VALUE_ADV_INT_MIN_1)
#define PARAM_ADV_INT_MAX_1  MS_TO_BLE_INTERVAL(VALUE_ADV_INT_MAX_1)

#define VALUE_ADV_INT_MIN_2  90   /* TGAP(adv_fast_interval1_coded) */
#define VALUE_ADV_INT_MAX_2  180
#define PARAM_ADV_INT_MIN_2  MS_TO_BLE_INTERVAL(VALUE_ADV_INT_MIN_2)
#define PARAM_ADV_INT_MAX_2  MS_TO_BLE_INTERVAL(VALUE_ADV_INT_MAX_2)

#define VALUE_ADV_INT_MIN_3  100  /* TGAP(adv_fast_interval2) */
#define VALUE_ADV_INT_MAX_3  150
#define PARAM_ADV_INT_MIN_3  MS_TO_BLE_INTERVAL(VALUE_ADV_INT_MIN_3)
#define PARAM_ADV_INT_MAX_3  MS_TO_BLE_INTERVAL(VALUE_ADV_INT_MAX_3)

#define VALUE_ADV_INT_MIN_4 200 
#define VALUE_ADV_INT_MAX_4 300 
#define PARAM_ADV_INT_MIN_4 (((unsigned int)VALUE_ADV_INT_MIN_4*16)/10)
#define PARAM_ADV_INT_MAX_4 (((unsigned int)VALUE_ADV_INT_MAX_4*16)/10)

#define VALUE_ADV_INT_MIN_5 300 //BLUETOOTH CORE SPEC 5.4, Vol 3, Part C, p.1377 ; TGAP(adv_fast_interval2_coded)
#define VALUE_ADV_INT_MAX_5 450 //BLUETOOTH CORE SPEC 5.4, Vol 3, Part C, p.1377 ; TGAP(adv_fast_interval2_coded)
#define PARAM_ADV_INT_MIN_5 (((unsigned int)VALUE_ADV_INT_MIN_5*16)/10)
#define PARAM_ADV_INT_MAX_5 (((unsigned int)VALUE_ADV_INT_MAX_5*16)/10)

#define VALUE_ADV_INT_MIN_6 500 
#define VALUE_ADV_INT_MAX_6 650 
#define PARAM_ADV_INT_MIN_6 (((unsigned int)VALUE_ADV_INT_MIN_6*16)/10)
#define PARAM_ADV_INT_MAX_6 (((unsigned int)VALUE_ADV_INT_MAX_6*16)/10)

#define VALUE_ADV_INT_MIN_7 750 
#define VALUE_ADV_INT_MAX_7 950 
#define PARAM_ADV_INT_MIN_7 (((unsigned int)VALUE_ADV_INT_MIN_7*16)/10)
#define PARAM_ADV_INT_MAX_7 (((unsigned int)VALUE_ADV_INT_MAX_7*16)/10)

#define VALUE_ADV_INT_MIN_8 1000 //BLUETOOTH CORE SPEC 5.4, Vol 3, Part C, p.1377 ; TGAP(adv_slow_interval)
#define VALUE_ADV_INT_MAX_8 1200 //BLUETOOTH CORE SPEC 5.4, Vol 3, Part C, p.1377 ; TGAP(adv_slow_interval)
#define PARAM_ADV_INT_MIN_8 (((unsigned int)VALUE_ADV_INT_MIN_8*16)/10)
#define PARAM_ADV_INT_MAX_8 (((unsigned int)VALUE_ADV_INT_MAX_8*16)/10)

#define VALUE_ADV_INT_MIN_9 2000 
#define VALUE_ADV_INT_MAX_9 2400 
#define PARAM_ADV_INT_MIN_9 (((unsigned int)VALUE_ADV_INT_MIN_9*16)/10)
#define PARAM_ADV_INT_MAX_9 (((unsigned int)VALUE_ADV_INT_MAX_9*16)/10)

#define VALUE_ADV_INT_MIN_10 3000 //BLUETOOTH CORE SPEC 5.4, Vol 3, Part C, p.1377 ; TGAP(adv_slow_interval_coded)
#define VALUE_ADV_INT_MAX_10 3600 //BLUETOOTH CORE SPEC 5.4, Vol 3, Part C, p.1377 ; TGAP(adv_slow_interval_coded)
#define PARAM_ADV_INT_MIN_10 (((unsigned int)VALUE_ADV_INT_MIN_10*16)/10)
#define PARAM_ADV_INT_MAX_10 (((unsigned int)VALUE_ADV_INT_MAX_10*16)/10)

/* ================== Platform Selection ================== */
#define PLATFORM_SILABS

/* ================== Advertising Options and Flags ================== */

/* Silicon Labs extended advertiser flags */
#define SL_BT_EXT_ADV_ANONYMOUS         0x1  /* Anonymous advertising */
#define SL_BT_EXT_ADV_INCLUDE_TX_POWER  0x2  /* Include TX power in adv packets */

/* Advertising options - control advertising behavior via bitmask */
#define BT_LE_ADV_OPT_NONE              0
#define BT_LE_ADV_OPT_USE_TX_POWER      (1 << 0)  /* Include TX power in advertising */
#define BT_LE_ADV_OPT_ANONYMOUS         (1 << 1)  /* Anonymous advertising */
#define BT_LE_ADV_OPT_EXT_ADV           (1 << 2)  /* Use extended advertising API */
#define BT_LE_ADV_OPT_NO_2M             (1 << 3)  /* Don't use 2M PHY */
#define BT_LE_ADV_OPT_CODED             (1 << 4)  /* Use Coded PHY (Long Range) */
#define BT_LE_ADV_OPT_USE_IDENTITY      (1 << 5)  /* Use identity address */
#define BT_LE_ADV_OPT_CONNECTABLE       (1 << 6)  /* Connectable advertising */

/* PHY type definitions */
#define SL_BT_GAP_PHY_1M                0x1
#define SL_BT_GAP_PHY_2M                0x2
#define SL_BT_GAP_PHY_CODED             0x4

/* ================== Data Structures ================== */

/**
 * @brief Advertisement status tracking structure
 * Tracks the state of each advertising set
 */
typedef union {
    uint8_t u8_val;
    struct {
        unsigned initialized:1;    /* Advertising set has been created */
        unsigned update_param:1;   /* Parameters have been updated */
        unsigned set_data:1;       /* Advertising data has been set */
        unsigned start:1;          /* Advertising has been started */
        unsigned stop:1;           /* Advertising has been stopped */
    };
} ext_adv_status_t;

/**
 * @brief Device information structure for advertising payload
 * Manufacturer-specific data format
 */
typedef struct __attribute__((__packed__)) {
    uint16_t man_id;      /* Manufacturer ID (0xFFFF) */
    uint16_t form_id;     /* Form ID (0) */
    int16_t  pre_cnt;     /* Packet counter */
    uint16_t flw_cnt;     /* Flow counter (10 heartbeat/min) */
    struct __attribute__((scalar_storage_order("big-endian"))) {
        uint64_t eui_64;  /* Device EUI-64 address */
    };
} device_info_t;

/**
 * @brief Bluetooth v4 compatible device information
 */
typedef struct __attribute__((__packed__)) {
    device_info_t device_info;
    uint8_t tail[10];     /* Additional data (device name) */
} device_info_bt4_t;

/**
 * @brief Number cast information structure
 */
typedef struct __attribute__((__packed__)) {
    uint16_t man_id;              /* Manufacturer ID (0xFFFF) */
    uint16_t form_id;             /* Form ID */
    uint16_t number_cast_form[4]; /* Number cast values */
} numcast_info_t;

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

typedef uint8_t adv_handle_t;  /* Advertising handle (0-based index) */

/* ================== Constants ================== */

#define MANUFACTURER_ID    0xFFFF
#define LOSS_TEST_FORM_ID  0x0000
#define LOSS_TEST_BURST_COUNT 250

/* BLE AD Types */
#define BT_DATA_FLAGS              0x01
#define BT_DATA_TX_POWER           0x0A
#define BT_DATA_NAME_COMPLETE      0x09
#define BT_DATA_MANUFACTURER_DATA  0xFF

/* BLE AD Flags */
#define BT_LE_AD_NO_BREDR          0x04
#define BT_LE_AD_GENERAL           0x02

/* ================== Public Functions ================== */

/**
 * @brief Initialize the advertising port module
 * 
 * This function must be called before any other advertising functions.
 * It initializes internal data structures and prepares the advertising system.
 * 
 * @param device_address Platform-specific device address (e.g., MAC address)
 *                       Pass NULL to use platform default
 * @return 0 on success, negative error code on failure
 */
int losstst_svc_init(const uint8_t *device_address);

/**
 * @brief Update BLE extended advertising set
 * 
 * This is the main function for updating advertising parameters, data, and state.
 * It can be called to:
 *  - Create a new advertising set (first call for an index)
 *  - Update advertising parameters (interval, PHY, etc.)
 *  - Update advertising data payload
 *  - Start/stop advertising with specific parameters
 * 
 * @param index         Advertising set index (0 to MAX_ADV_SETS-1)
 * @param adv_param     Advertising parameters (NULL to keep current)
 * @param adv_data      Advertising data array (NULL for default data)
 * @param adv_start_param Start parameters (NULL for default, timeout/max_events)
 * 
 * @return 0 on success, negative error code on failure
 *         -EINVAL: Invalid index
 *         -ENOTSUP: Feature not supported
 *         Other: Platform-specific error codes
 */
int update_adv(uint8_t index, 
               const adv_param_t *adv_param,
               adv_data_t *adv_data,
               const adv_start_param_t *adv_start_param);

/**
 * @brief Get current advertising status for a specific set
 * 
 * @param index Advertising set index
 * @return Pointer to status structure, NULL if invalid index
 */
const ext_adv_status_t* get_adv_status(uint8_t index);

/**
 * @brief Stop all advertising sets
 * 
 * @return 0 on success, negative error code on failure
 */
int stop_all_advertising(void);

/**
 * @brief Get the device name for a specific advertising set
 * 
 * @param index Advertising set index
 * @return Pointer to device name string, NULL if invalid index
 */
const char* get_adv_device_name(uint8_t index);

/**
 * @brief Set custom device name for an advertising set
 * 
 * @param index Advertising set index
 * @param name Device name (max MAX_DEVICE_NAME_LEN chars)
 * @return 0 on success, negative error code on failure
 */
int set_adv_device_name(uint8_t index, const char *name);

/**
 * @brief Set TX power for a specific advertising set (Silicon Labs)
 * 
 * Use this function to dynamically adjust TX power after advertising set creation.
 * If BT_LE_ADV_OPT_USE_TX_POWER is set in options, TX power will be included in
 * advertising packets.
 * 
 * @param index Advertising set index
 * @param power TX power in 0.1 dBm steps (e.g., 10 = 1 dBm, 55 = 5.5 dBm)
 * @param set_power Pointer to return the actual TX power set
 * @return 0 on success, negative error code on failure
 */
int set_adv_tx_power(uint8_t index, int16_t power, int16_t *set_power);

/**
 * @brief Get Silicon Labs advertiser flags from Nordic-style options
 * 
 * Converts Nordic BT_LE_ADV_OPT_* options to Silicon Labs flags.
 * Used internally by the platform abstraction layer.
 * 
 * @param nordic_options Nordic-style advertising options bitmask
 * @return Silicon Labs extended advertiser flags
 */
uint8_t get_silabs_adv_flags(uint16_t nordic_options);

/**
 * @brief Get PHY settings from Nordic-style options
 * 
 * Extracts primary and secondary PHY from Nordic options like
 * BT_LE_ADV_OPT_NO_2M and BT_LE_ADV_OPT_CODED.
 * 
 * @param nordic_options Nordic-style advertising options bitmask
 * @param primary_phy Output: primary PHY setting
 * @param secondary_phy Output: secondary PHY setting
 */
void get_phy_from_options(uint16_t nordic_options, uint8_t *primary_phy, uint8_t *secondary_phy);

/* ================== Platform-Specific Callbacks ================== */

/**
 * @brief Advertising sent callback (platform-specific)
 * 
 * This callback is invoked when an advertising packet is sent.
 * Platform implementations should call this from their event handlers.
 * 
 * @param adv_handle Advertising handle/set that sent the packet
 * @param num_sent Number of advertising packets sent
 */
void adv_sent_callback(adv_handle_t *adv_handle, uint16_t num_sent);

/**
 * @brief Finalize sender (application-specific)
 * 
 * Application-defined function for sender finalization.
 */
void sender_finit(void);

#ifdef __cplusplus
}
#endif

#endif /* __losstst_svc_H__ */
