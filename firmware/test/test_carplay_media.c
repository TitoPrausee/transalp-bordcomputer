/*
 * test_carplay_media.c — Unit tests for the Lenker-Controller -> CarPlay
 * media-command mapping (gap C4 / FU-4, D2-1 / D2-4).
 *
 * Verifies that carplay_media_cmd_from_event() maps the controller media
 * events to the command bytes expected by the CarPlay display's
 * CHR_CP_MEDIA_CTRL (0x9104), and returns 0 (no mapping) for everything
 * else — including the volume events, which have no display protocol.
 *
 * Runs on the host PC with GCC; FreeRTOS/ESP-IDF primitives are mocked.
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "app_event_loop.h"
#include "../components/carplay_bridge/carplay_uuids.h"
#include "../components/carplay_bridge/carplay_media.h"

#include <stdint.h>

/* Include the mapping implementation directly (host-compilable, no NimBLE). */
#ifdef TEST_HOST
#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#define ESP_LOGE(tag, fmt, ...)  (void)0
#define ESP_LOGW(tag, fmt, ...)  (void)0
#define ESP_LOGI(tag, fmt, ...)  (void)0
#include "../components/carplay_bridge/carplay_media.c"
#endif

void setUp(void)
{
}

void tearDown(void)
{
}

/* ============================================================
 * Test: MEDIA_NEXT maps to CARPLAY_MEDIA_CMD_NEXT (0x02)
 * ============================================================ */

void test_media_next_maps_to_next_cmd(void)
{
    TEST_ASSERT_EQUAL_UINT8(CARPLAY_MEDIA_CMD_NEXT,
                            carplay_media_cmd_from_event(APP_EVENT_CTRL_MEDIA_NEXT));
    TEST_ASSERT_EQUAL_UINT8(0x02,
                            carplay_media_cmd_from_event(APP_EVENT_CTRL_MEDIA_NEXT));
}

/* ============================================================
 * Test: MEDIA_PREV maps to CARPLAY_MEDIA_CMD_PREV (0x03)
 * ============================================================ */

void test_media_prev_maps_to_prev_cmd(void)
{
    TEST_ASSERT_EQUAL_UINT8(CARPLAY_MEDIA_CMD_PREV,
                            carplay_media_cmd_from_event(APP_EVENT_CTRL_MEDIA_PREV));
    TEST_ASSERT_EQUAL_UINT8(0x03,
                            carplay_media_cmd_from_event(APP_EVENT_CTRL_MEDIA_PREV));
}

/* ============================================================
 * Test: volume up/down have no display protocol -> 0
 * (enum carplay_media_cmd defines no volume command)
 * ============================================================ */

void test_volume_events_unmapped(void)
{
    TEST_ASSERT_EQUAL_UINT8(0,
                            carplay_media_cmd_from_event(APP_EVENT_CTRL_VOL_UP));
    TEST_ASSERT_EQUAL_UINT8(0,
                            carplay_media_cmd_from_event(APP_EVENT_CTRL_VOL_DOWN));
}

/* ============================================================
 * Test: non-media controller events are unmapped -> 0
 * ============================================================ */

void test_non_media_controller_events_unmapped(void)
{
    TEST_ASSERT_EQUAL_UINT8(0,
                            carplay_media_cmd_from_event(APP_EVENT_CTRL_NAV_UP));
    TEST_ASSERT_EQUAL_UINT8(0,
                            carplay_media_cmd_from_event(APP_EVENT_CTRL_NAV_DOWN));
    TEST_ASSERT_EQUAL_UINT8(0,
                            carplay_media_cmd_from_event(APP_EVENT_CTRL_SELECT));
    TEST_ASSERT_EQUAL_UINT8(0,
                            carplay_media_cmd_from_event(APP_EVENT_CTRL_BACK));
    TEST_ASSERT_EQUAL_UINT8(0,
                            carplay_media_cmd_from_event(APP_EVENT_CTRL_HOME));
}

/* ============================================================
 * Test: non-controller system events are unmapped -> 0
 * ============================================================ */

void test_system_events_unmapped(void)
{
    TEST_ASSERT_EQUAL_UINT8(0,
                            carplay_media_cmd_from_event(APP_EVENT_GPS_UPDATE));
    TEST_ASSERT_EQUAL_UINT8(0,
                            carplay_media_cmd_from_event(APP_EVENT_SENSOR_UPDATE));
    TEST_ASSERT_EQUAL_UINT8(0,
                            carplay_media_cmd_from_event(APP_EVENT_BLE_CONNECTED));
    TEST_ASSERT_EQUAL_UINT8(0,
                            carplay_media_cmd_from_event(APP_EVENT_CARPLAY_CONNECTED));
}

/* ============================================================
 * Test: the two mapped commands are distinct (NEXT != PREV)
 * Guards against an accidental duplicate-enum-value mapping.
 * ============================================================ */

void test_next_and_prev_commands_distinct(void)
{
    uint8_t next = carplay_media_cmd_from_event(APP_EVENT_CTRL_MEDIA_NEXT);
    uint8_t prev = carplay_media_cmd_from_event(APP_EVENT_CTRL_MEDIA_PREV);
    TEST_ASSERT_NOT_EQUAL(next, prev);
}

/* ============================================================
 * Test Runner
 * ============================================================ */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_media_next_maps_to_next_cmd);
    RUN_TEST(test_media_prev_maps_to_prev_cmd);
    RUN_TEST(test_volume_events_unmapped);
    RUN_TEST(test_non_media_controller_events_unmapped);
    RUN_TEST(test_system_events_unmapped);
    RUN_TEST(test_next_and_prev_commands_distinct);

    return UNITY_END();
}
