idf_component_register(
    SRCS "carplay_bridge.c" "carplay_client.c" "carplay_telemetry.c" "carplay_pairing.c" "carplay_media.c"
    INCLUDE_DIRS "include" "."
    REQUIRES main utils esp_timer ble_privacy
    PRIV_REQUIRES bt nvs_flash
)
