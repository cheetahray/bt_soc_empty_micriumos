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
#include "sl_board_control.h"  // For sl_board_enable_display()

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
static GLIB_Context_t glibContext;
static bool lcd_initialized = false;

/* Selection cursor state */
static uint8_t current_selection = 0;  // Current selected item (0-based)
static uint8_t max_selection_items = 9;  // Total number of selectable items
static test_param_t *cached_param = NULL;  // Cache for redraw (non-const for editing)
static uint8_t scroll_offset = 0;  // Menu scroll offset (items scrolled up)

/* Sub-menu state */
typedef enum {
    LCD_MODE_MAIN_MENU,     // Main configuration menu
    LCD_MODE_SUB_MENU       // Sub-menu for specific item
} lcd_menu_mode_t;

static lcd_menu_mode_t menu_mode = LCD_MODE_MAIN_MENU;
static uint8_t sub_selection = 0;       // Current selection in sub-menu
static uint8_t max_sub_items = 0;       // Number of items in current sub-menu
static uint8_t sub_scroll_offset = 0;   // Sub-menu scroll offset

/* Item names for display */
static const char* item_names[] = {
    "TxPwr",      // 0
    "Intv",       // 1
    "Count",      // 2
    "PHY",        // 3
    "Channel",    // 4
    "NonAnon",    // 5
    "IgnResp",    // 6
    "StartTask",  // 7
    "StopTask"    // 8
};

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
    GLIB_drawString(&glibContext, text, strlen(text), x, y, false);
    (void)x;
    (void)y;
    (void)text;
}

/**
 * @brief Draw selection triangle indicator
 * 
 * @param x X coordinate (left edge)
 * @param y Y coordinate (vertical center)
 */
