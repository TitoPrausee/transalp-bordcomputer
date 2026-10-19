#ifndef _CONTROLLER_MAPPING_H_
#define _CONTROLLER_MAPPING_H_

#include <stdint.h>
#include <stdbool.h>
#include "controller_input.h"

/*
 * controller_mapping.h — Button-to-Event-Bus mapping table.
 *
 * Maps controller_button_t values to app_event_type_t events.
 * The mapping table is static (compile-time) and matches the
 * event types defined in app_event_loop.h.
 *
 * Since app_event_loop.h is in the main component, we define
 * controller event IDs here and map them to the corresponding
 * app_event_type_t values at dispatch time.
 */

/* ============================================================
 * Controller event types (mapped onto app_event_type_t)
 * ============================================================ */

/*
 * These are the app_event_type_t values that the controller
 * dispatches. They are added to the enum in app_event_loop.h.
 *
 * If APP_EVENT_CONTROLLER_BASE is not defined, these are
 * used as standalone identifiers in tests.
 */
typedef enum {
    CONTROLLER_NAV_UP      = 0,
    CONTROLLER_NAV_DOWN    = 1,
    CONTROLLER_NAV_LEFT    = 2,
    CONTROLLER_NAV_RIGHT   = 3,
    CONTROLLER_BTN_SELECT  = 4,
    CONTROLLER_BTN_BACK    = 5,
    CONTROLLER_BTN_HOME    = 6,
    CONTROLLER_VOL_UP      = 7,
    CONTROLLER_VOL_DOWN    = 8,
    CONTROLLER_MEDIA_NEXT  = 9,
    CONTROLLER_MEDIA_PREV  = 10,
    CONTROLLER_EVENT_COUNT,
} controller_event_id_t;

/* ============================================================
 * Mapping table entry
 * ============================================================ */

typedef struct {
    controller_button_t    button;    /* Physical button                */
    controller_event_id_t  event;     /* Logical event on the bus       */
    const char            *name;      /* Human-readable name (logging)  */
} controller_map_entry_t;

/*
 * The static button-to-event mapping table.
 * Indexed by controller_button_t, so the entry for
 * CTRL_BTN_NAV_UP is at g_controller_map[CTRL_BTN_NAV_UP].
 */
extern const controller_map_entry_t g_controller_map[CTRL_MAX_BUTTONS];

/* Number of valid entries in g_controller_map. */
extern const int g_controller_map_count;

/* ============================================================
 * API
 * ============================================================ */

/*
 * Initialise the mapping layer. Builds the lookup table.
 */
void controller_mapping_init(void);

/*
 * Dispatch a button press as an event on the event bus.
 * Called by controller_input_process_button() on a confirmed press.
 *
 *   button — the physical button that was pressed
 */
void controller_mapping_dispatch(controller_button_t button);

/*
 * Look up the event ID for a button without dispatching.
 * Returns CONTROLLER_EVENT_COUNT if the button is unmapped.
 */
controller_event_id_t controller_mapping_lookup(controller_button_t button);

/*
 * Look up the human-readable name for a button.
 */
const char *controller_mapping_name(controller_button_t button);

#endif /* _CONTROLLER_MAPPING_H_ */
