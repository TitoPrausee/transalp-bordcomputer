/*
 * test_power_manager.c — Unit Tests fuer Power-Management
 *
 * Testet firmware/components/power/power_manager.c:
 *   - State-Transitions ACTIVE -> DIMMED -> SUSPENDED -> DEEP_SLEEP
 *   - power_set_state / power_get_state
 *   - Backlight set/get
 *   - Estimated current per state
 *   - Callback-System
 *   - Inaktivitaets-Tracking
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC. ESP-IDF-Abhaengigkeiten werden durch Mocks ersetzt.
 *
 * Hinweis: power_manager.c hat viele Hardware-Abhaengigkeiten (LEDC, GPIO,
 * RTC, esp_sleep). Fuer Host-Tests testen wir nur die State-Machine-Logik
 * und die nicht-hardwareabhaengigen Funktionen.
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "power.h"

#include <string.h>
#include <stdint.h>

/* ============================================================
 * Mock ESP-IDF dependencies for host build
 * ============================================================ */
#ifdef TEST_HOST

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#define ESP_LOGE(tag, fmt, ...)  (void)0
#define ESP_LOGW(tag, fmt, ...)  (void)0
#define ESP_LOGI(tag, fmt, ...)  (void)0

/* Mock esp_timer.h (already in freertos_mock, but ensure available) */
#ifndef esp_timer_get_time
static inline int64_t esp_timer_get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)(ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL);
}
#endif

/* Mock esp_sleep.h */
typedef int esp_sleep_wakeup_cause_t;
#define ESP_SLEEP_WAKEUP_UNDEFINED  0
#define ESP_SLEEP_WAKEUP_TIMER      1
#define ESP_SLEEP_WAKEUP_GPIO       2
#define ESP_SLEEP_WAKEUP_UART       3
#define ESP_SLEEP_WAKEUP_TOUCHPAD   4
#define ESP_SLEEP_WAKEUP_ULP        5
static inline esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause(void) {
    return ESP_SLEEP_WAKEUP_UNDEFINED;
}
static inline void esp_sleep_enable_timer_wakeup(uint64_t t) { (void)t; }
static inline void esp_sleep_enable_ext0_wakeup(int pin, int level) { (void)pin; (void)level; }
static inline void esp_sleep_enable_uart_wakeup(int uart_num) { (void)uart_num; }
static inline void esp_deep_sleep_start(void) { /* no-op in tests */ }

/* Mock driver/rtc_io.h */
typedef int gpio_num_t;
#define GPIO_NUM_5  5
#define GPIO_NUM_6  6
static inline bool rtc_gpio_is_valid_gpio(int pin) { (void)pin; return false; }
static inline void rtc_gpio_isolate(int pin) { (void)pin; }

/* Mock driver/ledc.h */
typedef int ledc_timer_t;
typedef int ledc_channel_t;
typedef int ledc_mode_t;
typedef int ledc_timer_bit_t;
typedef int ledc_intr_type_t;
typedef int ledc_clk_cfg_t;
#define LEDC_TIMER_1          1
#define LEDC_LOW_SPEED_MODE   0
#define LEDC_CHANNEL_1        1
#define LEDC_TIMER_10_BIT     10
#define LEDC_AUTO_CLK         0
#define LEDC_INTR_DISABLE     0
typedef struct {
    int speed_mode;
    int timer_num;
    int duty_resolution;
    int freq_hz;
    int clk_cfg;
} ledc_timer_config_t;
typedef struct {
    int speed_mode;
    int channel;
    int timer_sel;
    int intr_type;
    int gpio_num;
    uint32_t duty;
    int hpoint;
} ledc_channel_config_t;
static inline esp_err_t ledc_timer_config(const ledc_timer_config_t *c) { (void)c; return ESP_OK; }
static inline esp_err_t ledc_channel_config(const ledc_channel_config_t *c) { (void)c; return ESP_OK; }
static inline esp_err_t ledc_set_duty(int mode, int ch, uint32_t duty) { (void)mode; (void)ch; (void)duty; return ESP_OK; }
static inline esp_err_t ledc_update_duty(int mode, int ch) { (void)mode; (void)ch; return ESP_OK; }

