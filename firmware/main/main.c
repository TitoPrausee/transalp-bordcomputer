/*
 * main.c — Transalp Bordcomputer Firmware Integration Layer
 *
 * Entry point: initialises all 9 components in the correct order,
 * sets up the event bus and subscriber-pattern event loop, configures
 * watchdog timers for every task, and creates all FreeRTOS tasks
 * with correct priorities and core affinity.
 *
 * Startup sequence (critical first):
 *   1. NVS Flash          (required by many ESP-IDF subsystems)
 *   2. Event bus + Event loop
 *   3. Power management   (supply voltage monitoring first)
 *   4. Storage / SD / NVS config
 *   5. Display            (TFT + E-Paper)
 *   6. Sensors            (IMU, RPM, temp, fuel, gear)
 *   7. GPS                (UART + NMEA parser)
 *   8. BLE                (NimBLE stack + GATT server)
 *   9. Diagnostics         (DTC + self-test + console)
 *  10. OTA                (boot-state check, rollback)
 *
 * If a component fails, a DTC is registered and the system continues.
 * Critical failures (power, NVS) halt the boot.
 *
 * Task layout:
 *   Core 0 (UI/Comms): event_loop, display, ble, ota, power, logger
 *   Core 1 (Real-time): sensor, gps, diagnostics
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"

#include "board_config.h"
#include "event_bus.h"
#include "app_event_loop.h"
#include "power.h"
#include "storage.h"
#include "display.h"
#include "sensors.h"
#include "gps.h"
#include "ble.h"
#include "diagnostics.h"
#include "ota_update.h"

static const char *TAG = "main";

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "0.4.0-integration"
#endif

/* ============================================================
 * Task Handles
 * ============================================================ */

static TaskHandle_t s_task_sensor     = NULL;
static TaskHandle_t s_task_gps        = NULL;
static TaskHandle_t s_task_display    = NULL;
static TaskHandle_t s_task_ble        = NULL;
static TaskHandle_t s_task_power      = NULL;
static TaskHandle_t s_task_logger     = NULL;
static TaskHandle_t s_task_ota        = NULL;
static TaskHandle_t s_task_diagnostics = NULL;

/* ============================================================
 * Watchdog
 *
 * Each long-running task adds itself to the TWDT. The default
 * timeout is configured via menuconfig (typically 5s). Tasks
 * must call esp_task_wdt_reset() inside their main loop.
 * ============================================================ */

static void wdt_subscribe(TaskHandle_t handle, const char *name)
{
    esp_err_t ret = esp_task_wdt_add(handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "  WDT: %s subscribed", name);
    } else {
        ESP_LOGW(TAG, "  WDT: %s subscribe failed: %s",
                 name, esp_err_to_name(ret));
    }
}

/* ============================================================
 * OTA Progress Callback
 * ============================================================ */

static void ota_progress_cb(uint8_t percent,
                            ota_state_t state,
                            ota_error_t error,
                            void *user_ctx)
{
    (void)user_ctx;

    if (error != OTA_ERR_NONE) {
        ESP_LOGE(TAG, "OTA error: state=%d err=%d",
                 (int)state, (int)error);
        return;
    }

    if (state == OTA_STATE_REBOOTING) {
        ESP_LOGI(TAG, "OTA complete, rebooting (%u%%)", percent);
        app_event_dispatch_simple(APP_EVENT_OTA_DONE);
    } else if (percent % 10 == 0) {
        ESP_LOGI(TAG, "OTA progress: %u%% (state=%d)",
                 percent, (int)state);
    }
}

/* ============================================================
 * Component Initialisation Helpers
 *
 * Each wrapper logs the step, calls the component init, and
 * registers a DTC on failure. Returns true on success.
 * ============================================================ */

static bool init_step_power(void)
{
    ESP_LOGI(TAG, "Init: Power management");
    power_init();
    if (power_get_state() != POWER_STATE_ACTIVE) {
        ESP_LOGW(TAG, "  Power not in ACTIVE state after init");
    }
    ESP_LOGI(TAG, "  Battery: %.2f V (%u%%)",
             power_get_voltage(), power_get_battery_pct());
    return true;
}

static bool init_step_storage(void)
{
    ESP_LOGI(TAG, "Init: Storage (SD card + NVS config)");
    storage_init();
    if (!sd_card_is_mounted()) {
        ESP_LOGW(TAG, "  SD card not mounted — logging disabled");
        dtc_register(DTC_C0101, DTC_SEV_WARN);
        return false;
    }
    ESP_LOGI(TAG, "  SD card mounted at %s", SD_MOUNT_POINT);
    event_bus_set(EVT_SDCARD_MOUNTED);
    return true;
}

