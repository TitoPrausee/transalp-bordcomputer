idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        gps
        display
        sensors
        storage
        ble
        power
)