/* Mock driver/gpio.h */
typedef struct {
    uint64_t pin_bit_mask;
    int mode;
    int pull_up_en;
    int pull_down_en;
    int intr_type;
} gpio_config_t;
#define GPIO_MODE_OUTPUT 2
#define GPIO_PULLUP_DISABLE 0
#define GPIO_PULLDOWN_DISABLE 0
#define GPIO_INTR_DISABLE 0
static inline esp_err_t gpio_config(const gpio_config_t *cfg) { (void)cfg; return ESP_OK; }
static inline esp_err_t gpio_set_level(int pin, int level) { (void)pin; (void)level; return ESP_OK; }

/* Mock driver/uart.h */
#define GPS_UART_NUM 1

/* Mock board_config.h */
#define GPS_POWER_EN_PIN  5
#define BOOST_EN_PIN      6

/* Mock power.c functions (referenced by power_manager.c) */
void power_adc_init(uint8_t filter_size) { (void)filter_size; }

/* Forward-declare the types and functions we need to mock */
typedef int battery_status_t;
static battery_status_t s_mock_battery_status = 0;  /* BATTERY_STATUS_NORMAL */
static float s_mock_voltage = 12.6f;
static uint8_t s_mock_battery_pct = 85;
static bool s_mock_generator = false;

/* These are defined in power.c, but we provide stubs for the test */
float power_get_voltage(void) { return s_mock_voltage; }
uint8_t power_get_battery_pct(void) { return s_mock_battery_pct; }
float power_get_raw_voltage(void) { return s_mock_voltage; }
battery_status_t power_get_battery_status(void) { return s_mock_battery_status; }
bool power_is_generator_running(void) { return s_mock_generator; }
uint16_t power_get_charge_cycles(void) { return 0; }
uint16_t power_get_discharge_cycles(void) { return 0; }
float power_get_battery_voltage(void) { return s_mock_voltage; }
uint8_t power_get_battery_percent(void) { return s_mock_battery_pct; }

/* The mock battery_status enum values must match power.h */
#define BATTERY_STATUS_NORMAL    0
#define BATTERY_STATUS_CHARGING  1
#define BATTERY_STATUS_LOW       2
#define BATTERY_STATUS_CRITICAL  3
#define BATTERY_STATUS_OVERVOLT  4

/* Include the actual power_manager.c implementation directly */
#include "../../components/power/power_manager.c"

#endif /* TEST_HOST */

void setUp(void)
{
    /* Initialize power with defaults for each test.
     * We need to reset s_initialized to allow re-init.
     * Since s_initialized is static, we use a trick:
     * power_init_with_config(NULL) will set it up. */
    s_initialized = false;
    s_state = POWER_STATE_ACTIVE;
    s_backlight_pct = 100;
    s_gps_enabled = true;
    s_ble_enabled = true;
    s_callback_count = 0;
    s_last_activity_us = esp_timer_get_time();
    s_last_state_change = esp_timer_get_time();

    /* Create event group if needed */
    if (g_power_event_group == NULL) {
        g_power_event_group = xEventGroupCreate();
    }
}

void tearDown(void)
{
}

/* ============================================================
 * Test: Initial state is ACTIVE
 * ============================================================ */

void test_power_initial_state_active(void)
{
    power_init();
    TEST_ASSERT_EQUAL(POWER_STATE_ACTIVE, power_get_state());
}

/* ============================================================
 * Test: State transitions
 * ============================================================ */

void test_power_set_state_dimmed(void)
{
    power_init();
    esp_err_t err = power_set_state(POWER_STATE_DIMMED);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(POWER_STATE_DIMMED, power_get_state());
}

void test_power_set_state_suspended(void)
{
    power_init();
    power_set_state(POWER_STATE_DIMMED);
    esp_err_t err = power_set_state(POWER_STATE_SUSPENDED);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(POWER_STATE_SUSPENDED, power_get_state());
}

void test_power_set_state_active_back(void)
{
    power_init();
    power_set_state(POWER_STATE_DIMMED);
    power_set_state(POWER_STATE_SUSPENDED);
    esp_err_t err = power_set_state(POWER_STATE_ACTIVE);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(POWER_STATE_ACTIVE, power_get_state());
}