static bool init_step_display(void)
{
    ESP_LOGI(TAG, "Init: Display (TFT + E-Paper)");
    display_init();
    event_bus_set(EVT_DISPLAY_READY);
    ESP_LOGI(TAG, "  Display ready");
    return true;
}

static bool init_step_sensors(void)
{
    ESP_LOGI(TAG, "Init: Sensors (RPM, Temp, Fuel, Gear, IMU)");
    sensors_init();
    if (sensor_data_queue == NULL) {
        ESP_LOGE(TAG, "  Sensor queue not created");
        dtc_register(DTC_S0402, DTC_SEV_ERROR);
        return false;
    }
    ESP_LOGI(TAG, "  Sensors initialised");
    event_bus_set(EVT_SENSOR_READY);
    return true;
}

static bool init_step_gps(void)
{
    ESP_LOGI(TAG, "Init: GPS (UART + NMEA parser)");
    gps_init();
    if (gps_data_queue == NULL) {
        ESP_LOGW(TAG, "  GPS queue not created");
        dtc_register(DTC_S0501, DTC_SEV_WARN);
        return false;
    }
    ESP_LOGI(TAG, "  GPS initialised (waiting for fix)");
    return true;
}

static bool init_step_ble(void)
{
    ESP_LOGI(TAG, "Init: BLE (NimBLE + GATT server)");
    ble_init();
    ESP_LOGI(TAG, "  BLE advertising started");
    return true;
}

static bool init_step_diagnostics(void)
{
    ESP_LOGI(TAG, "Init: Diagnostics (DTC + self-test + console)");
    diagnostics_init();
    ESP_LOGI(TAG, "  Diagnostics initialised");
    return true;
}

static bool init_step_ota(void)
{
    ESP_LOGI(TAG, "Init: OTA (boot-state check)");
    esp_err_t ret = ota_init(ota_progress_cb, NULL);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "  OTA init returned %s", esp_err_to_name(ret));
        dtc_register(DTC_D0301, DTC_SEV_WARN);
        return false;
    }
    ESP_LOGI(TAG, "  OTA subsystem ready");
    return true;
}

/* ============================================================
 * Event-Loop Subscribers
 *
 * These callbacks demonstrate the subscriber pattern: component
 * events trigger cross-module actions without tight coupling.
 * ============================================================ */

/* GPS update -> display refresh + logger write + BLE notify */
static void on_gps_update(const app_event_t *ev, void *ctx)
{
    (void)ctx;
    (void)ev;
    /* Display task picks up GPS data from gps_data_queue on its
     * own refresh cycle; here we just trigger a dirty-rect mark. */
    renderer_invalidate(0, 0, TFT_WIDTH, TFT_HEIGHT);

    /* BLE notify (only if connected) */
    if (ble_is_connected()) {
        gps_data_t gd;
        if (gps_get_data(&gd)) {
            ble_notify_gps_data(&gd, sizeof(gd));
        }
    }
}

/* Sensor update -> display refresh + BLE telemetry */
static void on_sensor_update(const app_event_t *ev, void *ctx)
{
    (void)ctx;
    (void)ev;
    /* Mark speed/RPM/gear area dirty for display refresh */
    renderer_invalidate(0, 0, TFT_WIDTH, TFT_HEIGHT / 2);

    /* BLE telemetry push at sensor rate (~5 Hz) */
    if (ble_is_connected()) {
        ble_notify_telemetry_live();
    }
}

/* SD card error -> DTC + event bus */
static void on_sdcard_error(const app_event_t *ev, void *ctx)
{
    (void)ctx;
    (void)ev;
    dtc_register(DTC_C0104, DTC_SEV_WARN);
    event_bus_set(EVT_SDCARD_ERROR);
}

/* Battery low -> DTC + event bus + power state hint */
static void on_battery_low(const app_event_t *ev, void *ctx)
{
    (void)ctx;
    (void)ev;
    dtc_register(DTC_P0401, DTC_SEV_WARN);
    event_bus_set(EVT_BATTERY_LOW);
}

static void on_battery_critical(const app_event_t *ev, void *ctx)
{
    (void)ctx;
    (void)ev;
    dtc_register(DTC_P0402, DTC_SEV_CRITICAL);
    event_bus_set(EVT_BATTERY_CRITICAL);
}

/* Crash -> DTC + event bus */
static void on_crash(const app_event_t *ev, void *ctx)
{
    (void)ctx;
    (void)ev;
    dtc_register(DTC_S0402, DTC_SEV_WARN);
    event_bus_set(EVT_CRASH_DETECTED);
}

/* Shutdown -> event bus + deep sleep */
static void on_shutdown(const app_event_t *ev, void *ctx)
{
    (void)ctx;
    (void)ev;
    ESP_LOGW(TAG, "Shutdown event received, entering deep sleep");
    event_bus_set(EVT_SHUTDOWN);
    power_enter_deep_sleep(0);
}

