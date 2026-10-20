# Build-time code generation: extract RSA-3072 public key (DER) from the
# Secure Boot signing key and generate a C header for embedding.
# The signing key is gitignored (private). When absent (e.g. CI), the
# committed header in include/ota_public_key_der.h is used as fallback.
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(KEY_HEADER "${GEN_DIR}/ota_public_key_der.h")
set(SIGNING_KEY "${CMAKE_SOURCE_DIR}/secure_boot/ota_signing_key.pem")
set(KEYGEN_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/gen_ota_key_header.py")

idf_component_register(
    SRCS "ota_update.c" "ota_secure_boot.c"
    INCLUDE_DIRS "include"
    REQUIRES utils app_update efuse spi_flash
)

if(EXISTS "${SIGNING_KEY}")
    add_custom_command(
        OUTPUT "${KEY_HEADER}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
        COMMAND python3 "${KEYGEN_SCRIPT}" "${SIGNING_KEY}" "${KEY_HEADER}"
        DEPENDS "${SIGNING_KEY}" "${KEYGEN_SCRIPT}"
        COMMENT "Generating OTA RSA-3072 public key header (build-time codegen)"
        VERBATIM
    )
    add_custom_target(ota_key_header DEPENDS "${KEY_HEADER}")
    add_dependencies(${COMPONENT_LIB} ota_key_header)
    target_include_directories(${COMPONENT_LIB} PRIVATE "${GEN_DIR}")
endif()
