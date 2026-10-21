/*
 * power_manager.c — Power-States, Deep-Sleep, Backlight & Power-Gating
 *
 * State-Machine:
 *   ACTIVE -> DIMMED:     Nach Inaktivitaet (default 30s)
 *   DIMMED -> ACTIVE:     Bei Tastendruck oder Sensor-Event
 *   DIMMED -> SUSPENDED:  Nach doppelter Inaktivitaet (default 120s)
 *   SUSPENDED -> ACTIVE:  Bei Tastendruck, Sensor-Event, BLE
 *   SUSPENDED -> DEEP_SLEEP: Bei <11.5V oder manuell
 *   DEEP_SLEEP -> ACTIVE: RTC Timer oder Wake-Pin
 *
 * Backlight: LEDC PWM auf GPIO18 (TFT_BL), 0-100%.
 * Peripherie-Power-Gating: GPS_EN, BOOST_EN.
 * RTC-Speicher fuer Status-Erhaltung im Deep-Sleep.
 * Wake-Up-Quellen: GPIO (Taster), RTC Timer, UART (GPS).
 *
 * FreeRTOS Event-Group fuer Power-Events.
 */

#include "power.h"
#include "board_config.h"
#include "event_bus.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "driver/rtc_io.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"

static const char *TAG = "power_mgr";

/* ============================================================
 * Default-Konfiguration
 * ============================================================ */

static power_config_t s_config = {
    .dim_timeout_s        = 30,
    .suspend_timeout_s    = 120,
    .deep_sleep_timeout_s = 600,
    .vwarn_low            = 12.0f,
    .vwarn_critical       = 11.5f,
    .vwarn_overvoltage    = 15.0f,
    .vgen_threshold       = 13.8f,
    .backlight_active     = 100,
    .backlight_dimmed     = 15,
    .measure_interval_ms = 500,
    .filter_size          = 16,
};

/* ============================================================
 * State
 * ============================================================ */

static power_state_t s_state            = POWER_STATE_ACTIVE;
static power_state_t s_state_before_sleep = POWER_STATE_ACTIVE;
static uint8_t       s_backlight_pct    = 100;
static bool          s_gps_enabled      = true;
static bool          s_ble_enabled      = true;
static bool          s_initialized      = false;

/* Inaktivitaets-Tracking */
static int64_t s_last_activity_us  = 0;
static int64_t s_last_state_change = 0;

/* Callback-System (max 4 registrierbar) */
#define MAX_CALLBACKS 4
static power_callback_t s_callbacks[MAX_CALLBACKS];
static uint8_t s_callback_count = 0;

/* Event-Group fuer Power-Events */
EventGroupHandle_t g_power_event_group = NULL;

/* ============================================================
 * RTC-Speicher: Status-Erhaltung im Deep-Sleep
 * ============================================================ */

/* RTC-Slow-Memory fuer Persistenz ueber Deep-Sleep */
typedef struct {
    uint32_t   magic;
    power_state_t state_before_sleep;
    uint32_t  sleep_count;
    int64_t   sleep_enter_time;
} rtc_persist_t;

static RTC_NOINIT_ATTR rtc_persist_t s_rtc_persist;

#define RTC_PERSIST_MAGIC 0x504F5733  /* "POW3" */

/* ============================================================
 * LEDC PWM Konfiguration fuer Backlight
 *
 * Verwendet LEDC_TIMER_1 (TFT-Treiber nutzt TIMER_0),
 * GPIO18 (TFT_BL), 10-bit Aufloesung, 5 kHz.
 * ============================================================ */

#define BACKLIGHT_GPIO       18
#define BACKLIGHT_LEDC_TIMER LEDC_TIMER_1
#define BACKLIGHT_LEDC_CH    LEDC_CHANNEL_1
#define BACKLIGHT_FREQ_HZ    5000
#define BACKLIGHT_RESOLUTION LEDC_TIMER_10_BIT
#define BACKLIGHT_MAX_DUTY   1023