static void draw_selection_triangle(uint8_t x, uint8_t y)
{
    // Uncomment after installing GLIB component
    
    // Draw a filled right-pointing triangle (▶)
    // Triangle: 3 pixels wide, 5 pixels tall
    GLIB_drawLine(&glibContext, x, y-2, x, y+2);       // Left edge
    GLIB_drawLine(&glibContext, x+1, y-1, x+1, y+1);   // Middle
    GLIB_drawPixel(&glibContext, x+2, y);              // Tip
    
    (void)x;
    (void)y;
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
 * @brief TX power mapping: index to dBm value
 * 
 * Maps sub-menu index to actual dBm value for round_test_parm.txpwr
 */
static const int8_t txpwr_values[] = {
    -40,  // 0
    -20,  // 1
    -16,  // 2
    -12,  // 3
    -8,   // 4
    -4,   // 5
    0,    // 6
    2,    // 7
    4,    // 8
    6,    // 9
    8,    // 10
    10    // 11
};

/**
 * @brief Get TX power string from index
 * 
 * Actual available power levels depend on hardware capabilities.
 * EFR32MG27 typically supports -20 to +10 dBm range.
 */
static const char* get_txpwr_string_by_idx(uint8_t idx)
{
    // Common TX power levels for EFR32MG27
    static const char* power_levels[] = {
        "-40dBm",   // 0
        "-20dBm",   // 1
        "-16dBm",   // 2
        "-12dBm",   // 3
        "-8dBm",    // 4
        "-4dBm",    // 5
        "0dBm",     // 6
        "+2dBm",    // 7
        "+4dBm",    // 8
        "+6dBm",    // 9
        "+8dBm",    // 10
        "+10dBm"    // 11
    };
    
    if (idx < sizeof(power_levels) / sizeof(power_levels[0])) {
        return power_levels[idx];
    }
    return "?dBm";
}

/**
 * @brief Get number of available TX power options
 */
static uint8_t get_txpwr_count(void)
{
    return 12;  // -40, -20, -16, -12, -8, -4, 0, +2, +4, +6, +8, +10
}

/**
 * @brief Get interval string from index
 * 
 * Based on value_interval array from losstst_svc.c
 * Shows min-max ranges in milliseconds
 */
static const char* get_interval_string_by_idx(uint8_t idx)
{
    // 11 advertising interval ranges (matches value_interval array)
    static const char* intervals[] = {
        "30-60ms",      // 0: TGAP(adv_fast_interval1)
        "60-120ms",     // 1
        "90-180ms",     // 2: TGAP(adv_fast_interval1_coded)
        "100-150ms",    // 3: TGAP(adv_fast_interval2)
        "200-300ms",    // 4
        "300-450ms",    // 5: TGAP(adv_fast_interval2_coded)
        "500-650ms",    // 6
        "750-950ms",    // 7
        "1000-1200ms", // 8: TGAP(adv_slow_interval)
        "2000-2400ms", // 9
        "3000ms+"       // 10: TGAP(adv_slow_interval_coded)
    };
    
    if (idx < sizeof(intervals) / sizeof(intervals[0])) {
        return intervals[idx];
    }
    return "?ms";
}

/**
 * @brief Get number of available interval options
 */
static uint8_t get_interval_count(void)
{
    return 11;  // 0-10
}

/**
 * @brief Get count string from index
 * 
 * Based on enum_total_num array from losstst_svc.c
 */
static const char* get_count_string_by_idx(uint8_t idx)
{
    // 7 packet count options (matches enum_total_num array)
    static const char* counts[] = {
        "500",      // 0
        "1000",     // 1
        "2000",     // 2
        "5000",     // 3
        "10000",    // 4
        "20000",    // 5
        "50000"     // 6
    };
    
    if (idx < sizeof(counts) / sizeof(counts[0])) {
        return counts[idx];
    }
    return "?";
}

/**
 * @brief Get number of available count options
 */
static uint8_t get_count_count(void)
{
    return 7;  // 0-6
}

/**
 * @brief Convert TX power value to human-readable string
 * 
 * @param txpwr Actual TX power in dBm from test_param_t
 * @return String representation (e.g., "+10dBm")
 * 
 * @note This function receives actual dBm values, not indices
 */
static const char* get_txpwr_string(int8_t txpwr)
{
    // Format actual dBm value as string
    static char buf[8];
    if (txpwr > 0) {
        snprintf(buf, sizeof(buf), "+%ddBm", txpwr);
    } else {
        snprintf(buf, sizeof(buf), "%ddBm", txpwr);
    }
    return buf;
}

/**
 * @brief Convert interval index to millisecond string
 * 
 * @param idx Advertising interval index from test_param_t
 * @return String representation (e.g., "100-150ms")
 */
static const char* get_interval_string(uint8_t idx)
{
    // Use the same array as get_interval_string_by_idx
    return get_interval_string_by_idx(idx);
}

/**
 * @brief Convert count index to packet count string
 * 
 * @param idx Count index from test_param_t
 * @return String representation (e.g., "1000")
 */
static const char* get_count_string(uint8_t idx)
{
    // Use the same array as get_count_string_by_idx
    return get_count_string_by_idx(idx);
}

/* ==================== Public Functions ==================== */

int lcd_ui_init(void)
{
    LCD_PRINT("[LCD] Initializing display...\n");
    
    /* Enable display power on BRD4002A */
    sl_status_t status = sl_board_enable_display();
    if (status != SL_STATUS_OK) {
        LCD_PRINT("[LCD] sl_board_enable_display() failed: 0x%lX\n", (unsigned long)status);
        return -3;
    }
    
    /* Initialize DMD (Display Module Driver) */
    // Uncomment after installing dmd_memlcd component
    
    status = DMD_init(0);
    if (status != DMD_OK) {
        LCD_PRINT("[LCD] DMD_init() failed: 0x%X\n", (unsigned int)status);
        return -1;
    }
    
    
    /* Initialize GLIB (Graphics Library) context */
    // Uncomment after installing glib component
    
    status = GLIB_contextInit(&glibContext);
    if (status != GLIB_OK) {
        LCD_PRINT("[LCD] GLIB_contextInit() failed: 0x%X\n", (unsigned int)status);
        return -2;
    }
    
    
    /* Configure display appearance */
    // Uncomment after installing glib component
    
    glibContext.backgroundColor = White;
    glibContext.foregroundColor = Black;
    GLIB_clear(&glibContext);
    DMD_updateDisplay();
    
    
    lcd_initialized = true;
    LCD_PRINT("[LCD] Display initialized successfully\n");
    return 0;
}

void lcd_ui_show_startup(void *param_ptr)
{
    if (!lcd_initialized) {
        LCD_PRINT("[LCD] Not initialized, skipping startup screen\n");
        return;
    }
    
    test_param_t *param = (test_param_t *)param_ptr;
    cached_param = param;  // Cache for redraw and Button 1 editing
    
    LCD_PRINT("[LCD] Showing startup screen with config (sel=%d)\n", current_selection);
    
    // Uncomment after installing GLIB component
    
    char buf[32];
    
    GLIB_clear(&glibContext);
    GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNarrow6x8);
    
    // Title
    draw_text(10, 2, "BLE Loss Test");
    draw_text(10, 12, "Default Config:");
    
    // Horizontal separator
    GLIB_drawLineH(&glibContext, 0, 127, 22);
    
    if (param != NULL) {
        uint8_t text_x = 10;  // Text starts at x=10 (leave space for triangle)
        
        // Calculate scroll: show items [scroll_offset .. scroll_offset+8]
        // Maximum 9 visible items at a time
        const uint8_t MAX_VISIBLE_ITEMS = 9;
        const uint8_t BASE_Y = 25;  // Starting Y position for first visible item
        const uint8_t LINE_HEIGHT = 10;  // Spacing between items
        
        // Draw only visible items
        for (uint8_t i = 0; i < max_selection_items; i++) {
            // Check if item is in visible range
            if (i < scroll_offset || i >= scroll_offset + MAX_VISIBLE_ITEMS) {
                continue;  // Skip items outside visible area
            }
            
            // Calculate Y position relative to visible area
            uint8_t visible_index = i - scroll_offset;
            uint8_t y = BASE_Y + (visible_index * LINE_HEIGHT);
            uint8_t triangle_y = y + 2;
            
            // Draw selection triangle if this is the current item
            if (current_selection == i) {
                draw_selection_triangle(2, triangle_y);
            }
            
            // Draw menu item based on index
            switch (i) {
                case 0: // TX Power
                    snprintf(buf, sizeof(buf), "TxPwr:%s", get_txpwr_string(param->txpwr));
                    draw_text(text_x, y, buf);
                    break;
                    
                case 1: // Interval
                    snprintf(buf, sizeof(buf), "Intv:%s", get_interval_string(param->interval_idx));
                    draw_text(text_x, y, buf);
                    break;
                    
                case 2: // Count
                    snprintf(buf, sizeof(buf), "Count:%s", get_count_string(param->count_idx));
                    draw_text(text_x, y, buf);
                    break;
                    
                case 3: // PHY Selection
                    snprintf(buf, sizeof(buf), "PHY:%s%s%s%s",
                             param->phy_2m ? "2M " : "",
                             param->phy_1m ? "1M " : "",
                             param->phy_s8 ? "S8 " : "",
                             param->phy_ble4 ? "BLE4" : "");
                    draw_text(text_x, y, buf);
                    break;
                    
                case 4: // Channel Status
                    if (param->inhibit_ch37 || param->inhibit_ch38 || param->inhibit_ch39) {
                        snprintf(buf, sizeof(buf), "CH:%s%s%s",
                                 param->inhibit_ch37 ? "X37 " : "O37 ",
                                 param->inhibit_ch38 ? "X38 " : "O38 ",
                                 param->inhibit_ch39 ? "X39" : "O39");
                    } else {
                        snprintf(buf, sizeof(buf), "CH:All Enabled");
                    }
                    draw_text(text_x, y, buf);
                    break;
                    
                case 5: // NonAnonymous
                    snprintf(buf, sizeof(buf), "NonAnon:%s", param->non_ANONYMOUS ? "YES" : "NO");
                    draw_text(text_x, y, buf);
                    break;
                    
                case 6: // Ignore Response
                    snprintf(buf, sizeof(buf), "IgnResp:%s", param->ignore_rcv_resp ? "YES" : "NO");
                    draw_text(text_x, y, buf);
                    break;
                    
                case 7: // Start Task
                    draw_text(text_x, y, "StartTask:Select");
                    break;
                    
                case 8: // Stop Task
                    draw_text(text_x, y, "StopTask:Stop All");
                    break;
            }
        }
        
        // Bottom status
        draw_text(2, 115, "BTN0:Next");
    } else {
        // No config provided - basic startup
        draw_text(10, 40, "Initializing...");
    }
    
    DMD_updateDisplay();
    
    
    (void)param_ptr;  // Suppress warning when GLIB is commented out
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
    
    
    (void)buf;  // Suppress unused warning when GLIB is commented out
}

