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

/* ================== TX Power Control ================== */

/**
 * @brief Set TX power for advertising sets
 * 
 * This function sets the transmit power for advertising on all advertising sets.
 * The actual power level may be adjusted by the platform to the nearest supported value.
 * 
 * @param tx_power_dbm Requested TX power in dBm (e.g., -40 to +8)
 * @param num_handles Number of advertising handles to configure (0-4)
 * @return 0 on success, negative error code on failure
 *         -EINVAL: Invalid parameters
 *         -ENOTSUP: Feature not supported
 */
int set_adv_tx_power(int8_t tx_power_dbm, uint8_t num_handles);

/* ================== Scanner Control ================== */

/**
 * @brief Start passive scanning with specific PHY
 * 
 * This function starts passive BLE scanning to discover advertising devices.
 * The scan method determines which PHY to use for scanning.
 * 
 * @param method Scan method:
 *               0: All PHYs (1M + Coded if available)
 *               1: 1M PHY only
 *               2: Coded PHY only
 *               -1: Stop scanning
 * @return 0 on success, negative error code on failure
 *         -EINVAL: Invalid method
 *         -ENOTSUP: Feature not supported
 */
int passive_scan_control(int8_t method);

/**
 * @brief Stop passive scanning
 * 
 * @return 0 on success, negative error code on failure
 */
int stop_passive_scan(void);

/* ================== Test Setup Functions ================== */

/**
 * @brief Test parameter structure for setup functions
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

/**
 * @brief Setup device as sender for packet loss test
 * 
 * This function configures the device to act as a transmitter
 * in a BLE packet loss test scenario. It initializes advertising
 * parameters, TX power, PHY selection, and starts advertising.
 * 
 * @param param Test parameters structure
 * @return 0 on success, negative error code on failure
 */
int sender_setup(const test_param_t *param);

/**
 * @brief Setup device as scanner/receiver for packet loss test
 * 
 * This function configures the device to act as a receiver
 * in a BLE packet loss test scenario. It initializes scanning
 * parameters and starts passive scanning.
 * 
 * @param param Test parameters structure
 * @return 0 on success, negative error code on failure
 */
int scanner_setup(const test_param_t *param);

/**
 * @brief Setup device for number casting mode
 * 
 * This function configures the device to broadcast numeric values
 * using BLE advertising. Useful for broadcasting sensor data or
 * status information.
 * 
 * @param param Test parameters structure
 * @return 0 on success, negative error code on failure
 */
int numcast_setup(const test_param_t *param);

/**
 * @brief Setup device for environment monitoring mode
 * 
 * This function configures the device for passive environment
 * monitoring, typically stopping all advertising and enabling
 * scanning to monitor BLE traffic.
 * 
 * @param param Test parameters structure
 * @return 0 on success, negative error code on failure
 */
int envmon_setup(const test_param_t *param);

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
    } eui;                /* EUI-64 in big-endian format */
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

/**
 * @brief Task type identifiers for mutual exclusion
 * 
 * These constants identify different test modes. The task management
 * system ensures only one task type runs at a time.
 */
extern const int8_t sender_tgr;   /**< Sender mode task ID (1) */
extern const int8_t scanner_tgr;  /**< Scanner mode task ID (2) */
extern const int8_t numcst_tgr;   /**< Number cast mode task ID (3) */
extern const int8_t envmon_tgr;   /**< Environment monitor task ID (4) */

/**
 * @brief Set or check sender task trigger
 * 
 * @param set Task control:
 *            - Positive value: Start sender task (if no other task running)
 *            - Negative value: Stop sender task (if currently running)
 *            - Zero: Query current task status
 * @return Current active task ID (0 = no task, 1 = sender, 2 = scanner, etc.)
 */
int8_t sender_task_tgr(int8_t set);

/**
 * @brief Get sender task status
 * 
 * @return 0 = idle, 1 = sender running, 2 = blocked by another task
 */
int8_t sender_task_status(void);

/**
 * @brief Set or check scanner task trigger
 * 
 * @param set Task control (positive=start, negative=stop, zero=query)
 * @return Current active task ID
 */
int8_t scanner_task_tgr(int8_t set);

