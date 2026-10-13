/*
 * widgets.c — UI-Widgets: Speed, RPM-Bar, Gear, Temp, Fuel, Nav-Arrow, Warning
 *
 * Zeichnet in den Framebuffer ueber die Renderer-Paint-Engine.
 * Layout siehe docs/research/10-ui-ux-design.md und mockups/screens/.
 *
 * Widgets sind Stateless: Sie nehmen einen Wert und zeichnen ihn.
 * Animationen werden ueber die Renderer-Animation-Queue abgewickelt.
 */

#include "display.h"
#include "board_config.h"
#include "esp_log.h"
#include "string.h"
#include "stdio.h"

static const char *TAG = "widgets";

/* Forward declaration */
static uint16_t widgets_get_display_width(void);

/* ============================================================
 * 7-Segment Font (5x7 Pixel pro Ziffer)
 *
 * Jede Ziffer ist 5 Pixel breit, 7 Pixel hoch.
 * 1 = Pixel an, 0 = Pixel aus.
 * Reihenfolge: Segment a (top), b (top-right), c (bottom-right),
 * d (bottom), e (bottom-left), f (top-left), g (middle)
 * ============================================================ */

#define SEG_W  5
#define SEG_H  7
#define SEG_SPACING 1

/* 7-segment bit patterns for digits 0-9 (each row is a bitmask of 5 pixels) */
static const uint8_t seg_digits[10][SEG_H] = {
    /* 0 */ {0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F},
    /* 1 */ {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    /* 2 */ {0x1F, 0x01, 0x01, 0x1F, 0x10, 0x10, 0x1F},
    /* 3 */ {0x1F, 0x01, 0x01, 0x1F, 0x01, 0x01, 0x1F},
    /* 4 */ {0x11, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x01},
    /* 5 */ {0x1F, 0x10, 0x10, 0x1F, 0x01, 0x01, 0x1F},
    /* 6 */ {0x1F, 0x10, 0x10, 0x1F, 0x11, 0x11, 0x1F},
    /* 7 */ {0x1F, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
    /* 8 */ {0x1F, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x1F},
    /* 9 */ {0x1F, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x1F},
};

/* 5x7 monospace bitmap font for uppercase letters A-Z and a few symbols */
static const uint8_t font5x7[96][5] = {
    /* ASCII 32-47: space and symbols */
    {0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00}, /* " */
    {0x14,0x7F,0x14,0x7F,0x14}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* $ */
    {0x23,0x13,0x08,0x64,0x62}, /* % */
    {0x36,0x49,0x55,0x22,0x50}, /* & */
    {0x00,0x05,0x03,0x00,0x00}, /* ' */
    {0x00,0x1C,0x22,0x41,0x00}, /* ( */
    {0x00,0x41,0x22,0x1C,0x00}, /* ) */
    {0x08,0x2A,0x1C,0x2A,0x08}, /* * */
    {0x08,0x08,0x3E,0x08,0x08}, /* + */
    {0x00,0x50,0x30,0x00,0x00}, /* , */
    {0x08,0x08,0x08,0x08,0x08}, /* - */
    {0x00,0x60,0x60,0x00,0x00}, /* . */
    {0x20,0x10,0x08,0x04,0x02}, /* / */
    /* ASCII 48-57: digits 0-9 */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
    /* ASCII 58-63: symbols */
    {0x00,0x36,0x36,0x00,0x00}, /* : */
    {0x00,0x56,0x36,0x00,0x00}, /* ; */
    {0x00,0x08,0x14,0x22,0x41}, /* < */
    {0x14,0x14,0x14,0x14,0x14}, /* = */
    {0x41,0x22,0x14,0x08,0x00}, /* > */
    {0x02,0x01,0x51,0x09,0x06}, /* ? */
    /* ASCII 64-95: @, A-Z, [ ] ^ _ */
    {0x32,0x49,0x79,0x41,0x3E}, /* @ */
    {0x7E,0x11,0x11,0x11,0x7E}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x01,0x01}, /* F */
    {0x3E,0x41,0x41,0x51,0x32}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x04,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */
    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V */
    {0x7F,0x20,0x18,0x20,0x7F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x03,0x04,0x78,0x04,0x03}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
    {0x00,0x7F,0x41,0x41,0x00}, /* [ */
    {0x02,0x04,0x08,0x10,0x20}, /* \ */
    {0x00,0x41,0x41,0x7F,0x00}, /* ] */
    {0x04,0x02,0x01,0x02,0x04}, /* ^ */
    {0x40,0x40,0x40,0x40,0x40}, /* _ */
};

/* Draw a single character using the 5x7 bitmap font */
static void draw_char5x7(int x, int y, char ch, uint16_t color,
                         uint8_t scale)
{
    if (ch < 32 || ch > 95) ch = '?';
    const uint8_t *glyph = font5x7[(int)(ch - 32)];

    for (int col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; row++) {
            if (bits & (1 << row)) {
                if (scale == 1) {
                    renderer_draw_pixel(x + col, y + row, color);
                } else {
                    renderer_fill_rect(x + col * scale, y + row * scale,
                                       scale, scale, color);
                }
            }
        }
    }
}

