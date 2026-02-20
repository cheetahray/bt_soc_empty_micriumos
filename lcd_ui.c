/**
 * @file lcd_ui.c
 * @brief LCD UI Implementation for BLE Loss Test
 * 
 * This module implements the LCD display functionality for showing
 * BLE Loss Test configuration and real-time status on BRD4002A LCD.
 * 
 * @note Before building, ensure these components are installed:
 *       - dmd_memlcd (Display Driver)
 *       - glib (Graphics Library)
 *       - memlcd_eusart or memlcd_usart (SPI interface)
 */

#include "lcd_ui.h"
#include "losstst_svc.h"

// Uncomment after installing LCD components in Simplicity Studio
#include "glib.h"
#include "dmd.h"

#include <stdio.h>
#include <string.h>

/* ==================== Configuration ==================== */

// Set to 1 to enable debug output via DEBUG_BLE_PRINT
#define LCD_DEBUG 1

#if LCD_DEBUG
#include "ble_log.h"
#define LCD_PRINT(...) DEBUG_BLE_PRINT(__VA_ARGS__)
#else
#define LCD_PRINT(...)
#endif

/* ==================== Private Variables ==================== */

// Uncomment after installing GLIB component
// static GLIB_Context_t glibContext;
static bool lcd_initialized = false;

/* ==================== Helper Functions ==================== */

/**
 * @brief Draw text at specified position
 * 
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-127)
 * @param text Text string to display
 */
static void draw_text(uint8_t x, uint8_t y, const char *text)
{
    // Uncomment after installing GLIB component
    // GLIB_drawString(&glibContext, text, strlen(text), x, y, false);
    (void)x;
    (void)y;
    (void)text;
}

/**
 * @brief Draw PHY enable status
 * 
 * Displays enabled PHY types horizontally (e.g., "PHY: 2M 1M S8")
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param param Test parameters containing PHY flags
 */
static void draw_phy_status(uint8_t x, uint8_t y, const test_param_t *param)
{
    char buf[32];
    
    snprintf(buf, sizeof(buf), "PHY:");
    draw_text(x, y, buf);
    
    uint8_t offset = 25;
    if (param->phy_2m) {
        draw_text(x + offset, y, "2M");
        offset += 15;
    }
    if (param->phy_1m) {
        draw_text(x + offset, y, "1M");
        offset += 15;
    }
    if (param->phy_s8) {
        draw_text(x + offset, y, "S8");
        offset += 15;
    }
    if (param->phy_ble4) {
        draw_text(x + offset, y, "BLE4");
    }
}

/**
 * @brief Convert TX power index to human-readable string
 * 
 * @param txpwr_idx TX power index from test_param_t
 * @return String representation (e.g., "+10dBm")
 * 
 * @note Adjust mapping based on your hardware capabilities
 */
static const char* get_txpwr_string(int8_t txpwr_idx)
{
    // EFR32MG27 typical TX power levels
    switch(txpwr_idx) {
        case -3: return "-20dBm";
        case -2: return "-10dBm";
        case -1: return "-5dBm";
        case 0:  return "0dBm";
        case 1:  return "+5dBm";
        case 2:  return "+10dBm";
        default: return "?dBm";
    }
}

/**
 * @brief Convert interval index to millisecond string
 * 
 * @param idx Advertising interval index from test_param_t
 * @return String representation (e.g., "50ms")
 */
static const char* get_interval_string(uint8_t idx)
{
    switch(idx) {
        case 0: return "20ms";
        case 1: return "50ms";
        case 2: return "100ms";
        case 3: return "200ms";
        case 4: return "500ms";
        default: return "?ms";
    }
}

/**
 * @brief Convert count index to packet count string
 * 
 * @param idx Count index from test_param_t
 * @return String representation (e.g., "1000")
 */
static const char* get_count_string(uint8_t idx)
{
    switch(idx) {
        case 0: return "100";
        case 1: return "1000";
        case 2: return "10000";
        default: return "?";
    }
}

