idf_component_register(
    SRCS "ble_privacy.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash
    PRIV_REQUIRES bt esp_system esp_log
)