static void backlight_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = BACKLIGHT_LEDC_TIMER,
        .duty_resolution = BACKLIGHT_RESOLUTION,
        .freq_hz         = BACKLIGHT_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&timer_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed: %s", esp_err_to_name(ret));
        return;
    }

    ledc_channel_config_t ch_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = BACKLIGHT_LEDC_CH,
        .timer_sel  = BACKLIGHT_LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = BACKLIGHT_GPIO,
        .duty       = 0,
        .hpoint     = 0,
    };
    ret = ledc_channel_config(&ch_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel config failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Backlight PWM initialized: GPIO%d, %d Hz, %d-bit",
             BACKLIGHT_GPIO, BACKLIGHT_FREQ_HZ, 10);
}

void power_set_backlight(uint8_t percent)
{
    if (percent > 100) percent = 100;
    uint32_t duty = (uint32_t)percent * BACKLIGHT_MAX_DUTY / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_LEDC_CH, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_LEDC_CH);
    s_backlight_pct = percent;
}

uint8_t power_get_backlight(void)
{
    return s_backlight_pct;
}

/* ============================================================
 * Peripherie-Power-Gating
 *
 * GPS_POWER_EN_PIN (GPIO5): High-Side MOSFET fuer GPS.
 * BOOST_EN_PIN (GPIO6):     5V-Rail Schalten.
 * ============================================================ */

static void power_gating_init(void)
{
    /* GPIO5 als Output, GPS-Power-Enable */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPS_POWER_EN_PIN) |
                        (1ULL << BOOST_EN_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Power-gating GPIO config failed: %s", esp_err_to_name(ret));
        return;
    }

    /* Peripherie einschalten */
    gpio_set_level(GPS_POWER_EN_PIN, 1);
    gpio_set_level(BOOST_EN_PIN, 1);
    s_gps_enabled = true;

    ESP_LOGI(TAG, "Power-gating initialized: GPS_EN=GPIO%d, BOOST_EN=GPIO%d",
             GPS_POWER_EN_PIN, BOOST_EN_PIN);
}

void power_set_gps_enabled(bool enabled)
{
    gpio_set_level(GPS_POWER_EN_PIN, enabled ? 1 : 0);
    s_gps_enabled = enabled;
    ESP_LOGI(TAG, "GPS power: %s", enabled ? "ON" : "OFF");
}

bool power_is_gps_enabled(void)
{
    return s_gps_enabled;
}

void power_set_ble_enabled(bool enabled)
{
    s_ble_enabled = enabled;
    ESP_LOGI(TAG, "BLE: %s", enabled ? "enabled" : "disabled");
    /* BLE-Task muss selbst auf dieses Flag pruefen; hier nur Zustand speichern. */
}

bool power_is_ble_enabled(void)
{
    return s_ble_enabled;
}

/* ============================================================
 * Callback-System
 * ============================================================ */

static void notify_callbacks(power_event_type_t type,
                             power_state_t old_state,
                             power_state_t new_state,
                             battery_status_t batt,
                             float voltage,
                             uint8_t pct)
{
    power_event_t event = {
        .type           = type,
        .old_state      = old_state,
        .new_state      = new_state,
        .battery_status = batt,
        .voltage        = voltage,
        .battery_pct     = pct,
    };
    for (uint8_t i = 0; i < s_callback_count; i++) {
        if (s_callbacks[i] != NULL) {
            s_callbacks[i](&event);
        }
    }
}

esp_err_t power_register_callback(power_callback_t cb)
{
    if (cb == NULL) return ESP_ERR_INVALID_ARG;
    if (s_callback_count >= MAX_CALLBACKS) {
        ESP_LOGW(TAG, "Callback table full (max %d)", MAX_CALLBACKS);
        return ESP_ERR_NO_MEM;
    }
    s_callbacks[s_callback_count++] = cb;
    return ESP_OK;
}

void power_unregister_callback(power_callback_t cb)
{
    for (uint8_t i = 0; i < s_callback_count; i++) {
        if (s_callbacks[i] == cb) {
            /* Letztes Element an die freie Stelle schieben */
            s_callbacks[i] = s_callbacks[s_callback_count - 1];
            s_callbacks[s_callback_count - 1] = NULL;
            s_callback_count--;
            break;
        }
    }
}

