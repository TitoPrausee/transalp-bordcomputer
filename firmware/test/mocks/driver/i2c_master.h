#ifndef _DRIVER_I2C_MASTER_H_
#define _DRIVER_I2C_MASTER_H_

/*
 * Mock header for host-side unit tests.
 */

#include <stdint.h>
#include <stddef.h>

typedef int i2c_master_bus_handle_t;
typedef int i2c_master_dev_handle_t;
typedef int i2c_port_t;
typedef int gpio_num_t;

typedef struct {
    i2c_port_t i2c_port;
    int sda_io_num;
    int scl_io_num;
    int clk_source;
} i2c_master_bus_config_t;

typedef struct {
    int dev_addr_length;
    int device_address;
    int scl_speed_hz;
    int scl_speed;
} i2c_device_config_t;

static inline int i2c_new_master_bus(void *cfg, i2c_master_bus_handle_t *h) {
    (void)cfg; if (h) *h = 1; return 0;
}
static inline int i2c_master_bus_add_device(i2c_master_bus_handle_t h,
    i2c_device_config_t *cfg, i2c_master_dev_handle_t *dev) {
    (void)h; (void)cfg; if (dev) *dev = 1; return 0;
}
static inline int i2c_master_transmit(i2c_master_dev_handle_t d,
    const uint8_t *w, size_t wl, int t) {
    (void)d; (void)w; (void)wl; (void)t; return 0;
}
static inline int i2c_master_receive(i2c_master_dev_handle_t d,
    uint8_t *r, size_t rl, int t) {
    (void)d; if (r) *r = 0; (void)rl; (void)t; return 0;
}
static inline int i2c_master_transmit_receive(i2c_master_dev_handle_t d,
    const uint8_t *w, size_t wl, uint8_t *r, size_t rl, int t) {
    (void)d; (void)w; (void)wl; if (r) *r = 0; (void)rl; (void)t; return 0;
}

#endif /* _DRIVER_I2C_MASTER_H_ */
