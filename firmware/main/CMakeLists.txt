idf_component_register(
    SRCS "main.c" "app_event_loop.c"
    INCLUDE_DIRS "."
    REQUIRES
        gps
        display
        sensors
        storage
        ble
        carplay_bridge
        power
        diagnostics
        ota
        utils
        hid_controller
)