/* Draw a text string using the 5x7 font */
static void draw_text5x7(int x, int y, const char *text, uint16_t color,
                         uint8_t scale)
{
    int cx = x;
    for (int i = 0; text[i]; i++) {
        draw_char5x7(cx, y, text[i], color, scale);
        cx += (5 + 1) * scale;  /* 5px glyph + 1px spacing */
    }
}

/* Draw a large 7-segment digit */
static void draw_seg_digit(int x, int y, uint8_t digit, uint16_t color,
                           uint8_t scale)
{
    if (digit > 9) digit = 9;
    const uint8_t *pattern = seg_digits[digit];

    for (int row = 0; row < SEG_H; row++) {
        uint8_t bits = pattern[row];
        for (int col = 0; col < SEG_W; col++) {
            if (bits & (1 << (SEG_W - 1 - col))) {
                renderer_fill_rect(x + col * scale, y + row * scale,
                                   scale, scale, color);
            }
        }
    }
}

/* Draw a multi-digit 7-segment number */
static void draw_seg_number(int x, int y, uint16_t value, uint8_t num_digits,
                            uint16_t color, uint8_t scale)
{
    char buf[8];
    int len = snprintf(buf, sizeof(buf), "%0*u", num_digits, value);
    int seg_total_w = (SEG_W + SEG_SPACING) * scale;
    int cx = x;

    for (int i = 0; i < len; i++) {
        uint8_t d = (uint8_t)(buf[i] - '0');
        if (d <= 9) {
            draw_seg_digit(cx, y, d, color, scale);
        }
        cx += seg_total_w;
    }
}

/* ============================================================
 * Widget: Speed (7-Segment)
 * ============================================================ */

void widgets_draw_speed(uint16_t speed_kmh, uint16_t x, uint16_t y,
                        uint8_t font_size)
{
    /* Background clear for the speed area */
    int digit_w = (SEG_W + SEG_SPACING) * font_size;
    int total_w = digit_w * 3;   /* up to 3 digits */
    int total_h = SEG_H * font_size;

    renderer_fill_rect(x, y, total_w, total_h, COLOR_BLACK);

    /* Draw the speed number */
    draw_seg_number(x, y, speed_kmh, 3, COLOR_FG_PRIMARY, font_size);

    /* Draw "km/h" label below */
    int label_y = y + total_h + 2;
    draw_text5x7(x, label_y, "km/h", COLOR_FG_SECONDARY, 1);

    renderer_invalidate(x, y, total_w, total_h + 10);
}

/* ============================================================
 * Widget: RPM Bar (horizontal, mit Shift-Light)
 * ============================================================ */

