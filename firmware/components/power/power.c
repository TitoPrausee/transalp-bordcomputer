/*
 * power.c — Bordspannung-Messung & Batterie-Monitoring
 *
 * Hardware:
 *   Bordspannung: GPIO1 (ADC1_CH0), Spannungsteiler 10:1 (100k/10k).
 *   ADC-Referenz: 3.3V (HT7333 LDO, 30 uVrms Rauschen).
 *   ADC-Attenuation: 12 dB (Range 0-2900 mV).
 *
 * Batterie: 12V Bleiakku (PD06 Bordnetz).
 *   Ruhespannung: 11.8V (0%) bis 12.6V (100%).
 *   Voll geladen (Generator): 14.4V.
 *   Kritisch: < 11.5V.  Ueberspannung: > 15.0V.
 *
 * Filter: Moving-Average (N=16) fuer stabile Werte.
 * Messung: Alle 500ms.
 *
 * ESP-IDF v5.x: adc_oneshot + adc_cali (linepoint scheme).
 */

#include "power.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_cali.h"

static const char *TAG = "power";

/* ============================================================
 * ADC-Handles (v5.x oneshot API)
 * ============================================================ */

static adc_oneshot_unit_handle_t s_adc1_handle   = NULL;
static adc_cali_handle_t         s_adc_cali_handle = NULL;

/* ============================================================
 * Moving-Average Filter
 * ============================================================ */

#define FILTER_MAX_SIZE  32

static uint16_t s_filter_buf[FILTER_MAX_SIZE];
static uint8_t  s_filter_idx     = 0;
static uint8_t  s_filter_count   = 0;
static uint8_t  s_filter_size    = 16;

/* ============================================================
 * Mess-Daten
 * ============================================================ */

static float           s_voltage_filtered  = 12.6f;
static float           s_voltage_raw       = 12.6f;
static battery_status_t s_battery_status   = BATTERY_STATUS_NORMAL;
static bool            s_generator_running = false;
static uint8_t         s_battery_pct       = 100;

/* Batterie-Health: Zyklenzaehler */
static uint16_t s_charge_cycles    = 0;
static uint16_t s_discharge_cycles = 0;

/* Tracking fuer Zyklenerkennung */
static bool  s_was_charging       = false;
static float s_last_voltage_for_cycle = 12.6f;

/* ============================================================
 * Kalibrierung: ADC-Rohwert -> Volt
 *
 * ADC-Rohwert (0-4095) -> kalibrierte Spannung (mV) -> Bordspannung (V)
 *   vbat = adc_voltage_mv * BATT_VOLTAGE_DIV / 1000.0
 *
 * Der Spannungsteiler ist 10:1 (100k/10k, 1% Toleranz).
 * Bei 15V Bordnetz: 1.5V am ADC (im 12dB-Bereich 0-2900 mV).
 * ============================================================ */

/* ============================================================
 * Interne Hilfsfunktionen
 * ============================================================ */

/*
 * Moving-Average Filter initialisieren
 */
static void filter_init(uint8_t size)
{
    if (size == 0 || size > FILTER_MAX_SIZE) {
        size = 16;
    }
    s_filter_size  = size;
    s_filter_idx    = 0;
    s_filter_count  = 0;
    for (int i = 0; i < FILTER_MAX_SIZE; i++) {
        s_filter_buf[i] = 0;
    }
}

/*
 * Neuen Wert in den Moving-Average einringen
 */
static float filter_push(uint16_t raw_adc_mv)
{
    s_filter_buf[s_filter_idx] = raw_adc_mv;
    s_filter_idx = (s_filter_idx + 1) % s_filter_size;
    if (s_filter_count < s_filter_size) {
        s_filter_count++;
    }

    uint32_t sum = 0;
    for (uint8_t i = 0; i < s_filter_count; i++) {
        sum += s_filter_buf[i];
    }
    return (float)sum / s_filter_count;
}

/*
 * Batterie-Prozent aus Spannung berechnen (12V Bleiakku).
 *
 * Bleiakku-Entladekurve (naeherungsweise linear im Bereich 11.8-12.6V):
 *   11.8V = 0%
 *   12.0V = ~25%
 *   12.2V = ~50%
 *   12.4V = ~75%
 *   12.6V = 100%
 *   >13.8V = 100% (Generator laedt)
 *   14.4V = voll geladen
 */
static uint8_t voltage_to_percent(float voltage)
{
    if (voltage >= 12.6f) return 100;
    if (voltage <= 11.8f) return 0;

    /* Lineare Interpolation zwischen 11.8V und 12.6V */
    float pct = (voltage - 11.8f) / (12.6f - 11.8f) * 100.0f;
    if (pct < 0.0f)   pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    return (uint8_t)pct;
}

