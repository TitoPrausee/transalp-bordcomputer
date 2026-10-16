idf_component_register(
    SRCS "power.c" "power_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES utils esp_adc driver
    PRIV_REQUIRES main
)
