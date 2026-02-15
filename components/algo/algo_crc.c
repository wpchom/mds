/* Include ----------------------------------------------------------------- */
#include "algo_crc.h"

/******************************************************************************
 * Name:    CRC8                    x8+x2+x+1
 * Poly:    0x07
 * Init:    0x00
 * Refin:   False
 * Refout:  False
 * Xorout:  0x00
 *****************************************************************************/
uint8_t ALGO_CRC8(uint8_t plus, const uint8_t *data, size_t size)
{
    uint8_t crc = plus;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x80U) {
                crc = (crc << 1) ^ 0x07U;
            } else {
                crc <<= 1;
            }
        }
    }

    return (crc);
}

/******************************************************************************
 * Name:    CRC-8/ITU/ATM           x8+x2+x+1
 * Poly:    0x07
 * Init:    0x00
 * Refin:   False
 * Refout:  False
 * Xorout:  0x55
 *****************************************************************************/
uint8_t ALGO_CRC8_ITU(uint8_t plus, const uint8_t *data, size_t size)
{
    uint8_t crc = plus;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x80U) {
                crc = (crc << 1) ^ 0x07U;
            } else {
                crc <<= 1;
            }
        }
    }

    return (crc ^ 0x55);
}

/******************************************************************************
 * Name:    CRC-8/ROHC              x8+x2+x+1
 * Poly:    0x07
 * Init:    0xFF
 * Refin:   True
 * Refout:  True
 * Xorout:  0x00
 *****************************************************************************/
uint8_t ALGO_CRC8_ROHC(uint8_t plus, const uint8_t *data, size_t size)
{
    uint8_t crc = plus ^ 0xFFU;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x01U) {
                crc = (crc >> 1) ^ 0xE0U;
            } else {
                crc >>= 1;
            }
        }
    }

    return (crc);
}

/******************************************************************************
 * Name:    CRC-8/MAXIM             x8+x5+x4+1
 * Poly:    0x31
 * Init:    0x00
 * Refin:   True
 * Refout:  True
 * Xorout:  0x00
 *****************************************************************************/
uint8_t ALGO_CRC8_MAXIM(uint8_t plus, const uint8_t *data, size_t size)
{
    uint8_t crc = plus;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x01U) {
                crc = (crc >> 1) ^ 0x8CU;
            } else {
                crc >>= 1;
            }
        }
    }

    return (crc);
}

/******************************************************************************
 * Name:    CRC16/IBM/ARC           x16+x15+x2+1
 * Poly:    0x8005
 * Init:    0x0000
 * Refin:   True
 * Refout:  True
 * Xorout:  0x0000
 *****************************************************************************/
uint16_t ALGO_CRC16(uint16_t plus, const uint8_t *data, size_t size)
{
    uint16_t crc = plus;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x0001U) {
                crc = (crc >> 1) ^ 0xA001U;
            } else {
                crc >>= 1;
            }
        }
    }

    return (crc);
}

/******************************************************************************
 * Name:    CRC-16/MAXIM            x16+x15+x2+1
 * Poly:    0x8005
 * Init:    0x0000
 * Refin:   True
 * Refout:  True
 * Xorout:  0xFFFF
 *****************************************************************************/
uint16_t ALGO_CRC16_MAXIM(uint16_t plus, const uint8_t *data, size_t size)
{
    uint16_t crc = plus;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x0001U) {
                crc = (crc >> 1) ^ 0xA001U;
            } else {
                crc >>= 1;
            }
        }
    }

    return (crc ^ 0xFFFFU);
}

/******************************************************************************
 * Name:    CRC-16/USB              x16+x15+x2+1
 * Poly:    0x8005
 * Init:    0xFFFF
 * Refin:   True
 * Refout:  True
 * Xorout:  0xFFFF
 *****************************************************************************/
uint16_t ALGO_CRC16_USB(uint16_t plus, const uint8_t *data, size_t size)
{
    uint16_t crc = plus ^ 0xFFFFU;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x0001U) {
                crc = (crc >> 1) ^ 0xA001U;
            } else {
                crc >>= 1;
            }
        }
    }

    return (crc ^ 0xFFFFU);
}

/******************************************************************************
 * Name:    CRC-16/MODBUS           x16+x15+x2+1
 * Poly:    0x8005
 * Init:    0xFFFF
 * Refin:   True
 * Refout:  True
 * Xorout:  0x0000
 *****************************************************************************/
uint16_t ALGO_CRC16_MODBUS(uint16_t plus, const uint8_t *data, size_t size)
{
    uint16_t crc = plus ^ 0xFFFFU;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x0001U) {
                crc = (crc >> 1) ^ 0xA001U;
            } else {
                crc >>= 1;
            }
        }
    }

    return (crc);
}