/*
 * Batterie-Status aus Spannung ableiten
 */
static battery_status_t evaluate_battery_status(float voltage, float vwarn_low,
                                                float vwarn_crit, float vwarn_over)
{
    if (voltage <= vwarn_crit)     return BATTERY_STATUS_CRITICAL;
    if (voltage >= vwarn_over)     return BATTERY_STATUS_OVERVOLT;
    if (voltage < vwarn_low)       return BATTERY_STATUS_LOW;
    if (voltage > 13.8f)           return BATTERY_STATUS_CHARGING;
    return BATTERY_STATUS_NORMAL;
}

/*
 * Lade-/Entladezyklen zaehlen.
 *
 * Ein Zyklus wird erkannt, wenn die Spannung von <13.8V (Entladen)
 * auf >13.8V (Laden) wechselt (Ladezyklus) bzw. umgekehrt
 * (Entladezyklus).
 */
static void update_cycle_count(float voltage)
{
    bool is_charging = (voltage > 13.8f);

    if (is_charging && !s_was_charging) {
        /* Uebergang: Entladen -> Laden */
        s_charge_cycles++;
        ESP_LOGI(TAG, "Charge cycle detected (total: %u)", (unsigned)s_charge_cycles);
    } else if (!is_charging && s_was_charging) {
        /* Uebergang: Laden -> Entladen */
        s_discharge_cycles++;
        ESP_LOGI(TAG, "Discharge cycle detected (total: %u)", (unsigned)s_discharge_cycles);
    }
    s_was_charging = is_charging;
    s_last_voltage_for_cycle = voltage;
}

/* ============================================================
 * ADC-Initialisierung (ESP-IDF v5.x adc_oneshot API)
 * ============================================================ */

static esp_err_t adc_init(void);

/*
 * Oeffentliche Init-Funktion: Initialisiert ADC und Moving-Average Filter.
 * Wird von power_manager.c aufgerufen.
 */
void power_adc_init(uint8_t filter_size)
{
    filter_init(filter_size);
    adc_init();
}

static esp_err_t adc_init(void)
{
    esp_err_t ret;

    /* --- ADC1 Oneshot Unit --- */
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ret = adc_oneshot_new_unit(&init_config, &s_adc1_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* --- Channel konfigurieren --- */
    adc_oneshot_chan_cfg_t chan_config = {
        .atten    = ADC_ATTEN_DB_12,    /* 0-2900 mV */
        .bitwidth = ADC_BITWIDTH_12,   /* 12-bit (0-4095) */
    };
    ret = adc_oneshot_config_channel(s_adc1_handle, BATT_ADC_CHANNEL, &chan_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_channel failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* --- Kalibrierung (linepoint scheme) --- */
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_curve_cfg = {
        .unit_id  = ADC_UNIT_1,
        .chan     = BATT_ADC_CHANNEL,
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_curve_cfg, &s_adc_cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_line_cfg = {
        .unit_id  = ADC_UNIT_1,
        .chan     = BATT_ADC_CHANNEL,
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_line_cfg, &s_adc_cali_handle);
#else
    /* Fallback: keine Kalibrierung verfuegbar */
    s_adc_cali_handle = NULL;
    ret = ESP_OK;
    ESP_LOGW(TAG, "No ADC calibration scheme available, using raw values");
#endif
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC calibration failed: %s (continuing with raw)", esp_err_to_name(ret));
        s_adc_cali_handle = NULL;
    }

    ESP_LOGI(TAG, "ADC initialized: ADC1_CH%d, 12dB atten, %s calibration",
             BATT_ADC_CHANNEL,
             s_adc_cali_handle ? "with" : "without");
    return ESP_OK;
}

/*
 * Eine ADC-Messung durchfuehren und Bordspannung berechnen.
 *
 * Liest den ADC mehrfach (Oversampling) und mittelt fuer Rauschminderung.
 * Dann Kalibrierung anwenden und Spannungsteiler umrechnen.
 */
static float adc_read_voltage(void)
{
    int adc_raw;
    int adc_voltage_mv = 0;
    esp_err_t ret;

    /* Oversampling: 4x messen und mitteln */
    uint32_t raw_sum = 0;
    int valid_reads = 0;

    for (int i = 0; i < 4; i++) {
        ret = adc_oneshot_read(s_adc1_handle, BATT_ADC_CHANNEL, &adc_raw);
        if (ret == ESP_OK) {
            raw_sum += (uint32_t)adc_raw;
            valid_reads++;
        } else {
            ESP_LOGW(TAG, "ADC read failed (attempt %d): %s", i, esp_err_to_name(ret));
        }
    }

    if (valid_reads == 0) {
        ESP_LOGE(TAG, "All ADC reads failed, returning last known voltage");
        return s_voltage_filtered;
    }

    uint16_t avg_raw = (uint16_t)(raw_sum / valid_reads);

    /* Kalibrierung: Rohwert -> mV */
    if (s_adc_cali_handle != NULL) {
        ret = adc_cali_raw_to_voltage(s_adc_cali_handle, avg_raw, &adc_voltage_mv);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "adc_cali_raw_to_voltage failed, using raw formula");
            /* Fallback: 12-bit, 12dB atten = 0-2900 mV */
            adc_voltage_mv = (int)((uint32_t)avg_raw * 2900 / 4095);
        }
    } else {
        /* Ohne Kalibrierung: direkte Umrechnung */
        adc_voltage_mv = (int)((uint32_t)avg_raw * 2900 / 4095);
    }

    /* Rohspannung am ADC-Pin (mit Spannungsteiler umrechnen) */
    float adc_voltage_v = (float)adc_voltage_mv / 1000.0f;
    s_voltage_raw = adc_voltage_v * BATT_VOLTAGE_DIV;

    /* Gefilterte Bordspannung ueber Moving-Average */
    s_voltage_filtered = filter_push((uint16_t)adc_voltage_mv) / 1000.0f
                         * BATT_VOLTAGE_DIV;

    return s_voltage_filtered;
}

