/*
 * gps_task.c — GPS-Task: UART lesen, NMEA parsen, Queue senden
 *
 * Laeuft auf Core 1 mit 10Hz Update-Rate.
 * UART: RX=GPIO44, TX=GPIO43, Baud=38400 (Default).
 *
 * TODO: UBX-Konfiguration beim Start senden:
 *   - CFG-RATE: measRate=100ms (10Hz)
 *   - CFG-MSG: nur GGA und RMC aktivieren
 *   - CFG-PRT: Baudrate auf 115200 erhoehen
 *
 * TODO: PPS-Signal (GPIO1) fuer Zeitsynchronisation nutzen.
 */

#include "gps.h"
#include "board_config.h"
#include "event_bus.h"
#include "esp_log.h"
#include "driver/uart.h"

static const char *TAG = "gps";
QueueHandle_t gps_data_queue = NULL;

#define GPS_BUF_SIZE  512

void gps_init(void)
{
    /* UART konfigurieren */
    uart_config_t uart_config = {
        .baud_rate  = GPS_UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(GPS_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GPS_UART_NUM, GPS_UART_TX_PIN,
                                  GPS_UART_RX_PIN,
                                  UART_PIN_NO_CHANGE,
                                  UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(GPS_UART_NUM, GPS_BUF_SIZE * 2,
                                         GPS_BUF_SIZE, 0, NULL, 0));

    /* Queue: neueste GPS-Daten (overwrite aelteste) */
    gps_data_queue = xQueueCreate(4, sizeof(gps_data_t));
    if (!gps_data_queue) {
        ESP_LOGE(TAG, "Failed to create gps_data_queue");
        return;
    }

    /* TODO: UBX-Konfiguration senden (10Hz, GGA+RMC only) */

    ESP_LOGI(TAG, "GPS initialized on UART%d (baud=%d)", GPS_UART_NUM, GPS_UART_BAUD);
}

void gps_task(void *arg)
{
    gps_data_t data = {0};
    uint8_t buf[GPS_BUF_SIZE];

    while (1) {
        int len = uart_read_bytes(GPS_UART_NUM, buf, GPS_BUF_SIZE - 1,
                                   pdMS_TO_TICKS(100));
        if (len > 0) {
            buf[len] = '\0';

            /* NMEA-Zeilen trennen und einzeln parsen.
             * TODO: strtok_r oder eigener Line-Splitter. */
            char *line = (char *)buf;
            char *next;
            while (line != NULL && *line != '\0') {
                next = strchr(line, '\n');
                if (next) {
                    *next = '\0';
                    /* Zeile parsen */
                    nmea_parse_line(line, &data);
                    line = next + 1;
                } else {
                    nmea_parse_line(line, &data);
                    break;
                }
            }

            data.timestamp_ms = xthal_get_ccount() / 240000;

            /* Fix-Status als Event */
            if (data.valid) {
                event_bus_set(EVT_GPS_FIX);
            }

            /* Neueste Daten in Queue (overwrite aelteste) */
            xQueueOverwrite(gps_data_queue, &data);
        }
    }
}
