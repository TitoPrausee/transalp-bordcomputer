#ifndef _ESP_ADC_ADC_ONESHOT_H_
#define _ESP_ADC_ADC_ONESHOT_H_

/*
 * Mock header for host-side unit tests.
 * Provides empty stubs for ESP-IDF ADC oneshot API.
 */

typedef int adc_oneshot_unit_handle_t;
typedef int adc_oneshot_chan_handle_t;
typedef int adc_channel_t;
typedef int adc_unit_t;
typedef int adc_atten_t;
typedef int adc_bitwidth_t;

typedef struct {
    adc_unit_t unit_id;
    adc_atten_t atten;
    adc_bitwidth_t bitwidth;
} adc_oneshot_chan_cfg_t;

static inline int adc_oneshot_new_unit(void *cfg, adc_oneshot_unit_handle_t *h) {
    (void)cfg; (void)h; return 0;
}
static inline int adc_oneshot_config_channel(adc_oneshot_unit_handle_t h,
    adc_channel_t c, adc_oneshot_chan_cfg_t *cfg) {
    (void)h; (void)c; (void)cfg; return 0;
}
static inline int adc_oneshot_read(adc_oneshot_unit_handle_t h,
    adc_channel_t c, int *out) {
    (void)h; (void)c; if (out) *out = 0; return 0;
}

#endif /* _ESP_ADC_ADC_ONESHOT_H_ */
