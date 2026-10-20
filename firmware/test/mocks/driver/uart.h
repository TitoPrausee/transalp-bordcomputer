#ifndef _DRIVER_UART_H_
#define _DRIVER_UART_H_

/*
 * driver/uart.h — Mock for host-side unit tests.
 *
 * board_config.h defines GPS_UART_NUM as UART_NUM_1 (and
 * OBD_UART_NUM as UART_NUM_2). Those enum constants are expanded
 * at use-time, so defining them here as integer macros lets the
 * board_config.h pin definitions resolve on host without conflicts.
 */

#define UART_NUM_0  0
#define UART_NUM_1  1
#define UART_NUM_2  2

#endif /* _DRIVER_UART_H_ */
