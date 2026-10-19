/*
 * controller_mapping.c — Button-to-Event-Bus mapping table.
 *
 * Maps physical controller_button_t values to logical controller_event_id_t
 * events. On dispatch, the event is sent to the app_event_loop.
 *
 * The mapping is a compile-time constant table indexed by button ID.
 */

#include "controller_mapping.h"
#include "controller_input.h"

#ifdef TEST_HOST
#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#define ESP_LOGE(tag, fmt, ...)  (void)0
#define ESP_LOGW(tag, fmt, ...)  (void)0
#define ESP_LOGI(tag, fmt, ...)  (void)0
#endif

#include "esp_log.h"
#include <string.h>

#ifndef TEST_HOST
#include "app_event_loop.h"
#endif

static const char *TAG = "ctrl_map";

/* ============================================================
 * Static mapping table
 *
 * Indexed by controller_button_t. Each entry maps a physical button
 * to a logical event ID and a human-readable name.
 * ============================================================ */

const controller_map_entry_t g_controller_map[CTRL_MAX_BUTTONS] = {
    [CTRL_BTN_NAV_UP]     = { CTRL_BTN_NAV_UP,     CONTROLLER_NAV_UP,     "NAV_UP"     },
    [CTRL_BTN_NAV_DOWN]   = { CTRL_BTN_NAV_DOWN,   CONTROLLER_NAV_DOWN,   "NAV_DOWN"   },
    [CTRL_BTN_NAV_LEFT]   = { CTRL_BTN_NAV_LEFT,   CONTROLLER_NAV_LEFT,   "NAV_LEFT"   },
    [CTRL_BTN_NAV_RIGHT]  = { CTRL_BTN_NAV_RIGHT,  CONTROLLER_NAV_RIGHT,  "NAV_RIGHT"  },
    [CTRL_BTN_SELECT]     = { CTRL_BTN_SELECT,     CONTROLLER_BTN_SELECT, "SELECT"     },
    [CTRL_BTN_BACK]       = { CTRL_BTN_BACK,       CONTROLLER_BTN_BACK,   "BACK"       },
    [CTRL_BTN_HOME]       = { CTRL_BTN_HOME,       CONTROLLER_BTN_HOME,   "HOME"       },
    [CTRL_BTN_VOL_UP]     = { CTRL_BTN_VOL_UP,     CONTROLLER_VOL_UP,     "VOL_UP"     },
    [CTRL_BTN_VOL_DOWN]   = { CTRL_BTN_VOL_DOWN,   CONTROLLER_VOL_DOWN,   "VOL_DOWN"   },
    [CTRL_BTN_MEDIA_NEXT] = { CTRL_BTN_MEDIA_NEXT, CONTROLLER_MEDIA_NEXT, "MEDIA_NEXT" },
    [CTRL_BTN_MEDIA_PREV] = { CTRL_BTN_MEDIA_PREV, CONTROLLER_MEDIA_PREV, "MEDIA_PREV" },
};

const int g_controller_map_count = CTRL_BTN_COUNT;

/* ============================================================
 * API
 * ============================================================ */

void controller_mapping_init(void)
{
    ESP_LOGI(TAG, "Mapping table initialised (%d buttons)", g_controller_map_count);

    /* Verify mapping table integrity */
    for (int i = 0; i < CTRL_MAX_BUTTONS; i++) {
        if (g_controller_map[i].button != (controller_button_t)i) {
            ESP_LOGE(TAG, "Mapping table mismatch at index %d", i);
        }
    }
}

controller_event_id_t controller_mapping_lookup(controller_button_t button)
{
    if (button < 0 || button >= CTRL_MAX_BUTTONS) {
        return CONTROLLER_EVENT_COUNT;  /* Invalid/unmapped */
    }
    return g_controller_map[button].event;
}

const char *controller_mapping_name(controller_button_t button)
{
    if (button < 0 || button >= CTRL_MAX_BUTTONS) {
        return "UNKNOWN";
    }
    return g_controller_map[button].name;
}

void controller_mapping_dispatch(controller_button_t button)
{
    if (button < 0 || button >= CTRL_MAX_BUTTONS) {
        ESP_LOGW(TAG, "Dispatch: invalid button %d", button);
        return;
    }

    const controller_map_entry_t *entry = &g_controller_map[button];
    ESP_LOGI(TAG, "Dispatch: %s -> event %d", entry->name, entry->event);

#ifndef TEST_HOST
    /*
     * Dispatch onto the app event loop.
     *
     * Controller events are mapped to app_event_type_t values
     * starting at APP_EVENT_CONTROLLER_BASE (defined in app_event_loop.h).
     * The event ID is offset by the base so it lands in the
     * controller-specific range of the event type enum.
     */
    app_event_type_t ev_type = (app_event_type_t)(APP_EVENT_CONTROLLER_BASE + entry->event);

    /* Payload: the button ID as a uint8_t */
    uint8_t payload = (uint8_t)button;
    app_event_dispatch(ev_type, &payload, sizeof(payload));
#else
    /* In host test mode, we can't dispatch to the real event loop.
     * Tests verify the mapping via controller_mapping_lookup(). */
    (void)entry;
#endif
}
