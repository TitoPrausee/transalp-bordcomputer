/*
 * test_event_loop.c — Unit Tests fuer Event-Bus und Event-Loop
 *
 * Testet:
 *   - event_bus.c: init, set/clear bits, wait mit Timeout
 *   - Event-Bit-Konstanten (EVT_GPS_FIX etc.)
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC. FreeRTOS-Primitives werden ueber POSIX-Mocks
 * nachgebildet.
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "event_bus.h"

#include <string.h>

/* ============================================================
 * Include the actual event_bus.c implementation directly.
 * We mock esp_log to suppress output.
 * ============================================================ */
#ifdef TEST_HOST
#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#define ESP_LOGE(tag, fmt, ...)  (void)0
#define ESP_LOGW(tag, fmt, ...)  (void)0
#define ESP_LOGI(tag, fmt, ...)  (void)0

#include "../../components/utils/event_bus.c"
#endif

void setUp(void)
{
    /* Re-initialise the event bus before each test */
    event_bus_init();
}

void tearDown(void)
{
}

/* ============================================================
 * Test: Event-Bus Initialisierung
 * ============================================================ */
void test_event_bus_init_creates_group(void)
{
    TEST_ASSERT_NOT_NULL(g_event_group);
}

/* ============================================================
 * Test: Bit setzen und lesen
 * ============================================================ */
void test_event_bus_set_bit(void)
{
    event_bus_set(EVT_GPS_FIX);
    bool result = event_bus_wait(EVT_GPS_FIX, 100);
    TEST_ASSERT_TRUE(result);
}

/* ============================================================
 * Test: Bit loeschen
 * ============================================================ */
void test_event_bus_clear_bit(void)
{
    event_bus_set(EVT_CRASH_DETECTED);
    event_bus_clear(EVT_CRASH_DETECTED);

    bool result = event_bus_wait(EVT_CRASH_DETECTED, 50);
    TEST_ASSERT_FALSE(result);
}

/* ============================================================
 * Test: Mehrere Bits gleichzeitig setzen
 * ============================================================ */
void test_event_bus_set_multiple_bits(void)
{
    uint32_t mask = EVT_GPS_FIX | EVT_SENSOR_READY | EVT_SDCARD_MOUNTED;
    event_bus_set(mask);

    bool result = event_bus_wait(mask, 100);
    TEST_ASSERT_TRUE(result);
}

/* ============================================================
 * Test: Wait mit Timeout (kein Bit gesetzt)
 * ============================================================ */
void test_event_bus_wait_timeout(void)
{
    bool result = event_bus_wait(EVT_SHUTDOWN, 50);
    TEST_ASSERT_FALSE(result);
}

/* ============================================================
 * Test: Wait loescht Bits nach Lesen (clear-on-exit)
 * ============================================================ */
void test_event_bus_wait_clears_bits(void)
{
    event_bus_set(EVT_BLE_CONNECTED);

    bool first = event_bus_wait(EVT_BLE_CONNECTED, 100);
    TEST_ASSERT_TRUE(first);

    bool second = event_bus_wait(EVT_BLE_CONNECTED, 50);
    TEST_ASSERT_FALSE(second);
}

/* ============================================================
 * Test: Alle Event-Bit-Konstanten sind eindeutig (distinct bits)
 * ============================================================ */
void test_event_bits_are_unique(void)
{
    uint32_t bits[] = {
        EVT_GPS_FIX, EVT_GPS_LOST_FIX, EVT_SENSOR_READY,
        EVT_CRASH_DETECTED, EVT_BATTERY_LOW, EVT_BATTERY_CRITICAL,
        EVT_SDCARD_MOUNTED, EVT_SDCARD_ERROR, EVT_BLE_CONNECTED,
        EVT_BLE_DISCONNECTED, EVT_DISPLAY_READY, EVT_SHUTDOWN,
        EVT_OTA_START, EVT_OTA_DONE, EVT_OVERTEMPERATURE,
        EVT_FUEL_LOW, EVT_RPM_TIMEOUT
    };
    int n = sizeof(bits) / sizeof(bits[0]);

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            TEST_ASSERT_NOT_EQUAL(bits[i], bits[j]);
        }
    }
}

