/*
 * widgets.c — UI-Widgets: Speed, RPM-Bar, Gear, Nav-Arrow
 *
 * Zeichnet in den Framebuffer (Renderer).
 * Layout siehe docs/research/03-software-stack.md Abschnitt 4.5.
 *
 * TODO: Font-Rendering implementieren (Bitmap-Fonts).
 * TODO: Bitmap-Zeichnung-Primitive: fillRect, drawLine, drawCircle.
 * TODO: Layout-Engine fuer verschiedene Display-Aufloesungen.
 */

#include "display.h"
#include "board_config.h"
#include "esp_log.h"

static const char *TAG = "widgets";

void widgets_draw_speed(uint16_t speed_kmh)
{
    /* TODO: Geschwindigkeit als grosse Zahl im oberen linken Bereich.
     * Font: FreeMonoBold24pt oder aehnlich.
     * Position: x=5, y=30 (siehe Layout in 03-software-stack.md).
     */
    (void)speed_kmh;
}

void widgets_draw_rpm_bar(uint16_t rpm, uint16_t redline)
{
    /* TODO: RPM als horizontale Bar (rechts neben Speed).
     * Farbe/Invertierung bei Shift-Light (rpm > 7000) und Redline (rpm > 8000).
     * TODO: Gang-Anzeige neben RPM.
     */
    (void)rpm;
    (void)redline;
}

void widgets_draw_gear(uint8_t gear)
{
    /* TODO: Grosser Gang-Indikator (N, 1, 2, 3, 4, 5).
     * Position: rechts neben RPM-Bar.
     */
    (void)gear;
}

void widgets_draw_altitude(float altitude_m)
{
    /* TODO: Hoehe ueber NN in Metern.
     * Position: untere Haelfte, rechts.
     */
    (void)altitude_m;
}

void widgets_draw_nav_arrow(uint16_t direction_deg, uint32_t distance_m)
{
    /* TODO: Navigations-Pfeil (Richtung) + Distanz.
     * Nur sichtbar wenn Route aktiv (per BLE von Smartphone empfangen).
     * TODO: Pfeil rotieren entsprechend direction_deg.
     */
    (void)direction_deg;
    (void)distance_m;
}
