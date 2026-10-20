idf_component_register(
    SRCS "ble_service.c" "ble_callbacks.c"
    INCLUDE_DIRS "include" "."
    REQUIRES utils sensors gps power diagnostics carplay_bridge
    PRIV_REQUIRES esp_timer bt
)
