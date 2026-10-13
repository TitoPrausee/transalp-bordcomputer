/*
 * diagnostics.c — Diagnostics Coordinator
 *
 * Initialisiert DTC-Subsystem, registriert Event-Bus-Callbacks fuer
 * automatische DTC-Generierung und startet die Serial-Console.
 *
 * Event-Bus Integration:
 *   EVT_GPS_LOST_FIX       -> DTC_S0502 (GPS-Fix verloren)
 *   EVT_GPS_FIX            -> DTC_S0503 (GPS-Fix wiederhergestellt)
 *   EVT_SDCARD_ERROR       -> DTC_C0104 (SD-Schreibfehler)
 *   EVT_SDCARD_MOUNTED     -> clear DTC_C0101 (SD wieder da)
 *   EVT_BLE_DISCONNECTED   -> DTC_C0202 (BLE getrennt)
 *   EVT_BATTERY_LOW        -> DTC_P0401 (Bordspannung niedrig)
 *   EVT_BATTERY_CRITICAL   -> DTC_P0402 (Bordspannung kritisch)
 *   EVT_CRASH_DETECTED     -> DTC_S0402 (Crash erkannt)
 *
 * Serial Console:
 *   Laeuft als FreeRTOS-Task auf Core 0, Prio 3, Stack 4096.
 *   Liest UART0 (USB-Serial), verarbeitet Befehle.
 *
 * Befehle:
 *   dtc list              Aktive DTCs anzeigen
 *   dtc history           DTC-Historie
 *   dtc clear <code>      Einzelnen DTC loeschen
 *   dtc clear all         Alle DTCs loeschen
 *   selftest full         Vollstaendiger Self-Test
 *   selftest quick        Schneller Self-Test
 *   selftest result       Letztes Self-Test-Ergebnis
 *   info                  System-Info (Version, Uptime, Heap)
 *   sensors               Sensor-Werte live
 *   gps                   GPS-Daten
 *   stats                 Statistiken (Heap, Tasks, Uptime)
 *   help                  Befehlsuebersicht
 */

#include "diagnostics.h"
#include "sensors.h"
#include "gps.h"
#include "power.h"
#include "ble.h"
#include "storage.h"
#include "event_bus.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static const char *TAG = "diag";

/* Console-Task Konstanten (aus Task-Spec) */
#define DIAG_CONSOLE_TASK_PRIO    3
#define DIAG_CONSOLE_TASK_STACK    4096
#define DIAG_CONSOLE_TASK_CORE     0

/* UART-Buffer fuer Console-Eingabe */
#define CONSOLE_BUF_SIZE    256
#define CONSOLE_MAX_TOKENS   8

/* Firmware-Version (kann von Build-System ueberschrieben werden) */
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "0.3.0-dev"
#endif

/* ============================================================
 * Event-Bus Monitor
 *
 * Da der Event-Bus FreeRTOS EventGroups verwendet (keine Callbacks),
 * laeuft ein eigener Task, der auf relevante Events wartet und
 * DTCs generiert.
 * ============================================================ */

#define DIAG_EVENT_MONITOR_STACK  2048

/* Events, die der Monitor ueberwacht */
#define DIAG_EVENT_MASK  (EVT_GPS_LOST_FIX | EVT_GPS_FIX | \
                          EVT_SDCARD_ERROR | EVT_SDCARD_MOUNTED | \
                          EVT_BLE_DISCONNECTED | EVT_BLE_CONNECTED | \
                          EVT_BATTERY_LOW | EVT_BATTERY_CRITICAL | \
                          EVT_CRASH_DETECTED)

