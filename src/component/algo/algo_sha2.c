/* Include ----------------------------------------------------------------- */
#include "algo/algo_sha2.h"

/* Define ----------------------------------------------------------------- */
#define SHA256_SHR(x, n)  ((x) >> (n))
#define SHA256_ROTR(x, n) (SHA256_SHR(x, n) | ((x) << (32 - (n))))

#define SHA256_S0(x) (SHA256_ROTR(x, 7) ^ SHA256_ROTR(x, 18) ^ SHA256_SHR(x, 3))
#define SHA256_S1(x) (SHA256_ROTR(x, 17) ^ SHA256_ROTR(x, 19) ^ SHA256_SHR(x, 10))
#define SHA256_S2(x) (SHA256_ROTR(x, 2) ^ SHA256_ROTR(x, 13) ^ SHA256_ROTR(x, 22))
#define SHA256_S3(x) (SHA256_ROTR(x, 6) ^ SHA256_ROTR(x, 11) ^ SHA256_ROTR(x, 25))

#define SHA512_SHR(x, n)  ((x) >> (n))
#define SHA512_ROTR(x, n) (SHA512_SHR(x, n) | ((x) << (64 - (n))))

#define SHA512_S0(x) (SHA512_ROTR(x, 1) ^ SHA512_ROTR(x, 8) ^ SHA512_SHR(x, 7))
#define SHA512_S1(x) (SHA512_ROTR(x, 19) ^ SHA512_ROTR(x, 61) ^ SHA512_SHR(x, 6))
#define SHA512_S2(x) (SHA512_ROTR(x, 28) ^ SHA512_ROTR(x, 34) ^ SHA512_ROTR(x, 39))
#define SHA512_S3(x) (SHA512_ROTR(x, 14) ^ SHA512_ROTR(x, 18) ^ SHA512_ROTR(x, 41))

#define F0(x, y, z) (((x) & (y)) | ((z) & ((x) | (y))))
#define F1(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))

/* Variable ---------------------------------------------------------------- */
static const uint32_t SHA256_K[] = {
    0x428A2F98UL, 0x71374491UL, 0xB5C0FBCFUL, 0xE9B5DBA5UL, 0x3956C25BUL, 0x59F111F1UL, 0x923F82A4UL, 0xAB1C5ED5UL,
    0xD807AA98UL, 0x12835B01UL, 0x243185BEUL, 0x550C7DC3UL, 0x72BE5D74UL, 0x80DEB1FEUL, 0x9BDC06A7UL, 0xC19BF174UL,
    0xE49B69C1UL, 0xEFBE4786UL, 0x0FC19DC6UL, 0x240CA1CCUL, 0x2DE92C6FUL, 0x4A7484AAUL, 0x5CB0A9DCUL, 0x76F988DAUL,
    0x983E5152UL, 0xA831C66DUL, 0xB00327C8UL, 0xBF597FC7UL, 0xC6E00BF3UL, 0xD5A79147UL, 0x06CA6351UL, 0x14292967UL,
    0x27B70A85UL, 0x2E1B2138UL, 0x4D2C6DFCUL, 0x53380D13UL, 0x650A7354UL, 0x766A0ABBUL, 0x81C2C92EUL, 0x92722C85UL,
    0xA2BFE8A1UL, 0xA81A664BUL, 0xC24B8B70UL, 0xC76C51A3UL, 0xD192E819UL, 0xD6990624UL, 0xF40E3585UL, 0x106AA070UL,
    0x19A4C116UL, 0x1E376C08UL, 0x2748774CUL, 0x34B0BCB5UL, 0x391C0CB3UL, 0x4ED8AA4AUL, 0x5B9CCA4FUL, 0x682E6FF3UL,
    0x748F82EEUL, 0x78A5636FUL, 0x84C87814UL, 0x8CC70208UL, 0x90BEFFFAUL, 0xA4506CEBUL, 0xBEF9A3F7UL, 0xC67178F2UL,
};

