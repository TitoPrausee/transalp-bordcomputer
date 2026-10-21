/*
 * renderer.c — Rendering-Engine: Framebuffer, Dirty-Rect, Animation-Queue
 *
 * Verwaltet einen virtuellen Framebuffer, in den Widgets zeichnen.
 * Der Renderer uebertraegt den Framebuffer an das aktive Display
 * (TFT oder E-Paper) mittels Partial- oder Full-Refresh.
 *
 * Pipeline:
 *   Widget -> Paint-Engine (draw prims) -> Back-Buffer (PSRAM)
 *          -> Dirty-Rect Tracker -> Compositor (merge rects)
 *          -> Blit (SPI/DMA zum Display)
 *
 * 30 FPS Target fuer TFT, 1 Hz Refresh fuer E-Paper.
 */

#include "display.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "string.h"

static const char *TAG = "renderer";

/* ============================================================
 * Constants
 * ============================================================ */

#define TFT_FB_SIZE  ((size_t)(TFT_WIDTH * TFT_HEIGHT) * 2)
#define EPD_FB_SIZE  ((EPD_WIDTH * EPD_HEIGHT) / 8)

/* ============================================================
 * State
 * ============================================================ */

static display_type_t s_active_display = DISPLAY_TYPE_NONE;
static bool s_initialized = false;

/* Dirty-rect system */
typedef struct {
    rect_t rects[MAX_DIRTY_RECTS];
    uint8_t count;
} dirty_list_t;

static dirty_list_t s_dirty;
static rect_t s_clip;
static bool s_has_clip = false;

/* Animation queue */
static animation_t *s_anims[MAX_ANIMATIONS];
static uint8_t s_anim_count = 0;

/* SPI bus mutex (shared between TFT, E-Paper, SD) */
static SemaphoreHandle_t s_spi_mutex = NULL;

/* ============================================================
 * SPI Bus Mutex
 * ============================================================ */

bool spi_bus_lock(TickType_t ticks)
{
    if (!s_spi_mutex) return true;  /* no mutex = no contention */
    return xSemaphoreTake(s_spi_mutex, ticks) == pdTRUE;
}

void spi_bus_unlock(void)
{
    if (s_spi_mutex) xSemaphoreGive(s_spi_mutex);
}

/* ============================================================
 * Dirty-Rect System
 * ============================================================ */

static void dirty_reset(void)
{
    s_dirty.count = 0;
}

static void dirty_add(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }

    int max_w, max_h;
    if (s_active_display == DISPLAY_TYPE_TFT) {
        max_w = TFT_WIDTH;
        max_h = TFT_HEIGHT;
    } else {
        max_w = EPD_WIDTH;
        max_h = EPD_HEIGHT;
    }
    if (x >= max_w || y >= max_h) return;
    if (x + w > max_w) w = max_w - x;
    if (y + h > max_h) h = max_h - y;
    if (w <= 0 || h <= 0) return;

    /* If we still have room, add it */
    if (s_dirty.count < MAX_DIRTY_RECTS) {
        s_dirty.rects[s_dirty.count].x = x;
        s_dirty.rects[s_dirty.count].y = y;
        s_dirty.rects[s_dirty.count].w = w;
        s_dirty.rects[s_dirty.count].h = h;
        s_dirty.count++;
        return;
    }

    /* List full: merge everything into one full-screen rect */
    s_dirty.rects[0].x = 0;
    s_dirty.rects[0].y = 0;
    s_dirty.rects[0].w = max_w;
    s_dirty.rects[0].h = max_h;
    s_dirty.count = 1;
}

/* Check if two rects overlap */
static bool rects_overlap(const rect_t *a, const rect_t *b)
{
    return !(a->x + a->w < b->x || b->x + b->w < a->x ||
             a->y + a->h < b->y || b->y + b->h < a->y);
}

/* Merge two rects into their bounding box */
static rect_t rect_merge(const rect_t *a, const rect_t *b)
{
    rect_t r;
    int x0 = (a->x < b->x) ? a->x : b->x;
    int y0 = (a->y < b->y) ? a->y : b->y;
    int x1a = a->x + a->w;
    int x1b = b->x + b->w;
    int y1a = a->y + a->h;
    int y1b = b->y + b->h;
    int x1 = (x1a > x1b) ? x1a : x1b;
    int y1 = (y1a > y1b) ? y1a : y1b;
    r.x = x0;
    r.y = y0;
    r.w = x1 - x0;
    r.h = y1 - y0;
    return r;
}

/* Compositor: merge overlapping rects (O(n^2), n <= 16) */
static void dirty_merge(void)
{
    bool merged = true;
    while (merged && s_dirty.count > 1) {
        merged = false;
        for (int i = 0; i < s_dirty.count - 1; i++) {
            for (int j = i + 1; j < s_dirty.count; j++) {
                if (rects_overlap(&s_dirty.rects[i], &s_dirty.rects[j])) {
                    s_dirty.rects[i] =
                        rect_merge(&s_dirty.rects[i], &s_dirty.rects[j]);
                    /* Replace j with last element */
                    s_dirty.rects[j] = s_dirty.rects[s_dirty.count - 1];
                    s_dirty.count--;
                    j--;
                    merged = true;
                }
            }
        }
    }
}

