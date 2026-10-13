# Firmware-Architektur: Rendering-Engine, Animations-Framework, BLE-Stack, Diagnose, OTA, Konfiguration

> Stand: Oktober 2026
> Plattform: ESP32-S3-WROOM-1, 16MB Flash, 8MB PSRAM (Octal), Dual-Core 240MHz
> Framework: ESP-IDF v5.4 mit FreeRTOS SMP
> Fahrzeug: Honda XL600V Transalp (PD06), rein mechanisch, keine ECU
> Vorgaenger-Dokumente: 01-sensoren-fahrzeugdaten, 02-hardware-deep-dive, 03-software-stack, 07-display-navigation-smartphone

---

## 0. Architektur-Ueberblick

Die Firmware ist in sieben Subsysteme gegliedert, die ueber FreeRTOS-Primitives
(Queues, EventGroups, Mutexe, Task-Notifications) gekoppelt sind. Jedes Subsystem
laeuft in einem eigenen Task mit definiertem Core-Pin und Prioritaet. Die
Kernphilosophie: strikte Trennung von Datenakquisition (Core 1, echtzeitkritisch)
und Praesentation/Kommunikation (Core 0, best-effort).

```
                    +--------------------------------------------------+
                    |              FREERTOS KERNEL (SMP)               |
                    |                                                  |
     Core 1 (RT)    |  Core 0 (UI/Comms)                               |
     +-----------+  |  +-------------+    +-----------+               |
     | sensor    |  |  | display     |    | ble       |               |
     | task      |--+->| render      |    | nimble    |               |
     | 50Hz      |  |  | pipeline    |    | GATT      |               |
     +-----------+  |  +-------------+    +-----------+               |
     | gps       |  |  | logger      |    | config    |               |
     | task      |--+->| SD batch    |    | NVS       |               |
     | 10Hz      |  |  +-------------+    +-----------+               |
     +-----------+  |  | ota         |    | diag      |               |
     | can/rpm   |  |  | esp_ota     |    | DTC        |               |
     | task      |--+->| + verify    |    | self-test  |               |
     +-----------+  |  +-------------+    +-----------+               |
                    +--------------------------------------------------+
                              |                    |
                    +---------v----------+ +-------v---------+
                    | Queues / EventGrp  | | NVS Flash       |
                    | Mutex (SPI shared) | | /logs/dtc.log   |
                    +--------------------+ +-----------------+
```

Die sieben Subsysteme:

1. **Rendering-Engine** -- Framebuffer, Double-Buffering, Dirty-Rect, Paint/Blit
2. **Animations-Framework** -- Tween-Engine, Spring-Physics, Interruptible Timeline
3. **Sensor-Daten-Pipeline** -- Raw -> Filtered -> Displayed fuer jeden Sensor
4. **BLE-Stack** -- NimBLE GATT-Server, Services, Notifications, Robustheit
5. **Diagnose-System** -- DTC-Log, Self-Test, Diagnose-Mode
6. **OTA-Update Pipeline** -- BLE-Transfer, Flash, Verify, Rollback
7. **Konfigurations-System** -- NVS-basiert, Versioning, Live-Update, JSON-Export

---

## 1. Rendering-Engine fuer Display

### 1.1 Architektur-Ueberblick

Die Rendering-Engine ist eine vierschichtige Pipeline: Layout-Engine, Paint-Engine,
Compositor und Blit-Stage. Sie operiert auf einem PSRAM-residenten Framebuffer
mit Dirty-Rect-Tracking fuer partielle Updates.

```
  Widget Tree        Layout Engine       Paint Engine
  +----------+       +-------------+     +----------------+
  | Screen   |  -->  | Position    | --> | Draw Prims     |
  |  + RPM   |       | Size        |     | Rect/Circle/   |
  |  + Speed |       | Bounds      |     | Text/Line/Arc  |
  |  + Gear  |       +-------------+     +----------------+
  +----------+                                    |
                                                   v
  +----------+       +-------------+     +----------------+
  | SPI/DMA  | <--  | Compositor  | <-- | Dirty-Rect     |
  | Blit     |       | Merge Rects |     | Tracker        |
  +----------+       +-------------+     +----------------+
```

### 1.2 Framebuffer im PSRAM: Double-Buffering

Der ESP32-S3-WROOM-1 mit N8R8-Konfiguration hat 8MB octal PSRAM. Bei einem
3.5" TFT mit 480x320 Pixeln und RGB565 (2 Bytes/Pixel) benoetigt ein
Framebuffer 307.200 Bytes (~300KB). Das passt nicht in den internen SRAM
(512KB, davon ~200-250KB fuer Anwendungen nach RTOS/WiFi/BT-Stack).

Loesung: PSRAM-Framebuffer mit internem SRAM-Bounce-Buffer fuer DMA-Transfers.
SPI-DMA kann nicht direkt von PSRAM lesen -- der esp_lcd-Treiber kopiert
Daten blockweise in einen SRAM-Bounce-Buffer und transferiert von dort.

```c
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#define DISPLAY_H_RES   480
#define DISPLAY_V_RES   320
#define DISPLAY_BPP      2    // RGB565 = 2 Bytes/Pixel
#define FB_SIZE          (DISPLAY_H_RES * DISPLAY_V_RES * DISPLAY_BPP)

// Double-Buffer: Front = was auf dem Display steht, Back = wird gemalt
typedef struct {
    uint16_t *front;  // PSRAM, aktuell auf dem Display
    uint16_t *back;   // PSRAM, wird von Paint-Engine beschrieben
    bool      swapped;
} framebuffer_t;

static framebuffer_t fb;

esp_err_t framebuffer_init(void) {
    fb.front = heap_caps_malloc(FB_SIZE, MALLOC_CAP_SPIRAM);
    fb.back  = heap_caps_malloc(FB_SIZE, MALLOC_CAP_SPIRAM);
    if (!fb.front || !fb.back) return ESP_ERR_NO_MEM;
    memset(fb.front, 0xFF, FB_SIZE);  // weiss
    memset(fb.back,  0xFF, FB_SIZE);
    fb.swapped = false;
    return ESP_OK;
}

// Buffer-Swap: Back wird Front, altes Front wird neues Back
void framebuffer_swap(void) {
    uint16_t *tmp = fb.front;
    fb.front = fb.back;
    fb.back  = tmp;
    fb.swapped = true;
}
```

Der Bounce-Buffer fuer DMA liegt im internen SRAM:

```c
#define BOUNCE_BUF_LINES  40   // 40 Zeilen pro DMA-Transfer
#define BOUNCE_BUF_SIZE   (DISPLAY_H_RES * BOUNCE_BUF_LINES * DISPLAY_BPP)

static uint16_t *bounce_buf;  // SRAM, DMA-faehig

esp_err_t bounce_buf_init(void) {
    bounce_buf = heap_caps_malloc(BOUNCE_BUF_SIZE,
                                  MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!bounce_buf) return ESP_ERR_NO_MEM;
    return ESP_OK;
}

// Blit: kopiere Dirty-Region aus PSRAM-Frontbuffer ueber Bounce-Buffer zum Display
void blit_region(int x_start, int y_start, int w, int h) {
    for (int y = y_start; y < y_start + h; y += BOUNCE_BUF_LINES) {
        int lines = min(BOUNCE_BUF_LINES, (y_start + h) - y);
        int offset = y * DISPLAY_H_RES + x_start;
        int copy_bytes = w * lines * DISPLAY_BPP;

        // PSRAM -> SRAM Bounce-Buffer
        memcpy(bounce_buf, &fb.front[offset], copy_bytes);

        // SRAM -> Display via SPI/DMA
        esp_lcd_panel_draw_bitmap(lcd_panel,
                                   x_start, y,
                                   x_start + w, y + lines,
                                   bounce_buf);
    }
}
```

### 1.3 Dirty-Rect-Tracking

Voll-Refreshs sind teuer: 480x320x2 = 307KB pro Frame. Bei 80MHz SPI sind das
~15-20 FPS Maximum. Dirty-Rect-Tracking reduziert das auf geaenderte Bereiche.

```c
#define MAX_DIRTY_RECTS  16

typedef struct {
    int16_t x, y, w, h;
} rect_t;

typedef struct {
    rect_t rects[MAX_DIRTY_RECTS];
    uint8_t count;
} dirty_list_t;

static dirty_list_t dirty;

void dirty_reset(void) {
    dirty.count = 0;
}

void dirty_add(int x, int y, int w, int h) {
    if (dirty.count >= MAX_DIRTY_RECTS) {
        // Liste voll: vereinige zu einem grossen Rechteck
        dirty.rects[0].x = 0;
        dirty.rects[0].y = 0;
        dirty.rects[0].w = DISPLAY_H_RES;
        dirty.rects[0].h = DISPLAY_V_RES;
        dirty.count = 1;
        return;
    }
    // Clip an Display-Grenzen
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > DISPLAY_H_RES) w = DISPLAY_H_RES - x;
    if (y + h > DISPLAY_V_RES) h = DISPLAY_V_RES - y;
    if (w <= 0 || h <= 0) return;

    dirty.rects[dirty.count].x = x;
    dirty.rects[dirty.count].y = y;
    dirty.rects[dirty.count].w = w;
    dirty.rects[dirty.count].h = h;
    dirty.count++;
}

// Compositor: benachbarte/ueberlappende Rects vereinigen
rect_t dirty_merge(rect_t a, rect_t b) {
    int x0 = min(a.x, b.x);
    int y0 = min(a.y, b.y);
    int x1 = max(a.x + a.w, b.x + b.w);
    int y1 = max(a.y + a.h, b.y + b.h);
    return (rect_t){ x0, y0, x1 - x0, y1 - y0 };
}

void dirty_flush_to_display(void) {
    if (dirty.count == 0) return;

    // Vereinige ueberlappende Rects (Simple O(n^2) fuer n<=16)
    for (int i = 0; i < dirty.count - 1; i++) {
        for (int j = i + 1; j < dirty.count; j++) {
            // Pruefe Ueberlappung
            if (dirty.rects[i].x <= dirty.rects[j].x + dirty.rects[j].w &&
                dirty.rects[j].x <= dirty.rects[i].x + dirty.rects[i].w &&
                dirty.rects[i].y <= dirty.rects[j].y + dirty.rects[j].h &&
                dirty.rects[j].y <= dirty.rects[i].y + dirty.rects[i].h) {
                dirty.rects[i] = dirty_merge(dirty.rects[i], dirty.rects[j]);
                dirty.rects[j] = dirty.rects[dirty.count - 1];
                dirty.count--;
                j--;  // Neues Element an Position j erneut pruefen
            }
        }
    }

    // Blit jedes Dirty-Rect
    for (int i = 0; i < dirty.count; i++) {
        blit_region(dirty.rects[i].x, dirty.rects[i].y,
                    dirty.rects[i].w, dirty.rects[i].h);
    }
    dirty_reset();
}
```

### 1.4 Paint-Engine: Grafik-Primitive

Die Paint-Engine zeichnet Primitive (Rects, Circles, Lines, Text, Arcs) in den
Back-Buffer und registriert Dirty-Rects.

