#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

/*
 * board_config.h — Pin-Definitionen und Hardware-Konstanten
 *
 * Alle Werte aus docs/PINMAP.md und docs/PCB-LAYOUT.md.
 * Bei Pin-Aenderungen diese Datei anpassen und PINMAP.md nachziehen.
 */

#include <stdint.h>

/* ============================================================
 * UART
 * ============================================================ */

/* GPS (NEO-M9N) */
#define GPS_UART_NUM        UART_NUM_1
#define GPS_UART_RX_PIN     44      /* NEO-M9N TX -> ESP RX */
#define GPS_UART_TX_PIN     43      /* ESP TX -> NEO-M9N RX (Konfig) */
#define GPS_UART_BAUD       38400   /* Default; auf 115200 per UBX setzen */
#define GPS_PPS_PIN         1       /* 1PPS Zeitsync (IRQ) */

/* OBD-II (falls K-Line direkt, nicht BT) */
#define OBD_UART_NUM        UART_NUM_2
#define OBD_UART_RX_PIN     18
#define OBD_UART_TX_PIN     17
#define OBD_UART_BAUD       10400   /* OBD-II K-Line Default */

/* ============================================================
 * I2C (Sensoren)
 * ============================================================ */

#define I2C_MASTER_SDA_IO   8
#define I2C_MASTER_SCL_IO   9
#define I2C_MASTER_NUM      0
#define I2C_MASTER_FREQ_HZ  400000  /* 400kHz Fast Mode */

#define MPU6050_I2C_ADDR    0x68    /* AD0=LOW */
#define MPU6050_INT_PIN     2       /* IRQ: Bewegung/Crash */
#define BMP280_I2C_ADDR     0x76    /* oder 0x77, je nach Modul */

/* ============================================================
 * SPI (Display + SD shared Bus)
 * ============================================================ */

#define SPI_MOSI_PIN        11      /* shared: Display + SD */
#define SPI_MISO_PIN        13
#define SPI_SCLK_PIN        12

/* Display (E-Paper) */
#define DISPLAY_CS_PIN      10
#define DISPLAY_DC_PIN      14      /* Data/Command */
#define DISPLAY_RST_PIN     21
#define DISPLAY_BUSY_PIN    47      /* IRQ, Refresh warten */

/* SD-Karte */
#define SD_CS_PIN           38

/* ============================================================
 * ADC
 * ============================================================ */

#define BATT_ADC_UNIT       1       /* ADC1 */
#define BATT_ADC_CHANNEL    4       /* ADC1_CH4 = GPIO4 */
#define BATT_VOLTAGE_DIV    2.0f    /* Spannungsteiler 2:1 (max 3.3V) */

/* ============================================================
 * Power Control
 * ============================================================ */

#define GPS_POWER_EN_PIN    5       /* High-Side MOSFET (GPS ganz aus) */
#define BOOST_EN_PIN        6       /* MT3608 EN-Pin (5V-Rail schalten) */

/* ============================================================
 * Display-Aufloesung
 * ============================================================ */

#define TFT_WIDTH           240
#define TFT_HEIGHT          320

/* E-Paper 2.9" b/w (GDEM029T94 / SSD1680) */
#define EPD_WIDTH           296
#define EPD_HEIGHT          128

/* ============================================================
 * FreeRTOS Task-Konstanten
 * ============================================================ */

/* Core-Pinning: Core 1 = Echtzeit, Core 0 = UI/Comms */
#define TASK_CORE_RT        1       /* Sensor, GPS, CAN */
#define TASK_CORE_UI        0       /* Display, BLE, Logger, Power */

/* Stack sizes (Bytes, nicht Words — ESP-IDF Convention) */
#define TASK_STACK_GPS      4096
#define TASK_STACK_SENSOR   4096
#define TASK_STACK_DISPLAY  8192
#define TASK_STACK_LOGGER   4096
#define TASK_STACK_BLE      6144
#define TASK_STACK_POWER    2048

/* Prioritaeten */
#define TASK_PRIO_GPS       5
#define TASK_PRIO_SENSOR    4
#define TASK_PRIO_DISPLAY   2
#define TASK_PRIO_LOGGER    3
#define TASK_PRIO_BLE       3
#define TASK_PRIO_POWER     6

#endif /* _BOARD_CONFIG_H_ */