void lcd_ui_show_progress(uint32_t current, uint32_t total, int8_t rssi)
{
    if (!lcd_initialized) {
        return;
    }
    
    char buf[32];
    
    // Uncomment after installing GLIB component
    
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
        GLIB_Rectangle_t outline = {2, 105, 126, 112};
        GLIB_drawRect(&glibContext, &outline);
        
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
    
    GLIB_clear(&glibContext);
    DMD_updateDisplay();

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

    
    (void)error_msg;
    (void)error_code;
}

void lcd_ui_show_connection(bool connected)
{
    if (!lcd_initialized) {
        return;
    }
    
    // Uncomment after installing GLIB component
    
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
    
    
    (void)connected;
}

/* ==================== Selection Control Implementation ==================== */

/**
 * @brief Draw sub-menu screen
 */
static void draw_sub_menu(void)
{
    // Uncomment after installing GLIB component
    
    char buf[32];
    
    GLIB_clear(&glibContext);
    GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNarrow6x8);
    
    // Title: show which item we're editing
    snprintf(buf, sizeof(buf), "Edit: %s", item_names[current_selection]);
    draw_text(2, 2, buf);
    
    // Horizontal separator
    GLIB_drawLineH(&glibContext, 0, 127, 12);
    
    const uint8_t BASE_Y = 18;
    const uint8_t LINE_HEIGHT = 10;
    const uint8_t MAX_VISIBLE_ITEMS = 10;  // Max items visible in sub-menu
    uint8_t text_x = 10;
    
    switch (current_selection) {
        case 0: // TxPwr
        {
            uint8_t pwr_count = get_txpwr_count();
            max_sub_items = pwr_count + 1;  // All power options + Back
            
            // Draw visible items with scroll support
            for (uint8_t i = 0; i < max_sub_items; i++) {
                // Check if item is in visible range
                if (i < sub_scroll_offset || i >= sub_scroll_offset + MAX_VISIBLE_ITEMS) {
                    continue;
                }
                
                uint8_t visible_index = i - sub_scroll_offset;
                uint8_t y = BASE_Y + (visible_index * LINE_HEIGHT);
                
                if (sub_selection == i) draw_selection_triangle(2, y+2);
                
                if (i < pwr_count) {
                    draw_text(text_x, y, get_txpwr_string_by_idx(i));
                } else {
                    draw_text(text_x, y, "< Back");
                }
            }
            break;
        }
            
        case 1: // Interval
        {
            uint8_t int_count = get_interval_count();
            max_sub_items = int_count + 1;  // 11 intervals + Back
            
            // Draw visible items with scroll support
            for (uint8_t i = 0; i < max_sub_items; i++) {
                // Check if item is in visible range
                if (i < sub_scroll_offset || i >= sub_scroll_offset + MAX_VISIBLE_ITEMS) {
                    continue;
                }
                
                uint8_t visible_index = i - sub_scroll_offset;
                uint8_t y = BASE_Y + (visible_index * LINE_HEIGHT);
                
                if (sub_selection == i) draw_selection_triangle(2, y+2);
                
                if (i < int_count) {
                    draw_text(text_x, y, get_interval_string_by_idx(i));
                } else {
                    draw_text(text_x, y, "< Back");
                }
            }
            break;
        }
            
        case 2: // Count
        {
            uint8_t cnt_count = get_count_count();
            max_sub_items = cnt_count + 1;  // 7 counts + Back
            
            // Draw visible items with scroll support (though 8 items fit without scroll)
            for (uint8_t i = 0; i < max_sub_items; i++) {
                if (i < sub_scroll_offset || i >= sub_scroll_offset + MAX_VISIBLE_ITEMS) {
                    continue;
                }
                
                uint8_t visible_index = i - sub_scroll_offset;
                uint8_t y = BASE_Y + (visible_index * LINE_HEIGHT);
                
                if (sub_selection == i) draw_selection_triangle(2, y+2);
                
                if (i < cnt_count) {
                    draw_text(text_x, y, get_count_string_by_idx(i));
                } else {
                    draw_text(text_x, y, "< Back");
                }
            }
            break;
        }
            
        case 3: // PHY
        {
            max_sub_items = 5;  // 4 options + Back
            const char* phy_items[] = {
                cached_param->phy_2m ? "[X] 2M PHY" : "[ ] 2M PHY",
                cached_param->phy_1m ? "[X] 1M PHY" : "[ ] 1M PHY",
                cached_param->phy_s8 ? "[X] S8 PHY" : "[ ] S8 PHY",
                cached_param->phy_ble4 ? "[X] BLE4" : "[ ] BLE4",
                "< Back"
            };
            
            for (uint8_t i = 0; i < max_sub_items; i++) {
                if (i < sub_scroll_offset || i >= sub_scroll_offset + MAX_VISIBLE_ITEMS) {
                    continue;
                }
                
                uint8_t visible_index = i - sub_scroll_offset;
                uint8_t y = BASE_Y + (visible_index * LINE_HEIGHT);
                
                if (sub_selection == i) draw_selection_triangle(2, y+2);
                draw_text(text_x, y, phy_items[i]);
            }
            break;
        }
            
        case 4: // Channel
        {
            max_sub_items = 4;  // 3 channels + Back
            const char* ch_items[] = {
                cached_param->inhibit_ch37 ? "[X] Ch37 OFF" : "[ ] Ch37 ON",
                cached_param->inhibit_ch38 ? "[X] Ch38 OFF" : "[ ] Ch38 ON",
                cached_param->inhibit_ch39 ? "[X] Ch39 OFF" : "[ ] Ch39 ON",
                "< Back"
            };
            
            for (uint8_t i = 0; i < max_sub_items; i++) {
                if (i < sub_scroll_offset || i >= sub_scroll_offset + MAX_VISIBLE_ITEMS) {
                    continue;
                }
                
                uint8_t visible_index = i - sub_scroll_offset;
                uint8_t y = BASE_Y + (visible_index * LINE_HEIGHT);
                
                if (sub_selection == i) draw_selection_triangle(2, y+2);
                draw_text(text_x, y, ch_items[i]);
            }
            break;
        }
            
        case 5: // NonAnonymous
        {
            max_sub_items = 3;  // ON, OFF, Back
            const char* items[] = {"ON", "OFF", "< Back"};
            
            for (uint8_t i = 0; i < max_sub_items; i++) {
                if (i < sub_scroll_offset || i >= sub_scroll_offset + MAX_VISIBLE_ITEMS) {
                    continue;
                }
                
                uint8_t visible_index = i - sub_scroll_offset;
                uint8_t y = BASE_Y + (visible_index * LINE_HEIGHT);
                
                if (sub_selection == i) draw_selection_triangle(2, y+2);
                draw_text(text_x, y, items[i]);
            }
            break;
        }
            
        case 6: // IgnoreResponse
        {
            max_sub_items = 3;  // ON, OFF, Back
            const char* items[] = {"ON", "OFF", "< Back"};
            
            for (uint8_t i = 0; i < max_sub_items; i++) {
                if (i < sub_scroll_offset || i >= sub_scroll_offset + MAX_VISIBLE_ITEMS) {
                    continue;
                }
                
                uint8_t visible_index = i - sub_scroll_offset;
                uint8_t y = BASE_Y + (visible_index * LINE_HEIGHT);
                
                if (sub_selection == i) draw_selection_triangle(2, y+2);
                draw_text(text_x, y, items[i]);
            }
            break;
        }
            
        case 7: // Start Task
        {
            max_sub_items = 5;  // 4 tasks + Back
            const char* items[] = {"Sender", "Scanner", "Numcast", "Envmon", "< Back"};
            
            for (uint8_t i = 0; i < max_sub_items; i++) {
                if (i < sub_scroll_offset || i >= sub_scroll_offset + MAX_VISIBLE_ITEMS) {
                    continue;
                }
                
                uint8_t visible_index = i - sub_scroll_offset;
                uint8_t y = BASE_Y + (visible_index * LINE_HEIGHT);
                
                if (sub_selection == i) draw_selection_triangle(2, y+2);
                draw_text(text_x, y, items[i]);
            }
            break;
        }
    }
    
    // Bottom hint
    draw_text(2, 115, "BTN0:Next BTN1:Sel");
    
    DMD_updateDisplay();
    
}

