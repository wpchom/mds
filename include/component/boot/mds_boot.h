/**
 * Copyright (c) [2022] [pchom]
 * [MDS] is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 **/
#ifndef __MDS_BOOT_H__
#define __MDS_BOOT_H__

/* Include ----------------------------------------------------------------- */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define ------------------------------------------------------------------ */
#ifndef CONFIG_MDS_BOOT_SWAP_SECTION
#define CONFIG_MDS_BOOT_SWAP_SECTION ".bootSwap"
#endif

#ifndef CONFIG_MDS_BOOT_UPGRADE_MAGIC
#define CONFIG_MDS_BOOT_UPGRADE_MAGIC 0xBDAC9ADE
#endif

// SHA256
#ifndef CONFIG_MDS_BOOT_CHKHASH_SIZE
#define CONFIG_MDS_BOOT_CHKHASH_SIZE 32
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct {
    void *arg;
} MDS_BOOT_Device_t;

typedef enum MDS_BOOT_Result {
    MDS_BOOT_RESULT_NONE = 0x0000,
    MDS_BOOT_RESULT_SUCCESS = 0xE000,
    MDS_BOOT_RESULT_ECHECK,
    MDS_BOOT_RESULT_ERETRY,
    MDS_BOOT_RESULT_EIO,
    MDS_BOOT_RESULT_ENOMEM,
    MDS_BOOT_RESULT_ELZMA = 0xE200,
} MDS_BOOT_Result_t;

enum MDS_BOOT_FLAG {
    MDS_BOOT_FLAG_NONE = 0x0000,
    MDS_BOOT_FLAG_COPY = 0x0001,
    MDS_BOOT_FLAG_LZMA = 0x0020,
};

typedef struct MDS_BOOT_BinInfo {
    uint8_t check[sizeof(uint16_t)];
    uint8_t flag[sizeof(uint16_t)];
    uint8_t dstAddr[sizeof(uint32_t)];
    uint8_t srcSize[sizeof(uint32_t)];
    uint8_t hash[CONFIG_MDS_BOOT_CHKHASH_SIZE];

    // context of `uint8_t data[srcSize]` for upgrade bin
} MDS_BOOT_BinInfo_t;

typedef struct MDS_BOOT_UpgradeInfo {
    uint8_t check[sizeof(uint16_t)]; // check upgradeInfo header
    uint8_t magic[sizeof(uint32_t)]; // magic for firmware check
    uint8_t count[sizeof(uint16_t)]; // count of binInfos
    uint8_t size[sizeof(uint32_t)]; // totalSize
    uint8_t hash[CONFIG_MDS_BOOT_CHKHASH_SIZE];

    // context of `MDS_BOOT_BinInfo_t binInfo[count]` for upgrade bin combain
} MDS_BOOT_UpgradeInfo_t;

typedef struct MDS_BOOT_SwapInfo {
    uint16_t check;
    uint32_t magic;
    uint16_t count;
    uint32_t size;
    uint8_t hash[CONFIG_MDS_BOOT_CHKHASH_SIZE];

    uint16_t retry;
    uint16_t version;
    uint32_t reset;
    uint32_t result;
} MDS_BOOT_SwapInfo_t;

typedef struct MDS_BOOT_UpgradeOps {
    int (*read)(MDS_BOOT_Device_t *dev, uintptr_t ofs, uint8_t *data, size_t len);
    int (*write)(MDS_BOOT_Device_t *dev, uintptr_t ofs, const uint8_t *data, size_t len);
    int (*erase)(MDS_BOOT_Device_t *dev);
} MDS_BOOT_UpgradeOps_t;

/* Function ---------------------------------------------------------------- */
int MDS_BOOT_DeviceRead(MDS_BOOT_Device_t *dev, uintptr_t ofs, uint8_t *buff,
                        size_t size);
int MDS_BOOT_DeviceWrite(MDS_BOOT_Device_t *dev, uintptr_t ofs, const uint8_t *buff,
                         size_t size);
int MDS_BOOT_DeviceErase(MDS_BOOT_Device_t *dev);

MDS_BOOT_Result_t MDS_BOOT_UpgradeCheck(MDS_BOOT_SwapInfo_t *swapInfo,
                                        MDS_BOOT_Device_t *dst, MDS_BOOT_Device_t *src,
                                        const MDS_BOOT_UpgradeOps_t *ops);
MDS_BOOT_SwapInfo_t *MDS_BOOT_GetSwapInfo(void);

MDS_BOOT_Result_t MDS_BOOT_UpgradeCopy(MDS_BOOT_Device_t *dst, MDS_BOOT_Device_t *src,
                                       uint32_t srcOfs, uint32_t srcSize);
MDS_BOOT_Result_t MDS_BOOT_UpgradeLzma(MDS_BOOT_Device_t *dst, MDS_BOOT_Device_t *src,
                                       uint32_t srcOfs, uint32_t srcSize);

#ifdef __cplusplus
}
#endif

#endif /* __MDS_BOOT_H__ */