static void diag_event_monitor_task(void *arg)
{
    (void)arg;

    /* Initiale Verzoegerung, damit andere Module fertig werden */
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "Event monitor started");

    while (1) {
        if (g_event_group == NULL) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        EventBits_t bits = xEventGroupWaitBits(g_event_group,
                                              DIAG_EVENT_MASK,
                                              pdTRUE,   /* Clear on exit */
                                              pdFALSE,  /* Any bit */
                                              pdMS_TO_TICKS(5000));

        if (bits == 0)
            continue;  /* Timeout, nichts passiert */

        if (bits & EVT_GPS_LOST_FIX) {
            dtc_register(DTC_S0502, DTC_SEV_WARN);
        }
        if (bits & EVT_GPS_FIX) {
            dtc_register(DTC_S0503, DTC_SEV_INFO);
        }
        if (bits & EVT_SDCARD_ERROR) {
            dtc_register(DTC_C0104, DTC_SEV_WARN);
        }
        if (bits & EVT_SDCARD_MOUNTED) {
            dtc_clear(DTC_C0101);
        }
        if (bits & EVT_BLE_DISCONNECTED) {
            dtc_register(DTC_C0202, DTC_SEV_WARN);
        }
        if (bits & EVT_BATTERY_LOW) {
            dtc_register(DTC_P0401, DTC_SEV_WARN);
        }
        if (bits & EVT_BATTERY_CRITICAL) {
            dtc_register(DTC_P0402, DTC_SEV_CRITICAL);
        }
        if (bits & EVT_CRASH_DETECTED) {
            dtc_register(DTC_S0402, DTC_SEV_WARN);
        }
    }
}

/* ============================================================
 * Console: Hilfsfunktionen fuer Ausgabe
 * ============================================================ */

static void console_print(const char *text)
{
    uart_write_bytes(UART_NUM_0, text, strlen(text));
}

static void console_printf(const char *fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0) {
        uart_write_bytes(UART_NUM_0, buf, (size_t)len);
    }
}

/* ============================================================
 * Console: DTC-Befehle
 * ============================================================ */

static void cmd_dtc_print_entry(const dtc_entry_t *e)
{
    console_printf("  %-6s  %-8s  %-8s  count=%-4u  first=%lu  last=%lu\r\n",
                   dtc_code_to_string(e->code),
                   dtc_severity_to_string(e->severity),
                   dtc_status_to_string(e->status),
                   e->count,
                   (unsigned long)e->first_occurrence_ms,
                   (unsigned long)e->last_occurrence_ms);

    console_printf("         Freeze: vbat=%.1fV  cool=%.1fC  amb=%.1fC  fuel=%.1fL  "
                   "rpm=%.0f  spd=%.1f  roll=%.1f  sats=%u\r\n",
                   e->freeze_frame.battery_v,
                   e->freeze_frame.coolant_temp_c,
                   e->freeze_frame.ambient_temp_c,
                   e->freeze_frame.fuel_percent,
                   e->freeze_frame.rpm,
                   e->freeze_frame.speed_kmh,
                   e->freeze_frame.roll_deg,
                   e->freeze_frame.gps_sats);
}

static int cmd_dtc(int argc, char **argv)
{
    if (argc < 2) {
        console_printf("Usage: dtc <list|history|clear <code>|clear all>\r\n");
        return 1;
    }

    if (strcmp(argv[1], "list") == 0) {
        dtc_entry_t entries[DTC_MAX_ENTRIES];
        uint8_t n = dtc_get_active(entries, DTC_MAX_ENTRIES);
        console_printf("=== Active DTCs (%u) ===\r\n", n);
        for (uint8_t i = 0; i < n; i++) {
            cmd_dtc_print_entry(&entries[i]);
        }
        if (n == 0)
            console_printf("  (none)\r\n");
        return 0;
    }

    if (strcmp(argv[1], "history") == 0) {
        dtc_entry_t entries[DTC_MAX_ENTRIES];
        uint8_t n = dtc_get_history(entries, DTC_MAX_ENTRIES);
        console_printf("=== DTC History (%u) ===\r\n", n);
        for (uint8_t i = 0; i < n; i++) {
            cmd_dtc_print_entry(&entries[i]);
        }
        if (n == 0)
            console_printf("  (none)\r\n");
        return 0;
    }

    if (strcmp(argv[1], "clear") == 0) {
        if (argc < 3) {
            console_printf("Usage: dtc clear <code|all>\r\n");
            return 1;
        }

        if (strcmp(argv[2], "all") == 0) {
            dtc_clear_all();
            console_printf("All DTCs cleared.\r\n");
            return 0;
        }

        /* DTC-Code anhand String suchen */
        for (uint16_t i = 0; i < DTC_CODE_MAX; i++) {
            if (strcmp(argv[2], dtc_code_to_string((dtc_code_t)i)) == 0) {
                dtc_clear((dtc_code_t)i);
                console_printf("DTC %s cleared.\r\n", argv[2]);
                return 0;
            }
        }

        console_printf("Unknown DTC code: %s\r\n", argv[2]);
        return 1;
    }

    console_printf("Unknown dtc subcommand: %s\r\n", argv[1]);
    return 1;
}