```c
typedef struct {
    uint16_t *buf;  // Pointer in Back-Buffer
    int       width;
    int       height;
} canvas_t;

static canvas_t canvas = { 0, DISPLAY_H_RES, DISPLAY_V_RES };

void paint_set_pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= canvas.width || y < 0 || y >= canvas.height) return;
    canvas.buf[y * canvas.width + x] = color;
}

// RGB565: 5 Bit Rot, 6 Bit Gruen, 5 Bit Blau
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void paint_fill_rect(int x, int y, int w, int h, uint16_t color) {
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            paint_set_pixel(x + dx, y + dy, color);
        }
    }
    dirty_add(x, y, w, h);
}

// Wu-Lines mit Anti-Aliasing (vereinfacht: Bresenham + 2-bit Alpha)
void paint_draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        paint_set_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    // Dirty-Rect als Bounding-Box der Linie
    int minx = min(x0, x1), miny = min(y0, y1);
    int maxx = max(x0, x1), maxy = max(y0, y1);
    dirty_add(minx, miny, maxx - minx + 1, maxy - miny + 1);
}

// Kreis mit Mittelpunkt-Algorithmus
void paint_draw_circle(int cx, int cy, int r, uint16_t color) {
    int x = r, y = 0, err = 0;
    while (x >= y) {
        paint_set_pixel(cx + x, cy + y, color);
        paint_set_pixel(cx + y, cy + x, color);
        paint_set_pixel(cx - y, cy + x, color);
        paint_set_pixel(cx - x, cy + y, color);
        paint_set_pixel(cx - x, cy - y, color);
        paint_set_pixel(cx - y, cy - x, color);
        paint_set_pixel(cx + y, cy - x, color);
        paint_set_pixel(cx + x, cy - y, color);
        y++;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) { x--; err += 1 - 2 * x; }
    }
    dirty_add(cx - r, cy - r, 2 * r, 2 * r);
}

// Gefuellter Kreis (fuer Tachometer-Skala)
void paint_fill_circle(int cx, int cy, int r, uint16_t color) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                paint_set_pixel(cx + x, cy + y, color);
            }
        }
    }
    dirty_add(cx - r, cy - r, 2 * r, 2 * r);
}

// Arc fuer Tachometer-Nadel-Skala
void paint_draw_arc(int cx, int cy, int r, int start_deg, int end_deg,
                     uint16_t color) {
    for (int deg = start_deg; deg <= end_deg; deg++) {
        float rad = deg * M_PI / 180.0f;
        int x = cx + (int)(r * cosf(rad));
        int y = cy + (int)(r * sinf(rad));
        paint_set_pixel(x, y, color);
    }
    dirty_add(cx - r, cy - r, 2 * r, 2 * r);
}
```

### 1.5 Custom Font-Format: 4-bit Anti-Aliased Bitmap-Fonts

Standard-Bitmap-Fonts sind 1-bit (Pixel an/aus) und sehen auf TFT-Displays
treppenartig aus. Ein 4-bit Alpha-Channel (16 Graustufen) ermoeglicht
Subpixel-Anti-Aliasing bei moderatem Speicherbedarf.

Font-Format (binary, im Flash als const):

```
Header (16 Bytes):
  magic      u32  0x424F4E46  ("BONF")
  version    u8   1
  bpp        u8   4  (bits per pixel for alpha)
  count      u8   Anzahl Glyphen
  reserved   u8   0
  height     u8   Zeilenhoehe in Pixeln
  baseline   u8   Baseline-Offset
  advance    u8   Default Advance-Width
  reserved2  u16   0
  reserved3  u32   0

Glyph-Table (count * 8 Bytes):
  codepoint  u32  Unicode-Codepoint
  offset     u32  Offset im Bitmap-Data

Per-Glyph:
  width      u8   Breite in Pixeln
  height     u8   Hoehe in Pixeln
  advance    u8   Advance (Cursor-Weiterschub)
  bearing_x  i8   Left Bearing
  bearing_y  i8   Top Bearing
  data[]     width * height * 4 / 8 Bytes (4-bit packed)
```

```c
typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t  version;
    uint8_t  bpp;
    uint8_t  glyph_count;
    uint8_t  reserved1;
    uint8_t  line_height;
    uint8_t  baseline;
    uint8_t  default_advance;
    uint16_t reserved2;
    uint32_t reserved3;
} font_header_t;

typedef struct __attribute__((packed)) {
    uint32_t codepoint;
    uint32_t data_offset;
} font_glyph_entry_t;

typedef struct __attribute__((packed)) {
    uint8_t width;
    uint8_t height;
    uint8_t advance;
    int8_t  bearing_x;
    int8_t  bearing_y;
} font_glyph_meta_t;

// Glyph fuer ein Zeichen finden
const font_glyph_entry_t *font_find_glyph(const font_header_t *font,
                                            uint32_t codepoint) {
    const font_glyph_entry_t *table =
        (const font_glyph_entry_t *)((const uint8_t *)font + sizeof(font_header_t));
    for (int i = 0; i < font->glyph_count; i++) {
        if (table[i].codepoint == codepoint) return &table[i];
    }
    return NULL;  // Glyph nicht gefunden -> Fallback-Quad
}

// Anti-Aliased Text-Rendering mit Alpha-Blending
void paint_draw_char(int x, int y, uint32_t codepoint,
                     const font_header_t *font, uint16_t fg, uint16_t bg) {
    const font_glyph_entry_t *entry = font_find_glyph(font, codepoint);
    if (!entry) {
        paint_fill_rect(x, y, font->default_advance, font->line_height, bg);
        return;
    }
    const uint8_t *data = (const uint8_t *)font + entry->data_offset;
    const font_glyph_meta_t *meta = (const font_glyph_meta_t *)data;
    const uint8_t *bitmap = data + sizeof(font_glyph_meta_t);

    for (int row = 0; row < meta->height; row++) {
        for (int col = 0; col < meta->width; col++) {
            int bit_index = (row * meta->width + col) * 4;  // 4-bit alpha
            int byte_idx = bit_index / 8;
            int nibble   = (bit_index % 8 == 0) ? 0 : 4;
            uint8_t alpha4 = (bitmap[byte_idx] >> nibble) & 0x0F;
            uint8_t alpha = alpha4 * 17;  // 0-255 skalieren

            // Alpha-Blending: out = fg * a + bg * (1-a)
            if (alpha == 0) continue;
            if (alpha == 255) {
                paint_set_pixel(x + meta->bearing_x + col,
                                y + meta->bearing_y + row, fg);
            } else {
                // RGB565 dekomprimieren, blenden, rekomprimieren
                int px = x + meta->bearing_x + col;
                int py = y + meta->bearing_y + row;
                if (px < 0 || px >= canvas.width) continue;
                if (py < 0 || py >= canvas.height) continue;
                uint16_t bg_px = canvas.buf[py * canvas.width + px];
                uint8_t r_bg = (bg_px >> 8) & 0xF8;
                uint8_t g_bg = (bg_px >> 3) & 0xFC;
                uint8_t b_bg = (bg_px << 3) & 0xF8;
                uint8_t r_fg = (fg >> 8) & 0xF8;
                uint8_t g_fg = (fg >> 3) & 0xFC;
                uint8_t b_fg = (fg << 3) & 0xF8;
                uint8_t a = alpha;
                uint8_t r = (r_fg * a + r_bg * (255 - a)) / 255;
                uint8_t g = (g_fg * a + g_bg * (255 - a)) / 255;
                uint8_t b = (b_fg * a + b_bg * (255 - a)) / 255;
                canvas.buf[py * canvas.width + px] = rgb565(r, g, b);
            }
        }
    }
    dirty_add(x, y, meta->advance, font->line_height);
}

void paint_draw_text(int x, int y, const char *utf8_str,
                     const font_header_t *font, uint16_t fg, uint16_t bg) {
    int cursor_x = x;
    while (*utf8_str) {
        uint32_t codepoint = 0;
        uint8_t c = (uint8_t)*utf8_str;
        if (c < 0x80) {
            codepoint = c;
            utf8_str++;
        } else if ((c & 0xE0) == 0xC0) {
            codepoint = (c & 0x1F) << 6 | (*(utf8_str + 1) & 0x3F);
            utf8_str += 2;
        } else if ((c & 0xF0) == 0xE0) {
            codepoint = (c & 0x0F) << 12 | (*(utf8_str + 1) & 0x3F) << 6
                       | (*(utf8_str + 2) & 0x3F);
            utf8_str += 3;
        } else {
            utf8_str++;  // 4-byte UTF-8: vereinfacht ueberspringen
        }
        paint_draw_char(cursor_x, y, codepoint, font, fg, bg);
        cursor_x += font->default_advance;
    }
}
```

### 1.6 Partial-Update Strategie fuer E-Paper

Fuer das optionale E-Paper-Zweitdisplay (Phase 2) wird eine andere Strategie
gebraucht: E-Paper unterstuetzt nur 1-bit (Schwarz/Weiss) bei Partial-Refresh.
Der Ansatz ist ein separater 1-bit Shadow-Buffer mit differentiellem Update.

```c
#define EPAPER_W  296
#define EPAPER_H  128
#define EPAPER_FB_SIZE  (EPAPER_W * EPAPER_H / 8)  // 4736 Bytes

static uint8_t *epaper_current;  // Was auf dem Display steht
static uint8_t *epaper_next;     // Was gemalt wurde
static uint8_t *epaper_diff;     // Differentieller Buffer

// Nur geaenderte Pixel an das Display senden
void epaper_flush_diff(void) {
    // XOR von current und next -> geaenderte Pixel
    for (int i = 0; i < EPAPER_FB_SIZE; i++) {
        epaper_diff[i] = epaper_current[i] ^ epaper_next[i];
    }
    // dirty_add fuer geaenderte Regionen
    // ... (Scanne epaper_diff nach geaenderten Bytes)
    // Sende nur Dirty-Regionen an E-Paper-Controller
    // Kopiere next -> current
    memcpy(epaper_current, epaper_next, EPAPER_FB_SIZE);
}
```

### 1.7 Voll-Refresh fuer TFT mit 30+ FPS

Bei 80MHz SPI-Takt und 480x320 RGB565:

```
Theoretisch: 80.000.000 / (480 * 320 * 2) = ~26 FPS
Praktisch:   ~15-20 FPS (DMA-Overhead, Bounce-Buffer-Copy)
```

Mit Dirty-Rect-Tracking und typischen Dashboard-Updates (nur Zahlen aendern sich)
wird die tatsaechliche Uebertragungsmenge drastisch reduziert. Bei einem
update von 4 Zahlenbereichen (je ~80x40 Pixel) sind das nur 4 * 80 * 40 * 2
= 25.600 Bytes statt 307.200 Bytes -- ein Faktor 12 weniger. Das entspricht
~300+ FPS fuer partielle Updates, limitiert durch die Render-Zeit der
Paint-Engine.

Der Display-Task laeuft mit einer festen Frame-Rate von 30 FPS (33ms Intervall).
Wenn keine Dirty-Rects vorhanden sind, schlaeft der Task und verbraucht keine
CPU.

```c
void display_task(void *arg) {
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t frame_period = pdMS_TO_TICKS(33);  // 30 FPS

    for (;;) {
        // 1. Layout: Widget-Tree traversieren, Positionen berechnen
        layout_update();

        // 2. Paint: Widgets in Back-Buffer zeichnen, Dirty-Rects sammeln
        paint_all_widgets();

        // 3. Swap: Back wird Front
        framebuffer_swap();

        // 4. Blit: Dirty-Regionen zum Display senden
        dirty_flush_to_display();

        // Frame-Rate begrenzen
        vTaskDelayUntil(&last_wake, frame_period);
    }
}
```

---

## 2. Animations-Framework

### 2.1 Tween-Engine

Die Tween-Engine interpoliert einen Wert zwischen Start und Ende ueber eine
definierte Dauer mit einer Easing-Funktion. Sie ist frame-rate-unabhaengig:
alle Berechnungen basieren auf verstrichener Zeit, nicht auf Frame-Counter.

