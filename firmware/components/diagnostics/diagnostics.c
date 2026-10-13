/*
 * diagnostics.c — Diagnose-Modul Entry Point
 */

#include "diagnostics.h"
#include "esp_log.h"

static const char *TAG = "diag";

void diagnostics_init(void)
{
    dtc_init();
    ESP_LOGI(TAG, "Diagnostics initialized");
}
