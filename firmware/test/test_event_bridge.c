/*
 * test_event_bridge.c — Integration test for the event_bus bridge
 *
 * Verifies that event_bus_set(EVT_*) dispatches the corresponding
 * APP_EVENT_* through the app_event_loop subscriber system,
 * closing gap H1 from the architecture review.
 *
 * The test includes both event_bus.c and app_event_loop.c source
 * files directly.  Because the host mock does not spawn a real
 * FreeRTOS task for the event loop, the test manually drains the
 * internal event queue and invokes the subscriber dispatch logic.
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "event_bus.h"
#include "app_event_loop.h"

#include <string.h>

/* ============================================================
 * Mock ESP log for host build
 * ============================================================ */
#ifdef TEST_HOST
#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#define ESP_LOGE(tag, fmt, ...)  (void)0
#define ESP_LOGW(tag, fmt, ...)  (void)0
#define ESP_LOGI(tag, fmt, ...)  (void)0

/* Provide the three board_config.h constants that app_event_loop.c
 * needs, without pulling in the full board_config.h (which references
 * ESP-IDF driver constants unavailable in the host mock). */
#define _BOARD_CONFIG_H_
#define TASK_STACK_EVENT_LOOP  4096
#define TASK_PRIO_EVENT_LOOP   3
#define TASK_CORE_UI           0

/* Rename TAG to avoid duplicate symbol when including two .c files */
#define TAG TAG_event_bus
#include "../../components/utils/event_bus.c"
#undef TAG

#define TAG TAG_event_loop
#include "../../main/app_event_loop.c"
#undef TAG
#endif

/* ============================================================
 * Test subscriber: records the last received event type
 * ============================================================ */

static app_event_type_t s_last_received_type = APP_EVENT_MAX;
static int s_receive_count = 0;

static void test_subscriber(const app_event_t *ev, void *ctx)
{
    (void)ctx;
    s_last_received_type = ev->type;
    s_receive_count++;
}

/* ============================================================
 * Helper: drain one event from the internal queue and dispatch
 * to subscribers (mimics the event loop task body).
 * Returns true if an event was processed.
 * ============================================================ */

static bool drain_one_event(void)
{
    app_event_t ev;
    if (xQueueReceive(s_event_queue, &ev, 0) != pdTRUE) {
        return false;
    }

    xSemaphoreTake(s_sub_mutex, portMAX_DELAY);
    for (int i = 0; i < APP_EVENT_MAX_SUBSCRIBERS; i++) {
        if (!s_subs[i].active)
            continue;
        if (s_subs[i].type_mask & (1U << ev.type)) {
            subscriber_t cb_copy = s_subs[i];
            xSemaphoreGive(s_sub_mutex);
            cb_copy.callback(&ev, cb_copy.user_ctx);
            xSemaphoreTake(s_sub_mutex, portMAX_DELAY);
        }
    }
    xSemaphoreGive(s_sub_mutex);
    return true;
}

void setUp(void)
{
    s_last_received_type = APP_EVENT_MAX;
    s_receive_count = 0;

    /* Reset app_event_loop internal state.  The mock does not spawn
     * a real FreeRTOS task, so s_running stays true after the first
     * init and subsequent inits are no-ops.  We manually reset so
     * each test starts with a clean subscriber table and queue. */
    s_running = false;
    if (s_sub_mutex) {
        vSemaphoreDelete(s_sub_mutex);
        s_sub_mutex = NULL;
    }
    if (s_event_queue) {
        vQueueDelete(s_event_queue);
        s_event_queue = NULL;
    }
    memset(s_subs, 0, sizeof(s_subs));

    event_bus_init();
    app_event_loop_init();
    app_event_bridge_init();
}

void tearDown(void)
{
    /* Unregister bridge to avoid affecting other tests */
    event_bus_set_bridge_cb(NULL);
}

/* ============================================================
 * Test: EVT_BLE_CONNECTED bridges to APP_EVENT_BLE_CONNECTED
 * ============================================================ */
void test_bridge_ble_connected(void)
{
    app_event_subscribe((1U << APP_EVENT_BLE_CONNECTED),
                        test_subscriber, NULL);

    event_bus_set(EVT_BLE_CONNECTED);
    drain_one_event();

    TEST_ASSERT_EQUAL_INT(APP_EVENT_BLE_CONNECTED, s_last_received_type);
    TEST_ASSERT_EQUAL(1, s_receive_count);
}

/* ============================================================
 * Test: EVT_OTA_START bridges to APP_EVENT_OTA_START
 * ============================================================ */
void test_bridge_ota_start(void)
{
    app_event_subscribe((1U << APP_EVENT_OTA_START),
                        test_subscriber, NULL);

    event_bus_set(EVT_OTA_START);
    drain_one_event();

    TEST_ASSERT_EQUAL_INT(APP_EVENT_OTA_START, s_last_received_type);
}

/* ============================================================
 * Test: EVT_CRASH_DETECTED bridges to APP_EVENT_CRASH_DETECTED
 * ============================================================ */
