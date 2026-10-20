#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * storage.h — SD-Karte, Logger, Konfiguration
 *
 * SD-Karte ueber SPI (shared mit Display).
 * FATFS fuer CSV/GPX/Binaer-Logging.
 */

#define SD_MOUNT_POINT  "/sdcard"

/* ============================================================
 * SD-Karten Konstanten
 * ============================================================ */

#define SD_MAX_PATH_LEN   128
#define SD_MAX_FILES      4

/* Return-Codes fuer SD-Karten-Operationen */
#define SD_OK                0
#define SD_ERR_INVALID_ARG  (-1)
#define SD_ERR_NOT_MOUNTED  (-2)
#define SD_ERR_OPEN_FAILED  (-3)
#define SD_ERR_WRITE_FAILED (-4)
#define SD_ERR_READ_FAILED  (-5)
#define SD_ERR_NOT_FOUND    (-6)

/* ============================================================
 * Logger-Konstanten
 * ============================================================ */

#define LOGGER_RINGBUF_SIZE     256
#define LOGGER_MAX_RATE_HZ      10
#define LOGGER_MIN_DIST_M       10.0f
#define LOGGER_FLUSH_TIMEOUT_MS 5000

/* ============================================================
 * Konfiguration
 * ============================================================ */

#define CONFIG_VERSION  3

typedef enum {
    CFG_KEY_DISPLAY_MODE = 0,
    CFG_KEY_DISPLAY_THEME,
    CFG_KEY_DISPLAY_BRIGHTNESS,
    CFG_KEY_LOG_INTERVAL,
    CFG_KEY_LOG_FORMAT,
    CFG_KEY_LOG_AUTOSTART,
    CFG_KEY_GPS_RATE,
    CFG_KEY_DEVICE_NAME,
    CFG_KEY_PAIRING_CODE,
    CFG_KEY_RPM_REDLINE,
    CFG_KEY_RPM_SHIFT_LIGHT,
    CFG_KEY_BATTERY_LOW,
    CFG_KEY_BATTERY_SHUTDOWN,
    CFG_KEY_COOLANT_WARN,
    CFG_KEY_COOLANT_CRIT,
    CFG_KEY_FUEL_MIN,
    CFG_KEY_CALIB_IMU,
    CFG_KEY_CALIB_SEA_LEVEL,
    CFG_KEY_ANIM_ENABLED,
} config_key_t;

typedef struct {
    uint16_t version;
    int16_t  gyro_offset[3];
    int16_t  accel_offset[3];
    uint32_t bmp_sea_level_pa;
    uint16_t adc_vref_mv;
    float    fuel_cal_liters[6];
    uint16_t fuel_cal_adc[6];
    uint16_t rpm_redline;
    uint16_t rpm_shift_light;
    float    battery_low_v;
    float    battery_shutdown_v;
    float    oil_pressure_warn;
    float    oil_pressure_high;
    float    coolant_temp_warn;
    float    coolant_temp_crit;
    float    fuel_min_l;
    uint8_t  log_interval_s;
    uint8_t  log_format;
    uint8_t  log_autostart;
    uint8_t  gps_rate_hz;
    uint8_t  display_mode;
    uint8_t  display_theme;
    uint8_t  display_brightness;
    char     device_name[32];
    uint32_t pairing_code;
    uint8_t  anim_enabled;
    uint8_t  rpm_needle_spring;
} device_config_t;

/* ============================================================
 * SD-Karten-Typen
 * ============================================================ */

typedef struct {
    bool     mounted;
    uint32_t card_size_mb;
    bool     write_protected;
    uint32_t free_space_mb;
    uint32_t errors;
    uint32_t bytes_written;
} sd_card_info_t;

typedef struct {
    char     name[256];
    bool     is_dir;
    uint32_t size;
} sd_dir_entry_t;

/* ============================================================
 * Logger-Typen
 * ============================================================ */

