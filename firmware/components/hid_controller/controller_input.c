/*
 * controller_input.c — GPIO/I2C/BLE Input-Reading with debouncing.
 *
 * Transports:
 *   GPIO:  Interrupt-based, active-LOW with pull-up. Buttons on
 *          dedicated GPIO pins. ISR dispatches to a FreeRTOS queue,
 *          a processing task applies debounce and calls the mapping.
 *   I2C:   Custom PCB (ATmega328P/RP2040) as I2C slave. A task polls
 *          the slave at CTRL_I2C_POLL_MS intervals, reads a 2-byte
 *          button mask, and translates bits to controller_button_t.
 *   BLE:   NimBLE GATT client. Subscribes to the controller
 *          characteristic; incoming notifications are processed in
 *          the NimBLE host task context.
 *
 * Debounce: Each button has a per-button timestamp. A second press
 * within CTRL_DEBOUNCE_MS (50 ms) is rejected. Edge detection:
 * only the rising edge (idle -> pressed) triggers an event.
 */

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
#include "esp_timer.h"
#include <string.h>

#ifndef TEST_HOST
#include "hid_controller_internal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "board_config.h"

/* I2C bus handle owned by the sensors component (sensors.h). */
extern i2c_master_bus_handle_t g_i2c_bus;
#endif

static const char *TAG = "ctrl_input";

/* ============================================================
 * Debounce state
 * ============================================================ */

button_debounce_t g_btn_debounce[CTRL_MAX_BUTTONS];

#ifndef TEST_HOST

/* ============================================================
 * GPIO button table
 *
 * PINMAP.md says all GPIOs are assigned. For the Lenker-Controller
 * we repurpose the existing button pins (GPIO6=MENU, GPIO7=SELECT,
 * GPIO3=BACK, GPIO0=BOOT) for direct-wired mode. The remaining
 * buttons (NAV_DOWN/LEFT/RIGHT, VOL, MEDIA) go through the I2C
 * custom PCB or BLE wireless path.
 *
 * Pin numbers come from board_config.h (CTRL_BTN_*_PIN).
 * ============================================================ */

typedef struct {
    controller_button_t  button;
    int                  gpio_pin;
} gpio_button_map_t;

static const gpio_button_map_t s_gpio_buttons[] = {
    { CTRL_BTN_BACK,    CTRL_BTN_BACK_PIN    },  /* GPIO3  */
    { CTRL_BTN_SELECT,  CTRL_BTN_SELECT_PIN  },  /* GPIO7  */
    { CTRL_BTN_HOME,    CTRL_BTN_HOME_PIN     },  /* GPIO6  */
    { CTRL_BTN_NAV_UP,  CTRL_BTN_NAV_UP_PIN   },  /* GPIO0  */
};

#define GPIO_BTN_COUNT  (sizeof(s_gpio_buttons) / sizeof(s_gpio_buttons[0]))

/* ISR queue: button GPIO -> processing task */
static QueueHandle_t s_gpio_queue  = NULL;
static TaskHandle_t   s_gpio_task   = NULL;
static bool           s_gpio_running = false;

/* I2C device handle (custom PCB controller on the sensors I2C bus) */
static i2c_master_dev_handle_t s_i2c_dev = NULL;
static TaskHandle_t            s_i2c_task = NULL;
static bool                    s_i2c_running = false;

/* BLE task handle */
static TaskHandle_t s_ble_task    = NULL;
static bool         s_ble_running  = false;

/* ============================================================
 * GPIO ISR handler
 * ============================================================ */

/* 'arg' is read-only, but ESP-IDF's gpio_isr_t is fixed to `void (*)(void *)`,
 * so it cannot be declared const without an unsafe function-pointer cast at
 * registration. Suppressed per TOT-61 strict-analysis pass. */
// cppcheck-suppress constParameterCallback
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    controller_button_t btn = (controller_button_t)(intptr_t)arg;
    if (s_gpio_queue != NULL) {
        xQueueSendFromISR(s_gpio_queue, &btn, NULL);
    }
}

/* ============================================================
 * GPIO input task: debounce + dispatch
 * ============================================================ */

static void gpio_input_task(void *arg)
{
    (void)arg;
    controller_button_t btn;
    ESP_LOGI(TAG, "GPIO input task started");

    while (s_gpio_running) {
        if (xQueueReceive(s_gpio_queue, &btn, pdMS_TO_TICKS(100)) != pdTRUE) {
            continue;
        }
        /* Active-LOW: button pressed = GPIO reads 0 */
        const gpio_button_map_t *map = NULL;
        for (int i = 0; i < (int)GPIO_BTN_COUNT; i++) {
            if (s_gpio_buttons[i].button == btn) {
                map = &s_gpio_buttons[i];
                break;
            }
        }
        if (map == NULL) continue;

        int level = gpio_get_level(map->gpio_pin);
        bool pressed = (level == 0);  /* active-LOW */

        controller_input_process_button(btn, pressed);
    }

    vTaskDelete(NULL);
}