/* ============================================================
 * Oeffentliche API: Bordspannung & Batterie
 * ============================================================ */

float power_get_voltage(void)
{
    return s_voltage_filtered;
}

float power_get_raw_voltage(void)
{
    return s_voltage_raw;
}

float power_get_battery_voltage(void)
{
    return power_get_voltage();
}

uint8_t power_get_battery_pct(void)
{
    return s_battery_pct;
}

uint8_t power_get_battery_percent(void)
{
    return power_get_battery_pct();
}

battery_status_t power_get_battery_status(void)
{
    return s_battery_status;
}

bool power_is_generator_running(void)
{
    return s_generator_running;
}

uint16_t power_get_charge_cycles(void)
{
    return s_charge_cycles;
}

uint16_t power_get_discharge_cycles(void)
{
    return s_discharge_cycles;
}

/*
 * Diese Funktion wird von power_manager.c (dem power_task) aufgerufen,
 * um eine Messung durchzufuehren und alle abgeleiteten Werte zu aktualisieren.
 *
 * Gibt den neuen Batterie-Status zurueck, damit der Manager auf Aenderungen
 * reagieren kann (z.B. Deep-Sleep bei kritischer Spannung).
 */
battery_status_t power_measure_and_update(float vwarn_low, float vwarn_crit,
                                           float vwarn_over, float vgen_thresh)
{
    float voltage = adc_read_voltage();

    /* Batterie-Status evaluieren */
    battery_status_t new_status = evaluate_battery_status(
        voltage, vwarn_low, vwarn_crit, vwarn_over);

    /* Generator-Status */
    bool gen_running = (voltage > vgen_thresh);

    /* Zyklenzaehler aktualisieren */
    update_cycle_count(voltage);

    /* Batterie-Prozent */
    s_battery_pct = voltage_to_percent(voltage);

    battery_status_t old_status = s_battery_status;
    s_battery_status   = new_status;
    s_generator_running = gen_running;

    /* Status-Aenderungen loggen */
    if (new_status != old_status) {
        static const char *status_str[] = {
            "NORMAL", "CHARGING", "LOW", "CRITICAL", "OVERVOLT"
        };
        ESP_LOGI(TAG, "Battery status: %s -> %s (%.2fV, %u%%)",
                 status_str[old_status], status_str[new_status],
                 voltage, (unsigned)s_battery_pct);
    }

    if (gen_running != s_generator_running) {
        ESP_LOGI(TAG, "Generator %s (%.2fV)",
                 gen_running ? "ON" : "OFF", voltage);
    }

    return new_status;
}

/* ============================================================
 * ADC-Deinitialisierung
 * ============================================================ */

void power_adc_deinit(void)
{
    if (s_adc_cali_handle != NULL) {
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        adc_cali_delete_scheme_line_fitting(s_adc_cali_handle);
#endif
    }
    s_adc_cali_handle = NULL;
    if (s_adc1_handle != NULL) {
        adc_oneshot_del_unit(s_adc1_handle);
        s_adc1_handle = NULL;
    }
    ESP_LOGI(TAG, "ADC deinitialized");
}
