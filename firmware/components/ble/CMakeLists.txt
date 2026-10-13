idf_component_register(
    SRCS "ble_service.c" "ble_callbacks.c" "ble.c"
    INCLUDE_DIRS "include"
    REQUIRES utils
)
