#ifndef _HID_CONTROLLER_H_
#define _HID_CONTROLLER_H_

#include <stdint.h>
#include <stdbool.h>
#include "controller_input.h"
#include "controller_mapping.h"

/*
 * hid_controller.h — Aggregated public API for the Lenker-Controller component.
 *
 * This header bundles the input-reading, mapping, and transport layers
 * (GPIO / I2C / BLE) into a single init entry point.
 *
 * Typical flow:
 *   1. hid_controller_init()  — registers event-bus events, configures GPIOs
 *   2. hid_controller_start()  — spawns input tasks (GPIO IRQ, I2C poll, BLE)
 *   3. Button press -> debounce -> mapping -> app_event_dispatch()
 */

/* ============================================================
 * Controller input source (transport layer)
 * ============================================================ */

typedef enum {
    CONTROLLER_SRC_NONE = 0,
    CONTROLLER_SRC_GPIO,     /* Direct-wired buttons via GPIO interrupts       */
    CONTROLLER_SRC_I2C,      /* Custom PCB: ATmega328P/RP2040 as I2C slave    */
    CONTROLLER_SRC_BLE,      /* Wireless: NimBLE GATT client                  */
} controller_source_t;

/* ============================================================
 * Lifecycle
 * ============================================================ */

/*
 * Initialise the controller subsystem: builds the button map,
 * prepares debounce state, and registers event-bus event types.
 * Does NOT start tasks or configure hardware — call hid_controller_start().
 */
esp_err_t hid_controller_init(void);

/*
 * Configure hardware and start input tasks for the selected source(s).
 * Multiple sources can be active simultaneously (bitmask).
 * Returns ESP_OK if at least one source started successfully.
 */
esp_err_t hid_controller_start(uint8_t sources);

/*
 * Stop all controller input tasks. Safe to call from any task.
 */
void hid_controller_stop(void);

/* ============================================================
 * Convenience: inject a raw button event (for testing or simulator)
 * ============================================================ */

/*
 * Inject a button press as if it came from hardware. Applies debounce
 * and dispatches the mapped event-bus event. Used by the firmware
 * simulator and unit tests.
 */
void hid_controller_inject_button(controller_button_t button,
                                  bool pressed);

#endif /* _HID_CONTROLLER_H_ */
