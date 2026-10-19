/*
 * test_controller.c — Unit Tests fuer den Lenker-Controller (hid_controller)
 *
 * Testet:
 *   - Debounce-Logik (controller_input.c)
 *   - Button-Mapping (controller_mapping.c)
 *   - I2C-Mock (controller_input.c I2C button mask parsing)
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC. ESP-IDF/FreeRTOS-Primitives werden ueber POSIX-Mocks
 * nachgebildet.
 *
 * Compile:
 *   gcc -std=c11 -Wall -Wextra -DTEST_HOST \
 *     -I../components/hid_controller/include \
 *     -I../components/utils/include \
 *     -I../main \
 *     -I./mocks \
 *     test_controller.c \
 *     ../components/hid_controller/controller_input.c \
 *     ../components/hid_controller/controller_mapping.c \
 *     unity.c -o test_controller -lm -lpthread
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "controller_input.h"
#include "controller_mapping.h"

#include <string.h>

/* ============================================================
 * The source files are compiled separately and linked (not #include'd
 * directly), because g_btn_debounce is declared extern in the header.
 * ============================================================ */

/* ============================================================
 * Helpers
 * ============================================================ */

/* Simulate a button press at a given time (microseconds) */
static bool simulate_press(controller_button_t btn, int64_t time_us)
{
    /* Set the "current time" by pre-loading last_press_us for the
     * debounce check, then call process_button.
     *
     * In test mode, esp_timer_get_time() uses the real monotonic clock.
     * To test debounce timing, we manipulate g_btn_debounce directly. */

    controller_input_reset_button(btn);
    g_btn_debounce[btn].last_press_us = time_us - (CTRL_DEBOUNCE_MS * 1000 + 1);
    g_btn_debounce[btn].last_state = false;

    /* Now process: the "current" time is from the real clock, but
     * we've set last_press_us far enough in the past that it will pass. */
    return controller_input_process_button(btn, true);
}

/* ============================================================
 * setUp / tearDown
 * ============================================================ */

void setUp(void)
{
    controller_input_init();
    controller_mapping_init();
}

void tearDown(void)
{
    controller_input_reset_all();
}

/* ============================================================
 * Test 1: Debounce — Erster Tastendruck wird akzeptiert
 * ============================================================ */
void test_debounce_first_press_accepted(void)
{
    controller_input_reset_all();

    bool result = controller_input_process_button(CTRL_BTN_SELECT, true);
    TEST_ASSERT_TRUE(result);
}

/* ============================================================
 * Test 2: Debounce — Zweiter Tastendruck innerhalb 50ms wird blockiert
 * ============================================================ */
void test_debounce_rapid_press_rejected(void)
{
    controller_input_reset_all();

    /* First press: accepted */
    bool first = controller_input_process_button(CTRL_BTN_NAV_UP, true);
    TEST_ASSERT_TRUE(first);

    /* Second press immediately after: rejected (same state, no edge) */
    bool second = controller_input_process_button(CTRL_BTN_NAV_UP, true);
    TEST_ASSERT_FALSE(second);
}

/* ============================================================
 * Test 3: Debounce — Loslassen und neu druecken innerhalb 50ms
 * ============================================================ */
void test_debounce_release_and_press_within_window(void)
{
    controller_input_reset_all();

    /* First press: accepted */
    bool first = controller_input_process_button(CTRL_BTN_BACK, true);
    TEST_ASSERT_TRUE(first);

    /* Release: should not be forwarded (returns false) */
    bool release = controller_input_process_button(CTRL_BTN_BACK, false);
    TEST_ASSERT_FALSE(release);

    /* Immediate re-press: rejected by debounce (within 50ms) */
    bool repress = controller_input_process_button(CTRL_BTN_BACK, true);
    TEST_ASSERT_FALSE(repress);
}

/* ============================================================
 * Test 4: Debounce — would_accept bei unterschiedlichen Zeitpunkten
 * ============================================================ */
