/**
 * @file losstst_svc.c
 * @brief Portable BLE Advertisement Update Module for Silicon Labs
 * 
 * This module provides the implementation of portable BLE extended advertising
 * functionality that can be adapted from Nordic nRF52 to Silicon Labs platforms.
 */

#include "losstst_svc.h"
#include "ble_log.h"
#include "lcd_ui.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <stdlib.h>

/* Silicon Labs SDK headers */
#include "sl_status.h"
#include "gatt_db.h"
#include "sl_sleeptimer.h"

/* CMSIS-RTOS2 headers for task management */
#include "cmsis_os2.h"

/* ================== Utility Macros ================== */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

/* ================== Debug Configuration ================== */
#define CHK_UPDATE_ADV_PROCEDURE 1  /* Set to 1 to enable debug prints */

#if CHK_UPDATE_ADV_PROCEDURE
    /* Output to BLE if connected, otherwise to UART */
    #define DEBUG_PRINT(fmt, ...) do { \
        if (ble_log_is_connected()) { \
            BLE_PRINTF(fmt, ##__VA_ARGS__); \
        } else { \
            printf(fmt, ##__VA_ARGS__); \
        } \
    } while(0)
#else
    /* Always output to UART when debug is disabled */
    #define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

/* ================== Advertising Options Presets ================== */
/* Pre-defined advertising option combinations for common use cases */
#define BT4_ADV_OPT_CLR_MASK (BT_LE_ADV_OPT_USE_TX_POWER | BT_LE_ADV_OPT_ANONYMOUS | \
                              BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_NO_2M | BT_LE_ADV_OPT_CODED)

#define ADV_OPT_IDX_0 (BT_LE_ADV_OPT_NONE \
    | BT_LE_ADV_OPT_USE_TX_POWER | BT_LE_ADV_OPT_ANONYMOUS | BT_LE_ADV_OPT_EXT_ADV \
    )

#define ADV_OPT_IDX_1 (BT_LE_ADV_OPT_NONE \
    | BT_LE_ADV_OPT_USE_TX_POWER | BT_LE_ADV_OPT_ANONYMOUS | BT_LE_ADV_OPT_EXT_ADV \
    | BT_LE_ADV_OPT_NO_2M \
    )

#define ADV_OPT_IDX_2 (BT_LE_ADV_OPT_NONE \
    | BT_LE_ADV_OPT_USE_TX_POWER | BT_LE_ADV_OPT_ANONYMOUS | BT_LE_ADV_OPT_EXT_ADV \
    | BT_LE_ADV_OPT_NO_2M \
    | BT_LE_ADV_OPT_CODED \
    )

#define ADV_OPT_IDX_3 (BT_LE_ADV_OPT_NONE \
    | BT_LE_ADV_OPT_USE_IDENTITY \
    )

/* ================== Internal State Variables ================== */
#define FIXED_BT_LE_ADV_PARAM(_id, _options, _int_min, _int_max, _peer) \
	&((const adv_param_t) { \
		.id = _id, \
		.sid = 0, \
		.secondary_max_skip = 0, \
		.options = (_options), \
		.interval_min = (_int_min), \
		.interval_max = (_int_max), \
		.peer = (_peer), \
	})

/**
 * Helper to initialize extended advertising start parameters inline
 *
 * @param _timeout Advertiser timeout
 * @param _n_evts  Number of advertising events
 */
#define BT_LE_EXT_ADV_START_PARAM_INIT(_timeout, _n_evts) \
{ \
	.timeout = (_timeout), \
	.num_events = (_n_evts), \
}

/**
 * Helper to declare extended advertising start parameters inline
 *
 * @param _timeout Advertiser timeout
 * @param _n_evts  Number of advertising events
 */
#define BT_LE_EXT_ADV_START_PARAM(_timeout, _n_evts) \
	((const adv_start_param_t[]) { \
		BT_LE_EXT_ADV_START_PARAM_INIT((_timeout), (_n_evts)) \
	})

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

/* ================== Platform Selection ================== */
#define PLATFORM_SILABS

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

typedef uint8_t adv_handle_t;  /* Advertising handle (0-based index) */

static bool round_phy_sel[4]={true,false,false,false};
static bool sndr_abort_flag[4]={false,false,false,false};
static const adv_param_t *non_connectable_adv_param_x[][4] ={
	{	// group 0
		//IDX_0, IDX_1, IDX_2, IDX_3
		 FIXED_BT_LE_ADV_PARAM(1,ADV_OPT_IDX_0, PARAM_ADV_INT_MIN_0, PARAM_ADV_INT_MAX_0, NULL)
		,FIXED_BT_LE_ADV_PARAM(2,ADV_OPT_IDX_1, PARAM_ADV_INT_MIN_0, PARAM_ADV_INT_MAX_0, NULL)
		,FIXED_BT_LE_ADV_PARAM(3,ADV_OPT_IDX_2, PARAM_ADV_INT_MIN_0, PARAM_ADV_INT_MAX_0, NULL)
		,FIXED_BT_LE_ADV_PARAM(4,ADV_OPT_IDX_3, PARAM_ADV_INT_MIN_0, PARAM_ADV_INT_MAX_0, NULL)},
	{	// group 1
		//IDX_0, IDX_1, IDX_2, IDX_3
		 FIXED_BT_LE_ADV_PARAM(1,ADV_OPT_IDX_0, PARAM_ADV_INT_MIN_1, PARAM_ADV_INT_MAX_1, NULL)
		,FIXED_BT_LE_ADV_PARAM(2,ADV_OPT_IDX_1, PARAM_ADV_INT_MIN_1, PARAM_ADV_INT_MAX_1, NULL)
		,FIXED_BT_LE_ADV_PARAM(3,ADV_OPT_IDX_2, PARAM_ADV_INT_MIN_1, PARAM_ADV_INT_MAX_1, NULL)
		,FIXED_BT_LE_ADV_PARAM(4,ADV_OPT_IDX_3, PARAM_ADV_INT_MIN_1, PARAM_ADV_INT_MAX_1, NULL)},
	{	// group 2
		//IDX_0, IDX_1, IDX_2, IDX_3
		 FIXED_BT_LE_ADV_PARAM(1,ADV_OPT_IDX_0, PARAM_ADV_INT_MIN_2, PARAM_ADV_INT_MAX_2, NULL)
		,FIXED_BT_LE_ADV_PARAM(2,ADV_OPT_IDX_1, PARAM_ADV_INT_MIN_2, PARAM_ADV_INT_MAX_2, NULL)
		,FIXED_BT_LE_ADV_PARAM(3,ADV_OPT_IDX_2, PARAM_ADV_INT_MIN_2, PARAM_ADV_INT_MAX_2, NULL)
		,FIXED_BT_LE_ADV_PARAM(4,ADV_OPT_IDX_3, PARAM_ADV_INT_MIN_2, PARAM_ADV_INT_MAX_2, NULL)},
	{	// group 3
		//IDX_0, IDX_1, IDX_2, IDX_3
		 FIXED_BT_LE_ADV_PARAM(1,ADV_OPT_IDX_0, PARAM_ADV_INT_MIN_3, PARAM_ADV_INT_MAX_3, NULL)
		,FIXED_BT_LE_ADV_PARAM(2,ADV_OPT_IDX_1, PARAM_ADV_INT_MIN_3, PARAM_ADV_INT_MAX_3, NULL)
		,FIXED_BT_LE_ADV_PARAM(3,ADV_OPT_IDX_2, PARAM_ADV_INT_MIN_3, PARAM_ADV_INT_MAX_3, NULL)
		,FIXED_BT_LE_ADV_PARAM(4,ADV_OPT_IDX_3, PARAM_ADV_INT_MIN_3, PARAM_ADV_INT_MAX_3, NULL)},
	{	// group 4
		//IDX_0, IDX_1, IDX_2, IDX_3
		 FIXED_BT_LE_ADV_PARAM(1,ADV_OPT_IDX_0, PARAM_ADV_INT_MIN_4, PARAM_ADV_INT_MAX_4, NULL)
		,FIXED_BT_LE_ADV_PARAM(2,ADV_OPT_IDX_1, PARAM_ADV_INT_MIN_4, PARAM_ADV_INT_MAX_4, NULL)
		,FIXED_BT_LE_ADV_PARAM(3,ADV_OPT_IDX_2, PARAM_ADV_INT_MIN_4, PARAM_ADV_INT_MAX_4, NULL)
		,FIXED_BT_LE_ADV_PARAM(4,ADV_OPT_IDX_3, PARAM_ADV_INT_MIN_4, PARAM_ADV_INT_MAX_4, NULL)},
	{	// group 5
		//IDX_0, IDX_1, IDX_2, IDX_3
		 FIXED_BT_LE_ADV_PARAM(1,ADV_OPT_IDX_0, PARAM_ADV_INT_MIN_5, PARAM_ADV_INT_MAX_5, NULL)
		,FIXED_BT_LE_ADV_PARAM(2,ADV_OPT_IDX_1, PARAM_ADV_INT_MIN_5, PARAM_ADV_INT_MAX_5, NULL)
		,FIXED_BT_LE_ADV_PARAM(3,ADV_OPT_IDX_2, PARAM_ADV_INT_MIN_5, PARAM_ADV_INT_MAX_5, NULL)
		,FIXED_BT_LE_ADV_PARAM(4,ADV_OPT_IDX_3, PARAM_ADV_INT_MIN_5, PARAM_ADV_INT_MAX_5, NULL)},
	{	// group 6
		//IDX_0, IDX_1, IDX_2, IDX_3
		 FIXED_BT_LE_ADV_PARAM(1,ADV_OPT_IDX_0, PARAM_ADV_INT_MIN_6, PARAM_ADV_INT_MAX_6, NULL)
		,FIXED_BT_LE_ADV_PARAM(2,ADV_OPT_IDX_1, PARAM_ADV_INT_MIN_6, PARAM_ADV_INT_MAX_6, NULL)
		,FIXED_BT_LE_ADV_PARAM(3,ADV_OPT_IDX_2, PARAM_ADV_INT_MIN_6, PARAM_ADV_INT_MAX_6, NULL)
		,FIXED_BT_LE_ADV_PARAM(4,ADV_OPT_IDX_3, PARAM_ADV_INT_MIN_6, PARAM_ADV_INT_MAX_6, NULL)},
	{	// group 7
		//IDX_0, IDX_1, IDX_2, IDX_3
		 FIXED_BT_LE_ADV_PARAM(1,ADV_OPT_IDX_0, PARAM_ADV_INT_MIN_7, PARAM_ADV_INT_MAX_7, NULL)
		,FIXED_BT_LE_ADV_PARAM(2,ADV_OPT_IDX_1, PARAM_ADV_INT_MIN_7, PARAM_ADV_INT_MAX_7, NULL)
		,FIXED_BT_LE_ADV_PARAM(3,ADV_OPT_IDX_2, PARAM_ADV_INT_MIN_7, PARAM_ADV_INT_MAX_7, NULL)
		,FIXED_BT_LE_ADV_PARAM(4,ADV_OPT_IDX_3, PARAM_ADV_INT_MIN_7, PARAM_ADV_INT_MAX_7, NULL)},
	{	// group 8
		//IDX_0, IDX_1, IDX_2, IDX_3
		 FIXED_BT_LE_ADV_PARAM(1,ADV_OPT_IDX_0, PARAM_ADV_INT_MIN_8, PARAM_ADV_INT_MAX_8, NULL)
		,FIXED_BT_LE_ADV_PARAM(2,ADV_OPT_IDX_1, PARAM_ADV_INT_MIN_8, PARAM_ADV_INT_MAX_8, NULL)
		,FIXED_BT_LE_ADV_PARAM(3,ADV_OPT_IDX_2, PARAM_ADV_INT_MIN_8, PARAM_ADV_INT_MAX_8, NULL)
		,FIXED_BT_LE_ADV_PARAM(4,ADV_OPT_IDX_3, PARAM_ADV_INT_MIN_8, PARAM_ADV_INT_MAX_8, NULL)},
	{	// group 9
		//IDX_0, IDX_1, IDX_2, IDX_3
		 FIXED_BT_LE_ADV_PARAM(1,ADV_OPT_IDX_0, PARAM_ADV_INT_MIN_9, PARAM_ADV_INT_MAX_9, NULL)
		,FIXED_BT_LE_ADV_PARAM(2,ADV_OPT_IDX_1, PARAM_ADV_INT_MIN_9, PARAM_ADV_INT_MAX_9, NULL)
		,FIXED_BT_LE_ADV_PARAM(3,ADV_OPT_IDX_2, PARAM_ADV_INT_MIN_9, PARAM_ADV_INT_MAX_9, NULL)
		,FIXED_BT_LE_ADV_PARAM(4,ADV_OPT_IDX_3, PARAM_ADV_INT_MIN_9, PARAM_ADV_INT_MAX_9, NULL)},
	{	// group 10
		//IDX_0, IDX_1, IDX_2, IDX_3
		 FIXED_BT_LE_ADV_PARAM(1,ADV_OPT_IDX_0, PARAM_ADV_INT_MIN_10, PARAM_ADV_INT_MAX_10, NULL)
		,FIXED_BT_LE_ADV_PARAM(2,ADV_OPT_IDX_1, PARAM_ADV_INT_MIN_10, PARAM_ADV_INT_MAX_10, NULL)
		,FIXED_BT_LE_ADV_PARAM(3,ADV_OPT_IDX_2, PARAM_ADV_INT_MIN_10, PARAM_ADV_INT_MAX_10, NULL)
		,FIXED_BT_LE_ADV_PARAM(4,ADV_OPT_IDX_3, PARAM_ADV_INT_MIN_10, PARAM_ADV_INT_MAX_10, NULL)}
};
static uint32_t adv_param_mask[2];
static const adv_start_param_t p_adv_default_start_param[]=BT_LE_EXT_ADV_START_PARAM(0, 0);
static const adv_start_param_t p_adv_finit_start_param[]=BT_LE_EXT_ADV_START_PARAM(300,0);
static const adv_start_param_t p_adv_1sec_start_param[]=BT_LE_EXT_ADV_START_PARAM(100,0);
static const adv_start_param_t p_adv_5sec_start_param[]=BT_LE_EXT_ADV_START_PARAM(500,0);
static const adv_start_param_t p_adv_burst_start_param[]=BT_LE_EXT_ADV_START_PARAM(0,LOSS_TEST_BURST_COUNT);

const int8_t sender_tgr=1;
const int8_t scanner_tgr=2;
const int8_t numcst_tgr=3;
const int8_t envmon_tgr=4;

static int8_t losstst_task_val;

/* Initialization flag */
static bool svc_init_success = false;

/* Number of advertising sets configured */
static uint8_t num_adv_set = MAX_ADV_SETS;

/* Device address storage (8 bytes for EUI-64) */
static uint8_t device_address[8] = {0};

/* Advertising set status tracking */
static ext_adv_status_t ext_adv_status[MAX_ADV_SETS] = {
    {.u8_val = 0}, {.u8_val = 0}, {.u8_val = 0}, 
    {.u8_val = 0}, {.u8_val = 0}
};

/* RSSI tracking structures */
typedef struct {
    int64_t expired_tm;
    int8_t rssi;
} rssi_stamp_t;

/* Device names for each advertising set */
static char adv_dev_nm[MAX_ADV_SETS][MAX_DEVICE_NAME_LEN + 1];

/* Stored advertising parameters for each set (needed for start with options) */
static adv_param_t stored_adv_params[MAX_ADV_SETS];

/* Advertising data payloads */
static device_info_t device_info_form[4] = {
    {MANUFACTURER_ID, LOSS_TEST_FORM_ID, INT16_MIN, 255},
    {MANUFACTURER_ID, LOSS_TEST_FORM_ID, INT16_MIN, 255},
    {MANUFACTURER_ID, LOSS_TEST_FORM_ID, INT16_MIN, 255},
    {MANUFACTURER_ID, LOSS_TEST_FORM_ID, INT16_MIN, 255}
};

static device_info_bt4_t device_info_bt4_form = {
    .device_info = {MANUFACTURER_ID, LOSS_TEST_FORM_ID, INT16_MIN, 255}
};

static device_info_bt4_t numcast_bt4_form;
static device_info_t remote_resp_form[4]={{.man_id=0},{.man_id=0},{.man_id=0},{.man_id=0}};

static numcast_info_t numcast_info_form = {MANUFACTURER_ID, LOSS_TEST_FORM_ID};
static uint16_t *const p_number_cast_form = (uint16_t *)&numcast_info_form.number_cast_form;

/* Advertising data sets for each advertising set */
static adv_data_t ratio_test_data_set[MAX_ADV_SETS][8];
static adv_data_t number_cast_data_set[2][4];

/* Common advertising flags */
static const uint8_t p_common_adv_flags[] = {BT_LE_AD_NO_BREDR};

/* Peek message strings (for detailed status advertising) */
static char peek_msg_str[4][64];

/* Platform-specific advertising handles */
static adv_handle_t ext_adv[MAX_ADV_SETS];

/* PHY type string mappings */
static const char *pri_phy_typ[] = {
    "NA",  /* 0x00 */
    "1M",  /* 0x01 */
    "NA",  /* 0x02 */
    "S8",  /* 0x03 Coded PHY S=8 */
    "S2",  /* 0x04 Coded PHY S=2 */
    "NA"   /* 0x05 */
};

static const char *sec_phy_typ[] = {
    "NA",  /* 0x00 */
    "1M",  /* 0x01 */
    "2M",  /* 0x02 */
    "S8",  /* 0x03 Coded PHY S=8 */
    "S2",  /* 0x04 Coded PHY S=2 */
    "NA"   /* 0x05 */
};

/* Message format strings */
static const char *peek_sndpkt_form = "\\xff\\xffSND:%03u P:%s/%s R:%u/%u T:%d";
static const char *peek_sndpkt_btv4_form = "\\xff\\xffSND:%03u P:%s%s R:%u/%u T:%d";
static const char *peek_rcvpkt_form = "\\xff\\xffRCV:%03u P:%s/%s R:%u/%u S:%s(%s..%s) T:%s";
static const char *peek_rcvpkt_btv4_form = "\\xff\\xffRCV:%03u P:%s%s R:%u/%u S:%s(%s..%s) T:%s";
/* Log format strings */
static const char *log_sender_form = "SENDER:%03u P:%s/%s R:%d/%u S:%s(%s..%s) T:%s";
static const char *log_rcvpkt_form = "RCV:%03u P:%s/%s R:%d/%u S:%s(%s..%s) T:%s";