/* ==================== Public Functions ==================== */

int lcd_ui_init(void)
{
    LCD_PRINT("[LCD] Initializing display...\n");
    
    /* Initialize DMD (Display Module Driver) */
    // Uncomment after installing dmd_memlcd component
    /*
    sl_status_t status = DMD_init(0);
    if (status != DMD_OK) {
        LCD_PRINT("[LCD] DMD_init() failed: 0x%X\n", (unsigned int)status);
        return -1;
    }
    */
    
    /* Initialize GLIB (Graphics Library) context */
    // Uncomment after installing glib component
    /*
    status = GLIB_contextInit(&glibContext);
    if (status != GLIB_OK) {
        LCD_PRINT("[LCD] GLIB_contextInit() failed: 0x%X\n", (unsigned int)status);
        return -2;
    }
    */
    
    /* Configure display appearance */
    // Uncomment after installing glib component
    /*
    glibContext.backgroundColor = White;
    glibContext.foregroundColor = Black;
    GLIB_clear(&glibContext);
    DMD_updateDisplay();
    */
    
    lcd_initialized = true;
    LCD_PRINT("[LCD] Display initialized successfully\n");
    return 0;
}

void lcd_ui_show_startup(void)
{
    if (!lcd_initialized) {
        LCD_PRINT("[LCD] Not initialized, skipping startup screen\n");
        return;
    }
    
    LCD_PRINT("[LCD] Showing startup screen\n");
    
    // Uncomment after installing GLIB component
    /*
    GLIB_clear(&glibContext);
    
    // Set font
    GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNarrow6x8);
    
    // Title
    draw_text(10, 20, "BLE Loss Test");
    draw_text(10, 35, "Silicon Labs");
    draw_text(10, 50, "Version 1.0");
    
    // Status
    draw_text(10, 70, "Initializing...");
    
    DMD_updateDisplay();
    */
}

void lcd_ui_update(const void *param_ptr, const char *test_mode, const char *status)
{
    if (!lcd_initialized || !param_ptr) {
        LCD_PRINT("[LCD] Cannot update: init=%d, param=%p\n", lcd_initialized, param_ptr);
        return;
    }
    
    const test_param_t *param = (const test_param_t *)param_ptr;
    char buf[32];
    
    LCD_PRINT("[LCD] Updating display: mode=%s, status=%s\n", test_mode, status);
    
    // Uncomment after installing GLIB component
    /*
    GLIB_clear(&glibContext);
    GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNarrow6x8);
    
    // Line 1: Test Mode
    snprintf(buf, sizeof(buf), "Mode: %s", test_mode);
    draw_text(2, 5, buf);
    
    // Line 2: Status
    snprintf(buf, sizeof(buf), "Stat: %s", status);
    draw_text(2, 15, buf);
    
    // Horizontal separator
    GLIB_drawLineH(&glibContext, 0, 127, 25);
    
    // Line 3: TX Power
    snprintf(buf, sizeof(buf), "Pwr: %s", get_txpwr_string(param->txpwr));
    draw_text(2, 28, buf);
    
    // Line 4: Interval
    snprintf(buf, sizeof(buf), "Int: %s", get_interval_string(param->interval_idx));
    draw_text(2, 38, buf);
    
    // Line 5: Count (only for Sender mode)
    if (strcmp(test_mode, "Sender") == 0) {
        snprintf(buf, sizeof(buf), "Cnt: %s", get_count_string(param->count_idx));
        draw_text(2, 48, buf);
    }
    
    // Line 6: PHY Status
    draw_phy_status(2, 58, param);
    
    // Line 7: Channel Inhibit (if any)
    if (param->inhibit_ch37 || param->inhibit_ch38 || param->inhibit_ch39) {
        snprintf(buf, sizeof(buf), "CH-%s%s%s", 
                 param->inhibit_ch37 ? "37" : "",
                 param->inhibit_ch38 ? "38" : "",
                 param->inhibit_ch39 ? "39" : "");
        draw_text(2, 68, buf);
    }
    
    // Line 8: Optional flags
    uint8_t y = 78;
    if (param->non_ANONYMOUS) {
        draw_text(2, y, "NonAnon");
        y += 10;
    }
    if (param->ignore_rcv_resp) {
        draw_text(2, y, "IgnResp");
    }
    
    DMD_updateDisplay();
    */
    
    (void)buf;  // Suppress unused warning when GLIB is commented out
}

