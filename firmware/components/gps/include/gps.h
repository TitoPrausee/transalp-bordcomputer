#ifndef _GPS_H_
#define _GPS_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

/*
 * gps.h — GPS-Modul: GNSS-Empfaenger ueber UART, NMEA-Parsing
 *
 * GPS-Task laeuft auf Core 1 und liest NMEA-Saetze vom UART.
 * UART: RX=GPIO44, TX=GPIO43, Baud=38400 (Default).
 * Positionsdaten sind thread-safe ueber Mutex verfuegbar.
 */

/* Fix-Qualitaet (aus GGA sentence) */
typedef enum {
    GPS_FIX_NONE = 0,
    GPS_FIX_2D,       /* SPS fix, no DGPS */
    GPS_FIX_3D,       /* SPS fix */
    GPS_FIX_DGPS,     /* Differential GPS */
} gps_fix_quality_t;

/*
 * GPS-Datenstruktur.
 * Alle Winkel in Dezimalgrad, Geschwindigkeit in km/h.
 * Datum/Uhrzeit aus RMC-Satz (UTC).
 */
typedef struct {
    bool has_fix;               /* true wenn ein gueltiger Fix vorliegt */
    float lat;                  /* Breitengrad (dezimal) */
    float lon;                  /* Laengengrad (dezimal) */
    float altitude_m;           /* Hoehe ueber NN (m) */
    float speed_kmh;            /* Geschwindigkeit (km/h) */
    float course_deg;           /* Kurs (Grad true) */
    uint8_t satellites;         /* Anzahl Satelliten im Fix */
    float hdop;                 /* Horizontal Dilution of Precision */
    gps_fix_quality_t fix_quality;
    bool valid;                 /* true wenn Fix+HDOP<5 */

    /* Datum und Uhrzeit (UTC, aus RMC-Satz) */
    uint16_t year;              /* z.B. 2026 */
    uint8_t month;              /* 1..12 */
    uint8_t day;                /* 1..31 */
    uint8_t hour;               /* 0..23 */
    uint8_t minute;             /* 0..59 */
    uint8_t second;             /* 0..59 */

    uint32_t timestamp_ms;      /* FreeRTOS-Tick bei letztem Update */
} gps_data_t;

/*
 * NMEA-Parser-Statistik.
 * Wird bei jedem Parse-Aufruf aktualisiert.
 */
typedef struct {
    uint32_t total;             /* Alle Saetze (inkl. ungueltig) */
    uint32_t valid;             /* Checksumme ok + bekannter Satz */
    uint32_t checksum_failed;    /* XOR-Checksumme falsch */
    uint32_t malformed;         /* Satzstruktur fehlerhaft */
    uint32_t gga;               /* GGA-Saetze geparst */
    uint32_t rmc;               /* RMC-Saetze geparst */
    uint32_t vtg;               /* VTG-Saetze geparst */
    uint32_t gsv;               /* GSV-Saetze geparst */
} nmea_stats_t;

/* API */

/* UART-Setup, Mutex/Queue erstellen. Einmal in app_main aufrufen. */
void gps_init(void);

/* GPS-Task — liest UART, parst NMEA, aktualisiert Daten. */
void gps_task(void *arg);

/*
 * Thread-safe Lesen der aktuellen GPS-Daten.
 * Kopiert die Werte unter Mutex in *out.
 * Rueckgabe: true wenn has_fix, false sonst.
 */
bool gps_get_data(gps_data_t *out);

/* Parser-Statistik abfragen (Kopie). */
void gps_get_stats(nmea_stats_t *out);

/* NMEA-Parser (in nmea_parser.c) */

/*
 * Einen NMEA-Satz parsen und Felder in *out aktualisieren.
 * Nur Felder, die im Satz enthalten sind, werden geschrieben;
 * bestehende Werte in *out bleiben unveraendert fuer fehlende Felder.
 *
 * sentence:  NMEA-Satz inkl. $ und *CC (z.B. "$GPGGA,...,*47")
 * out:       Ziel-Struktur (wird teilweise aktualisiert)
 * Rueckgabe: true wenn Satz gueltig und bekannt, false sonst
 */
bool nmea_parse(const char *sentence, gps_data_t *out);

/* Queue: neueste GPS-Daten (andere Tasks lesen hieraus). */
extern QueueHandle_t gps_data_queue;

#endif /* _GPS_H_ */