```c
typedef enum {
    EASE_LINEAR,
    EASE_IN_QUAD,
    EASE_OUT_QUAD,
    EASE_IN_OUT_QUAD,
    EASE_IN_CUBIC,
    EASE_OUT_CUBIC,
    EASE_IN_OUT_CUBIC,
    EASE_IN_BACK,
    EASE_OUT_BACK,
    EASE_IN_OUT_BACK,
    EASE_OUT_BOUNCE,
} ease_type_t;

// Easing-Funktionen: t ist normalisierte Zeit 0.0..1.0
static float ease_linear(float t) { return t; }
static float ease_in_quad(float t) { return t * t; }
static float ease_out_quad(float t) { return 1 - (1 - t) * (1 - t); }
static float ease_in_out_quad(float t) {
    return t < 0.5f ? 2 * t * t : 1 - powf(-2 * t + 2, 2) / 2;
}
static float ease_in_cubic(float t) { return t * t * t; }
static float ease_out_cubic(float t) { return 1 - powf(1 - t, 3); }
static float ease_in_out_cubic(float t) {
    return t < 0.5f ? 4 * t * t * t : 1 - powf(-2 * t + 2, 3) / 2;
}
static float ease_in_back(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1;
    return c3 * t * t * t - c1 * t * t;
}
static float ease_out_back(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1;
    return 1 + c3 * powf(t - 1, 3) + c1 * powf(t - 1, 2);
}
static float ease_in_out_back(float t) {
    const float c1 = 1.70158f, c2 = c1 * 1.525f;
    return t < 0.5f
        ? (powf(2 * t, 2) * ((c2 + 1) * 2 * t - c2)) / 2
        : (powf(2 * t - 2, 2) * ((c2 + 1) * (t * 2 - 2) + c2) + 2) / 2;
}
static float ease_out_bounce(float t) {
    const float n1 = 7.5625f, d1 = 2.75f;
    if (t < 1 / d1) return n1 * t * t;
    if (t < 2 / d1) { t -= 1.5f / d1; return n1 * t * t + 0.75f; }
    if (t < 2.5f / d1) { t -= 2.25f / d1; return n1 * t * t + 0.9375f; }
    t -= 2.625f / d1; return n1 * t * t + 0.984375f;
}

typedef float (*ease_fn)(float);

static ease_fn ease_functions[] = {
    ease_linear, ease_in_quad, ease_out_quad, ease_in_out_quad,
    ease_in_cubic, ease_out_cubic, ease_in_out_cubic,
    ease_in_back, ease_out_back, ease_in_out_back,
    ease_out_bounce,
};
```

### 2.2 Animations-Struct und Update-Logik

```c
typedef enum {
    ANIM_TWEEN,     // Wert-Interpolation mit Easing
    ANIM_SPRING,    // Physikbasiert (Masse-Feder-Daempfer)
} anim_type_t;

typedef struct {
    anim_type_t type;
    ease_type_t ease;

    // Tween-Parameter
    float start_value;
    float end_value;
    float current_value;

    // Zeit-Parameter
    uint32_t start_time_ms;   // Zeitstempel bei Start
    uint32_t duration_ms;

    // Spring-Parameter
    float position;           // Aktuelle Position
    float velocity;           // Aktuelle Geschwindigkeit
    float target;             // Zielwert
    float stiffness;          // Federkonstante (k)
    float damping;            // Daempfung (c)
    float mass;               // Masse (m)

    // Status
    bool active;
    bool finished;

    // Callback bei Wert-Aenderung
    void (*on_update)(float value, void *user_data);
    void *user_data;
} animation_t;

#define MAX_ANIMATIONS 32
static animation_t animations[MAX_ANIMATIONS];

// Tween starten
int anim_tween_start(float from, float to, uint32_t duration_ms,
                     ease_type_t ease,
                     void (*on_update)(float, void*), void *user_data) {
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (!animations[i].active) {
            animations[i].type = ANIM_TWEEN;
            animations[i].ease = ease;
            animations[i].start_value = from;
            animations[i].end_value = to;
            animations[i].current_value = from;
            animations[i].start_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            animations[i].duration_ms = duration_ms;
            animations[i].active = true;
            animations[i].finished = false;
            animations[i].on_update = on_update;
            animations[i].user_data = user_data;
            return i;
        }
    }
    return -1;  // Kein freier Slot
}

// Spring-Animation starten (fuer Drehzahl-Nadel)
int anim_spring_start(float from, float to,
                       float stiffness, float damping, float mass,
                       void (*on_update)(float, void*), void *user_data) {
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (!animations[i].active) {
            animations[i].type = ANIM_SPRING;
            animations[i].position = from;
            animations[i].velocity = 0;
            animations[i].target = to;
            animations[i].stiffness = stiffness;
            animations[i].damping = damping;
            animations[i].mass = mass;
            animations[i].start_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            animations[i].active = true;
            animations[i].finished = false;
            animations[i].on_update = on_update;
            animations[i].user_data = user_data;
            return i;
        }
    }
    return -1;
}

// Alle aktiven Animationen updaten (pro Frame aufrufen)
void anim_update_all(void) {
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (!animations[i].active) continue;

        if (animations[i].type == ANIM_TWEEN) {
            uint32_t elapsed = now - animations[i].start_time_ms;
            float t = (float)elapsed / (float)animations[i].duration_ms;
            if (t >= 1.0f) {
                t = 1.0f;
                animations[i].finished = true;
                animations[i].active = false;
            }
            float eased = ease_functions[animations[i].ease](t);
            animations[i].current_value =
                animations[i].start_value +
                (animations[i].end_value - animations[i].start_value) * eased;

            if (animations[i].on_update) {
                animations[i].on_update(animations[i].current_value,
                                        animations[i].user_data);
            }
        } else if (animations[i].type == ANIM_SPRING) {
            // Fest-Daempfer-System: F = -k*x - c*v, a = F/m
            // x = Abweichung vom Ziel, v = Geschwindigkeit
            uint32_t elapsed = now - animations[i].start_time_ms;
            // Fixed timestep fuer Physik (16ms = ~60Hz)
            static uint32_t last_physics_ms = 0;
            if (now - last_physics_ms < 16) continue;
            float dt = (now - last_physics_ms) / 1000.0f;
            last_physics_ms = now;

            float displacement = animations[i].position - animations[i].target;
            float force = -animations[i].stiffness * displacement
                         - animations[i].damping * animations[i].velocity;
            float acceleration = force / animations[i].mass;
            animations[i].velocity += acceleration * dt;
            animations[i].position += animations[i].velocity * dt;

            animations[i].current_value = animations[i].position;

            if (animations[i].on_update) {
                animations[i].on_update(animations[i].current_value,
                                        animations[i].user_data);
            }

            // Beenden wenn praktisch stillstehend
            if (fabsf(displacement) < 0.01f &&
                fabsf(animations[i].velocity) < 0.01f) {
                animations[i].position = animations[i].target;
                animations[i].finished = true;
                animations[i].active = false;
                if (animations[i].on_update) {
                    animations[i].on_update(animations[i].target,
                                            animations[i].user_data);
                }
            }
        }
    }
}
```

### 2.3 Interruptible Animations

Wenn ein neuer Zielwert gesetzt wird, waehrend eine Animation laeuft, startet
die neue Animation von der aktuellen Position. Die alte Animation wird
abgebrochen.

```c
// Neue Animation bricht laufende ab und startet von aktueller Position
int anim_tween_to(float to, uint32_t duration_ms, ease_type_t ease,
                   void (*on_update)(float, void*), void *user_data) {
    // Finde laufende Animation mit gleichem Callback (gleiche Eigenschaft)
    float current = to;  // Default falls keine laeuft
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (animations[i].active && animations[i].on_update == on_update) {
            current = animations[i].current_value;
            animations[i].active = false;  // Abbrechen
            break;
        }
    }
    return anim_tween_start(current, to, duration_ms, ease,
                             on_update, user_data);
}
```

### 2.4 Animation-Timeline: Boot-Sequenz

Die Timeline orchestriert sequenzielle und parallele Animationen fuer den
Boot-Prozess: Logo einblenden -> Loading-Bar -> Instruments einfahren.

```c
typedef enum {
    TIMELINE_BOOT_LOGO,
    TIMELINE_BOOT_LOADING,
    TIMELINE_BOOT_INSTRUMENTS,
    TIMELINE_RUNNING,
} timeline_phase_t;

static timeline_phase_t current_phase = TIMELINE_BOOT_LOGO;
static uint32_t phase_start_ms = 0;

void timeline_start_boot(void) {
    current_phase = TIMELINE_BOOT_LOGO;
    phase_start_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    // Logo: Fade-In ueber 800ms
    anim_tween_start(0.0f, 1.0f, 800, EASE_OUT_CUBIC,
                     on_logo_opacity_update, NULL);
}

void timeline_update(void) {
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t elapsed = now - phase_start_ms;

    switch (current_phase) {
    case TIMELINE_BOOT_LOGO:
        if (elapsed > 1000) {  // 800ms Animation + 200ms Pause
            current_phase = TIMELINE_BOOT_LOADING;
            phase_start_ms = now;
            // Loading-Bar: 0 -> 100% ueber 1500ms
            anim_tween_start(0.0f, 100.0f, 1500, EASE_IN_OUT_QUAD,
                             on_loading_progress_update, NULL);
        }
        break;
    case TIMELINE_BOOT_LOADING:
        if (elapsed > 1700) {
            current_phase = TIMELINE_BOOT_INSTRUMENTS;
            phase_start_ms = now;
            // RPM-Nadel: von 0 auf aktuelle Drehzahl mit Overshoot
            anim_spring_start(0, current_rpm, 120, 14, 1,
                               on_rpm_needle_update, NULL);
            // Speed: einfades
            anim_tween_start(0, 1.0f, 600, EASE_OUT_CUBIC,
                             on_speed_opacity_update, NULL);
        }
        break;
    case TIMELINE_BOOT_INSTRUMENTS:
        if (elapsed > 800) {
            current_phase = TIMELINE_RUNNING;
        }
        break;
    case TIMELINE_RUNNING:
        // Im Running-State: RPM-Nadel folgt aktuellen Werten mit Spring-Physics
        break;
    }
}
```

### 2.5 Frame-Rate-Independent: Drehzahl-Nadel mit Traegheit

Die Drehzahl-Nadel ist das Paradebeispiel fuer physikbasierte Animation. Bei
schnellem Gasgeben soll die Nadel mit leichtem Overshoot folgen, bei
Gaswegnehmen sanft zurueckfallen. Die Spring-Physik sorgt fuer ein natuerliches
Gefuehl.

```c
// Wird bei jedem neuen RPM-Wert aus dem Sensor-Task aufgerufen
void on_new_rpm(float new_rpm) {
    // Spring-Ziel aktualisieren -- Nadel bewegt sich von selbst
    // Finde aktive Spring-Animation und aktualisiere Ziel
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (animations[i].active && animations[i].type == ANIM_SPRING &&
            animations[i].on_update == on_rpm_needle_update) {
            animations[i].target = new_rpm;
            return;
        }
    }
    // Keine aktive Animation -- neue starten
    anim_spring_start(0, new_rpm, 150, 18, 1,
                       on_rpm_needle_update, NULL);
}

// Callback: Nadel-Winkel aus RPM berechnen und Dirty-Rect setzen
void on_rpm_needle_update(float value, void *user_data) {
    // RPM -> Winkel: 0 rpm = -120 Grad, 8000 rpm = +120 Grad
    float angle = (value / 8000.0f) * 240.0f - 120.0f;
    // Nadel zeichnen (altes Dirty-Rect loeschen, neues zeichnen)
    render_rpm_needle(angle);
}
```

Die Frame-Rate-Unabhaengigkeit ist durch die Zeitstempel-basierte Berechnung
gewaehrleistet: egal ob die Display-Task mit 30 oder 60 FPS laeuft, die
Animation erreicht ihr Ziel in der gleichen Zeit. Die Physik-Simulation nutzt
einen festen 16ms-Zeitschritt, unabhaengig von der Frame-Rate.