void test_power_set_state_invalid_returns_error(void)
{
    power_init();
    esp_err_t err = power_set_state((power_state_t)99);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

/* ============================================================
 * Test: Full transition chain ACTIVE -> DIMMED -> SUSPENDED -> DEEP_SLEEP
 * ============================================================ */

void test_power_full_transition_chain(void)
{
    power_init();
    TEST_ASSERT_EQUAL(POWER_STATE_ACTIVE, power_get_state());

    power_set_state(POWER_STATE_DIMMED);
    TEST_ASSERT_EQUAL(POWER_STATE_DIMMED, power_get_state());

    power_set_state(POWER_STATE_SUSPENDED);
    TEST_ASSERT_EQUAL(POWER_STATE_SUSPENDED, power_get_state());

    /* DEEP_SLEEP calls esp_deep_sleep_start which is a no-op in tests,
     * but transition_state is called first */
    power_set_state(POWER_STATE_DEEP_SLEEP);
    /* State should be DEEP_SLEEP (transition happens before sleep) */
    TEST_ASSERT_EQUAL(POWER_STATE_DEEP_SLEEP, power_get_state());
}

/* ============================================================
 * Test: Backlight
 * ============================================================ */

void test_power_set_backlight(void)
{
    power_init();
    power_set_backlight(50);
    TEST_ASSERT_EQUAL_UINT8(50, power_get_backlight());
}

void test_power_set_backlight_clamped(void)
{
    power_init();
    power_set_backlight(150);
    TEST_ASSERT_EQUAL_UINT8(100, power_get_backlight());

    power_set_backlight(0);
    TEST_ASSERT_EQUAL_UINT8(0, power_get_backlight());
}

void test_power_backlight_on_state_transition(void)
{
    power_init();

    /* ACTIVE -> DIMMED should dim backlight */
    power_set_state(POWER_STATE_DIMMED);
    TEST_ASSERT_EQUAL_UINT8(15, power_get_backlight());  /* backlight_dimmed default */

    /* DIMMED -> SUSPENDED should turn off backlight */
    power_set_state(POWER_STATE_SUSPENDED);
    TEST_ASSERT_EQUAL_UINT8(0, power_get_backlight());

    /* SUSPENDED -> ACTIVE should restore full backlight */
    power_set_state(POWER_STATE_ACTIVE);
    TEST_ASSERT_EQUAL_UINT8(100, power_get_backlight());  /* backlight_active default */
}

/* ============================================================
 * Test: Estimated current per state
 * ============================================================ */

void test_power_estimated_current_active(void)
{
    power_init();
    power_set_state(POWER_STATE_ACTIVE);
    TEST_ASSERT_EQUAL_UINT16(190, power_get_estimated_current_ma());
}

void test_power_estimated_current_dimmed(void)
{
    power_init();
    power_set_state(POWER_STATE_DIMMED);
    TEST_ASSERT_EQUAL_UINT16(145, power_get_estimated_current_ma());
}

void test_power_estimated_current_suspended(void)
{
    power_init();
    power_set_state(POWER_STATE_SUSPENDED);
    TEST_ASSERT_EQUAL_UINT16(35, power_get_estimated_current_ma());
}

void test_power_estimated_current_deep_sleep(void)
{
    power_init();
    power_set_state(POWER_STATE_DEEP_SLEEP);
    TEST_ASSERT_EQUAL_UINT16(7, power_get_estimated_current_ma());
}

/* ============================================================
 * Test: GPS power gating on state transitions
 * ============================================================ */

void test_power_gps_disabled_in_suspended(void)
{
    power_init();
    TEST_ASSERT_TRUE(power_is_gps_enabled());

    power_set_state(POWER_STATE_SUSPENDED);
    TEST_ASSERT_FALSE(power_is_gps_enabled());
}

void test_power_gps_reenabled_in_active(void)
{
    power_init();
    power_set_state(POWER_STATE_SUSPENDED);
    TEST_ASSERT_FALSE(power_is_gps_enabled());

    power_set_state(POWER_STATE_ACTIVE);
    TEST_ASSERT_TRUE(power_is_gps_enabled());
}

/* ============================================================
 * Test: BLE power gating in DEEP_SLEEP
 * ============================================================ */

void test_power_ble_disabled_in_deep_sleep(void)
{
    power_init();
    TEST_ASSERT_TRUE(power_is_ble_enabled());

    power_set_state(POWER_STATE_DEEP_SLEEP);
    TEST_ASSERT_FALSE(power_is_ble_enabled());
}

/* ============================================================
 * Test: Callback system
 * ============================================================ */

static int s_callback_count = 0;
static power_state_t s_callback_old_state = POWER_STATE_ACTIVE;
static power_state_t s_callback_new_state = POWER_STATE_ACTIVE;

static void test_callback(const power_event_t *event)
{
    if (event->type == POWER_EVENT_STATE_CHANGED) {
        s_callback_count++;
        s_callback_old_state = event->old_state;
        s_callback_new_state = event->new_state;
    }
}

void test_power_callback_fires_on_transition(void)
{
    power_init();
    s_callback_count = 0;

    esp_err_t err = power_register_callback(test_callback);
    TEST_ASSERT_EQUAL(ESP_OK, err);

    power_set_state(POWER_STATE_DIMMED);
    TEST_ASSERT_EQUAL(1, s_callback_count);
    TEST_ASSERT_EQUAL(POWER_STATE_ACTIVE, s_callback_old_state);
    TEST_ASSERT_EQUAL(POWER_STATE_DIMMED, s_callback_new_state);

    power_unregister_callback(test_callback);
}

void test_power_callback_max_limit(void)
{
    power_init();

    /* Register 4 callbacks (max) */
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL(ESP_OK, power_register_callback(test_callback));
    }

    /* 5th should fail */
    esp_err_t err = power_register_callback(test_callback);
    TEST_ASSERT_EQUAL(ESP_ERR_NO_MEM, err);
}

