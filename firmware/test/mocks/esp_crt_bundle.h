#ifndef _ESP_CRT_BUNDLE_H_
#define _ESP_CRT_BUNDLE_H_

/*
 * esp_crt_bundle.h — Mock for host-side unit tests.
 * The OTA HTTP path references esp_crt_bundle_attach as a function pointer in
 * the esp_http_client config; provide a no-op with a stable typedef.
 */

#include <stddef.h>

typedef void (*esp_crt_bundle_attach_cb_t)(void *conf);

static inline void esp_crt_bundle_attach(void *conf) { (void)conf; }

#endif /* _ESP_CRT_BUNDLE_H_ */