/* ============================================================
 * State-Machine
 * ============================================================ */

/*
 * State-Transition durchfuehren.
 * Setzt Backlight, Peripherie und Event-Bits entsprechend.
 */
static void transition_state(power_state_t new_state)
{
    if (new_state == s_state) return;

    power_state_t old_state = s_state;
    s_state = new_state;
    s_last_state_change = esp_timer_get_time();

    ESP_LOGI(TAG, "State transition: %d -> %d", old_state, new_state);

    /* State-spezifische Aktionen */
    switch (new_state) {
    case POWER_STATE_ACTIVE:
        power_set_backlight(s_config.backlight_active);
        power_set_gps_enabled(true);
        power_set_ble_enabled(true);
        xEventGroupClearBits(g_power_event_group, PEVT_ENTER_SLEEP);
        xEventGroupSetBits(g_power_event_group, PEVT_WAKEUP);
        break;

    case POWER_STATE_DIMMED:
        power_set_backlight(s_config.backlight_dimmed);
        /* GPS und BLE bleiben an */
        break;

    case POWER_STATE_SUSPENDED:
        power_set_backlight(0);
        power_set_gps_enabled(false);
        /* BLE bleibt an (Advertising) */
        break;

    case POWER_STATE_DEEP_SLEEP:
        power_set_backlight(0);
        power_set_gps_enabled(false);
        power_set_ble_enabled(false);
        xEventGroupSetBits(g_power_event_group, PEVT_ENTER_SLEEP);
        break;
    }

    /* Event-Bus benachrichtigen */
    switch (new_state) {
    case POWER_STATE_ACTIVE:
        event_bus_set(0);  /* Clear nichts, nur Information */
        break;
    case POWER_STATE_DEEP_SLEEP:
        event_bus_set(EVT_SHUTDOWN);
        break;
    default:
        break;
    }

    /* Callbacks informieren */
    notify_callbacks(POWER_EVENT_STATE_CHANGED, old_state, new_state,
                     power_get_battery_status(), power_get_voltage(),
                     power_get_battery_pct());

    /* Event-Group Bit setzen */
    xEventGroupSetBits(g_power_event_group, PEVT_STATE_CHANGED);
}

/* ============================================================
 * Inaktivitaets-Tracking
 * ============================================================ */

void power_reset_inactivity(void)
{
    s_last_activity_us = esp_timer_get_time();
    /* Bei Aktivitaet zurueck in ACTIVE */
    if (s_state == POWER_STATE_DIMMED || s_state == POWER_STATE_SUSPENDED) {
        transition_state(POWER_STATE_ACTIVE);
    }
}

/*
 * Prueft, ob ein Timeout erreicht wurde.
 */
static void check_inactivity_timeouts(void)
{
    if (s_state != POWER_STATE_ACTIVE && s_state != POWER_STATE_DIMMED &&
        s_state != POWER_STATE_SUSPENDED) {
        return;
    }

    int64_t now_us = esp_timer_get_time();
    int64_t idle_us = now_us - s_last_activity_us;

    uint32_t dim_timeout_us      = (int64_t)s_config.dim_timeout_s * 1000000;
    uint32_t suspend_timeout_us  = (int64_t)s_config.suspend_timeout_s * 1000000;
    uint32_t dsleep_timeout_us   = (int64_t)s_config.deep_sleep_timeout_s * 1000000;

    switch (s_state) {
    case POWER_STATE_ACTIVE:
        if (idle_us >= dim_timeout_us) {
            ESP_LOGI(TAG, "Inactivity timeout (%lus) -> DIMMED",
                     s_config.dim_timeout_s);
            transition_state(POWER_STATE_DIMMED);
        }
        break;

    case POWER_STATE_DIMMED:
        if (idle_us >= suspend_timeout_us) {
            ESP_LOGI(TAG, "Inactivity timeout (%lus) -> SUSPENDED",
                     s_config.suspend_timeout_s);
            transition_state(POWER_STATE_SUSPENDED);
        }
        break;

    case POWER_STATE_SUSPENDED:
        if (idle_us >= dsleep_timeout_us) {
            ESP_LOGI(TAG, "Inactivity timeout (%lus) -> DEEP_SLEEP",
                     s_config.deep_sleep_timeout_s);
            transition_state(POWER_STATE_DEEP_SLEEP);
            /* Deep-Sleep wird im power_task ausgefuehrt */
        }
        break;

    default:
        break;
    }
}

