/*
 * sd_card.c -- MicroSD-Karten Treiber ueber SPI (FATFS)
 *
 * SPI-Bus: Shared mit Display (FSPI / SPI2_HOST).
 * Pins: MOSI=11, MISO=13, SCLK=12, CS=38 (board_config.h).
 * Mount-Point: /sdcard
 *
 * Verzeichnisstruktur:
 *   /sdcard/logs/   -- CSV-Logdateien
 *   /sdcard/tracks/ -- GPX-Trackdateien
 *   /sdcard/config/ -- JSON-Konfiguration (Companion-App)
 */

#include "storage.h"
#include "board_config.h"
#include "event_bus.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

static const char *TAG = "sdcard";

/* ---- Interne Zustandsvariablen ---- */
static bool s_mounted = false;
static sdmmc_card_t *s_card = NULL;
static sd_card_info_t s_info;
static SemaphoreHandle_t s_file_mutex = NULL;

/* SPI-Mutex: shared zwischen Display und SD-Karte.
 * Wird beim ersten Zugriff auf den Bus genommen. */
SemaphoreHandle_t spi_mutex = NULL;

/* ---- Hilfsfunktionen ---- */

static void update_card_info(void)
{
    memset(&s_info, 0, sizeof(s_info));
    s_info.mounted = s_mounted;

    if (s_card) {
        s_info.card_size_mb = (uint32_t)(s_card->csd.capacity / 1024);
        s_info.write_protected = false;

        FATFS *fs;
        DWORD free_clust;
        if (f_getfree("0:", &free_clust, &fs) == FR_OK) {
            uint32_t free_sectors = free_clust * fs->csize;
            s_info.free_space_mb = free_sectors / 2048;
        }
    }
}

static void ensure_directories(void)
{
    char path[SD_MAX_PATH_LEN];

    snprintf(path, sizeof(path), "%s/logs", SD_MOUNT_POINT);
    mkdir(path, 0777);

    snprintf(path, sizeof(path), "%s/tracks", SD_MOUNT_POINT);
    mkdir(path, 0777);

    snprintf(path, sizeof(path), "%s/config", SD_MOUNT_POINT);
    mkdir(path, 0777);
}

/* ---- Oeffentliche API ---- */

void sd_card_init(void)
{
    if (!spi_mutex) {
        spi_mutex = xSemaphoreCreateMutex();
        if (!spi_mutex) {
            ESP_LOGE(TAG, "Failed to create SPI mutex");
            return;
        }
    }

    if (!s_file_mutex) {
        s_file_mutex = xSemaphoreCreateMutex();
        if (!s_file_mutex) {
            ESP_LOGE(TAG, "Failed to create file mutex");
            return;
        }
    }

    memset(&s_info, 0, sizeof(s_info));
    sd_card_mount();
}

bool sd_card_mount(void)
{
    if (s_mounted) {
        ESP_LOGW(TAG, "SD card already mounted");
        return true;
    }

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SPI_MOSI_PIN,
        .miso_io_num = SPI_MISO_PIN,
        .sclk_io_num = SPI_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;

    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to init SPI bus: %s", esp_err_to_name(ret));
        s_info.errors++;
        event_bus_set(EVT_SDCARD_ERROR);
        return false;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = SD_MAX_FILES,
        .allocation_unit_size = 16 * 1024,
    };

    ESP_LOGI(TAG, "Mounting SD card (CS=%d, MOSI=%d, MISO=%d, SCLK=%d)...",
             SD_CS_PIN, SPI_MOSI_PIN, SPI_MISO_PIN, SPI_SCLK_PIN);

    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host,
                                   &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGW(TAG, "SD card not found (no card inserted?)");
        } else {
            ESP_LOGE(TAG, "Mount failed: %s", esp_err_to_name(ret));
        }
        s_info.errors++;
        event_bus_set(EVT_SDCARD_ERROR);
        s_card = NULL;
        return false;
    }

    s_mounted = true;
    update_card_info();
    ensure_directories();

    event_bus_set(EVT_SDCARD_MOUNTED);
    ESP_LOGI(TAG, "SD card mounted: %lu MB (%lu MB free)",
             (unsigned long)s_info.card_size_mb,
             (unsigned long)s_info.free_space_mb);

    return true;
}

