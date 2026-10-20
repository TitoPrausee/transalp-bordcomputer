idf_component_register(
    SRCS "storage.c" "sd_card.c" "logger.c" "config.c"
    INCLUDE_DIRS "include"
    REQUIRES gps sensors utils nvs_flash fatfs sdmmc
    PRIV_REQUIRES main esp_timer
)