uint16_t sub_total_snd_2m, sub_total_snd_1m, sub_total_snd_s8, sub_total_snd_ble4;
uint16_t round_total_num;
int8_t round_tx_pwr;

recv_stats_t rec_sets[4];
uint16_t sub_total_rcv[4];
int8_t peek_rcv_rssi[4][3];
int8_t remote_tx_pwr[4];

uint16_t round_total_num;       /**< Target total packet count */
int8_t round_tx_pwr;            /**< Current TX power (d
ner mode variables (required by scanner_peek_msg) */
recv_stats_t rec_sets[4];       /**< Reception statistics for 4 PHYs */
uint16_t sub_total_rcv[4];      /**< Total packets received per PHY */
int8_t peek_rcv_rssi[4][3];     /**< RSSI [PHY][cur/min/max] */
int8_t remote_tx_pwr[4];        /**< Remote device TX power per 
tional variables required by setup functions */
uint8_t round_adv_param_index;  /**< Advertising parameter index */
uint16_t enum_total_num[]={500,1000,2000,5000,10000,20000,50000};   /**< Total packet count enumeration */
bool ignore_rcv_resp;           /**< Ignore received responses flag */
bool inhibit_ch37;              /**< Inhibit advertising channel 37 */
bool inhibit_ch38;              /**< Inhibit advertising channel 38 */
bool inhibit_ch39;              /**< Inhibit advertising channel 39 */
bool non_ANONYMOUS;             /**< Non-anonymous advertising flag */
bool scanner_inactive;          /**< Scanner inactive flag */
uint64_t number_cast_val;       /**< Number cast value */
uint64_t number_cast_rxval;     /**< Number cast received value */
bool number_cast_auto;          /**< Number cast auto mode flag */
static int16_t precnt_rcv[4];
static rssi_stamp_t env_rssi_rec[4][256];

typedef bool (*envmon_task_abort)(void);
typedef bool (*sender_task_abort)(void);
typedef bool (*scanner_task_abort)(void);
typedef bool (*numcast_task_abort)(void);
//typedef void (*txpower_setup)(int8_t);
static envmon_task_abort envmon_abort_p;
static sender_task_abort sender_abort_p;
static scanner_task_abort scanner_abort_p;
static numcast_task_abort numcast_abort_p;
static int8_t env_rssi[4][3];

/* Silicon Labs advertisement info structure (equivalent to Nordic's bt_hci_evt_le_ext_advertising_info) */
typedef struct {
    int8_t rssi;            /* RSSI value in dBm */
    int8_t tx_power;        /* TX power in dBm */
    uint8_t prim_phy;       /* Primary PHY: 1=1M, 3=Coded */
    uint8_t sec_phy;        /* Secondary PHY: 0=none(legacy), 1=1M, 2=2M, 3=Coded */
    uint8_t address_type;   /* Address type */
    bd_addr address;        /* Bluetooth device address */
} sl_adv_info_t;

/* Device found parameter structure for parser callbacks */
typedef struct {
    uint16_t flw_cnt;
    union {
        struct {
            unsigned step_flag :2;
            unsigned step_special_stream :3;
            unsigned step_devnm :2;
            unsigned :7;
            unsigned step_fail :1;
            unsigned step_success :1;
        };
        struct {
            unsigned :14;
            unsigned step_completed:2;
        };
        uint16_t step_raw;
    };
    sl_adv_info_t *adv_info_p;  /* Platform-independent advertisement info pointer */
    const void *temp_ptr;       /* Temporary pointer for data processing (preserves const) */
} dev_found_param_t;

volatile bool ack_remote_resp[4];  // 远程响应ACK
// scr input val
static uint16_t xmt_ratio_val[4][2];
static uint16_t rcv_ratio_val[4][2];
static int8_t rcv_rssi_val[4][3];
static int8_t rcv_state_val[4];
static int8_t snd_state_val[4];
static uint32_t rcv_stats[4];
static uint32_t env_stats[4];
static uint16_t sndr_id;
static int8_t sndr_txpower;
static char rcv_msg_str[3][80];
static int64_t numcst_rssi_rec_tm;
static rssi_stamp_t numcst_rssi_rec[32];
static int8_t numcst_rssi[3];
static int64_t numcst_phy_stamp_tm[4] = {0, 0, 0, 0};
static uint8_t numcst_src_node[2] = {0, 0};
static uint16_t numcst_rssi_idx = 0;
static rcv_stamp_t rcv_stamp[4];  /* Current burst state */
static char rssi_str[3][5];
static char tx_pwr_str[5];
static dev_found_param_t dev_chr;  /* Device found parser state */
static uint32_t env_rssi_idx[4];   /* Environment RSSI index per PHY */

static const uint16_t value_interval[][2]={
	{VALUE_ADV_INT_MIN_0, VALUE_ADV_INT_MAX_0}, //BLUETOOTH CORE SPEC 5.4, Vol 3, Part C, p.1376 ; TGAP(adv_fast_interval1)
	{VALUE_ADV_INT_MIN_1, VALUE_ADV_INT_MAX_1}, 
	{VALUE_ADV_INT_MIN_2, VALUE_ADV_INT_MAX_2}, //BLUETOOTH CORE SPEC 5.4, Vol 3, Part C, p.1376 ; TGAP(adv_fast_interval1_coded)
	{VALUE_ADV_INT_MIN_3, VALUE_ADV_INT_MAX_3}, //BLUETOOTH CORE SPEC 5.4, Vol 3, Part C, p.1377 ; TGAP(adv_fast_interval2)
	{VALUE_ADV_INT_MIN_4, VALUE_ADV_INT_MAX_4},
	{VALUE_ADV_INT_MIN_5, VALUE_ADV_INT_MAX_5}, //BLUETOOTH CORE SPEC 5.4, Vol 3, Part C, p.1377 ; TGAP(adv_fast_interval2_coded)
	{VALUE_ADV_INT_MIN_6, VALUE_ADV_INT_MAX_6},
	{VALUE_ADV_INT_MIN_7, VALUE_ADV_INT_MAX_7},
	{VALUE_ADV_INT_MIN_8, VALUE_ADV_INT_MAX_8}, //BLUETOOTH CORE SPEC 5.4, Vol 3, Part C, p.1377 ; TGAP(adv_slow_interval)
	{VALUE_ADV_INT_MIN_9, VALUE_ADV_INT_MAX_9},
	{VALUE_ADV_INT_MIN_10,VALUE_ADV_INT_MAX_10} //BLUETOOTH CORE SPEC 5.4, Vol 3, Part C, p.1377 ; TGAP(adv_slow_interval_coded)
	//,{VALUE_ADV_INT_MIN_n1, VALUE_ADV_INT_MAX_n1}
	//,{VALUE_ADV_INT_MIN_n2, VALUE_ADV_INT_MAX_n2}
	//,{VALUE_ADV_INT_MIN_n3, VALUE_ADV_INT_MAX_n3}
};

int64_t platform_uptime_get(void) {
    // 使用64位 tick 计数（避免溢出）
    uint64_t ticks = sl_sleeptimer_get_tick_count64();
    uint64_t ms = 0;
    sl_sleeptimer_tick64_to_ms(ticks, &ms);
    return (int64_t)ms;
}

/**
 * @brief Check if platform can yield to other tasks
 * 
 * Determines if the current execution context allows yielding control
 * to other tasks. Yielding is only safe in task context, not in ISR.
 * 
 * @return true if can yield (in task context), false otherwise (in ISR)
 */
static bool platform_can_yield(void) {
    /* Check if RTOS kernel is running */
    osKernelState_t state = osKernelGetState();
    if (state != osKernelRunning) {
        return false;
    }
    
    /* In CMSIS-RTOS2, we cannot directly check interrupt nesting.
     * A simple heuristic: if we can get the current thread, we're in task context.
     * If osThreadGetId() returns NULL, we're likely in ISR or before scheduler start.
     */
    return (osThreadGetId() != NULL);
}

/**
 * @brief Yield CPU to other tasks
 * 
 * Voluntarily gives up the CPU to allow other ready tasks of the same
 * or higher priority to execute. This is useful in busy-wait loops
 * to prevent blocking lower-priority tasks.
 */
static void platform_yield(void) {
    /* Use CMSIS-RTOS2 thread yield */
    osThreadYield();
}

/* ================== Platform Abstraction Functions ================== */

/**
 * @brief Helper: Convert RSSI value to string
 * 
 * @param rssi RSSI value (-128 to 127)
 * @param str_p Output string buffer (must be at least 5 bytes)
 * @return Pointer to string (empty if invalid RSSI)
 */
static char* rssi_toa(int16_t rssi, char *str_p)
{
    if (str_p == NULL) {
        static char empty[] = "\\0";
        return empty;
    }
    
    /* Check for invalid RSSI (out of int8_t range) */
    if (rssi >= INT8_MAX || rssi <= INT8_MIN) {
        *str_p = '\0';
    } else {
        /* Convert to string (max 5 bytes: "-128" + null) */
        snprintf(str_p, 8, "%d", (int)rssi);
    }
    
    return str_p;
}

/**
 * @brief Helper: Convert TX power value to string
 * 
 * @param pwr TX power value (-128 to 127)
 * @param str_p Output string buffer (must be at least 5 bytes)
 * @return Pointer to string (empty if invalid power)
 */
static char* txpwr_toa(int8_t pwr, char *str_p)
{
    if (str_p == NULL) {
        static char empty[] = "\0";
        return empty;
    }
    
    /* Check for invalid power (INT8_MAX used as "not set") */
    if (pwr == INT8_MAX) {
        *str_p = '\0';
    } else {
        /* Convert to string (max 5 bytes: "-128" + null) */
        snprintf(str_p, 8, "%d", (int)pwr);
    }
    
    return str_p;
}

/**
 * @brief Calculate advertising channel map based on inhibit flags
 * 
 * @param inhibit_ch37 true to disable channel 37
 * @param inhibit_ch38 true to disable channel 38
 * @param inhibit_ch39 true to disable channel 39
 * @return Channel map value (bit 0=ch37, bit 1=ch38, bit 2=ch39)
 */
static uint8_t get_adv_channel_map(bool inhibit_ch37, bool inhibit_ch38, bool inhibit_ch39)
{
    uint8_t channel_map = 0x07;  /* Default: all channels (37, 38, 39) */
    
    /* Clear bits for inhibited channels */
    if (inhibit_ch37) {
        channel_map &= ~0x01;  /* Clear bit 0 (channel 37) */
    }
    if (inhibit_ch38) {
        channel_map &= ~0x02;  /* Clear bit 1 (channel 38) */
    }
    if (inhibit_ch39) {
        channel_map &= ~0x04;  /* Clear bit 2 (channel 39) */
    }
    
    /* Ensure at least one channel is enabled */
    if (channel_map == 0) {
        channel_map = 0x07;  /* Fall back to all channels */
        DEBUG_PRINT("Warning: All channels inhibited, using all channels\n");
    }
    
    return channel_map;
}

/**
 * @brief Convert advertising options to Silicon Labs flags
 */
uint8_t get_silabs_adv_flags(uint16_t nordic_options)
{
    uint8_t sl_flags = 0;
    
    /* Map BT_LE_ADV_OPT_ANONYMOUS to SL_BT_EXT_ADV_ANONYMOUS */
    if (nordic_options & BT_LE_ADV_OPT_ANONYMOUS) {
        sl_flags |= SL_BT_EXT_ADV_ANONYMOUS;
    }
    
    /* Map BT_LE_ADV_OPT_USE_TX_POWER to SL_BT_EXT_ADV_INCLUDE_TX_POWER */
    if (nordic_options & BT_LE_ADV_OPT_USE_TX_POWER) {
        sl_flags |= SL_BT_EXT_ADV_INCLUDE_TX_POWER;
    }
    
    return sl_flags;
}

/**
 * @brief Extract PHY settings from Nordic options
 */
void get_phy_from_options(uint16_t nordic_options, uint8_t *primary_phy, uint8_t *secondary_phy)
{
    /* Default PHY values */
    *primary_phy = SL_BT_GAP_PHY_1M;
    *secondary_phy = SL_BT_GAP_PHY_2M;
    
    /* Check for NO_2M option - means use only 1M or Coded PHY */
    if (nordic_options & BT_LE_ADV_OPT_NO_2M) {
        *secondary_phy = SL_BT_GAP_PHY_1M;
    }
    
    /* Check for CODED option - use Coded PHY */
    if (nordic_options & BT_LE_ADV_OPT_CODED) {
        *primary_phy = SL_BT_GAP_PHY_CODED;
        *secondary_phy = SL_BT_GAP_PHY_CODED;
    }
}

/* ================== Silicon Labs Platform Implementations ================== */

/**
 * @brief Create and configure advertising set based on options
 * 
 * This function processes all options and configures
 * the Silicon Labs advertising set accordingly.
 */
static int platform_create_adv_set(const adv_param_t *param, 
                                   adv_handle_t *handle)
{
        sl_status_t status;
        uint8_t primary_phy, secondary_phy;
        bool use_extended_adv;
        bool use_legacy_adv;
        
        // Step 1: Create advertising set
        status = sl_bt_advertiser_create_set(handle);
        if (status != SL_STATUS_OK) {
            return -EIO;
        }
        
        // Step 2: Set timing (interval)
        status = sl_bt_advertiser_set_timing(*handle, 
                                             param->interval_min,
                                             param->interval_max,
                                             0, 0);
        if (status != SL_STATUS_OK) {
            return -EIO;
        }
        
        // Step 3: Process options to determine advertising type
        use_extended_adv = (param->options & BT_LE_ADV_OPT_EXT_ADV) != 0;
        use_legacy_adv = (param->options & BT_LE_ADV_OPT_USE_IDENTITY) != 0 && 
                        !(param->options & BT_LE_ADV_OPT_EXT_ADV);
        
        if (use_extended_adv) {
            // Extended advertising path
            
            // Get PHY settings from options
            get_phy_from_options(param->options, &primary_phy, &secondary_phy);
            
            // Set PHY
            status = sl_bt_extended_advertiser_set_phy(*handle, 
                                                       primary_phy,
                                                       secondary_phy);
            if (status != SL_STATUS_OK) {
                return -EIO;
            }
            
            // Note: Flags (anonymous, tx_power) will be applied when starting advertising
            
        } else if (use_legacy_adv) {
            // Legacy (BT4) advertising - no PHY setting needed
            // Clear any random address to use identity address
            sl_bt_advertiser_clear_random_address(*handle);
        }
        
        // Note: TX power should be set using set_adv_tx_power() function if needed
        
        return 0;
        
    }
    
    /**
     * @brief Update advertising parameters
     * 
     * Re-configures advertising set with new parameters.
     * Must stop advertising before calling this.
     */
    static int platform_update_adv_param(adv_handle_t handle,
                                         const adv_param_t *param)
    {
        sl_status_t status;
        uint8_t primary_phy, secondary_phy;
        bool use_extended_adv;
        
        // Update timing
        status = sl_bt_advertiser_set_timing(handle,
                                             param->interval_min,
                                             param->interval_max,
                                             0, 0);
        if (status != SL_STATUS_OK) {
            return -EIO;
        }
        
        // Update PHY if using extended advertising
        use_extended_adv = (param->options & BT_LE_ADV_OPT_EXT_ADV) != 0;
        if (use_extended_adv) {
            get_phy_from_options(param->options, &primary_phy, &secondary_phy);
            
            status = sl_bt_extended_advertiser_set_phy(handle,
                                                       primary_phy,
                                                       secondary_phy);
            if (status != SL_STATUS_OK) {
                return -EIO;
            }
        }
        
        // Note: TX power should be updated using set_adv_tx_power() function if needed
        
        return 0;
        
    }
    
    static int platform_set_adv_data(adv_handle_t handle,
                                     adv_data_t *data, uint8_t data_len)
    {
        // Example Silicon Labs implementation:
        sl_status_t status;
        uint8_t adv_data[256];
        uint16_t offset = 0;
        
        // Build advertising data packet
        for (uint8_t i = 0; i < data_len; i++) {
            adv_data[offset++] = data[i].data_len + 1;  // Length
            adv_data[offset++] = data[i].type;          // Type
            memcpy(&adv_data[offset], data[i].data, data[i].data_len);
            offset += data[i].data_len;
        }
        
        status = sl_bt_extended_advertiser_set_data(handle, offset, adv_data);
        return (status == SL_STATUS_OK) ? 0 : -EIO;
        
    }
    
    /**
     * @brief Start advertising with configured parameters
     * 
     * Uses the advertising set configuration and starts advertising.
     * Needs access to options to determine extended vs legacy and flags.
     */
    static int platform_start_adv(adv_handle_t handle,
                                  //const adv_start_param_t *param,
                                  uint16_t options)
    {
        sl_status_t status;
        bool use_extended_adv = (options & BT_LE_ADV_OPT_EXT_ADV) != 0;
        bool is_connectable = (options & BT_LE_ADV_OPT_CONNECTABLE) != 0;
        uint8_t sl_flags;
        
        if (use_extended_adv) {
            // Extended advertising
            sl_flags = get_silabs_adv_flags(options);
            
            uint8_t connection_mode = is_connectable ? 
                sl_bt_extended_advertiser_connectable :
                sl_bt_extended_advertiser_non_connectable;
            
            status = sl_bt_extended_advertiser_start(handle,
                                                     connection_mode,
                                                     sl_flags);
        } else {
            // Legacy advertising
            uint8_t connection_mode = is_connectable ?
                sl_bt_legacy_advertiser_connectable :
                sl_bt_legacy_advertiser_non_connectable;
                
            status = sl_bt_legacy_advertiser_start(handle, connection_mode);
        }
        
        return (status == SL_STATUS_OK) ? 0 : -EIO;
        
    }
    
    static int platform_stop_adv(adv_handle_t handle)
    {
        sl_status_t status = sl_bt_advertiser_stop(handle);
        return (status == SL_STATUS_OK) ? 0 : -EIO;
        
    }

