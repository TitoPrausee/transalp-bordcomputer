idf_component_register(
    SRCS "display.c" "epaper_driver.c" "tft_driver.c" "renderer.c" "widgets.c"
    INCLUDE_DIRS "include"
    REQUIRES gps sensors utils
    PRIV_REQUIRES main
)