typedef enum {
    TOUR_IDLE    = 0,
    TOUR_ACTIVE  = 1,
    TOUR_PAUSED  = 2,
} tour_state_t;

typedef struct {
    uint32_t timestamp_ms;
    float    lat;
    float    lon;
    float    speed_kmh;
    float    course_deg;
    float    altitude_m;
    uint16_t rpm;
    float    temp_c;
    float    roll_deg;
    float    pitch_deg;
    float    fuel_l;
} log_entry_t;

typedef struct {
    uint32_t points_skipped;
    uint32_t points_written;
    uint32_t write_errors;
    uint32_t flush_count;
    uint32_t tours_started;
    uint32_t tours_stopped;
    uint32_t total_points;
} logger_stats_t;

typedef struct {
    tour_state_t state;
    uint32_t     start_time_ms;
    uint32_t     point_count;
    uint32_t     segment_count;
    bool         has_last_point;
    float        last_lat;
    float        last_lon;
    char         csv_path[SD_MAX_PATH_LEN];
    char         gpx_path[SD_MAX_PATH_LEN];
} tour_context_t;

/* ============================================================
 * Storage-Statistik
 * ============================================================ */

typedef struct {
    uint32_t bytes_written;
    uint32_t bytes_read;
    uint32_t write_errors;
    uint32_t read_errors;
    uint32_t mount_count;
    uint32_t unmount_count;
} storage_stats_t;

/* ============================================================
 * Externe Variablen
 * ============================================================ */

extern SemaphoreHandle_t spi_mutex;

/* ============================================================
 * Storage-API
 * ============================================================ */

void storage_init(void);
int storage_write(const char *path, const void *data, size_t len);
int storage_read(const char *path, void *buf, size_t max_len);
const storage_stats_t *storage_get_stats(void);
void storage_process(void);

/* ============================================================
 * SD-Karten-API
 * ============================================================ */

void sd_card_init(void);
bool sd_card_mount(void);
bool sd_card_unmount(void);
bool sd_card_is_mounted(void);
bool sd_card_is_present(void);
bool sd_card_is_write_protected(void);
const sd_card_info_t *sd_card_get_info(void);
int sd_card_mkdir(const char *path);
int sd_card_write_file(const char *path, const void *data, size_t len);
int sd_card_read_file(const char *path, void *buf, size_t max_len);
int sd_card_append_file(const char *path, const void *data, size_t len);
int sd_card_delete_file(const char *path);
bool sd_card_file_exists(const char *path);
int sd_card_list_dir(const char *path, sd_dir_entry_t *entries,
                      int max_entries, int *count_out);
uint32_t sd_card_get_free_space_mb(void);

/* ============================================================
 * Logger-API
 * ============================================================ */

void logger_init(void);
void logger_task(void *arg);
int logger_write_csv_header(FILE *f);
int logger_write_csv_row(FILE *f, const log_entry_t *entry);
int logger_export_gpx(const char *path);
int logger_start_tour(void);
int logger_stop_tour(void);
int logger_pause_tour(void);
int logger_resume_tour(void);
int logger_add_track_point(const log_entry_t *entry);
void logger_set_rate(uint8_t rate_hz);
int logger_flush(void);
const logger_stats_t *logger_get_stats(void);
const tour_context_t *logger_get_tour_context(void);

/* ============================================================
 * Konfigurations-API
 * ============================================================ */

void config_init(void);
int config_load(void *cfg, uint16_t max_len);
int config_save(const void *cfg, uint16_t len);
device_config_t *config_get(void);
esp_err_t config_load_all(void);
esp_err_t config_save_all(void);
esp_err_t config_migrate(uint16_t old_version);
esp_err_t config_factory_reset(void);
int config_set_value(config_key_t key, const void *value, uint8_t len);
int config_get_value(config_key_t key, void *value, uint8_t max_len);
int config_export_json(char *buf, size_t max_len);
int config_import_json(const char *json);

#endif /* _STORAGE_H_ */
