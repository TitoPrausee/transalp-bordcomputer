#ifndef _POWER_H_
#define _POWER_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"

/*
 * power.h — Power-Management: Bordspannungs-Ueberwachung,
 *           Batterie-Monitoring, Power-States und Deep-Sleep.
 *
 * Hardware:
 *   Bordspannung: GPIO1 (ADC1_CH0), Spannungsteiler 10:1 (100k/10k).
 *   Backlight:    GPIO18 (TFT_BL), LEDC PWM.
 *   3.3V Referenzspannung fuer ADC (HT7333 LDO, 30 uVrms Rauschen).
 *
 * Batterie: 12V Bleiakku (PD06 Bordnetz).
 *   11.8V = 0%, 12.6V = 100%, 14.4V = voll geladen (Lima).
 *
 * Task laeuft auf Core 0 mit 500ms Messintervall, hoechste Prioritaet (6).
 */

/* ============================================================
 * Power-States
 * ============================================================ */

typedef enum {
    POWER_STATE_ACTIVE     = 0,   /* Vollbetrieb: Display an, alle Peripherie */
    POWER_STATE_DIMMED     = 1,   /* Display gedimmt, Sensoren/GPS aktiv */
    POWER_STATE_SUSPENDED  = 2,   /* Display aus, nur GPS+Logger, BLE Advertising */
    POWER_STATE_DEEP_SLEEP = 3,   /* RTC only, Wake via GPIO/Timer */
} power_state_t;

/* Rueckwaertskompatibilitaet mit altem Interface */
#define POWER_STATE_IDLE     POWER_STATE_DIMMED
#define POWER_STATE_SLEEP    POWER_STATE_SUSPENDED
#define POWER_STATE_SHUTDOWN POWER_STATE_DEEP_SLEEP

/* ============================================================
 * Batterie-Status
 * ============================================================ */

typedef enum {
    BATTERY_STATUS_NORMAL    = 0,
    BATTERY_STATUS_CHARGING  = 1,   /* Bordspannung > 13.8V (Generator) */
    BATTERY_STATUS_LOW       = 2,   /* < 12.0V */
    BATTERY_STATUS_CRITICAL = 3,    /* < 11.5V */
    BATTERY_STATUS_OVERVOLT  = 4,   /* > 15.0V */
} battery_status_t;

/* ============================================================
 * Konfiguration
 * ============================================================ */

typedef struct {
    /* Inaktivitaets-Timeouts (Sekunden) */
    uint32_t dim_timeout_s;          /* ACTIVE -> DIMMED (default 30) */
    uint32_t suspend_timeout_s;      /* DIMMED -> SUSPENDED (default 120) */
    uint32_t deep_sleep_timeout_s;   /* SUSPENDED -> DEEP_SLEEP (default 600) */

    /* Batterie-Warnschwellen (Volt) */
    float    vwarn_low;              /* Warnung niedrig (default 12.0) */
    float    vwarn_critical;         /* Kritisch, Deep-Sleep (default 11.5) */
    float    vwarn_overvoltage;     /* Ueberspannung (default 15.0) */
    float    vgen_threshold;         /* Generator-Schwelle (default 13.8) */

    /* Backlight */
    uint8_t  backlight_active;       /* Prozent im ACTIVE-State (default 100) */
    uint8_t  backlight_dimmed;       /* Prozent im DIMMED-State (default 15) */

    /* Messintervall */
    uint32_t measure_interval_ms;   /* ADC Messintervall (default 500) */

    /* Moving-Average Filter */
    uint8_t  filter_size;            /* N fuer Moving-Average (default 16) */
} power_config_t;

/* ============================================================
 * Callbacks fuer State-Changes
 * ============================================================ */

typedef enum {
    POWER_EVENT_STATE_CHANGED   = 0,   /* Neuer Power-State aktiv */
    POWER_EVENT_BATTERY_WARNING = 1,   /* Batterie-Status geaendert */
    POWER_EVENT_VOLTAGE_UPDATE  = 2,   /* Neue Spannungsmessung */
} power_event_type_t;

typedef struct {
    power_event_type_t type;
    power_state_t      old_state;
    power_state_t      new_state;
    battery_status_t   battery_status;
    float              voltage;
    uint8_t            battery_pct;
} power_event_t;

typedef void (*power_callback_t)(const power_event_t *event);

