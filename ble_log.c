/***************************************************************************//**
 * @file ble_log.c
 * @brief BLE Log Service Implementation
 ******************************************************************************/

#include "ble_log.h"
#include "sl_bt_api.h"
#include <stdio.h>
#include <string.h>

/* Connection state */
static struct {
    uint8_t connection;
    uint16_t characteristic;
    bool connected;
} ble_log_state = {0};

/* Log cache for messages sent before connection */
#if BLE_LOG_CACHE_ENABLE
#define LOG_CACHE_SIZE 5
#define LOG_CACHE_ENTRY_SIZE BLE_LOG_MAX_LENGTH

static struct {
    char messages[LOG_CACHE_SIZE][LOG_CACHE_ENTRY_SIZE];
    uint16_t lengths[LOG_CACHE_SIZE];
    uint8_t write_idx;
    uint8_t read_idx;
    uint8_t count;
} log_cache = {0};

static void cache_log_message(const char *msg, uint16_t len);
#endif

/* Internal send buffer */
static char send_buffer[BLE_LOG_MAX_LENGTH];

void ble_log_init(void)
{
    ble_log_state.connection = 0;
    ble_log_state.characteristic = 0;
    ble_log_state.connected = false;
    
#if BLE_LOG_CACHE_ENABLE
    memset(&log_cache, 0, sizeof(log_cache));
#endif
}

void ble_log_set_connection(uint8_t connection, uint16_t characteristic)
{
    ble_log_state.connection = connection;
    ble_log_state.characteristic = characteristic;
    ble_log_state.connected = true;
    
#if BLE_LOG_UART_ENABLE
    printf("[BLE LOG] Connected to client (conn=%d, char=0x%04X)\n", 
           connection, characteristic);
#endif

#if BLE_LOG_CACHE_ENABLE
    /* Flush any cached messages */
    ble_log_process_cache();
#endif
}

void ble_log_clear_connection(void)
{
#if BLE_LOG_UART_ENABLE
    if (ble_log_state.connected) {
        printf("[BLE LOG] Disconnected\n");
    }
#endif
    
    ble_log_state.connected = false;
    ble_log_state.connection = 0;
    ble_log_state.characteristic = 0;
}

bool ble_log_is_connected(void)
{
    return ble_log_state.connected;
}

void ble_log_printf(const char *format, ...)
{
    va_list args;
    int len;
    
    /* Format the message */
    va_start(args, format);
    len = vsnprintf(send_buffer, sizeof(send_buffer), format, args);
    va_end(args);
    
    if (len <= 0) {
        return;
    }
    
    /* Ensure null termination and limit length */
    if (len >= sizeof(send_buffer)) {
        len = sizeof(send_buffer) - 1;
    }
    send_buffer[len] = '\0';
    
#if BLE_LOG_UART_ENABLE
    /* Output to UART */
    printf("%s", send_buffer);
#endif
    
    /* Send via BLE if connected */
    if (ble_log_state.connected && ble_log_state.characteristic != 0) {
        sl_status_t sc;
        
        /* Send as GATT notification */
        sc = sl_bt_gatt_server_send_notification(
            ble_log_state.connection,
            ble_log_state.characteristic,
            len,
            (uint8_t *)send_buffer
        );
        
        if (sc != SL_STATUS_OK) {
            /* If send fails, connection may be lost */
            if (sc == SL_STATUS_INVALID_PARAMETER || sc == SL_STATUS_INVALID_STATE) {
                ble_log_clear_connection();
            }
#if BLE_LOG_CACHE_ENABLE
            /* Cache the message for later retry */
            cache_log_message(send_buffer, len);
#endif
        }
    } else {
#if BLE_LOG_CACHE_ENABLE
        /* Not connected - cache the message */
        cache_log_message(send_buffer, len);
#endif
    }
}

#if BLE_LOG_CACHE_ENABLE
static void cache_log_message(const char *msg, uint16_t len)
{
    if (len == 0 || len >= LOG_CACHE_ENTRY_SIZE) {
        return;
    }
    
    /* If cache is full, overwrite oldest message */
    if (log_cache.count >= LOG_CACHE_SIZE) {
        log_cache.read_idx = (log_cache.read_idx + 1) % LOG_CACHE_SIZE;
        log_cache.count--;
    }
    
    /* Store message */
    memcpy(log_cache.messages[log_cache.write_idx], msg, len);
    log_cache.messages[log_cache.write_idx][len] = '\0';
    log_cache.lengths[log_cache.write_idx] = len;
    
    log_cache.write_idx = (log_cache.write_idx + 1) % LOG_CACHE_SIZE;
    log_cache.count++;
}

void ble_log_process_cache(void)
{
    if (!ble_log_state.connected || log_cache.count == 0) {
        return;
    }
    
    /* Send all cached messages */
    while (log_cache.count > 0) {
        sl_status_t sc;
        uint8_t idx = log_cache.read_idx;
        
        sc = sl_bt_gatt_server_send_notification(
            ble_log_state.connection,
            ble_log_state.characteristic,
            log_cache.lengths[idx],
            (uint8_t *)log_cache.messages[idx]
        );
        
        if (sc != SL_STATUS_OK) {
            /* Failed to send, stop processing cache */
            break;
        }
        
        /* Message sent successfully, advance */
        log_cache.read_idx = (log_cache.read_idx + 1) % LOG_CACHE_SIZE;
        log_cache.count--;
        
        /* Small delay between cached messages to avoid overwhelming BLE stack */
        /* Note: In real implementation, might want to use a timer or rate limiting */
    }
    
#if BLE_LOG_UART_ENABLE
    if (log_cache.count == 0) {
        printf("[BLE LOG] Cache flushed\n");
    }
#endif
}
#else
void ble_log_process_cache(void)
{
    /* Cache disabled - nothing to do */
}
#endif