void test_bridge_crash_detected(void)
{
    app_event_subscribe((1U << APP_EVENT_CRASH_DETECTED),
                        test_subscriber, NULL);

    event_bus_set(EVT_CRASH_DETECTED);
    drain_one_event();

    TEST_ASSERT_EQUAL_INT(APP_EVENT_CRASH_DETECTED, s_last_received_type);
}

/* ============================================================
 * Test: EVT_BATTERY_LOW bridges to APP_EVENT_BATTERY_LOW
 * ============================================================ */
void test_bridge_battery_low(void)
{
    app_event_subscribe((1U << APP_EVENT_BATTERY_LOW),
                        test_subscriber, NULL);

    event_bus_set(EVT_BATTERY_LOW);
    drain_one_event();

    TEST_ASSERT_EQUAL_INT(APP_EVENT_BATTERY_LOW, s_last_received_type);
}

/* ============================================================
 * Test: EVT_SDCARD_MOUNTED bridges to APP_EVENT_SDCARD_MOUNTED
 * ============================================================ */
void test_bridge_sdcard_mounted(void)
{
    app_event_subscribe((1U << APP_EVENT_SDCARD_MOUNTED),
                        test_subscriber, NULL);

    event_bus_set(EVT_SDCARD_MOUNTED);
    drain_one_event();

    TEST_ASSERT_EQUAL_INT(APP_EVENT_SDCARD_MOUNTED, s_last_received_type);
}

/* ============================================================
 * Test: EVT_GPS_FIX bridges to APP_EVENT_GPS_FIX_ACQUIRED
 * ============================================================ */
void test_bridge_gps_fix(void)
{
    app_event_subscribe((1U << APP_EVENT_GPS_FIX_ACQUIRED),
                        test_subscriber, NULL);

    event_bus_set(EVT_GPS_FIX);
    drain_one_event();

    TEST_ASSERT_EQUAL_INT(APP_EVENT_GPS_FIX_ACQUIRED, s_last_received_type);
}

/* ============================================================
 * Test: EVT_FUEL_LOW bridges to APP_EVENT_LOW_FUEL (name differs)
 * ============================================================ */
void test_bridge_fuel_low_name_mismatch(void)
{
    app_event_subscribe((1U << APP_EVENT_LOW_FUEL),
                        test_subscriber, NULL);

    event_bus_set(EVT_FUEL_LOW);
    drain_one_event();

    TEST_ASSERT_EQUAL_INT(APP_EVENT_LOW_FUEL, s_last_received_type);
}

/* ============================================================
 * Test: EVT_CARPLAY_CONNECTED bridges to APP_EVENT_CARPLAY_CONNECTED
 * ============================================================ */
void test_bridge_carplay_connected(void)
{
    app_event_subscribe((1U << APP_EVENT_CARPLAY_CONNECTED),
                        test_subscriber, NULL);

    event_bus_set(EVT_CARPLAY_CONNECTED);
    drain_one_event();

    TEST_ASSERT_EQUAL_INT(APP_EVENT_CARPLAY_CONNECTED, s_last_received_type);
}

/* ============================================================
 * Test: Multiple bits set dispatch multiple events
 * ============================================================ */
void test_bridge_multiple_bits(void)
{
    app_event_subscribe((1U << APP_EVENT_BLE_CONNECTED) |
                        (1U << APP_EVENT_BLE_DISCONNECTED),
                        test_subscriber, NULL);

    event_bus_set(EVT_BLE_CONNECTED | EVT_BLE_DISCONNECTED);

    /* Two events should have been enqueued */
    bool got1 = drain_one_event();
    bool got2 = drain_one_event();
    bool got3 = drain_one_event();

    TEST_ASSERT_TRUE(got1);
    TEST_ASSERT_TRUE(got2);
    TEST_ASSERT_FALSE(got3);
    TEST_ASSERT_EQUAL(2, s_receive_count);
}

/* ============================================================
 * Test: EVT_SENSOR_READY has no mapping — no event dispatched
 * ============================================================ */
void test_bridge_unmapped_event_no_dispatch(void)
{
    app_event_subscribe((1U << APP_EVENT_SENSOR_UPDATE),
                        test_subscriber, NULL);

    event_bus_set(EVT_SENSOR_READY);

    /* No event should be in the queue */
    bool got = drain_one_event();
    TEST_ASSERT_FALSE(got);
    TEST_ASSERT_EQUAL(0, s_receive_count);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_bridge_ble_connected);
    RUN_TEST(test_bridge_ota_start);
    RUN_TEST(test_bridge_crash_detected);
    RUN_TEST(test_bridge_battery_low);
    RUN_TEST(test_bridge_sdcard_mounted);
    RUN_TEST(test_bridge_gps_fix);
    RUN_TEST(test_bridge_fuel_low_name_mismatch);
    RUN_TEST(test_bridge_carplay_connected);
    RUN_TEST(test_bridge_multiple_bits);
    RUN_TEST(test_bridge_unmapped_event_no_dispatch);

    return UNITY_END();
}
