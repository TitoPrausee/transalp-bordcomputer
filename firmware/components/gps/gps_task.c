/*
 * gps_task.c — GPS-Task: UART lesen, NMEA parsen, Daten bereitstellen
 *
 * Laeuft auf Core 1 (TASK_CORE_RT) mit Prioritaet 5 (TASK_PRIO_GPS).
 * UART: RX=GPIO44, TX=GPIO43, Baud=38400 (Default).
 *
 * Der Task liest UART-Daten zeilenweise (bis \r\n), uebergibt jede
 * NMEA-Sentence an den Parser und speichert die resultierenden Daten
 * thread-safe in einer statischen gps_data_t-Struktur (Mutex-geschuetzt).
 *
 * Bei neuem Fix wird EVT_GPS_FIX im Event-Bus gesetzt, bei Verlust
 * EVT_GPS_LOST_FIX. Ein Watchdog warnt, wenn 30s lang kein Fix empfangen
 * wurde.
 */

#include "gps.h"
#include "board_config.h"
#include "event_bus.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "gps";

/* ---- Konfiguration ---- */

#define GPS_UART_RX_BUF_SIZE  1024   /* RX-Buffer */
#define GPS_UART_TX_BUF_SIZE  256    /* TX-Buffer (Konfig-Befehle) */
#define GPS_LINE_BUF_SIZE     128    /* NMEA max 82 Bytes, etwas Reserve */
#define GPS_TASK_STACK        4096
#define GPS_TASK_PRIO         5
#define GPS_TASK_CORE         1

#define GPS_NO_FIX_TIMEOUT_MS 30000  /* 30s Watchdog-Schwelle */
#define GPS_READ_TIMEOUT_MS   100    /* UART-Poll-Intervall */

/* 1 knot = 1.852 km/h */
#define KNOTS_TO_KMH         1.852f

/* ---- Globale Zustaende ---- */

QueueHandle_t gps_data_queue = NULL;

/* Thread-safe Daten */
static gps_data_t s_gps_data;
static SemaphoreHandle_t s_gps_mutex = NULL;

/* Letzter Fix-Zeitstempel (FreeRTOS-Ticks) */
static TickType_t s_last_fix_tick = 0;
static bool s_had_fix = false;

/* ---- Hilfsfunktionen ---- */

/*
 * Thread-safe Kopie der GPS-Daten an den Aufrufer.
 * Rueckgabe: true wenn has_fix, false sonst (Daten trotzdem kopiert).
 */
bool gps_get_data(gps_data_t *out)
{
    if (!out) return false;

    if (s_gps_mutex && xSemaphoreTake(s_gps_mutex, pdMS_TO_TICKS(50))) {
        memcpy(out, &s_gps_data, sizeof(gps_data_t));
        xSemaphoreGive(s_gps_mutex);
        return out->has_fix;
    }
    return false;
}

/*
 * Interne Funktion: GPS-Daten unter Mutex aktualisieren.
 */
static void gps_update_data(const gps_data_t *new_data)
{
    if (!s_gps_mutex || !new_data) return;

    xSemaphoreTake(s_gps_mutex, portMAX_DELAY);
    memcpy(&s_gps_data, new_data, sizeof(gps_data_t));
    s_gps_data.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    xSemaphoreGive(s_gps_mutex);
}

/*
 * UART konfigurieren und Treiber installieren.
 */
static void gps_uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate  = GPS_UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(GPS_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GPS_UART_NUM,
                                  GPS_UART_TX_PIN,
                                  GPS_UART_RX_PIN,
                                  UART_PIN_NO_CHANGE,
                                  UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(GPS_UART_NUM,
                                        GPS_UART_RX_BUF_SIZE,
                                        GPS_UART_TX_BUF_SIZE,
                                        0, NULL, 0));

    ESP_LOGI(TAG, "UART%d initialized: baud=%d, rx_pin=%d, tx_pin=%d",
             GPS_UART_NUM, GPS_UART_BAUD, GPS_UART_RX_PIN, GPS_UART_TX_PIN);
}

/*
 * Eine komplette NMEA-Zeile verarbeiten: parsen, Daten aktualisieren,
 * Event-Bus benachrichtigen.
 */
