/**
 * @file losstst_svc.c
 * @brief Portable BLE Advertisement Update Module for Silicon Labs
 * 
 * This module provides the implementation of portable BLE extended advertising
 * functionality that can be adapted from Nordic nRF52 to Silicon Labs platforms.
 */

#include "losstst_svc.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* Silicon Labs SDK headers */
#include "sl_status.h"
#include "sl_bt_api.h"

/* ================== Debug Configuration ================== */
#define CHK_UPDATE_ADV_PROCEDURE 0  /* Set to 1 to enable debug prints */

#if CHK_UPDATE_ADV_PROCEDURE
    #define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
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
static bool rc_party;
static const adv_start_param_t p_adv_finit_start_param[]=BT_LE_EXT_ADV_START_PARAM(300,0);
static const adv_start_param_t p_adv_1sec_start_param[]=BT_LE_EXT_ADV_START_PARAM(100,0);
static const adv_start_param_t p_adv_5sec_start_param[]=BT_LE_EXT_ADV_START_PARAM(500,0);
static const adv_start_param_t p_adv_burst_start_param[]=BT_LE_EXT_ADV_START_PARAM(0,LOSS_TEST_BURST_COUNT);

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

/* Default advertising start parameters */
static const adv_start_param_t p_adv_default_start_param = {
    .timeout = 0,      /* 0 = no timeout, advertise continuously */
    .num_events = 0    /* 0 = no limit on advertising events */
};

/* ================== Platform Abstraction Functions ================== */

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
        uint8_t sl_flags;
        bool use_extended_adv;
        bool use_legacy_adv;
        int16_t actual_tx_power;
        
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
            
            // Get flags (anonymous, tx_power)
            sl_flags = get_silabs_adv_flags(param->options);
            
            // Note: flags will be used in platform_start_adv()
            
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
        int16_t actual_tx_power;
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
                                  const adv_start_param_t *param,
                                  uint16_t options)
    {
        sl_status_t status;
        bool use_extended_adv = (options & BT_LE_ADV_OPT_EXT_ADV) != 0;
        bool is_connectable = (options & BT_LE_ADV_OPT_CONNECTABLE) != 0;
        uint8_t discoverable = sl_bt_advertiser_non_discoverable;
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
    
    sprintf(adv_dev_nm[0], "LossTst(%03u)", node_id);
    sprintf(adv_dev_nm[1], "LossTst(%03u)", node_id);
    sprintf(adv_dev_nm[2], "LossTst(%03u)", node_id);
    sprintf(adv_dev_nm[3], "LossTst%03u", node_id);
    sprintf(adv_dev_nm[4], "%s(PEEK %03u)", DEFAULT_DEVICE_NAME, node_id);
    
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
        const adv_start_param_t *start_param = adv_start_param 
                                             ? adv_start_param 
                                             : &p_adv_default_start_param;
        
        /* Silicon Labs needs options to determine extended/legacy and flags */
        err = platform_start_adv(ext_adv[index], start_param, 
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

void adv_sent_callback(adv_handle_t *adv_handle, uint16_t num_sent)
{
    /* Platform-specific callback handler */
    /* This can be used to track advertising packet transmission */
    DEBUG_PRINT("Adv sent: %d packets\n", num_sent);
}

int set_adv_tx_power(uint8_t index, int16_t power, int16_t *set_power)
{
    if (index >= MAX_ADV_SETS) {
        return -EINVAL;
    }
    
    if (!ext_adv_status[index].initialized) {
        return -EPERM;  /* Advertising set not created yet */
    }
    
    /* Silicon Labs implementation */
    sl_status_t status;
    status = sl_bt_advertiser_set_tx_power(ext_adv[index], power, set_power);
    return (status == SL_STATUS_OK) ? 0 : -EIO;
}

void sender_finit(void)
{
	// 啟動廣播
    adv_param_t work_adv_param;

	for(int idx=0; idx<4 ;idx++) {
		if(round_phy_sel[idx]) {
			sndr_abort_flag[idx]=true;
			device_info_form[idx].pre_cnt=INT16_MAX;
			if(3==idx) device_info_bt4_form.device_info=device_info_form[3];
			work_adv_param=*non_connectable_adv_param_x[3][idx];
			work_adv_param.options|=adv_param_mask[1];
			work_adv_param.options&=~adv_param_mask[0];
			update_adv(idx, &work_adv_param, ratio_test_data_set[idx], p_adv_finit_start_param);
		}
	}

	if(!rc_party) update_adv(4,NULL,NULL,NULL);
}