/**
 * @brief Get scanner task status
 * 
 * @return 0 = idle, 1 = scanner running, 2 = blocked by another task
 */
int8_t scanner_task_status(void);

/**
 * @brief Set or check number cast task trigger
 * 
 * @param set Task control (positive=start, negative=stop, zero=query)
 * @return Current active task ID
 */
int8_t numcst_task_tgr(int8_t set);

/**
 * @brief Get number cast task status
 * 
 * @return 0 = idle, 1 = numcast running, 2 = blocked by another task
 */
int8_t numcst_task_status(void);

/**
 * @brief Set or check environment monitor task trigger
 * 
 * @param set Task control (positive=start, negative=stop, zero=query)
 * @return Current active task ID
 */
int8_t envmon_task_tgr(int8_t set);

/**
 * @brief Get environment monitor task status
 * 
 * @return 0 = idle, 1 = envmon running, 2 = blocked by another task
 */
int8_t envmon_task_status(void);

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
//void adv_sent_callback(adv_handle_t *adv_handle, uint16_t num_sent);

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
int update_adv_port_init(const uint8_t *device_address);

int complete_system_init(void);

/* ================== Status Message Generation (Application Layer) ================== */

/**
 * @brief Test statistics structure for peek messages
 * 
 * This structure holds runtime statistics for generating
 * status broadcast messages.
 */
typedef struct {
    uint16_t sub_total_snd_2m;    /**< Total packets sent on 2M PHY */
    uint16_t sub_total_snd_1m;    /**< Total packets sent on 1M PHY */
    uint16_t sub_total_snd_s8;    /**< Total packets sent on Coded PHY */
    uint16_t sub_total_snd_ble4;  /**< Total packets sent on BLE 4.x */
    uint16_t sub_total_rcv[4];    /**< Total packets received per PHY */
    uint16_t round_total_num;      /**< Target total packet count */
    int8_t round_tx_pwr;           /**< Current TX power (dBm) */
    bool round_phy_sel[4];         /**< PHY selection flags */
} peek_stats_t;

/**
 * @brief Reception statistics structure
 * 
 * Per-PHY reception statistics for scanner peek messages.
 */
typedef struct {
    uint16_t  node;
	uint8_t  pri_phy;
	uint8_t  sec_phy;
	int8_t   tx_pwr;
	uint16_t flow;
	uint16_t subtotal;
	int16_t  rssi;
	int16_t  rssi_upper;
	int16_t  rssi_lower;
	unsigned det_sender:1;
	unsigned dump_rcvinfo:1;
	unsigned complete:1;
	unsigned notified:1;
} recv_stats_t;

typedef struct __attribute__((__packed__)) {
	recv_stats_t rec;
	int rssi_acc;
	int rssi_idx;
} rcv_stamp_t;

/**
 * @brief Generate sender status messages
 * 
 * This function generates status broadcast messages for sender mode,
 * including packet counts, PHY types, and TX power information.
 * The generated messages are stored in peek_msg_str[] buffers.
 * 
 * Requires application-provided global variables:
 * - sub_total_snd_2m, sub_total_snd_1m, sub_total_snd_s8, sub_total_snd_ble4
 * - round_phy_sel[4]
 * - round_total_num
 * - round_tx_pwr
 * - device_address[0]
 */
void sender_peek_msg(void);

/**
 * @brief Generate scanner status messages
 * 
 * This function generates status broadcast messages for scanner mode,
 * including reception counts, RSSI ranges, and remote TX power.
 * 
 * Requires application-provided global variables:
 * - rec_sets[4]
 * - sub_total_rcv[4]
 * - peek_rcv_rssi[4][3]
 * - remote_tx_pwr[4]
 */
void scanner_peek_msg(void);

/**
 * @brief Get pointer to peek message buffer
 * 
 * Returns the internal peek message buffer for a specific PHY index.
 * These buffers contain the formatted status messages.
 * 
 * @param index PHY index (0=2M, 1=1M, 2=Coded, 3=BLE4)
 * @return Pointer to message buffer (64 bytes), NULL if invalid index
 */
const char* get_peek_msg_buffer(uint8_t index);