void lcd_ui_show_progress(uint32_t current, uint32_t total, int8_t rssi)
{
    if (!lcd_initialized) {
        return;
    }
    
    char buf[32];
    
    // Uncomment after installing GLIB component
    /*
    // Clear bottom area only (partial update for efficiency)
    GLIB_Rectangle_t rect = {0, 90, 128, 128};
    glibContext.foregroundColor = White;
    GLIB_drawRectFilled(&glibContext, &rect);
    glibContext.foregroundColor = Black;
    
    // Progress text
    snprintf(buf, sizeof(buf), "%lu / %lu", (unsigned long)current, (unsigned long)total);
    draw_text(2, 95, buf);
    
    // Progress bar
    if (total > 0) {
        // Bar outline
        GLIB_drawRect(&glibContext, 2, 105, 126, 112);
        
        // Bar fill
        uint8_t bar_width = (uint8_t)(((uint32_t)current * 120) / total);
        if (bar_width > 0) {
            GLIB_Rectangle_t bar = {4, 107, 4 + bar_width, 110};
            GLIB_drawRectFilled(&glibContext, &bar);
        }
    }
    
    // RSSI (for scanner mode)
    if (rssi != 0) {
        snprintf(buf, sizeof(buf), "RSSI: %d dBm", rssi);
        draw_text(2, 115, buf);
    }
    
    DMD_updateDisplay();
    */
    
    (void)buf;
    (void)current;
    (void)total;
    (void)rssi;
}

void lcd_ui_clear(void)
{
    if (!lcd_initialized) {
        return;
    }
    
    LCD_PRINT("[LCD] Clearing display\n");
    
    // Uncomment after installing GLIB component
    /*
    GLIB_clear(&glibContext);
    DMD_updateDisplay();
    */
}

bool lcd_ui_is_ready(void)
{
    return lcd_initialized;
}

void lcd_ui_show_error(const char *error_msg, int error_code)
{
    if (!lcd_initialized) {
        return;
    }
    
    LCD_PRINT("[LCD] Showing error: %s (0x%X)\n", error_msg, error_code);
    
    // Uncomment after installing GLIB component
    /*
    char buf[32];
    
    GLIB_clear(&glibContext);
    GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNarrow6x8);
    
    // Error header
    draw_text(35, 40, "ERROR");
    
    // Error message
    draw_text(10, 55, error_msg);
    
    // Error code
    snprintf(buf, sizeof(buf), "Code: 0x%X", error_code);
    draw_text(10, 70, buf);
    
    DMD_updateDisplay();
    */
    
    (void)error_msg;
    (void)error_code;
}

void lcd_ui_show_connection(bool connected)
{
    if (!lcd_initialized) {
        return;
    }
    
    // Uncomment after installing GLIB component
    /*
    // Draw connection indicator in top-right corner
    if (connected) {
        // Draw small filled circle
        GLIB_drawCircleFilled(&glibContext, 120, 5, 3);
    } else {
        // Clear the area
        GLIB_Rectangle_t rect = {115, 0, 128, 10};
        glibContext.foregroundColor = White;
        GLIB_drawRectFilled(&glibContext, &rect);
        glibContext.foregroundColor = Black;
    }
    
    DMD_updateDisplay();
    */
    
    (void)connected;
}