void lcd_ui_next_selection(void)
{
    if (!lcd_initialized) {
        return;
    }
    
    if (menu_mode == LCD_MODE_MAIN_MENU) {
        // Move to next item in main menu
        current_selection++;
        
        // Wrap around to first item
        if (current_selection >= max_selection_items) {
            current_selection = 0;
            scroll_offset = 0;  // Reset scroll when wrapping to first item
        } else {
            // Update scroll offset: keep triangle visible in comfortable zone
            // Start scrolling when selection reaches item 8 (9th item, index 8)
            // This keeps the triangle from getting too close to the bottom
            const uint8_t SCROLL_THRESHOLD = 8;  // Start scrolling at 9th item
            if (current_selection >= SCROLL_THRESHOLD) {
                // Scroll up: shift menu so selected item appears at position 8
                scroll_offset = current_selection - SCROLL_THRESHOLD;
            } else {
                // No scroll needed for items 0-7
                scroll_offset = 0;
            }
        }
        
        LCD_PRINT("[LCD] Main menu selection: %d, scroll_offset: %d\n", current_selection, scroll_offset);
        
        // Redraw main menu
        if (cached_param != NULL) {
            lcd_ui_show_startup(cached_param);
        }
    } else {
        // Move to next item in sub-menu
        sub_selection++;
        
        // Wrap around
        if (sub_selection >= max_sub_items) {
            sub_selection = 0;
            sub_scroll_offset = 0;  // Reset scroll when wrapping to first item
        } else {
            // Update scroll offset for sub-menu: keep selected item visible
            // Start scrolling when selection reaches item 9 (10th item, index 9)
            const uint8_t SUB_SCROLL_THRESHOLD = 9;
            if (sub_selection >= SUB_SCROLL_THRESHOLD) {
                sub_scroll_offset = sub_selection - SUB_SCROLL_THRESHOLD;
            } else {
                sub_scroll_offset = 0;
            }
        }
        
        LCD_PRINT("[LCD] Sub-menu selection: %d/%d, sub_scroll: %d\n", sub_selection, max_sub_items-1, sub_scroll_offset);
        
        // Redraw sub-menu
        draw_sub_menu();
    }
}

