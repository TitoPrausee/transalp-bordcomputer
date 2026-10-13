#include "gps.h"
#include "esp_log.h"
#include "driver/uart.h"

static const char *TAG = "gps";
QueueHandle_t gps_data_queue = NULL;

#define GPS_UART_NUM   UART_NUM_1
#define GPS_BUF_SIZE    512

void gps_init(void)
{
    /* UART für NEO-M9N konfigurieren */
    uart_config_t cfg = {
        .baud_rate  = 38400,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(GPS_UART_NUM, GPS_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(GPS_UART_NUM, &cfg);
    /* Pins: GPIO44=RX, GPIO43=TX (siehe PINMAP.md) */
    uart_set_pin(GPS_UART_NUM, 43, 44, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    gps_data_queue = xQueueCreate(4, sizeof(gps_data_t));
    ESP_LOGI(TAG, "GPS initialized on UART1");
}

void gps_task(void *arg)
{
    gps_data_t data = {0};
    uint8_t buf[GPS_BUF_SIZE];
    /* TODO: NMEA-Parser (minmea oder eigen) */
    /* TODO: UBX-Konfig: 10Hz, aktiviere GGA+RMC nur */
    while (1) {
        int len = uart_read_bytes(GPS_UART_NUM, buf, GPS_BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            buf[len] = 0;
            /* parse_nmea(buf, &data); */
            xQueueOverwrite(gps_data_queue, &data);
        }
    }
}