static const uint64_t SHA512_K[] = {
    0x428A2F98D728AE22ULL, 0x7137449123EF65CDULL, 0xB5C0FBCFEC4D3B2FULL, 0xE9B5DBA58189DBBCULL, 0x3956C25BF348B538ULL,
    0x59F111F1B605D019ULL, 0x923F82A4AF194F9BULL, 0xAB1C5ED5DA6D8118ULL, 0xD807AA98A3030242ULL, 0x12835B0145706FBEULL,
    0x243185BE4EE4B28CULL, 0x550C7DC3D5FFB4E2ULL, 0x72BE5D74F27B896FULL, 0x80DEB1FE3B1696B1ULL, 0x9BDC06A725C71235ULL,
    0xC19BF174CF692694ULL, 0xE49B69C19EF14AD2ULL, 0xEFBE4786384F25E3ULL, 0x0FC19DC68B8CD5B5ULL, 0x240CA1CC77AC9C65ULL,
    0x2DE92C6F592B0275ULL, 0x4A7484AA6EA6E483ULL, 0x5CB0A9DCBD41FBD4ULL, 0x76F988DA831153B5ULL, 0x983E5152EE66DFABULL,
    0xA831C66D2DB43210ULL, 0xB00327C898FB213FULL, 0xBF597FC7BEEF0EE4ULL, 0xC6E00BF33DA88FC2ULL, 0xD5A79147930AA725ULL,
    0x06CA6351E003826FULL, 0x142929670A0E6E70ULL, 0x27B70A8546D22FFCULL, 0x2E1B21385C26C926ULL, 0x4D2C6DFC5AC42AEDULL,
    0x53380D139D95B3DFULL, 0x650A73548BAF63DEULL, 0x766A0ABB3C77B2A8ULL, 0x81C2C92E47EDAEE6ULL, 0x92722C851482353BULL,
    0xA2BFE8A14CF10364ULL, 0xA81A664BBC423001ULL, 0xC24B8B70D0F89791ULL, 0xC76C51A30654BE30ULL, 0xD192E819D6EF5218ULL,
    0xD69906245565A910ULL, 0xF40E35855771202AULL, 0x106AA07032BBD1B8ULL, 0x19A4C116B8D2D0C8ULL, 0x1E376C085141AB53ULL,
    0x2748774CDF8EEB99ULL, 0x34B0BCB5E19B48A8ULL, 0x391C0CB3C5C95A63ULL, 0x4ED8AA4AE3418ACBULL, 0x5B9CCA4F7763E373ULL,
    0x682E6FF3D6B2B8A3ULL, 0x748F82EE5DEFB2FCULL, 0x78A5636F43172F60ULL, 0x84C87814A1F0AB72ULL, 0x8CC702081A6439ECULL,
    0x90BEFFFA23631E28ULL, 0xA4506CEBDE82BDE9ULL, 0xBEF9A3F7B2C67915ULL, 0xC67178F2E372532BULL, 0xCA273ECEEA26619CULL,
    0xD186B8C721C0C207ULL, 0xEADA7DD6CDE0EB1EULL, 0xF57D4F7FEE6ED178ULL, 0x06F067AA72176FBAULL, 0x0A637DC5A2C898A6ULL,
    0x113F9804BEF90DAEULL, 0x1B710B35131C471BULL, 0x28DB77F523047D84ULL, 0x32CAAB7B40C72493ULL, 0x3C9EBE0A15C9BEBCULL,
    0x431D67C49C100D4CULL, 0x4CC5D4BECB3E42B6ULL, 0x597F299CFC657E2AULL, 0x5FCB6FAB3AD6FAECULL, 0x6C44198C4A475817ULL};

/* Function ----------------------------------------------------------------- */
static void SHA256_SHA224_Process(uint32_t state[8], const uint8_t data[64])
{
    size_t i;
    struct {
        uint32_t temp1, temp2, W[64];
        uint32_t A[8];
    } local;

    for (i = 0; i < 8; i++) {
        local.A[i] = state[i];
    }

    for (i = 0; i < 64; i++) {
        if (i < 16) {
            local.W[i] = ALGO_GetU32BE(&(data[sizeof(uint32_t) * i]));
        } else {
            local.W[i] = SHA256_S1(local.W[i - 2]) + local.W[i - 7] + SHA256_S0(local.W[i - 15]) + local.W[i - 16];
        }

        local.temp1 = local.A[7] + SHA256_S3(local.A[4]) + F1(local.A[4], local.A[5], local.A[6]) + SHA256_K[i] +
                      local.W[i];
        local.temp2 = SHA256_S2(local.A[0]) + F0((local.A[0]), (local.A[1]), (local.A[2]));
        local.A[3] += local.temp1;
        local.A[7] = local.temp1 + local.temp2;

        local.temp1 = local.A[7];
        local.A[7] = local.A[6];
        local.A[6] = local.A[5];
        local.A[5] = local.A[4];
        local.A[4] = local.A[3];
        local.A[3] = local.A[2];
        local.A[2] = local.A[1];
        local.A[1] = local.A[0];
        local.A[0] = local.temp1;
    }

    for (i = 0; i < 8; i++) {
        state[i] += local.A[i];
    }
}