/* ============================================================
 * Test: power_reset_inactivity wakes from DIMMED/SUSPENDED
 * ============================================================ */

void test_power_reset_inactivity_wakes_to_active(void)
{
    power_init();
    power_set_state(POWER_STATE_DIMMED);
    TEST_ASSERT_EQUAL(POWER_STATE_DIMMED, power_get_state());

    power_reset_inactivity();
    TEST_ASSERT_EQUAL(POWER_STATE_ACTIVE, power_get_state());
}

void test_power_reset_inactivity_from_suspended(void)
{
    power_init();
    power_set_state(POWER_STATE_SUSPENDED);
    TEST_ASSERT_EQUAL(POWER_STATE_SUSPENDED, power_get_state());

    power_reset_inactivity();
    TEST_ASSERT_EQUAL(POWER_STATE_ACTIVE, power_get_state());
}

/* ============================================================
 * Test: Config set/get
 * ============================================================ */

void test_power_config_set_get(void)
{
    power_init();

    power_config_t cfg = {
        .dim_timeout_s = 10,
        .suspend_timeout_s = 60,
        .deep_sleep_timeout_s = 300,
        .vwarn_low = 11.8f,
        .vwarn_critical = 11.2f,
        .vwarn_overvoltage = 14.5f,
        .vgen_threshold = 13.6f,
        .backlight_active = 90,
        .backlight_dimmed = 10,
        .measure_interval_ms = 250,
        .filter_size = 8,
    };

    power_set_config(&cfg);

    power_config_t out;
    power_get_config(&out);

    TEST_ASSERT_EQUAL_UINT32(10, out.dim_timeout_s);
    TEST_ASSERT_EQUAL_UINT32(60, out.suspend_timeout_s);
    TEST_ASSERT_EQUAL_UINT32(300, out.deep_sleep_timeout_s);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 11.8f, out.vwarn_low);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 11.2f, out.vwarn_critical);
    TEST_ASSERT_EQUAL_UINT8(90, out.backlight_active);
    TEST_ASSERT_EQUAL_UINT8(10, out.backlight_dimmed);
    TEST_ASSERT_EQUAL_UINT32(250, out.measure_interval_ms);
    TEST_ASSERT_EQUAL_UINT8(8, out.filter_size);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_power_initial_state_active);

    RUN_TEST(test_power_set_state_dimmed);
    RUN_TEST(test_power_set_state_suspended);
    RUN_TEST(test_power_set_state_active_back);
    RUN_TEST(test_power_set_state_invalid_returns_error);

    RUN_TEST(test_power_full_transition_chain);

    RUN_TEST(test_power_set_backlight);
    RUN_TEST(test_power_set_backlight_clamped);
    RUN_TEST(test_power_backlight_on_state_transition);

    RUN_TEST(test_power_estimated_current_active);
    RUN_TEST(test_power_estimated_current_dimmed);
    RUN_TEST(test_power_estimated_current_suspended);
    RUN_TEST(test_power_estimated_current_deep_sleep);

    RUN_TEST(test_power_gps_disabled_in_suspended);
    RUN_TEST(test_power_gps_reenabled_in_active);
    RUN_TEST(test_power_ble_disabled_in_deep_sleep);

    RUN_TEST(test_power_callback_fires_on_transition);
    RUN_TEST(test_power_callback_max_limit);

    RUN_TEST(test_power_reset_inactivity_wakes_to_active);
    RUN_TEST(test_power_reset_inactivity_from_suspended);

    RUN_TEST(test_power_config_set_get);

    return UNITY_END();
}