void test_debounce_would_accept_timing(void)
{
    controller_input_reset_all();

    int64_t base_time = 1000000;  /* 1 second in microseconds */

    /* Set last press to base_time */
    g_btn_debounce[CTRL_BTN_HOME].last_press_us = base_time;
    g_btn_debounce[CTRL_BTN_HOME].last_state = false;

    /* 30ms later: should NOT accept (within 50ms window) */
    TEST_ASSERT_FALSE(controller_input_would_accept(CTRL_BTN_HOME, base_time + 30000));

    /* 50ms later: should accept (exactly at boundary) */
    TEST_ASSERT_TRUE(controller_input_would_accept(CTRL_BTN_HOME, base_time + 50000));

    /* 100ms later: should accept */
    TEST_ASSERT_TRUE(controller_input_would_accept(CTRL_BTN_HOME, base_time + 100000));
}

/* ============================================================
 * Test 5: Mapping — Alle Buttons haben gueltige Event-IDs
 * ============================================================ */
void test_mapping_all_buttons_mapped(void)
{
    for (int i = 0; i < CTRL_BTN_COUNT; i++) {
        controller_event_id_t ev = controller_mapping_lookup((controller_button_t)i);
        TEST_ASSERT_NOT_EQUAL(CONTROLLER_EVENT_COUNT, ev);
        TEST_ASSERT_TRUE(ev >= 0 && ev < CONTROLLER_EVENT_COUNT);
    }
}

/* ============================================================
 * Test 6: Mapping — Jeder Button hat einen eindeutigen Event
 * ============================================================ */
void test_mapping_events_are_unique(void)
{
    controller_event_id_t events[CTRL_BTN_COUNT];

    for (int i = 0; i < CTRL_BTN_COUNT; i++) {
        events[i] = controller_mapping_lookup((controller_button_t)i);
    }

    for (int i = 0; i < CTRL_BTN_COUNT; i++) {
        for (int j = i + 1; j < CTRL_BTN_COUNT; j++) {
            TEST_ASSERT_NOT_EQUAL(events[i], events[j]);
        }
    }
}

/* ============================================================
 * Test 7: Mapping — Button-IDs stimmen mit Tabellen-Index ueberein
 * ============================================================ */
void test_mapping_table_integrity(void)
{
    for (int i = 0; i < CTRL_BTN_COUNT; i++) {
        TEST_ASSERT_EQUAL_INT(i, g_controller_map[i].button);
    }
}

/* ============================================================
 * Test 8: Mapping — Namen sind nicht NULL und nicht leer
 * ============================================================ */
void test_mapping_names_valid(void)
{
    for (int i = 0; i < CTRL_BTN_COUNT; i++) {
        const char *name = controller_mapping_name((controller_button_t)i);
        TEST_ASSERT_NOT_NULL(name);
        TEST_ASSERT_TRUE(strlen(name) > 0);
    }
}

/* ============================================================
 * Test 9: I2C-Mock — Button-Masken-Parstabelle ist vollstaendig
 *
 * Simuliert die I2C-Button-Masken-Logik: jedes Bit in der 16-bit
 * Maske entspricht einem Button. Wir testen, dass alle 11 Buttons
 * in der Maske abgedeckt sind.
 * ============================================================ */

/* I2C button mask bit definitions (mirror of controller_input.c) */
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

void test_i2c_mask_all_buttons_covered(void)
{
    /* Build the full button mask */
    uint16_t full_mask =
        I2C_BIT_NAV_UP | I2C_BIT_NAV_DOWN | I2C_BIT_NAV_LEFT |
        I2C_BIT_NAV_RIGHT | I2C_BIT_SELECT | I2C_BIT_BACK |
        I2C_BIT_HOME | I2C_BIT_VOL_UP | I2C_BIT_VOL_DOWN |
        I2C_BIT_MEDIA_NEXT | I2C_BIT_MEDIA_PREV;

    /* Count set bits = 11 buttons */
    int count = 0;
    for (int i = 0; i < 16; i++) {
        if (full_mask & (1 << i)) count++;
    }
    TEST_ASSERT_EQUAL_INT(11, count);

    /* Each bit should map to a unique button */
    uint16_t single_bit_masks[] = {
        I2C_BIT_NAV_UP, I2C_BIT_NAV_DOWN, I2C_BIT_NAV_LEFT,
        I2C_BIT_NAV_RIGHT, I2C_BIT_SELECT, I2C_BIT_BACK,
        I2C_BIT_HOME, I2C_BIT_VOL_UP, I2C_BIT_VOL_DOWN,
        I2C_BIT_MEDIA_NEXT, I2C_BIT_MEDIA_PREV,
    };

    for (int i = 0; i < 11; i++) {
        /* Each mask has exactly one bit set */
        TEST_ASSERT_EQUAL_UINT16(0, single_bit_masks[i] & (single_bit_masks[i] - 1));
    }
}

