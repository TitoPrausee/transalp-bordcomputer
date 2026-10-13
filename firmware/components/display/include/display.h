#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * display.h — Display-Subsystem: E-Paper und TFT Treiber, Renderer, Widgets
 *
 * Display-Task laeuft auf Core 0 mit 1Hz (E-Paper) oder hoeher (TFT).
 * SPI: MOSI=11, SCLK=12, CS=10, DC=14, RST=21, BUSY=47.
 */

typedef enum {
    DISPLAY_TYPE_NONE = 0,
    DISPLAY_TYPE_EPAPER,     /* 2.9" b/w (296x128) */
    DISPLAY_TYPE_TFT,        /* 2.4" IPS (240x320) */
} display_type_t;

typedef enum {
    DISPLAY_MODE_TOUR = 0,   /* Standard: Speed, RPM, Hoehe, Temp */
    DISPLAY_MODE_TRACK,      /* GPS-Fokus: Position, HDOP, Distanz */
    DISPLAY_MODE_CONFIG,     /* Einstellungen: BLE, Kalibrierung */
} display_mode_t;

/* API */
void display_init(void);
void display_task(void *arg);

/* Renderer */
void renderer_init(void);
void renderer_clear(void);
void renderer_flush(bool partial);

/* Widgets */
void widgets_draw_speed(uint16_t speed_kmh);
void widgets_draw_rpm_bar(uint16_t rpm, uint16_t redline);
void widgets_draw_gear(uint8_t gear);
void widgets_draw_altitude(float altitude_m);
void widgets_draw_nav_arrow(uint16_t direction_deg, uint32_t distance_m);

#endif /* _DISPLAY_H_ */
