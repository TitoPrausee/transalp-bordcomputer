#ifndef _CONTROLLER_INPUT_H_
#define _CONTROLLER_INPUT_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * controller_input.h — Input types and debounce logic for the Lenker-Controller.
 *
 * Button identifiers are transport-agnostic. Each transport layer
 * (GPIO, I2C, BLE) translates its raw input into these IDs, then
 * calls controller_input_process_button() which handles debouncing
 * and forwards confirmed presses to the mapping layer.
 */

/* ============================================================
 * Button identifiers
 * ============================================================ */

typedef enum {
    CTRL_BTN_NONE = -1,
    CTRL_BTN_NAV_UP    = 0,
    CTRL_BTN_NAV_DOWN  = 1,
    CTRL_BTN_NAV_LEFT  = 2,
    CTRL_BTN_NAV_RIGHT = 3,
    CTRL_BTN_SELECT    = 4,
    CTRL_BTN_BACK      = 5,
    CTRL_BTN_HOME      = 6,
    CTRL_BTN_VOL_UP    = 7,
    CTRL_BTN_VOL_DOWN  = 8,
    CTRL_BTN_MEDIA_NEXT  = 9,
    CTRL_BTN_MEDIA_PREV  = 10,
    CTRL_BTN_COUNT       = 11,
} controller_button_t;

/* ============================================================
 * Debounce configuration
 * ============================================================ */

/* Minimum time between two presses of the same button (milliseconds). */
#define CTRL_DEBOUNCE_MS       50

/* Maximum buttons tracked simultaneously. */
#define CTRL_MAX_BUTTONS       CTRL_BTN_COUNT

/* ============================================================
 * Debounce state (per-button)
 * ============================================================ */

typedef struct {
    int64_t  last_press_us;  /* esp_timer_get_time() of last accepted press */
    bool     last_state;     /* Last raw state (for edge detection)          */
} button_debounce_t;

/*
 * Per-button debounce table. Indexed by controller_button_t.
 * Initialised to zeros by controller_input_init().
 */
extern button_debounce_t g_btn_debounce[CTRL_MAX_BUTTONS];

/* ============================================================
 * API
 * ============================================================ */

/*
 * Initialise the input layer: zero the debounce table.
 * Call once before processing any button events.
 */
void controller_input_init(void);

/*
 * Process a raw button state change. Applies the debounce filter.
 * On a confirmed rising edge (press), calls controller_mapping_dispatch().
 *
 *   button  — which button changed
 *   pressed — true = pressed (active), false = released
 *
 * Returns true if the press was accepted (passed debounce), false if
 * rejected (duplicate within debounce window).
 */
bool controller_input_process_button(controller_button_t button, bool pressed);

/*
 * Check whether a button press would pass the debounce filter
 * without actually recording it. Useful for testing.
 */
bool controller_input_would_accept(controller_button_t button, int64_t now_us);

/*
 * Reset the debounce state for a single button (for testing).
 */
void controller_input_reset_button(controller_button_t button);

/*
 * Reset all debounce state (for testing).
 */
void controller_input_reset_all(void);

#endif /* _CONTROLLER_INPUT_H_ */