bool sd_card_unmount(void)
{
    if (!s_mounted) {
        return true;
    }

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Unmount failed: %s", esp_err_to_name(ret));
        s_info.errors++;
        return false;
    }

    s_mounted = false;
    s_card = NULL;
    s_info.mounted = false;
    event_bus_clear(EVT_SDCARD_MOUNTED);

    ESP_LOGI(TAG, "SD card unmounted");
    return true;
}

bool sd_card_is_mounted(void)
{
    return s_mounted;
}

bool sd_card_is_present(void)
{
    if (s_mounted) {
        return true;
    }
    if (s_info.errors > 0 && s_info.errors < 10) {
        return sd_card_mount();
    }
    return false;
}

bool sd_card_is_write_protected(void)
{
    return false;
}

const sd_card_info_t *sd_card_get_info(void)
{
    if (s_mounted) {
        update_card_info();
    }
    return &s_info;
}

/* ---- Datei-Operationen ---- */

int sd_card_mkdir(const char *path)
{
    if (!path) {
        return SD_ERR_INVALID_ARG;
    }
    if (!s_mounted) {
        return SD_ERR_NOT_MOUNTED;
    }

    char full[SD_MAX_PATH_LEN];
    snprintf(full, sizeof(full), "%s/%s", SD_MOUNT_POINT, path);

    if (mkdir(full, 0777) == 0 || errno == EEXIST) {
        return SD_OK;
    }

    ESP_LOGW(TAG, "mkdir '%s' failed: %s", full, strerror(errno));
    s_info.errors++;
    return SD_ERR_OPEN_FAILED;
}