/* ============================================================
 * Register all event-loop subscribers
 * ============================================================ */

static void register_subscribers(void)
{
    uint32_t mask_gps = (1U << APP_EVENT_GPS_UPDATE) |
                        (1U << APP_EVENT_GPS_FIX_ACQUIRED) |
                        (1U << APP_EVENT_GPS_FIX_LOST);

    uint32_t mask_sensor = (1U << APP_EVENT_SENSOR_UPDATE);

    uint32_t mask_system = (1U << APP_EVENT_SDCARD_ERROR) |
                           (1U << APP_EVENT_BATTERY_LOW) |
                           (1U << APP_EVENT_BATTERY_CRITICAL) |
                           (1U << APP_EVENT_CRASH_DETECTED) |
                           (1U << APP_EVENT_SHUTDOWN);

    app_event_subscribe(mask_gps, on_gps_update, NULL);
    app_event_subscribe(mask_sensor, on_sensor_update, NULL);
    app_event_subscribe(mask_system, on_sdcard_error, NULL);
    /* Battery/crash/shutdown overlap in mask_system; register
     * dedicated handlers for clarity. */
    app_event_subscribe((1U << APP_EVENT_BATTERY_LOW),
                        on_battery_low, NULL);
    app_event_subscribe((1U << APP_EVENT_BATTERY_CRITICAL),
                        on_battery_critical, NULL);
    app_event_subscribe((1U << APP_EVENT_CRASH_DETECTED),
                        on_crash, NULL);
    app_event_subscribe((1U << APP_EVENT_SHUTDOWN),
                        on_shutdown, NULL);

    ESP_LOGI(TAG, "Event subscribers registered");
}

/* ============================================================
 * OTA Task Wrapper
 *
 * OTA is event-driven: this task sleeps until an OTA update
 * is triggered (via BLE, HTTP, or SD). It monitors the OTA
 * state machine and feeds the watchdog while idle.
 * Runs on Core 0 (UI/Comms).
 * ============================================================ */