/******************************************************************************
 * Name:    CRC-16/CCITT/KERMIT     x16+x12+x5+1
 * Poly:    0x1021
 * Init:    0x0000
 * Refin:   True
 * Refout:  True
 * Xorout:  0x0000
 *****************************************************************************/
uint16_t ALGO_CRC16_CCITT(uint16_t plus, const uint8_t *data, size_t size)
{
    uint16_t crc = plus;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x0001U) {
                crc = (crc >> 1) ^ 0x8408U;
            } else {
                crc >>= 1;
            }
        }
    }

    return (crc);
}

/******************************************************************************
 * Name:    CRC-16/CCITT-FALSE      x16+x12+x5+1
 * Poly:    0x1021
 * Init:    0xFFFF
 * Refin:   False
 * Refout:  False
 * Xorout:  0x0000
 *****************************************************************************/
uint16_t ALGO_CRC16_CCITT_FALSE(uint16_t plus, const uint8_t *data, size_t size)
{
    uint16_t crc = plus ^ 0xFFFFU;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x8000U) {
                crc = (crc << 1) ^ 0x1021U;
            } else {
                crc <<= 1;
            }
        }
    }

    return (crc);
}

/******************************************************************************
 * Name:    CRC-16/X25          x16+x12+x5+1
 * Poly:    0x1021
 * Init:    0xFFFF
 * Refin:   True
 * Refout:  True
 * Xorout:  0XFFFF
 *****************************************************************************/
uint16_t ALGO_CRC16_X25(uint16_t plus, const uint8_t *data, size_t size)
{
    uint16_t crc = plus ^ 0xFFFFU;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x0001U) {
                crc = (crc >> 1) ^ 0x8408U;
            } else {
                crc >>= 1;
            }
        }
    }

    return (crc ^ 0xFFFFU);
}

/******************************************************************************
 * Name:    CRC-16/XMODEM/ZMODEM/ACORN  x16+x12+x5+1
 * Poly:    0x1021
 * Init:    0x0000
 * Refin:   False
 * Refout:  False
 * Xorout:  0x0000
 *****************************************************************************/
uint16_t ALGO_CRC16_XMODEM(uint16_t plus, const uint8_t *data, size_t size)
{
    uint16_t crc = plus;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x8000U) {
                crc = (crc << 1) ^ 0x1021U;
            } else {
                crc <<= 1;
            }
        }
    }

    return (crc);
}

/******************************************************************************
 * Name:    CRC-16/DNP/M-BUS        x16+x13+x12+x11+x10+x8+x6+x5+x2+1
 * Poly:    0x3D65
 * Init:    0x0000
 * Refin:   True
 * Refout:  True
 * Xorout:  0xFFFF
 *****************************************************************************/
uint16_t ALGO_CRC16_DNP(uint16_t plus, const uint8_t *data, size_t size)
{
    uint16_t crc = plus;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x0001U) {
                crc = (crc >> 1) ^ 0x16BCU;
            } else {
                crc >>= 1;
            }
        }
    }

    return (crc ^ 0xFFFFU);
}

/******************************************************************************
 * Name:    CRC-32/ADCCP            x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1
 * Poly:    0x4C11DB7
 * Init:    0xFFFFFFFF
 * Refin:   True
 * Refout:  True
 * Xorout:  0xFFFFFFFF
 *****************************************************************************/
uint32_t ALGO_CRC32(uint32_t plus, const uint8_t *data, size_t size)
{
    uint32_t crc = plus ^ 0xFFFFFFFFUL;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x00000001UL) {
                crc = (crc >> 1) ^ 0xEDB88320UL;
            } else {
                crc >>= 1;
            }
        }
    }

    return (crc ^ 0xFFFFFFFFUL);
}

/******************************************************************************
 * Name:    CRC-32/MPEG-2           x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1
 * Poly:    0x4C11DB7
 * Init:    0xFFFFFFFF
 * Refin:   False
 * Refout:  False
 * Xorout:  0x0000000
 *****************************************************************************/
uint32_t ALGO_CRC32_MPEG2(uint32_t plus, const uint8_t *data, size_t size)
{
    uint32_t crc = plus ^ 0xFFFFFFFFUL;

    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int j = 0; j < __CHAR_BIT__; ++j) {
            if (crc & 0x80000000UL) {
                crc = (crc << 1) ^ 0x04C11DB7UL;
            } else {
                crc <<= 1;
            }
        }
    }

    return (crc);
}
