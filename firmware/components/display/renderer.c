/*
 * renderer.c — Framebuffer-Verwaltung und Double-Buffering
 *
 * Verwaltet einen virtuellen Framebuffer, in den Widgets zeichnen.
 * Der Renderer uebertraegt den Framebuffer an das aktive Display
 * (E-Paper oder TFT) mittels Partial- oder Full-Refresh.
 *
 * TODO: Double-Buffering: Front- und Back-Buffer fuer differentielles Update.
 * TODO: Dirty-Region-Tracking (nur geaenderte Bereiche refreshen).
 * TODO: Font-Rendering (Adafruit_GFX oder eigen).
 */

#include "display.h"
#include "board_config.h"
#include "esp_log.h"

static const char *TAG = "renderer";

static display_type_t active_display = DISPLAY_TYPE_NONE;
static bool dirty = false;

void renderer_init(void)
{
    /* TODO: Display-Treiber initialisieren (E-Paper oder TFT).
     *
     * // Zur Laufzeit erkennen oder aus NVS-Konfiguration laden.
     * active_display = DISPLAY_TYPE_EPAPER;
     * epaper_init();
     * // oder:
     * // active_display = DISPLAY_TYPE_TFT;
     * // tft_init();
     */

    ESP_LOGI(TAG, "Renderer init (stub)");
}

void renderer_clear(void)
{
    /* TODO: Framebuffer mit Hintergrundfarbe fuellen.
     * if (active_display == DISPLAY_TYPE_EPAPER) {
     *     memset(epaper_get_framebuffer(), 0xFF, EPD_BUF_SIZE);  // white
     * } else if (active_display == DISPLAY_TYPE_TFT) {
     *     memset(tft_get_framebuffer(), 0x00, TFT_BUF_SIZE);  // black
     * }
     */
    dirty = true;
}

void renderer_flush(bool partial)
{
    if (!dirty) return;

    /* TODO: Framebuffer an Display senden.
     * if (active_display == DISPLAY_TYPE_EPAPER) {
     *     epaper_refresh(partial);
     * } else if (active_display == DISPLAY_TYPE_TFT) {
     *     tft_flush();
     * }
     */

    dirty = false;
    (void)partial;
}