int sd_card_write_file(const char *path, const void *data, size_t len)
{
    if (!path || (!data && len > 0)) {
        return SD_ERR_INVALID_ARG;
    }
    if (!s_mounted) {
        return SD_ERR_NOT_MOUNTED;
    }

    char full[SD_MAX_PATH_LEN];
    snprintf(full, sizeof(full), "%s/%s", SD_MOUNT_POINT, path);

    if (!s_file_mutex || xSemaphoreTake(s_file_mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        return SD_ERR_OPEN_FAILED;
    }

    FILE *f = fopen(full, "wb");
    if (!f) {
        ESP_LOGE(TAG, "open('%s', 'wb') failed: %s", full, strerror(errno));
        s_info.errors++;
        xSemaphoreGive(s_file_mutex);
        return SD_ERR_OPEN_FAILED;
    }

    size_t written = fwrite(data, 1, len, f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    xSemaphoreGive(s_file_mutex);

    if (written != len) {
        ESP_LOGE(TAG, "Short write: %u/%u", (unsigned)written, (unsigned)len);
        s_info.errors++;
        return SD_ERR_WRITE_FAILED;
    }

    s_info.bytes_written += (uint32_t)len;
    return SD_OK;
}

int sd_card_read_file(const char *path, void *buf, size_t max_len)
{
    if (!path || !buf || max_len == 0) {
        return SD_ERR_INVALID_ARG;
    }
    if (!s_mounted) {
        return SD_ERR_NOT_MOUNTED;
    }

    char full[SD_MAX_PATH_LEN];
    snprintf(full, sizeof(full), "%s/%s", SD_MOUNT_POINT, path);

    if (!s_file_mutex || xSemaphoreTake(s_file_mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        return SD_ERR_READ_FAILED;
    }

    FILE *f = fopen(full, "rb");
    if (!f) {
        xSemaphoreGive(s_file_mutex);
        if (errno == ENOENT) {
            return SD_ERR_NOT_FOUND;
        }
        ESP_LOGE(TAG, "open('%s', 'rb') failed: %s", full, strerror(errno));
        s_info.errors++;
        return SD_ERR_OPEN_FAILED;
    }

    size_t n = fread(buf, 1, max_len, f);
    fclose(f);
    xSemaphoreGive(s_file_mutex);

    return (int)n;
}

int sd_card_append_file(const char *path, const void *data, size_t len)
{
    if (!path || (!data && len > 0)) {
        return SD_ERR_INVALID_ARG;
    }
    if (!s_mounted) {
        return SD_ERR_NOT_MOUNTED;
    }

    char full[SD_MAX_PATH_LEN];
    snprintf(full, sizeof(full), "%s/%s", SD_MOUNT_POINT, path);

    if (!s_file_mutex || xSemaphoreTake(s_file_mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        return SD_ERR_OPEN_FAILED;
    }

    FILE *f = fopen(full, "ab");
    if (!f) {
        ESP_LOGE(TAG, "open('%s', 'ab') failed: %s", full, strerror(errno));
        s_info.errors++;
        xSemaphoreGive(s_file_mutex);
        return SD_ERR_OPEN_FAILED;
    }

    size_t written = fwrite(data, 1, len, f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    xSemaphoreGive(s_file_mutex);

    if (written != len) {
        ESP_LOGE(TAG, "Short append write: %u/%u", (unsigned)written, (unsigned)len);
        s_info.errors++;
        return SD_ERR_WRITE_FAILED;
    }

    s_info.bytes_written += (uint32_t)len;
    return SD_OK;
}

int sd_card_delete_file(const char *path)
{
    if (!path) {
        return SD_ERR_INVALID_ARG;
    }
    if (!s_mounted) {
        return SD_ERR_NOT_MOUNTED;
    }

    char full[SD_MAX_PATH_LEN];
    snprintf(full, sizeof(full), "%s/%s", SD_MOUNT_POINT, path);

    if (unlink(full) == 0) {
        return SD_OK;
    }
    if (errno == ENOENT) {
        return SD_ERR_NOT_FOUND;
    }

    ESP_LOGE(TAG, "unlink('%s') failed: %s", full, strerror(errno));
    s_info.errors++;
    return SD_ERR_OPEN_FAILED;
}

bool sd_card_file_exists(const char *path)
{
    if (!path || !s_mounted) {
        return false;
    }

    char full[SD_MAX_PATH_LEN];
    snprintf(full, sizeof(full), "%s/%s", SD_MOUNT_POINT, path);

    struct stat st;
    return (stat(full, &st) == 0);
}

int sd_card_list_dir(const char *path, sd_dir_entry_t *entries,
                      int max_entries, int *count_out)
{
    if (!path || !entries || max_entries <= 0) {
        return SD_ERR_INVALID_ARG;
    }
    if (!s_mounted) {
        return SD_ERR_NOT_MOUNTED;
    }

    char full[SD_MAX_PATH_LEN];
    snprintf(full, sizeof(full), "%s/%s", SD_MOUNT_POINT, path);

    DIR *dir = opendir(full);
    if (!dir) {
        ESP_LOGE(TAG, "opendir('%s') failed: %s", full, strerror(errno));
        s_info.errors++;
        return SD_ERR_OPEN_FAILED;
    }

    int count = 0;
    struct dirent *de;
    while ((de = readdir(dir)) != NULL && count < max_entries) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }

        strncpy(entries[count].name, de->d_name, sizeof(entries[count].name) - 1);
        entries[count].name[sizeof(entries[count].name) - 1] = '\0';
        entries[count].is_dir = (de->d_type == DT_DIR);

        char fpath[SD_MAX_PATH_LEN];
        snprintf(fpath, sizeof(fpath), "%s/%s", full, de->d_name);
        struct stat st;
        if (stat(fpath, &st) == 0) {
            entries[count].size = (uint32_t)st.st_size;
        } else {
            entries[count].size = 0;
        }

        count++;
    }

    closedir(dir);

    if (count_out) {
        *count_out = count;
    }

    return SD_OK;
}

uint32_t sd_card_get_free_space_mb(void)
{
    if (!s_mounted) {
        return 0;
    }

    FATFS *fs;
    DWORD free_clust;

    if (f_getfree("0:", &free_clust, &fs) != FR_OK) {
        return 0;
    }

    uint32_t free_sectors = free_clust * fs->csize;
    return free_sectors / 2048;
}