/* ================== Public API Implementation ================== */

int losstst_svc_init(const uint8_t *device_addr)
{
    if (svc_init_success) {
        return 0;  /* Already initialized */
    }
    
    /* Store device address */
    if (device_addr != NULL) {
        memcpy(device_address, device_addr, sizeof(device_address));
    } else {
        /* Use default address if none provided */
        /* On Nordic: Would read from NRF_FICR->DEVICEADDR */
        /* On Silicon Labs: Would read from system/gecko_configuration */
        memset(device_address, 0x00, sizeof(device_address));
    }
    
    /* Initialize device names (will be set on first use) */
    memset(adv_dev_nm, 0, sizeof(adv_dev_nm));
    
    /* Initialize advertising data sets */
    memset(ratio_test_data_set, 0, sizeof(ratio_test_data_set));
    memset(number_cast_data_set, 0, sizeof(number_cast_data_set));
    
    /* Set initialization flag */
    svc_init_success = true;
    
    return 0;
}

/**
 * @brief Internal helper to initialize device names
 */
static void init_device_names(void)
{
    if (adv_dev_nm[0][0] != '\0') {
        return;  /* Already initialized */
    }
    
    /* Generate device names based on device address */
    uint8_t node_id = device_address[0];
    
    /* Read device name from GATT database */
    char gatt_device_name[32] = {0};
    size_t gatt_name_len = 0;
    sl_status_t status = sl_bt_gatt_server_read_attribute_value(
        gattdb_device_name,
        0,
        sizeof(gatt_device_name) - 1,
        &gatt_name_len,
        (uint8_t*)gatt_device_name
    );
    
    /* Use GATT device name if available, otherwise use default */
    const char *base_name = (status == SL_STATUS_OK && gatt_name_len > 0) 
                            ? gatt_device_name 
                            : "Turnkey LossTest";
    
    snprintf(adv_dev_nm[0], sizeof(adv_dev_nm[0]), "LossTst(%03u)", node_id);
    snprintf(adv_dev_nm[1], sizeof(adv_dev_nm[1]), "LossTst(%03u)", node_id);
    snprintf(adv_dev_nm[2], sizeof(adv_dev_nm[2]), "LossTst(%03u)", node_id);
    snprintf(adv_dev_nm[3], sizeof(adv_dev_nm[3]), "LossTst%03u", node_id);
    /* Limit base_name to 19 chars to fit "(PEEK 999)" in 31-byte buffer */
    snprintf(adv_dev_nm[4], sizeof(adv_dev_nm[4]), "%.19s(PEEK %03u)", base_name, node_id);
    
    /* Initialize number cast form */
    memcpy(p_number_cast_form, device_address, 
           sizeof(numcast_info_form.number_cast_form));
    for (int idx = 0; idx < 4; idx++) {
        p_number_cast_form[idx] %= 1000;
    }
    
    /* Setup default advertising data structures */
    
    /* Number cast data set 0 */
    number_cast_data_set[0][0] = (adv_data_t){
        .type = BT_DATA_FLAGS,
        .data_len = 1,
        .data = p_common_adv_flags
    };
    number_cast_data_set[0][1] = (adv_data_t){
        .type = BT_DATA_MANUFACTURER_DATA,
        .data_len = sizeof(device_info_t),
        .data = (const uint8_t *)&device_info_form[0]
    };
    number_cast_data_set[0][2] = (adv_data_t){
        .type = BT_DATA_MANUFACTURER_DATA,
        .data = (uint8_t *)&numcast_info_form,
        .data_len = sizeof(numcast_info_form)
    };
    
    /* Number cast data set 1 (BLE v4 compatible) */
    number_cast_data_set[1][0] = (adv_data_t){
        .type = BT_DATA_FLAGS,
        .data_len = 1,
        .data = p_common_adv_flags
    };
    number_cast_data_set[1][1] = (adv_data_t){
        .type = BT_DATA_MANUFACTURER_DATA,
        .data_len = sizeof(device_info_bt4_t),
        .data = (const uint8_t *)&device_info_bt4_form
    };
}

/**
 * @brief Internal helper to prepare default advertising data
 */
static void prepare_default_adv_data(uint8_t index)
{
    if (index <= 2) {
        /* PHY test data (1M, 2M, Coded) */
        ratio_test_data_set[index][0] = (adv_data_t){
            .type = BT_DATA_FLAGS,
            .data_len = 1,
            .data = p_common_adv_flags
        };
        ratio_test_data_set[index][1] = (adv_data_t){
            .type = BT_DATA_MANUFACTURER_DATA,
            .data_len = sizeof(device_info_t),
            .data = (const uint8_t *)&device_info_form[index]
        };
        ratio_test_data_set[index][2] = (adv_data_t){
            .type = BT_DATA_NAME_COMPLETE,
            .data = (uint8_t *)adv_dev_nm[index],
            .data_len = strlen(adv_dev_nm[index])
        };
    } else if (index == 3) {
        /* BLE v4 compatible data */
        ratio_test_data_set[index][0] = (adv_data_t){
            .type = BT_DATA_FLAGS,
            .data_len = 1,
            .data = p_common_adv_flags
        };
        ratio_test_data_set[index][1] = (adv_data_t){
            .type = BT_DATA_MANUFACTURER_DATA,
            .data_len = sizeof(device_info_bt4_t),
            .data = (const uint8_t *)&device_info_bt4_form
        };
        memcpy((uint8_t *)device_info_bt4_form.tail, 
               adv_dev_nm[3], 
               sizeof(device_info_bt4_form.tail));
    } else if (index == 4) {
        /* Peek/status message data */
        ratio_test_data_set[4][0] = (adv_data_t){
            .type = BT_DATA_FLAGS,
            .data_len = 1,
            .data = p_common_adv_flags
        };
        for (int i = 0; i < 4; i++) {
            ratio_test_data_set[4][i+1] = (adv_data_t){
                .type = BT_DATA_MANUFACTURER_DATA,
                .data_len = strlen(peek_msg_str[i]),
                .data = (uint8_t *)peek_msg_str[i]
            };
        }
        ratio_test_data_set[4][5] = (adv_data_t){
            .type = BT_DATA_NAME_COMPLETE,
            .data = (uint8_t *)adv_dev_nm[4],
            .data_len = strlen(adv_dev_nm[4])
        };
    }
}

int update_adv(uint8_t index, 
               const adv_param_t *adv_param,
               adv_data_t *adv_data,
               const adv_start_param_t *adv_start_param)
{
    int retval = 0;
    int err;
    
    /* Validate initialization */
    if (!svc_init_success) {
        DEBUG_PRINT("%s: Not initialized\n", __func__);
        return -EPERM;
    }
    
    /* Validate index */
    if (index >= num_adv_set) {
        DEBUG_PRINT("%s: Invalid index %d\n", __func__, index);
        return -EINVAL;
    }
    
    /* Initialize device names on first use */
    init_device_names();
    
    /* ========== Create advertising set if needed ========== */
    if (!ext_adv_status[index].initialized) {
        /* Set appropriate default parameters based on index */
        adv_param_t default_param = {
            .id = index,
            .sid = 0,
            .secondary_max_skip = 0,
            .interval_min = PARAM_ADV_INT_MIN_0,
            .interval_max = PARAM_ADV_INT_MAX_0,
            .primary_phy = SL_BT_GAP_PHY_1M,
            .secondary_phy = SL_BT_GAP_PHY_2M,
            .options = BT_LE_ADV_OPT_EXT_ADV,  /* Default to extended adv */
            .peer = NULL
        };
        
        /* Store parameters for later use */
        memcpy(&stored_adv_params[index], &default_param, sizeof(adv_param_t));
        
        err = platform_create_adv_set(&default_param, &ext_adv[index]);
        if (err) {
            DEBUG_PRINT("%s LN%d: Create failed, err %d\n", 
                       __func__, __LINE__, err);
            return err;
        }
        
        ext_adv_status[index].initialized = 1;
        ext_adv_status[index].update_param = 1;
    }
    
    /* ========== Update advertising parameters if provided ========== */
    if (adv_param != NULL) {
        /* Stop advertising before updating parameters */
        if (ext_adv_status[index].update_param) {
            platform_stop_adv(ext_adv[index]);
        }
        
        /* Store new parameters */
        memcpy(&stored_adv_params[index], adv_param, sizeof(adv_param_t));
        
        err = platform_update_adv_param(ext_adv[index], adv_param);
        if (err) {
            DEBUG_PRINT("%s LN%d: Update param failed, err %d\n",
                       __func__, __LINE__, err);
            if (retval == 0) retval = err;
        }
        ext_adv_status[index].update_param = 1;
    }
    
    /* ========== Set advertising data ========== */
    if (adv_data != NULL) {
        /* Count number of data elements */
        uint8_t ad_len = 0;
        while (ad_len < 8 && (adv_data + ad_len)->data_len != 0) {
            ad_len++;
        }
        
        err = platform_set_adv_data(ext_adv[index], adv_data, ad_len);
        if (err) {
            DEBUG_PRINT("%s LN%d: Set data failed, err %d\n",
                       __func__, __LINE__, err);
            if (retval == 0) retval = err;
        }
        ext_adv_status[index].set_data = 1;
    } else {
        /* Use default advertising data */
        prepare_default_adv_data(index);
        
        uint8_t ad_len = 0;
        if (index <= 2) {
            ad_len = 3;
        } else if (index == 3) {
            ad_len = 2;
        } else if (index == 4) {
            ad_len = 6;
        }
        
        err = platform_set_adv_data(ext_adv[index], 
                                    ratio_test_data_set[index], 
                                    ad_len);
        if (err) {
            DEBUG_PRINT("%s LN%d: Set default data failed, err %d\n",
                       __func__, __LINE__, err);
            if (retval == 0) retval = err;
        }
        ext_adv_status[index].set_data = 1;
    }
    
    /* ========== Reset start/stop state if both are set ========== */
    if (ext_adv_status[index].start && ext_adv_status[index].stop) {
        ext_adv_status[index].start = 0;
        ext_adv_status[index].stop = 0;
    }
    
    /* ========== Start advertising ========== */
    if (!ext_adv_status[index].start || adv_start_param != NULL) {
        // const adv_start_param_t *start_param = adv_start_param 
        //                                      ? adv_start_param 
        //                                      : &p_adv_default_start_param;
        
        /* Silicon Labs needs options to determine extended/legacy and flags */
        err = platform_start_adv(ext_adv[index],// start_param, 
                                stored_adv_params[index].options);
        
        if (err) {
            DEBUG_PRINT("%s LN%d: Start adv failed, err %d\n",
                       __func__, __LINE__, err);
            if (retval == 0) retval = err;
        } else {
            ext_adv_status[index].start = 1;
        }
    }
    
    return retval;
}

const ext_adv_status_t* get_adv_status(uint8_t index)
{
    if (index >= num_adv_set) {
        return NULL;
    }
    return &ext_adv_status[index];
}

int stop_all_advertising(void)
{
    int retval = 0;
    
    for (uint8_t i = 0; i < num_adv_set; i++) {
        if (ext_adv_status[i].initialized && ext_adv_status[i].start) {
            int err = platform_stop_adv(ext_adv[i]);
            if (err && retval == 0) {
                retval = err;
            }
            ext_adv_status[i].stop = 1;
            ext_adv_status[i].start = 0;
        }
    }
    
    return retval;
}

const char* get_adv_device_name(uint8_t index)
{
    if (index >= MAX_ADV_SETS) {
        return NULL;
    }
    return adv_dev_nm[index];
}

int set_adv_device_name(uint8_t index, const char *name)
{
    if (index >= MAX_ADV_SETS || name == NULL) {
        return -EINVAL;
    }
    
    strncpy(adv_dev_nm[index], name, MAX_DEVICE_NAME_LEN);
    adv_dev_nm[index][MAX_DEVICE_NAME_LEN] = '\0';
    
    return 0;
}

// void adv_sent_callback(adv_handle_t *adv_handle, uint16_t num_sent)
// {
//     /* Platform-specific callback handler */
//     /* This can be used to track advertising packet transmission */
//     DEBUG_PRINT("Adv sent: %d packets\n", num_sent);
// }

/* ================== TX Power Control Implementation ================== */

int set_adv_tx_power(int8_t tx_power_dbm, uint8_t num_handles)
{
    if (!svc_init_success) {
        return -EAGAIN;
    }
    
    if (num_handles > MAX_ADV_SETS) {
        return -EINVAL;
    }
    
    int err = 0;
    
    /* Silicon Labs implementation using system API */
    /* Example implementation: */
    sl_status_t status;
    int16_t set_min, set_max;
    
    // Convert dBm to 0.1dBm units for Silicon Labs
    set_min = tx_power_dbm * 10;
    set_max = tx_power_dbm * 10;
    
    // Set TX power for advertising
    status = sl_bt_system_set_tx_power(
        set_min,
        set_max,
        &set_min,  // Returns actual min set
        &set_max   // Returns actual max set
    );
    
    if (status != SL_STATUS_OK) {
        return -EIO;
    }
    
    DEBUG_PRINT("TX Power set: requested=%d.%ddBm, actual=%d.%ddBm\n",
               tx_power_dbm, 0, set_max / 10, set_max % 10);
    
    
    return err;
}

/* ================== Scanner Control Implementation ================== */

int passive_scan_control(int8_t method)
{
    if (!svc_init_success) {
        return -EAGAIN;
    }
    
    static int8_t scan_method = -1;
    int err = 0;
    
    /* BLE scan timing parameters (in units of 0.625ms) */
    #define BT_GAP_SCAN_FAST_INTERVAL       0x0060  /* 96 = 60ms */
    #define BT_GAP_SCAN_FAST_WINDOW         0x0060  /* 96 = 60ms (100% duty cycle) */
    #define BT_GAP_SCAN_FAST_INTERVAL_CODED 0x0120  /* 288 = 180ms */
    #define BT_GAP_SCAN_FAST_WINDOW_CODED   0x0090  /* 144 = 90ms (50% duty cycle) */
    
    /* Silicon Labs implementation using sl_bt_scanner API */
    sl_status_t status;
    uint8_t scanning_phy;
    uint16_t scan_interval;
    uint16_t scan_window;
    
    if (method < 0) {
        /* Stop scanning */
        status = sl_bt_scanner_stop();
        scan_method = -1;
        return (status == SL_STATUS_OK) ? 0 : -EIO;
    }
    
    if (method != scan_method) {
        /* Stop current scanning */
        sl_bt_scanner_stop();
        
        /* Determine PHY and timing parameters */
        switch (method) {
            case 0:  /* All PHYs - standard timing */
                scanning_phy = sl_bt_scanner_scan_phy_1m_and_coded;
                scan_interval = BT_GAP_SCAN_FAST_INTERVAL;
                scan_window = BT_GAP_SCAN_FAST_WINDOW;
                break;
            
            case 1:  /* 1M only - standard timing */
                scanning_phy = sl_bt_scanner_scan_phy_1m;
                scan_interval = BT_GAP_SCAN_FAST_INTERVAL;
                scan_window = BT_GAP_SCAN_FAST_WINDOW;
                break;
            
            case 2:  /* Coded only - standard timing */
                scanning_phy = sl_bt_scanner_scan_phy_coded;
                scan_interval = BT_GAP_SCAN_FAST_INTERVAL;
                scan_window = BT_GAP_SCAN_FAST_WINDOW;
                break;
            
            case 3:  /* Numcast mode - all PHYs with longer interval */
                scanning_phy = sl_bt_scanner_scan_phy_1m_and_coded;
                scan_interval = BT_GAP_SCAN_FAST_INTERVAL_CODED;
                scan_window = BT_GAP_SCAN_FAST_WINDOW_CODED;
                break;
            
            default:
                scanning_phy = sl_bt_scanner_scan_phy_1m;
                scan_interval = BT_GAP_SCAN_FAST_INTERVAL;
                scan_window = BT_GAP_SCAN_FAST_WINDOW;
                break;
        }
        
        /* Set scan timing parameters */
        status = sl_bt_scanner_set_parameters(
            sl_bt_scanner_scan_mode_passive,
            scan_interval,
            scan_window
        );
        
        if (status != SL_STATUS_OK) {
            DEBUG_PRINT("Failed to set scan parameters: 0x%04X\n", status);
            return -EIO;
        }
        
        /* Start scanning */
        status = sl_bt_scanner_start(
            scanning_phy,
            sl_bt_scanner_discover_observation  /* Passive scanning */
        );
        
        if (status != SL_STATUS_OK) {
            DEBUG_PRINT("Scanning failed (err 0x%04X)\n", (unsigned int)status);
            return -EIO;
        }
        
        scan_method = method;
    }

    return err;
}

int stop_passive_scan(void)
{
    return passive_scan_control(-1);
}

void sender_peek_msg(void)
{
    if (!svc_init_success) {
        return;
    }
    
    /* Generate message for 2M PHY (index 0) */
    snprintf(peek_msg_str[0], sizeof(peek_msg_str[0]), peek_sndpkt_form,
            device_address[0],
            pri_phy_typ[1], sec_phy_typ[2],  /* 1M/2M */
            sub_total_snd_2m,
            (round_phy_sel[0]) ? round_total_num : 0,
            round_tx_pwr);
    *(uint16_t *)(peek_msg_str[0]) = MANUFACTURER_ID;
    
    /* Generate message for 1M PHY (index 1) */
    snprintf(peek_msg_str[1], sizeof(peek_msg_str[1]), peek_sndpkt_form,
            device_address[0],
            pri_phy_typ[1], sec_phy_typ[1],  /* 1M/1M */
            sub_total_snd_1m,
            (round_phy_sel[1]) ? round_total_num : 0,
            round_tx_pwr);
    *(uint16_t *)(peek_msg_str[1]) = MANUFACTURER_ID;
    
    /* Generate message for Coded PHY (index 2) */
    snprintf(peek_msg_str[2], sizeof(peek_msg_str[2]), peek_sndpkt_form,
            device_address[0],
            pri_phy_typ[3], sec_phy_typ[3],  /* S8/S8 */
            sub_total_snd_s8,
            (round_phy_sel[2]) ? round_total_num : 0,
            round_tx_pwr);
    *(uint16_t *)(peek_msg_str[2]) = MANUFACTURER_ID;
    
    /* Generate message for BLE 4.x (index 3) */
    snprintf(peek_msg_str[3], sizeof(peek_msg_str[3]), peek_sndpkt_btv4_form,
            device_address[0],
            "BLE", "v4",
            sub_total_snd_ble4,
            (round_phy_sel[3]) ? round_total_num : 0,
            round_tx_pwr);
    *(uint16_t *)(peek_msg_str[3]) = MANUFACTURER_ID;
}