/* ============================================================
 * Clip-Region Support
 * ============================================================ */

static bool clip_rect(int *x, int *y, int *w, int *h)
{
    if (!s_has_clip) return true;
    if (*x < s_clip.x) {
        *w -= (s_clip.x - *x);
        *x = s_clip.x;
    }
    if (*y < s_clip.y) {
        *h -= (s_clip.y - *y);
        *y = s_clip.y;
    }
    if (*x + *w > s_clip.x + s_clip.w) *w = s_clip.x + s_clip.w - *x;
    if (*y + *h > s_clip.y + s_clip.h) *h = s_clip.y + s_clip.h - *y;
    return (*w > 0 && *h > 0);
}

/* ============================================================
 * Paint-Engine: Grafik-Primitive
 * ============================================================ */

void renderer_draw_pixel(int x, int y, uint16_t color)
{
    int max_w, max_h;
    if (s_active_display == DISPLAY_TYPE_TFT) {
        max_w = TFT_WIDTH;
        max_h = TFT_HEIGHT;
    } else {
        max_w = EPD_WIDTH;
        max_h = EPD_HEIGHT;
    }
    if (x < 0 || x >= max_w || y < 0 || y >= max_h) return;

    /* Clip check */
    if (s_has_clip &&
        (x < s_clip.x || x >= s_clip.x + s_clip.w ||
         y < s_clip.y || y >= s_clip.y + s_clip.h))
        return;

    if (s_active_display == DISPLAY_TYPE_TFT) {
        tft_draw_pixel((uint16_t)x, (uint16_t)y, color);
    }
    /* E-Paper: set/clear bit in 1-bit buffer */
    if (s_active_display == DISPLAY_TYPE_EPAPER) {
        uint8_t *fb = epaper_get_framebuffer();
        if (fb) {
            uint16_t byte_idx = y * (EPD_WIDTH / 8) + x / 8;
            uint8_t bit = 0x80 >> (x % 8);
            if (color == COLOR_WHITE) {
                fb[byte_idx] |= bit;   /* 1 = white */
            } else {
                fb[byte_idx] &= ~bit;  /* 0 = black */
            }
        }
    }
    dirty_add(x, y, 1, 1);
}

void renderer_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (!clip_rect(&x, &y, &w, &h)) return;

    if (s_active_display == DISPLAY_TYPE_TFT) {
        tft_fill_rect((uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h,
                      color);
    } else if (s_active_display == DISPLAY_TYPE_EPAPER) {
        uint8_t *fb = epaper_get_framebuffer();
        if (fb) {
            for (int dy = 0; dy < h; dy++) {
                for (int dx = 0; dx < w; dx++) {
                    int px = x + dx;
                    int py = y + dy;
                    uint16_t idx = py * (EPD_WIDTH / 8) + px / 8;
                    uint8_t bit = 0x80 >> (px % 8);
                    if (color == COLOR_WHITE) {
                        fb[idx] |= bit;
                    } else {
                        fb[idx] &= ~bit;
                    }
                }
            }
        }
    }
    dirty_add(x, y, w, h);
}

void renderer_draw_line(int x0, int y0, int x1, int y1, uint16_t color)
{
    /* Bresenham's line algorithm */
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    for (;;) {
        renderer_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void renderer_draw_circle(int cx, int cy, int r, uint16_t color)
{
    if (r <= 0) return;
    int x = r, y = 0, err = 0;

    while (x >= y) {
        renderer_draw_pixel(cx + x, cy + y, color);
        renderer_draw_pixel(cx + y, cy + x, color);
        renderer_draw_pixel(cx - y, cy + x, color);
        renderer_draw_pixel(cx - x, cy + y, color);
        renderer_draw_pixel(cx - x, cy - y, color);
        renderer_draw_pixel(cx - y, cy - x, color);
        renderer_draw_pixel(cx + y, cy - x, color);
        renderer_draw_pixel(cx + x, cy - y, color);

        if (err <= 0) { y += 1; err += 2 * y; }
        if (err > 0)  { x -= 1; err -= 2 * x; }
    }
}

void renderer_fill_circle(int cx, int cy, int r, uint16_t color)
{
    if (r <= 0) return;
    for (int y = -r; y <= r; y++) {
        int dx = r;
        /* Simple sqrt-free approach: scan horizontal lines */
        int chord = r * r - y * y;
        if (chord < 0) continue;
        /* sqrt(chord) approximated via loop */
        while (dx * dx > chord && dx > 0) dx--;
        renderer_fill_rect(cx - dx, cy + y, 2 * dx + 1, 1, color);
    }
}

/* ============================================================
 * Clip API
 * ============================================================ */

void renderer_set_clip(rect_t clip)
{
    s_clip = clip;
    s_has_clip = true;
}

void renderer_reset_clip(void)
{
    s_has_clip = false;
}

void renderer_invalidate(int x, int y, int w, int h)
{
    dirty_add(x, y, w, h);
}

/* ============================================================
 * Animation-Queue
 * ============================================================ */

void renderer_anim_register(animation_t *anim)
{
    if (!anim || !anim->cb) return;
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (s_anims[i] == anim) return;  /* already registered */
    }
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (s_anims[i] == NULL) {
            s_anims[i] = anim;
            anim->active = true;
            anim->start_tick = xTaskGetTickCount() * portTICK_PERIOD_MS;
            anim->elapsed_ms = 0;
            s_anim_count++;
            return;
        }
    }
    ESP_LOGW(TAG, "Animation queue full (%d)", MAX_ANIMATIONS);
}

