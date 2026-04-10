#ifndef __ALGO_COMMON_H__
#define __ALGO_COMMON_H__

/* Include ----------------------------------------------------------------- */
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Function ---------------------------------------------------------------- */
static inline uint16_t ALGO_GetU16BE(const uint8_t *array)
{
    return ((uint16_t)((((uint16_t)(array[0x00])) << (__CHAR_BIT__ * 0x01)) |
                       (((uint16_t)(array[0x01])) << (__CHAR_BIT__ * 0x00))));
}

static inline void ALGO_PutU16BE(uint8_t *array, uint16_t value)
{
    array[0x00] = (uint8_t)(value >> (__CHAR_BIT__ * 0x01));
    array[0x01] = (uint8_t)(value >> (__CHAR_BIT__ * 0x00));
}

static inline uint16_t ALGO_GetU16LE(const uint8_t *array)
{
    return ((uint16_t)((((uint16_t)(array[0x00])) << (__CHAR_BIT__ * 0x00)) |
                       (((uint16_t)(array[0x01])) << (__CHAR_BIT__ * 0x01))));
}

static inline void ALGO_PutU16LE(uint8_t *array, uint16_t value)
{
    array[0x00] = (uint8_t)(value >> (__CHAR_BIT__ * 0x00));
    array[0x01] = (uint8_t)(value >> (__CHAR_BIT__ * 0x01));
}

static inline uint32_t ALGO_GetU32BE(const uint8_t *array)
{
    return ((uint32_t)((((uint32_t)(array[0x00])) << (__CHAR_BIT__ * 0x03)) |
                       (((uint32_t)(array[0x01])) << (__CHAR_BIT__ * 0x02)) |
                       (((uint32_t)(array[0x02])) << (__CHAR_BIT__ * 0x01)) |
                       (((uint32_t)(array[0x03])) << (__CHAR_BIT__ * 0x00))));
}

static inline void ALGO_PutU32BE(uint8_t *array, uint32_t value)
{
    array[0x00] = (uint8_t)(value >> (__CHAR_BIT__ * 0x03));
    array[0x01] = (uint8_t)(value >> (__CHAR_BIT__ * 0x02));
    array[0x02] = (uint8_t)(value >> (__CHAR_BIT__ * 0x01));
    array[0x03] = (uint8_t)(value >> (__CHAR_BIT__ * 0x00));
}

static inline uint32_t ALGO_GetU32LE(const uint8_t *array)
{
    return ((uint32_t)((((uint32_t)(array[0x00])) << (__CHAR_BIT__ * 0x00)) |
                       (((uint32_t)(array[0x01])) << (__CHAR_BIT__ * 0x01)) |
                       (((uint32_t)(array[0x02])) << (__CHAR_BIT__ * 0x02)) |
                       (((uint32_t)(array[0x03])) << (__CHAR_BIT__ * 0x03))));
}

static inline void ALGO_PutU32LE(uint8_t *array, uint32_t value)
{
    array[0x00] = (uint8_t)(value >> (__CHAR_BIT__ * 0x00));
    array[0x01] = (uint8_t)(value >> (__CHAR_BIT__ * 0x01));
    array[0x02] = (uint8_t)(value >> (__CHAR_BIT__ * 0x02));
    array[0x03] = (uint8_t)(value >> (__CHAR_BIT__ * 0x03));
}

static inline uint64_t ALGO_GetU64BE(const uint8_t *array)
{
    return ((uint64_t)((((uint64_t)(array[0x00])) << (__CHAR_BIT__ * 0x07)) |
                       (((uint64_t)(array[0x01])) << (__CHAR_BIT__ * 0x06)) |
                       (((uint64_t)(array[0x02])) << (__CHAR_BIT__ * 0x05)) |
                       (((uint64_t)(array[0x03])) << (__CHAR_BIT__ * 0x04)) |
                       (((uint64_t)(array[0x04])) << (__CHAR_BIT__ * 0x03)) |
                       (((uint64_t)(array[0x05])) << (__CHAR_BIT__ * 0x02)) |
                       (((uint64_t)(array[0x06])) << (__CHAR_BIT__ * 0x01)) |
                       (((uint64_t)(array[0x07])) << (__CHAR_BIT__ * 0x00))));
}

static inline void ALGO_PutU64BE(uint8_t *array, uint64_t value)
{
    array[0x00] = (uint8_t)(value >> (__CHAR_BIT__ * 0x07));
    array[0x01] = (uint8_t)(value >> (__CHAR_BIT__ * 0x06));
    array[0x02] = (uint8_t)(value >> (__CHAR_BIT__ * 0x05));
    array[0x03] = (uint8_t)(value >> (__CHAR_BIT__ * 0x04));
    array[0x04] = (uint8_t)(value >> (__CHAR_BIT__ * 0x03));
    array[0x05] = (uint8_t)(value >> (__CHAR_BIT__ * 0x02));
    array[0x06] = (uint8_t)(value >> (__CHAR_BIT__ * 0x01));
    array[0x07] = (uint8_t)(value >> (__CHAR_BIT__ * 0x00));
}

static inline uint64_t ALGO_GetU64LE(const uint8_t *array)
{
    return ((uint64_t)((((uint64_t)(array[0x00])) << (__CHAR_BIT__ * 0x00)) |
                       (((uint64_t)(array[0x01])) << (__CHAR_BIT__ * 0x01)) |
                       (((uint64_t)(array[0x02])) << (__CHAR_BIT__ * 0x02)) |
                       (((uint64_t)(array[0x03])) << (__CHAR_BIT__ * 0x03)) |
                       (((uint64_t)(array[0x04])) << (__CHAR_BIT__ * 0x04)) |
                       (((uint64_t)(array[0x05])) << (__CHAR_BIT__ * 0x05)) |
                       (((uint64_t)(array[0x06])) << (__CHAR_BIT__ * 0x06)) |
                       (((uint64_t)(array[0x07])) << (__CHAR_BIT__ * 0x07))));
}

static inline void ALGO_PutU64LE(uint8_t *array, uint64_t value)
{
    array[0x00] = (uint8_t)(value >> (__CHAR_BIT__ * 0x00));
    array[0x01] = (uint8_t)(value >> (__CHAR_BIT__ * 0x01));
    array[0x02] = (uint8_t)(value >> (__CHAR_BIT__ * 0x02));
    array[0x03] = (uint8_t)(value >> (__CHAR_BIT__ * 0x03));
    array[0x04] = (uint8_t)(value >> (__CHAR_BIT__ * 0x04));
    array[0x05] = (uint8_t)(value >> (__CHAR_BIT__ * 0x05));
    array[0x06] = (uint8_t)(value >> (__CHAR_BIT__ * 0x06));
    array[0x07] = (uint8_t)(value >> (__CHAR_BIT__ * 0x07));
}

#ifdef __cplusplus
}
#endif

#endif /* __ALGO_COMMON_H__ */
