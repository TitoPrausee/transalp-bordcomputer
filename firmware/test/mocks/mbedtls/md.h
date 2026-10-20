#ifndef _MBEDTLS_MD_H_
#define _MBEDTLS_MD_H_

/*
 * mbedtls/md.h — Host mock for unit tests.
 * Provides the MBEDTLS_MD_SHA256 constant used by mbedtls_pk_verify.
 */

typedef enum {
    MBEDTLS_MD_NONE = 0,
    MBEDTLS_MD_MD5,
    MBEDTLS_MD_SHA1,
    MBEDTLS_MD_SHA224,
    MBEDTLS_MD_SHA256,
    MBEDTLS_MD_SHA384,
    MBEDTLS_MD_SHA512,
} mbedtls_md_type_t;

#endif /* _MBEDTLS_MD_H_ */