---

## 3. Sensor-Daten-Pipeline

### 3.1 Pipeline-Architektur

Jeder Sensor durchlaeuft eine dreistufige Pipeline: Raw -> Filtered -> Displayed.

```
  +--------+     +----------+     +-----------+     +---------+
  | Sensor | --> | Raw      | --> | Filter    | --> | Display |
  | HW     |     | Read     |     | (Smooth)  |     | Driver  |
  +--------+     +----------+     +-----------+     +---------+
                    |                  |                |
                    v                  v                v
                 Queue            Animation         Dirty-Rect
                 (logger)         Driver
```

```c
// Globale Datenstruktur fuer alle gefilterten Sensordaten
typedef struct {
    // Powertrain
    float rpm;              // Drehzahl
    float speed_kmh;        // Geschwindigkeit
    int8_t gear;            // Gang (0=Leerlauf, 1-5)
    bool  neutral;          // Leerlaufschalter

    // Thermals
    float coolant_temp_c;   // Kuehlmitteltemperatur
    float oil_pressure_bar; // Oeldruck
    float ambient_temp_c;   // Umgebungstemperatur (BMP280)

    // Fuel
    float fuel_liters;      // Kraftstoffstand
    float fuel_percent;     // Kraftstoff %

    // Orientation
    float roll_deg;         // Seitenneigung
    float pitch_deg;        // Längsneigung

    // GPS
    float lat, lon;         // Position
    float gps_speed_kmh;    // GPS-Geschwindigkeit
    float altitude_m;       // Hoehe
    uint8_t satellites;     // Satellitenanzahl
    float hdop;             // Horizontale Praezision
    uint8_t gps_fix;        // 0=none, 1=2D, 2=3D, 3=DGPS

    // Power
    float battery_v;       // Bordspannung
    bool  generator_ok;     // Generator laeuft (>13.8V)

    // Timestamp
    uint32_t timestamp;
} sensor_data_t;

static sensor_data_t g_sensor_data;
static SemaphoreHandle_t sensor_data_mutex;

// Der sensor_task auf Core 1 faellt alle Sensoren ab und fuellt die Pipeline
void sensor_task(void *arg) {
    const TickType_t period = pdMS_TO_TICKS(20);  // 50 Hz
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        xSemaphoreTake(sensor_data_mutex, portMAX_DELAY);

        // --- Stage 1: Raw Read ---
        int16_t ax, ay, az, gx, gy, gz;
        mpu6050_read_all(&ax, &ay, &az, &gx, &gy, &gz);

        int pulse_count;
        pcnt_unit_get_count(rpm_pcnt_unit, &pulse_count);
        pcnt_unit_clear_count(rpm_pcnt_unit);

        // ADC-Werte (16x Oversampling)
        uint32_t coolant_raw = read_adc_avg(ADC_CHANNEL_6, 16);
        uint32_t fuel_raw    = read_adc_avg(ADC_CHANNEL_7, 16);
        uint32_t oil_raw     = read_adc_avg(ADC_CHANNEL_5, 16);
        uint32_t vbat_raw    = read_adc_avg(ADC_CHANNEL_4, 16);

        // --- Stage 2: Convert + Filter ---
        // RPM: Pulse -> RPM -> Moving Average
        float raw_rpm = (float)pulse_count * 120.0f;  // 500ms Fenster
        g_sensor_data.rpm = moving_average_filter(raw_rpm);

        // Coolant: ADC -> R -> Temp (Steinhart-Hart) -> EMA
        float coolant_r = adc_to_resistance(coolant_raw);
        float coolant_t = steinhart_hart(coolant_r);
        g_sensor_data.coolant_temp_c = ema_filter(coolant_t, 0.1f);

        // Fuel: ADC -> R -> Liters (Lookup) -> EMA
        float fuel_r = adc_to_resistance_fuel(fuel_raw);
        float fuel_l = fuel_lookup_liters(fuel_r);
        g_sensor_data.fuel_liters = ema_filter(fuel_l, 0.05f);

        // Oil: ADC -> V -> Bar (Linear) -> Threshold-Check
        float oil_v = adc_to_voltage(oil_raw);
        float oil_bar = (oil_v - 0.5f) / 4.0f * 10.0f;
        g_sensor_data.oil_pressure_bar = ema_filter(oil_bar, 0.2f);

        // IMU: Raw -> Complementary Filter -> Roll/Pitch
        sensor_fusion_update(ax, ay, az, gx, gy, gz);
        // (roll/pitch werden in sensor_fusion_update gesetzt)

        // Battery: ADC -> V -> Generator-Status
        float vbat = adc_to_vbat(vbat_raw);
        g_sensor_data.battery_v = ema_filter(vbat, 0.1f);
        g_sensor_data.generator_ok = (vbat > 13.8f);

        // --- Stage 3: Derived ---
        g_sensor_data.gear = gear_detect(g_sensor_data.rpm,
                                          g_sensor_data.speed_kmh);
        g_sensor_data.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // An Animation-Driver weitergeben
        on_new_rpm(g_sensor_data.rpm);

        xSemaphoreGive(sensor_data_mutex);

        // An Logger und BLE weiterleiten
        xQueueOverwrite(sensor_data_queue, &g_sensor_data);

        vTaskDelayUntil(&last_wake, period);
    }
}
```

### 3.2 Filter-Implementierungen

**Moving-Average (RPM):** Fenstergroesse 5, Median fuer EMV-Stabilitaet.

```c
#define RPM_FILTER_SIZE 5
static float rpm_buffer[RPM_FILTER_SIZE] = {0};
static int rpm_buf_idx = 0;

float moving_average_filter(float new_rpm) {
    rpm_buffer[rpm_buf_idx] = new_rpm;
    rpm_buf_idx = (rpm_buf_idx + 1) % RPM_FILTER_SIZE;

    // Median-Filter (robuster gegen Ausreisser als Mittelwert)
    float sorted[RPM_FILTER_SIZE];
    memcpy(sorted, rpm_buffer, sizeof(rpm_buffer));
    for (int i = 0; i < RPM_FILTER_SIZE - 1; i++)
        for (int j = 0; j < RPM_FILTER_SIZE - i - 1; j++)
            if (sorted[j] > sorted[j+1]) {
                float tmp = sorted[j];
                sorted[j] = sorted[j+1];
                sorted[j+1] = tmp;
            }
    return sorted[RPM_FILTER_SIZE / 2];
}
```

**Exponential Moving Average (EMA):** Fuer Temperatur, Kraftstoff, Bordspannung.

```c
// alpha: 0.0 = kein Update (gefaehrlich), 1.0 = kein Filter (roh)
// Kleines alpha = starke Glaettung, grosses alpha = schnelle Reaktion
float ema_filter(float new_value, float alpha) {
    static float smoothed = 0;
    static bool initialized = false;
    if (!initialized) {
        smoothed = new_value;
        initialized = true;
    }
    smoothed = alpha * new_value + (1.0f - alpha) * smoothed;
    return smoothed;
}
```

**Steinhart-Hart (Kuehlmitteltemperatur, NTC 10k B3950):**

```c
float steinhart_hart(float r_ntc) {
    // 1/T = 1/T0 + (1/B) * ln(R/R0)
    float t0 = 298.15f;    // 25C in Kelvin
    float r0 = 10000.0f;   // 10k @ 25C
    float b = 3950.0f;
    float temp_k = 1.0f / (1.0f/t0 + (1.0f/b) * logf(r_ntc / r0));
    return temp_k - 273.15f;  // Celsius
}
```

**Complementary Filter (IMU, Roll/Pitch):**

```c
#define COMP_ALPHA  0.98f
#define IMU_DT      0.02f  // 50 Hz

static float roll = 0, pitch = 0;

void sensor_fusion_update(int16_t ax, int16_t ay, int16_t az,
                           int16_t gx, int16_t gy, int16_t gz) {
    // Rohwerte in physikalische Einheiten
    float ax_g = ax / 8192.0f;  // +-4g
    float ay_g = ay / 8192.0f;
    float az_g = az / 8192.0f;
    float gx_dps = gx / 16.4f;  // +-2000 deg/s
    float gy_dps = gy / 16.4f;
    float gz_dps = gz / 16.4f;

    // Accel-basierte Winkel (langfristig stabil)
    float accel_roll  = atan2f(ay_g, az_g) * 180.0f / M_PI;
    float accel_pitch = atan2f(-ax_g,
        sqrtf(ay_g * ay_g + az_g * az_g)) * 180.0f / M_PI;

    // Complementary Filter: Gyro (kurzfristig) + Accel (langfristig)
    roll  = COMP_ALPHA * (roll  + gx_dps * IMU_DT)
            + (1.0f - COMP_ALPHA) * accel_roll;
    pitch = COMP_ALPHA * (pitch + gy_dps * IMU_DT)
            + (1.0f - COMP_ALPHA) * accel_pitch;

    // In globale Datenstruktur schreiben
    xSemaphoreTake(sensor_data_mutex, portMAX_DELAY);
    g_sensor_data.roll_deg = roll;
    g_sensor_data.pitch_deg = pitch;
    xSemaphoreGive(sensor_data_mutex);
}
```

**Kalman-Filter (GPS, 1-D vereinfacht fuer Geschwindigkeit):**

```c
typedef struct {
    float x;     // Geschätzter Zustand (Geschwindigkeit)
    float p;     // Schaetzung der Unsicherheit
    float q;     // Prozessrauschen
    float r;     // Messrauschen
    float k;     // Kalman-Gain
} kalman1d_t;

static kalman1d_t gps_kalman = { 0, 1, 0.01f, 0.5f, 0 };

float kalman_update(kalman1d_t *k, float measurement) {
    // Predict
    k->p = k->p + k->q;
    // Update
    k->k = k->p / (k->p + k->r);
    k->x = k->x + k->k * (measurement - k->x);
    k->p = (1 - k->k) * k->p;
    return k->x;
}
```

### 3.3 Crash-Detection (IMU)

Der Ringbuffer speichert die letzten 5 Sekunden IMU-Rohdaten. Bei einem
Crash-Ereignis (Beschleunigung > Schwelle) wird der Buffer gesichert und als
DTC geloggt.

```c
#define CRASH_THRESHOLD_G  3.0f   // 3g = starker Aufprall
#define IMU_RINGBUF_SIZE   8192   // 5s bei 50Hz * 24 Bytes = 6000 Bytes

static RingbufHandle_t imu_ringbuf;

void check_crash(float ax_g, float ay_g, float az_g) {
    float total_g = sqrtf(ax_g*ax_g + ay_g*ay_g + az_g*az_g);
    if (total_g > CRASH_THRESHOLD_G) {
        // Crash erkannt -- DTC setzen
        dtc_log(DTC_CRASH_DETECTED, DTC_SEVERITY_CRITICAL,
                "Crash: %.1fg impact detected", total_g);
        // Ringbuffer-Inhalt auf SD sichern
        trigger_crash_log_dump();
    }
}
```

---

## 4. BLE-Stack Implementation

### 4.1 NimBLE GATT-Server Setup

Der BLE-Stack basiert auf NimBLE (nicht Bluedroid) wegen des geringeren
RAM-Verbrauchs (~50% weniger) und der besseren Wartung. Der ESP32-S3
unterstuetzt nur BLE (kein Classic), womit NimBLE die optimale Wahl ist.

