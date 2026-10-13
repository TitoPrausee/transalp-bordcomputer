#ifndef _DIAGNOSTICS_H_
#define _DIAGNOSTICS_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * diagnostics.h — Diagnose: DTC, Self-Test, Serial Console
 *
 * DTC-System: Fehler werden als Codes gespeichert (aehnlich OBD-II).
 *             Codes ueberleben Reboot (NVS-Persistenz).
 * Self-Test:  Beim Boot werden alle Module geprueft.
 * Console:    USB-Serial Befehlskonsole fuer Diagnose.
 */

/* ============================================================
 * Severity
 * ============================================================ */

typedef enum {
    DTC_SEV_INFO     = 0,
    DTC_SEV_WARN     = 1,
    DTC_SEV_ERROR    = 2,
    DTC_SEV_CRITICAL = 3,
} dtc_severity_t;

/* ============================================================
 * DTC Status
 * ============================================================ */

typedef enum {
    DTC_STATUS_ACTIVE  = 0,
    DTC_STATUS_HISTORY = 1,
    DTC_STATUS_PENDING = 2,
} dtc_status_t;

/* ============================================================
 * DTC-Codes (4-stellig mit Praefix, aehnlich OBD-II)
 *   P = Powertrain, S = Sensoren, C = Communication
 *   B = Body, D = Diagnose-System
 * ============================================================ */

typedef enum {
    DTC_NONE = 0,

    /* --- Powertrain P0xxx --- */
    DTC_P0101,  /* RPM-Signal unplausibel */
    DTC_P0102,  /* RPM-Signal ausgefallen */
    DTC_P0103,  /* RPM-Signal noisy */
    DTC_P0201,  /* Zuendimpuls-Signalpegel niedrig */
    DTC_P0202,  /* Zuendimpuls-Signal ausgefallen */
    DTC_P0301,  /* Gang-Erkennung unplausibel */
    DTC_P0401,  /* Bordspannung unter 11.5V */
    DTC_P0402,  /* Bordspannung unter 10.5V (kritisch) */
    DTC_P0403,  /* Bordspannung ueber 15.0V */
    DTC_P0404,  /* Bordspannung normalisiert */

    /* --- Sensoren S0xxx --- */
    DTC_S0101,  /* Kuehlmitteltemp-Sensor unplausibel */
    DTC_S0102,  /* Kuehlmitteltemp > 110C (Warnung) */
    DTC_S0103,  /* Kuehlmitteltemp > 120C (Kritisch) */
    DTC_S0201,  /* Kraftstoffstand-Sensor unplausibel */
    DTC_S0202,  /* Kraftstoffstand < 10% (Reserve) */
    DTC_S0301,  /* Oeldruck-Schalter unplausibel */
    DTC_S0302,  /* Oeldruck niedrig bei laufendem Motor */
    DTC_S0401,  /* IMU nicht am I2C-Bus gefunden */
    DTC_S0402,  /* IMU-Daten unplausibel */
    DTC_S0501,  /* GPS-Modul antwortet nicht */
    DTC_S0502,  /* GPS-Fix verloren */
    DTC_S0503,  /* GPS-Fix wiederhergestellt */
    DTC_S0601,  /* Barometer nicht am I2C-Bus */
    DTC_S0602,  /* Barometer-Daten unplausibel */

    /* --- Communication C0xxx --- */
    DTC_C0101,  /* SD-Karte nicht mountbar */
    DTC_C0102,  /* SD-Karte fast voll */
    DTC_C0103,  /* SD-Karte voll */
    DTC_C0104,  /* SD-Schreibfehler */
    DTC_C0201,  /* BLE-Advertising konnte nicht starten */
    DTC_C0202,  /* BLE-Verbindung getrennt */
    DTC_C0203,  /* BLE-Stack-Fehler */
    DTC_C0301,  /* I2C-Bus-Fehler */
    DTC_C0302,  /* I2C-Geraet antwortet nicht */
    DTC_C0401,  /* SPI-Bus-Fehler (Display) */
    DTC_C0402,  /* UART-Fehler (GPS) */

    /* --- Body B0xxx --- */
    DTC_B0101,  /* Display-Controller nicht erreichbar */
    DTC_B0102,  /* Display-Refresh fehlgeschlagen */
    DTC_B0301,  /* Hintergrundbeleuchtung PWM-Fehler */
    DTC_B0401,  /* Taster 1 entprellt ungueltig */
    DTC_B0402,  /* Taster 2 entprellt ungueltig */

    /* --- Diagnose-System D0xxx --- */
    DTC_D0101,  /* NVS-Schreibfehler */
    DTC_D0102,  /* NVS fast voll */
    DTC_D0103,  /* RTC-Zeit nicht gesetzt */
    DTC_D0201,  /* PSRAM-Test fehlgeschlagen */
    DTC_D0202,  /* Heap-Fragmentierung hoch */
    DTC_D0301,  /* OTA-Partition ungueltig */
    DTC_D0302,  /* OTA-Rollback aktiviert */
    DTC_D0401,  /* Self-Test fehlgeschlagen */
    DTC_D0402,  /* Self-Test bestanden */
    DTC_D0501,  /* Watchdog-Reset erkannt */
    DTC_D0502,  /* Brownout-Detector ausgeloest */
    DTC_D0503,  /* Akku altert */
    DTC_D0601,  /* Kalibrierungs-Daten ungueltig */

    DTC_CODE_MAX,  /* Sentinel */
} dtc_code_t;

