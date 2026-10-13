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
#define GPS_PPS_PIN         39      /* 1PPS Zeitsync (IRQ, RTC-GPIO) */

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
#define BATT_ADC_CHANNEL    ADC1_CH0 /* ADC1_CH0 = GPIO1 */
#define BATT_VOLTAGE_DIV    10.0f   /* Spannungsteiler 10:1 (100k/10k) */

/* Coolant Temp: NTC 10k via ADC1_CH6 = GPIO7 */
#define TEMP_NTC_ADC_CHANNEL    6       /* ADC1_CH6 */
/* DS18B20 1-Wire (digital, primary coolant sensor) */
#define TEMP_ONEWIRE_PIN        15

/* Fuel Level: via ADC1_CH2 = GPIO3 */
#define FUEL_ADC_CHANNEL        2       /* ADC1_CH2 */

/* ============================================================
 * RPM Pulse Counter (PCNT)
 * ============================================================ */

#define RPM_PCNT_PIN            16      /* Optokoppler-Ausgang */
#define RPM_PCNT_HIGH_LIMIT     10000
#define RPM_PULSES_PER_REV      1       /* 1 Pickup-Puls pro Umdrehung */
#define RPM_MAX_VALUE           8000    /* Redline ~7500 */

/* ============================================================
 * Sensor-Konstanten
 * ============================================================ */

/* NTC 10k B3950 (Kuehlmittel-Temperatur, Fallback) */
#define NTC_R25                 10000.0f
#define NTC_B_VALUE             3950.0f
#define NTC_SERIES_R            10000.0f
#define TEMP_OVERTEMP_C         120.0f

/* Kraftstoffstand */
#define TANK_CAPACITY_L         17.0f
#define FUEL_RESERVE_L          3.0f

/* Coolant Temp: NTC 10k via ADC1_CH6 = GPIO7 */
#define TEMP_NTC_ADC_CHANNEL    6       /* ADC1_CH6 */
/* DS18B20 1-Wire (digital, primary coolant sensor) */
#define TEMP_ONEWIRE_PIN        15

/* Fuel Level: via ADC1_CH2 = GPIO3 */
#define FUEL_ADC_CHANNEL        2       /* ADC1_CH2 */

/* ============================================================
 * RPM Pulse Counter (PCNT)
 * ============================================================ */

#define RPM_PCNT_PIN            16      /* Optokoppler-Ausgang */
#define RPM_PCNT_HIGH_LIMIT     10000
#define RPM_PULSES_PER_REV      1       /* 1 Pickup-Puls pro Umdrehung */
#define RPM_MAX_VALUE           8000    /* Redline ~7500 */

/* ============================================================
 * Sensor-Konstanten
 * ============================================================ */

/* NTC 10k B3950 (Kuehlmittel-Temperatur, Fallback) */
#define NTC_R25                 10000.0f
#define NTC_B_VALUE             3950.0f
#define NTC_SERIES_R            10000.0f
#define TEMP_OVERTEMP_C         120.0f

/* Kraftstoffstand */
#define TANK_CAPACITY_L         17.0f
#define FUEL_RESERVE_L          3.0f

/* Coolant Temp: NTC 10k via ADC1_CH6 = GPIO7 */
#define TEMP_NTC_ADC_CHANNEL    6       /* ADC1_CH6 */
/* DS18B20 1-Wire (digital, primary coolant sensor) */
#define TEMP_ONEWIRE_PIN        15

/* Fuel Level: via ADC1_CH2 = GPIO3 */
#define FUEL_ADC_CHANNEL        2       /* ADC1_CH2 */

/* ============================================================
 * RPM Pulse Counter (PCNT)
 * ============================================================ */

#define RPM_PCNT_PIN            16      /* Optokoppler-Ausgang */
#define RPM_PCNT_HIGH_LIMIT     10000
#define RPM_PULSES_PER_REV      1       /* 1 Pickup-Puls pro Umdrehung */
#define RPM_MAX_VALUE           8000    /* Redline ~7500 */

/* ============================================================
 * Sensor-Konstanten
 * ============================================================ */

/* NTC 10k B3950 (Kuehlmittel-Temperatur, Fallback) */
#define NTC_R25                 10000.0f
#define NTC_B_VALUE             3950.0f
#define NTC_SERIES_R            10000.0f
#define TEMP_OVERTEMP_C         120.0f

/* Kraftstoffstand */
#define TANK_CAPACITY_L         17.0f
#define FUEL_RESERVE_L          3.0f

/* Coolant Temp: NTC 10k via ADC1_CH6 = GPIO7 */
#define TEMP_NTC_ADC_CHANNEL    6       /* ADC1_CH6 */
/* DS18B20 1-Wire (digital, primary coolant sensor) */
#define TEMP_ONEWIRE_PIN        15

/* Fuel Level: via ADC1_CH2 = GPIO3 */
#define FUEL_ADC_CHANNEL        2       /* ADC1_CH2 */

/* ============================================================
 * RPM Pulse Counter (PCNT)
 * ============================================================ */

#define RPM_PCNT_PIN            16      /* Optokoppler-Ausgang */
#define RPM_PCNT_HIGH_LIMIT     10000
#define RPM_PULSES_PER_REV      1       /* 1 Pickup-Puls pro Umdrehung */
#define RPM_MAX_VALUE           8000    /* Redline ~7500 */

/* ============================================================
 * Sensor-Konstanten
 * ============================================================ */

/* NTC 10k B3950 (Kuehlmittel-Temperatur, Fallback) */
#define NTC_R25                 10000.0f
#define NTC_B_VALUE             3950.0f
#define NTC_SERIES_R            10000.0f
#define TEMP_OVERTEMP_C         120.0f

/* Kraftstoffstand */
#define TANK_CAPACITY_L         17.0f
#define FUEL_RESERVE_L          3.0f

/* ============================================================
 * Power Control
 * ============================================================ */

#define GPS_POWER_EN_PIN    5       /* High-Side MOSFET (GPS ganz aus) */
#define BOOST_EN_PIN        6       /* MT3608 EN-Pin (5V-Rail schalten) */

/* ============================================================
 * Display Backlight (LEDC PWM)
 * ============================================================ */

#define TFT_BL_PIN          18      /* ILI9488 Backlight (LEDC PWM) */
#define TFT_BL_LEDC_TIMER   LEDC_TIMER_1
#define TFT_BL_LEDC_CH      LEDC_CHANNEL_1
#define TFT_BL_FREQ_HZ      5000
#define TFT_BL_RESOLUTION   LEDC_TIMER_10_BIT

/* ============================================================
 * Buttons (Active-LOW mit internem Pull-Up)
 * ============================================================ */

#define BTN_MENU_PIN        6       /* Taster Menu */
#define BTN_SELECT_PIN      7       /* Taster Select */
#define BTN_BACK_PIN        3       /* Taster Zurueck (Strapping) */
#define BOOT_BTN_PIN        0       /* Boot-Taster */

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

/* Event-Loop (Core 0) */
#define TASK_STACK_EVENT_LOOP  4096
#define TASK_PRIO_EVENT_LOOP   3

/* OTA-Task (Core 0, laeuft nur bei Update) */
#define TASK_STACK_OTA       8192
#define TASK_PRIO_OTA        4

/* Diagnostics-Task / Event-Monitor (Core 1) */
#define TASK_STACK_DIAG      4096
#define TASK_PRIO_DIAG       2

#endif /* _BOARD_CONFIG_H_ */
