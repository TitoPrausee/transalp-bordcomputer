idf_component_register(
    SRCS "diagnostics.c" "dtc.c" "selftest.c"
    INCLUDE_DIRS "include"
    REQUIRES storage utils sensors gps power ble
    PRIV_REQUIRES main esp_timer
)