/* ============================================================
 * I2C poll task: read button mask from ATmega328P/RP2040 slave
 * ============================================================ */

/* Register: 2-byte button bitmask */
#define I2C_REG_BUTTON_MASK  0x00

/* Button bit positions in the I2C button mask */
#define I2C_BIT_NAV_UP       (1 << 0)
#define I2C_BIT_NAV_DOWN     (1 << 1)
#define I2C_BIT_NAV_LEFT     (1 << 2)
#define I2C_BIT_NAV_RIGHT    (1 << 3)
#define I2C_BIT_SELECT       (1 << 4)
#define I2C_BIT_BACK         (1 << 5)
#define I2C_BIT_HOME         (1 << 6)
#define I2C_BIT_VOL_UP       (1 << 7)
#define I2C_BIT_VOL_DOWN     (1 << 8)
#define I2C_BIT_MEDIA_NEXT   (1 << 9)
#define I2C_BIT_MEDIA_PREV   (1 << 10)

static const struct {
    uint16_t            mask;
    controller_button_t btn;
} s_i2c_btn_map[] = {
    { I2C_BIT_NAV_UP,     CTRL_BTN_NAV_UP     },
    { I2C_BIT_NAV_DOWN,   CTRL_BTN_NAV_DOWN   },
    { I2C_BIT_NAV_LEFT,   CTRL_BTN_NAV_LEFT   },
    { I2C_BIT_NAV_RIGHT,  CTRL_BTN_NAV_RIGHT  },
    { I2C_BIT_SELECT,     CTRL_BTN_SELECT     },
    { I2C_BIT_BACK,       CTRL_BTN_BACK       },
    { I2C_BIT_HOME,       CTRL_BTN_HOME       },
    { I2C_BIT_VOL_UP,     CTRL_BTN_VOL_UP     },
    { I2C_BIT_VOL_DOWN,   CTRL_BTN_VOL_DOWN   },
    { I2C_BIT_MEDIA_NEXT, CTRL_BTN_MEDIA_NEXT },
    { I2C_BIT_MEDIA_PREV, CTRL_BTN_MEDIA_PREV },
};

#define I2C_BTN_COUNT  (sizeof(s_i2c_btn_map) / sizeof(s_i2c_btn_map[0]))

static uint16_t s_i2c_prev_mask = 0;

static void i2c_poll_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "I2C controller poll task started (addr=0x%02X, poll=%dms)",
             CTRL_I2C_ADDR, CTRL_I2C_POLL_MS);

    while (s_i2c_running) {
        /* Read 2-byte button mask from I2C slave */
        uint8_t reg = I2C_REG_BUTTON_MASK;
        uint8_t buf[2] = {0, 0};

        esp_err_t ret = i2c_master_transmit_receive(
            s_i2c_dev,
            &reg, 1,
            buf, 2,
            pdMS_TO_TICKS(50));

        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "I2C controller read failed: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(CTRL_I2C_POLL_MS));
            continue;
        }

        uint16_t mask = (uint16_t)(buf[0] | (buf[1] << 8));

        /* Detect new presses (bits that are set now but weren't before) */
        uint16_t new_presses  = mask & ~s_i2c_prev_mask;
        uint16_t new_releases = ~mask & s_i2c_prev_mask;
        s_i2c_prev_mask = mask;

        /* Process each changed button */
        for (int i = 0; i < (int)I2C_BTN_COUNT; i++) {
            if (new_presses & s_i2c_btn_map[i].mask) {
                controller_input_process_button(s_i2c_btn_map[i].btn, true);
            }
            if (new_releases & s_i2c_btn_map[i].mask) {
                controller_input_process_button(s_i2c_btn_map[i].btn, false);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(CTRL_I2C_POLL_MS));
    }

    vTaskDelete(NULL);
}

/* ============================================================
 * BLE notification handler (called from NimBLE host task)
 * ============================================================ */

/* Button bit positions in the BLE notification byte(s) */
#define BLE_BIT_NAV_UP       (1 << 0)
#define BLE_BIT_NAV_DOWN     (1 << 1)
#define BLE_BIT_NAV_LEFT     (1 << 2)
#define BLE_BIT_NAV_RIGHT    (1 << 3)
#define BLE_BIT_SELECT       (1 << 4)
#define BLE_BIT_BACK         (1 << 5)
#define BLE_BIT_HOME         (1 << 6)
#define BLE_BIT_VOL_UP       (1 << 7)

