/*
 * controller_input.c — GPIO/ADC/I2C Input-Reading with debouncing.
 *
 * Transports:
 *   GPIO:  Interrupt-based, active-LOW with pull-up. 6 buttons on
 *          dedicated GPIO pins. ISR dispatches to a FreeRTOS queue,
 *          a processing task applies debounce and calls the mapping.
 *   I2C:   Custom PCB (ATmega328P/RP2040) as I2C slave. A task polls
 *          the slave at 100 ms intervals, reads a 2-byte button mask,
 *          and translates bits to controller_button_t values.
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "board_config.h"
#endif

static const char *TAG = "ctrl_input";

/* ============================================================
 * Debounce state
 * ============================================================ */

button_debounce_t g_btn_debounce[CTRL_MAX_BUTTONS];

/* ============================================================
 * GPIO button table (only used on target, not in host tests)
 * ============================================================ */

#ifndef TEST_HOST

typedef struct {
    controller_button_t  button;
    int                  gpio_pin;
} gpio_button_map_t;

/*
 * GPIO pin assignments for controller buttons.
 *
 * PINMAP.md says all GPIOs are assigned. For the Lenker-Controller
 * we use a PCF8574 I2C port expander on the existing I2C bus
 * (GPIO8=SDA, GPIO9=SCL) for the wired option. This avoids
 * conflicting with existing pin assignments.
 *
 * For direct GPIO mode (prototype/DevKit), we repurpose the
 * existing button pins (GPIO6=MENU, GPIO7=SELECT, GPIO3=BACK,
 * GPIO0=BOOT) and add 2 more via the I2C expander.
 *
 * The actual pin numbers come from board_config.h.
 */
static const gpio_button_map_t s_gpio_buttons[] = {
    { CTRL_BTN_BACK,      BTN_BACK_PIN   },  /* GPIO3 */
    { CTRL_BTN_SELECT,    BTN_SELECT_PIN },  /* GPIO7 */
    { CTRL_BTN_HOME,      BTN_MENU_PIN   },  /* GPIO6 (repurposed) */
    { CTRL_BTN_NAV_UP,    BOOT_BTN_PIN   },  /* GPIO0 (repurposed) */
};

#define GPIO_BTN_COUNT  (sizeof(s_gpio_buttons) / sizeof(s_gpio_buttons[0]))

/* ISR queue: button GPIO -> processing task */
static QueueHandle_t s_gpio_queue = NULL;
static TaskHandle_t  s_gpio_task  = NULL;
static bool          s_gpio_running = false;

/* I2C poll task handle */
static TaskHandle_t  s_i2c_task    = NULL;
static bool          s_i2c_running  = false;

/* BLE task handle */
static TaskHandle_t  s_ble_task    = NULL;
static bool          s_ble_running  = false;

/* ============================================================
 * GPIO ISR handler
 * ============================================================ */

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

#define I2C_CONTROLLER_ADDR   0x42   /* Custom PCB I2C address */
#define I2C_POLL_INTERVAL_MS  100    /* Poll every 100 ms */
#define I2C_REG_BUTTON_MASK  0x00   /* Register: 2-byte button bitmask */

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
    uint16_t             mask;
    controller_button_t btn;
} s_i2c_btn_map[] = {
    { I2C_BIT_NAV_UP,     CTRL_BTN_NAV_UP     },
    { I2C_BIT_NAV_DOWN,   CTRL_BTN_NAV_DOWN   },
    { I2C_BIT_NAV_LEFT,   CTRL_BTN_NAV_LEFT   },
    { I2C_BIT_NAV_RIGHT,  CTRL_BTN_NAV_RIGHT  },
    { I2C_BIT_SELECT,     CTRL_BTN_SELECT     },
    { I2C_BIT_NAV_RIGHT,  CTRL_BTN_BACK       },
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
             I2C_CONTROLLER_ADDR, I2C_POLL_INTERVAL_MS);

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
            vTaskDelay(pdMS_TO_TICKS(I2C_POLL_INTERVAL_MS));
            continue;
        }

        uint16_t mask = (uint16_t)(buf[0] | (buf[1] << 8));

        /* Detect new presses (bits that are set now but weren't before) */
        uint16_t new_presses = mask & ~s_i2c_prev_mask;
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

        vTaskDelay(pdMS_TO_TICKS(I2C_POLL_INTERVAL_MS));
    }

    vTaskDelete(NULL);
}