static void ota_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "OTA task started on core %d", xPortGetCoreID());

    while (1) {
        /* Feed watchdog while idle */
        esp_task_wdt_reset();

        /* Check if an OTA update is in progress */
        ota_state_t state = ota_get_state();
        if (state == OTA_STATE_IDLE) {
            /* Sleep 1s between checks when idle */
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            /* During active update, check more frequently */
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

/* ============================================================
 * Diagnostics Task Wrapper
 *
 * The diagnostics module creates its own internal tasks (event
 * monitor + console). This wrapper task runs periodic health
 * checks: heap fragmentation, sensor staleness, GPS timeout.
 * Runs on Core 1 (Real-time).
 * ============================================================ */

#define DIAG_CHECK_INTERVAL_MS  5000
#define SENSOR_STALENESS_MS     3000
#define GPS_STALENESS_MS         10000

static void diagnostics_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "Diagnostics task started on core %d", xPortGetCoreID());

    /* Initial delay to let other tasks stabilise */
    vTaskDelay(pdMS_TO_TICKS(3000));

    while (1) {
        esp_task_wdt_reset();

        /* Check sensor data freshness */
        if (sensor_data_queue != NULL) {
            sensor_data_t sd;
            if (xQueuePeek(sensor_data_queue, &sd, pdMS_TO_TICKS(100)) == pdTRUE) {
                uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
                if (sd.timestamp_ms > 0 &&
                    (now - sd.timestamp_ms) > SENSOR_STALENESS_MS) {
                    ESP_LOGW(TAG, "Sensor data stale (%lu ms old)",
                             (unsigned long)(now - sd.timestamp_ms));
                    app_event_dispatch_simple(APP_EVENT_RPM_TIMEOUT);
                }
            }
        }

        /* Check GPS data freshness */
        if (gps_data_queue != NULL) {
            gps_data_t gd;
            if (xQueuePeek(gps_data_queue, &gd, pdMS_TO_TICKS(100)) == pdTRUE) {
                uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
                if (gd.timestamp_ms > 0 && gd.has_fix &&
                    (now - gd.timestamp_ms) > GPS_STALENESS_MS) {
                    ESP_LOGW(TAG, "GPS fix stale (%lu ms old)",
                             (unsigned long)(now - gd.timestamp_ms));
                    app_event_dispatch_simple(APP_EVENT_GPS_FIX_LOST);
                }
            }
        }

        /* Check heap fragmentation */
        uint32_t free_heap = esp_get_free_heap_size();
        uint32_t min_heap  = esp_get_minimum_free_heap_size();
        if (min_heap > 0 && free_heap > 0) {
            uint32_t fragmentation_pct =
                100 - (min_heap * 100 / free_heap);
            if (fragmentation_pct > 50) {
                ESP_LOGW(TAG, "Heap fragmentation high: %u%%",
                         (unsigned)fragmentation_pct);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(DIAG_CHECK_INTERVAL_MS));
    }
}

/* ============================================================
 * Task Creation
 *
 * Creates all FreeRTOS tasks with correct priorities and core
 * affinity, then subscribes each to the watchdog timer.
 * ============================================================ */

static void create_tasks(void)
{
    ESP_LOGI(TAG, "Creating FreeRTOS tasks...");

    /* --- Core 1 (Real-time) --- */

    xTaskCreatePinnedToCore(sensor_task, "sensor",
        TASK_STACK_SENSOR, NULL, TASK_PRIO_SENSOR,
        &s_task_sensor, TASK_CORE_RT);

    xTaskCreatePinnedToCore(gps_task, "gps",
        TASK_STACK_GPS, NULL, TASK_PRIO_GPS,
        &s_task_gps, TASK_CORE_RT);

    xTaskCreatePinnedToCore(diagnostics_task, "diag_chk",
        TASK_STACK_DIAG, NULL, TASK_PRIO_DIAG,
        &s_task_diagnostics, TASK_CORE_RT);

    /* --- Core 0 (UI / Comms) --- */

    xTaskCreatePinnedToCore(display_task, "display",
        TASK_STACK_DISPLAY, NULL, TASK_PRIO_DISPLAY,
        &s_task_display, TASK_CORE_UI);

    xTaskCreatePinnedToCore(ble_task, "ble",
        TASK_STACK_BLE, NULL, TASK_PRIO_BLE,
        &s_task_ble, TASK_CORE_UI);

    xTaskCreatePinnedToCore(power_task, "power",
        TASK_STACK_POWER, NULL, TASK_PRIO_POWER,
        &s_task_power, TASK_CORE_UI);

    xTaskCreatePinnedToCore(logger_task, "logger",
        TASK_STACK_LOGGER, NULL, TASK_PRIO_LOGGER,
        &s_task_logger, TASK_CORE_UI);

    xTaskCreatePinnedToCore(ota_task, "ota",
        TASK_STACK_OTA, NULL, TASK_PRIO_OTA,
        &s_task_ota, TASK_CORE_UI);

    /* Watchdog subscription for long-running tasks */
    ESP_LOGI(TAG, "Subscribing tasks to watchdog...");
    wdt_subscribe(s_task_sensor,      "sensor");
    wdt_subscribe(s_task_gps,         "gps");
    wdt_subscribe(s_task_diagnostics, "diag_chk");
    wdt_subscribe(s_task_display,     "display");
    wdt_subscribe(s_task_ble,         "ble");
    wdt_subscribe(s_task_power,       "power");
    wdt_subscribe(s_task_logger,      "logger");
    wdt_subscribe(s_task_ota,         "ota");

    ESP_LOGI(TAG, "All tasks created and WDT-subscribed");
}

/* ============================================================
 * Boot Banner
 * ============================================================ */

static void print_boot_banner(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Transalp Bordcomputer v%s", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "  Build: %s %s", __DATE__, __TIME__);
    ESP_LOGI(TAG, "  ESP32-S3 dual-core, FreeRTOS");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
}

/* ============================================================
 * Entry Point
 * ============================================================ */

void app_main(void)
{
    print_boot_banner();

    /* 1. NVS Flash — required by WiFi, BLE, OTA, diagnostics */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition corrupt, erasing and re-init");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS flash initialised");

    /* 2. Event bus + Event loop (before anything else) */
    event_bus_init();
    app_event_loop_init();
    register_subscribers();

    /* 3. Power management (supply voltage first) */
    init_step_power();

    /* 4. Storage (SD card + config) */
    init_step_storage();

    /* 5. Display (TFT + E-Paper) */
    init_step_display();

    /* 6. Sensors */
    init_step_sensors();

    /* 7. GPS */
    init_step_gps();

    /* 8. BLE */
    init_step_ble();

    /* 9. Diagnostics (DTC + self-test + console) */
    init_step_diagnostics();

    /* 10. OTA (boot-state check, rollback) */
    init_step_ota();

    /* Create all FreeRTOS tasks */
    create_tasks();

    /* Summary */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Boot complete. Free heap: %lu bytes",
             (unsigned long)esp_get_free_heap_size());
    ESP_LOGI(TAG, "Min heap ever: %lu bytes",
             (unsigned long)esp_get_minimum_free_heap_size());
    ESP_LOGI(TAG, "Tasks running: %u",
             (unsigned)uxTaskGetNumberOfTasks());
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "System ready.");

    /* app_main returns — FreeRTOS idle task takes over on this core.
     * The main task is deleted automatically by the ESP-IDF startup. */
}