/* For 2-byte BLE notifications (volume down + media) */
#define BLE_BIT_VOL_DOWN    (1 << 0)  /* byte[1], bit 0 */
#define BLE_BIT_MEDIA_NEXT  (1 << 1)  /* byte[1], bit 1 */
#define BLE_BIT_MEDIA_PREV  (1 << 2)  /* byte[1], bit 2 */

static const struct {
    uint8_t             byte_idx;
    uint8_t             bit;
    controller_button_t btn;
} s_ble_btn_map[] = {
    { 0, BLE_BIT_NAV_UP,      CTRL_BTN_NAV_UP     },
    { 0, BLE_BIT_NAV_DOWN,    CTRL_BTN_NAV_DOWN   },
    { 0, BLE_BIT_NAV_LEFT,    CTRL_BTN_NAV_LEFT   },
    { 0, BLE_BIT_NAV_RIGHT,   CTRL_BTN_NAV_RIGHT  },
    { 0, BLE_BIT_SELECT,      CTRL_BTN_SELECT     },
    { 0, BLE_BIT_BACK,        CTRL_BTN_BACK       },
    { 0, BLE_BIT_HOME,        CTRL_BTN_HOME       },
    { 0, BLE_BIT_VOL_UP,      CTRL_BTN_VOL_UP     },
    { 1, BLE_BIT_VOL_DOWN,    CTRL_BTN_VOL_DOWN   },
    { 1, BLE_BIT_MEDIA_NEXT,  CTRL_BTN_MEDIA_NEXT },
    { 1, BLE_BIT_MEDIA_PREV,  CTRL_BTN_MEDIA_PREV },
};

#define BLE_BTN_COUNT  (sizeof(s_ble_btn_map) / sizeof(s_ble_btn_map[0]))

static uint8_t s_ble_prev[2] = {0, 0};

/*
 * Called by the NimBLE host task when a controller notification arrives.
 * Parses the button mask and processes state changes.
 */
void hid_controller_ble_notify_handler(const uint8_t *data, uint8_t len)
{
    if (data == NULL || len < 1) return;

    uint8_t cur[2] = {0, 0};
    cur[0] = data[0];
    if (len >= 2) cur[1] = data[1];

    for (int i = 0; i < (int)BLE_BTN_COUNT; i++) {
        uint8_t cur_bit  = (cur[s_ble_btn_map[i].byte_idx]  & s_ble_btn_map[i].bit) ? 1 : 0;
        uint8_t prev_bit = (s_ble_prev[s_ble_btn_map[i].byte_idx] & s_ble_btn_map[i].bit) ? 1 : 0;

        if (cur_bit && !prev_bit) {
            controller_input_process_button(s_ble_btn_map[i].btn, true);
        }
        if (!cur_bit && prev_bit) {
            controller_input_process_button(s_ble_btn_map[i].btn, false);
        }
    }

    s_ble_prev[0] = cur[0];
    s_ble_prev[1] = cur[1];
}

/* ============================================================
 * BLE input task (NimBLE GATT client skeleton)
 * ============================================================ */

static void ble_input_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "BLE controller client task started");

    while (s_ble_running) {
        /*
         * BLE GATT client connection management:
         * 1. If not connected, scan for controller (CTRL_BLE_SVC_UUID)
         * 2. On connect, discover service + subscribe to char
         *    (CTRL_BLE_CHAR_UUID)
         * 3. Notifications arrive via hid_controller_ble_notify_handler()
         *
         * Full implementation requires NimBLE GATT client API calls
         * (ble_gap_connect, ble_gattc_disc_all_svcs, ble_gattc_write
         * for CCCD subscription). Deferred to ble component integration.
         */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

/* ============================================================
 * Transport start/stop functions
 * ============================================================ */

esp_err_t controller_input_start_gpio(void)
{
    s_gpio_queue = xQueueCreate(8, sizeof(controller_button_t));
    if (s_gpio_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create GPIO queue");
        return ESP_ERR_NO_MEM;
    }

    /* Configure GPIO pins (active-LOW with pull-up) */
    for (int i = 0; i < (int)GPIO_BTN_COUNT; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << s_gpio_buttons[i].gpio_pin),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_NEGEDGE,  /* active-LOW */
        };
        esp_err_t gret = gpio_config(&io_conf);
        if (gret != ESP_OK) {
            ESP_LOGW(TAG, "gpio_config pin %d failed: %s",
                     s_gpio_buttons[i].gpio_pin, esp_err_to_name(gret));
        }
    }

    /* Install ISR service if not already installed */
    gpio_install_isr_service(0);

    /* Attach ISR handlers */
    for (int i = 0; i < (int)GPIO_BTN_COUNT; i++) {
        gpio_isr_handler_add(s_gpio_buttons[i].gpio_pin, gpio_isr_handler,
                             (void *)(intptr_t)s_gpio_buttons[i].button);
    }

    s_gpio_running = true;
    xTaskCreatePinnedToCore(gpio_input_task, "ctrl_gpio",
                            TASK_STACK_CONTROLLER, NULL,
                            TASK_PRIO_CONTROLLER, &s_gpio_task, TASK_CORE_UI);
    ESP_LOGI(TAG, "GPIO controller source started (%d buttons)", (int)GPIO_BTN_COUNT);
    return ESP_OK;
}