/* ================== Test Setup Functions Implementation ================== */

/* 
 * Note: These setup functions depend on application-specific global variables
 * and callbacks. The implementations below provide the portable platform-specific
 * operations. Applications must provide the following:
 * 
 * - Global variables: round_tx_pwr, round_phy_sel[], device_info_form[], etc.
 * - Callback functions: sender_peek_msg(), scanner_peek_msg(), etc.
 * - Helper functions: get_txpower_sv(), get_txpower_effect(), blocking_adv(), etc.
 * 
 * The implementations here show the structure and platform-specific calls.
 */

int sender_setup(const test_param_t *param)
{
    if (!svc_init_success || param == NULL) {
        return -EINVAL;
    }
    
    int err = 0;
    
    /* Initialize application layer variables */
    round_tx_pwr = param->txpwr;
    round_adv_param_index = param->interval_idx;
    round_phy_sel[0] = param->phy_2m;
    round_phy_sel[1] = param->phy_1m;
    round_phy_sel[2] = param->phy_s8;
    round_phy_sel[3] = param->phy_ble4;
    
    /* Reset device info structures */
    device_info_form[0].pre_cnt = INT16_MIN;
    device_info_form[0].flw_cnt = 0;
    device_info_form[1].pre_cnt = INT16_MIN;
    device_info_form[1].flw_cnt = 0;
    device_info_form[2].pre_cnt = INT16_MIN;
    device_info_form[2].flw_cnt = 0;
    device_info_form[3].pre_cnt = INT16_MIN;
    device_info_form[3].flw_cnt = 0;
    device_info_bt4_form.device_info = device_info_form[3];
    
    /* Reset transmission counters */
    sub_total_snd_2m = 0;
    sub_total_snd_1m = 0;
    sub_total_snd_s8 = 0;
    sub_total_snd_ble4 = 0;
    
    /* Initialize total packet count from parameter */
    uint16_t total_num = enum_total_num[param->count_idx];
    round_total_num = total_num;
    
    /* Setup transmission ratio values */
    memset(xmt_ratio_val, 0, sizeof(xmt_ratio_val));
    if (round_phy_sel[0]) xmt_ratio_val[0][1] = total_num;
    if (round_phy_sel[1]) xmt_ratio_val[1][1] = total_num;
    if (round_phy_sel[2]) xmt_ratio_val[2][1] = total_num;
    if (round_phy_sel[3]) xmt_ratio_val[3][1] = total_num;
    
    /* Set other config flags */
    ignore_rcv_resp = param->ignore_rcv_resp;
    inhibit_ch37 = param->inhibit_ch37;
    inhibit_ch38 = param->inhibit_ch38;
    inhibit_ch39 = param->inhibit_ch39;
    non_ANONYMOUS = param->non_ANONYMOUS;
    
    /* Store abort callbacks */
    envmon_abort_p = param->envmon_abort;
    sender_abort_p = param->sender_abort;
    scanner_abort_p = param->scanner_abort;
    numcast_abort_p = param->numcast_abort;
    
    /* Print task startup banner */
    DEBUG_PRINT("Packet Loss Test (node %03u) **** SND SIDE ****\n", device_address[0]);
    
    /* Generate initial status message */
    sender_peek_msg();
    
    /* Set TX power for all advertising handles */
    err = set_adv_tx_power(param->txpwr, 4);
    if (err) {
        DEBUG_PRINT("sender_setup: TX power set failed: %d\n", err);
        return err;
    }
    
    /* Build advertising parameter masks based on configuration flags */
    /* adv_param_mask[0]: bits to clear (using & ~mask) */
    /* adv_param_mask[1]: bits to set (using | mask) */
    adv_param_mask[0] = (non_ANONYMOUS) ? BT_LE_ADV_OPT_ANONYMOUS : 0;
    adv_param_mask[1] = ((non_ANONYMOUS) ? BT_LE_ADV_OPT_USE_IDENTITY : 0);
    
    /* Calculate advertising channel map based on inhibit flags */
    /* Note: Channel disabling is done via sl_bt_advertiser_set_channel_map() below */
    uint8_t channel_map = get_adv_channel_map(inhibit_ch37, inhibit_ch38, inhibit_ch39);
    
    /* Start advertising on selected PHYs with modified parameters if needed */
    /* PHY index mapping: 0=2M, 1=1M, 2=Coded(S8), 3=BLE4 */
    if (param->phy_2m) {
        if (adv_param_mask[0] != 0 || adv_param_mask[1] != 0) {
            const adv_param_t *base_param = non_connectable_adv_param_x[round_adv_param_index][0];
            adv_param_t work_adv_param = *base_param;
            work_adv_param.options |= adv_param_mask[1];
            work_adv_param.options &= ~adv_param_mask[0];
            err = update_adv(0, &work_adv_param, NULL, NULL);
        } else {
            err = update_adv(0, NULL, NULL, NULL);
        }
        if (err) {
            DEBUG_PRINT("sender_setup: PHY 2M adv failed\n");
        } else {
            /* Set channel map for this advertising set */
            sl_bt_advertiser_set_channel_map(ext_adv[0], channel_map);
        }
    }
    
    if (param->phy_1m) {
        if (adv_param_mask[0] != 0 || adv_param_mask[1] != 0) {
            const adv_param_t *base_param = non_connectable_adv_param_x[round_adv_param_index][1];
            adv_param_t work_adv_param = *base_param;
            work_adv_param.options |= adv_param_mask[1];
            work_adv_param.options &= ~adv_param_mask[0];
            err = update_adv(1, &work_adv_param, NULL, NULL);
        } else {
            err = update_adv(1, NULL, NULL, NULL);
        }
        if (err) {
            DEBUG_PRINT("sender_setup: PHY 1M adv failed\n");
        } else {
            /* Set channel map for this advertising set */
            sl_bt_advertiser_set_channel_map(ext_adv[1], channel_map);
        }
    }
    
    if (param->phy_s8) {
        if (adv_param_mask[0] != 0 || adv_param_mask[1] != 0) {
            const adv_param_t *base_param = non_connectable_adv_param_x[round_adv_param_index][2];
            adv_param_t work_adv_param = *base_param;
            work_adv_param.options |= adv_param_mask[1];
            work_adv_param.options &= ~adv_param_mask[0];
            err = update_adv(2, &work_adv_param, NULL, NULL);
        } else {
            err = update_adv(2, NULL, NULL, NULL);
        }
        if (err) {
            DEBUG_PRINT("sender_setup: PHY Coded adv failed\n");
        } else {
            /* Set channel map for this advertising set */
            sl_bt_advertiser_set_channel_map(ext_adv[2], channel_map);
        }
    }
    
    if (param->phy_ble4) {
        if (adv_param_mask[0] != 0 || adv_param_mask[1] != 0) {
            const adv_param_t *base_param = non_connectable_adv_param_x[round_adv_param_index][3];
            adv_param_t work_adv_param = *base_param;
            work_adv_param.options |= adv_param_mask[1];
            work_adv_param.options &= ~adv_param_mask[0];
            err = update_adv(3, &work_adv_param, NULL, NULL);
        } else {
            err = update_adv(3, NULL, NULL, NULL);
        }
        if (err) {
            DEBUG_PRINT("sender_setup: BLE4 adv failed\n");
        } else {
            /* Set channel map for this advertising set */
            sl_bt_advertiser_set_channel_map(ext_adv[3], channel_map);
        }
    }

    /* Start passive scanning */
    err = passive_scan_control(0);
    if (err) {
        DEBUG_PRINT("sender_setup: Scan start failed: %d\n", err);
    }
    
    DEBUG_PRINT("Sender setup complete (TX power: %d dBm)\n", param->txpwr);
    
    return 0;
}

void scanner_peek_msg(void)
{
    if (!svc_init_success) {
        return;
    }
    
    /* Temporary buffers for RSSI and TX power strings */
    char rssi_str[3][8];
    char tx_pwr_str[8];
    
    /* Generate message for 2M PHY (index 0) */
    snprintf(peek_msg_str[0], sizeof(peek_msg_str[0]), peek_rcvpkt_form,
            (uint8_t)rec_sets[0].node,
            pri_phy_typ[rec_sets[0].pri_phy],
            sec_phy_typ[rec_sets[0].sec_phy],
            sub_total_rcv[0],
            LOSS_TEST_BURST_COUNT * rec_sets[0].flow,
            rssi_toa(peek_rcv_rssi[0][0], rssi_str[0]),  /* current RSSI */
            rssi_toa(peek_rcv_rssi[0][1], rssi_str[1]),  /* min RSSI */
            rssi_toa(peek_rcv_rssi[0][2], rssi_str[2]),  /* max RSSI */
            txpwr_toa(remote_tx_pwr[0], tx_pwr_str));
    *(uint16_t *)(peek_msg_str[0]) = MANUFACTURER_ID;
    
    /* Generate message for 1M PHY (index 1) */
    snprintf(peek_msg_str[1], sizeof(peek_msg_str[1]), peek_rcvpkt_form,
            (uint8_t)rec_sets[1].node,
            pri_phy_typ[rec_sets[1].pri_phy],
            sec_phy_typ[rec_sets[1].sec_phy],
            sub_total_rcv[1],
            LOSS_TEST_BURST_COUNT * rec_sets[1].flow,
            rssi_toa(peek_rcv_rssi[1][0], rssi_str[0]),
            rssi_toa(peek_rcv_rssi[1][1], rssi_str[1]),
            rssi_toa(peek_rcv_rssi[1][2], rssi_str[2]),
            txpwr_toa(remote_tx_pwr[1], tx_pwr_str));
    *(uint16_t *)(peek_msg_str[1]) = MANUFACTURER_ID;
    
    /* Generate message for Coded PHY (index 2) */
    snprintf(peek_msg_str[2], sizeof(peek_msg_str[2]), peek_rcvpkt_form,
            (uint8_t)rec_sets[2].node,
            pri_phy_typ[rec_sets[2].pri_phy],
            sec_phy_typ[rec_sets[2].sec_phy],
            sub_total_rcv[2],
            LOSS_TEST_BURST_COUNT * rec_sets[2].flow,
            rssi_toa(peek_rcv_rssi[2][0], rssi_str[0]),
            rssi_toa(peek_rcv_rssi[2][1], rssi_str[1]),
            rssi_toa(peek_rcv_rssi[2][2], rssi_str[2]),
            txpwr_toa(remote_tx_pwr[2], tx_pwr_str));
    *(uint16_t *)(peek_msg_str[2]) = MANUFACTURER_ID;
    
    /* Generate message for BLE 4.x (index 3) */
    snprintf(peek_msg_str[3], sizeof(peek_msg_str[3]), peek_rcvpkt_btv4_form,
            (uint8_t)rec_sets[3].node,
            "BLE", "v4",
            sub_total_rcv[3],
            LOSS_TEST_BURST_COUNT * rec_sets[3].flow,
            rssi_toa(peek_rcv_rssi[3][0], rssi_str[0]),
            rssi_toa(peek_rcv_rssi[3][1], rssi_str[1]),
            rssi_toa(peek_rcv_rssi[3][2], rssi_str[2]),
            txpwr_toa(remote_tx_pwr[3], tx_pwr_str));
    *(uint16_t *)(peek_msg_str[3]) = MANUFACTURER_ID;
}

int scanner_setup(const test_param_t *param)
{
    if (!svc_init_success || param == NULL) {
        return -EINVAL;
    }
    
    int err = 0;
    
    /* Initialize application layer variables */
    round_tx_pwr = param->txpwr;
    round_adv_param_index = param->interval_idx;
    round_phy_sel[0] = param->phy_2m;
    round_phy_sel[1] = param->phy_1m;
    round_phy_sel[2] = param->phy_s8;
    round_phy_sel[3] = param->phy_ble4;
    
    /* Reset all counters */
    sub_total_snd_2m = 0;
    sub_total_snd_1m = 0;
    sub_total_snd_s8 = 0;
    sub_total_snd_ble4 = 0;
    sub_total_rcv[0] = 0;
    sub_total_rcv[1] = 0;
    sub_total_rcv[2] = 0;
    sub_total_rcv[3] = 0;
    
    /* Reset reception statistics */
    memset(rec_sets, 0, sizeof(rec_sets));
    
    /* Set config flags */
    ignore_rcv_resp = param->ignore_rcv_resp;
    inhibit_ch37 = param->inhibit_ch37;
    inhibit_ch38 = param->inhibit_ch38;
    inhibit_ch39 = param->inhibit_ch39;
    non_ANONYMOUS = param->non_ANONYMOUS;
    
    /* Store abort callbacks */
    envmon_abort_p = param->envmon_abort;
    sender_abort_p = param->sender_abort;
    scanner_abort_p = param->scanner_abort;
    numcast_abort_p = param->numcast_abort;
    
    /* Set scanner inactive flag */
    extern bool scanner_inactive;
    scanner_inactive = true;
    
    /* Print task startup banner */
    DEBUG_PRINT("Packet Loss Test (node %03u) **** RCV SIDE ****\n", device_address[0]);
    
    /* Generate initial status message */
    scanner_peek_msg();
    
    /* Set TX power for potential response advertising */
    err = set_adv_tx_power(param->txpwr, 4);
    if (err) {
        DEBUG_PRINT("scanner_setup: TX power set failed: %d\n", err);
        return err;
    }
    
    /* Build advertising parameter masks based on configuration flags */
    /* Note: Scanner doesn't use adv_param_mask but set for consistency */
    adv_param_mask[0] = (non_ANONYMOUS) ? BT_LE_ADV_OPT_ANONYMOUS : 0;
    adv_param_mask[1] = ((non_ANONYMOUS) ? BT_LE_ADV_OPT_USE_IDENTITY : 0);
    
    /* Calculate advertising channel map based on inhibit flags */
    /* Note: Scanner may use advertising for responses */
    uint8_t channel_map = get_adv_channel_map(inhibit_ch37, inhibit_ch38, inhibit_ch39);
    
    /* Apply channel map to all advertising sets if needed for responses */
    for (int i = 0; i < 4; i++) {
        if (round_phy_sel[i] && ext_adv_status[i].initialized) {
            sl_bt_advertiser_set_channel_map(ext_adv[i], channel_map);
        }
    }
    
    /* Start passive scanning */
    err = passive_scan_control(0);
    if (err) {
        DEBUG_PRINT("scanner_setup: Scan start failed: %d\n", err);
        return err;
    }
    
    DEBUG_PRINT("Scanner setup complete\n");
    
    /* Update LCD display */
    lcd_ui_update(param, "Scanner", "Ready");
    
    return 0;
}

int numcast_setup(const test_param_t *param)
{
    if (!svc_init_success || param == NULL) {
        return -EINVAL;
    }
    
    int err = 0;
    
    /* Initialize application layer variables */
    round_adv_param_index = param->interval_idx;
    round_phy_sel[0] = param->phy_2m;
    round_phy_sel[1] = param->phy_1m;
    round_phy_sel[2] = param->phy_s8;
    round_phy_sel[3] = param->phy_ble4;
    
    /* Initialize number cast value */
    extern uint64_t number_cast_val;
    number_cast_val = *(uint64_t *)p_number_cast_form;
    
    /* Set config flags */
    inhibit_ch37 = param->inhibit_ch37;
    inhibit_ch38 = param->inhibit_ch38;
    inhibit_ch39 = param->inhibit_ch39;
    non_ANONYMOUS = param->non_ANONYMOUS;
    
    /* Store abort callbacks */
    envmon_abort_p = param->envmon_abort;
    sender_abort_p = param->sender_abort;
    scanner_abort_p = param->scanner_abort;
    numcast_abort_p = param->numcast_abort;
    
    /* Set TX power */
    err = set_adv_tx_power(param->txpwr, 4);
    if (err) {
        DEBUG_PRINT("numcast_setup: TX power set failed: %d\n", err);
        return err;
    }
    
    /* Build advertising parameter masks based on configuration flags */
    adv_param_mask[0] = (non_ANONYMOUS) ? BT_LE_ADV_OPT_ANONYMOUS : 0;
    adv_param_mask[1] = ((non_ANONYMOUS) ? BT_LE_ADV_OPT_USE_IDENTITY : 0);
    
    /* Calculate advertising channel map based on inhibit flags */
    uint8_t channel_map = get_adv_channel_map(inhibit_ch37, inhibit_ch38, inhibit_ch39);
    
    /* Apply channel map to all advertising sets */
    for (int i = 0; i < 4; i++) {
        if (round_phy_sel[i] && ext_adv_status[i].initialized) {
            sl_bt_advertiser_set_channel_map(ext_adv[i], channel_map);
        }
    }
    
    /* Stop all advertising first (blocking mode) */
    err = stop_all_advertising();
    if (err) {
        DEBUG_PRINT("numcast_setup: Stop advertising failed: %d\n", err);
    }
    
    /* Determine scan method based on PHY selection */
    int8_t scan_method;
    if (param->phy_s8 && (param->phy_ble4 || param->phy_1m || param->phy_2m)) {
        scan_method = 0;  /* All PHYs */
    } else if (param->phy_s8) {
        scan_method = 2;  /* Coded PHY only */
    } else {
        scan_method = 1;  /* 1M PHY only */
    }
    
    /* Start passive scanning */
    err = passive_scan_control(scan_method);
    if (err) {
        DEBUG_PRINT("numcast_setup: Scan start failed: %d\n", err);
    }
    
    /* Initialize number cast control variables */
    extern uint64_t number_cast_rxval;
    extern bool number_cast_auto;
    number_cast_rxval = UINT64_MAX;
    number_cast_auto = false;
    
    DEBUG_PRINT("Number cast setup complete (scan method: %d)\n", scan_method);
    
    /* Update LCD display */
    lcd_ui_update(param, "NumCast", "Ready");
    
    return 0;
}

