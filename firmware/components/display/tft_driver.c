/*
 * tft_driver.c — TFT Display Treiber (SPI, ILI9341/ST7789)
 *
 * Panel: 2.4" IPS, 240x320 Pixel, RGB565, ILI9341 Controller.
 * SPI: MOSI=11, SCLK=12, CS=10, DC=14, RST=21 (shared bus mit SD + E-Paper).
 *
 * Features:
 *   - ESP-IDF SPI Master mit DMA
 *   - Double-Buffering: Front (Display) / Back (Paint) in PSRAM
 *   - DMA-Bounce-Buffer im internen SRAM (SPI-DMA kann nicht von PSRAM lesen)
 *   - Dirty-Region Push (nur geaenderte Bereiche)
 *   - Backlight-PWM Steuerung
 *
 * Pin-Belegung siehe board_config.h.
 */

#include "display.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "string.h"

static const char *TAG = "tft";

/* ============================================================
 * Constants
 * ============================================================ */

#define TFT_BUF_SIZE        ((TFT_WIDTH * TFT_HEIGHT) * 2)   /* 153600 Bytes */
#define TFT_BOUNCE_LINES    40
#define TFT_BOUNCE_SIZE     (TFT_WIDTH * TFT_BOUNCE_LINES * 2)
#define TFT_SPI_FREQ_HZ     40000000   /* 40 MHz */
#define TFT_BACKLIGHT_GPIO  48         /* LED PWM pin (board-specific) */
#define TFT_BACKLIGHT_CH    LEDC_CHANNEL_0

/* ILI9341 commands */
#define ILI9341_SWRESET     0x01
#define ILI9341_SLPOUT      0x11
#define ILI9341_GAMMASET    0x26
#define ILI9341_DISPON      0x29
#define ILI9341_CASET       0x2A
#define ILI9341_PASET       0x2B
#define ILI9341_RAMWR       0x2C
#define ILI9341_MADCTL      0x36
#define ILI9341_PIXFMT      0x3A
#define ILI9341_FRMCTR1     0xB1
#define ILI9341_DFUNCTR     0xB6
#define ILI9341_PWCTR1      0xC0
#define ILI9341_PWCTR2      0xC1
#define ILI9341_VMCTR1      0xC5
#define ILI9341_VMCTR2      0xC7
#define ILI9341_GMCTRP1     0xE0
#define ILI9341_GMCTRN1     0xE1

/* MADCTL bits */
#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x08

/* ============================================================
 * State
 * ============================================================ */

static spi_device_handle_t s_spi = NULL;
static uint16_t *s_fb_front = NULL;   /* PSRAM — was auf dem Display steht */
static uint16_t *s_fb_back  = NULL;   /* PSRAM — Paint-Buffer */
static uint16_t *s_bounce   = NULL;   /* SRAM — DMA-faehig */
static bool      s_initialized = false;
static uint16_t  s_backlight_pct = 0;

/* ============================================================
 * Low-level SPI helpers
 * ============================================================ */

static void tft_dc_command(void)
{
    gpio_set_level(DISPLAY_DC_PIN, 0);
}

static void tft_dc_data(void)
{
    gpio_set_level(DISPLAY_DC_PIN, 1);
}

static esp_err_t tft_write_cmd(uint8_t cmd)
{
    spi_transaction_t t = {0};
    t.length    = 8;
    t.tx_buffer = &cmd;
    t.flags     = SPI_TRANS_USE_TXSINGLE;
    tft_dc_command();
    return spi_device_polling_transmit(s_spi, &t);
}

static esp_err_t tft_write_data(const uint8_t *data, size_t len)
{
    if (len == 0) return ESP_OK;
    spi_transaction_t t = {0};
    t.length    = len * 8;
    t.tx_buffer = data;
    tft_dc_data();
    return spi_device_polling_transmit(s_spi, &t);
}

static esp_err_t tft_write_cmd_data(uint8_t cmd, const uint8_t *data, size_t len)
{
    esp_err_t ret = tft_write_cmd(cmd);
    if (ret != ESP_OK) return ret;
    return tft_write_data(data, len);
}

/* ============================================================
 * ILI9341 Init Sequence
 * ============================================================ */

static const uint8_t s_init_gamma_pos[] = {
    0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
    0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00
};
static const uint8_t s_init_gamma_neg[] = {
    0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
    0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F
};