void renderer_anim_unregister(animation_t *anim)
{
    if (!anim) return;
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (s_anims[i] == anim) {
            s_anims[i] = NULL;
            anim->active = false;
            if (s_anim_count > 0) s_anim_count--;
            return;
        }
    }
}

void renderer_anim_tick(uint32_t now_ms)
{
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        animation_t *a = s_anims[i];
        if (!a || !a->active) continue;

        a->elapsed_ms = now_ms - a->start_tick;
        if (a->duration_ms > 0 && a->elapsed_ms >= a->duration_ms) {
            a->elapsed_ms = a->duration_ms;  /* clamp */
            a->cb(a->ctx);                    /* final callback */
            a->active = false;
            s_anims[i] = NULL;
            if (s_anim_count > 0) s_anim_count--;
        } else {
            a->cb(a->ctx);  /* intermediate callback */
        }
    }
}

uint8_t renderer_anim_count(void)
{
    return s_anim_count;
}

/* ============================================================
 * Init / Clear / Flush
 * ============================================================ */

void renderer_init(display_type_t type)
{
    s_active_display = type;

    /* Create SPI bus mutex if not yet created */
    if (!s_spi_mutex) {
        s_spi_mutex = xSemaphoreCreateMutex();
    }

    if (type == DISPLAY_TYPE_TFT) {
        tft_init();
    } else if (type == DISPLAY_TYPE_EPAPER) {
        epaper_init();
    } else {
        ESP_LOGW(TAG, "Renderer init with DISPLAY_TYPE_NONE");
        return;
    }

    dirty_reset();
    s_has_clip = false;
    s_anim_count = 0;
    memset((void *)s_anims, 0, sizeof(s_anims));
    s_initialized = true;

    ESP_LOGI(TAG, "Renderer init done, display=%s",
             (type == DISPLAY_TYPE_TFT) ? "TFT" : "E-Paper");
}

void renderer_clear(void)
{
    if (!s_initialized) return;

    if (s_active_display == DISPLAY_TYPE_TFT) {
        uint8_t *fb = tft_get_framebuffer();
        if (fb) memset(fb, 0x00, TFT_FB_SIZE);  /* black */
        dirty_add(0, 0, TFT_WIDTH, TFT_HEIGHT);
    } else if (s_active_display == DISPLAY_TYPE_EPAPER) {
        uint8_t *fb = epaper_get_framebuffer();
        if (fb) memset(fb, 0xFF, EPD_FB_SIZE);  /* white */
        dirty_add(0, 0, EPD_WIDTH, EPD_HEIGHT);
    }
}

void renderer_flush(bool partial)
{
    if (!s_initialized) return;
    if (s_dirty.count == 0) return;

    /* Merge overlapping dirty rects */
    dirty_merge();

    if (s_active_display == DISPLAY_TYPE_TFT) {
        /* TFT: swap back->front, then push each dirty region via DMA */
        tft_swap_buffers();
        for (int i = 0; i < s_dirty.count; i++) {
            tft_push_region((uint16_t)s_dirty.rects[i].x,
                            (uint16_t)s_dirty.rects[i].y,
                            (uint16_t)s_dirty.rects[i].w,
                            (uint16_t)s_dirty.rects[i].h);
        }
    } else if (s_active_display == DISPLAY_TYPE_EPAPER) {
        if (partial && s_dirty.count <= 4) {
            /* Partial refresh for small changes */
            for (int i = 0; i < s_dirty.count; i++) {
                epaper_display_partial(
                    epaper_get_framebuffer(),
                    (uint16_t)s_dirty.rects[i].x,
                    (uint16_t)s_dirty.rects[i].y,
                    (uint16_t)s_dirty.rects[i].w,
                    (uint16_t)s_dirty.rects[i].h);
            }
        } else {
            /* Full refresh */
            epaper_display_frame(epaper_get_framebuffer());
        }
    }

    dirty_reset();
}