void widgets_draw_rpm_bar(uint16_t rpm, uint16_t max_rpm,
                          uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    /* Clear bar background */
    renderer_fill_rect(x, y, w, h, COLOR_BG_RAISED);

    /* Draw border */
    renderer_draw_line(x, y, x + w - 1, y, COLOR_FG_TERTIARY);
    renderer_draw_line(x, y + h - 1, x + w - 1, y + h - 1, COLOR_FG_TERTIARY);
    renderer_draw_line(x, y, x, y + h - 1, COLOR_FG_TERTIARY);
    renderer_draw_line(x + w - 1, y, x + w - 1, y + h - 1, COLOR_FG_TERTIARY);

    /* Calculate fill percentage */
    uint16_t pct;
    if (max_rpm > 0) {
        pct = (uint16_t)((uint32_t)rpm * 100 / max_rpm);
    } else {
        pct = 0;
    }
    if (pct > 100) pct = 100;

    /* Determine color based on RPM zone */
    uint16_t bar_color;
    if (pct < 40) {
        bar_color = COLOR_ACCENT_GREEN;   /* 0-40%: green */
    } else if (pct < 70) {
        bar_color = COLOR_ACCENT_YELLOW;  /* 40-70%: yellow */
    } else if (pct < 85) {
        bar_color = COLOR_ACCENT_ORANGE;  /* 70-85%: orange */
    } else {
        bar_color = COLOR_ACCENT_RED;     /* 85%+: red (shift light) */
    }

    /* Draw fill bar (inset by 1px for border) */
    uint16_t fill_w = (uint16_t)((uint32_t)(w - 2) * pct / 100);
    if (fill_w > 0) {
        renderer_fill_rect(x + 1, y + 1, fill_w, h - 2, bar_color);
    }

    /* Shift-light indicator: red dot in top-right when near redline */
    if (pct >= 85) {
        renderer_fill_circle(x + w - 6, y - 4, 3, COLOR_ACCENT_RED);
    }

    /* RPM digital readout */
    char rpm_text[12];
    snprintf(rpm_text, sizeof(rpm_text), "%u", rpm);
    draw_text5x7(x + w + 4, y + (h / 2) - 3, rpm_text, COLOR_FG_SECONDARY, 1);

    renderer_invalidate(x - 8, y - 8, w + 40, h + 16);
}

/* ============================================================
 * Widget: Gear Indicator
 * ============================================================ */

void widgets_draw_gear(uint8_t gear, uint16_t x, uint16_t y)
{
    /* Clear area */
    renderer_fill_rect(x, y, 24, 28, COLOR_BLACK);

    /* Draw border */
    renderer_draw_line(x, y, x + 23, y, COLOR_FG_TERTIARY);
    renderer_draw_line(x, y + 27, x + 23, y + 27, COLOR_FG_TERTIARY);
    renderer_draw_line(x, y, x, y + 27, COLOR_FG_TERTIARY);
    renderer_draw_line(x + 23, y, x + 23, y + 27, COLOR_FG_TERTIARY);

    /* Display gear: 0=N, 1-6 */
    char gear_str[2];
    if (gear == 0) {
        gear_str[0] = 'N';
    } else if (gear <= 6) {
        gear_str[0] = '0' + gear;
    } else {
        gear_str[0] = '?';
    }
    gear_str[1] = '\0';

    /* Center the character in the 24x28 box */
    int char_w = 5 * 3;  /* 5px font * scale 3 = 15px */
    int char_h = 7 * 3;  /* 7px font * scale 3 = 21px */
    int cx = x + (24 - char_w) / 2;
    int cy = y + (28 - char_h) / 2;

    uint16_t gear_color = (gear == 0) ? COLOR_ACCENT_AMBER : COLOR_FG_PRIMARY;
    draw_char5x7(cx, cy, gear_str[0], gear_color, 3);

    /* Label */
    draw_text5x7(x - 4, y + 30, "GEAR", COLOR_FG_TERTIARY, 1);

    renderer_invalidate(x - 4, y, 32, 40);
}

/* ============================================================
 * Widget: Temperature (with color coding)
 * ============================================================ */