```c
#include "esp_bt.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"

#define MAX_CONNECTIONS  1
#define MAX_BONDS        3
#define DEVICE_NAME      "Transalp-BC"

static uint16_t conn_handle_active = BLE_HS_CONN_HANDLE_NONE;
static bool ble_connected = false;

// GATT-Service-Handles (werden beim Registrieren zugewiesen)
static uint16_t svc_telemetry_handle;
static uint16_t svc_config_handle;
static uint16_t svc_nav_handle;
static uint16_t svc_ota_handle;
static uint16_t svc_diag_handle;

// Characteristic-Handles fuer Notifications
static uint16_t chr_gps_handle;
static uint16_t chr_rpm_handle;
static uint16_t chr_sensors_handle;
static uint16_t chr_battery_handle;
static uint16_t chr_ota_status_handle;
static uint16_t chr_diag_handle;

void ble_init(void) {
    ESP_ERROR_CHECK(nimble_port_init());

    // Security- und Bonding-Konfiguration
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;   // Just Works
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_sc = 1;                          // LE Secure Connections
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;

    // Device-Name setzen
    ble_svc_gap_device_name_set(DEVICE_NAME);

    // Services registrieren
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ESP_ERROR_CHECK(ble_gatts_count_cfg(gatt_services));
    ESP_ERROR_CHECK(ble_gatts_add_svcs(gatt_services));

    // Host-Task starten
    nimble_port_freertos_init(ble_host_task);
}
```

### 4.2 Service- und Characteristic-Definitionen

Fuenf Services: Telemetry, Config, Navigation, OTA und Diagnose.

```
+------------------------------------------------------------------+
|                    GATT SERVICE MAP                              |
+------------------------------------------------------------------+
|                                                                  |
|  Telemetry (0xA001)                                             |
|  +----------------+--------+------+-----------------------------+ |
|  | GPS-Data        | notify | 20B  | lat, lon, speed, alt, sats | |
|  | RPM-Speed-Gear  | notify | 8B   | rpm, speed_kmh, gear       | |
|  | Sensors         | notify | 16B  | temp, alt, roll, pitch     | |
|  | Battery         | read   | 4B   | voltage, percent           | |
|  +----------------+--------+------+-----------------------------+ |
|                                                                  |
|  Config (0xA002)                                                |
|  +----------------+--------+------+-----------------------------+ |
|  | Display-Mode   | rw     | 1B   | 0=Tour, 1=Track, 2=Config  | |
|  | Log-Interval   | rw     | 1B   | 1-60 seconds               | |
|  | Calibration     | w      | var  | IMU-offsets, sea-level     | |
|  | Device-Info     | r      | var  | FW-version, uptime         | |
|  +----------------+--------+------+-----------------------------+ |
|                                                                  |
|  Navigation (0xA003)                                            |
|  +----------------+--------+------+-----------------------------+ |
|  | Next-Turn       | w      | var  | dir, dist_m, street       | |
|  | ETA             | w      | 8B   | arrival_time, dist_total   | |
|  | Route-Active    | rw     | 1B   | bool                       | |
|  +----------------+--------+------+-----------------------------+ |
|                                                                  |
|  OTA (0xA004)                                                   |
|  +----------------+--------+------+-----------------------------+ |
|  | OTA-Control     | w      | 1B   | start/cancel/verify       | |
|  | OTA-Data        | w      | var  | firmware chunks             | |
|  | OTA-Status      | notify | 4B   | progress%, error           | |
|  +----------------+--------+------+-----------------------------+ |
|                                                                  |
|  Diagnose (0xA005)                                             |
|  +----------------+--------+------+-----------------------------+ |
|  | DTC-Log         | notify | var  | active trouble codes       | |
|  | Diag-Command    | w      | 1B   | trigger self-test/report    | |
|  | Sensor-Report   | r      | var  | detailed sensor status     | |
|  +----------------+--------+------+-----------------------------+ |
|                                                                  |
+------------------------------------------------------------------+
```

```c
// Telemetry-Service UUID: 0000A001-0000-1000-8000-00805F9B34FB
static const ble_uuid128_t svc_telemetry_uuid =
    BLE_UUID128_INIT(0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
                     0x00, 0x10, 0x00, 0x00, 0x01, 0xA0, 0x00, 0x00);

// Characteristic UUIDs (vereinfacht: letzte 2 Bytes variieren)
static const ble_uuid128_t chr_gps_uuid =
    BLE_UUID128_INIT(0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
                     0x00, 0x10, 0x00, 0x00, 0x11, 0xA0, 0x00, 0x00);

// Access-Callback fuer alle Characteristics
static int gatt_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg) {
    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR:
        return handle_read(attr_handle, ctxt);
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        return handle_write(attr_handle, ctxt);
    default:
        return BLE_ATT_ERR_UNLIKELY;
    }
}

// GATT-Service-Tabelle
static const struct ble_gatt_svc_def gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_telemetry_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &chr_gps_uuid.u,
                .access_cb = gatt_access_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &chr_gps_handle,
            },
            // ... weitere Characteristics ...
            { 0 },  // Terminator
        },
    },
    // ... weitere Services ...
    { 0 },  // Service-Terminator
};
```

### 4.3 Connection-Parameters und MTU

```c
static int ble_gap_event_handler(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        conn_handle_active = event->connect.conn_handle;
        ble_connected = true;

        // MTU auf 247 verhandeln (BLE 5.0 Maximum)
        ble_att_set_preferred_mtu(247);
        ble_gattc_exchange_mtu(conn_handle_active, NULL, NULL);

        // Connection-Parameters fuer niedrige Latenz anfordern
        struct ble_gap_upd_params conn_params = {
            .itvl_min = 0x0006,    // 7.5 ms
            .itvl_max = 0x0010,    // 20 ms
            .latency = 0,
            .supervision_timeout = 0x00C8,  // 2s
        };
        ble_gap_update_params(conn_handle_active, &conn_params);

        // Connection-Watchdog starten
        start_connection_watchdog();
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        conn_handle_active = BLE_HS_CONN_HANDLE_NONE;
        ble_connected = false;
        stop_connection_watchdog();
        // Advertising neu starten
        ble_advertising_start();
        break;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU negotiated: %d", event->mtu.value);
        break;

    case BLE_GAP_EVENT_NOTIFY_TX:
        // Notification wurde gesendet (oder fehlgeschlagen)
        break;
    }
    return 0;
}

static struct ble_gap_adv_params adv_params = {
    .conn_mode = BLE_GAP_CONN_MODE_UND,  // Connectable undirected
    .disc_mode = BLE_GAP_DISC_MODE_GEN,  // General discoverable
    .itvl_min = 0x0020,  // 20 ms min
    .itvl_max = 0x0040,  // 40 ms max
};
```

### 4.4 Notification-Strategie

Welche Daten werden wie oft gepusht?

| Characteristic | Rate | Trigger | Payload |
|---|---|---|---|
| GPS-Data | 1 Hz | Timer | lat, lon, speed, alt, sats (20B) |
| RPM-Speed-Gear | 2 Hz | Timer | rpm, speed, gear (8B) |
| Sensors | 0.5 Hz | Timer | temp, alt, roll, pitch (16B) |
| Battery | 0.1 Hz | Timer | voltage, percent (4B) |
| OTA-Status | Event | On-Chunk | progress%, error (4B) |
| DTC-Log | Event | On-Change | code, severity, ts (12B) |

Die Raten sind bewusst niedrig gehalten, um Energie zu sparen und die
BLE-Bandbreite nicht zu blockieren. Die App kann bei Bedarf hoehere Raten
anfordern (ueber eine Config-Characteristic).

```c
// Timer-basierte Notifications
void ble_notify_telemetry(void) {
    if (!ble_connected) return;

    // GPS-Data (1 Hz)
    static uint32_t last_gps_notify = 0;
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (now - last_gps_notify >= 1000) {
        last_gps_notify = now;

        gps_notify_t pkt = {
            .lat = g_sensor_data.lat,
            .lon = g_sensor_data.lon,
            .speed = g_sensor_data.gps_speed_kmh,
            .alt = g_sensor_data.altitude_m,
            .sats = g_sensor_data.satellites,
        };
        struct os_mbuf *om = ble_hs_mbuf_from_flat(&pkt, sizeof(pkt));
        ble_gattc_notify(conn_handle_active, chr_gps_handle, om);
    }

    // RPM-Speed-Gear (2 Hz)
    static uint32_t last_rpm_notify = 0;
    if (now - last_rpm_notify >= 500) {
        last_rpm_notify = now;

        rpm_notify_t pkt = {
            .rpm = (uint16_t)g_sensor_data.rpm,
            .speed = (uint8_t)(g_sensor_data.speed_kmh),
            .gear = (int8_t)g_sensor_data.gear,
        };
        struct os_mbuf *om = ble_hs_mbuf_from_flat(&pkt, sizeof(pkt));
        ble_gattc_notify(conn_handle_active, chr_rpm_handle, om);
    }
}
```

### 4.5 Robustheit: Reconnect-Logic und Connection-Watchdog

```c
static TimerHandle_t watchdog_timer;
static uint32_t last_rx_time = 0;

#define WATCHDOG_TIMEOUT_MS  10000  // 10s ohne Empfang -> Disconnect erzwingen

void watchdog_callback(TimerHandle_t timer) {
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (ble_connected && (now - last_rx_time) > WATCHDOG_TIMEOUT_MS) {
        ESP_LOGW(TAG, "Connection watchdog: no RX for %lu ms, disconnecting",
                 now - last_rx_time);
        ble_gap_terminate(conn_handle_active, BLE_ERR_REM_USER_CONN_TERM);
    }
}

void start_connection_watchdog(void) {
    last_rx_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    watchdog_timer = xTimerCreate("ble_wdt", pdMS_TO_TICKS(5000),
                                   pdTRUE, NULL, watchdog_callback);
    xTimerStart(watchdog_timer, 0);
}

void stop_connection_watchdog(void) {
    if (watchdog_timer) {
        xTimerStop(watchdog_timer, 0);
        xTimerDelete(watchdog_timer, 0);
        watchdog_timer = NULL;
    }
}

// Graceful-Degradation: wenn BLE nicht verfuegbar, laeuft der Bordcomputer
// autonom weiter. Sensoren, Display und Logger arbeiten unabhaengig von BLE.
// BLE ist ein Add-On, kein kritischer Pfad.
```

### 4.6 Pairing/Bonding: Just-Works mit Secure Connections

Just-Works ist fuer diesen Use-Case ausreichend: keine sensiblen Daten,
keine Tastatur am Geraet. LE Secure Connections (ECDH P-256) bietet Schutz
gegen Man-in-the-Middle-Angriffe trotz fehlendem Passkey. Bonding speichert
die Schluessel im NVS -- beim erneuten Verbinden entfaellt das Pairing.

---

## 5. Diagnose-System

### 5.1 DTC-System (Diagnostic Trouble Codes)

Das DTC-System ist eine eigene Implementierung, angelehnt an OBD-II-Style
Codes, aber fuer dieses Projekt massgeschneidert.

```c
typedef enum {
    DTC_SEV_INFO     = 0,
    DTC_SEV_WARN     = 1,
    DTC_SEV_ERROR    = 2,
    DTC_SEV_CRITICAL = 3,
} dtc_severity_t;

typedef struct {
    char     code[5];       // z.B. "P0101", "S0201", "C0301", "B0401"
    uint8_t  severity;
    char     description[48];
    uint32_t timestamp;
    bool     active;
} dtc_entry_t;

#define MAX_DTC_ENTRIES 64
static dtc_entry_t dtc_table[MAX_DTC_ENTRIES];
static uint8_t dtc_count = 0;
static SemaphoreHandle_t dtc_mutex;
```

DTC-Kategorien:

