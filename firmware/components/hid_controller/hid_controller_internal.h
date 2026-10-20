#ifndef _HID_CONTROLLER_INTERNAL_H_
#define _HID_CONTROLLER_INTERNAL_H_

/*
 * hid_controller_internal.h — Internal definitions shared between
 * controller_input.c and hid_controller.c.
 *
 * NOT part of the public HID controller API. Do not include from
 * outside the hid_controller component.
 *
 * The transport start/stop functions are target-only: they configure
 * GPIO/I2C/BLE hardware and spawn FreeRTOS tasks. In host test mode
 * (TEST_HOST) they are not compiled.
 */

#include <stdint.h>
#include <stdbool.h>
#include "controller_input.h"

#ifndef TEST_HOST
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/i2c_master.h"
#include "board_config.h"

/* ============================================================
 * Transport start/stop (defined in controller_input.c)
 *
 * Each start function configures its transport and spawns the
 * corresponding input task. Returns ESP_OK on success.
 * ============================================================ */

/* GPIO: direct-wired buttons via interrupt + debounce task. */
esp_err_t controller_input_start_gpio(void);

/* I2C: custom PCB (ATmega328P/RP2040) polled at CTRL_I2C_POLL_MS. */
esp_err_t controller_input_start_i2c(void);

/* BLE: NimBLE GATT client for wireless controllers. */
esp_err_t controller_input_start_ble(void);

/* Stop individual transports (idempotent, safe to call when stopped). */
void controller_input_stop_gpio(void);
void controller_input_stop_i2c(void);
void controller_input_stop_ble(void);

/* ============================================================
 * BLE notification handler
 *
 * Called by the NimBLE host task when a controller notification
 * arrives. Parses the button mask and processes state changes.
 * ============================================================ */
void hid_controller_ble_notify_handler(const uint8_t *data, uint8_t len);

#endif /* !TEST_HOST */

#endif /* _HID_CONTROLLER_INTERNAL_H_ */