static void SHA256_SHA224_Update(ALGO_SHA256_Context_t *ctx, const uint8_t *data, size_t size)
{
    uint32_t left = ctx->total[0] & 0x3F;
    uint32_t fill = 64 - left;

    ctx->total[0] += (uint32_t)size;
    if (ctx->total[0] < (uint32_t)size) {
        ctx->total[1]++;
    }

    if ((left > 0) && (size >= fill)) {
        memcpy(&(ctx->buff[left]), data, fill);

        SHA256_SHA224_Process(ctx->state, ctx->buff);

        data += fill;
        size -= fill;
        left = 0;
    }

    while (size >= 64) {
        SHA256_SHA224_Process(ctx->state, data);
        data += 64;
        size -= 64;
    }

    if (size > 0) {
        memcpy(&(ctx->buff[left]), data, size);
    }
}

static void SHA256_SHA224_Finish(ALGO_SHA256_Context_t *ctx, uint8_t *hash, size_t size)
{
    uint32_t used = ctx->total[0x00] & 0x3F;
    ctx->buff[used++] = 0x80;

    if (used <= 56) {
        memset(&(ctx->buff[used]), 0, 56 - used);
    } else {
        memset(&(ctx->buff[used]), 0, 64 - used);

        SHA256_SHA224_Process(ctx->state, ctx->buff);

        memset(&(ctx->buff[0]), 0, 56);
    }

    ALGO_PutU32BE(&(ctx->buff[56]), (ctx->total[0] >> 29) | (ctx->total[1] << 3));
    ALGO_PutU32BE(&(ctx->buff[60]), (ctx->total[0] << 3));

    SHA256_SHA224_Process(ctx->state, ctx->buff);

    for (size_t i = 0; i < (size / sizeof(uint32_t)); i++) {
        ALGO_PutU32BE(&(hash[sizeof(uint32_t) * i]), ctx->state[i]);
    }
}

void ALGO_SHA256_Init(ALGO_SHA256_Context_t *ctx)
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x6A09E667UL;
    ctx->state[1] = 0xBB67AE85UL;
    ctx->state[2] = 0x3C6EF372UL;
    ctx->state[3] = 0xA54FF53AUL;
    ctx->state[4] = 0x510E527FUL;
    ctx->state[5] = 0x9B05688CUL;
    ctx->state[6] = 0x1F83D9ABUL;
    ctx->state[7] = 0x5BE0CD19UL;
}

void ALGO_SHA256_Update(ALGO_SHA256_Context_t *ctx, const uint8_t *data, size_t size)
{
    if (size > 0) {
        SHA256_SHA224_Update(ctx, data, size);
    }
}

void ALGO_SHA256_Finish(ALGO_SHA256_Context_t *ctx, ALGO_SHA256_Digest_t *digest)
{
    SHA256_SHA224_Finish(ctx, digest->hash, sizeof(digest->hash));
}

void ALGO_SHA256_Calulate(const uint8_t *data, size_t len, ALGO_SHA256_Digest_t *digest)
{
    ALGO_SHA256_Context_t ctx;

    ALGO_SHA256_Init(&ctx);
    ALGO_SHA256_Update(&ctx, data, len);
    ALGO_SHA256_Finish(&ctx, digest);
}