void lcd_ui_expand_selection(void)
{
    if (!lcd_initialized) {
        return;
    }
    
    if (menu_mode == LCD_MODE_MAIN_MENU) {
        // Special handling: Stop Task (no sub-menu)
        if (current_selection == 8) {
            // Stop all tasks directly
            LCD_PRINT("[LCD] Stopping all tasks\n");
            sender_task_tgr(-1);
            scanner_task_tgr(-1);
            numcst_task_tgr(-1);
            envmon_task_tgr(-1);
            LCD_PRINT("[LCD] All tasks stopped\n");
            
            // Stay in main menu, just redraw
            if (cached_param != NULL) {
                lcd_ui_show_startup(cached_param);
            }
            return;
        }
        
        // Enter sub-menu for current item
        LCD_PRINT("[LCD] Expanding item %d: %s\n", current_selection, item_names[current_selection]);
        
        menu_mode = LCD_MODE_SUB_MENU;
        sub_selection = 0;
        sub_scroll_offset = 0;  // Reset sub-menu scroll
        
        // Draw sub-menu
        draw_sub_menu();
    } else {
        // In sub-menu: check if "Back" is selected
        bool is_back = (sub_selection == (max_sub_items - 1));
        
        if (is_back) {
            // Return to main menu
            LCD_PRINT("[LCD] Back to main menu\n");
            menu_mode = LCD_MODE_MAIN_MENU;
            sub_selection = 0;
            sub_scroll_offset = 0;  // Reset sub-menu scroll
            
            // Redraw main menu
            if (cached_param != NULL) {
                lcd_ui_show_startup(cached_param);
            }
        } else {
            // Apply the selected sub-option
            LCD_PRINT("[LCD] Selected sub-option %d for item %s\n", 
                     sub_selection, item_names[current_selection]);
            
            if (cached_param != NULL) {
                // Update configuration based on current menu item
                switch (current_selection) {
                    case 0: // TxPwr
                        if (sub_selection < sizeof(txpwr_values) / sizeof(txpwr_values[0])) {
                            cached_param->txpwr = txpwr_values[sub_selection];
                            LCD_PRINT("[LCD] TxPwr set to %d dBm\n", cached_param->txpwr);
                        }
                        break;
                        
                    case 1: // Interval
                        if (sub_selection < get_interval_count()) {
                            cached_param->interval_idx = sub_selection;
                            LCD_PRINT("[LCD] Interval set to index %d\n", cached_param->interval_idx);
                        }
                        break;
                        
                    case 2: // Count
                        if (sub_selection < get_count_count()) {
                            cached_param->count_idx = sub_selection;
                            LCD_PRINT("[LCD] Count set to index %d\n", cached_param->count_idx);
                        }
                        break;
                        
                    case 3: // PHY - toggle the selected PHY
                        switch (sub_selection) {
                            case 0: 
                                cached_param->phy_2m = !cached_param->phy_2m;
                                LCD_PRINT("[LCD] 2M PHY: %s\n", cached_param->phy_2m ? "ON" : "OFF");
                                break;
                            case 1:
                                cached_param->phy_1m = !cached_param->phy_1m;
                                LCD_PRINT("[LCD] 1M PHY: %s\n", cached_param->phy_1m ? "ON" : "OFF");
                                break;
                            case 2:
                                cached_param->phy_s8 = !cached_param->phy_s8;
                                LCD_PRINT("[LCD] S8 PHY: %s\n", cached_param->phy_s8 ? "ON" : "OFF");
                                break;
                            case 3:
                                cached_param->phy_ble4 = !cached_param->phy_ble4;
                                LCD_PRINT("[LCD] BLE4: %s\n", cached_param->phy_ble4 ? "ON" : "OFF");
                                break;
                        }
                        break;
                        
                    case 4: // Channel - toggle the selected channel inhibit
                        switch (sub_selection) {
                            case 0:
                                cached_param->inhibit_ch37 = !cached_param->inhibit_ch37;
                                LCD_PRINT("[LCD] Ch37: %s\n", cached_param->inhibit_ch37 ? "OFF" : "ON");
                                break;
                            case 1:
                                cached_param->inhibit_ch38 = !cached_param->inhibit_ch38;
                                LCD_PRINT("[LCD] Ch38: %s\n", cached_param->inhibit_ch38 ? "OFF" : "ON");
                                break;
                            case 2:
                                cached_param->inhibit_ch39 = !cached_param->inhibit_ch39;
                                LCD_PRINT("[LCD] Ch39: %s\n", cached_param->inhibit_ch39 ? "OFF" : "ON");
                                break;
                        }
                        break;
                        
                    case 5: // NonAnonymous - set value
                        if (sub_selection == 0) {
                            cached_param->non_ANONYMOUS = true;
                            LCD_PRINT("[LCD] NonAnonymous: ON\n");
                        } else if (sub_selection == 1) {
                            cached_param->non_ANONYMOUS = false;
                            LCD_PRINT("[LCD] NonAnonymous: OFF\n");
                        }
                        break;
                        
                    case 6: // IgnoreResponse - set value
                        if (sub_selection == 0) {
                            cached_param->ignore_rcv_resp = true;
                            LCD_PRINT("[LCD] IgnoreResponse: ON\n");
                        } else if (sub_selection == 1) {
                            cached_param->ignore_rcv_resp = false;
                            LCD_PRINT("[LCD] IgnoreResponse: OFF\n");
                        }
                        break;
                        
                    case 7: // Start Task - trigger task
                        switch (sub_selection) {
                            case 0: // Sender
                                sender_task_tgr(1);
                                LCD_PRINT("[LCD] Sender task started\n");
                                break;
                            case 1: // Scanner
                                scanner_task_tgr(1);
                                LCD_PRINT("[LCD] Scanner task started\n");
                                break;
                            case 2: // Numcast
                                numcst_task_tgr(1);
                                LCD_PRINT("[LCD] Numcast task started\n");
                                break;
                            case 3: // Envmon
                                envmon_task_tgr(1);
                                LCD_PRINT("[LCD] Envmon task started\n");
                                break;
                        }
                        break;
                }
            }
            
            // Return to main menu and redraw with updated values
            menu_mode = LCD_MODE_MAIN_MENU;
            sub_scroll_offset = 0;  // Reset sub-menu scroll
            
            if (cached_param != NULL) {
                lcd_ui_show_startup(cached_param);
            }
        }
    }
}

uint8_t lcd_ui_get_selection(void)
{
    return current_selection;
}

void lcd_ui_reset_selection(void)
{
    current_selection = 0;
    scroll_offset = 0;  // Reset scroll offset too
    menu_mode = LCD_MODE_MAIN_MENU;
    sub_selection = 0;
    sub_scroll_offset = 0;  // Reset sub-menu scroll too
    LCD_PRINT("[LCD] Selection reset to first item\n");
}