/* ============================================================
 * Test 10: I2C-Mock — Delta-Erkennung (neue Presses)
 * ============================================================ */
void test_i2c_mask_delta_detection(void)
{
    uint16_t prev = 0x0000;
    uint16_t curr = I2C_BIT_NAV_UP | I2C_BIT_SELECT;

    /* New presses = bits in curr that weren't in prev */
    uint16_t new_presses = curr & ~prev;
    TEST_ASSERT_EQUAL_UINT16(I2C_BIT_NAV_UP | I2C_BIT_SELECT, new_presses);

    /* No releases yet */
    uint16_t new_releases = ~curr & prev;
    TEST_ASSERT_EQUAL_UINT16(0, new_releases);

    /* Next tick: release NAV_UP, add HOME */
    prev = curr;
    curr = I2C_BIT_SELECT | I2C_BIT_HOME;

    new_presses = curr & ~prev;
    TEST_ASSERT_EQUAL_UINT16(I2C_BIT_HOME, new_presses);

    new_releases = ~curr & prev;
    TEST_ASSERT_EQUAL_UINT16(I2C_BIT_NAV_UP, new_releases);
}

/* ============================================================
 * Test 11: Ungueltige Button-ID wird abgewiesen
 * ============================================================ */
void test_invalid_button_rejected(void)
{
    bool result = controller_input_process_button(CTRL_BTN_NONE, true);
    TEST_ASSERT_FALSE(result);

    result = controller_input_process_button((controller_button_t)99, true);
    TEST_ASSERT_FALSE(result);

    controller_event_id_t ev = controller_mapping_lookup((controller_button_t)99);
    TEST_ASSERT_EQUAL_INT(CONTROLLER_EVENT_COUNT, ev);
}

/* ============================================================
 * Test 12: Mehrere verschiedene Buttons gleichzeitig
 * ============================================================ */
void test_multiple_different_buttons_accepted(void)
{
    /* Pressing different buttons should each be accepted (independent debounce) */
    bool r1 = controller_input_process_button(CTRL_BTN_NAV_UP, true);
    bool r2 = controller_input_process_button(CTRL_BTN_NAV_DOWN, true);
    bool r3 = controller_input_process_button(CTRL_BTN_VOL_UP, true);

    TEST_ASSERT_TRUE(r1);
    TEST_ASSERT_TRUE(r2);
    TEST_ASSERT_TRUE(r3);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    /* Debounce tests */
    RUN_TEST(test_debounce_first_press_accepted);
    RUN_TEST(test_debounce_rapid_press_rejected);
    RUN_TEST(test_debounce_release_and_press_within_window);
    RUN_TEST(test_debounce_would_accept_timing);

    /* Mapping tests */
    RUN_TEST(test_mapping_all_buttons_mapped);
    RUN_TEST(test_mapping_events_are_unique);
    RUN_TEST(test_mapping_table_integrity);
    RUN_TEST(test_mapping_names_valid);

    /* I2C mock tests */
    RUN_TEST(test_i2c_mask_all_buttons_covered);
    RUN_TEST(test_i2c_mask_delta_detection);

    /* Edge cases */
    RUN_TEST(test_invalid_button_rejected);
    RUN_TEST(test_multiple_different_buttons_accepted);

    return UNITY_END();
}