/* ============================================================
 * Console: Self-Test Befehle
 * ============================================================ */

static void cmd_selftest_print_result(const selftest_result_t *r)
{
    console_printf("=== Self-Test Result ===\r\n");
    console_printf("Overall: %s  Duration: %lu ms  Time: %lu ms\r\n",
                   r->all_pass ? "PASS" : "FAIL",
                   (unsigned long)r->duration_ms,
                   (unsigned long)r->timestamp_ms);
    for (uint8_t i = 0; i < r->count; i++) {
        const char *st_str;
        switch (r->items[i].status) {
            case ST_PASS: st_str = "PASS"; break;
            case ST_FAIL: st_str = "FAIL"; break;
            case ST_SKIP: st_str = "SKIP"; break;
            default:      st_str = "????"; break;
        }
        console_printf("  %-10s  %s  %s\r\n",
                       r->items[i].name, st_str,
                       r->items[i].code != DTC_NONE ?
                           dtc_code_to_string(r->items[i].code) : "-");
    }
}

static int cmd_selftest(int argc, char **argv)
{
    if (argc < 2) {
        console_printf("Usage: selftest <full|quick|result>\r\n");
        return 1;
    }

    if (strcmp(argv[1], "full") == 0) {
        console_printf("Running full self-test...\r\n");
        bool ok = selftest_run_full();
        cmd_selftest_print_result(selftest_get_last_result());
        console_printf("Result: %s\r\n", ok ? "PASS" : "FAIL");
        return 0;
    }

    if (strcmp(argv[1], "quick") == 0) {
        console_printf("Running quick self-test...\r\n");
        bool ok = selftest_run_quick();
        cmd_selftest_print_result(selftest_get_last_result());
        console_printf("Result: %s\r\n", ok ? "PASS" : "FAIL");
        return 0;
    }

    if (strcmp(argv[1], "result") == 0) {
        cmd_selftest_print_result(selftest_get_last_result());
        return 0;
    }

    console_printf("Unknown selftest subcommand: %s\r\n", argv[1]);
    return 1;
}

/* ============================================================
 * Console: Info / System-Info
 * ============================================================ */

static int cmd_info(int argc, char **argv)
{
    (void)argc; (void)argv;

    uint32_t uptime_s = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    uint32_t heap_free = esp_get_free_heap_size();
    uint32_t heap_min  = esp_get_minimum_free_heap_size();

    console_printf("=== System Info ===\r\n");
    console_printf("Firmware:   %s\r\n", FIRMWARE_VERSION);
    console_printf("Uptime:     %lu s (%lu h %lu m)\r\n",
                   (unsigned long)uptime_s,
                   (unsigned long)(uptime_s / 3600),
                   (unsigned long)((uptime_s % 3600) / 60));
    console_printf("Heap:       %lu bytes free, %lu min\r\n",
                   (unsigned long)heap_free,
                   (unsigned long)heap_min);
    console_printf("Battery:    %.2f V (%u%%)\r\n",
                   power_get_battery_voltage(),
                   power_get_battery_percent());
    console_printf("BLE:        %s\r\n",
                   ble_is_connected() ? "connected" : "disconnected");
    console_printf("SD Card:    %s\r\n",
                   sd_card_is_mounted() ? "mounted" : "not mounted");
    console_printf("Power:      %s\r\n",
                   power_get_state() == POWER_STATE_ACTIVE  ? "ACTIVE"  :
                   power_get_state() == POWER_STATE_IDLE   ? "IDLE"   :
                   power_get_state() == POWER_STATE_SLEEP   ? "SLEEP"  :
                   power_get_state() == POWER_STATE_SHUTDOWN ? "SHUTDOWN" : "????");

    /* Active DTCs zusammenfassen */
    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t n = dtc_get_active(entries, DTC_MAX_ENTRIES);
    console_printf("Active DTCs: %u\r\n", n);

    return 0;
}

