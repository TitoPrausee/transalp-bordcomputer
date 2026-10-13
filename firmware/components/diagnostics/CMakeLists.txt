idf_component_register(
    SRCS "diagnostics.c" "dtc.c" "selftest.c"
    INCLUDE_DIRS "include"
    REQUIRES storage utils
)