/* ============================================================
 * BLE input task: NimBLE GATT client
 * ============================================================ */

/*
 * BLE GATT client for wireless controllers (Chigee CGRC Multi).
 *
 * Connection flow:
 *   1. Scan for controller advertisement (service UUID)
 *   2. Connect and discover controller service
 *   3. Subscribe to controller characteristic (notifications)
 *   4. Incoming notifications contain a 1-byte button mask
 *
 * The actual NimBLE client API is large; this implementation
 * provides the task skeleton and the notification handler. Full
 * BLE GATT client setup is deferred to the ble component integration.
 */

#define BLE_CONTROLLER_SERVICE_UUID  0xFFE0
#define BLE_CONTROLLER_CHAR_UUID     0xFFE1
#define BLE_SCAN_TIMEOUT_MS          10000

/* Button bit positions in the BLE notification byte */
#define BLE_BIT_NAV_UP       (1 << 0)
#define BLE_BIT_NAV_DOWN     (1 << 1)
#define BLE_BIT_NAV_LEFT     (1 << 2)
#define BLE_BIT_NAV_RIGHT   (1 << 3)
#define BLE_BIT_SELECT      (1 << 4)
#define BLE_BIT_BACK       (1 << 5)
#define BLE_BIT_HOME       (1 << 6)
#define BLE_BIT_VOL_UP     (1 << 7)

/* For 2-byte BLE notifications (volume down + media) */
#define BLE_BIT_VOL_DOWN    (1 << 0)  /* byte[1], bit 0 */
#define BLE_BIT_MEDIA_NEXT  (1 << 1)  /* byte[1], bit 1 */
#define BLE_BIT_MEDIA_PREV  (1 << 2)  /* byte[1], bit 2 */

static const struct {
    uint8_t             byte_idx;
    uint8_t             bit;
    controller_button_t btn;
} s_ble_btn_map[] = {
    { 0, BLE_BIT_NAV_UP,     CTRL_BTN_NAV_UP     },
    { 0, BLE_BIT_NAV_DOWN,   CTRL_BTN_NAV_DOWN   },
    { 0, BLE_BIT_NAV_LEFT,   CTRL_BTN_NAV_LEFT   },
    { 0, BLE_BIT_NAV_RIGHT,  CTRL_BTN_NAV_RIGHT  },
    { 0, BLE_BIT_SELECT,     CTRL_BTN_SELECT     },
    { 0, BLE_BIT_BACK,       CTRL_BTN_BACK       },
    { 0, BLE_BIT_HOME,       CTRL_BTN_HOME       },
    { 0, BLE_BIT_VOL_UP,     CTRL_BTN_VOL_UP     },
    { 1, BLE_BIT_VOL_DOWN,   CTRL_BTN_VOL_DOWN   },
    { 1, BLE_BIT_MEDIA_NEXT, CTRL_BTN_MEDIA_NEXT },
    { 1, BLE_BIT_MEDIA_PREV, CTRL_BTN_MEDIA_PREV },
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
        uint8_t cur_bit = (cur[s_ble_btn_map[i].byte_idx] & s_ble_btn_map[i].bit) ? 1 : 0;
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

static void ble_input_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "BLE controller client task started");

    while (s_ble_running) {
        /*
         * BLE GATT client connection management:
         * 1. If not connected, scan for controller
         * 2. On connect, discover service + subscribe to char
         * 3. Notifications arrive via hid_controller_ble_notify_handler()
         *
         * This is a simplified skeleton. Full implementation requires
         * NimBLE GATT client API calls (ble_gap_connect, ble_gattc_disc_all_svcs,
         * ble_gattc_write for CCCD subscription).
         */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
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