int envmon_setup(const test_param_t *param)
{
    if (!svc_init_success || param == NULL) {
        return -EINVAL;
    }
    
    int err = 0;
    
    /* Stop all advertising */
    err = stop_all_advertising();
    if (err) {
        DEBUG_PRINT("envmon_setup: Stop advertising failed: %d\n", err);
    }
    
    /* Start passive scanning */
    err = passive_scan_control(0);
    if (err) {
        DEBUG_PRINT("envmon_setup: Scan start failed: %d\n", err);
    }
    
    DEBUG_PRINT("Environment monitor setup complete\n");
    
    /* Update LCD display */
    lcd_ui_update(param, "EnvMon", "Ready");
    
    return 0;
}

/* ================== Complete Initialization (losstst_init port) ================== */

int ble_test_init(bool auto_start_scan, bool auto_start_adv)
{
    int err = 0;
    
    /* Check if already initialized */
    if (svc_init_success) {
        DEBUG_PRINT("Already initialized\n");
        return -EPERM;
    }
    
    /* ========== Silicon Labs Platform Initialization ========== */
    
    /* Note: Bluetooth stack is typically initialized by Simplicity SDK
     * before main() is called, via sl_system_init() */
    
    /* Get device address */
    bd_addr address;
    uint8_t address_type;
    sl_status_t status = sl_bt_system_get_identity_address(&address, &address_type);
    if (status == SL_STATUS_OK) {
        memcpy(device_address, address.addr, 6);
        /* Pad with zeros for EUI-64 format */
        device_address[6] = 0x00;
        device_address[7] = 0x00;
        DEBUG_PRINT("Device address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   address.addr[5], address.addr[4], address.addr[3],
                   address.addr[2], address.addr[1], address.addr[0]);
    } else {
        DEBUG_PRINT("Bluetooth init failed (err 0x%04X)\n", (unsigned int)status);
        return -EIO;
    }
    
    /* Silicon Labs doesn't need explicit advertising set config check
     * as it's handled dynamically by the stack */
    num_adv_set = MAX_ADV_SETS;
    
    /* Validate advertising set configuration */
    if (num_adv_set < 5) {
        DEBUG_PRINT("error CONFIG_BT_EXT_ADV_MAX_ADV_SET < 5\n");
        return -EINVAL;
    }
    
    /* ========== Platform-Independent Initialization ========== */
    
    /* Initialize advertising data structures */
    /* Note: These would typically be initialized by application-specific
     * callback functions like sender_peek_msg() in the original code */
    
    /* Set device EUI-64 in all device info structures */
    uint64_t eui64 = *(uint64_t *)device_address;
    device_info_form[0].eui.eui_64 = eui64;
    device_info_form[1].eui.eui_64 = eui64;
    device_info_form[2].eui.eui_64 = eui64;
    device_info_form[3].eui.eui_64 = eui64;
    device_info_bt4_form.device_info.eui.eui_64 = eui64;
    
    /* Mark as initialized */
    svc_init_success = true;
    DEBUG_PRINT("Service initialization complete\n");
    
    /* Initialize all advertising sets if requested */
    if (auto_start_adv) {
        DEBUG_PRINT("Initializing advertising sets...\n");
        for (uint8_t i = 0; i < num_adv_set; i++) {
            /* Use different timeout for index 4 (remote control) */
            const adv_start_param_t *start_param = (i == 4) ? 
                p_adv_default_start_param :  /* Continuous */
                p_adv_finit_start_param;     /* 3 seconds */
            
            err = update_adv(i, NULL, NULL, start_param);
            if (err) {
                DEBUG_PRINT("Failed to initialize adv set %d: %d\n", i, err);
            }
        }
    }
    
    /* Start passive scanning if requested */
    if (auto_start_scan) {
        DEBUG_PRINT("Starting passive scan...\n");
        err = passive_scan_control(0);  /* Scan all PHYs */
        if (err) {
            DEBUG_PRINT("Failed to start scan: %d\n", err);
            /* Non-fatal, continue */
        }
    }
    
    DEBUG_PRINT("BLE test initialization complete\n");
    return 0;
}

static int8_t losstst_task_tgr(int8_t set,int8_t TGR_VAL)
{
	if(TGR_VAL!=losstst_task_val && 0==set) return 0;
	if(0==losstst_task_val && 0<set) losstst_task_val=TGR_VAL;
	else if(TGR_VAL==losstst_task_val && -TGR_VAL==set) losstst_task_val=0;
	return losstst_task_val;
}
static int8_t losstst_task_status(int8_t TGR_VAL)
{
	if(0==losstst_task_val) return 0; // idle
	if(TGR_VAL==losstst_task_val) return 1; // running
	return 2; // blocking	
}

int8_t numcst_task_tgr(int8_t set) { return losstst_task_tgr( set, numcst_tgr  ); }
int8_t numcst_task_status(void) { return losstst_task_status( numcst_tgr ); }
int8_t scanner_task_tgr(int8_t set){ return losstst_task_tgr( set, scanner_tgr ); }
int8_t scanner_task_status(void){ return losstst_task_status( scanner_tgr); }
int8_t sender_task_tgr(int8_t set) { return losstst_task_tgr( set, sender_tgr  ); }
int8_t sender_task_status(void) { return losstst_task_status( sender_tgr ); }
int8_t envmon_task_tgr(int8_t set) { return losstst_task_tgr( set, envmon_tgr  ); }
int8_t envmon_task_status(void) { return losstst_task_status( envmon_tgr ); }

/* ===============================================
 * 雙層初始化範例
 * =============================================== */

// 應用層特定變數（依您的需求定義）
static adv_data_t resp_burst_end_data[4];
static adv_data_t remote_ctrl_data[4];

/**
 * @brief 應用層初始化
 * 
 * 此函數在 ble_test_init() 之後調用，初始化應用層特定功能
 */
int my_app_init(void)
{
    /* 初始化回應突發廣播資料 */
    resp_burst_end_data[0] = (adv_data_t){
        .type = BT_DATA_FLAGS,
        .data_len = 1,
        .data = p_common_adv_flags
    };
    resp_burst_end_data[1] = (adv_data_t){
        .type = BT_DATA_MANUFACTURER_DATA,
        .data_len = sizeof(device_info_t)
    };
    resp_burst_end_data[2] = (adv_data_t){
        .type = 0,
        .data = NULL,
        .data_len = 0
    };
    
    /* 初始化遠端控制資料 */
    remote_ctrl_data[0] = (adv_data_t){
        .type = BT_DATA_FLAGS,
        .data_len = 1,
        .data = p_common_adv_flags
    };
    remote_ctrl_data[2] = (adv_data_t){
        .type = 0,
        .data = NULL,
        .data_len = 0
    };
    
    /* 生成初始狀態訊息 */
    sender_peek_msg();
    
    return 0;
}

/**
 * @brief 完整的系統初始化（雙層）
 * 
 * 這是推薦的初始化順序
 */
int losstst_init(void)
{
    int err;
    
    /* 第一層：核心 BLE 初始化 */
    err = ble_test_init(true, true);
    if (err) {
        DEBUG_PRINT("Core BLE init failed: %d\n", err);
        return err;
    }
    DEBUG_PRINT("✓ Core BLE initialized\n");
    
    /* 第二層：應用層初始化 */
    err = my_app_init();
    if (err) {
        DEBUG_PRINT("Application init failed: %d\n", err);
        return err;
    }
    DEBUG_PRINT("✓ Application layer initialized\n");
    
    DEBUG_PRINT("=== System Ready ===\n");
    return 0;
}

const char* get_peek_msg_buffer(uint8_t index)
{
    if (index >= 4) {
        return NULL;
    }
    return peek_msg_str[index];
}

/* ================== Burst Test Functions Implementation ================== */

/**
 * @brief Advertising sent callback handler (equivalent to loss_tst_sent_cb)
 * 
 * This function should be called when an advertising event completes.
 * In Silicon Labs, this is typically triggered by sl_bt_evt_advertiser_timeout.
 * 
 * Nordic equivalent: static void loss_tst_sent_cb(struct bt_le_ext_adv *adv, 
 *                                                   struct bt_le_ext_adv_sent_info *info)
 * 
 * @param adv_handle Advertising handle that completed
 */
void losstst_adv_sent_handler(adv_handle_t adv_handle)
{
    uint8_t index = adv_handle;
    
    if (num_adv_set <= index) {
        return;
    }
    
    /* Mark advertising as stopped */
    ext_adv_status[index].stop = 1;
    
    /* Handle sender abort flag for PHY test sets (index 0-3) */
    if (index < ARRAY_SIZE(sndr_abort_flag) && sndr_abort_flag[index]) {
        sndr_abort_flag[index] = false;
        
        /* Adjust flow count for burst test completion */
        if (device_info_form[index].flw_cnt <= 200) {
            device_info_form[index].flw_cnt *= LOSS_TEST_BURST_COUNT;
        } else {
            device_info_form[index].flw_cnt = 256;
        }
        
        /* Update BT4 form if this is the legacy PHY */
        if (index == 3) {
            device_info_bt4_form.device_info = device_info_form[3];
        }
        
        /* Re-advertise with updated data for 5 seconds */
        update_adv(index, NULL, ratio_test_data_set[index], p_adv_5sec_start_param);
    }
    
    /* Handle remote control message output callback (index 4) */
    // if (index == 4) {
    //     /* Remote control message sent callback */
    //     extern void rc_msg_out_cb(void);
    //     rc_msg_out_cb();
    // }
}

void blocking_adv(uint8_t index)
{
    if (index >= MAX_ADV_SETS) {
        return;
    }
    
    /* Stop advertising immediately */
    int err = platform_stop_adv(ext_adv[index]);
    
    DEBUG_PRINT("blocking_adv(%u): %s\n", index, err ? "failed" : "stopped");
    
    /* Mark as stopped and clear start flag */
    ext_adv_status[index].stop = 1;
    ext_adv_status[index].start = 0;
}

void sender_finit(void)
{
    if (!svc_init_success) {
        return;
    }
    
    /* Calculate channel map (use global inhibit flags) */
    uint8_t channel_map = get_adv_channel_map(inhibit_ch37, inhibit_ch38, inhibit_ch39);
    
    /* Mark all enabled PHY transmissions as complete */
    for (int idx = 0; idx < 4; idx++) {
        if (round_phy_sel[idx]) {
            sndr_abort_flag[idx] = true;
            device_info_form[idx].pre_cnt = INT16_MAX;
            
            if (idx == 3) {
                device_info_bt4_form.device_info = device_info_form[3];
            }
            
            /* Use group 3 parameters (1 second interval) for finalization */
            const adv_param_t *base_param = non_connectable_adv_param_x[3][idx];
            adv_param_t work_adv_param = *base_param;
            
            /* Apply masks: clear unwanted bits, set wanted bits */
            work_adv_param.options |= adv_param_mask[1];
            work_adv_param.options &= ~adv_param_mask[0];
            
            update_adv(idx, &work_adv_param, ratio_test_data_set[idx], p_adv_finit_start_param);
            
            /* Set channel map for this advertising set */
            sl_bt_advertiser_set_channel_map(ext_adv[idx], channel_map);
        }
    }
    
    /* Update peek message advertising if not in remote control mode */
    //if (!rc_party) {
        update_adv(4, NULL, NULL, NULL);
    //}
    
    DEBUG_PRINT("Sender finalized\n");
}

int losstst_sender(void)
{
    if (!svc_init_success) {
        return -1;
    }
    
    int retval = 1;
    int16_t lc_pre_cnt;
    uint16_t sub_phy0, sub_phy1, sub_phy2;//, sub_phy3;
    bool lc_phy_sel[4] = {false, false, false, false};
    bool abort = false;
    int64_t uptime_64_barrier, period_msec, pitch_msec;
    
    /* Determine which PHYs still need transmission */
    sub_phy0 = (round_phy_sel[0]) ? sub_total_snd_2m : round_total_num;
    sub_phy1 = (round_phy_sel[1]) ? sub_total_snd_1m : round_total_num;
    sub_phy1 = (sub_phy1 < sub_phy0) ? sub_phy1 : sub_phy0;  /* MIN */
    sub_phy2 = (round_phy_sel[2]) ? sub_total_snd_s8 : round_total_num;
    //sub_phy3 = (round_phy_sel[3]) ? sub_total_snd_ble4 : round_total_num;
    
    /* Select PHYs for this burst cycle */
    if (sub_phy1 <= sub_phy2) {
        lc_phy_sel[0] = round_phy_sel[0];
        lc_phy_sel[1] = round_phy_sel[1];
        lc_phy_sel[3] = round_phy_sel[3];
    } else {
        lc_phy_sel[2] = round_phy_sel[2];
    }
    
    /* Check if any PHY needs more transmissions */
    if ((lc_phy_sel[0] && sub_total_snd_2m < round_total_num)
        || (lc_phy_sel[1] && sub_total_snd_1m < round_total_num)
        || (lc_phy_sel[2] && sub_total_snd_s8 < round_total_num)
        || (lc_phy_sel[3] && sub_total_snd_ble4 < round_total_num)) {
        
        /* ========== Phase 1: Pre-burst countdown (3 seconds) ========== */
        uptime_64_barrier = platform_uptime_get();
        lc_pre_cnt = -3;
        
        /* Initialize pre-burst progress */
        for (int idx = 0; idx <= 3; idx++) {
            if (lc_phy_sel[idx]) {
                device_info_form[idx].pre_cnt = lc_pre_cnt;
                device_info_form[idx].flw_cnt++;
                if (idx == 3) {
                    device_info_bt4_form.device_info = device_info_form[3];
                }
            }
        }
        
        /* Countdown loop */
        do {
            /* Start advertising with countdown value */
            for (int idx = 0; idx <= 3; idx++) {
                if (lc_phy_sel[idx]) {
                    /* Use group 3 parameters (1 second interval) for pre-burst */
                    const adv_param_t *work_adv_param = non_connectable_adv_param_x[3][idx];
                    if (idx == 3) {
                        device_info_bt4_form.device_info = device_info_form[3];
                    }
                    update_adv(idx, work_adv_param, ratio_test_data_set[idx], p_adv_default_start_param);
                    snd_state_val[idx] = 1;
                }
            }
            
            /* Wait 1 second */
            uptime_64_barrier += 1000;
            while (uptime_64_barrier > platform_uptime_get()) {
                if (platform_can_yield()) {
                    platform_yield();
                }
                if (sender_abort_p != NULL) {
                    bool (*abort_fn)(void) = (bool (*)(void))sender_abort_p;
                    if ((abort = abort_fn())) {
                        break;
                    }
                }
            }
            
            lc_pre_cnt++;
            
            /* Update countdown value */
            for (int idx = 0; idx <= 3; idx++) {
                if (lc_phy_sel[idx]) {
                    device_info_form[idx].pre_cnt = lc_pre_cnt;
                    if (idx == 3) {
                        device_info_bt4_form.device_info = device_info_form[3];
                    }
                }
            }
            
        } while (!abort && lc_pre_cnt != 0);
        
        if (abort) {
            snd_state_val[0] = snd_state_val[1] = snd_state_val[2] = snd_state_val[3] = 0;
            sender_finit();
            return -1;
        }
        
        /* Stop all countdown advertising */
        blocking_adv(0);
        blocking_adv(1);
        blocking_adv(2);
        blocking_adv(3);
        
        /* ========== Phase 2: Burst transmission ========== */
        period_msec = LOSS_TEST_BURST_COUNT * value_interval[round_adv_param_index][1];
        int32_t period_sec = 1 + period_msec / 1000;
        
        /* Initialize burst progress counter */
        for (int idx = 0; idx <= 3; idx++) {
            if (lc_phy_sel[idx]) {
                device_info_form[idx].pre_cnt = period_sec;
            }
            if (idx == 3) {
                device_info_bt4_form.device_info = device_info_form[3];
            }
        }
        
        /* Wait 100ms before starting burst */
        uptime_64_barrier += 100;
        while (uptime_64_barrier > platform_uptime_get()) {
            if (platform_can_yield()) {
                platform_yield();
            }
            if (sender_abort_p != NULL) {
                bool (*abort_fn)(void) = (bool (*)(void))sender_abort_p;
                if ((abort = abort_fn())) {
                    break;
                }
            }
        }
        
        if (abort) {
            snd_state_val[0] = snd_state_val[1] = snd_state_val[2] = snd_state_val[3] = 0;
            sender_finit();
            return -1;
        }
        
        /* Start burst advertising (250 events per PHY) */
        for (int idx = 0; idx <= 3; idx++) {
            if (lc_phy_sel[idx]) {
                /* Use configured interval for burst transmission */
                const adv_param_t *work_adv_param = non_connectable_adv_param_x[round_adv_param_index][idx];
                if (idx == 3) {
                    device_info_bt4_form.device_info = device_info_form[3];
                }
                update_adv(idx, work_adv_param, ratio_test_data_set[idx], p_adv_burst_start_param);
                snd_state_val[idx] = 2;
            }
        }
        
        /* Wait for burst completion while updating countdown */
        uptime_64_barrier += period_msec;
        pitch_msec = 1000 + platform_uptime_get();
        
        while (((lc_phy_sel[0] && !ext_adv_status[0].stop)
                || (lc_phy_sel[1] && !ext_adv_status[1].stop)
                || (lc_phy_sel[2] && !ext_adv_status[2].stop)
                || (lc_phy_sel[3] && !ext_adv_status[3].stop))
            && (uptime_64_barrier > (period_msec = platform_uptime_get()))) {
            
            if (platform_can_yield()) {
                platform_yield();
            }
            
            if (sender_abort_p != NULL) {
                bool (*abort_fn)(void) = (bool (*)(void))sender_abort_p;
                if ((abort = abort_fn())) {
                    break;
                }
            }
            
            /* Update countdown every second */
            if (period_msec >= pitch_msec) {
                pitch_msec += 1000;
                period_sec--;
                
                for (int idx = 0; idx <= 3; idx++) {
                    if (lc_phy_sel[idx]) {
                        device_info_form[idx].pre_cnt = period_sec;
                        if (idx == 3) {
                            device_info_bt4_form.device_info = device_info_form[3];
                        }
                        update_adv(idx, NULL, ratio_test_data_set[idx], NULL);
                    }
                }
            }
        }
        
        if (abort) {
            snd_state_val[0] = snd_state_val[1] = snd_state_val[2] = snd_state_val[3] = 0;
            sender_finit();
            return -1;
        }
        
        /* ========== Phase 3: Post-burst reporting ========== */
        ack_remote_resp[0] = ack_remote_resp[1] = ack_remote_resp[2] = ack_remote_resp[3] = false;
        
        /* Update counters and start reporting advertising */
        for (int idx = 0; idx <= 3; idx++) {
            if (lc_phy_sel[idx]) {
                device_info_form[idx].pre_cnt = 0;
                
                if (idx == 3) {
                    device_info_bt4_form.device_info = device_info_form[3];
                }
                
                /* Use group 3 parameters (1 second interval) for post-burst */
                const adv_param_t *work_adv_param = non_connectable_adv_param_x[3][idx];
                update_adv(idx, work_adv_param, ratio_test_data_set[idx], p_adv_default_start_param);
                
                /* Update transmission counters */
                switch (idx) {
                    case 0:
                        xmt_ratio_val[idx][0] = (sub_total_snd_2m += LOSS_TEST_BURST_COUNT);
                        break;
                    case 1:
                        xmt_ratio_val[idx][0] = (sub_total_snd_1m += LOSS_TEST_BURST_COUNT);
                        break;
                    case 2:
                        xmt_ratio_val[idx][0] = (sub_total_snd_s8 += LOSS_TEST_BURST_COUNT);
                        break;
                    case 3:
                        xmt_ratio_val[idx][0] = (sub_total_snd_ble4 += LOSS_TEST_BURST_COUNT);
                        break;
                }
                snd_state_val[idx] = 3;
            }
        }
        
        /* Generate and print status messages */
        sender_peek_msg();
        //if (!rc_party) {
            update_adv(4, NULL, NULL, NULL);
        //}
        
        if (lc_phy_sel[0]) {
            DEBUG_PRINT("%s\n", peek_msg_str[0] + 2);
        }
        if (lc_phy_sel[1]) {
            DEBUG_PRINT("%s\n", peek_msg_str[1] + 2);
        }
        if (lc_phy_sel[2]) {
            DEBUG_PRINT("%s\n", peek_msg_str[2] + 2);
        }
        if (lc_phy_sel[3]) {
            DEBUG_PRINT("%s\n", peek_msg_str[3] + 2);
        }
        
        /* Wait for remote response or timeout (100ms) */
        uptime_64_barrier += 100;
        while (((lc_phy_sel[0] && !ack_remote_resp[0])
                || (lc_phy_sel[1] && !ack_remote_resp[1])
                || (lc_phy_sel[2] && !ack_remote_resp[2])
                || (lc_phy_sel[3] && !ack_remote_resp[3]) || ignore_rcv_resp)
            && (uptime_64_barrier > platform_uptime_get())) {
            
            if (platform_can_yield()) {
                platform_yield();
            }
            
            if (sender_abort_p != NULL) {
                bool (*abort_fn)(void) = (bool (*)(void))sender_abort_p;
                if ((abort = abort_fn())) {
                    break;
                }
            }
        }
        
        if (abort) {
            snd_state_val[0] = snd_state_val[1] = snd_state_val[2] = snd_state_val[3] = 0;
            sender_finit();
            return -1;
        }
        
        snd_state_val[0] = snd_state_val[1] = snd_state_val[2] = snd_state_val[3] = 0;
        
        /* ========== Phase 4: Check for completion ========== */
        /* Check each PHY for completion */
        if (lc_phy_sel[0] && sub_total_snd_2m >= round_total_num) {
            DEBUG_PRINT("SND:%u P:%s/%s Complete\n",
                       (uint8_t)device_address[0],
                       pri_phy_typ[1], sec_phy_typ[2]);
            
            device_info_form[0].pre_cnt = INT16_MAX;
            update_adv(0, NULL, ratio_test_data_set[0], p_adv_default_start_param);
        }
        
        if (lc_phy_sel[1] && sub_total_snd_1m >= round_total_num) {
            DEBUG_PRINT("SND:%u P:%s/%s Complete\n",
                       (uint8_t)device_address[0],
                       pri_phy_typ[1], sec_phy_typ[1]);
            
            device_info_form[1].pre_cnt = INT16_MAX;
            update_adv(1, NULL, ratio_test_data_set[1], p_adv_default_start_param);
        }
        
        if (lc_phy_sel[2] && sub_total_snd_s8 >= round_total_num) {
            DEBUG_PRINT("SND:%u P:%s/%s Complete\n",
                       (uint8_t)device_address[0],
                       pri_phy_typ[3], sec_phy_typ[3]);
            
            device_info_form[2].pre_cnt = INT16_MAX;
            update_adv(2, NULL, ratio_test_data_set[2], p_adv_default_start_param);
        }
        
        if (lc_phy_sel[3] && sub_total_snd_ble4 >= round_total_num) {
            DEBUG_PRINT("SND:%u P:BLEv4 Complete\n",
                       (uint8_t)device_address[0]);
            
            device_info_form[3].pre_cnt = INT16_MAX;
            device_info_bt4_form.device_info = device_info_form[3];
            update_adv(3, NULL, ratio_test_data_set[3], p_adv_default_start_param);
        }
        
        /* Wait 500ms + 500ms with abort check on second half */
        uptime_64_barrier = 500 + platform_uptime_get();
        while (uptime_64_barrier > platform_uptime_get()) {
            if (platform_can_yield()) {
                platform_yield();
            }
        }
        
        uptime_64_barrier += 500;
        while (uptime_64_barrier > platform_uptime_get()) {
            if (platform_can_yield()) {
                platform_yield();
            }
            if (sender_abort_p != NULL) {
                bool (*abort_fn)(void) = (bool (*)(void))sender_abort_p;
                if ((abort = abort_fn())) {
                    break;
                }
            }
        }
        
    } else {
        /* All PHYs complete */
        sender_finit();
        retval = 0;
    }
    
    return retval;
}