void widgets_draw_temp(int8_t temp_c, uint16_t x, uint16_t y)
{
    /* Clear area */
    renderer_fill_rect(x, y, 50, 20, COLOR_BLACK);

    /* Color coding:
     *   < 40°C   -> green (normal)
     *   40-100°C -> yellow (caution)
     *   100-115°C -> orange (warning)
     *   > 115°C  -> red (critical)
     */
    uint16_t temp_color;
    if (temp_c < 40) {
        temp_color = COLOR_ACCENT_GREEN;
    } else if (temp_c < 100) {
        temp_color = COLOR_ACCENT_YELLOW;
    } else if (temp_c < 115) {
        temp_color = COLOR_ACCENT_ORANGE;
    } else {
        temp_color = COLOR_ACCENT_RED;
    }

    /* Draw temperature value */
    char temp_str[8];
    snprintf(temp_str, sizeof(temp_str), "%d", temp_c);
    draw_text5x7(x, y, temp_str, temp_color, 2);

    /* Draw degree symbol + C */
    int val_w = 0;
    int len = strlen(temp_str);
    val_w = len * (5 + 1) * 2;
    draw_text5x7(x + val_w, y, "C", temp_color, 2);

    /* Label */
    draw_text5x7(x, y + 16, "TEMP", COLOR_FG_TERTIARY, 1);

    renderer_invalidate(x, y, 60, 28);
}

/* ============================================================
 * Widget: Fuel Level
 * ============================================================ */

void widgets_draw_fuel(uint8_t liters, uint8_t max_liters,
                       uint16_t x, uint16_t y)
{
    /* Clear area */
    renderer_fill_rect(x, y, 40, 20, COLOR_BLACK);

    /* Draw fuel bar outline */
    renderer_draw_line(x, y, x + 39, y, COLOR_FG_TERTIARY);
    renderer_draw_line(x, y + 19, x + 39, y + 19, COLOR_FG_TERTIARY);
    renderer_draw_line(x, y, x, y + 19, COLOR_FG_TERTIARY);
    renderer_draw_line(x + 39, y, x + 39, y + 19, COLOR_FG_TERTIARY);

    /* Calculate fill */
    uint16_t pct = 0;
    if (max_liters > 0) {
        pct = (uint16_t)((uint32_t)liters * 100 / max_liters);
    }
    if (pct > 100) pct = 100;

    /* Color: green > 50%, yellow 25-50%, red < 25% */
    uint16_t fuel_color;
    if (pct > 50) {
        fuel_color = COLOR_ACCENT_GREEN;
    } else if (pct >= 25) {
        fuel_color = COLOR_ACCENT_YELLOW;
    } else {
        fuel_color = COLOR_ACCENT_RED;
    }

    /* Draw fill bar */
    uint16_t fill_h = (uint16_t)((uint32_t)18 * pct / 100);
    if (fill_h > 0) {
        renderer_fill_rect(x + 1, y + 19 - fill_h, 38, fill_h, fuel_color);
    }

    /* Draw fuel value text */
    char fuel_str[8];
    snprintf(fuel_str, sizeof(fuel_str), "%uL", liters);
    draw_text5x7(x + 4, y + 22, fuel_str, COLOR_FG_SECONDARY, 1);

    renderer_invalidate(x, y, 44, 32);
}

/* ============================================================
 * Widget: Navigation Arrow (for E-Paper)
 * ============================================================ */

