/*
 * tft_driver.c — TFT Display Treiber (SPI)
 *
 * Panel: 2.4" IPS, 240x320 Pixel, ILI9341 Controller.
 * SPI: MOSI=11, SCLK=12, CS=10, DC=14, RST=21.
 *
 * TODO: ILI9341 Init-Sequenz implementieren.
 * TODO: Double-Buffering: Front-Buffer im PSRAM, DMA-Transfer.
 * TODO: Backlight-PWM (falls backlight GPIO verfuegbar).
 */

#include "display.h"
#include "board_config.h"
#include "esp_log.h"
#include "driver/spi_master.h"

static const char *TAG = "tft";

/* Framebuffer: 240*320*2 Bytes (RGB565) = 153600 Bytes -> PSRAM */
#define TFT_BUF_SIZE  ((TFT_WIDTH * TFT_HEIGHT) * 2)

static uint8_t *tft_framebuffer = NULL;
static spi_device_handle_t tft_spi = NULL;

void tft_init(void)
{
    /* TODO: SPI-Geraet konfigurieren (shared Bus).
     *
     * spi_device_interface_config_t dev_config = {
     *     .clock_speed_hz = 40000000,  // 40MHz
     *     .mode = 0,
     *     .spics_io_num = DISPLAY_CS_PIN,
     *     .queue_size = 6,
     * };
     * spi_bus_add_device(SPI2_HOST, &dev_config, &tft_spi);
     *
     * // GPIOs
     * gpio_set_direction(DISPLAY_DC_PIN, GPIO_MODE_OUTPUT);
     * gpio_set_direction(DISPLAY_RST_PIN, GPIO_MODE_OUTPUT);
     *
     * // ILI9341 Init-Sequenz
     * // TODO: Siehe ILI9341 Datasheet / Adafruit_ILI9341 init code
     */

    /* Framebuffer im PSRAM */
    /* tft_framebuffer = heap_caps_malloc(TFT_BUF_SIZE, MALLOC_CAP_SPIRAM); */

    ESP_LOGI(TAG, "TFT init (stub), buffer=%d bytes", TFT_BUF_SIZE);
}

void tft_flush(void)
{
    /* TODO: Framebuffer per DMA an TFT senden.
     *
     * // Set column/page address window
     * tft_write_cmd(0x2A);  // Column Address Set
     * tft_write_data16(0, TFT_WIDTH - 1);
     * tft_write_cmd(0x2B);  // Page Address Set
     * tft_write_data16(0, TFT_HEIGHT - 1);
     * tft_write_cmd(0x2C);  // Memory Write
     *
     * // DMA-Transfer
     * spi_transaction_t t = {
     *     .tx_buffer = tft_framebuffer,
     *     .length = TFT_BUF_SIZE * 8,  // in bits
     * };
     * spi_device_polling_transmit(tft_spi, &t);
     */
}

uint8_t *tft_get_framebuffer(void)
{
    return tft_framebuffer;
}