void ALGO_SHA224_Init(ALGO_SHA224_Context_t *ctx)
{
    ((ALGO_SHA256_Context_t *)ctx)->total[0] = 0;
    ((ALGO_SHA256_Context_t *)ctx)->total[1] = 0;

    ((ALGO_SHA256_Context_t *)ctx)->state[0] = 0xC1059ED8UL;
    ((ALGO_SHA256_Context_t *)ctx)->state[1] = 0x367CD507UL;
    ((ALGO_SHA256_Context_t *)ctx)->state[2] = 0x3070DD17UL;
    ((ALGO_SHA256_Context_t *)ctx)->state[3] = 0xF70E5939UL;
    ((ALGO_SHA256_Context_t *)ctx)->state[4] = 0xFFC00B31UL;
    ((ALGO_SHA256_Context_t *)ctx)->state[5] = 0x68581511UL;
    ((ALGO_SHA256_Context_t *)ctx)->state[6] = 0x64F98FA7UL;
    ((ALGO_SHA256_Context_t *)ctx)->state[7] = 0xBEFA4FA4UL;
}

void ALGO_SHA224_Update(ALGO_SHA224_Context_t *ctx, const uint8_t *data, size_t size)
{
    if (size > 0) {
        SHA256_SHA224_Update((ALGO_SHA256_Context_t *)ctx, data, size);
    }
}

void ALGO_SHA224_Finish(ALGO_SHA224_Context_t *ctx, ALGO_SHA224_Digest_t *digest)
{
    SHA256_SHA224_Finish((ALGO_SHA256_Context_t *)ctx, digest->hash, sizeof(digest->hash));
}

void ALGO_SHA224_Calulate(const uint8_t *data, size_t len, ALGO_SHA224_Digest_t *digest)
{
    ALGO_SHA224_Context_t ctx;

    ALGO_SHA224_Init(&ctx);
    ALGO_SHA224_Update(&ctx, data, len);
    ALGO_SHA224_Finish(&ctx, digest);
}

static void SHA512_SHA384_Process(uint64_t state[8], const uint8_t data[64])
{
    size_t i;
    struct {
        uint64_t temp1, temp2, W[80];
        uint64_t A[8];
    } local;

    for (i = 0; i < 8; i++) {
        local.A[i] = state[i];
    }

    for (i = 0; i < 80; i++) {
        if (i < 16) {
            local.W[i] = ALGO_GetU64BE(&(data[sizeof(uint64_t) * i]));
        } else {
            local.W[i] = SHA512_S1(local.W[i - 2]) + local.W[i - 7] + SHA512_S0(local.W[i - 15]) + local.W[i - 16];
        }

        local.temp1 = local.A[7] + SHA512_S3(local.A[4]) + F1(local.A[4], local.A[5], local.A[6]) + SHA512_K[i] +
                      local.W[i];
        local.temp2 = SHA512_S2(local.A[0]) + F0(local.A[0], local.A[1], local.A[2]);
        local.A[3] += local.temp1;
        local.A[7] = local.temp1 + local.temp2;

        local.temp1 = local.A[7];
        local.A[7] = local.A[6];
        local.A[6] = local.A[5];
        local.A[5] = local.A[4];
        local.A[4] = local.A[3];
        local.A[3] = local.A[2];
        local.A[2] = local.A[1];
        local.A[1] = local.A[0];
        local.A[0] = local.temp1;
    }

    for (i = 0; i < 8; i++) {
        state[i] += local.A[i];
    }
}

static void SHA512_SHA384_Update(ALGO_SHA512_Context_t *ctx, const uint8_t *data, size_t size)
{
    uint32_t left = (unsigned int)(ctx->total[0] & 0x7F);
    uint32_t fill = 128 - left;

    ctx->total[0] += (uint64_t)size;
    if (ctx->total[0] < (uint64_t)size) {
        ctx->total[1]++;
    }

    if ((left > 0) && (size >= fill)) {
        memcpy(&(ctx->buff[left]), data, fill);

        SHA512_SHA384_Process(ctx->state, ctx->buff);

        data += fill;
        size -= fill;
        left = 0;
    }

    while (size >= 128) {
        SHA512_SHA384_Process(ctx->state, data);
        data += 128;
        size -= 128;
    }

    if (size > 0) {
        memcpy(&(ctx->buff[left]), data, size);
    }
}

