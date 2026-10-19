/*
 * hid_controller.c — Aggregated lifecycle for the Lenker-Controller component.
 *
 * Provides a single init/start/stop entry point that ties together
 * the input, mapping, GPIO, I2C, and BLE layers.
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "board_config.h"
#endif

static const char *TAG = "hid_ctrl";

#ifndef TEST_HOST

/* I2C device handle for the custom PCB controller */
static i2c_master_dev_handle_t s_i2c_dev = NULL;

/* External task declarations from controller_input.c */
extern QueueHandle_t s_gpio_queue;
extern TaskHandle_t  s_gpio_task;
extern TaskHandle_t  s_i2c_task;
extern TaskHandle_t  s_ble_task;
extern bool s_gpio_running;
extern bool s_i2c_running;
extern bool s_ble_running;

/* External task functions from controller_input.c */
extern void gpio_input_task(void *arg);
extern void i2c_poll_task(void *arg);
extern void ble_input_task(void *arg);

#endif /* !TEST_HOST */

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

#ifndef TEST_HOST
    esp_err_t ret = ESP_OK;

    /* ---- GPIO source ---- */
    if (sources & (1 << CONTROLLER_SRC_GPIO)) {
        s_gpio_queue = xQueueCreate(8, sizeof(controller_button_t));
        if (s_gpio_queue == NULL) {
            ESP_LOGE(TAG, "Failed to create GPIO queue");
            ret = ESP_ERR_NO_MEM;
        } else {
            /* Configure GPIO pins */
            const int btn_pins[] = { BTN_BACK_PIN, BTN_SELECT_PIN,
                                     BTN_MENU_PIN, BOOT_BTN_PIN };
            for (int i = 0; i < 4; i++) {
                gpio_config_t io_conf = {
                    .pin_bit_mask = (1ULL << btn_pins[i]),
                    .mode = GPIO_MODE_INPUT,
                    .pull_up_en = GPIO_PULLUP_ENABLE,
                    .pull_down_en = GPIO_PULLDOWN_DISABLE,
                    .intr_type = GPIO_INTR_NEGEDGE,  /* active-LOW */
                };
                gpio_config(&io_conf);
            }

            /* Install ISR service if not already installed */
            gpio_install_isr_service(0);

            /* Attach ISR handlers */
            static const controller_button_t isr_btns[] = {
                CTRL_BTN_BACK, CTRL_BTN_SELECT,
                CTRL_BTN_HOME, CTRL_BTN_NAV_UP,
            };
            for (int i = 0; i < 4; i++) {
                gpio_isr_handler_add(btn_pins[i], gpio_isr_handler,
                                     (void *)(intptr_t)isr_btns[i]);
            }

            s_gpio_running = true;
            xTaskCreatePinnedToCore(gpio_input_task, "ctrl_gpio",
                                    4096, NULL, 3, &s_gpio_task, TASK_CORE_UI);
            ESP_LOGI(TAG, "GPIO controller source started");
        }
    }

    /* ---- I2C source ---- */
    if (sources & (1 << CONTROLLER_SRC_I2C)) {
        /* Add the controller as a device on the existing I2C bus */
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address  = I2C_CONTROLLER_ADDR,
            .scl_speed_hz    = I2C_MASTER_FREQ_HZ,
        };

        extern i2c_master_bus_handle_t i2c_bus_handle;  /* from sensors init */
        esp_err_t i2c_ret = i2c_master_bus_add_device(
            i2c_bus_handle, &dev_cfg, &s_i2c_dev);

        if (i2c_ret != ESP_OK) {
            ESP_LOGW(TAG, "I2C controller device add failed: %s",
                     esp_err_to_name(i2c_ret));
            ret = i2c_ret;
        } else {
            s_i2c_running = true;
            xTaskCreatePinnedToCore(i2c_poll_task, "ctrl_i2c",
                                    4096, NULL, 3, &s_i2c_task, TASK_CORE_UI);
            ESP_LOGI(TAG, "I2C controller source started (addr=0x%02X)",
                     I2C_CONTROLLER_ADDR);
        }
    }

    /* ---- BLE source ---- */
    if (sources & (1 << CONTROLLER_SRC_BLE)) {
        s_ble_running = true;
        xTaskCreatePinnedToCore(ble_input_task, "ctrl_ble",
                                6144, NULL, 3, &s_ble_task, TASK_CORE_UI);
        ESP_LOGI(TAG, "BLE controller source started");
    }

    return ret;
#else
    (void)sources;
    return ESP_OK;
#endif
}

void hid_controller_stop(void)
{
#ifndef TEST_HOST
    if (s_gpio_running) {
        s_gpio_running = false;
        if (s_gpio_task) vTaskDelete(s_gpio_task);
        s_gpio_task = NULL;
    }
    if (s_i2c_running) {
        s_i2c_running = false;
        if (s_i2c_task) vTaskDelete(s_i2c_task);
        s_i2c_task = NULL;
    }
    if (s_ble_running) {
        s_ble_running = false;
        if (s_ble_task) vTaskDelete(s_ble_task);
        s_ble_task = NULL;
    }
    ESP_LOGI(TAG, "HID controller stopped");
#endif
}

void hid_controller_inject_button(controller_button_t button, bool pressed)
{
    controller_input_process_button(button, pressed);
}