```
P0xxx  Powertrain   (Motor, Getriebe, Antrieb)
  P0101  RPM-Signal unplausibel (ausserhalb 0-12000)
  P0102  RPM-Signal ausgefallen (0 Pulse > 5s bei laufendem Motor)
  P0201  Kuehlmitteltemperatur > 115C (Warnung)
  P0202  Kuehlmitteltemperatur > 120C (Kritisch -- Motor stoppen)
  P0301  Oeldruck < 0.5 bar bei laufendem Motor
  P0302  Oeldruck > 6 bar (Ueberdruck)
  P0401  Bordspannung < 11.5V (Batterie schwach)
  P0402  Bordspannung > 15.5V (Regler defekt)
  P0501  Kraftstoffstand < 2L (Reserve)

S0xxx  Sensors     (Sensor-Ausfaelle)
  S0101  I2C: IMU nicht erreichbar (WHO_AM_I falsch)
  S0102  I2C: BMP280 nicht erreichbar
  S0201  GPS: kein Fix innerhalb 30s
  S0202  GPS: HDOP > 5 (schlechte Position)
  S0301  ADC: Kuehlmittel-ADC unplausibel (< 0.1V oder > 3.2V)
  S0302  ADC: Kraftstoff-ADC unplausibel
  S0303  ADC: Oeldruck-ADC unplausibel
  S0401  IMU: Crash erkannt (> 3g)

C0xxx  Comms       (Kommunikation)
  C0101  BLE: Advertising konnte nicht starten
  C0102  BLE: Verbindung unerwartet getrennt
  C0103  BLE: MTU-Verhandlung fehlgeschlagen
  C0201  SD-Karte: Mount fehlgeschlagen
  C0202  SD-Karte: Schreibfehler
  C0203  SD-Karte: fast voll (< 10% frei)

B0xxx  Body        (System-Intern)
  B0101  OTA-Update fehlgeschlagen (Flash-Schreibfehler)
  B0102  OTA-Image Verification fehlgeschlagen (SHA256 mismatch)
  B0103  OTA-Rollback ausgeloest
  B0201  NVS: Lesefehler
  B0202  Config-Migration: Schema-Version mismatch
  B0301  Watchdog-Timeout -> Neustart
  B0302  Stack-Overflow erkannt
  B0401  Boot-Loop erkannt (>3 Resets in 60s)
```

### 5.2 DTC-Logging

```c
void dtc_log(const char *code, dtc_severity_t sev, const char *fmt, ...) {
    xSemaphoreTake(dtc_mutex, portMAX_DELAY);

    // Pruefen ob DTC bereits aktiv ist
    for (int i = 0; i < dtc_count; i++) {
        if (strcmp(dtc_table[i].code, code) == 0 && dtc_table[i].active) {
            xSemaphoreGive(dtc_mutex);
            return;  // Bereits aktiv, nicht doppelt loggen
        }
    }

    if (dtc_count >= MAX_DTC_ENTRIES) {
        // Tabelle voll: aeltesten nicht-aktiven Eintrag ueberschreiben
        for (int i = 0; i < dtc_count; i++) {
            if (!dtc_table[i].active) {
                // Diesen Slot wiederverwenden
                memmove(&dtc_table[i], &dtc_table[i+1],
                        (dtc_count - i - 1) * sizeof(dtc_entry_t));
                dtc_count--;
                break;
            }
        }
    }

    // Neuen DTC anlegen
    dtc_entry_t *entry = &dtc_table[dtc_count];
    strncpy(entry->code, code, sizeof(entry->code) - 1);
    entry->severity = sev;
    entry->timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    entry->active = true;

    va_list args;
    va_start(args, fmt);
    vsnprintf(entry->description, sizeof(entry->description), fmt, args);
    va_end(args);
    dtc_count++;

    xSemaphoreGive(dtc_mutex);

    // Auf SD-Karte loggen
    dtc_log_to_sd(entry);

    // Per BLE notify (falls verbunden)
    dtc_notify_ble(entry);

    // Bei kritischen DTCs: Immediate Action
    if (sev == DTC_SEV_CRITICAL) {
        ESP_LOGE(TAG, "CRITICAL DTC: %s - %s", code, entry->description);
        // z.B. Warn-Screen auf Display anzeigen
        display_show_warning(code, entry->description);
    }
}

void dtc_log_to_sd(const dtc_entry_t *entry) {
    xSemaphoreTake(spi_mutex, portMAX_DELAY);
    FILE *f = fopen("/sdcard/logs/dtc.log", "a");
    if (f) {
        fprintf(f, "[%lu] %s [%d] %s\n",
                entry->timestamp, entry->code,
                entry->severity, entry->description);
        fflush(f);
        fsync(fileno(f));
        fclose(f);
    }
    xSemaphoreGive(spi_mutex);
}
```

### 5.3 Self-Test-Routine beim Boot

```c
typedef struct {
    const char *name;
    bool (*test_fn)(void);
    const char *dtc_code_on_fail;
    dtc_severity_t severity_on_fail;
} self_test_t;

bool test_imu(void) {
    uint8_t who_am_i;
    mpu6050_read_regs(0x75, &who_am_i, 1);
    return (who_am_i == 0x68 || who_am_i == 0x70);
}

bool test_bmp280(void) {
    uint8_t id;
    bmp280_read_regs(0xD0, &id, 1);
    return (id == 0x58);
}

bool test_gps_fix(void) {
    // 30 Sekunden auf GPS-Fix warten
    for (int i = 0; i < 300; i++) {
        if (g_sensor_data.gps_fix > 0) return true;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    return false;
}

bool test_sd_card(void) {
    FILE *f = fopen("/sdcard/logs/selftest.tmp", "w");
    if (!f) return false;
    fprintf(f, "selftest ok");
    fclose(f);
    f = fopen("/sdcard/logs/selftest.tmp", "r");
    if (!f) return false;
    char buf[16];
    fgets(buf, sizeof(buf), f);
    fclose(f);
    remove("/sdcard/logs/selftest.tmp");
    return (strcmp(buf, "selftest ok") == 0);
}

bool test_battery_voltage(void) {
    float v = g_sensor_data.battery_v;
    return (v > 3.0f && v < 18.0f);  // Plausibler Bereich
}

bool test_ble_advertising(void) {
    // Versuche Advertising zu starten
    int rc = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER,
                                &adv_params, ble_gap_event_handler, NULL);
    if (rc == 0) {
        ble_gap_adv_stop();
        return true;
    }
    return false;
}

bool test_imu_movement(void) {
    // Pruefe ob IMU auf Bewegung reagiert (Beschleunigung nicht exakt 0)
    int16_t ax, ay, az, gx, gy, gz;
    mpu6050_read_all(&ax, &ay, &az, &gx, &gy, &gz);
    float total = sqrtf((float)(ax*ax + ay*ay + az*az));
    return (total > 1000.0f);  // Irgendeine Beschleunigung messbar
}

static const self_test_t self_tests[] = {
    { "IMU Presence",       test_imu,            "S0101", DTC_SEV_ERROR },
    { "BMP280 Presence",    test_bmp280,         "S0102", DTC_SEV_ERROR },
    { "GPS Fix (30s)",     test_gps_fix,         "S0201", DTC_SEV_WARN },
    { "SD Card R/W",       test_sd_card,         "C0201", DTC_SEV_ERROR },
    { "Battery Voltage",   test_battery_voltage, "P0401", DTC_SEV_WARN },
    { "BLE Advertising",   test_ble_advertising, "C0101", DTC_SEV_WARN },
    { "IMU Movement",      test_imu_movement,    "S0101", DTC_SEV_INFO },
};

#define NUM_SELF_TESTS  (sizeof(self_tests) / sizeof(self_tests[0]))

void run_self_tests(void) {
    ESP_LOGI(TAG, "=== Self-Test Start ===");

    for (int i = 0; i < NUM_SELF_TESTS; i++) {
        ESP_LOGI(TAG, "Test %d/%d: %s...", i+1, NUM_SELF_TESTS,
                 self_tests[i].name);

        bool passed = self_tests[i].test_fn();

        if (passed) {
            ESP_LOGI(TAG, "  PASS");
        } else {
            ESP_LOGW(TAG, "  FAIL -> DTC %s", self_tests[i].dtc_code_on_fail);
            dtc_log(self_tests[i].dtc_code_on_fail,
                     self_tests[i].severity_on_fail,
                     "Self-test failed: %s", self_tests[i].name);
        }
    }
    ESP_LOGI(TAG, "=== Self-Test Complete ===");
}
```

### 5.4 Diagnose-Mode

Der Diagnose-Mode kann ueber Seriell (USB-CDC) oder BLE ausgeloest werden und
liefert einen detaillierten Sensor-Report.

```c
// Serielles Kommando-Interface
void diag_serial_handler(void) {
    char line[128];
    if (!fgets(line, sizeof(line), stdin)) return;

    if (strncmp(line, "DIAG", 4) == 0) {
        print_full_sensor_report();
    } else if (strncmp(line, "DTC", 3) == 0) {
        print_dtc_table();
    } else if (strncmp(line, "CLEAR_DTC", 9) == 0) {
        dtc_clear_all();
        printf("DTC table cleared\n");
    } else if (strncmp(line, "SELFTEST", 8) == 0) {
        run_self_tests();
    } else if (strncmp(line, "SENSOR ", 7) == 0) {
        // SENSOR <name> -> detaillierter Report fuer einen Sensor
        print_sensor_detail(line + 7);
    }
}

void print_full_sensor_report(void) {
    xSemaphoreTake(sensor_data_mutex, portMAX_DELAY);

    printf("\n=== SENSOR REPORT ===\n");
    printf("Timestamp:    %lu ms\n", g_sensor_data.timestamp);
    printf("RPM:          %.0f\n", g_sensor_data.rpm);
    printf("Speed:        %.1f km/h\n", g_sensor_data.speed_kmh);
    printf("Gear:         %d\n", g_sensor_data.gear);
    printf("Coolant:      %.1f C\n", g_sensor_data.coolant_temp_c);
    printf("Oil:          %.2f bar\n", g_sensor_data.oil_pressure_bar);
    printf("Fuel:         %.1f L (%.0f%%)\n", g_sensor_data.fuel_liters,
           g_sensor_data.fuel_percent);
    printf("Roll:         %.1f deg\n", g_sensor_data.roll_deg);
    printf("Pitch:        %.1f deg\n", g_sensor_data.pitch_deg);
    printf("GPS Lat/Lon:  %.6f / %.6f\n", g_sensor_data.lat,
           g_sensor_data.lon);
    printf("GPS Speed:    %.1f km/h\n", g_sensor_data.gps_speed_kmh);
    printf("GPS Alt:      %.1f m\n", g_sensor_data.altitude_m);
    printf("GPS Sats:     %d (HDOP: %.1f, Fix: %d)\n",
           g_sensor_data.satellites, g_sensor_data.hdop, g_sensor_data.gps_fix);
    printf("Battery:      %.2f V (Gen: %s)\n", g_sensor_data.battery_v,
           g_sensor_data.generator_ok ? "OK" : "OFF");
    printf("Active DTCs:  %d\n", dtc_count_active());
    printf("=====================\n\n");

    xSemaphoreGive(sensor_data_mutex);
}
```

---

## 6. OTA-Update Pipeline

### 6.1 Ueberblick

Die OTA-Pipeline uebertraegt ein Firmware-Image ueber BLE in 4KB Chunks,
schreibt es in die inaktive Flash-Partition, verifiziert es mit SHA256 und
fuehrt bei Bedarf einen Rollback durch.

```
  +-----------+     +-----------+     +-----------+     +-----------+
  | Companion | --> | BLE OTA   | --> | Flash     | --> | Verify    |
  | App       |     | Service   |     | Write     |     | SHA256    |
  +-----------+     +-----------+     +-----------+     +-----------+
                          |               |               |
                          v               v               v
                    CRC per Chunk    Dual-Partition   Hash Compare
                    Progress Notify  esp_ota_*        Rollback if mismatch
```