static void tft_init_sequence(void)
{
    /* Hardware reset */
    gpio_set_level(DISPLAY_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(DISPLAY_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    tft_write_cmd(ILI9341_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(5));

    tft_write_cmd(ILI9341_SLPOUT);      /* Exit sleep */
    vTaskDelay(pdMS_TO_TICKS(120));

    /* Pixel format: RGB565 (16-bit/pixel) */
    uint8_t pixfmt = 0x55;              /* 16-bit both MCU and IF */
    tft_write_cmd_data(ILI9341_PIXFMT, &pixfmt, 1);

    /* MADCTL: row/col exchange for landscape, RGB order */
    uint8_t madctl = MADCTL_MV | MADCTL_MX | MADCTL_RGB; /* landscape 320x240 */
    tft_write_cmd_data(ILI9341_MADCTL, &madctl, 1);

    /* Frame rate control */
    uint8_t frmctr1[] = {0x00, 0x06, 0x03};
    tft_write_cmd_data(ILI9341_FRMCTR1, frmctr1, 3);

    /* Display function control */
    uint8_t dfunctr[] = {0x0A, 0x82, 0x27, 0x00};
    tft_write_cmd_data(ILI9341_DFUNCTR, dfunctr, 4);

    /* Power control */
    uint8_t pwctr1[] = {0x23};          /* GVDD=4.6V */
    tft_write_cmd_data(ILI9341_PWCTR1, pwctr1, 1);
    uint8_t pwctr2[] = {0x10};          /* VGH=2x, VGL=-1x */
    tft_write_cmd_data(ILI9341_PWCTR2, pwctr2, 1);

    /* VCOM voltage */
    uint8_t vmctr1[] = {0x3E, 0x28};
    tft_write_cmd_data(ILI9341_VMCTR1, vmctr1, 2);
    uint8_t vmctr2[] = {0x86};
    tft_write_cmd_data(ILI9341_VMCTR2, vmctr2, 1);

    /* Gamma set: curve 1 */
    uint8_t gammaset = 0x01;
    tft_write_cmd_data(ILI9341_GAMMASET, &gammaset, 1);

    /* Gamma correction */
    tft_write_cmd_data(ILI9341_GMCTRP1, s_init_gamma_pos,
                       sizeof(s_init_gamma_pos));
    tft_write_cmd_data(ILI9341_GMCTRN1, s_init_gamma_neg,
                       sizeof(s_init_gamma_neg));

    tft_write_cmd(ILI9341_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));
    tft_write_cmd(ILI9341_DISPON);      /* Display ON */
}

/* ============================================================
 * Backlight PWM
 * ============================================================ */

static void tft_backlight_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .freq_hz         = 5000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_cfg);

    ledc_channel_config_t ch_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = TFT_BACKLIGHT_CH,
        .timer_sel  = LEDC_TIMER_0,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = TFT_BACKLIGHT_GPIO,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&ch_cfg);
}

void tft_set_backlight(uint8_t percent)
{
    if (percent > 100) percent = 100;
    uint32_t duty = (uint32_t)percent * 1023 / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, TFT_BACKLIGHT_CH, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, TFT_BACKLIGHT_CH);
    s_backlight_pct = percent;
}

uint16_t tft_get_backlight(void)
{
    return s_backlight_pct;
}

/* ============================================================
 * Public API
 * ============================================================ */

void tft_init(void)
{
    if (s_initialized) return;

    esp_err_t ret;

    /* GPIOs for DC and RST */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DISPLAY_DC_PIN) |
                        (1ULL << DISPLAY_RST_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    /* SPI device config */
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = TFT_SPI_FREQ_HZ,
        .mode           = 0,            /* CPOL=0, CPHA=0 */
        .spics_io_num   = DISPLAY_CS_PIN,
        .queue_size     = 6,
        .flags          = SPI_DEVICE_NO_DUMMY,
    };
    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &s_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(ret));
        return;
    }

    /* Framebuffers in PSRAM (double-buffer) */
    s_fb_front = heap_caps_malloc(TFT_BUF_SIZE, MALLOC_CAP_SPIRAM);
    s_fb_back  = heap_caps_malloc(TFT_BUF_SIZE, MALLOC_CAP_SPIRAM);
    s_bounce   = heap_caps_malloc(TFT_BOUNCE_SIZE,
                                  MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!s_fb_front || !s_fb_back || !s_bounce) {
        ESP_LOGE(TAG, "PSRAM/SRAM alloc failed (front=%p back=%p bounce=%p)",
                 s_fb_front, s_fb_back, s_bounce);
        return;
    }
    memset(s_fb_front, 0x00, TFT_BUF_SIZE);   /* black */
    memset(s_fb_back, 0x00, TFT_BUF_SIZE);
    memset(s_bounce, 0x00, TFT_BOUNCE_SIZE);

    /* Backlight PWM */
    tft_backlight_init();

    /* ILI9341 init sequence */
    tft_init_sequence();

    /* Ramp backlight to 80% */
    tft_set_backlight(80);

    s_initialized = true;
    ESP_LOGI(TAG, "TFT init done, fb=%d bytes, bounce=%d bytes",
             TFT_BUF_SIZE, TFT_BOUNCE_SIZE);
}

