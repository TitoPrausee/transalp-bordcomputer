/*
 * epaper_driver.c — E-Paper Display Treiber (SPI, SSD1680/UC8151D)
 *
 * Panel: 2.9" b/w (GDEM029T94 / SSD1680), 296x128 Pixel, 1-Bit monochrom.
 * SPI: MOSI=11, SCLK=12, CS=10, DC=14, RST=21, BUSY=47 (shared bus).
 *
 * Features:
 *   - SSD1680 Init-Sequenz
 *   - Full-Refresh: kompletter Bildschirm (gegen Ghosting)
 *   - Partial-Refresh: nur geaenderte Regionen (schneller, kein Flackern)
 *   - Busy-Wait Pin Handling
 *   - 1-Bit Framebuffer in PSRAM (296*128/8 = 4736 Bytes)
 *
 * Pin-Belegung siehe board_config.h.
 */

#include "display.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "string.h"

static const char *TAG = "epd";

/* ============================================================
 * Constants
 * ============================================================ */

#define EPD_BUF_SIZE   ((EPD_WIDTH * EPD_HEIGHT) / 8)  /* 4736 Bytes (1-Bit) */
#define EPD_SPI_FREQ   4000000   /* 4 MHz — E-Paper needs slow SPI */
#define EPD_TIMEOUT_MS 3000      /* Busy-wait timeout */

/* SSD1680 commands */
#define EPD_DRIVER_OUTPUT       0x01
#define EPD_GATE_DRIVING_VOLT   0x03
#define EPD_SOURCE_DRIVING_VOLT 0x04
#define EPD_BOOSTER_SOFT_START  0x0C
#define EPD_GATE_SCAN_START     0x0F
#define EPD_DEEP_SLEEP          0x10
#define EPD_DATA_ENTRY_MODE     0x11
#define EPD_SW_RESET            0x12
#define EPD_TEMP_SENSOR         0x1A
#define EPD_MASTER_ACTIVATION   0x20
#define EPD_UPDATE_CTRL2        0x22
#define EPD_WRITE_RAM           0x24
#define EPD_WRITE_RED_RAM       0x26
#define EPD_RAM_X_START         0x44
#define EPD_RAM_X_END           0x44
#define EPD_RAM_Y_START         0x45
#define EPD_RAM_Y_END           0x45
#define EPD_BORDER_WAVEFORM     0x3C
#define EPD_VCOM_VOLTAGE        0x2C
#define EPD_WRITE_LUT           0x32
#define EPD_STATUS              0x1F

/* Update sequence modes for CTRL2 (0x22) */
#define EPD_UPD_FULL            0xC7   /* Full refresh: clk -> CP -> LUT -> pattern */
#define EPD_UPD_PARTIAL         0xFF   /* Partial refresh */
#define EPD_UPD_NONE            0x03   /* Disable clock, CP, LUT */

/* ============================================================
 * State
 * ============================================================ */

static spi_device_handle_t s_spi = NULL;
static uint8_t *s_framebuffer = NULL;    /* PSRAM */
static uint8_t *s_prev_buffer  = NULL;   /* PSRAM — previous frame for diff */
static bool     s_initialized = false;

/* ============================================================
 * Low-level SPI helpers
 * ============================================================ */

static void epd_dc_command(void)
{
    gpio_set_level(DISPLAY_DC_PIN, 0);
}

static void epd_dc_data(void)
{
    gpio_set_level(DISPLAY_DC_PIN, 1);
}

static esp_err_t epd_write_cmd(uint8_t cmd)
{
    spi_transaction_t t = {0};
    t.length    = 8;
    t.tx_buffer = &cmd;
    t.flags     = SPI_TRANS_USE_TXSINGLE;
    epd_dc_command();
    return spi_device_polling_transmit(s_spi, &t);
}

static esp_err_t epd_write_data(const uint8_t *data, size_t len)
{
    if (len == 0) return ESP_OK;
    spi_transaction_t t = {0};
    t.length    = len * 8;
    t.tx_buffer = data;
    epd_dc_data();
    return spi_device_polling_transmit(s_spi, &t);
}

static esp_err_t epd_write_cmd_data(uint8_t cmd, const uint8_t *data, size_t len)
{
    esp_err_t ret = epd_write_cmd(cmd);
    if (ret != ESP_OK) return ret;
    return epd_write_data(data, len);
}

/* ============================================================
 * Busy-Wait Handling
 * ============================================================ */

bool epaper_is_busy(void)
{
    return gpio_get_level(DISPLAY_BUSY_PIN) == 1;  /* HIGH = busy */
}

