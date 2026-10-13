#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

/*
 * display.h — Display-Subsystem: TFT + E-Paper Treiber, Renderer, Widgets
 *
 * Zwei Displays:
 *   TFT     2.4" IPS 240x320 RGB565 (ILI9341/ST7789)  — 30 FPS, DMA
 *   E-Paper 2.9" b/w 296x128 (SSD1680/UC8151D)        — 1 Hz, partial refresh
 *
 * SPI shared bus: MOSI=11, SCLK=12, MISO=13
 *   TFT:     CS=10, DC=14, RST=21
 *   E-Paper: CS=10, DC=14, RST=21, BUSY=47
 *
 * SPI-Bus wird ueber Mutex gesichert (shared mit SD-Karte).
 */

/* ============================================================
 * Types & Enums
 * ============================================================ */

typedef enum {
    DISPLAY_TYPE_NONE = 0,
    DISPLAY_TYPE_EPAPER,     /* 2.9" b/w (296x128) */
    DISPLAY_TYPE_TFT,        /* 2.4" IPS (240x320) */
} display_type_t;

typedef enum {
    DISPLAY_MODE_TOUR = 0,   /* Standard: Speed, RPM, Hoehe, Temp */
    DISPLAY_MODE_TRACK,      /* GPS-Fokus: Position, HDOP, Distanz */
    DISPLAY_MODE_NAV,        /* Navigations-Modus */
    DISPLAY_MODE_CONFIG,     /* Einstellungen: BLE, Kalibrierung */
} display_mode_t;

typedef enum {
    WARNING_NONE = 0,
    WARNING_OVERTEMP,
    WARNING_LOW_VOLTAGE,
    WARNING_LOW_FUEL,
    WARNING_NO_GPS,
    WARNING_BLE_LOST,
    WARNING_GENERIC,
} warning_type_t;

/* RGB565 color helper */
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/* Color table (RGB565) */
#define COLOR_BLACK          0x0000
#define COLOR_WHITE          0xFFFF
#define COLOR_BG_VOID        0x0000
#define COLOR_BG_PANEL       0x1084
#define COLOR_BG_RAISED      0x2104
#define COLOR_BG_OVERLAY     0x3186
#define COLOR_FG_PRIMARY      0xFFFF
#define COLOR_FG_SECONDARY   0x8C71
#define COLOR_FG_TERTIARY    0x4208
#define COLOR_ACCENT_AMBER   0xFC80
#define COLOR_ACCENT_ORANGE  0xFB20
#define COLOR_ACCENT_RED     0xF800
#define COLOR_ACCENT_BLUE    0x041F
#define COLOR_ACCENT_GREEN   0x3660
#define COLOR_ACCENT_CYAN    0x4CDA
#define COLOR_ACCENT_YELLOW  0xFFE0

/* Display geometry (fallback if board_config.h not included) */
#ifndef TFT_WIDTH
#define TFT_WIDTH            240
#endif
#ifndef TFT_HEIGHT
#define TFT_HEIGHT           320
#endif
#ifndef EPD_WIDTH
#define EPD_WIDTH            296
#endif
#ifndef EPD_HEIGHT
#define EPD_HEIGHT           128
#endif

/* Dirty-rect system */
#define MAX_DIRTY_RECTS     16

typedef struct {
    int16_t x, y, w, h;
} rect_t;

/* ============================================================
 * Display-Task API
 * ============================================================ */

void display_init(void);
void display_task(void *arg);

/* ============================================================
 * TFT Driver API
 * ============================================================ */

void     tft_init(void);
void     tft_set_backlight(uint8_t percent);
uint16_t tft_get_backlight(void);
uint8_t *tft_get_framebuffer(void);
void     tft_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void     tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                       uint16_t color);
void     tft_draw_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                         const uint8_t *data);
void     tft_push_buffer(void);
void     tft_push_region(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void     tft_swap_buffers(void);

/* ============================================================
 * E-Paper Driver API
 * ============================================================ */

void     epaper_init(void);
void     epaper_clear(void);
void     epaper_display_frame(const uint8_t *buffer);
void     epaper_display_partial(const uint8_t *buffer, uint16_t x,
                                uint16_t y, uint16_t w, uint16_t h);
uint8_t *epaper_get_framebuffer(void);
bool     epaper_is_busy(void);

/* ============================================================
 * Renderer API
 * ============================================================ */

void renderer_init(display_type_t type);
void renderer_clear(void);
void renderer_flush(bool partial);
void renderer_set_clip(rect_t clip);
void renderer_reset_clip(void);
void renderer_invalidate(int x, int y, int w, int h);
void renderer_draw_pixel(int x, int y, uint16_t color);
void renderer_fill_rect(int x, int y, int w, int h, uint16_t color);
void renderer_draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void renderer_draw_circle(int cx, int cy, int r, uint16_t color);
void renderer_fill_circle(int cx, int cy, int r, uint16_t color);

/* Animation-Queue */
typedef void (*anim_callback_t)(void *ctx);

typedef struct {
    bool           active;
    anim_callback_t cb;
    void          *ctx;
    uint32_t       duration_ms;
    uint32_t       elapsed_ms;
    uint32_t       start_tick;
} animation_t;

#define MAX_ANIMATIONS 8

void    renderer_anim_register(animation_t *anim);
void    renderer_anim_unregister(animation_t *anim);
void    renderer_anim_tick(uint32_t now_ms);
uint8_t renderer_anim_count(void);

/* ============================================================
 * Widget API
 * ============================================================ */

void widgets_draw_speed(uint16_t speed_kmh, uint16_t x, uint16_t y,
                        uint8_t font_size);
void widgets_draw_rpm_bar(uint16_t rpm, uint16_t max_rpm,
                          uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void widgets_draw_gear(uint8_t gear, uint16_t x, uint16_t y);
void widgets_draw_temp(int8_t temp_c, uint16_t x, uint16_t y);
void widgets_draw_fuel(uint8_t liters, uint8_t max_liters,
                       uint16_t x, uint16_t y);
void widgets_draw_nav_arrow(uint16_t heading, uint16_t next_turn,
                            uint16_t x, uint16_t y);
void widgets_draw_warning(warning_type_t type, const char *msg);
void widgets_draw_altitude(float altitude_m, uint16_t x, uint16_t y);

/* ============================================================
 * SPI Bus Mutex (shared between TFT, E-Paper, SD)
 * ============================================================ */

bool spi_bus_lock(TickType_t ticks);
void spi_bus_unlock(void);

#endif /* _DISPLAY_H_ */