### 6.2 BLE-Transfer: 4KB Chunks mit CRC

```c
#define OTA_CHUNK_SIZE    4096
#define OTA_CHUNK_OVERHEAD 8  // offset(4) + len(2) + crc(2)
#define OTA_MTU_PAYLOAD   244 // MTU 247 - 3 ATT-Header

// Ein 4KB Chunk wird in mehreren BLE-Paketen uebertragen
// Jedes BLE-Paket: [seq(2)] [data(242)]
// Ein kompletter Chunk: 4096 / 242 = ~17 Pakete

typedef struct __attribute__((packed)) {
    uint32_t offset;        // Byte-Offset im Firmware-Image
    uint16_t length;        // Datenlaenge in diesem Chunk (<= 4096)
    uint16_t crc16;         // CRC16 ueber data[]
    uint8_t  data[OTA_CHUNK_SIZE];
} ota_chunk_t;

static esp_ota_handle_t ota_handle;
static const esp_partition_t *ota_partition = NULL;
static uint32_t ota_total_size = 0;
static uint32_t ota_bytes_written = 0;
static bool ota_in_progress = false;

esp_err_t ota_begin(uint32_t total_size) {
    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) {
        ESP_LOGE(TAG, "No OTA partition available");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle));
    ota_total_size = total_size;
    ota_bytes_written = 0;
    ota_in_progress = true;

    ESP_LOGI(TAG, "OTA begin: partition=%s, size=%lu",
             ota_partition->label, total_size);
    return ESP_OK;
}

esp_err_t ota_write_chunk(const ota_chunk_t *chunk) {
    if (!ota_in_progress) return ESP_ERR_INVALID_STATE;

    // CRC pruefen
    uint16_t computed_crc = crc16_ccitt(chunk->data, chunk->length);
    if (computed_crc != chunk->crc16) {
        ESP_LOGE(TAG, "OTA chunk CRC mismatch at offset %lu: got %04X, exp %04X",
                 chunk->offset, computed_crc, chunk->crc16);
        // App muss Chunk erneut senden
        ota_notify_status(0xFF, "CRC mismatch");  // Error code
        return ESP_ERR_INVALID_CRC;
    }

    // In Flash schreiben
    esp_err_t err = esp_ota_write(ota_handle, chunk->data, chunk->length);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA write failed at offset %lu: %s",
                 chunk->offset, esp_err_to_name(err));
        dtc_log("B0101", DTC_SEV_ERROR, "OTA flash write failed at offset %lu",
                chunk->offset);
        return err;
    }

    ota_bytes_written += chunk->length;

    // Fortschritt an App melden
    uint8_t progress = (ota_total_size > 0)
        ? (uint8_t)(ota_bytes_written * 100 / ota_total_size)
        : 0;
    ota_notify_status(progress, NULL);

    return ESP_OK;
}

esp_err_t ota_finish_and_verify(void) {
    if (!ota_in_progress) return ESP_ERR_INVALID_STATE;

    ESP_ERROR_CHECK(esp_ota_end(ota_handle));

    // SHA256-Verifikation
    uint8_t hash[32];
    esp_err_t err = esp_partition_get_sha256(ota_partition, hash);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SHA256 computation failed: %s", esp_err_to_name(err));
        dtc_log("B0102", DTC_SEV_ERROR, "OTA SHA256 computation failed");
        return err;
    }

    // Hash mit erwartetem Hash vergleichen (von App uebermittelt)
    if (memcmp(hash, expected_ota_hash, 32) != 0) {
        ESP_LOGE(TAG, "OTA image SHA256 mismatch!");
        dtc_log("B0102", DTC_SEV_ERROR, "OTA image verification failed");
        return ESP_ERR_INVALID_VERSION;  // Image ist nicht vertrauenswuerdig
    }

    ESP_LOGI(TAG, "OTA image verified, setting boot partition");
    ESP_ERROR_CHECK(esp_ota_set_boot_partition(ota_partition));

    ota_in_progress = false;
    dtc_log("B0100", DTC_SEV_INFO, "OTA update completed successfully");

    // Neustart
    esp_restart();
    return ESP_OK;  // Unreachable
}
```

### 6.3 CRC16-Implementierung

```c
uint16_t crc16_ccitt(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}
```

### 6.4 Rollback-Mechanismus

Bei aktivem Rollback-Schutz wird eine neue Firmware nicht sofort als gueltig
markiert. Sie muss sich beim naechsten Boot bewaehren.

```c
// Beim Boot pruefen ob Firmware neu ist und validiert werden muss
void ota_check_on_boot(void) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    esp_ota_get_state_partition(running, &state);

    if (state == ESP_OTA_IMG_PENDING_VERIFY) {
        ESP_LOGI(TAG, "New firmware pending verification");

        // Self-Test durchfuehren
        bool ok = firmware_self_test();

        if (ok) {
            esp_ota_mark_app_valid_cancel_rollback();
            ESP_LOGI(TAG, "Firmware validated, rollback cancelled");
            dtc_log("B0100", DTC_SEV_INFO, "Firmware validated after OTA");
        } else {
            ESP_LOGE(TAG, "Self-test failed, rolling back");
            dtc_log("B0103", DTC_SEV_CRITICAL,
                    "Self-test failed after OTA, rolling back");
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
    }
}

// Firmware-Self-Test: kurzer Gesundheitstest nach Update
bool firmware_self_test(void) {
    // 1. FreeRTOS laeuft (implizit, sonst waeren wir nicht hier)
    // 2. NVS lesbar
    nvs_handle_t h;
    if (nvs_open("config", NVS_READONLY, &h) != ESP_OK) return false;
    nvs_close(h);
    // 3. Display ansprechbar
    if (!display_self_test()) return false;
    // 4. BLE-Stack initialisierbar
    if (!ble_self_test()) return false;
    // 5. Innerhalb von 30 Sekunden kein Watchdog-Timeout
    //    (implizit durch erfolgreiches Durchlaufen der Init-Sequenz)
    return true;
}
```

### 6.5 Progress-Notification an App

```c
void ota_notify_status(uint8_t progress, const char *error_msg) {
    if (!ble_connected) return;

    typedef struct __attribute__((packed)) {
        uint8_t  progress;    // 0-100, oder 0xFF bei Fehler
        uint8_t  error_code;  // 0 = OK, sonst Fehlernummer
    } ota_status_t;

    ota_status_t status = {
        .progress = progress,
        .error_code = (error_msg != NULL) ? 0xFF : 0,
    };

    struct os_mbuf *om = ble_hs_mbuf_from_flat(&status, sizeof(status));
    ble_gattc_notify(conn_handle_active, chr_ota_status_handle, om);
}
```

---

## 7. Konfigurations-System

### 7.1 NVS-basiertes Config-System

```c
#include "nvs_flash.h"

#define CONFIG_NAMESPACE  "bc_cfg"
#define CONFIG_VERSION    3

typedef struct __attribute__((packed)) {
    uint16_t version;        // Schema-Version fuer Migrationen

    // Kalibrierung
    int16_t  gyro_offset[3];       // IMU Gyro-Offsets
    int16_t  accel_offset[3];     // IMU Accel-Offsets
    uint32_t bmp_sea_level_pa;     // QNH-Referenzdruck
    uint16_t adc_vref_mv;          // ADC-Referenzspannung

    // Fuel-Kalibrierung (6 Stuetzpunkte)
    float    fuel_cal_liters[6];
    uint16_t fuel_cal_adc[6];

    // Schwellen
    uint16_t rpm_redline;          // 8000
    uint16_t rpm_shift_light;      // 7000
    float    battery_low_v;        // 11.5
    float    battery_shutdown_v;   // 10.5
    float    oil_pressure_warn;    // 0.5 bar
    float    oil_pressure_high;    // 6.0 bar
    float    coolant_temp_warn;    // 115C
    float    coolant_temp_crit;    // 120C

    // Logging
    uint8_t  log_interval_s;       // 1-60
    uint8_t  log_format;           // 0=CSV, 1=Binary, 2=Both

    // GPS
    uint8_t  gps_rate_hz;          // 1, 5, 10

    // Display
    uint8_t  display_mode;         // 0=Tour, 1=Track, 2=Nav, 3=Config
    uint8_t  display_invert;       // 0=Normal, 1=Inverted
    uint8_t  display_brightness;  // 0-255 (Backlight-PWM)

    // BLE
    char     device_name[16];

    // Animation
    uint8_t  anim_enabled;         // 0=Off, 1=On
    uint8_t  rpm_needle_spring;    // 0=Linear, 1=Spring-Physics
} device_config_t;

static device_config_t g_config;

// Defaults
static const device_config_t default_config = {
    .version = CONFIG_VERSION,
    .bmp_sea_level_pa = 101325,
    .adc_vref_mv = 1100,
    .rpm_redline = 8000,
    .rpm_shift_light = 7000,
    .battery_low_v = 11.5f,
    .battery_shutdown_v = 10.5f,
    .oil_pressure_warn = 0.5f,
    .oil_pressure_high = 6.0f,
    .coolant_temp_warn = 115.0f,
    .coolant_temp_crit = 120.0f,
    .log_interval_s = 5,
    .log_format = 0,
    .gps_rate_hz = 10,
    .display_mode = 0,
    .display_invert = 0,
    .display_brightness = 200,
    .device_name = "Transalp-BC",
    .anim_enabled = 1,
    .rpm_needle_spring = 1,
};
```

### 7.2 Config laden mit Default-Merge und Migration

```c
esp_err_t config_load(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No config in NVS, using defaults");
        g_config = default_config;
        return ESP_OK;
    }

    size_t len = sizeof(g_config);
    err = nvs_get_blob(handle, "dev_cfg", &g_config, &len);

    if (err == ESP_ERR_NVS_INVALID_LENGTH) {
        // Struktur-Groesse hat sich geaendert -> Migration noetig
        ESP_LOGW(TAG, "Config size mismatch (stored=%d, expected=%d), migrating",
                 (int)len, (int)sizeof(g_config));
        config_migrate(handle, &g_config);
    } else if (err != ESP_OK) {
        ESP_LOGW(TAG, "Config read error: %s, using defaults", esp_err_to_name(err));
        g_config = default_config;
    } else {
        // Version pruefen
        if (g_config.version < CONFIG_VERSION) {
            ESP_LOGW(TAG, "Config version %d -> %d, migrating",
                     g_config.version, CONFIG_VERSION);
            config_migrate(handle, &g_config);
        }
    }
    nvs_close(handle);
    return ESP_OK;
}

// Migration: alte Werte behalten, neue Felder mit Defaults fuellen
void config_migrate(nvs_handle_t handle, device_config_t *cfg) {
    device_config_t old = *cfg;
    *cfg = default_config;  // Alle Defaults laden

    // Bekannte Felder aus alter Konfiguration uebernehmen
    // (nur Felder die in der alten Version existierten)
    if (old.version >= 1) {
        cfg->bmp_sea_level_pa = old.bmp_sea_level_pa;
        cfg->rpm_redline = old.rpm_redline;
        cfg->rpm_shift_light = old.rpm_shift_light;
        cfg->log_interval_s = old.log_interval_s;
        cfg->display_mode = old.display_mode;
    }
    if (old.version >= 2) {
        cfg->gps_rate_hz = old.gps_rate_hz;
        cfg->display_invert = old.display_invert;
        strncpy(cfg->device_name, old.device_name, sizeof(cfg->device_name));
    }
    if (old.version >= 3) {
        cfg->display_brightness = old.display_brightness;
        cfg->anim_enabled = old.anim_enabled;
    }
    cfg->version = CONFIG_VERSION;

    // Migrierte Konfiguration zurueckschreiben
    nvs_set_blob(handle, "dev_cfg", cfg, sizeof(*cfg));
    nvs_commit(handle);

    dtc_log("B0202", DTC_SEV_INFO,
            "Config migrated from v%d to v%d", old.version, CONFIG_VERSION);
}

esp_err_t config_save(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    g_config.version = CONFIG_VERSION;
    err = nvs_set_blob(handle, "dev_cfg", &g_config, sizeof(g_config));
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}
```