/* ============================================================
 * Stromverbrauch-Schaetzung pro State
 * ============================================================ */

uint16_t power_get_estimated_current_ma(void)
{
    switch (s_state) {
    case POWER_STATE_ACTIVE:
        return 190;    /* ESP32-S3 + Display + GPS + Sensoren + BLE */
    case POWER_STATE_DIMMED:
        return 145;    /* Display gedimmt */
    case POWER_STATE_SUSPENDED:
        return 35;     /* GPS aus, BLE Advertising, Sensoren aus */
    case POWER_STATE_DEEP_SLEEP:
        return 7;      /* RTC only (bare chip) */
    default:
        return 190;
    }
}

/* ============================================================
 * Deep-Sleep & Wakeup
 * ============================================================ */

wakeup_reason_t power_get_wakeup_reason(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    switch (cause) {
    case ESP_SLEEP_WAKEUP_TIMER:
        return WAKEUP_REASON_RTC_TIMER;
    case ESP_SLEEP_WAKEUP_GPIO:
        return WAKEUP_REASON_GPIO;
    case ESP_SLEEP_WAKEUP_UART:
        return WAKEUP_REASON_UART;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        return WAKEUP_REASON_TOUCH;
    case ESP_SLEEP_WAKEUP_ULP:
        return WAKEUP_REASON_ULP;
    case ESP_SLEEP_WAKEUP_UNDEFINED:
    default:
        /* Wenn RTC-Persist-Magic gesetzt ist, waren wir im Sleep */
        if (s_rtc_persist.magic == RTC_PERSIST_MAGIC) {
            return WAKEUP_REASON_RTC_TIMER;
        }
        return WAKEUP_REASON_BOOT;
    }
}

/*
 * Wakeup-Quellen konfigurieren:
 *   - RTC Timer (periodisch, z.B. alle 60s)
 *   - GPIO (Taster: BTN_MENU=6, BTN_SELECT=7, BTN_BACK=3)
 *   - UART (GPS-Daten)
 */
static void configure_wakeup_sources(uint32_t wakeup_timeout_s)
{
    /* RTC Timer */
    if (wakeup_timeout_s > 0) {
        esp_sleep_enable_timer_wakeup((uint64_t)wakeup_timeout_s * 1000000ULL);
        ESP_LOGI(TAG, "RTC timer wakeup in %lus", wakeup_timeout_s);
    }

    /* GPIO Wakeup (Taster sind Active-LOW mit Pull-Up) */
    /* GPIO0 (BOOT_BTN), GPIO6 (BTN_MENU), GPIO7 (BTN_SELECT), GPIO3 (BTN_BACK) */
    const int wake_pins[] = {0, 3, 6, 7};
    const int num_wake_pins = sizeof(wake_pins) / sizeof(wake_pins[0]);

    for (int i = 0; i < num_wake_pins; i++) {
        if (rtc_gpio_is_valid_gpio(wake_pins[i])) {
            esp_sleep_enable_ext0_wakeup(wake_pins[i], 0); /* Low-aktiv */
            break;  /* ext0 unterstuetzt nur einen Pin */
        }
    }

    /* UART Wakeup (GPS) */
    /* UART0 (USB-CDC) oder UART1 (GPS) koennen als Wakeup dienen.
     * Bei aktivem GPS (SUSPENDED mit GPS-Logging) ist UART1 relevant.
     */
    esp_sleep_enable_uart_wakeup(GPS_UART_NUM);

    ESP_LOGI(TAG, "Wakeup sources configured: RTC timer, GPIO buttons, UART1");
}

