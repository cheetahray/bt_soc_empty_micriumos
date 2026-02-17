/**
 * @file update_adv_port.c
 * @brief Portable BLE Advertisement Update Module for Silicon Labs
 * 
 * This module provides the implementation of portable BLE extended advertising
 * functionality that can be adapted from Nordic nRF52 to Silicon Labs platforms.
 */

#include "update_adv_port.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* ================== Debug Configuration ================== */
#define CHK_UPDATE_ADV_PROCEDURE 0  /* Set to 1 to enable debug prints */

#if CHK_UPDATE_ADV_PROCEDURE
    #define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
#endif

/* ================== Internal State Variables ================== */

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
#ifdef PLATFORM_NORDIC
    static struct bt_le_ext_adv *ext_adv[MAX_ADV_SETS];
#elif defined(PLATFORM_SILABS)
    static adv_handle_t ext_adv[MAX_ADV_SETS];
#endif

/* Default advertising start parameters */
static const adv_start_param_t p_adv_default_start_param = {
    .timeout = 0,      /* 0 = no timeout, advertise continuously */
    .max_events = 0    /* 0 = no limit on advertising events */
};

/* ================== Platform Abstraction Functions ================== */

#ifdef PLATFORM_NORDIC
    /* Nordic nRF52 platform implementations */
    #include <zephyr/bluetooth/bluetooth.h>
    #include <zephyr/bluetooth/conn.h>
    
    static int platform_create_adv_set(const adv_param_t *param, 
                                       struct bt_le_ext_adv **adv)
    {
        return bt_le_ext_adv_create(param, NULL, adv);
    }
    
    static int platform_update_adv_param(struct bt_le_ext_adv *adv,
                                         const adv_param_t *param)
    {
        return bt_le_ext_adv_update_param(adv, param);
    }
    
    static int platform_set_adv_data(struct bt_le_ext_adv *adv,
                                     adv_data_t *data, uint8_t data_len)
    {
        return bt_le_ext_adv_set_data(adv, data, data_len, NULL, 0);
    }
    
    static int platform_start_adv(struct bt_le_ext_adv *adv,
                                  const adv_start_param_t *param)
    {
        return bt_le_ext_adv_start(adv, param);
    }
    
    static int platform_stop_adv(struct bt_le_ext_adv *adv)
    {
        return bt_le_ext_adv_stop(adv);
    }
    
#elif defined(PLATFORM_SILABS)
    /* Silicon Labs platform implementations */
    /* TODO: Implement Silicon Labs-specific functions using sl_bt_advertiser_* APIs */
    
    static int platform_create_adv_set(const adv_param_t *param, 
                                       adv_handle_t *handle)
    {
        /* Example Silicon Labs implementation:
         * sl_status_t status;
         * status = sl_bt_advertiser_create_set(handle);
         * if (status == SL_STATUS_OK) {
         *     status = sl_bt_advertiser_set_timing(*handle, 
         *                                          param->interval_min,
         *                                          param->interval_max,
         *                                          0, 0);
         * }
         * return (status == SL_STATUS_OK) ? 0 : -EIO;
         */
        return -ENOTSUP;  /* Not yet implemented */
    }
    
    static int platform_update_adv_param(adv_handle_t handle,
                                         const adv_param_t *param)
    {
        /* Example Silicon Labs implementation:
         * sl_status_t status;
         * status = sl_bt_advertiser_set_timing(handle,
         *                                      param->interval_min,
         *                                      param->interval_max,
         *                                      0, 0);
         * if (status == SL_STATUS_OK) {
         *     status = sl_bt_extended_advertiser_set_phy(handle,
         *                                                param->primary_phy,
         *                                                param->secondary_phy);
         * }
         * return (status == SL_STATUS_OK) ? 0 : -EIO;
         */
        return -ENOTSUP;  /* Not yet implemented */
    }
    
    static int platform_set_adv_data(adv_handle_t handle,
                                     adv_data_t *data, uint8_t data_len)
    {
        /* Example Silicon Labs implementation:
         * sl_status_t status;
         * uint8_t adv_data[256];
         * uint16_t offset = 0;
         * 
         * // Build advertising data packet
         * for (uint8_t i = 0; i < data_len; i++) {
         *     adv_data[offset++] = data[i].data_len + 1;  // Length
         *     adv_data[offset++] = data[i].type;          // Type
         *     memcpy(&adv_data[offset], data[i].data, data[i].data_len);
         *     offset += data[i].data_len;
         * }
         * 
         * status = sl_bt_extended_advertiser_set_data(handle, offset, adv_data);
         * return (status == SL_STATUS_OK) ? 0 : -EIO;
         */
        return -ENOTSUP;  /* Not yet implemented */
    }
    
    static int platform_start_adv(adv_handle_t handle,
                                  const adv_start_param_t *param)
    {
        /* Example Silicon Labs implementation:
         * sl_status_t status;
         * uint16_t duration = (param->timeout == 0) ? 0 : param->timeout;
         * uint8_t max_events = (param->max_events > 255) ? 255 : param->max_events;
         * 
         * status = sl_bt_advertiser_start(handle,
         *                                 sl_bt_advertiser_non_connectable,
         *                                 sl_bt_advertiser_non_discoverable);
         * return (status == SL_STATUS_OK) ? 0 : -EIO;
         */
        return -ENOTSUP;  /* Not yet implemented */
    }
    
    static int platform_stop_adv(adv_handle_t handle)
    {
        /* Example Silicon Labs implementation:
         * sl_status_t status = sl_bt_advertiser_stop(handle);
         * return (status == SL_STATUS_OK) ? 0 : -EIO;
         */
        return -ENOTSUP;  /* Not yet implemented */
    }
    
#endif

/* ================== Public API Implementation ================== */

int update_adv_port_init(const uint8_t *device_addr)
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
        /* TODO: Set appropriate default parameters based on index */
        adv_param_t default_param = {
            .interval_min = PARAM_ADV_INT_MIN_0,
            .interval_max = PARAM_ADV_INT_MAX_0,
            .primary_phy = 1,    /* 1M PHY */
            .secondary_phy = 2,  /* 2M PHY */
            .options = 0
        };
        
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
        
        err = platform_start_adv(ext_adv[index], start_param);
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
