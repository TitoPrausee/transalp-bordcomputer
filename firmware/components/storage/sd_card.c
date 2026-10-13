/*
 * sd_card.c — MicroSD-Karte ueber SPI mounten (FATFS)
 *
 * SPI: MOSI=11, MISO=13, SCLK=12, CS=38 (shared mit Display).
 * Mount-Point: /sdcard
 *
 * TODO: SPI-Bus initialisieren (shared mit Display, SPI-Mutex!).
 * TODO: SDSPI-Device konfigurieren und FATFS mounten.
 * TODO: Verzeichnisstruktur erstellen: /sdcard/logs/, /sdcard/tracks/.
 *
 * Siehe docs/research/03-software-stack.md Abschnitt 6.1.
 */

#include "storage.h"
#include "board_config.h"
#include "event_bus.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/spi_common.h"
#include "sdspi_host.h"

static const char *TAG = "sdcard";

static bool s_mounted = false;
static sdmmc_card_t *s_card = NULL;

/* SPI-Mutex: shared zwischen Display und SD-Karte.
 * In den jeweiligen Task ueber xSemaphoreTake/Give. */
SemaphoreHandle_t spi_mutex = NULL;

bool sd_card_mount(void)
{
    /* TODO: SPI-Bus initialisieren und SD-Karte mounten.
     *
     * spi_bus_config_t bus_cfg = {
     *     .mosi_io_num = SPI_MOSI_PIN,
     *     .miso_io_num = SPI_MISO_PIN,
     *     .sclk_io_num = SPI_SCLK_PIN,
     *     .max_transfer_sz = 4096,
     * };
     * sdmmc_host_t host = SDSPI_HOST_DEFAULT();
     * ESP_ERROR_CHECK(spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA));
     *
     * sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
     * slot_config.gpio_cs = SD_CS_PIN;
     * slot_config.host_id = host.slot;
     *
     * esp_vfs_fat_sdmmc_mount_config_t mount_config = {
     *     .format_if_mount_failed = false,
     *     .max_files = 5,
     *     .allocation_unit_size = 16 * 1024,
     * };
     * ESP_ERROR_CHECK(esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host,
     *     &slot_config, &mount_config, &s_card));
     *
     * // Verzeichnisse erstellen
     * mkdir(SD_MOUNT_POINT "/logs", 0777);
     * mkdir(SD_MOUNT_POINT "/tracks", 0777);
     */

    /* Stub: always report success */
    s_mounted = true;
    event_bus_set(EVT_SDCARD_MOUNTED);
    ESP_LOGI(TAG, "SD card mounted (stub) at %s", SD_MOUNT_POINT);
    return true;
}

bool sd_card_is_mounted(void)
{
    return s_mounted;
}

void sd_card_init(void)
{
    /* SPI-Mutex erstellen (shared mit Display) */
    spi_mutex = xSemaphoreCreateMutex();
    if (!spi_mutex) {
        ESP_LOGE(TAG, "Failed to create SPI mutex");
        return;
    }

    sd_card_mount();
}