/* ============================================================
 * Freeze-Frame: Sensor-Snapshot bei DTC-Ausloesung
 * ============================================================ */

typedef struct {
    float battery_v;
    float coolant_temp_c;
    float ambient_temp_c;
    float fuel_percent;
    float rpm;
    float speed_kmh;
    float roll_deg;
    float pitch_deg;
    float altitude_m;
    uint8_t gps_sats;
    bool   gps_fix;
    bool   ble_connected;
    bool   sd_mounted;
} dtc_freeze_frame_t;

/* ============================================================
 * DTC Entry
 * ============================================================ */

typedef struct {
    dtc_code_t         code;
    dtc_severity_t     severity;
    dtc_status_t       status;
    uint32_t           first_occurrence_ms;
    uint32_t           last_occurrence_ms;
    uint16_t           count;
    dtc_freeze_frame_t freeze_frame;
} dtc_entry_t;

#define DTC_MAX_ENTRIES  32

/* ============================================================
 * Self-Test Result
 * ============================================================ */

typedef enum {
    ST_PASS = 0,
    ST_FAIL = 1,
    ST_SKIP = 2,
} selftest_status_t;

typedef struct {
    dtc_code_t        code;
    selftest_status_t status;
    const char       *name;
} selftest_item_t;

#define SELFTEST_MAX_ITEMS  8

typedef struct {
    selftest_item_t items[SELFTEST_MAX_ITEMS];
    uint8_t         count;
    bool            all_pass;
    uint32_t        timestamp_ms;
    uint32_t        duration_ms;
} selftest_result_t;

/* ============================================================
 * DTC API
 * ============================================================ */

void dtc_init(void);
void dtc_register(dtc_code_t code, dtc_severity_t sev);
void dtc_report(dtc_code_t code);
void dtc_clear(dtc_code_t code);
void dtc_clear_all(void);
uint8_t dtc_get_active(dtc_entry_t *out, int max_count);
uint8_t dtc_get_history(dtc_entry_t *out, int max_count);
uint8_t dtc_get_all(dtc_entry_t *out, int max_count);
const char *dtc_code_to_string(dtc_code_t code);
const char *dtc_severity_to_string(dtc_severity_t sev);
const char *dtc_status_to_string(dtc_status_t status);
dtc_severity_t dtc_default_severity(dtc_code_t code);

/* ============================================================
 * Self-Test API
 * ============================================================ */

bool selftest_run(void);
bool selftest_run_full(void);
bool selftest_run_quick(void);
const selftest_result_t *selftest_get_last_result(void);

/* ============================================================
 * Diagnostics Coordinator API
 * ============================================================ */

void diagnostics_init(void);
void diag_init(void);
void diag_console_init(void);
void diag_console_task(void *arg);

#endif /* _DIAGNOSTICS_H_ */