uint8_t *tft_get_framebuffer(void)
{
    return (uint8_t *)s_fb_back;
}

void tft_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    if (!s_fb_back) return;
    s_fb_back[y * TFT_WIDTH + x] = color;
}

void tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                   uint16_t color)
{
    if (!s_fb_back) return;
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    if (x + w > TFT_WIDTH)  w = TFT_WIDTH - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;

    for (uint16_t row = 0; row < h; row++) {
        uint16_t *line = &s_fb_back[(y + row) * TFT_WIDTH + x];
        for (uint16_t col = 0; col < w; col++) {
            line[col] = color;
        }
    }
}

void tft_draw_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                     const uint8_t *data)
{
    if (!s_fb_back || !data) return;
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    if (x + w > TFT_WIDTH)  w = TFT_WIDTH - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;

    size_t idx = 0;
    for (uint16_t row = 0; row < h; row++) {
        uint16_t *line = &s_fb_back[(y + row) * TFT_WIDTH + x];
        for (uint16_t col = 0; col < w; col++) {
            /* data is RGB565 little-endian: low byte first */
            line[col] = (uint16_t)data[idx] | ((uint16_t)data[idx + 1] << 8);
            idx += 2;
        }
    }
}

/* Swap front and back buffers (exposed for renderer) */
void tft_swap_buffers(void)
{
    uint16_t *tmp = s_fb_front;
    s_fb_front = s_fb_back;
    s_fb_back  = tmp;
}

/* Set the address window for a write region */
static void tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t col_data[4];
    uint8_t row_data[4];
    col_data[0] = (x0 >> 8) & 0xFF;
    col_data[1] = x0 & 0xFF;
    col_data[2] = (x1 >> 8) & 0xFF;
    col_data[3] = x1 & 0xFF;
    row_data[0] = (y0 >> 8) & 0xFF;
    row_data[1] = y0 & 0xFF;
    row_data[2] = (y1 >> 8) & 0xFF;
    row_data[3] = y1 & 0xFF;

    tft_write_cmd_data(ILI9341_CASET, col_data, 4);
    tft_write_cmd_data(ILI9341_PASET, row_data, 4);
    tft_write_cmd(ILI9341_RAMWR);
}

void tft_push_region(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    if (!s_initialized || !s_fb_front) return;
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    if (x + w > TFT_WIDTH)  w = TFT_WIDTH - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;
    if (w == 0 || h == 0) return;

    if (!spi_bus_lock(pdMS_TO_TICKS(100))) {
        ESP_LOGW(TAG, "SPI bus lock timeout, skipping push");
        return;
    }

    tft_set_window(x, y, x + w - 1, y + h - 1);
    tft_dc_data();

    /* Transfer via bounce buffer (PSRAM -> SRAM -> SPI-DMA) */
    for (uint16_t row = 0; row < h; row += TFT_BOUNCE_LINES) {
        uint16_t lines = (h - row < TFT_BOUNCE_LINES)
                         ? (h - row) : TFT_BOUNCE_LINES;
        size_t copy_bytes = (size_t)w * lines * 2;

        memcpy(s_bounce,
               &s_fb_front[(y + row) * TFT_WIDTH + x],
               copy_bytes);

        spi_transaction_t t = {0};
        t.length    = copy_bytes * 8;
        t.tx_buffer = s_bounce;
        spi_device_polling_transmit(s_spi, &t);
    }

    spi_bus_unlock();
}

void tft_push_buffer(void)
{
    if (!s_initialized) return;

    tft_swap_buffers();
    tft_push_region(0, 0, TFT_WIDTH, TFT_HEIGHT);
}
