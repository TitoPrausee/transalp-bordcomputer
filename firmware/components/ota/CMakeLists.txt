idf_component_register(
    SRCS "ota_update.c" "ota_secure_boot.c"
    INCLUDE_DIRS "include"
    REQUIRES utils
)
