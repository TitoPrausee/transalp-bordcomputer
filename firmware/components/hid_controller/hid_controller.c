/*
 * hid_controller.c — Aggregated lifecycle for the Lenker-Controller component.
 *
 * Provides a single init/start/stop entry point that ties together
 * the input, mapping, GPIO, I2C, and BLE layers.
 *
 * All hardware setup (GPIO config, ISR install, I2C device add, task
 * creation) lives in controller_input.c. This file is a thin wrapper
 * that selects which transports to start based on the sources bitmask.
 */

#include "hid_controller.h"
#include "controller_input.h"
#include "controller_mapping.h"

#ifdef TEST_HOST
#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#define ESP_LOGE(tag, fmt, ...)  (void)0
#define ESP_LOGW(tag, fmt, ...)  (void)0
#define ESP_LOGI(tag, fmt, ...)  (void)0
#endif

#include "esp_log.h"

#ifndef TEST_HOST
#include "hid_controller_internal.h"
#endif

static const char *TAG = "hid_ctrl";

/* ============================================================
 * Lifecycle
 * ============================================================ */

esp_err_t hid_controller_init(void)
{
    /* Initialise input layer (debounce table) */
    controller_input_init();

    /* Initialise mapping layer (verify table) */
    controller_mapping_init();

    ESP_LOGI(TAG, "HID controller component initialised");
    return ESP_OK;
}

esp_err_t hid_controller_start(uint8_t sources)
{
    if (sources == 0) {
        ESP_LOGW(TAG, "No controller sources selected");
        return ESP_ERR_INVALID_ARG;
    }

#ifdef TEST_HOST
    (void)sources;
    return ESP_OK;
#else
    esp_err_t ret = ESP_OK;

    /* ---- GPIO source ---- */
    if (sources & (1 << CONTROLLER_SRC_GPIO)) {
        esp_err_t r = controller_input_start_gpio();
        if (r != ESP_OK && ret == ESP_OK) ret = r;
    }

    /* ---- I2C source ---- */
    if (sources & (1 << CONTROLLER_SRC_I2C)) {
        esp_err_t r = controller_input_start_i2c();
        if (r != ESP_OK && ret == ESP_OK) ret = r;
    }

    /* ---- BLE source ---- */
    if (sources & (1 << CONTROLLER_SRC_BLE)) {
        esp_err_t r = controller_input_start_ble();
        if (r != ESP_OK && ret == ESP_OK) ret = r;
    }

    return ret;
#endif
}

void hid_controller_stop(void)
{
#ifndef TEST_HOST
    controller_input_stop_gpio();
    controller_input_stop_i2c();
    controller_input_stop_ble();
    ESP_LOGI(TAG, "HID controller stopped");
#endif
}

void hid_controller_inject_button(controller_button_t button, bool pressed)
{
    controller_input_process_button(button, pressed);
}
