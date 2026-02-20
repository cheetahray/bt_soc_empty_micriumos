/***************************************************************************//**
 * @file ble_log.h
 * @brief BLE Log Service - Send printf output to connected phone via BLE GATT
 *******************************************************************************
 * Provides functionality to send log messages to a connected BLE device
 * via GATT notifications or writes.
 ******************************************************************************/

#ifndef BLE_LOG_H
#define BLE_LOG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum log message length */
#define BLE_LOG_MAX_LENGTH 244  // Max notification length for BLE

/* Configuration */
#define BLE_LOG_UART_ENABLE     1   // 1: Also output to UART, 0: BLE only
#define BLE_LOG_CACHE_ENABLE    1   // 1: Cache logs when not connected

/**
 * @brief Initialize BLE log service
 * 
 * Call this during app initialization.
 */
void ble_log_init(void);

/**
 * @brief Set connection handle for BLE log transmission
 * 
 * Call this when a BLE connection is established.
 * 
 * @param connection Connection handle from sl_bt_evt_connection_opened_id
 * @param characteristic GATT characteristic handle for log transmission
 */
void ble_log_set_connection(uint8_t connection, uint16_t characteristic);

/**
 * @brief Clear connection (call on disconnect)
 */
void ble_log_clear_connection(void);

/**
 * @brief Check if BLE log is connected
 * 
 * @return true if connected and ready to send logs
 */
bool ble_log_is_connected(void);

/**
 * @brief Send a log message via BLE (and optionally UART)
 * 
 * This is the internal function. Use BLE_PRINTF macro instead.
 * 
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void ble_log_printf(const char *format, ...);

/**
 * @brief Process any cached log messages
 * 
 * Call this periodically or after connection established to flush cache.
 */
void ble_log_process_cache(void);

/**
 * @brief Macro for printf that outputs to both UART and BLE
 * 
 * Use this instead of printf() to send logs to connected phone.
 * Falls back to UART-only if not connected.
 */
#define BLE_PRINTF(...)  ble_log_printf(__VA_ARGS__)

/**
 * @brief Macro for debug prints (same as BLE_PRINTF)
 */
#define DEBUG_BLE_PRINT(...)  ble_log_printf(__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // BLE_LOG_H