int losstst_scanner(void)
{
    if (!svc_init_success) {
        return -1;
    }
    
    int retval = 1;
    int64_t period_msec, uptime_64_barrier;
    bool abort = false;
    static int8_t round_scan_method;
    static int8_t next_scan_method = 0;
    static int16_t assign;
    static int64_t cntdn = 0;
    static int64_t complete_mark, complete_elapse;
    static bool phy_mark[4];
    static int64_t hrtbt, hrtbt_stamp;
    static bool first_round;
    
    /* Initialize on first call or after inactive period */
    if (scanner_inactive) {
        memset(rcv_stamp, 0, sizeof(rcv_stamp));
        scanner_inactive = false;
        memset(rcv_ratio_val, 0, sizeof(rcv_ratio_val));
        
        /* Determine scan method based on PHY selection */
        round_scan_method = (round_phy_sel[2] && (round_phy_sel[3] || round_phy_sel[1] || round_phy_sel[0])) 
                          ? 0 : ((round_phy_sel[2]) ? 2 : 1);
        
        memset(rcv_stats, 0, sizeof(rcv_stats));
        memset(rcv_ratio_val, 0, sizeof(rcv_ratio_val));
        memset(rcv_rssi_val, 0, sizeof(rcv_rssi_val));
        first_round = true;
    }
    
    /* Start passive scanning */
    passive_scan_control((2 == round_scan_method /*&& rc_party*/) ? 0 : round_scan_method);
    
    /* Calculate expected period */
    period_msec = LOSS_TEST_BURST_COUNT * value_interval[round_adv_param_index][1];
    if (round_scan_method) {
        period_msec *= 2;
    } else {
        period_msec += 3000;
    }
    
    /* Heartbeat timeout check */
    if (0 == hrtbt_stamp) {
        hrtbt_stamp = platform_uptime_get();
    } else if (
        ((NULL != scanner_abort_p) && (true == (abort = ((bool (*)(void))scanner_abort_p)())))
        || ((round_scan_method ? 30000 : 10000) < (hrtbt += platform_uptime_get() - hrtbt_stamp))
        || (period_msec * (first_round ? 5 : 1) < hrtbt)) {
        hrtbt = hrtbt_stamp = 0;
        passive_scan_control(-1);  /* Stop scanning */
        return -1;
    }
    
    /* Stop all advertising before receiving */
    blocking_adv(0);
    blocking_adv(1);
    blocking_adv(2);
    blocking_adv(3);
    
    /* Determine next scan method based on received pre-counts */
    next_scan_method = 0;
    
    if (0 > (assign = precnt_rcv[0]) && round_phy_sel[0]) {
        if (INT16_MIN != assign) {
            next_scan_method = 1;
            cntdn = precnt_rcv[0] * -1000L;
        }
    } else if (0 > (assign = precnt_rcv[1]) && round_phy_sel[1]) {
        if (INT16_MIN != assign) {
            next_scan_method = 1;
            cntdn = precnt_rcv[1] * -1000L;
        }
    } else if (0 > (assign = precnt_rcv[2]) && round_phy_sel[2]) {
        if (INT16_MIN != assign) {
            next_scan_method = 2;
            cntdn = precnt_rcv[2] * -1000L;
        }
    } else if (0 > (assign = precnt_rcv[3]) && round_phy_sel[3]) {
        if (INT16_MIN != assign) {
            next_scan_method = 1;
            cntdn = precnt_rcv[3] * -1000L;
        }
    } else if (0 < (assign = precnt_rcv[0]) && round_phy_sel[0]) {
        if (INT16_MAX != assign) next_scan_method = 1;
    } else if (0 < (assign = precnt_rcv[1]) && round_phy_sel[1]) {
        if (INT16_MAX != assign) next_scan_method = 1;
    } else if (0 < (assign = precnt_rcv[2]) && round_phy_sel[2]) {
        if (INT16_MAX != assign) next_scan_method = 2;
    } else if (0 < (assign = precnt_rcv[3]) && round_phy_sel[3]) {
        if (INT16_MAX != assign) next_scan_method = 1;
    } else {
        /* Check if all receptions complete */
        if ((!rec_sets[0].flow && !rec_sets[1].flow && !rec_sets[2].flow && !rec_sets[3].flow)) {
            /* No flows detected yet */
        } else if ((rec_sets[0].complete || !rec_sets[0].flow)
                && (rec_sets[1].complete || !rec_sets[1].flow)
                && (rec_sets[2].complete || !rec_sets[2].flow)
                && (rec_sets[3].complete || !rec_sets[3].flow)) {
            /* All complete */
            hrtbt = hrtbt_stamp = 0;
            retval = 0;
            passive_scan_control(-1);  /* Stop scanning */
        }
        return retval;
    }
    
    if (0 == next_scan_method) {
        return retval;
    }
    
    first_round = false;
    
    /* Check for completion timeout */
    if (INT16_MAX == precnt_rcv[0] && INT16_MAX == precnt_rcv[1] && 
        INT16_MAX == precnt_rcv[2] && INT16_MAX == precnt_rcv[3]) {
        if (0 == complete_mark) {
            complete_mark = platform_uptime_get();
        } else if (10000 < (complete_elapse += platform_uptime_get() - complete_mark)) {
            DEBUG_PRINT("RCV_Task completed\n");
            passive_scan_control(-1);
            return 0;
        }
    } else {
        complete_elapse = complete_mark = 0;
    }
    
    /* Stop peek message advertising if not in remote control mode */
    //if (!rc_party) {
        blocking_adv(4);
    //}
    
    period_msec += cntdn;
    uptime_64_barrier = period_msec + platform_uptime_get();
    
    /* Start scanning with selected method */
    passive_scan_control(next_scan_method);
    
    cntdn = 0;
    phy_mark[0] = phy_mark[1] = phy_mark[2] = phy_mark[3] = false;
    
    /* Main reception loop */
    while (uptime_64_barrier > platform_uptime_get()) {
        /* Print received messages */
        if ('\0' != *rcv_msg_str[0]) {
            DEBUG_PRINT("%s\n", rcv_msg_str[0]);
            *rcv_msg_str[0] = '\0';
        }
        if ('\0' != *rcv_msg_str[1]) {
            DEBUG_PRINT("%s\n", rcv_msg_str[1]);
            *rcv_msg_str[1] = '\0';
        }
        if ('\0' != *rcv_msg_str[2]) {
            DEBUG_PRINT("%s\n", rcv_msg_str[2]);
            *rcv_msg_str[2] = '\0';
        }
        
        /* Check for abort */
        if (NULL == scanner_abort_p) {
        } else if (true == (abort = ((bool (*)(void))scanner_abort_p)())) {
            rcv_state_val[0] = rcv_state_val[1] = rcv_state_val[2] = rcv_state_val[3] = 0;
            break;
        }
        
        /* Track PHY states for scan method 1 (1M/2M/BLE4) */
        if (1 == next_scan_method) {
            if (0 > (precnt_rcv[0])) {
                if (INT16_MIN != precnt_rcv[0]) {
                    phy_mark[0] = true;
                    rcv_state_val[0] = 1;
                }
            } else if (0 < (precnt_rcv[0])) {
                phy_mark[0] = true;
                rcv_state_val[0] = 2;
            } else if (0 == (precnt_rcv[0])) {
                if (phy_mark[0]) rcv_state_val[0] = 3;
            }
            
            if (0 > (precnt_rcv[1])) {
                if (INT16_MIN != precnt_rcv[1]) {
                    phy_mark[1] = true;
                    rcv_state_val[1] = 1;
                }
            } else if (0 < (precnt_rcv[1])) {
                phy_mark[1] = true;
                rcv_state_val[1] = 2;
            } else if (0 == (precnt_rcv[1])) {
                if (phy_mark[1]) rcv_state_val[1] = 3;
            }
            
            if (0 > (precnt_rcv[3])) {
                if (INT16_MIN != precnt_rcv[3]) {
                    phy_mark[3] = true;
                    rcv_state_val[3] = 1;
                }
            } else if (0 < (precnt_rcv[3])) {
                phy_mark[3] = true;
                rcv_state_val[3] = 2;
            } else if (0 == (precnt_rcv[3])) {
                if (phy_mark[3]) rcv_state_val[3] = 3;
            }
            
            rcv_state_val[2] = 0;
        }
        
        /* Track PHY states for scan method 2 (Coded PHY) */
        if (2 == next_scan_method) {
            if (0 > precnt_rcv[2]) {
                if (INT16_MIN != precnt_rcv[2]) {
                    phy_mark[2] = true;
                    rcv_state_val[2] = 1;
                }
            }
            if (0 < precnt_rcv[2]) {
                phy_mark[2] = true;
                rcv_state_val[2] = 2;
            }
            if (0 == precnt_rcv[2]) {
                if (phy_mark[2]) rcv_state_val[2] = 3;
            }
            rcv_state_val[0] = rcv_state_val[1] = rcv_state_val[3] = 0;
        }
        
        /* Send response when burst complete */
        for (int idx = 0; idx <= 3; idx++) {
            if (phy_mark[idx] && rec_sets[idx].complete) {
                abort = true;
                break;
            }
            if (phy_mark[idx] && 0 == precnt_rcv[idx]) {
                if (!ignore_rcv_resp) {
                    resp_burst_end_data[1].data = (const uint8_t *)&remote_resp_form[idx];
                    update_adv(idx, NULL, resp_burst_end_data, p_adv_1sec_start_param);
                }
                
                rcv_state_val[idx] = 0;
                phy_mark[idx] = false;
            }
        }
        
        if (abort) break;
        
        /* Exit loop if all PHYs inactive */
        if (!phy_mark[0] && !phy_mark[1] && !phy_mark[2] && !phy_mark[3]) {
            if (0 == cntdn) {
                cntdn = 800 + platform_uptime_get();
            } else if (cntdn < platform_uptime_get()) {
                break;
            }
        }
        
        if (platform_can_yield()) {
            platform_yield();
        }
    }
    
    /* Clear marked PHYs */
    if (phy_mark[0]) precnt_rcv[0] = 0;
    if (phy_mark[1]) precnt_rcv[1] = 0;
    if (phy_mark[2]) precnt_rcv[2] = 0;
    if (phy_mark[3]) precnt_rcv[3] = 0;
    
    rcv_state_val[0] = rcv_state_val[1] = rcv_state_val[2] = rcv_state_val[3] = 0;
    hrtbt = hrtbt_stamp = 0;
    
    if (abort) retval = -1;
    
    /* Update status message */
    scanner_peek_msg();
    //if (!rc_party) {
        update_adv(4, NULL, NULL, p_adv_default_start_param);
    //}
    
    /* Resume scanning */
    passive_scan_control(((0 >= retval) || (2 == round_scan_method/* && rc_party*/)) ? 0 : round_scan_method);
    
    return retval;
}

/* ================== RSSI Helper Functions ================== */

/**
 * @brief Calculate number cast RSSI statistics
 * 
 * Updates average, min, max RSSI values from recent number cast packets.
 * Only recalculates if at least 50ms have passed since last calculation.
 */
static void numcst_rssi_calc(int64_t tm_stamp)
{
    if (llabs(numcst_rssi_rec_tm - tm_stamp) < 50) {
        return;
    }
    
    numcst_rssi_rec_tm = tm_stamp;
    int8_t cnt = 0;
    int16_t avg = 0;
    int8_t lower = 20;
    int8_t upper = -127;
    
    for (unsigned int idx = 0; idx < ARRAY_SIZE(numcst_rssi_rec); idx++) {
        int64_t rec_tm = numcst_rssi_rec[idx].expired_tm;
        int8_t rec_rssi = numcst_rssi_rec[idx].rssi;
        if (numcst_rssi_rec_tm <= rec_tm) {
            avg = avg + rec_rssi;
            cnt = cnt + 1;
            lower = (lower < rec_rssi) ? lower : rec_rssi;
            upper = (upper > rec_rssi) ? upper : rec_rssi;
        }
    }
    
    if (cnt) {
        avg /= cnt;
    }
    
    numcst_rssi[0] = avg;
    numcst_rssi[1] = (20 == lower) ? 0 : lower;
    numcst_rssi[2] = (-127 == upper) ? 0 : upper;
}

