#ifndef _ESP_HTTP_CLIENT_H_
#define _ESP_HTTP_CLIENT_H_

/*
 * esp_http_client.h — Mock for host-side unit tests of the OTA component.
 *
 * The OTA HTTP download path (ota_http_download) is not exercised by the host
 * unit tests — the BLE and SD paths are the primary host-testable surfaces.
 * These stubs provide just enough type surface for ota_update.c to compile;
 * the stubs return failure values so any accidental HTTP-path call fails fast
 * instead of looping.
 */

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "esp_crt_bundle.h"

typedef struct esp_http_client *esp_http_client_handle_t;

typedef struct {
    const char             *url;
    int                     timeout_ms;
    int                     buffer_size;
    int                     buffer_size_tx;
    esp_crt_bundle_attach_cb_t crt_bundle_attach;
    bool                    keep_alive_enable;
} esp_http_client_config_t;

static inline esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *cfg)
{
    (void)cfg;
    return NULL;
}

static inline esp_err_t esp_http_client_set_header(esp_http_client_handle_t client,
                                                    const char *key, const char *val)
{
    (void)client; (void)key; (void)val;
    return ESP_FAIL;
}

static inline esp_err_t esp_http_client_open(esp_http_client_handle_t client, int write_len)
{
    (void)client; (void)write_len;
    return ESP_FAIL;
}

static inline int64_t esp_http_client_fetch_headers(esp_http_client_handle_t client)
{
    (void)client;
    return -1;
}

static inline int esp_http_client_get_status_code(esp_http_client_handle_t client)
{
    (void)client;
    return 0;
}

static inline int esp_http_client_read(esp_http_client_handle_t client, char *buf, int len)
{
    (void)client; (void)buf; (void)len;
    return -1;
}

static inline void esp_http_client_close(esp_http_client_handle_t client) { (void)client; }
static inline void esp_http_client_cleanup(esp_http_client_handle_t client) { (void)client; }

#endif /* _ESP_HTTP_CLIENT_H_ */
