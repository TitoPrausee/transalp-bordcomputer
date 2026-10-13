#ifndef _ESP_ADC_ADC_CALI_H_
#define _ESP_ADC_ADC_CALI_H_

/*
 * Mock header for host-side unit tests.
 * Provides empty stubs for ESP-IDF ADC calibration API.
 */

typedef int adc_cali_handle_t;

typedef struct {
    int unit_id;
    int atten;
    int bitwidth;
} adc_cali_curve_fitting_config_t;

static inline int adc_cali_create_curve_fitting(void *cfg, adc_cali_handle_t *h) {
    (void)cfg; (void)h; return 0;
}
static inline int adc_cali_raw_to_voltage(adc_cali_handle_t h, int raw, int *v) {
    (void)h; if (v) *v = raw; return 0;
}

#endif /* _ESP_ADC_ADC_CALI_H_ */