/* ============================================================
 * Console: Sensor-Werte live
 * ============================================================ */

static int cmd_sensors(int argc, char **argv)
{
    (void)argc; (void)argv;

    if (sensor_data_queue == NULL) {
        console_printf("Sensor queue not initialized.\r\n");
        return 1;
    }

    sensor_data_t sd;
    if (xQueuePeek(sensor_data_queue, &sd, pdMS_TO_TICKS(1000)) != pdTRUE) {
        console_printf("No sensor data available.\r\n");
        return 1;
    }

    console_printf("=== Sensor Values ===\r\n");
    console_printf("Temperature:  %.1f C\r\n", sd.temp_c);
    console_printf("Pressure:     %.1f hPa\r\n", sd.pressure_hpa);
    console_printf("Altitude:     %.1f m\r\n", sd.altitude_m);
    console_printf("Roll:         %.1f deg\r\n", sd.roll_deg);
    console_printf("Pitch:        %.1f deg\r\n", sd.pitch_deg);
    console_printf("RPM:          %u\r\n", sd.rpm);
    console_printf("Speed:        %.1f km/h\r\n", sd.speed_kmh);
    console_printf("Gear:         %u\r\n", sd.gear);
    console_printf("Coolant:      %.1f C\r\n", sd.coolant_temp_c);
    console_printf("Fuel:         %.1f L\r\n", sd.fuel_level_l);
    console_printf("Crash:        %s\r\n", sd.crash_detected ? "YES" : "no");
    console_printf("Timestamp:    %lu ms\r\n", (unsigned long)sd.timestamp_ms);

    return 0;
}

/* ============================================================
 * Console: GPS-Daten
 * ============================================================ */

static int cmd_gps(int argc, char **argv)
{
    (void)argc; (void)argv;

    if (gps_data_queue == NULL) {
        console_printf("GPS queue not initialized.\r\n");
        return 1;
    }

    gps_data_t gd;
    if (xQueuePeek(gps_data_queue, &gd, pdMS_TO_TICKS(1000)) != pdTRUE) {
        console_printf("No GPS data available.\r\n");
        return 1;
    }

    console_printf("=== GPS Data ===\r\n");
    console_printf("Latitude:     %.6f\r\n", (double)gd.lat);
    console_printf("Longitude:    %.6f\r\n", (double)gd.lon);
    console_printf("Speed:        %.1f km/h\r\n", gd.speed_kmh);
    console_printf("Course:       %.1f deg\r\n", gd.course_deg);
    console_printf("Altitude:     %.1f m\r\n", gd.altitude_m);
    console_printf("Satellites:   %u\r\n", gd.satellites);
    console_printf("HDOP:         %.1f\r\n", gd.hdop);
    console_printf("Fix Quality:  %d\r\n", gd.fix_quality);
    console_printf("Valid:        %s\r\n", gd.valid ? "yes" : "no");
    console_printf("Timestamp:    %lu ms\r\n", (unsigned long)gd.timestamp_ms);

    return 0;
}

/* ============================================================
 * Console: Statistiken
 * ============================================================ */

