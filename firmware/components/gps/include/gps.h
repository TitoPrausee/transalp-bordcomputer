#ifndef _GPS_H_
#define _GPS_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/*
 * gps.h — GPS-Modul: NEO-M9N ueber UART, NMEA-Parsing
 *
 * GPS-Task laeuft auf Core 1 mit 10Hz Update-Rate.
 * UART: RX=GPIO44, TX=GPIO43, Baud=38400 (Default).
 */

typedef enum {
    GPS_FIX_NONE = 0,
    GPS_FIX_2D,
    GPS_FIX_3D,
    GPS_FIX_DGPS,
} gps_fix_quality_t;

typedef struct {
    float lat;               /* Breitengrad (dezimal) */
    float lon;               /* Laengengrad (dezimal) */
    float speed_kmh;         /* Geschwindigkeit (km/h) */
    float course_deg;        /* Kurs (Grad) */
    float altitude_m;        /* Hoehe ueber NN (m) */
    uint8_t satellites;      /* Anzahl Satelliten */
    float hdop;              /* Horizontal Dilution of Precision */
    gps_fix_quality_t fix_quality;
    bool valid;              /* true wenn Fix+HDOP<5 */
    uint32_t timestamp_ms;
} gps_data_t;

/* API */
void gps_init(void);
void gps_task(void *arg);

/* Queue: neueste GPS-Daten (andere Tasks lesen hieraus) */
extern QueueHandle_t gps_data_queue;

/* NMEA-Parser (in nmea_parser.c) */
void nmea_parse_line(const char *line, gps_data_t *data);

#endif /* _GPS_H_ */
