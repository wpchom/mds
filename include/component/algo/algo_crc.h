#ifndef __ALGO_CRC_H__
#define __ALGO_CRC_H__

/* Include ----------------------------------------------------------------- */
#include "algo_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Name:    CRC8                    x8+x2+x+1
 * Poly:    0x07
 * Init:    0x00
 * Refin:   False
 * Refout:  False
 * Xorout:  0x00
 *****************************************************************************/
uint8_t ALGO_CRC8(uint8_t plus, const uint8_t *data, size_t size);

/******************************************************************************
 * Name:    CRC-8/ITU/ATM           x8+x2+x+1
 * Poly:    0x07
 * Init:    0x00
 * Refin:   False
 * Refout:  False
 * Xorout:  0x55
 *****************************************************************************/
uint8_t ALGO_CRC8_ITU(uint8_t plus, const uint8_t *data, size_t size);

/******************************************************************************
 * Name:    CRC-8/ROHC              x8+x2+x+1
 * Poly:    0x07
 * Init:    0xFF
 * Refin:   True
 * Refout:  True
 * Xorout:  0x00
 *****************************************************************************/
uint8_t ALGO_CRC8_ROHC(uint8_t plus, const uint8_t *data, size_t size);

/******************************************************************************
 * Name:    CRC-8/MAXIM             x8+x5+x4+1
 * Poly:    0x31
 * Init:    0x00
 * Refin:   True
 * Refout:  True
 * Xorout:  0x00
 *****************************************************************************/
uint8_t ALGO_CRC8_MAXIM(uint8_t plus, const uint8_t *data, size_t size);

/******************************************************************************
 * Name:    CRC16/IBM/ARC           x16+x15+x2+1
 * Poly:    0x8005
 * Init:    0x0000
 * Refin:   True
 * Refout:  True
 * Xorout:  0x0000
 *****************************************************************************/
uint16_t ALGO_CRC16(uint16_t plus, const uint8_t *data, size_t size);

/******************************************************************************
 * Name:    CRC-16/MAXIM            x16+x15+x2+1
 * Poly:    0x8005
 * Init:    0x0000
 * Refin:   True
 * Refout:  True
 * Xorout:  0xFFFF
 *****************************************************************************/
uint16_t ALGO_CRC16_MAXIM(uint16_t plus, const uint8_t *data, size_t size);

/******************************************************************************
 * Name:    CRC-16/USB              x16+x15+x2+1
 * Poly:    0x8005
 * Init:    0xFFFF
 * Refin:   True
 * Refout:  True
 * Xorout:  0xFFFF
 *****************************************************************************/
uint16_t ALGO_CRC16_USB(uint16_t plus, const uint8_t *data, size_t size);

/******************************************************************************
 * Name:    CRC-16/MODBUS           x16+x15+x2+1
 * Poly:    0x8005
 * Init:    0xFFFF
 * Refin:   True
 * Refout:  True
 * Xorout:  0x0000
 *****************************************************************************/
uint16_t ALGO_CRC16_MODBUS(uint16_t plus, const uint8_t *data, size_t size);

/******************************************************************************
 * Name:    CRC-16/CCITT/KERMIT     x16+x12+x5+1
 * Poly:    0x1021
 * Init:    0x0000
 * Refin:   True
 * Refout:  True
 * Xorout:  0x0000
 *****************************************************************************/
uint16_t ALGO_CRC16_CCITT(uint16_t plus, const uint8_t *data, size_t size);

/******************************************************************************
 * Name:    CRC-16/CCITT-FALSE      x16+x12+x5+1
 * Poly:    0x1021
 * Init:    0xFFFF
 * Refin:   False
 * Refout:  False
 * Xorout:  0x0000
 *****************************************************************************/
uint16_t ALGO_CRC16_CCITT_FALSE(uint16_t plus, const uint8_t *data, size_t size);

/******************************************************************************
 * Name:    CRC-16/X25          x16+x12+x5+1
 * Poly:    0x1021
 * Init:    0xFFFF
 * Refin:   True
 * Refout:  True
 * Xorout:  0XFFFF
 *****************************************************************************/
uint16_t ALGO_CRC16_X25(uint16_t plus, const uint8_t *data, size_t size);

/******************************************************************************
 * Name:    CRC-16/XMODEM/ZMODEM/ACORN  x16+x12+x5+1
 * Poly:    0x1021
 * Init:    0x0000
 * Refin:   False
 * Refout:  False
 * Xorout:  0x0000
 *****************************************************************************/
uint16_t ALGO_CRC16_XMODEM(uint16_t plus, const uint8_t *data, size_t size);

/******************************************************************************
 * Name:    CRC-16/DNP/M-BUS        x16+x13+x12+x11+x10+x8+x6+x5+x2+1
 * Poly:    0x3D65
 * Init:    0x0000
 * Refin:   True
 * Refout:  True
 * Xorout:  0xFFFF
 *****************************************************************************/
uint16_t ALGO_CRC16_DNP(uint16_t plus, const uint8_t *data, size_t size);

/******************************************************************************
 * Name:    CRC-32/ADCCP            x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1
 * Poly:    0x4C11DB7
 * Init:    0xFFFFFFFF
 * Refin:   True
 * Refout:  True
 * Xorout:  0xFFFFFFFF
 *****************************************************************************/
uint32_t ALGO_CRC32(uint32_t plus, const uint8_t *data, size_t size);

/******************************************************************************
 * Name:    CRC-32/MPEG-2           x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1
 * Poly:    0x4C11DB7
 * Init:    0xFFFFFFF
 * Refin:   False
 * Refout:  False
 * Xorout:  0x0000000
 *****************************************************************************/
uint32_t ALGO_CRC32_MPEG2(uint32_t plus, const uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __ALGO_CRC_H__ */