static void gps_process_line(const char *line, gps_data_t *data)
{
    bool ok = nmea_parse(line, data);

    if (!ok) return;

    /* Bei einem GGA- oder RMC-Satz mit Fix: Daten und Events aktualisieren */
    if (data->has_fix) {
        s_last_fix_tick = xTaskGetTickCount();

        if (!s_had_fix) {
            s_had_fix = true;
            event_bus_set(EVT_GPS_FIX);
            ESP_LOGI(TAG, "GPS fix acquired: lat=%.6f lon=%.6f sats=%u hdop=%.1f",
                     data->lat, data->lon, (unsigned)data->satellites, data->hdop);
        }

        /* Daten thread-safe aktualisieren */
        gps_update_data(data);

        /* Neueste Daten in Queue (ueberschreibt aelteste) */
        if (gps_data_queue) {
            xQueueOverwrite(gps_data_queue, data);
        }
    }
}

/*
 * Watchdog: Wenn laenger als GPS_NO_FIX_TIMEOUT_MS kein Fix, Warning loggen.
 */
static void gps_watchdog(void)
{
    if (!s_had_fix) return;

    TickType_t now = xTaskGetTickCount();
    uint32_t elapsed_ms = (uint32_t)((now - s_last_fix_tick) * portTICK_PERIOD_MS);

    if (elapsed_ms > GPS_NO_FIX_TIMEOUT_MS) {
        ESP_LOGW(TAG, "No GPS fix for %lu ms", (unsigned long)elapsed_ms);
        s_had_fix = false;
        event_bus_set(EVT_GPS_LOST_FIX);

        /* has_fix unter Mutex zuruecksetzen */
        if (s_gps_mutex && xSemaphoreTake(s_gps_mutex, pdMS_TO_TICKS(50))) {
            s_gps_data.has_fix = false;
            s_gps_data.valid = false;
            xSemaphoreGive(s_gps_mutex);
        }
    }
}

/* ---- Oeffentliche API ---- */

void gps_init(void)
{
    /* Mutex erstellen */
    s_gps_mutex = xSemaphoreCreateMutex();
    if (!s_gps_mutex) {
        ESP_LOGE(TAG, "Failed to create GPS mutex");
        return;
    }

    /* Daten initialisieren */
    memset(&s_gps_data, 0, sizeof(s_gps_data));
    s_gps_data.fix_quality = GPS_FIX_NONE;
    s_gps_data.year = 2000;

    /* UART konfigurieren */
    gps_uart_init();

    /* Queue: neueste GPS-Daten (ueberschreibt aelteste) */
    gps_data_queue = xQueueCreate(4, sizeof(gps_data_t));
    if (!gps_data_queue) {
        ESP_LOGE(TAG, "Failed to create gps_data_queue");
        return;
    }

    s_last_fix_tick = xTaskGetTickCount();
    s_had_fix = false;

    ESP_LOGI(TAG, "GPS initialized on UART%d (baud=%d)", GPS_UART_NUM, GPS_UART_BAUD);
}

void gps_task(void *arg)
{
    (void)arg;

    /* Zeilen-Buffer fuer teilweise UART-Reads */
    char line_buf[GPS_LINE_BUF_SIZE];
    int line_pos = 0;

    /* Arbeitskopie der GPS-Daten — der Parser aktualisiert nur gefuellte Felder */
    gps_data_t data;
    memset(&data, 0, sizeof(data));
    data.fix_quality = GPS_FIX_NONE;
    data.year = 2000;

    uint8_t rx_buf[GPS_UART_RX_BUF_SIZE];

    ESP_LOGI(TAG, "GPS task started on core %d", xPortGetCoreID());

    while (1) {
        int len = uart_read_bytes(GPS_UART_NUM, rx_buf, sizeof(rx_buf) - 1,
                                   pdMS_TO_TICKS(GPS_READ_TIMEOUT_MS));

        if (len <= 0) {
            /* Timeout — Watchdog pruefen */
            gps_watchdog();
            continue;
        }

        /* Empfangene Bytes zeichenweise verarbeiten */
        for (int i = 0; i < len; i++) {
            char c = (char)rx_buf[i];

            /* Zeilenende erkennen (\r\n oder nur \n) */
            if (c == '\r') {
                continue;
            }

            if (c == '\n') {
                /* Zeile komplett: null-terminieren und parsen */
                if (line_pos > 0) {
                    line_buf[line_pos] = '\0';
                    gps_process_line(line_buf, &data);
                }
                line_pos = 0;
                continue;
            }

            /* Zeichen zum Buffer hinzufuegen (mit Overflow-Schutz) */
            if (line_pos < GPS_LINE_BUF_SIZE - 1) {
                line_buf[line_pos++] = c;
            } else {
                /* Zeile zu lang — verwerfen und neu beginnen */
                ESP_LOGW(TAG, "NMEA line overflow (%d bytes), discarding", line_pos);
                line_pos = 0;
            }
        }

        /* Watchdog nach Verarbeitung pruefen */
        gps_watchdog();
    }
}