/* ============================================================
 * Power-Event-Group Bits (fuer Event-Bus Integration)
 * ============================================================ */

#define PEVT_STATE_CHANGED     (1 << 0)
#define PEVT_BATTERY_LOW       (1 << 1)
#define PEVT_BATTERY_CRITICAL  (1 << 2)
#define PEVT_BATTERY_OVERVOLT  (1 << 3)
#define PEVT_GENERATOR_ON      (1 << 4)
#define PEVT_GENERATOR_OFF     (1 << 5)
#define PEVT_ENTER_SLEEP       (1 << 6)
#define PEVT_WAKEUP            (1 << 7)
#define PEVT_VOLTAGE_UPDATE    (1 << 8)

/* ============================================================
 * API
 * ============================================================ */

/* Initialisierung mit Default-Konfiguration */
void power_init(void);

/* Initialisierung mit benutzerdefinierter Konfiguration */
void power_init_with_config(const power_config_t *cfg);

/* Konfiguration zur Laufzeit aendern */
void power_set_config(const power_config_t *cfg);
void power_get_config(power_config_t *cfg);

/* --- Bordspannung / Batterie --- */

/* Gefilterte Bordspannung in Volt */
float power_get_voltage(void);

/* Batterie-Ladezustand in Prozent (0-100) */
uint8_t power_get_battery_pct(void);

/* Roh-Spannung (ungefiltert, letzter ADC-Wert) */
float power_get_raw_voltage(void);

/* Batterie-Status */
battery_status_t power_get_battery_status(void);

/* Generator laeuft? (Bordspannung > Schwelle) */
bool power_is_generator_running(void);

/* Batterie-Health: Lade-/Entladezyklen */
uint16_t power_get_charge_cycles(void);
uint16_t power_get_discharge_cycles(void);

/* Stromverbrauch-Schaetzung fuer aktuellen State (mA) */
uint16_t power_get_estimated_current_ma(void);

/* --- Power-States --- */

/* Aktuellen State abfragen */
power_state_t power_get_state(void);

/* State manuell setzen (fuer Button/Sensor-Events) */
esp_err_t power_set_state(power_state_t new_state);

/* Inaktivitaets-Timer zuruecksetzen (bei Tastendruck/Sensor-Event) */
void power_reset_inactivity(void);

/* --- Backlight --- */

/* Backlight-Helligkeit setzen (0-100%) */
void power_set_backlight(uint8_t percent);

/* Aktuelle Backlight-Helligkeit abfragen */
uint8_t power_get_backlight(void);

/* --- Peripherie-Power-Gating --- */

/* GPS-Power-Gating (true = an, false = aus) */
void power_set_gps_enabled(bool enabled);
bool power_is_gps_enabled(void);

/* BLE-Power-Gating (true = an, false = aus) */
void power_set_ble_enabled(bool enabled);
bool power_is_ble_enabled(void);

/* --- Callbacks --- */

/* Callback registrieren (max 4 gleichzeitig) */
esp_err_t power_register_callback(power_callback_t cb);
void power_unregister_callback(power_callback_t cb);

/* --- Deep-Sleep --- */

/* Deep-Sleep mit Wakeup-Quellen konfigurieren und starten */
void power_enter_deep_sleep(uint32_t wakeup_timeout_s);

/* Wakeup-Ursache nach Deep-Sleep abfragen */
typedef enum {
    WAKEUP_REASON_NONE       = 0,
    WAKEUP_REASON_RTC_TIMER  = 1,
    WAKEUP_REASON_GPIO       = 2,
    WAKEUP_REASON_UART       = 3,
    WAKEUP_REASON_TOUCH      = 4,
    WAKEUP_REASON_ULP        = 5,
    WAKEUP_REASON_BOOT       = 6,
} wakeup_reason_t;

wakeup_reason_t power_get_wakeup_reason(void);

/* --- Event-Group --- */

/* Eigene Event-Group fuer Power-Events */
extern EventGroupHandle_t g_power_event_group;

/* --- Task --- */

void power_task(void *arg);

/* --- Rueckwaertskompatibilitaet --- */

float power_get_battery_voltage(void);   /* = power_get_voltage() */
uint8_t power_get_battery_percent(void); /* = power_get_battery_pct() */

#endif /* _POWER_H_ */
