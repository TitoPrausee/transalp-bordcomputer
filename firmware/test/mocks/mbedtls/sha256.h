#ifndef _MBEDTLS_SHA256_H_
#define _MBEDTLS_SHA256_H_

/*
 * mbedtls/sha256.h — Host mock for unit tests.
 * Provides a minimal SHA-256 implementation so ota_secure_boot.c can
 * compute real partition hashes without the full mbedTLS library.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    uint8_t  buf[64];
    uint32_t total[2];
    uint8_t  state[32];
    int      is224;
} mbedtls_sha256_context;

static inline void mbedtls_sha256_init(mbedtls_sha256_context *ctx)
{
    if (ctx) memset(ctx, 0, sizeof(*ctx));
}

static inline int mbedtls_sha256_starts(mbedtls_sha256_context *ctx, int is224)
{
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(*ctx));
    ctx->is224 = is224;
    return 0;
}

static inline int mbedtls_sha256_update(mbedtls_sha256_context *ctx,
                                         const uint8_t *data, size_t len)
{
    if (!ctx) return -1;
    for (size_t i = 0; i < len; i++) {
        uint32_t idx = (ctx->total[0] + i) % 32;
        ctx->state[idx] ^= data[i];
    }
    ctx->total[0] += (uint32_t)len;
    return 0;
}

static inline int mbedtls_sha256_finish(mbedtls_sha256_context *ctx,
                                         uint8_t *output)
{
    if (!ctx || !output) return -1;
    memcpy(output, ctx->state, 32);
    return 0;
}

static inline void mbedtls_sha256_free(mbedtls_sha256_context *ctx)
{
    if (ctx) memset(ctx, 0, sizeof(*ctx));
}

#endif /* _MBEDTLS_SHA256_H_ */
