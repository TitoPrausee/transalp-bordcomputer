idf_component_register(
    SRCS "sensors.c" "rpm.c" "temperature.c" "fuel_level.c" "gear_indicator.c" "imu.c"
    INCLUDE_DIRS "include"
    REQUIRES utils diagnostics gps
    PRIV_REQUIRES main
)
