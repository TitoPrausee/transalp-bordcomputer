idf_component_register(
    SRCS "carplay_bridge.c" "carplay_client.c" "carplay_telemetry.c" "carplay_pairing.c"
    INCLUDE_DIRS "include" "."
    REQUIRES utils esp_timer
    PRIV_REQUIRES bt nvs_flash
)