int losstst_numcast(void)
{
    if (!svc_init_success) {
        return -1;
    }
    
    static bool cast_auto = false;
    
    /* Check for abort condition */
    if(numcast_abort_p()) {
        cast_auto = false;
        number_cast_rxval = UINT64_MAX;
        
        /* Stop advertising on all PHYs */
        blocking_adv(0);
        update_adv(0, NULL, NULL, p_adv_1sec_start_param);
        blocking_adv(1);
        update_adv(1, NULL, NULL, p_adv_1sec_start_param);
        blocking_adv(2);
        update_adv(2, NULL, NULL, p_adv_1sec_start_param);
        blocking_adv(3);
        update_adv(3, NULL, NULL, p_adv_1sec_start_param);
        
        /* Reset to scan method 0 (all PHYs) */
        passive_scan_control(0);
        return 0;
    }
    
    /* Check if number cast value or auto mode changed */
    if (number_cast_val != *(uint64_t *)p_number_cast_form || cast_auto != number_cast_auto) {
        number_cast_val = *(uint64_t *)p_number_cast_form;
        cast_auto = number_cast_auto;
        
        /* Prepare start parameters: continuous if auto, 10 events if manual */
        const adv_start_param_t *start_param = cast_auto ? 
            p_adv_default_start_param :  /* Continuous (timeout=0, events=0) */
            BT_LE_EXT_ADV_START_PARAM(0, 10);  /* 10 events */
        
        adv_param_t work_adv_param;
        uint8_t channel_map = get_adv_channel_map(inhibit_ch37, inhibit_ch38, inhibit_ch39);
        
        for (int idx = 0; idx < 4; idx++) {
            blocking_adv(idx);
            
            if (round_phy_sel[idx]) {
                /* Get base parameters from the table */
                const adv_param_t *base_param = non_connectable_adv_param_x[round_adv_param_index][idx];
                work_adv_param = *base_param;
                
                /* Apply channel inhibit and identity options */
                work_adv_param.options |= adv_param_mask[1];
                work_adv_param.options &= ~adv_param_mask[0];
                
                if (idx == 3) {
                    /* BLE4 uses special format with number cast in tail */
                    numcast_bt4_form = device_info_bt4_form;
                    *((uint16_t *)numcast_bt4_form.tail) = UINT16_MAX;
                    *((uint64_t *)(2 + numcast_bt4_form.tail)) = number_cast_val;
                    update_adv(idx, &work_adv_param, number_cast_data_set[1], start_param);
                } else {
                    update_adv(idx, &work_adv_param, number_cast_data_set[0], start_param);
                }
                
                /* Set channel map for this advertising set */
                sl_bt_advertiser_set_channel_map(ext_adv[idx], channel_map);
            }
        }
    }
    
    /* Update RSSI statistics */
    numcst_rssi_calc(platform_uptime_get());
    
    return 1;  /* Continue numcast loop */
}

/**
 * @brief Calculate environment RSSI statistics for all PHYs
 * 
 * Updates average, min, max RSSI values from recent environmental scan.
 */
static void env_rssi_calc(void)
{
    int64_t expire_tm = platform_uptime_get();
    
    for (unsigned int phy_idx = 0; phy_idx < ARRAY_SIZE(env_rssi_rec); phy_idx++) {
        int16_t cnt = 0;
        int32_t avg = 0;
        int8_t lower = 20;
        int8_t upper = -127;
        
        for (unsigned int idx = 0; idx < ARRAY_SIZE(env_rssi_rec[0]); idx++) {
            int64_t rec_tm = env_rssi_rec[phy_idx][idx].expired_tm;
            int8_t rec_rssi = env_rssi_rec[phy_idx][idx].rssi;
            if (expire_tm <= rec_tm) {
                avg = avg + rec_rssi;
                cnt = cnt + 1;
                lower = (lower < rec_rssi) ? lower : rec_rssi;
                upper = (upper > rec_rssi) ? upper : rec_rssi;
            }
        }
        
        if (cnt) {
            avg /= cnt;
        }
        
        env_rssi[phy_idx][0] = avg;
        env_rssi[phy_idx][1] = (20 == lower) ? 0 : lower;
        env_rssi[phy_idx][2] = (-127 == upper) ? 0 : upper;
    }
}

int losstst_envmon(void)
{
    if (!svc_init_success) {
        return -1;
    }
    
    /* Calculate environment RSSI statistics */
    env_rssi_calc();
    
    /* Check if envmon task is still active */
    return (0 != envmon_task_tgr(0)) ? 1 : 0;
}

/**
 * @brief Process received number cast packet
 * 
 * Updates the received number cast value and RSSI tracking when a valid
 * number cast packet is detected.
 * 
 * @param idx PHY index (0=2M, 1=1M, 2=Coded, 3=BLE4)
 * @param form_p Pointer to device info structure (read-only)
 * @param numcast_p Pointer to number cast value (64-bit, read-only)
 * @param rssi RSSI value in dBm
 */
static void numcast_packet_evt(uint8_t idx, const device_info_t *form_p, const uint64_t *numcast_p, int8_t rssi)
{
    if (idx >= 4) {
        return;
    }
    
    /* Copy number cast value (handle unaligned access) */
    memcpy(&number_cast_rxval, numcast_p, sizeof(uint64_t));
    
    /* Update timestamp (expires after 5 seconds) */
    int64_t tm_expire = platform_uptime_get() + 5000;
    numcst_phy_stamp_tm[idx] = tm_expire;
    
    /* Extract source node address (last 2 bytes of EUI-64) */
    /* Use byte-level access to avoid scalar_storage_order byte swap */
    numcst_src_node[0] = *((const uint8_t *)&form_p->eui + 6);
    numcst_src_node[1] = *((const uint8_t *)&form_p->eui + 7);
    
    /* Record RSSI with expiration timestamp */
    rssi_stamp_t loc_rec = {
        .expired_tm = tm_expire,
        .rssi = rssi
    };
    
    numcst_rssi_rec[(numcst_rssi_idx++) & 31] = loc_rec;
}

/**
 * @brief Parse BLE advertising data for number cast packets
 * 
 * This parser callback is invoked by bt_data_parse() to process each
 * advertising data element. It checks for valid number cast format:
 * - FLAGS element first
 * - MANUFACTURER_DATA element second with device info
 * - For extended advertising (1M/2M/Coded): separate MANUFACTURER_DATA with numcast values
 * - For legacy advertising (BLE4): numcast values embedded in device name tail
 * 
 * @param data BLE advertising data element
 * @param user_data Pointer to dev_found_param_t structure
 * @return true to continue parsing, false to stop
 */
static bool numcast_parser(adv_data_t *data, void *user_data)
{
    dev_found_param_t *dev_chr_p = (dev_found_param_t *)user_data;
    
    /* Check for FLAGS element */
    if (BT_DATA_FLAGS == data->type) {
        if (0 == dev_chr_p->step_raw) {
            dev_chr_p->step_flag++;
        } else {
            dev_chr_p->step_fail = 1;
        }
    }
    /* Check for MANUFACTURER_DATA element */
    else if (1 == dev_chr_p->step_flag && BT_DATA_MANUFACTURER_DATA == data->type) {
        dev_chr_p->step_special_stream++;
        
        /* Get RSSI and PHY index */
        int8_t arg_rssi;
        int8_t idx;
        sl_adv_info_t *adv_info_p = dev_chr_p->adv_info_p;
        
        /* Sanitize RSSI value */
        arg_rssi = adv_info_p->rssi;
        arg_rssi = (20 < arg_rssi) ? -128 : arg_rssi;
        
        /* Determine PHY index from primary and secondary PHY */
        if (1 == adv_info_p->prim_phy && 2 == adv_info_p->sec_phy) {
            idx = 0;  /* 2M PHY */
        } else if (1 == adv_info_p->prim_phy && 1 == adv_info_p->sec_phy) {
            idx = 1;  /* 1M PHY */
        } else if (3 == adv_info_p->prim_phy && 3 == adv_info_p->sec_phy) {
            idx = 2;  /* Coded PHY S=8 */
        } else if (1 == adv_info_p->prim_phy && 0 == adv_info_p->sec_phy) {
            idx = 3;  /* BLE4 (legacy advertising) */
        } else {
            dev_chr_p->step_fail = 1;
            return false;
        }
        
        /* Process BLE4 format (number cast in device name tail) */
        if (idx == 3) {
            device_info_bt4_t *rcv_data_p = (device_info_bt4_t *)data->data;
            
            /* Validate manufacturer ID, form ID, and UINT16_MAX marker */
            if (MANUFACTURER_ID == rcv_data_p->device_info.man_id &&
                LOSS_TEST_FORM_ID == rcv_data_p->device_info.form_id &&
                UINT16_MAX == *((uint16_t *)rcv_data_p->tail)) {
                
                /* Number cast value is at offset +2 in tail */
                numcast_packet_evt(idx, &rcv_data_p->device_info,
                                 (uint64_t *)(rcv_data_p->tail + 2), arg_rssi);
                dev_chr_p->step_success = 1;
            } else {
                dev_chr_p->step_fail = 1;
            }
        }
        /* Process extended advertising format (separate numcast element) */
        else {
            if (2 == dev_chr_p->step_special_stream) {
                /* Second MANUFACTURER_DATA should be numcast values */
                numcast_info_t *rcv_data_p = (numcast_info_t *)data->data;
                
                /* Validate we have device info from previous element */
                if (NULL != dev_chr_p->temp_ptr &&
                    0xFF == data->type &&
                    sizeof(numcast_info_t) == data->data_len) {
                    
                    /* Copy to aligned variable to avoid packed struct pointer warning */
                    uint64_t numcast_value;
                    memcpy(&numcast_value, rcv_data_p->number_cast_form, sizeof(uint64_t));
                    numcast_packet_evt(idx, (device_info_t *)dev_chr_p->temp_ptr,
                                     &numcast_value, arg_rssi);
                    dev_chr_p->step_success = 1;
                } else {
                    dev_chr_p->step_fail = 1;
                }
            } else if (1 == dev_chr_p->step_special_stream) {
                /* First MANUFACTURER_DATA should be device info */
                if (data->data_len == sizeof(device_info_t)) {
                    dev_chr_p->temp_ptr = data->data;
                } else {
                    dev_chr_p->step_fail = 1;
                }
            } else {
                dev_chr_p->step_fail = 1;
            }
        }
    } else {
        dev_chr_p->step_fail = 1;
    }
    
    /* Continue parsing if not completed */
    return (dev_chr_p->step_completed) ? false : true;
}

/**
 * @brief Update RSSI statistics with new sample
 * 
 * @param stamp_p Pointer to receive stamp structure
 * @param rssi_val New RSSI value
 */
static void rssi_avg_procedure(rcv_stamp_t *stamp_p, int16_t rssi_val)
{
    stamp_p->rec.rssi_upper = (rssi_val > stamp_p->rec.rssi_upper) ? 
                              rssi_val : stamp_p->rec.rssi_upper;
    stamp_p->rec.rssi_lower = (rssi_val < stamp_p->rec.rssi_lower) ? 
                              rssi_val : stamp_p->rec.rssi_lower;
    stamp_p->rssi_acc += rssi_val;
    stamp_p->rssi_idx += 1;
    stamp_p->rec.rssi = stamp_p->rssi_acc / stamp_p->rssi_idx;
}

/**
 * @brief Initialize RSSI statistics with first sample
 * 
 * @param stamp_p Pointer to receive stamp structure
 * @param rssi_val Initial RSSI value
 */
static void rssi_idx_init(rcv_stamp_t *stamp_p, int16_t rssi_val)
{
    stamp_p->rec.rssi = rssi_val;
    stamp_p->rec.rssi_upper = INT16_MIN;
    stamp_p->rec.rssi_lower = INT16_MAX;
    stamp_p->rssi_acc = rssi_val;
    stamp_p->rssi_idx = 1;
}

/**
 * @brief Process received burst test packet
 * 
 * This function handles burst test packets from remote senders, tracking:
 * - Sender configuration and preset progress
 * - Burst counting during active transmission
 * - RSSI statistics (average, min, max)
 * - Completion status and response acknowledgment
 * 
 * For sender mode: Checks for acknowledgment from remote receiver
 * For scanner mode: Tracks received packets and calculates statistics
 * 
 * @param info_p Pointer to sl_adv_info_t advertising info structure
 * @param form_p Pointer to device info from received packet
 */
static void tst_form_packet_rcv(sl_adv_info_t *info_p, device_info_t *form_p)
{
    rcv_stamp_t rcv_stamp_lc = {
        .rec.rssi_upper = INT16_MIN, 
        .rec.rssi_lower = INT16_MAX
    };
    uint8_t index;
    uint16_t subtotal;
    int16_t info_rssi;
    bool sndinfo_output_req = false;
    bool rcvinfo_output_req = false;

    /* Extract info from advertising event structure */
    /* Copy to avoid taking address of scalar with reverse storage order */
    uint64_t eui_copy = form_p->eui.eui_64;
    rcv_stamp_lc.rec.node = 0xFFFF & eui_copy;
    rcv_stamp_lc.rec.pri_phy = info_p->prim_phy;
    rcv_stamp_lc.rec.sec_phy = info_p->sec_phy;
    info_rssi = (20 < info_p->rssi) ? -128 : info_p->rssi;
    
    /* Determine PHY index from primary and secondary PHY */
    if (1 == rcv_stamp_lc.rec.pri_phy && 2 == rcv_stamp_lc.rec.sec_phy) {
        index = 0;  /* 2M PHY */
    } else if (1 == rcv_stamp_lc.rec.pri_phy && 1 == rcv_stamp_lc.rec.sec_phy) {
        index = 1;  /* 1M PHY */
    } else if (3 == rcv_stamp_lc.rec.pri_phy && 3 == rcv_stamp_lc.rec.sec_phy) {
        index = 2;  /* Coded PHY S=8 */
    } else if (1 == rcv_stamp_lc.rec.pri_phy && 0 == rcv_stamp_lc.rec.sec_phy) {
        index = 3;  /* BLE4 (legacy advertising) */
    } else {
        return;  /* Unknown PHY combination */
    }
    
    /* Check if in sender mode - look for acknowledgment */
    if (0 != sender_task_tgr(0)) {
        if (0 == memcmp((const char *)&device_info_form[index], form_p, 
                       sizeof(device_info_t))) {
            ack_remote_resp[index] = true;
        }
        return;
    }

    /* Check if scanner is active and PHY is enabled */
    if (0 == scanner_task_tgr(0) || scanner_inactive || !round_phy_sel[index]) {
        return;
    }

    rcv_stamp_lc.rec.tx_pwr = info_p->tx_power;
    rcv_stamp_lc.rec.flow = form_p->flw_cnt;

    /* Skip invalid flow count */
    if (201 < rcv_stamp_lc.rec.flow) {
        return;
    }
    /* Handle sender config-preset progress (pre_cnt = INT16_MIN) */
    else if (INT16_MIN == form_p->pre_cnt) {
        /* Check if this is a new sender or flow */
        if (0 != memcmp(&rcv_stamp[index], &rcv_stamp_lc, 
                       sizeof(rcv_stamp_lc.rec.flow) + 
                       offsetof(recv_stats_t, flow))) {
            rcv_stamp_lc.rec.subtotal = 0;
            rcv_stamp_lc.rec.det_sender = 1;
            rssi_idx_init(&rcv_stamp_lc, info_rssi);
            sndinfo_output_req = true;
        } else {
            rcv_stamp_lc.rec.det_sender = 1;
            rssi_avg_procedure(&rcv_stamp_lc, info_rssi);
        }
        
        rcv_stamp[index] = rcv_stamp_lc;
        rec_sets[index] = rcv_stamp[index].rec;
        subtotal = sub_total_rcv[index] = 0;
    }
    /* Handle burst counting (0 < pre_cnt < INT16_MAX) */
    else if (0 < form_p->pre_cnt && INT16_MAX != form_p->pre_cnt) {
        subtotal = ++sub_total_rcv[index];
        rcv_ratio_val[index][0] = subtotal;
        rcv_ratio_val[index][1] = LOSS_TEST_BURST_COUNT * rcv_stamp_lc.rec.flow;
        precnt_rcv[index] = form_p->pre_cnt;
        sndr_id = rcv_stamp_lc.rec.node;
        sndr_txpower = rcv_stamp_lc.rec.tx_pwr;
    }
    /* Handle pre-burst or other states */
    else {
        subtotal = sub_total_rcv[index];
        rcv_ratio_val[index][0] = subtotal;
        rcv_ratio_val[index][1] = LOSS_TEST_BURST_COUNT * rcv_stamp_lc.rec.flow;
        sndr_id = rcv_stamp_lc.rec.node;
        sndr_txpower = rcv_stamp_lc.rec.tx_pwr;
    }

    /* Update RSSI statistics if flow is valid and matches */
    if (0 == rcv_stamp_lc.rec.flow || 201 < rcv_stamp_lc.rec.flow) {
        /* Skip invalid flow */
    } else if (0 == memcmp(&rcv_stamp[index], &rcv_stamp_lc, 
                          offsetof(recv_stats_t, flow))) {
        /* Same sender, same session */
        if (rcv_stamp[index].rec.flow == rcv_stamp_lc.rec.flow) {
            rcv_stamp_lc = rcv_stamp[index];
            rcv_stamp_lc.rec.subtotal = subtotal;
            rssi_avg_procedure(&rcv_stamp_lc, info_rssi);

            /* Check for burst completion */
            if (INT16_MAX == form_p->pre_cnt && !rcv_stamp_lc.rec.complete) {
                /* Sender side completed */
                rcv_stamp_lc.rec.complete = 1;
                rec_sets[index] = rcv_stamp_lc.rec;
            } else if (0 == form_p->pre_cnt) {
                /* Sender side burst completed */
                precnt_rcv[index] = 0;
                remote_resp_form[index] = *form_p;
                if (!rcv_stamp_lc.rec.dump_rcvinfo) {
                    rcv_stamp_lc.rec.dump_rcvinfo = 1;
                    rcvinfo_output_req = true;
                }
                rec_sets[index] = rcv_stamp_lc.rec;
            } else if (0 > form_p->pre_cnt) {
                precnt_rcv[index] = form_p->pre_cnt;
            }
            
            rcv_stamp[index] = rcv_stamp_lc;
            rcv_rssi_val[index][0] = rcv_stamp_lc.rec.rssi;
            rcv_rssi_val[index][1] = (1 >= rcv_stamp_lc.rssi_idx) ? 
                                     rcv_stamp_lc.rec.rssi : 
                                     rcv_stamp_lc.rec.rssi_lower;
            rcv_rssi_val[index][2] = (1 >= rcv_stamp_lc.rssi_idx) ? 
                                     rcv_stamp_lc.rec.rssi : 
                                     rcv_stamp_lc.rec.rssi_upper;
            memcpy(peek_rcv_rssi[index], rcv_rssi_val[index], 
                   sizeof(peek_rcv_rssi[index]));
        } else {
            /* New flow from same sender */
            rcv_rssi_val[index][0] = rcv_stamp[index].rec.rssi = 
                rcv_stamp[index].rssi_acc / rcv_stamp[index].rssi_idx;
            rcv_rssi_val[index][1] = (1 >= rcv_stamp[index].rssi_idx) ? 
                                     rcv_stamp[index].rec.rssi : 
                                     rcv_stamp[index].rec.rssi_lower;
            rcv_rssi_val[index][2] = (1 >= rcv_stamp[index].rssi_idx) ? 
                                     rcv_stamp[index].rec.rssi : 
                                     rcv_stamp[index].rec.rssi_upper;
            memcpy(peek_rcv_rssi[index], rcv_rssi_val[index], 
                   sizeof(peek_rcv_rssi[index]));
            remote_tx_pwr[index] = rcv_stamp[index].rec.tx_pwr;

            if (!rcv_stamp[index].rec.dump_rcvinfo) {
                rcv_stamp[index].rec.dump_rcvinfo = 1;
                rcvinfo_output_req = true;
                rec_sets[index] = rcv_stamp[index].rec;
            }

            rssi_idx_init(&rcv_stamp_lc, info_rssi);
            rcv_stamp[index] = rcv_stamp_lc;
        }
    } else {
        /* New sender detected */
        rssi_idx_init(&rcv_stamp_lc, info_rssi);
        rcv_stamp[index] = rcv_stamp_lc;
        
        /* Log new sender info */
        char *dst_p = ('\0' == *rcv_msg_str[0]) ? rcv_msg_str[0] : 
                     (('\0' == *rcv_msg_str[1]) ? rcv_msg_str[1] : rcv_msg_str[2]);
        snprintf(dst_p, 80, log_sender_form,
                (uint8_t)rcv_stamp_lc.rec.node,
                pri_phy_typ[rcv_stamp_lc.rec.pri_phy],
                sec_phy_typ[rcv_stamp_lc.rec.sec_phy],
                subtotal, rcv_stamp_lc.rec.flow * LOSS_TEST_BURST_COUNT,
                rssi_toa(rcv_stamp_lc.rec.rssi, rssi_str[0]),
                rssi_toa(rcv_stamp_lc.rec.rssi_lower, rssi_str[1]),
                rssi_toa(rcv_stamp_lc.rec.rssi_upper, rssi_str[2]),
                txpwr_toa(rcv_stamp_lc.rec.tx_pwr, tx_pwr_str));
    }

    /* Output receive info if requested */
    if (rcvinfo_output_req) {
        char *dst_p = ('\0' == *rcv_msg_str[0]) ? rcv_msg_str[0] : 
                     (('\0' == *rcv_msg_str[1]) ? rcv_msg_str[1] : rcv_msg_str[2]);
        snprintf(dst_p, 80, log_rcvpkt_form,
                (uint8_t)rec_sets[index].node,
                pri_phy_typ[rec_sets[index].pri_phy],
                sec_phy_typ[rec_sets[index].sec_phy],
                sub_total_rcv[index], rec_sets[index].flow * LOSS_TEST_BURST_COUNT,
                rssi_toa(peek_rcv_rssi[index][0], rssi_str[0]),
                rssi_toa(peek_rcv_rssi[index][1], rssi_str[1]),
                rssi_toa(peek_rcv_rssi[index][2], rssi_str[2]),
                txpwr_toa(rec_sets[index].tx_pwr, tx_pwr_str));
    }

    /* Output sender info if requested */
    if (sndinfo_output_req) {
        char *dst_p = ('\0' == *rcv_msg_str[0]) ? rcv_msg_str[0] : 
                     (('\0' == *rcv_msg_str[1]) ? rcv_msg_str[1] : rcv_msg_str[2]);
        snprintf(dst_p, 80, log_sender_form,
                (uint8_t)rec_sets[index].node,
                pri_phy_typ[rec_sets[index].pri_phy],
                sec_phy_typ[rec_sets[index].sec_phy],
                subtotal, rec_sets[index].flow * LOSS_TEST_BURST_COUNT,
                rssi_toa(rec_sets[index].rssi, rssi_str[0]),
                rssi_toa(rec_sets[index].rssi_lower, rssi_str[1]),
                rssi_toa(rec_sets[index].rssi_upper, rssi_str[2]),
                txpwr_toa(rec_sets[index].tx_pwr, tx_pwr_str));
    }
}