void widgets_draw_nav_arrow(uint16_t heading, uint16_t next_turn,
                            uint16_t x, uint16_t y)
{
    /* Draw a directional arrow based on heading vs next_turn.
     * heading: current heading in degrees
     * next_turn: direction of next turn in degrees
     *
     * For simplicity: draw a large arrow pointing in the direction
     * of next_turn relative to heading.
     */
    int16_t rel = (int16_t)(next_turn - heading);
    /* Normalize to -180..180 */
    while (rel > 180)  rel -= 360;
    while (rel < -180) rel += 360;

    /* Clear area */
    renderer_fill_rect(x, y, 40, 40, COLOR_BLACK);

    uint16_t arrow_color = COLOR_ACCENT_BLUE;

    /* Determine arrow direction:
     *   |rel| < 45  -> straight up
     *   rel 45-135  -> right
     *   rel -45 to -135 -> left
     *   |rel| > 135 -> uturn (downward)
     */
    int abs_rel = (rel < 0) ? -rel : rel;

    if (abs_rel < 45) {
        /* Straight arrow (up) */
        /* Arrow shaft */
        renderer_fill_rect(x + 17, y + 10, 6, 25, arrow_color);
        /* Arrow head (triangle) */
        for (int i = 0; i < 10; i++) {
            renderer_fill_rect(x + 8 + i, y + 10 + i, 24 - 2 * i, 1,
                              arrow_color);
        }
    } else if (rel >= 45 && rel <= 135) {
        /* Right arrow */
        renderer_fill_rect(x + 10, y + 17, 25, 6, arrow_color);
        for (int i = 0; i < 10; i++) {
            renderer_fill_rect(x + 25 + i, y + 8 + i, 1, 24 - 2 * i,
                              arrow_color);
        }
    } else if (rel <= -45 && rel >= -135) {
        /* Left arrow */
        renderer_fill_rect(x + 5, y + 17, 25, 6, arrow_color);
        for (int i = 0; i < 10; i++) {
            renderer_fill_rect(x + 10 - i, y + 8 + i, 1, 24 - 2 * i,
                              arrow_color);
        }
    } else {
        /* U-turn arrow (downward) */
        renderer_fill_rect(x + 17, y + 5, 6, 25, arrow_color);
        for (int i = 0; i < 10; i++) {
            renderer_fill_rect(x + 8 + i, y + 25 - i, 24 - 2 * i, 1,
                              arrow_color);
        }
    }

    /* Draw heading text below */
    char dir_str[8];
    snprintf(dir_str, sizeof(dir_str), "%u", next_turn);
    draw_text5x7(x + 8, y + 42, dir_str, COLOR_FG_PRIMARY, 1);

    renderer_invalidate(x, y, 48, 52);
}

/* ============================================================
 * Widget: Warning Overlay
 * ============================================================ */

void widgets_draw_warning(warning_type_t type, const char *msg)
{
    if (type == WARNING_NONE) return;

    /* Warning overlay at top of screen */
    uint16_t y = 0;
    uint16_t w = widgets_get_display_width();
    renderer_fill_rect(0, y, w, 20, COLOR_ACCENT_RED);

    /* Warning icon (exclamation mark in circle) */
    renderer_draw_circle(10, 10, 8, COLOR_WHITE);
    renderer_fill_rect(9, 5, 2, 6, COLOR_ACCENT_RED);
    renderer_fill_rect(9, 12, 2, 2, COLOR_ACCENT_RED);

    /* Warning message */
    const char *default_msg = "WARNUNG";
    const char *text = msg ? msg : default_msg;
    draw_text5x7(24, 6, text, COLOR_WHITE, 2);

    renderer_invalidate(0, y, w, 20);
}

/* Helper to get the active display width */
static uint16_t widgets_get_display_width(void)
{
    /* This function should return the active display width.
     * We use a simple heuristic: if tft_get_framebuffer() returns non-NULL,
     * we're in TFT mode; otherwise E-Paper.
     * In practice, the renderer sets the active display type.
     */
    if (tft_get_framebuffer()) {
        return TFT_WIDTH;
    }
    return EPD_WIDTH;
}

/* ============================================================
 * Widget: Altitude
 * ============================================================ */

void widgets_draw_altitude(float altitude_m, uint16_t x, uint16_t y)
{
    /* Clear area */
    renderer_fill_rect(x, y, 60, 20, COLOR_BLACK);

    /* Draw altitude value */
    char alt_str[12];
    int alt_int = (int)altitude_m;
    snprintf(alt_str, sizeof(alt_str), "%dm", alt_int);
    draw_text5x7(x, y, alt_str, COLOR_ACCENT_CYAN, 2);

    /* Label */
    draw_text5x7(x, y + 16, "ALT", COLOR_FG_TERTIARY, 1);

    renderer_invalidate(x, y, 60, 24);
}