esp_err_t controller_input_start_i2c(void)
{
    /* Add the controller as a device on the sensors I2C bus */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = CTRL_I2C_ADDR,
        .scl_speed_hz    = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t ret = i2c_master_bus_add_device(g_i2c_bus, &dev_cfg, &s_i2c_dev);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2C controller device add failed: %s", esp_err_to_name(ret));
        return ret;
    }

    s_i2c_running = true;
    s_i2c_prev_mask = 0;
    xTaskCreatePinnedToCore(i2c_poll_task, "ctrl_i2c",
                            TASK_STACK_CONTROLLER, NULL,
                            TASK_PRIO_CONTROLLER, &s_i2c_task, TASK_CORE_UI);
    ESP_LOGI(TAG, "I2C controller source started (addr=0x%02X)", CTRL_I2C_ADDR);
    return ESP_OK;
}

esp_err_t controller_input_start_ble(void)
{
    s_ble_running = true;
    s_ble_prev[0] = 0;
    s_ble_prev[1] = 0;
    xTaskCreatePinnedToCore(ble_input_task, "ctrl_ble",
                            TASK_STACK_BLE, NULL,
                            TASK_PRIO_BLE, &s_ble_task, TASK_CORE_UI);
    ESP_LOGI(TAG, "BLE controller source started");
    return ESP_OK;
}

void controller_input_stop_gpio(void)
{
    if (s_gpio_running) {
        s_gpio_running = false;
        if (s_gpio_task) vTaskDelete(s_gpio_task);
        s_gpio_task = NULL;
    }
}

void controller_input_stop_i2c(void)
{
    if (s_i2c_running) {
        s_i2c_running = false;
        if (s_i2c_task) vTaskDelete(s_i2c_task);
        s_i2c_task = NULL;
    }
}

void controller_input_stop_ble(void)
{
    if (s_ble_running) {
        s_ble_running = false;
        if (s_ble_task) vTaskDelete(s_ble_task);
        s_ble_task = NULL;
    }
}

#endif /* !TEST_HOST */

/* ============================================================
 * Transport-agnostic debounce + dispatch
 * ============================================================ */

void controller_input_init(void)
{
    memset(g_btn_debounce, 0, sizeof(g_btn_debounce));
    ESP_LOGI(TAG, "Input layer initialised (%d buttons, debounce=%dms)",
             CTRL_MAX_BUTTONS, CTRL_DEBOUNCE_MS);
}

bool controller_input_would_accept(controller_button_t button, int64_t now_us)
{
    if (button < 0 || button >= CTRL_MAX_BUTTONS) return false;

    int64_t elapsed_ms = (now_us - g_btn_debounce[button].last_press_us) / 1000;
    return (elapsed_ms >= CTRL_DEBOUNCE_MS);
}

bool controller_input_process_button(controller_button_t button, bool pressed)
{
    if (button < 0 || button >= CTRL_MAX_BUTTONS) {
        return false;
    }

    button_debounce_t *bd = &g_btn_debounce[button];
    int64_t now_us = esp_timer_get_time();

    /* Edge detection: only process on state change */
    if (bd->last_state == pressed) {
        return false;  /* No state change, ignore */
    }

    /* Only forward press events (rising edge: idle -> pressed) */
    if (!pressed) {
        bd->last_state = false;
        return false;  /* Release events are not forwarded */
    }

    /* Debounce: reject if within debounce window */
    int64_t elapsed_us = now_us - bd->last_press_us;
    if (elapsed_us < (int64_t)(CTRL_DEBOUNCE_MS * 1000)) {
        ESP_LOGD(TAG, "Button %d debounced (%lld ms < %d ms)",
                 button, (long long)(elapsed_us / 1000), CTRL_DEBOUNCE_MS);
        bd->last_state = true;  /* Update state even if debounced */
        return false;
    }

    /* Accept the press */
    bd->last_press_us = now_us;
    bd->last_state = true;

    ESP_LOGI(TAG, "Button %d pressed -> dispatch", button);

    /* Forward to mapping layer */
    controller_mapping_dispatch(button);

    return true;
}

void controller_input_reset_button(controller_button_t button)
{
    if (button < 0 || button >= CTRL_MAX_BUTTONS) return;
    memset(&g_btn_debounce[button], 0, sizeof(button_debounce_t));
}

void controller_input_reset_all(void)
{
    memset(g_btn_debounce, 0, sizeof(g_btn_debounce));
}
