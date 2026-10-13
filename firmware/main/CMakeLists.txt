idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        gps
        display
        sensors
        logger
        ble
        power
        ota
)
