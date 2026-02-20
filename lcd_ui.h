/**
 * @file lcd_ui.h
 * @brief LCD User Interface for Loss Test Configuration Display
 * 
 * This module provides LCD display functionality for BRD4002A WSTK
 * to show BLE Loss Test configuration and status in real-time.
 * 
 * Features:
 * - Display test mode (Sender/Scanner/NumCast/EnvMon)
 * - Show PHY configuration (2M/1M/S8/BLE4)
 * - Real-time progress bar
 * - RSSI monitoring
 * - TX power and interval display
 * 
 * Hardware:
 * - BRD4002A WSTK
 * - LS013B7DH03 Memory LCD (128x128)
 * 
 * @note Requires GLIB and DMD components installed via Simplicity Studio
 */

#ifndef LCD_UI_H
#define LCD_UI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Public Functions ==================== */

/**
 * @brief Initialize LCD display
 * 
 * Initializes DMD driver and GLIB graphics context.
 * Must be called once at startup before any other LCD functions.
 * 
 * @return 0 on success, negative error code on failure
 *         -1: DMD_init() failed
 *         -2: GLIB_contextInit() failed
 * 
 * @note Call this AFTER sl_system_init() and BEFORE starting RTOS
 */
int lcd_ui_init(void);

/**
 * @brief Update display with current test parameters
 * 
 * Displays:
 * - Test mode name
 * - Status (Ready/Running/Complete/Error)
 * - TX power level
 * - Advertising interval
 * - Packet count (for Sender mode)
 * - PHY configuration
 * - Channel inhibit settings
 * - Optional flags
 * 
 * @param param Pointer to test_param_t structure (from losstst_svc.h)
 * @param test_mode Test mode string:
 *                  - "Sender"
 *                  - "Scanner"
 *                  - "NumCast"
 *                  - "EnvMon"
 * @param status Status string:
 *               - "Ready" - Test configured, waiting to start
 *               - "Running" - Test in progress
 *               - "Complete" - Test finished successfully
 *               - "Error" - Test encountered error
 * 
 * @note This function updates the entire screen. Call only when
 *       configuration changes, not for every packet.
 */
void lcd_ui_update(const void *param, const char *test_mode, const char *status);

/**
 * @brief Display startup screen with configuration
 * 
 * Shows:
 * - Application name
 * - Default configuration parameters
 * 
 * @param param Pointer to test_param_t structure to display configuration
 *              (NULL to show basic startup only)
 *              Note: Non-const to allow Button 1 editing functionality
 * 
 * @note Call this during app_init() after lcd_ui_init() and load_parm_cfg()
 */
void lcd_ui_show_startup(void *param);

/**
 * @brief Display test progress
 * 
 * Updates the bottom portion of screen with:
 * - Current/Total packet count
 * - Progress bar (0-100%)
 * - RSSI value (if non-zero)
 * 
 * @param current Current packet count
 * @param total Total packet count (0 for continuous mode)
 * @param rssi Current RSSI value in dBm (0 to hide)
 * 
 * @note This function only updates the progress area (bottom 1/4 of screen)
 *       to minimize SPI traffic. Safe to call every 100-500ms.
 * 
 * @example
 * // Sender mode: show packets sent
 * lcd_ui_show_progress(523, 1000, 0);
 * 
 * // Scanner mode: show packets received with RSSI
 * lcd_ui_show_progress(125, 1000, -65);
 */
void lcd_ui_show_progress(uint32_t current, uint32_t total, int8_t rssi);

/**
 * @brief Clear LCD screen
 * 
 * Clears entire display to white background.
 * Useful when entering low-power mode or switching contexts.
 */
void lcd_ui_clear(void);

/**
 * @brief Check if LCD is initialized
 * 
 * @return true if LCD is ready for use, false otherwise
 */
bool lcd_ui_is_ready(void);

/* ==================== Selection Control (Button Navigation) ==================== */

/**
 * @brief Move selection cursor to next item
 * 
 * Call this when Button 0 is pressed to move the triangle indicator
 * to the next configuration item. Wraps around after last item.
 * 
 * @note This updates internal state and triggers LCD refresh
 */
void lcd_ui_next_selection(void);

/**
 * @brief Expand current item to show sub-options
 * 
 * Call this when Button 1 is pressed to enter sub-menu for the
 * currently selected item. Shows all available options for that item.
 * Last option is always "Back" to return to main menu.
 * 
 * @note If item has no sub-options, this does nothing
 */
void lcd_ui_expand_selection(void);

/**
 * @brief Get current selection index
 * 
 * @return Current selected item index (0-based)
 */
uint8_t lcd_ui_get_selection(void);

/**
 * @brief Reset selection to first item
 */
void lcd_ui_reset_selection(void);

/* ==================== Advanced Functions (Optional) ==================== */

/**
 * @brief Display error message
 * 
 * Shows centered error message on screen.
 * 
 * @param error_msg Error message string (max 20 chars)
 * @param error_code Numeric error code (displayed in hex)
 */
void lcd_ui_show_error(const char *error_msg, int error_code);

/**
 * @brief Display connection status
 * 
 * Shows BLE connection state at top-right corner.
 * 
 * @param connected true = show "CONNECTED" icon, false = hide
 */
void lcd_ui_show_connection(bool connected);

#ifdef __cplusplus
}
#endif

#endif // LCD_UI_H