static int cmd_stats(int argc, char **argv)
{
    (void)argc; (void)argv;

    console_printf("=== Statistics ===\r\n");
    console_printf("Free Heap:      %lu bytes\r\n",
                   (unsigned long)esp_get_free_heap_size());
    console_printf("Min Free Heap:  %lu bytes\r\n",
                   (unsigned long)esp_get_minimum_free_heap_size());
    console_printf("Uptime:         %llu us\r\n",
                   (unsigned long long)esp_timer_get_time());

    /* Task-Anzahl (FreeRTOS) */
    TaskStatus_t task_buf[20];
    UBaseType_t n_tasks = uxTaskGetNumberOfTasks();
    UBaseType_t n_running = uxTaskGetSystemState(task_buf, 20, NULL);
    console_printf("Tasks:          %u total, %u listed\r\n",
                   (unsigned)n_tasks, (unsigned)n_running);

    console_printf("Tick Rate:      %d Hz\r\n", (int)configTICK_RATE_HZ);

    /* DTC-Statistik */
    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t n_active  = dtc_get_active(entries, DTC_MAX_ENTRIES);
    uint8_t n_history = dtc_get_history(entries, DTC_MAX_ENTRIES);
    console_printf("DTCs:           %u active, %u history\r\n",
                   n_active, n_history);

    /* Battery */
    console_printf("Battery:        %.2f V (%u%%)\r\n",
                   power_get_battery_voltage(),
                   power_get_battery_percent());

    return 0;
}

/* ============================================================
 * Console: Hilfe
 * ============================================================ */

static int cmd_help(int argc, char **argv)
{
    (void)argc; (void)argv;

    console_printf("\r\n=== Diagnostic Console ===\r\n");
    console_printf("Commands:\r\n");
    console_printf("  dtc list              Show active DTCs\r\n");
    console_printf("  dtc history           Show DTC history\r\n");
    console_printf("  dtc clear <code>      Clear a specific DTC (e.g. P0101)\r\n");
    console_printf("  dtc clear all         Clear all DTCs\r\n");
    console_printf("  selftest full         Run full self-test (8 modules)\r\n");
    console_printf("  selftest quick        Run quick self-test (Power, Display, Sensors)\r\n");
    console_printf("  selftest result       Show last self-test result\r\n");
    console_printf("  info                  System info (version, uptime, heap, battery)\r\n");
    console_printf("  sensors               Live sensor values\r\n");
    console_printf("  gps                   GPS data\r\n");
    console_printf("  stats                 System statistics (heap, tasks, DTCs)\r\n");
    console_printf("  help                  This help\r\n");
    console_printf("\r\n");

    return 0;
}

/* ============================================================
 * Console: Befehls-Tabelle
 * ============================================================ */

typedef struct {
    const char *cmd;
    const char *help;
    int (*handler)(int argc, char **argv);
} diag_command_t;

static const diag_command_t s_commands[] = {
    { "dtc",      "DTC management",         cmd_dtc      },
    { "selftest", "Run self-test",           cmd_selftest },
    { "info",     "System information",      cmd_info     },
    { "sensors",  "Live sensor values",      cmd_sensors  },
    { "gps",      "GPS data",                cmd_gps      },
    { "stats",    "System statistics",       cmd_stats    },
    { "help",     "Show this help",          cmd_help     },
    { "?",        "Show this help",          cmd_help     },
    { NULL, NULL, NULL },
};

/* ============================================================
 * Console: Befehl parsen und ausfuehren
 * ============================================================ */

static void console_process_line(char *line, int len)
{
    /* Zeilenumbrueche entfernen */
    while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')) {
        line[--len] = '\0';
    }
    if (len == 0)
        return;

    /* Tokenisieren */
    char *tokens[CONSOLE_MAX_TOKENS];
    int argc = 0;
    char *tok = strtok(line, " \t");
    while (tok != NULL && argc < CONSOLE_MAX_TOKENS) {
        tokens[argc++] = tok;
        tok = strtok(NULL, " \t");
    }

    if (argc == 0)
        return;

    /* Befehl suchen */
    for (int i = 0; s_commands[i].cmd != NULL; i++) {
        if (strcmp(tokens[0], s_commands[i].cmd) == 0) {
            s_commands[i].handler(argc, tokens);
            return;
        }
    }

    console_printf("Unknown command: '%s'. Type 'help' for commands.\r\n", tokens[0]);
}

