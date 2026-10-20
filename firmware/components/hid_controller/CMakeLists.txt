idf_component_register(
    SRCS "controller_input.c" "controller_mapping.c" "hid_controller.c"
    INCLUDE_DIRS "include" "."
    REQUIRES main utils
    PRIV_REQUIRES ble sensors driver esp_timer
)