/* ============================================================
 * Test: Event-Bits sind Zweierpotenzen
 * ============================================================ */
void test_event_bits_are_powers_of_two(void)
{
    uint32_t bits[] = {
        EVT_GPS_FIX, EVT_GPS_LOST_FIX, EVT_SENSOR_READY,
        EVT_CRASH_DETECTED, EVT_BATTERY_LOW, EVT_BATTERY_CRITICAL,
        EVT_SDCARD_MOUNTED, EVT_SDCARD_ERROR, EVT_BLE_CONNECTED,
        EVT_BLE_DISCONNECTED, EVT_DISPLAY_READY, EVT_SHUTDOWN,
        EVT_OTA_START, EVT_OTA_DONE, EVT_OVERTEMPERATURE,
        EVT_FUEL_LOW, EVT_RPM_TIMEOUT
    };
    int n = sizeof(bits) / sizeof(bits[0]);

    for (int i = 0; i < n; i++) {
        /* A power of 2 has exactly one bit set: (x & (x-1)) == 0 */
        TEST_ASSERT_EQUAL_UINT32(0, bits[i] & (bits[i] - 1));
    }
}

/* ============================================================
 * Test: Event-Bus ohne Initialisierung (NULL-Handle)
 * ============================================================ */
void test_event_bus_null_handle_safe(void)
{
    /* Reset g_event_group to NULL to test the null-guard */
    EventGroupHandle_t saved = g_event_group;
    g_event_group = NULL;

    /* These should not crash */
    event_bus_set(EVT_GPS_FIX);
    event_bus_clear(EVT_GPS_FIX);
    bool result = event_bus_wait(EVT_GPS_FIX, 10);
    TEST_ASSERT_FALSE(result);

    /* Restore */
    g_event_group = saved;
}

/* ============================================================
 * Bridge callback tests
 * ============================================================ */

static uint32_t s_bridge_received_bits = 0;
static int s_bridge_call_count = 0;

static void test_bridge_cb(uint32_t bits)
{
    s_bridge_received_bits |= bits;
    s_bridge_call_count++;
}

void test_bridge_callback_invoked_on_set(void)
{
    s_bridge_received_bits = 0;
    s_bridge_call_count = 0;

    event_bus_set_bridge_cb(test_bridge_cb);
    event_bus_set(EVT_BLE_CONNECTED | EVT_OTA_START);

    TEST_ASSERT_EQUAL(1, s_bridge_call_count);
    TEST_ASSERT_EQUAL_UINT32(EVT_BLE_CONNECTED | EVT_OTA_START,
                             s_bridge_received_bits);

    /* Clean up */
    event_bus_set_bridge_cb(NULL);
}

void test_bridge_callback_cleared(void)
{
    s_bridge_call_count = 0;

    event_bus_set_bridge_cb(test_bridge_cb);
    event_bus_set_bridge_cb(NULL);
    event_bus_set(EVT_GPS_FIX);

    TEST_ASSERT_EQUAL(0, s_bridge_call_count);
}

void test_bridge_callback_null_by_default(void)
{
    /* No bridge registered — event_bus_set must not crash */
    event_bus_set_bridge_cb(NULL);
    event_bus_set(EVT_SHUTDOWN);

    /* Verify the bit was still set in the event group */
    bool result = event_bus_wait(EVT_SHUTDOWN, 100);
    TEST_ASSERT_TRUE(result);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_event_bus_init_creates_group);
    RUN_TEST(test_event_bus_set_bit);
    RUN_TEST(test_event_bus_clear_bit);
    RUN_TEST(test_event_bus_set_multiple_bits);
    RUN_TEST(test_event_bus_wait_timeout);
    RUN_TEST(test_event_bus_wait_clears_bits);
    RUN_TEST(test_event_bits_are_unique);
    RUN_TEST(test_event_bits_are_powers_of_two);
    RUN_TEST(test_event_bus_null_handle_safe);
    RUN_TEST(test_bridge_callback_invoked_on_set);
    RUN_TEST(test_bridge_callback_cleared);
    RUN_TEST(test_bridge_callback_null_by_default);

    return UNITY_END();
}
