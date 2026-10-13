idf_component_register(
    SRCS "main.c" "app_event_loop.c"
    INCLUDE_DIRS "."
    REQUIRES
        gps
        display
        sensors
        storage
        ble
        power
        diagnostics
        ota
        utils
)