/* ============================================================
 * Console Task
 *
 * Liest UART0 (USB-Serial), verarbeitet Befehle zeilenorientiert.
 * ============================================================ */

void diag_console_task(void *arg)
{
    (void)arg;

    char rx_buf[CONSOLE_BUF_SIZE];
    int rx_len = 0;

    /* UART0 konfigurieren */
    const uart_config_t uart_cfg = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_NUM_0, &uart_cfg);

    ESP_LOGI(TAG, "Diagnostic console started on UART0");

    console_print("\r\n=== Bordcomputer Diagnostic Console ===\r\n");
    console_print("Type 'help' for available commands.\r\n> ");

    while (1) {
        uint8_t ch;
        int n = uart_read_bytes(UART_NUM_0, &ch, 1, pdMS_TO_TICKS(100));

        if (n <= 0)
            continue;

        /* Echo */
        uart_write_bytes(UART_NUM_0, (const char *)&ch, 1);

        switch (ch) {
        case '\r':
            /* Zeile komplett — ausfuehren */
            rx_buf[rx_len] = '\0';
            console_print("\r\n");
            console_process_line(rx_buf, rx_len);
            console_print("> ");
            rx_len = 0;
            break;

        case '\n':
            /* Manche Terminals senden nur \n — wie \r behandeln */
            if (rx_len > 0) {
                rx_buf[rx_len] = '\0';
                console_print("\r\n");
                console_process_line(rx_buf, rx_len);
                console_print("> ");
                rx_len = 0;
            }
            break;

        case '\b':
        case 0x7F:  /* DEL */
            /* Backspace */
            if (rx_len > 0) {
                rx_len--;
                console_print(" \b");
            }
            break;

        default:
            /* Normales Zeichen */
            if (rx_len < CONSOLE_BUF_SIZE - 1) {
                rx_buf[rx_len++] = (char)ch;
            } else {
                /* Buffer voll — Zeile abschliessen */
                rx_buf[rx_len] = '\0';
                console_print("\r\n");
                console_process_line(rx_buf, rx_len);
                console_print("> ");
                rx_len = 0;
            }
            break;
        }
    }
}

/* ============================================================
 * Console initialisieren (Task starten)
 * ============================================================ */

void diag_console_init(void)
{
    /* UART0 installieren (falls noch nicht geschehen) */
    if (!uart_is_driver_installed(UART_NUM_0)) {
        uart_driver_install(UART_NUM_0, CONSOLE_BUF_SIZE * 2, 0, 0, NULL, 0);
    }

    xTaskCreatePinnedToCore(diag_console_task,
                             "diag_console",
                             DIAG_CONSOLE_TASK_STACK,
                             NULL,
                             DIAG_CONSOLE_TASK_PRIO,
                             NULL,
                             DIAG_CONSOLE_TASK_CORE);

    ESP_LOGI(TAG, "Console task created (prio=%d, stack=%d, core=%d)",
             DIAG_CONSOLE_TASK_PRIO, DIAG_CONSOLE_TASK_STACK, DIAG_CONSOLE_TASK_CORE);
}

/* ============================================================
 * Diagnostics Initialisierung
 *
 * Wird von app_main / diagnostics_init aufgerufen.
 * Initialisiert DTC-Subsystem, startet Event-Monitor und Console.
 * ============================================================ */

void diag_init(void)
{
    /* 1. DTC-Subsystem initialisieren (laedt persistierte DTCs aus NVS) */
    dtc_init();

    /* 2. Event-Bus-Monitor starten (generiert DTCs aus Events) */
    xTaskCreatePinnedToCore(diag_event_monitor_task,
                             "diag_events",
                             DIAG_EVENT_MONITOR_STACK,
                             NULL,
                             2,
                             NULL,
                             TASK_CORE_UI);

    /* 3. Serial Console starten */
    diag_console_init();

    ESP_LOGI(TAG, "Diagnostics initialized (DTC + Event Monitor + Console)");
}

/* Backward-compat: diagnostics_init -> diag_init */
void diagnostics_init(void)
{
    diag_init();
}
