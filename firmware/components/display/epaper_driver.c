/*
 * epaper_driver.c — E-Paper Display Treiber (SPI)
 *
 * Panel: 2.9" b/w (GDEM029T94 / SSD1680), 296x128 Pixel.
 * SPI: MOSI=11, SCLK=12, CS=10, DC=14, RST=21, BUSY=47.
 *
 * TODO: GxEPD2 als ESP-IDF Component einbinden (als Arduino-Component).
 *       Alternativ: tuanpmt/esp_epaper (native ESP-IDF, LVGL-Support).
 *       Quelle: github.com/ZinggJM/GxEPD2
 *
 * TODO: Partial-Refresh fuer Live-Daten (Speed, RPM).
 * TODO: Full-Refresh alle N Updates gegen Ghosting.
 * TODO: LUT (Look-Up Table) fuer kalte Temperaturen anpassen.
 * TODO: SPI-Mutex (shared mit SD-Karte).
 */

#include "display.h"
#include "board_config.h"
#include "esp_log.h"
#include "driver/spi_master.h"

static const char *TAG = "epd";

#define EPD_BUF_SIZE  ((EPD_WIDTH * EPD_HEIGHT) / 8)  /* 4736 Bytes (1-Bit) */

/* Framebuffer im PSRAM */
static uint8_t *epd_framebuffer = NULL;

/* SPI-Handle */
static spi_device_handle_t epd_spi = NULL;

void epaper_init(void)
{
    /* TODO: SPI-Geraet hinzufuegen (shared Bus mit SD-Karte).
     *
     * spi_device_interface_config_t dev_config = {
     *     .clock_speed_hz = 4000000,   // 4MHz (E-Paper braucht nicht schnell)
     *     .mode = 0,
     *     .spics_io_num = DISPLAY_CS_PIN,
     *     .queue_size = 7,
     *     .flags = SPI_DEVICE_HALFDUPLEX,
     * };
     * spi_bus_add_device(SPI2_HOST, &dev_config, &epd_spi);
     *
     * // GPIOs: DC, RST, BUSY
     * gpio_set_direction(DISPLAY_DC_PIN, GPIO_MODE_OUTPUT);
     * gpio_set_direction(DISPLAY_RST_PIN, GPIO_MODE_OUTPUT);
     * gpio_set_direction(DISPLAY_BUSY_PIN, GPIO_MODE_INPUT);
     *
     * // Hardware-Reset
     * gpio_set_level(DISPLAY_RST_PIN, 0);
     * vTaskDelay(pdMS_TO_TICKS(10));
     * gpio_set_level(DISPLAY_RST_PIN, 1);
     * vTaskDelay(pdMS_TO_TICKS(200));
     *
     * // Init-Sequenz (SSD1680)
     * // TODO: Siehe GDEM029T94 Datasheet / GxEPD2 init code
     */

    /* Framebuffer im PSRAM allozieren */
    /* epd_framebuffer = heap_caps_malloc(EPD_BUF_SIZE, MALLOC_CAP_SPIRAM); */
    /* if (!epd_framebuffer) { ESP_LOGE(TAG, "PSRAM alloc failed"); return; } */

    ESP_LOGI(TAG, "E-Paper init (stub), buffer=%d bytes", EPD_BUF_SIZE);
}

void epaper_refresh(bool partial)
{
    /* TODO: Framebuffer ueber SPI an Display senden.
     *       Partial-Refresh: nur geaenderte Region.
     *       Full-Refresh: alle Pixel, flackert, gegen Ghosting.
     *
     * // Command: Write RAM (0x24)
     * epd_write_cmd(0x24);
     * epd_write_data(epd_framebuffer, EPD_BUF_SIZE);
     *
     * if (partial) {
     *     // Partial-Refresh aktivieren
     *     epd_write_cmd(0x22);  // Display Update Control 2
     *     epd_write_data_byte(0xFF);
     * } else {
     *     // Full-Refresh
     *     epd_write_cmd(0x22);
     *     epd_write_data_byte(0xC7);
     * }
     *
     * // Trigger Refresh
     * epd_write_cmd(0x20);
     *
     * // Warten bis BUSY losgeht
     * while (gpio_get_level(DISPLAY_BUSY_PIN) == 1) {
     *     vTaskDelay(pdMS_TO_TICKS(10));
     * }
     */

    (void)partial;
}

uint8_t *epaper_get_framebuffer(void)
{
    return epd_framebuffer;
}