static void epd_wait_busy(void)
{
    uint32_t start = xTaskGetTickCount();
    while (gpio_get_level(DISPLAY_BUSY_PIN) == 1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if ((xTaskGetTickCount() - start) * portTICK_PERIOD_MS
            > EPD_TIMEOUT_MS) {
            ESP_LOGW(TAG, "BUSY timeout (%d ms)", EPD_TIMEOUT_MS);
            break;
        }
    }
}

/* ============================================================
 * Hardware Reset
 * ============================================================ */

static void epd_hardware_reset(void)
{
    gpio_set_level(DISPLAY_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(DISPLAY_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    /* Wait for busy to go low */
    epd_wait_busy();
}

/* ============================================================
 * SSD1680 Init Sequence
 * ============================================================ */

static void epd_init_sequence(void)
{
    /* Driver output control: 128 lines, scan direction */
    uint8_t driver_out[] = {0x80, 0x00, 0x00};  /* MUX=128 */
    epd_write_cmd_data(EPD_DRIVER_OUTPUT, driver_out, 3);

    /* Gate driving voltage: 15V */
    uint8_t gate_volt[] = {0x0A};
    epd_write_cmd_data(EPD_GATE_DRIVING_VOLT, gate_volt, 1);

    /* Source driving voltage: 15V */
    uint8_t src_volt[] = {0x0A, 0x00, 0x0A};
    epd_write_cmd_data(EPD_SOURCE_DRIVING_VOLT, src_volt, 3);

    /* Booster soft start: phase A=10ms, B=10ms, C=5ms */
    uint8_t booster[] = {0x8A, 0x05, 0x1A};
    epd_write_cmd_data(EPD_BOOSTER_SOFT_START, booster, 3);

    /* VCOM voltage: -0.1V */
    uint8_t vcom[] = {0x1A};
    epd_write_cmd_data(EPD_VCOM_VOLTAGE, vcom, 1);

    /* Border waveform: don't change border */
    uint8_t border[] = {0x03};
    epd_write_cmd_data(EPD_BORDER_WAVEFORM, border, 1);

    /* Data entry mode: X increment, Y increment */
    uint8_t entry_mode[] = {0x03};  /* AM=0 (X), ID[1:0]=11 (inc X, inc Y) */
    epd_write_cmd_data(EPD_DATA_ENTRY_MODE, entry_mode, 1);

    /* RAM X address: 0 to (EPD_WIDTH/8 - 1) */
    uint8_t ram_x[] = {0x00, (uint8_t)((EPD_WIDTH / 8) - 1)};
    epd_write_cmd_data(EPD_RAM_X_START, ram_x, 2);

    /* RAM Y address: 0 to (EPD_HEIGHT - 1) */
    uint8_t ram_y[] = {0x00, 0x00, (uint8_t)((EPD_HEIGHT - 1) & 0xFF),
                       (uint8_t)((EPD_HEIGHT - 1) >> 8)};
    epd_write_cmd_data(EPD_RAM_Y_START, ram_y, 4);
}

/* ============================================================
 * Public API
 * ============================================================ */

void epaper_init(void)
{
    if (s_initialized) return;

    esp_err_t ret;

    /* GPIOs: DC, RST (output); BUSY (input) */
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << DISPLAY_DC_PIN) |
                        (1ULL << DISPLAY_RST_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&out_conf);

    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL << DISPLAY_BUSY_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&in_conf);

    /* SPI device — half-duplex for e-paper */
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = EPD_SPI_FREQ,
        .mode           = 0,
        .spics_io_num   = DISPLAY_CS_PIN,
        .queue_size     = 7,
        .flags          = SPI_DEVICE_HALFDUPLEX,
    };
    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &s_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(ret));
        return;
    }

    /* Framebuffer + prev-buffer in PSRAM */
    s_framebuffer = heap_caps_malloc(EPD_BUF_SIZE, MALLOC_CAP_SPIRAM);
    s_prev_buffer  = heap_caps_malloc(EPD_BUF_SIZE, MALLOC_CAP_SPIRAM);
    if (!s_framebuffer || !s_prev_buffer) {
        ESP_LOGE(TAG, "PSRAM alloc failed (fb=%p prev=%p)",
                 s_framebuffer, s_prev_buffer);
        return;
    }
    memset(s_framebuffer, 0xFF, EPD_BUF_SIZE);   /* white */
    memset(s_prev_buffer, 0xFF, EPD_BUF_SIZE);

    /* Hardware reset + init */
    epd_hardware_reset();
    epd_init_sequence();

    s_initialized = true;
    ESP_LOGI(TAG, "E-Paper init done, buffer=%d bytes", EPD_BUF_SIZE);
}

