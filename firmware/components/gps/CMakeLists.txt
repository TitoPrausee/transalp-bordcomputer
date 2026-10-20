idf_component_register(
    SRCS "gps_task.c" "nmea_parser.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES main esp_timer
)
