#ifndef _DIAGNOSTICS_H_
#define _DIAGNOSTICS_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * diagnostics.h — Diagnose: DTC (Diagnostic Trouble Codes) und Self-Test
 *
 * DTC-System: Fehler werden als Codes gespeichert (aehnlich OBD-II).
 * Self-Test: Beim Boot werden alle Module geprueft.
 */

/* DTC-Codes */
typedef enum {
    DTC_NONE = 0,
    DTC_GPS_NO_FIX,          /* GPS hat keinen Fix */
    DTC_GPS_UART_ERROR,      /* GPS UART Fehler */
    DTC_IMU_NOT_RESPONDING,  /* MPU6050 nicht am I2C-Bus */
    DTC_BARO_NOT_RESPONDING, /* BMP280 nicht am I2C-Bus */
    DTC_SD_MOUNT_FAILED,     /* SD-Karte nicht gemountet */
    DTC_SD_WRITE_FAILED,     /* SD-Schreibfehler */
    DTC_BATTERY_LOW,         /* Akku niedrig */
    DTC_BATTERY_CRITICAL,    /* Akku kritisch */
    DTC_BLE_INIT_FAILED,     /* BLE-Stack nicht initialisiert */
    DTC_DISPLAY_INIT_FAILED, /* Display-Treiber Fehler */
    DTC_SPI_BUS_ERROR,       /* SPI-Bus Fehler */
    DTC_OVERTEMPERATURE,     /* ESP32 zu heiss */
} dtc_code_t;

typedef struct {
    dtc_code_t code;
    uint32_t timestamp_ms;
    uint8_t count;           /* Wie oft aufgetreten */
    bool active;             /* Noch aktiv? */
} dtc_entry_t;

#define DTC_MAX_ENTRIES  16

/* API */
void diagnostics_init(void);

/* DTC */
void dtc_report(dtc_code_t code);
void dtc_clear(dtc_code_t code);
void dtc_clear_all(void);
uint8_t dtc_get_active(dtc_entry_t *entries, uint8_t max_entries);

/* Self-Test */
bool selftest_run(void);

#endif /* _DIAGNOSTICS_H_ */
