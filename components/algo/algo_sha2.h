#ifndef __ALGO_SHA2_H__
#define __ALGO_SHA2_H__

/* Include ----------------------------------------------------------------- */
#include "algo_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct ALGO_SHA256_Context {
    uint64_t total[0x02];
    uint32_t state[0x08];
    uint8_t buff[0x40];
} ALGO_SHA256_Context_t;

typedef struct ALGO_SHA224_Context {
    uint64_t total[0x02];
    uint32_t state[0x08];
    uint8_t buff[0x40];
} ALGO_SHA224_Context_t;

typedef struct ALGO_SHA512_Context {
    uint64_t total[0x02];
    uint64_t state[0x08];
    uint8_t buff[0x80];
} ALGO_SHA512_Context_t;

typedef struct ALGO_SHA384_Context {
    uint64_t total[0x02];
    uint64_t state[0x08];
    uint8_t buff[0x80];
} ALGO_SHA384_Context_t;

#define ALGO_SHA224_DIGEST_SIZE (224 / 8)
typedef struct ALGO_SHA224_Digest {
    uint8_t hash[ALGO_SHA224_DIGEST_SIZE];
} ALGO_SHA224_Digest_t;

#define ALGO_SHA256_DIGEST_SIZE (256 / 8)
typedef struct ALGO_SHA256_Digest {
    uint8_t hash[ALGO_SHA256_DIGEST_SIZE];
} ALGO_SHA256_Digest_t;

#define ALGO_SHA384_DIGEST_SIZE (384 / 8)
typedef struct ALGO_SHA384_Digest {
    uint8_t hash[ALGO_SHA384_DIGEST_SIZE];
} ALGO_SHA384_Digest_t;

#define ALGO_SHA512_DIGEST_SIZE (512 / 8)
typedef struct ALGO_SHA512_Digest {
    uint8_t hash[ALGO_SHA512_DIGEST_SIZE];
} ALGO_SHA512_Digest_t;

/* Function ---------------------------------------------------------------- */
void ALGO_SHA256_Init(ALGO_SHA256_Context_t *ctx);
void ALGO_SHA256_Update(ALGO_SHA256_Context_t *ctx, const uint8_t *data, size_t size);
void ALGO_SHA256_Finish(ALGO_SHA256_Context_t *ctx, ALGO_SHA256_Digest_t *digest);
void ALGO_SHA256_Calulate(const uint8_t *data, size_t len, ALGO_SHA256_Digest_t *digest);

void ALGO_SHA224_Init(ALGO_SHA224_Context_t *ctx);
void ALGO_SHA224_Update(ALGO_SHA224_Context_t *ctx, const uint8_t *data, size_t size);
void ALGO_SHA224_Finish(ALGO_SHA224_Context_t *ctx, ALGO_SHA224_Digest_t *digest);
void ALGO_SHA224_Calulate(const uint8_t *data, size_t len, ALGO_SHA224_Digest_t *digest);

void ALGO_SHA512_Init(ALGO_SHA512_Context_t *ctx);
void ALGO_SHA512_Update(ALGO_SHA512_Context_t *ctx, const uint8_t *data, size_t size);
void ALGO_SHA512_Finish(ALGO_SHA512_Context_t *ctx, ALGO_SHA512_Digest_t *digest);
void ALGO_SHA512_Calulate(const uint8_t *data, size_t len, ALGO_SHA512_Digest_t *digest);

void ALGO_SHA384_Init(ALGO_SHA384_Context_t *ctx);
void ALGO_SHA384_Update(ALGO_SHA384_Context_t *ctx, const uint8_t *data, size_t size);
void ALGO_SHA384_Finish(ALGO_SHA384_Context_t *ctx, ALGO_SHA384_Digest_t *digest);
void ALGO_SHA384_Calulate(const uint8_t *data, size_t len, ALGO_SHA384_Digest_t *digest);

#ifdef __cpluplus
}
#endif

#endif /* __ALGO_SHA2_H__ */