/**
 * @brief Parse BLE advertising data for burst test packets
 * 
 * This parser callback is invoked by bt_data_parse() to process each
 * advertising data element. It checks for valid burst test format:
 * - FLAGS element first
 * - MANUFACTURER_DATA element second with device info and burst data
 * 
 * Validates manufacturer ID and form ID before calling tst_form_packet_rcv()
 * to process the received data.
 * 
 * @param data BLE advertising data element
 * @param user_data Pointer to dev_found_param_t structure
 * @return true to continue parsing, false to stop
 */
static bool test_form_parser(adv_data_t *data, void *user_data)
{
    dev_found_param_t *dev_chr_p = (dev_found_param_t *)user_data;
    
    /* Check for FLAGS element */
    if (BT_DATA_FLAGS == data->type) {
        if (0 == dev_chr_p->step_raw) {
            dev_chr_p->step_flag++;
        } else {
            dev_chr_p->step_fail = 1;
        }
    }
    /* Check for MANUFACTURER_DATA element */
    else if (1 == dev_chr_p->step_flag && BT_DATA_MANUFACTURER_DATA == data->type) {
        device_info_t *rcv_data_p = (device_info_t *)data->data;
        
        /* Validate manufacturer ID and form ID */
        if (MANUFACTURER_ID == rcv_data_p->man_id && 
            LOSS_TEST_FORM_ID == rcv_data_p->form_id) {
            sl_adv_info_t *adv_info_p = dev_chr_p->adv_info_p;
            tst_form_packet_rcv(adv_info_p, rcv_data_p);
            dev_chr_p->step_success = 1;
        } else {
            dev_chr_p->step_fail = 1;
        }
    } else {
        dev_chr_p->step_fail = 1;
    }
    
    /* Continue parsing if not completed */
    return (dev_chr_p->step_completed) ? false : true;
}

/**
 * @brief Silicon Labs implementation of BLE advertising data parser
 * 
 * This function parses BLE advertising data in TLV (Type-Length-Value) format
 * and invokes a callback function for each AD element found.
 * 
 * In Silicon Labs BLE stack, advertising data is provided as a raw byte buffer
 * in TLV format:
 * - Byte 0: Length (includes Type byte, but not Length byte itself)
 * - Byte 1: AD Type
 * - Bytes 2-N: Data (length = Length - 1)
 * 
 * This function is designed to be compatible with the Nordic Zephyr bt_data_parse()
 * API, allowing parser callbacks to work on both platforms.
 * 
 * @param ad_data Pointer to advertising data buffer (TLV format)
 * @param ad_len Length of advertising data buffer in bytes
 * @param callback Parser callback function, called for each AD element
 * @param user_data User data pointer passed to callback
 * @return Number of AD elements parsed, or negative error code
 * 
 * @example
 * static bool my_parser(adv_data_t *data, void *user_data) {
 *     if (data->type == BT_DATA_FLAGS) {
 *         // Process flags
 *     }
 *     return true;  // Continue parsing
 * }
 * 
 * uint8_t ad_buffer[] = { 0x02, 0x01, 0x06, 0x05, 0xFF, 0xFF, 0xFF, 0x00, 0x00 };
 * sl_bt_data_parse(ad_buffer, sizeof(ad_buffer), my_parser, NULL);
 */
static int sl_bt_data_parse(const uint8_t *ad_data, uint16_t ad_len,
                           bool (*callback)(adv_data_t *data, void *user_data),
                           void *user_data)
{
    if (ad_data == NULL || callback == NULL) {
        return -EINVAL;
    }
    
    uint16_t offset = 0;
    int count = 0;
    
    /* Parse advertising data in TLV format */
    while (offset < ad_len) {
        /* Get Length field */
        uint8_t length = ad_data[offset];
        
        /* End of data or padding */
        if (length == 0) {
            break;
        }
        
        /* Check if we have enough data */
        if (offset + 1 + length > ad_len) {
            /* Invalid data - length field exceeds buffer */
            return -EBADMSG;
        }
        
        /* Extract Type and Data */
        uint8_t type = ad_data[offset + 1];
        const uint8_t *data = (length > 1) ? &ad_data[offset + 2] : NULL;
        uint8_t data_len = (length > 1) ? (length - 1) : 0;
        
        /* Create adv_data_t structure for callback */
        adv_data_t ad_element = {
            .type = type,
            .data_len = data_len,
            .data = data
        };
        
        /* Invoke callback */
        bool continue_parsing = callback(&ad_element, user_data);
        count++;
        
        /* Stop parsing if callback returns false */
        if (!continue_parsing) {
            break;
        }
        
        /* Move to next AD element (Length byte + Length field content) */
        offset += 1 + length;
    }
    
    return count;
}

/* ================== Scanner Callback - BLE Packet Reception ================== */

/**
 * @brief BLE scan callback for processing received advertising packets
 * 
 * This is the main entry point for all received BLE advertising packets during
 * scanning. It performs the following:
 * 
 * 1. Extracts PHY information from HCI event data
 * 2. Updates packet statistics for scanner/environment monitor tasks
 * 3. Routes packets to appropriate parser based on active task:
 *    - numcast_parser: For number cast packets
 *    - test_form_parser: For burst test packets
 *    - remote_ctrl_parser: For remote control packets (1M PHY only)
 * 
 * Each parser validates packet structure and calls corresponding event handlers.
 * The function uses net_buf_simple save/restore to allow multiple parse attempts
 * without consuming the buffer.
 * 
 * Platform abstraction note:
 * - Nordic: Uses bt_hci_evt_le_ext_advertising_info for PHY and RSSI extraction
 * - Silicon Labs: TODO - adapt to sl_bt_evt_scanner_legacy_advertisement_report_t
 *                  or sl_bt_evt_scanner_extended_advertisement_report_t
 * 
 * Silicon Labs device found callback for advertising reports
 * This function processes both legacy and extended advertising reports
 * 
 * @param adv_info Pointer to sl_adv_info_t with PHY, RSSI, TX power information
 * @param ad_data Advertising data buffer
 * @param ad_len Advertising data length
 */
static void device_found(sl_adv_info_t *adv_info, const uint8_t *ad_data, uint16_t ad_len)
{
    int8_t idx = -1;
    int8_t rssi = adv_info->rssi;
    
    /* Determine PHY index from primary and secondary PHY */
    if (1 == adv_info->prim_phy && 2 == adv_info->sec_phy) {
        idx = 0;  /* 2M PHY */
    } else if (1 == adv_info->prim_phy && 1 == adv_info->sec_phy) {
        idx = 1;  /* 1M PHY */
    } else if (3 == adv_info->prim_phy && 3 == adv_info->sec_phy) {
        idx = 2;  /* Coded PHY S=8 */
    } else if (1 == adv_info->prim_phy && 0 == adv_info->sec_phy) {
        idx = 3;  /* BLE4 (legacy advertising) */
    } else {
        return;  /* Unknown PHY combination */
    }
    
    /* Update burst test statistics if scanner task is active */
    if (0 != scanner_task_tgr(0)) {
        if (9999999ul < ++rcv_stats[idx]) {
            rcv_stats[idx] = 9999999ul;  /* Cap at max value */
        }
    }
    
    /* Update environment monitor statistics if envmon task is active */
    if (0 != envmon_task_tgr(0)) {
        int8_t rssi_clamped = (20 < rssi) ? 20 : rssi;  /* Clamp invalid RSSI */
        rssi_stamp_t loc_rec = {
            .expired_tm = platform_uptime_get() + 60000,  /* 60 second expiry */
            .rssi = rssi_clamped
        };
        env_rssi_rec[idx][(ARRAY_SIZE(env_rssi_rec[idx]) - 1) & env_rssi_idx[idx]++] = loc_rec;
        
        if (9999999ul < ++env_stats[idx]) {
            env_stats[idx] = 9999999ul;  /* Cap at max value */
        }
    }
    
    /* Initialize parser state */
    dev_chr.flw_cnt++;
    dev_chr.step_raw = 0;
    dev_chr.adv_info_p = adv_info;
    
    /* Try number cast parser if numcast task is active */
    if (0 != numcst_task_tgr(0)) {
        dev_chr.step_raw = 0;
        sl_bt_data_parse(ad_data, ad_len, numcast_parser, &dev_chr);
        if (dev_chr.step_success) {
            return;  /* Successfully parsed as numcast packet */
        }
    }
    
    /* Try burst test parser if sender or scanner task is active */
    if (0 != sender_task_tgr(0) || 0 != scanner_task_tgr(0)) {
        dev_chr.step_raw = 0;
        sl_bt_data_parse(ad_data, ad_len, test_form_parser, &dev_chr);
        if (dev_chr.step_success) {
            return;  /* Successfully parsed as burst test packet */
        }
    }
    
    /* Try remote control parser (only for 1M PHY) */
    /* TODO: Implement remote_ctrl_parser if needed
    if (1 == idx) {
        dev_chr.step_raw = 0;
        sl_bt_data_parse(ad_data, ad_len, remote_ctrl_parser, &dev_chr);
    }
    */
}

/* Silicon Labs helper functions for scan event handling */

/**
 * @brief Silicon Labs device found callback for legacy advertising
 * 
 * @param addr Device address (bd_addr*)
 * @param rssi RSSI value in dBm
 * @param ad_data Advertising data buffer
 * @param ad_len Advertising data length
 */
static void device_found_legacy(const bd_addr *addr, int8_t rssi,
                               const uint8_t *ad_data, uint16_t ad_len)
{
    /* Create advertising info structure for legacy advertising (BLE4) */
    sl_adv_info_t adv_info = {
        .rssi = rssi,
        .tx_power = 127,  /* Unknown TX power */
        .prim_phy = 1,    /* 1M PHY */
        .sec_phy = 0,     /* No secondary PHY (legacy) */
        .address_type = 0,
        .address = *addr
    };
    
    /* Call unified device_found function */
    device_found(&adv_info, ad_data, ad_len);
}

/**
 * @brief Silicon Labs device found callback for extended advertising
 * 
 * @param addr Device address (bd_addr*)
 * @param rssi RSSI value in dBm
 * @param tx_power TX power in dBm
 * @param prim_phy Primary PHY (1=1M, 3=Coded)
 * @param sec_phy Secondary PHY (1=1M, 2=2M, 3=Coded)
 * @param ad_data Advertising data buffer
 * @param ad_len Advertising data length
 */
static void device_found_extended(const bd_addr *addr, int8_t rssi, int8_t tx_power,
                                 uint8_t prim_phy, uint8_t sec_phy,
                                 const uint8_t *ad_data, uint16_t ad_len)
{
    /* Create advertising info structure for extended advertising */
    sl_adv_info_t adv_info = {
        .rssi = rssi,
        .tx_power = tx_power,
        .prim_phy = prim_phy,
        .sec_phy = sec_phy,
        .address_type = 0,
        .address = *addr
    };
    
    /* Call unified device_found function */
    device_found(&adv_info, ad_data, ad_len);
}

/**
 * @brief Silicon Labs scan event handler helper
 * 
 * This function should be called from your sl_bt_on_event() handler
 * when receiving scan advertisement reports:
 * 
 * @example
 * void sl_bt_on_event(sl_bt_msg_t *evt)
 * {
 *     switch (SL_BT_MSG_ID(evt->header)) {
 *         case sl_bt_evt_scanner_legacy_advertisement_report_id: {
 *             sl_bt_evt_scanner_legacy_advertisement_report_t *scan_evt = 
 *                 &evt->data.evt_scanner_legacy_advertisement_report;
 *             
 *             sl_bt_scanner_process_legacy_report(
 *                 &scan_evt->address,
 *                 scan_evt->rssi,
 *                 scan_evt->data.data,
 *                 scan_evt->data.len
 *             );
 *             break;
 *         }
 *         
 *         case sl_bt_evt_scanner_extended_advertisement_report_id: {
 *             sl_bt_evt_scanner_extended_advertisement_report_t *scan_evt = 
 *                 &evt->data.evt_scanner_extended_advertisement_report;
 *             
 *             sl_bt_scanner_process_extended_report(
 *                 &scan_evt->address,
 *                 scan_evt->rssi,
 *                 scan_evt->tx_power,
 *                 scan_evt->primary_phy,
 *                 scan_evt->secondary_phy,
 *                 scan_evt->data.data,
 *                 scan_evt->data.len
 *             );
 *             break;
 *         }
 *     }
 * }
 */
void sl_bt_scanner_process_legacy_report(const bd_addr *addr, int8_t rssi,
                                        const uint8_t *ad_data, uint16_t ad_len)
{
    device_found_legacy(addr, rssi, ad_data, ad_len);
}

void sl_bt_scanner_process_extended_report(const bd_addr *addr, int8_t rssi, int8_t tx_power,
                                          uint8_t prim_phy, uint8_t sec_phy,
                                          const uint8_t *ad_data, uint16_t ad_len)
{
    device_found_extended(addr, rssi, tx_power, prim_phy, sec_phy, ad_data, ad_len);
}