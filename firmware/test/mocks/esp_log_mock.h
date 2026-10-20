#ifndef _ESP_LOG_MOCK_H_
#define _ESP_LOG_MOCK_H_

/*
 * esp_log_mock.h — Mock for ESP-IDF logging macros.
 * Redirects log output to stderr in unit tests.
 */

#include <stdio.h>

#define ESP_LOGE(tag, fmt, ...)  fprintf(stderr, "E %s: " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...)  fprintf(stderr, "W %s: " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...)  fprintf(stderr, "I %s: " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...)  /* suppressed in tests */
#define ESP_LOGV(tag, fmt, ...)  /* suppressed in tests */
#define ESP_LOG_BUFFER_HEX(tag, buf, len) (void)(tag), (void)(buf), (void)(len)

#endif /* _ESP_LOG_MOCK_H_ */
