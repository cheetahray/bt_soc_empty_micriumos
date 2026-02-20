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

/* Selection cursor state */
static uint8_t current_selection = 0;  // Current selected item (0-based)
static uint8_t max_selection_items = 7;  // Total number of selectable items
static const test_param_t *cached_param = NULL;  // Cache for redraw

/* Sub-menu state */
typedef enum {
    LCD_MODE_MAIN_MENU,     // Main configuration menu
    LCD_MODE_SUB_MENU       // Sub-menu for specific item
} lcd_menu_mode_t;

static lcd_menu_mode_t menu_mode = LCD_MODE_MAIN_MENU;
static uint8_t sub_selection = 0;       // Current selection in sub-menu
static uint8_t max_sub_items = 0;       // Number of items in current sub-menu

/* Item names for display */
static const char* item_names[] = {
    "TxPwr",      // 0
    "Intv",       // 1
    "Count",      // 2
    "PHY",        // 3
    "Channel",    // 4
    "NonAnon",    // 5
    "IgnResp"     // 6
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
    // GLIB_drawString(&glibContext, text, strlen(text), x, y, false);
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
    /*
    // Draw a filled right-pointing triangle (▶)
    // Triangle: 3 pixels wide, 5 pixels tall
    GLIB_drawLine(&glibContext, x, y-2, x, y+2);       // Left edge
    GLIB_drawLine(&glibContext, x+1, y-1, x+1, y+1);   // Middle
    GLIB_drawPixel(&glibContext, x+2, y);              // Tip
    */
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
 * @brief Get TX power string from index
 */
static const char* get_txpwr_string_by_idx(uint8_t idx)
{
    switch(idx) {
        case 0: return "-20dBm";
        case 1: return "-10dBm";
        case 2: return "-5dBm";
        case 3: return "0dBm";
        case 4: return "+5dBm";
        case 5: return "+10dBm";
        default: return "?dBm";
    }
}

/**
 * @brief Get interval string from index
 */
static const char* get_interval_string_by_idx(uint8_t idx)
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
 * @brief Get count string from index
 */
static const char* get_count_string_by_idx(uint8_t idx)
{
    switch(idx) {
        case 0: return "100";
        case 1: return "1000";
        case 2: return "10000";
        default: return "?";
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

void lcd_ui_show_startup(const void *param_ptr)
{
    if (!lcd_initialized) {
        LCD_PRINT("[LCD] Not initialized, skipping startup screen\n");
        return;
    }
    
    const test_param_t *param = (const test_param_t *)param_ptr;
    cached_param = param;  // Cache for redraw on selection change
    
    LCD_PRINT("[LCD] Showing startup screen with config (sel=%d)\n", current_selection);
    
    // Uncomment after installing GLIB component
    /*
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
        
        // Line 0: TX Power
        if (current_selection == 0) draw_selection_triangle(2, 27);
        snprintf(buf, sizeof(buf), "TxPwr:%s", get_txpwr_string(param->txpwr));
        draw_text(text_x, 25, buf);
        
        // Line 1: Interval
        if (current_selection == 1) draw_selection_triangle(2, 37);
        snprintf(buf, sizeof(buf), "Intv:%s", get_interval_string(param->interval_idx));
        draw_text(text_x, 35, buf);
        
        // Line 2: Count
        if (current_selection == 2) draw_selection_triangle(2, 47);
        snprintf(buf, sizeof(buf), "Count:%s", get_count_string(param->count_idx));
        draw_text(text_x, 45, buf);
        
        // Line 3: PHY Selection
        if (current_selection == 3) draw_selection_triangle(2, 57);
        snprintf(buf, sizeof(buf), "PHY:%s%s%s%s",
                 param->phy_2m ? "2M " : "",
                 param->phy_1m ? "1M " : "",
                 param->phy_s8 ? "S8 " : "",
                 param->phy_ble4 ? "BLE4" : "");
        draw_text(text_x, 55, buf);
        
        // Line 4: Channel Status
        if (current_selection == 4) draw_selection_triangle(2, 67);
        if (param->inhibit_ch37 || param->inhibit_ch38 || param->inhibit_ch39) {
            snprintf(buf, sizeof(buf), "CH:%s%s%s",
                     param->inhibit_ch37 ? "X37 " : "O37 ",
                     param->inhibit_ch38 ? "X38 " : "O38 ",
                     param->inhibit_ch39 ? "X39" : "O39");
        } else {
            snprintf(buf, sizeof(buf), "CH:All Enabled");
        }
        draw_text(text_x, 65, buf);
        
        // Line 5: NonAnonymous
        if (current_selection == 5) draw_selection_triangle(2, 77);
        snprintf(buf, sizeof(buf), "NonAnon:%s", param->non_ANONYMOUS ? "YES" : "NO");
        draw_text(text_x, 75, buf);
        
        // Line 6: Ignore Response
        if (current_selection == 6) draw_selection_triangle(2, 87);
        snprintf(buf, sizeof(buf), "IgnResp:%s", param->ignore_rcv_resp ? "YES" : "NO");
        draw_text(text_x, 85, buf);
        
        // Bottom status
        draw_text(2, 115, "BTN0:Next");
    } else {
        // No config provided - basic startup
        draw_text(10, 40, "Initializing...");
    }
    
    DMD_updateDisplay();
    */
    
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

/* ==================== Selection Control Implementation ==================== */

/**
 * @brief Draw sub-menu screen
 */
static void draw_sub_menu(void)
{
    // Uncomment after installing GLIB component
    /*
    char buf[32];
    
    GLIB_clear(&glibContext);
    GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNarrow6x8);
    
    // Title: show which item we're editing
    snprintf(buf, sizeof(buf), "Edit: %s", item_names[current_selection]);
    draw_text(2, 2, buf);
    
    // Horizontal separator
    GLIB_drawLineH(&glibContext, 0, 127, 12);
    
    uint8_t y = 18;
    uint8_t text_x = 10;
    
    switch (current_selection) {
        case 0: // TxPwr
            max_sub_items = 7;  // 6 options + Back
            for (uint8_t i = 0; i < 6; i++) {
                if (sub_selection == i) draw_selection_triangle(2, y+2);
                draw_text(text_x, y, get_txpwr_string_by_idx(i));
                y += 10;
            }
            if (sub_selection == 6) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, "< Back");
            break;
            
        case 1: // Interval
            max_sub_items = 6;  // 5 options + Back
            for (uint8_t i = 0; i < 5; i++) {
                if (sub_selection == i) draw_selection_triangle(2, y+2);
                draw_text(text_x, y, get_interval_string_by_idx(i));
                y += 10;
            }
            if (sub_selection == 5) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, "< Back");
            break;
            
        case 2: // Count
            max_sub_items = 4;  // 3 options + Back
            for (uint8_t i = 0; i < 3; i++) {
                if (sub_selection == i) draw_selection_triangle(2, y+2);
                draw_text(text_x, y, get_count_string_by_idx(i));
                y += 10;
            }
            if (sub_selection == 3) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, "< Back");
            break;
            
        case 3: // PHY
            max_sub_items = 5;  // 4 options + Back
            if (sub_selection == 0) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, cached_param->phy_2m ? "[X] 2M PHY" : "[ ] 2M PHY");
            y += 10;
            if (sub_selection == 1) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, cached_param->phy_1m ? "[X] 1M PHY" : "[ ] 1M PHY");
            y += 10;
            if (sub_selection == 2) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, cached_param->phy_s8 ? "[X] S8 PHY" : "[ ] S8 PHY");
            y += 10;
            if (sub_selection == 3) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, cached_param->phy_ble4 ? "[X] BLE4" : "[ ] BLE4");
            y += 10;
            if (sub_selection == 4) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, "< Back");
            break;
            
        case 4: // Channel
            max_sub_items = 4;  // 3 channels + Back
            if (sub_selection == 0) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, cached_param->inhibit_ch37 ? "[X] Ch37 OFF" : "[ ] Ch37 ON");
            y += 10;
            if (sub_selection == 1) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, cached_param->inhibit_ch38 ? "[X] Ch38 OFF" : "[ ] Ch38 ON");
            y += 10;
            if (sub_selection == 2) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, cached_param->inhibit_ch39 ? "[X] Ch39 OFF" : "[ ] Ch39 ON");
            y += 10;
            if (sub_selection == 3) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, "< Back");
            break;
            
        case 5: // NonAnonymous
            max_sub_items = 3;  // ON, OFF, Back
            if (sub_selection == 0) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, "ON");
            y += 10;
            if (sub_selection == 1) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, "OFF");
            y += 10;
            if (sub_selection == 2) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, "< Back");
            break;
            
        case 6: // IgnoreResponse
            max_sub_items = 3;  // ON, OFF, Back
            if (sub_selection == 0) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, "ON");
            y += 10;
            if (sub_selection == 1) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, "OFF");
            y += 10;
            if (sub_selection == 2) draw_selection_triangle(2, y+2);
            draw_text(text_x, y, "< Back");
            break;
    }
    
    // Bottom hint
    draw_text(2, 115, "BTN0:Next BTN1:Sel");
    
    DMD_updateDisplay();
    */
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
        }
        
        LCD_PRINT("[LCD] Main menu selection: %d\n", current_selection);
        
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
        }
        
        LCD_PRINT("[LCD] Sub-menu selection: %d/%d\n", sub_selection, max_sub_items-1);
        
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
        // Enter sub-menu for current item
        LCD_PRINT("[LCD] Expanding item %d: %s\n", current_selection, item_names[current_selection]);
        
        menu_mode = LCD_MODE_SUB_MENU;
        sub_selection = 0;
        
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
            
            // Redraw main menu
            if (cached_param != NULL) {
                lcd_ui_show_startup(cached_param);
            }
        } else {
            // TODO: Apply the selected sub-option
            LCD_PRINT("[LCD] Selected sub-option %d for item %s\n", 
                     sub_selection, item_names[current_selection]);
            
            // For now, just go back to main menu after selection
            // In future, this should update the configuration
            menu_mode = LCD_MODE_MAIN_MENU;
            
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
    menu_mode = LCD_MODE_MAIN_MENU;
    sub_selection = 0;
    LCD_PRINT("[LCD] Selection reset to first item\n");
}