void power_enter_deep_sleep(uint32_t wakeup_timeout_s)
{
    ESP_LOGI(TAG, "Entering deep sleep (wakeup in %lus)...", wakeup_timeout_s);

    /* RTC-Persistenz sichern */
    s_rtc_persist.magic              = RTC_PERSIST_MAGIC;
    s_rtc_persist.state_before_sleep = s_state;
    s_rtc_persist.sleep_count++;
    s_rtc_persist.sleep_enter_time   = esp_timer_get_time();

    /* Peripherie ausschalten */
    power_set_backlight(0);
    power_set_gps_enabled(false);
    power_set_ble_enabled(false);

    /* RTC-GPIO-Pulls deaktivieren (Strom sparen) */
    rtc_gpio_isolate(GPIO_NUM_5);
    rtc_gpio_isolate(GPIO_NUM_6);

    /* Wakeup-Quellen konfigurieren */
    configure_wakeup_sources(wakeup_timeout_s);

    /* Event-Bus: Shutdown signalisieren */
    event_bus_set(EVT_SHUTDOWN);
    xEventGroupSetBits(g_power_event_group, PEVT_ENTER_SLEEP);

    ESP_LOGI(TAG, "Deep sleep. Good night.");

    /* Deep-Sleep starten */
    esp_deep_sleep_start();

    /* Diese Funktion kehrt nicht zurueck.
     * Nach Wakeup startet der ESP neu (app_main).
     */
}

/* ============================================================
 * Init
 * ============================================================ */

static void rtc_persist_check(void)
{
    if (s_rtc_persist.magic == RTC_PERSIST_MAGIC) {
        ESP_LOGI(TAG, "Woken from deep sleep (sleep count: %lu, prev state: %d)",
                 (unsigned long)s_rtc_persist.sleep_count,
                 s_rtc_persist.state_before_sleep);
        s_state_before_sleep = s_rtc_persist.state_before_sleep;
        /* Magic loeschen, damit kein falscher Wakeup beim naechsten Kaltstart */
        s_rtc_persist.magic = 0;
    } else {
        s_rtc_persist.sleep_count = 0;
    }
}

void power_init(void)
{
    power_init_with_config(NULL);
}

void power_init_with_config(const power_config_t *cfg)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Power already initialized");
        return;
    }

    /* Konfiguration uebernehmen */
    if (cfg != NULL) {
        s_config = *cfg;
    }

    /* ADC und Moving-Average Filter initialisieren (in power.c) */
    extern void power_adc_init(uint8_t filter_size);
    power_adc_init(s_config.filter_size);

    /* Event-Group erstellen */
    g_power_event_group = xEventGroupCreate();
    if (g_power_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create power event group");
        return;
    }

    /* RTC-Persistenz pruefen (Wakeup?) */
    rtc_persist_check();

    /* Wakeup-Reason loggen */
    wakeup_reason_t wake_reason = power_get_wakeup_reason();
    if (wake_reason != WAKEUP_REASON_BOOT) {
        // cppcheck-suppress variableScope
        static const char *reason_str[] = {
            "NONE", "RTC_TIMER", "GPIO", "UART", "TOUCH", "ULP", "BOOT"
        };
        ESP_LOGI(TAG, "Wakeup reason: %s", reason_str[wake_reason]);
    }

    /* Backlight PWM initialisieren */
    backlight_init();

    /* Peripherie-Power-Gating initialisieren */
    power_gating_init();

    s_last_activity_us  = esp_timer_get_time();
    s_last_state_change = esp_timer_get_time();

    /* Wenn wir aus Deep-Sleep kommen, direkt in ACTIVE */
    if (wake_reason != WAKEUP_REASON_BOOT) {
        s_state = POWER_STATE_ACTIVE;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Power management initialized (state: %d)", s_state);
}

void power_set_config(const power_config_t *cfg)
{
    if (cfg != NULL) {
        s_config = *cfg;
        ESP_LOGI(TAG, "Config updated: dim=%lus, suspend=%lus, dsleep=%lus",
                 s_config.dim_timeout_s, s_config.suspend_timeout_s,
                 s_config.deep_sleep_timeout_s);
    }
}

void power_get_config(power_config_t *cfg)
{
    if (cfg != NULL) {
        *cfg = s_config;
    }
}