void epaper_clear(void)
{
    if (!s_initialized) return;

    if (!spi_bus_lock(pdMS_TO_TICKS(100))) {
        ESP_LOGW(TAG, "SPI bus lock timeout");
        return;
    }

    memset(s_framebuffer, 0xFF, EPD_BUF_SIZE);   /* all white */

    /* Write to display RAM */
    epd_write_cmd(EPD_WRITE_RAM);
    epd_write_data(s_framebuffer, EPD_BUF_SIZE);

    /* Full refresh */
    uint8_t upd = EPD_UPD_FULL;
    epd_write_cmd_data(EPD_UPDATE_CTRL2, &upd, 1);
    epd_write_cmd(EPD_MASTER_ACTIVATION);
    epd_wait_busy();

    spi_bus_unlock();

    memcpy(s_prev_buffer, s_framebuffer, EPD_BUF_SIZE);
}

void epaper_display_frame(const uint8_t *buffer)
{
    if (!s_initialized) return;
    if (!buffer) buffer = s_framebuffer;

    if (!spi_bus_lock(pdMS_TO_TICKS(100))) {
        ESP_LOGW(TAG, "SPI bus lock timeout");
        return;
    }

    /* Copy buffer to internal framebuffer */
    memcpy(s_framebuffer, buffer, EPD_BUF_SIZE);

    /* Write to display RAM */
    epd_write_cmd(EPD_WRITE_RAM);
    epd_write_data(s_framebuffer, EPD_BUF_SIZE);

    /* Full refresh */
    uint8_t upd = EPD_UPD_FULL;
    epd_write_cmd_data(EPD_UPDATE_CTRL2, &upd, 1);
    epd_write_cmd(EPD_MASTER_ACTIVATION);
    epd_wait_busy();

    spi_bus_unlock();

    memcpy(s_prev_buffer, s_framebuffer, EPD_BUF_SIZE);
}

void epaper_display_partial(const uint8_t *buffer, uint16_t x,
                            uint16_t y, uint16_t w, uint16_t h)
{
    if (!s_initialized) return;
    if (!buffer || w == 0 || h == 0) return;

    /* Align x to byte boundary (8 pixels per byte in 1-bit mode) */
    uint16_t x_byte = x / 8;
    uint16_t w_bytes = (w + 7) / 8;
    if (x_byte + w_bytes > EPD_WIDTH / 8) {
        w_bytes = EPD_WIDTH / 8 - x_byte;
    }
    if (y + h > EPD_HEIGHT) {
        h = EPD_HEIGHT - y;
    }

    if (!spi_bus_lock(pdMS_TO_TICKS(100))) {
        ESP_LOGW(TAG, "SPI bus lock timeout");
        return;
    }

    /* Update internal framebuffer with the region */
    for (uint16_t row = 0; row < h; row++) {
        uint16_t dst_y = y + row;
        if (dst_y >= EPD_HEIGHT) break;
        uint8_t *dst = &s_framebuffer[dst_y * (EPD_WIDTH / 8) + x_byte];
        const uint8_t *src =
            &buffer[row * w_bytes];
        memcpy(dst, src, w_bytes);
    }

    /* Set RAM window for the changed region */
    uint8_t ram_x[] = {(uint8_t)x_byte,
                       (uint8_t)(x_byte + w_bytes - 1)};
    epd_write_cmd_data(EPD_RAM_X_START, ram_x, 2);

    uint8_t ram_y[] = {(uint8_t)(y & 0xFF), (uint8_t)(y >> 8),
                       (uint8_t)((y + h - 1) & 0xFF),
                       (uint8_t)((y + h - 1) >> 8)};
    epd_write_cmd_data(EPD_RAM_Y_START, ram_y, 4);

    /* Data entry: X increment, Y decrement (SSD1680 quirk) */
    uint8_t entry_mode[] = {0x02};
    epd_write_cmd_data(EPD_DATA_ENTRY_MODE, entry_mode, 1);

    /* Write region data to RAM */
    epd_write_cmd(EPD_WRITE_RAM);
    for (int16_t row = h - 1; row >= 0; row--) {
        uint16_t src_y = y + row;
        if (src_y >= EPD_HEIGHT) continue;
        uint8_t *line = &s_framebuffer[src_y * (EPD_WIDTH / 8) + x_byte];
        epd_write_data(line, w_bytes);
    }

    /* Partial refresh */
    uint8_t upd = EPD_UPD_PARTIAL;
    epd_write_cmd_data(EPD_UPDATE_CTRL2, &upd, 1);
    epd_write_cmd(EPD_MASTER_ACTIVATION);
    epd_wait_busy();

    spi_bus_unlock();

    memcpy(s_prev_buffer, s_framebuffer, EPD_BUF_SIZE);
}

uint8_t *epaper_get_framebuffer(void)
{
    return s_framebuffer;
}
