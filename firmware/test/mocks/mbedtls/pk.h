#ifndef _MBEDTLS_PK_H_
#define _MBEDTLS_PK_H_

/*
 * mbedtls/pk.h — Host mock for unit tests.
 * Provides controllable return values for mbedtls_pk_parse_public_key
 * and mbedtls_pk_verify so the fail-closed behavior of
 * ota_secure_verify_signature() can be tested without the full
 * mbedTLS library.
 *
 * Controllable globals (defined in the test file):
 *   g_mbedtls_pk_parse_result  — return value of pk_parse_public_key (0 = OK)
 *   g_mbedtls_pk_verify_result — return value of pk_verify (0 = OK)
 */

#include <stdint.h>
#include <stddef.h>
#include "md.h"

typedef struct {
    int dummy;
} mbedtls_pk_context;

/* Controllable mock outcomes (defined in the test). */
extern int g_mbedtls_pk_parse_result;
extern int g_mbedtls_pk_verify_result;

static inline void mbedtls_pk_init(mbedtls_pk_context *ctx)
{
    if (ctx) memset(ctx, 0, sizeof(*ctx));
}

static inline void mbedtls_pk_free(mbedtls_pk_context *ctx)
{
    if (ctx) memset(ctx, 0, sizeof(*ctx));
}

/* Returns g_mbedtls_pk_parse_result (0 = success). */
static inline int mbedtls_pk_parse_public_key(mbedtls_pk_context *ctx,
                                                const uint8_t *key,
                                                size_t keylen)
{
    (void)ctx; (void)key; (void)keylen;
    return g_mbedtls_pk_parse_result;
}

/* Returns g_mbedtls_pk_verify_result (0 = success). */
static inline int mbedtls_pk_verify(mbedtls_pk_context *ctx,
                                     mbedtls_md_type_t md_alg,
                                     const uint8_t *hash, size_t hash_len,
                                     const uint8_t *sig, size_t sig_len)
{
    (void)ctx; (void)md_alg; (void)hash; (void)hash_len;
    (void)sig; (void)sig_len;
    return g_mbedtls_pk_verify_result;
}

#endif /* _MBEDTLS_PK_H_ */