esp_err_t power_set_state(power_state_t new_state)
{
    if (new_state > POWER_STATE_DEEP_SLEEP) {
        return ESP_ERR_INVALID_ARG;
    }

    if (new_state == POWER_STATE_DEEP_SLEEP) {
        /* Deep-Sleep sofort ausloesen */
        transition_state(POWER_STATE_DEEP_SLEEP);
        /* power_task wird den eigentlichen Sleep ausfuehren */
        return ESP_OK;
    }

    transition_state(new_state);

    /* Bei manueller State-Aenderung Inaktivitaetstimer zuruecksetzen */
    if (new_state == POWER_STATE_ACTIVE) {
        power_reset_inactivity();
    }

    return ESP_OK;
}

power_state_t power_get_state(void)
{
    return s_state;
}

/* ============================================================
 * Power-Task
 *
 * Laeuft auf Core 0 mit hoechster Prioritaet (6).
 * Messintervall: 500ms (aus Konfiguration).
 * ============================================================ */

void power_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "Power task started (interval: %lums)",
             (unsigned long)s_config.measure_interval_ms);

    /* Erste Messung */
    extern battery_status_t power_measure_and_update(
        float vwarn_low, float vwarn_crit, float vwarn_over, float vgen_thresh);

    battery_status_t prev_status = power_measure_and_update(
        s_config.vwarn_low, s_config.vwarn_critical,
        s_config.vwarn_overvoltage, s_config.vgen_threshold);

    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&last_wake,
                        pdMS_TO_TICKS(s_config.measure_interval_ms));

        /* --- Spannung messen --- */
        battery_status_t status = power_measure_and_update(
            s_config.vwarn_low, s_config.vwarn_critical,
            s_config.vwarn_overvoltage, s_config.vgen_threshold);

        /* --- Batterie-Events verarbeiten --- */
        if (status != prev_status) {
            power_event_t event = {
                .type           = POWER_EVENT_BATTERY_WARNING,
                .old_state      = s_state,
                .new_state      = s_state,
                .battery_status = status,
                .voltage        = power_get_voltage(),
                .battery_pct     = power_get_battery_pct(),
            };
            for (uint8_t i = 0; i < s_callback_count; i++) {
                if (s_callbacks[i]) s_callbacks[i](&event);
            }

            /* Event-Bus und Power-Event-Group setzen */
            if (status == BATTERY_STATUS_CRITICAL) {
                event_bus_set(EVT_BATTERY_CRITICAL);
                xEventGroupSetBits(g_power_event_group, PEVT_BATTERY_CRITICAL);
            } else if (status == BATTERY_STATUS_LOW) {
                event_bus_set(EVT_BATTERY_LOW);
                xEventGroupSetBits(g_power_event_group, PEVT_BATTERY_LOW);
            } else if (status == BATTERY_STATUS_OVERVOLT) {
                xEventGroupSetBits(g_power_event_group, PEVT_BATTERY_OVERVOLT);
            }

            /* Generator-Events */
            if (power_is_generator_running()) {
                xEventGroupSetBits(g_power_event_group, PEVT_GENERATOR_ON);
            } else {
                xEventGroupSetBits(g_power_event_group, PEVT_GENERATOR_OFF);
            }

            /* Spannungs-Update Event */
            xEventGroupSetBits(g_power_event_group, PEVT_VOLTAGE_UPDATE);
        }
        prev_status = status;

        /* --- Kritische Batterie -> Deep-Sleep --- */
        if (status == BATTERY_STATUS_CRITICAL && s_state != POWER_STATE_DEEP_SLEEP) {
            ESP_LOGW(TAG, "Battery critical (%.2fV) -> DEEP_SLEEP",
                     power_get_voltage());
            transition_state(POWER_STATE_DEEP_SLEEP);
        }

        /* --- Inaktivitaets-Timeouts pruefen --- */
        check_inactivity_timeouts();

        /* --- Deep-Sleep ausfuehren, wenn State gesetzt --- */
        if (s_state == POWER_STATE_DEEP_SLEEP) {
            /* Wakeup-Timeout: Standard 60s fuer periodisches Aufwachen */
            power_enter_deep_sleep(60);
            /* Kein Return hier — esp_deep_sleep_start kehrt nicht zurueck */
        }
    }
}