static void SHA512_SHA384_Finish(ALGO_SHA512_Context_t *ctx, uint8_t *hash, size_t size)
{
    uint32_t used = ctx->total[0x00] & 0x7F;
    ctx->buff[used++] = 0x80;

    if (used <= 112) {
        memset(&(ctx->buff[used]), 0, 112 - used);
    } else {
        memset(&(ctx->buff[used]), 0, 128 - used);

        SHA512_SHA384_Process(ctx->state, ctx->buff);

        memset(&(ctx->buff[0]), 0, 112);
    }

    ALGO_PutU64BE(&(ctx->buff[112]), (ctx->total[0] >> 61) | (ctx->total[1] << 3));
    ALGO_PutU64BE(&(ctx->buff[120]), (ctx->total[0] << 3));

    SHA512_SHA384_Process(ctx->state, ctx->buff);

    for (size_t i = 0; i < (size / sizeof(uint64_t)); i++) {
        ALGO_PutU64BE(&(hash[sizeof(uint64_t) * i]), ctx->state[i]);
    }
}

void ALGO_SHA512_Init(ALGO_SHA512_Context_t *ctx)
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x6A09E667F3BCC908ULL;
    ctx->state[1] = 0xBB67AE8584CAA73BULL;
    ctx->state[2] = 0x3C6EF372FE94F82BULL;
    ctx->state[3] = 0xA54FF53A5F1D36F1ULL;
    ctx->state[4] = 0x510E527FADE682D1ULL;
    ctx->state[5] = 0x9B05688C2B3E6C1FULL;
    ctx->state[6] = 0x1F83D9ABFB41BD6BULL;
    ctx->state[7] = 0x5BE0CD19137E2179ULL;
}

void ALGO_SHA512_Update(ALGO_SHA512_Context_t *ctx, const uint8_t *data, size_t size)
{
    if (size > 0) {
        SHA512_SHA384_Update(ctx, data, size);
    }
}

void ALGO_SHA512_Finish(ALGO_SHA512_Context_t *ctx, ALGO_SHA512_Digest_t *digest)
{
    SHA512_SHA384_Finish(ctx, digest->hash, sizeof(digest->hash));
}

void ALGO_SHA512_Calulate(const uint8_t *data, size_t len, ALGO_SHA512_Digest_t *digest)
{
    ALGO_SHA512_Context_t ctx;

    ALGO_SHA512_Init(&ctx);
    ALGO_SHA512_Update(&ctx, data, len);
    ALGO_SHA512_Finish(&ctx, digest);
}

void ALGO_SHA384_Init(ALGO_SHA384_Context_t *ctx)
{
    ((ALGO_SHA512_Context_t *)ctx)->total[0] = 0;
    ((ALGO_SHA512_Context_t *)ctx)->total[1] = 0;

    ((ALGO_SHA512_Context_t *)ctx)->state[0] = 0xCBBB9D5DC1059ED8ULL;
    ((ALGO_SHA512_Context_t *)ctx)->state[1] = 0x629A292A367CD507ULL;
    ((ALGO_SHA512_Context_t *)ctx)->state[2] = 0x9159015A3070DD17ULL;
    ((ALGO_SHA512_Context_t *)ctx)->state[3] = 0x152FECD8F70E5939ULL;
    ((ALGO_SHA512_Context_t *)ctx)->state[4] = 0x67332667FFC00B31ULL;
    ((ALGO_SHA512_Context_t *)ctx)->state[5] = 0x8EB44A8768581511ULL;
    ((ALGO_SHA512_Context_t *)ctx)->state[6] = 0xDB0C2E0D64F98FA7ULL;
    ((ALGO_SHA512_Context_t *)ctx)->state[7] = 0x47B5481DBEFA4FA4ULL;
}

void ALGO_SHA384_Update(ALGO_SHA384_Context_t *ctx, const uint8_t *data, size_t size)
{
    if (size > 0) {
        SHA512_SHA384_Update((ALGO_SHA512_Context_t *)ctx, data, size);
    }
}

void ALGO_SHA384_Finish(ALGO_SHA384_Context_t *ctx, ALGO_SHA384_Digest_t *digest)
{
    SHA512_SHA384_Finish((ALGO_SHA512_Context_t *)ctx, digest->hash, sizeof(digest->hash));
}

void ALGO_SHA384_Calulate(const uint8_t *data, size_t len, ALGO_SHA384_Digest_t *digest)
{
    ALGO_SHA384_Context_t ctx;

    ALGO_SHA384_Init(&ctx);
    ALGO_SHA384_Update(&ctx, data, len);
    ALGO_SHA384_Finish(&ctx, digest);
}