### 7.3 Live-Update ueber BLE

Konfigurationsaenderungen ueber BLE werden sofort angewendet, ohne Reboot
(sofern moeglich).

```c
// Wenn eine Config-Characteristic geschrieben wird:
int handle_config_write(uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt) {
    // Payload: [key_id(1)] [value(var)]
    uint8_t key_id;
    uint16_t value_len = OS_MBUF_PKTLEN(ctxt->om) - 1;
    uint8_t value[32];

    ble_hs_mbuf_to_flat(ctxt->om, &key_id, 1, NULL);
    ble_hs_mbuf_to_flat(ctxt->om, value, value_len, NULL);

    switch (key_id) {
    case CONFIG_KEY_DISPLAY_MODE:
        g_config.display_mode = value[0];
        display_set_mode(value[0]);  // Live-Update: sofort Screen wechseln
        break;
    case CONFIG_KEY_DISPLAY_INVERT:
        g_config.display_invert = value[0];
        display_set_invert(value[0]);  // Live-Update: invert anwenden
        break;
    case CONFIG_KEY_BRIGHTNESS:
        g_config.display_brightness = value[0];
        display_set_brightness(value[0]);  // Live-Update: PWM anpassen
        break;
    case CONFIG_KEY_LOG_INTERVAL:
        g_config.log_interval_s = value[0];
        logger_set_interval(value[0]);  // Live-Update: Logger-Rate aendern
        break;
    case CONFIG_KEY_GPS_RATE:
        g_config.gps_rate_hz = value[0];
        gps_set_rate(value[0]);  // Live-Update: UBX-Kommando an GPS-Modul
        break;
    case CONFIG_KEY_DEVICE_NAME:
        strncpy(g_config.device_name, (char*)value,
                sizeof(g_config.device_name) - 1);
        ble_svc_gap_device_name_set(g_config.device_name);  // Live-Update
        break;
    // Schwellwerte und Kalibrierung erfordern kein Reboot
    case CONFIG_KEY_RPM_REDLINE:
        g_config.rpm_redline = value[0] | (value[1] << 8);
        break;
    case CONFIG_KEY_CALIB_IMU:
        // IMU-Kalibrierung: Offsets direkt anwenden
        memcpy(g_config.gyro_offset, value, 6);
        memcpy(g_config.accel_offset, value + 6, 6);
        mpu6050_apply_offsets(g_config.gyro_offset, g_config.accel_offset);
        break;
    default:
        return BLE_ATT_ERR_UNLIKELY;
    }

    // Aenderung in NVS persistieren (asynchron, nicht im BLE-Callback)
    config_save();
    return 0;
}
```

### 7.4 JSON-Export/Import

```c
// Export als JSON ueber seriell oder BLE
void config_export_json(void) {
    printf("{\n");
    printf("  \"version\": %d,\n", g_config.version);
    printf("  \"calibration\": {\n");
    printf("    \"sea_level_pa\": %lu,\n", g_config.bmp_sea_level_pa);
    printf("    \"gyro_offset\": [%d, %d, %d],\n",
           g_config.gyro_offset[0], g_config.gyro_offset[1],
           g_config.gyro_offset[2]);
    printf("    \"accel_offset\": [%d, %d, %d]\n",
           g_config.accel_offset[0], g_config.accel_offset[1],
           g_config.accel_offset[2]);
    printf("  },\n");
    printf("  \"thresholds\": {\n");
    printf("    \"rpm_redline\": %u,\n", g_config.rpm_redline);
    printf("    \"battery_low_v\": %.1f,\n", g_config.battery_low_v);
    printf("    \"coolant_warn_c\": %.1f,\n", g_config.coolant_temp_warn);
    printf("    \"coolant_crit_c\": %.1f\n", g_config.coolant_temp_crit);
    printf("  },\n");
    printf("  \"logging\": {\n");
    printf("    \"interval_s\": %u,\n", g_config.log_interval_s);
    printf("    \"format\": %u\n", g_config.log_format);
    printf("  },\n");
    printf("  \"display\": {\n");
    printf("    \"mode\": %u,\n", g_config.display_mode);
    printf("    \"invert\": %u,\n", g_config.display_invert);
    printf("    \"brightness\": %u\n", g_config.display_brightness);
    printf("  },\n");
    printf("  \"ble\": {\n");
    printf("    \"device_name\": \"%s\"\n", g_config.device_name);
    printf("  }\n");
    printf("}\n");
}
```

---

## 8. Task-Architektur und Inter-Task-Kommunikation

### 8.1 Vollstaendige Task-Liste

| Task | Core | Prio | Stack | Rate | Subsystem |
|---|---|---|---|---|---|
| sensor_task | 1 | 5 | 4096 | 50 Hz | Pipeline |
| gps_task | 1 | 4 | 4096 | 10 Hz | Pipeline |
| display_task | 0 | 2 | 8192 | 30 FPS | Rendering |
| ble_task | 0 | 3 | 6144 | Event | BLE |
| logger_task | 0 | 3 | 4096 | Event | Logging |
| power_task | 0 | 6 | 2048 | 1 Hz | Power |
| anim_task | 0 | 4 | 2048 | 60 Hz | Animation |
| ota_task | 0 | 1 | 6144 | Event | OTA |

### 8.2 Render-Loop Integration

Der Animations-Task laeuft mit 60 Hz und berechnet Animationswerte. Der
Display-Task rendert mit 30 FPS. Die Entkopplung stellt sicher, dass
Animationen fluessig laufen, auch wenn das Display mal ein Frame auslaesst.

```c
void anim_task(void *arg) {
    const TickType_t period = pdMS_TO_TICKS(16);  // ~60 Hz
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        anim_update_all();
        timeline_update();
        vTaskDelayUntil(&last_wake, period);
    }
}

// In app_main: alle Tasks erstellen
void app_main(void) {
    // Hardware initialisieren
    config_load();
    i2c_init();
    adc_init();
    pcnt_init();
    sd_init();
    display_init();
    framebuffer_init();
    bounce_buf_init();

    // BLE initialisieren
    ble_init();

    // RTOS-Primitives erstellen
    sensor_data_mutex = xSemaphoreCreateMutex();
    dtc_mutex = xSemaphoreCreateMutex();
    spi_mutex = xSemaphoreCreateMutex();
    sensor_data_queue = xQueueCreate(4, sizeof(sensor_data_t));

    // Self-Test
    run_self_tests();

    // OTA-Check (Rollback-Verify falls neu gebootet)
    ota_check_on_boot();

    // Tasks erstellen
    xTaskCreatePinnedToCore(sensor_task,  "sensor",  4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(gps_task,     "gps",     4096, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(display_task, "display", 8192, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(anim_task,    "anim",    2048, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(ble_task,     "ble",     6144, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(logger_task,  "logger",  4096, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(power_task,   "power",   2048, NULL, 6, NULL, 0);

    ESP_LOGI(TAG, "All tasks started. Free heap: %lu",
             (unsigned long)esp_get_free_heap_size());
}
```

---

## 9. Render-Pipeline: Vollstaendiges Beispiel

Dieses Beispiel zeigt den kompletten Render-Pfad fuer die
Tachometer-Anzeige (Speed + RPM-Nadel):

```
[Sensor-Task: 50Hz]
       |
       | rpm = moving_average(raw_rpm)
       v
[on_new_rpm(): setzt Spring-Animation-Ziel]
       |
       v
[Anim-Task: 60Hz]
       |
       | anim_update_all() -> spring physics -> on_rpm_needle_update(angle)
       v
[Paint: zeichnet Nadel in Back-Buffer, setzt Dirty-Rect]
       |
       v
[Display-Task: 30FPS]
       |
       | framebuffer_swap()
       | dirty_flush_to_display() -> blit_region() -> SPI/DMA
       v
[TFT-Display]
```

```c
// Vollstaendiger Render-Zyklus fuer einen Frame
void display_task(void *arg) {
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t frame_period = pdMS_TO_TICKS(33);

    // Canvas mit Back-Buffer verknuepfen
    canvas.buf = fb.back;

    for (;;) {
        // 1. Layout: Widget-Positionen berechnen (basierend auf Display-Mode)
        layout_update();

        // 2. Paint: Widgets in Back-Buffer zeichnen
        //    (nur wenn Animations-Werte sich geaendert haben)
        if (anim_values_changed()) {
            // Hintergrund loeschen (nur im betroffenen Bereich)
            paint_fill_rect(0, 0, DISPLAY_H_RES, DISPLAY_V_RES,
                            g_config.display_invert ? 0x0000 : 0xFFFF);

            // Tachometer-Skala zeichnen
            render_speed_display();
            render_rpm_gauge();
            render_gear_indicator();
            render_status_bar();
            render_warnings();
        }

        // 3. Swap
        framebuffer_swap();

        // 4. Blit: Dirty-Regionen zum Display senden
        dirty_flush_to_display();

        // 5. BLE-Telemetry senden (falls verbunden)
        ble_notify_telemetry();

        vTaskDelayUntil(&last_wake, frame_period);
    }
}
```

---

## 10. Speicher-Budget

| Komponente | Flash | RAM (intern) | RAM (PSRAM) |
|---|---|---|---|
| ESP-IDF + FreeRTOS | ~800 KB | ~80 KB | -- |
| NimBLE Stack | ~150 KB | ~20 KB | -- |
| WiFi (falls aktiv) | ~300 KB | ~50 KB | -- |
| Display-Treiber + Framebuffer | ~20 KB | ~32 KB (Bounce) | ~614 KB (2x FB) |
| Font-Data (2 Fonts, 4-bit AA) | ~120 KB | -- | -- |
| Sensor-Treiber | ~15 KB | ~4 KB | -- |
| BLE GATT Services | ~8 KB | ~2 KB | -- |
| OTA + Diagnose | ~12 KB | ~4 KB | -- |
| Logger (Buffer) | ~5 KB | ~4 KB | -- |
| Config-System | ~3 KB | ~0.5 KB | -- |
| **Gesamt (ohne App-Partition)** | **~1.4 MB** | **~200 KB** | **~614 KB** |

Bei 16 MB Flash und 8 MB PSRAM ist reichlich Platz. Die App-Partition ist
4 MB, von der ~1.4 MB verwendet werden -- genug Reserve fuer zukuenftige
Erweiterungen.

---

## 11. Quellen

- ESP-IDF v5.4 API Reference: https://docs.espressif.com/projects/esp-idf/en/v5.4.4/esp32s3/
- esp_lcd Component (Framebuffer, Bounce Buffer): https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32s3/api-reference/peripherals/lcd.html
- NimBLE Host Documentation: https://mynewt.apache.org/latest/network/index.html
- ESP-IDF OTA API: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/ota.html
- ESP-IDF NVS Flash: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html
- FreeRTOS SMP auf ESP32: https://docs.espressif.com/projects/esp-idf/en/v5.4.4/esp32s3/api-reference/system/freertos.html
- LVGL Buffer-Empfehlung fuer ESP32-S3: https://forum.lvgl.io/t/create-buffers-in-psram-esp32s3/11555
- GxEPD2 E-Paper Library: https://github.com/ZinggJM/GxEPD2
- minmea NMEA Parser: https://github.com/kosma/minmea
- Easing Functions Referenz: https://easings.net/
- Spring Physics in UI: https://developer.mozilla.org/en-US/docs/Web/CSS/animation-timing-function
