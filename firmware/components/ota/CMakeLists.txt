# Build-time code generation: extract RSA-3072 public key (DER) from the
# Secure Boot signing key and generate a C header for embedding.
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(KEY_HEADER "${GEN_DIR}/ota_public_key_der.h")
set(SIGNING_KEY "${CMAKE_SOURCE_DIR}/secure_boot/ota_signing_key.pem")
set(KEYGEN_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/gen_ota_key_header.py")

add_custom_command(
    OUTPUT "${KEY_HEADER}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND python3 "${KEYGEN_SCRIPT}" "${SIGNING_KEY}" "${KEY_HEADER}"
    DEPENDS "${SIGNING_KEY}" "${KEYGEN_SCRIPT}"
    COMMENT "Generating OTA RSA-3072 public key header (build-time codegen)"
    VERBATIM
)

add_custom_target(ota_key_header DEPENDS "${KEY_HEADER}")

idf_component_register(
    SRCS "ota_update.c" "ota_secure_boot.c"
    INCLUDE_DIRS "include"
    REQUIRES utils
)

# Ensure the key header is generated before compiling ota_secure_boot.c
add_dependencies(${COMPONENT_LIB} ota_key_header)
target_include_directories(${COMPONENT_LIB} PRIVATE "${GEN_DIR}")
