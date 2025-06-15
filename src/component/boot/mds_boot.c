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
/* Include ----------------------------------------------------------------- */
#include "mds_boot.h"
#include "algo/algo_crc.h"
#include "algo/algo_sha2.h"

/* Define ------------------------------------------------------------------ */
#ifndef CONFIG_MDS_BOOT_UPGRADE_RETRY
#define CONFIG_MDS_BOOT_UPGRADE_RETRY 3
#endif

#ifndef CONFIG_MDS_BOOT_UPGRADE_SIZE
#define CONFIG_MDS_BOOT_UPGRADE_SIZE 1024
#endif

#ifndef CONFIG_MDS_BOOT_UPGRADE_HASH
#define CONFIG_MDS_BOOT_UPGRADE_HASH 1
#endif

/* Variable ---------------------------------------------------------------- */
static __attribute__((
    used, section(CONFIG_MDS_BOOT_SWAP_SECTION))) MDS_BOOT_SwapInfo_t g_bootSwapInfo;
static MDS_BOOT_UpgradeInfo_t g_bootUpgradeInfo = {0};
static uint8_t g_bootCheckBuff[CONFIG_MDS_BOOT_UPGRADE_SIZE];
static const MDS_BOOT_UpgradeOps_t *g_bootUpgradeOps = NULL;

/* Function ---------------------------------------------------------------- */
int MDS_BOOT_DeviceRead(MDS_BOOT_Device_t *dev, uintptr_t ofs, uint8_t *buff, size_t size)
{
    if ((g_bootUpgradeOps != NULL) && (g_bootUpgradeOps->read != NULL)) {
        return (g_bootUpgradeOps->read(dev, ofs, buff, size));
    }

    return (MDS_BOOT_RESULT_EIO);
}

int MDS_BOOT_DeviceWrite(MDS_BOOT_Device_t *dev, uintptr_t ofs, const uint8_t *buff,
                         size_t size)
{
    if ((g_bootUpgradeOps != NULL) && (g_bootUpgradeOps->write != NULL)) {
        return (g_bootUpgradeOps->write(dev, ofs, buff, size));
    }

    return (MDS_BOOT_RESULT_EIO);
}

int MDS_BOOT_DeviceErase(MDS_BOOT_Device_t *dev)
{
    if ((g_bootUpgradeOps != NULL) && (g_bootUpgradeOps->erase != NULL)) {
        return (g_bootUpgradeOps->erase(dev));
    }

    return (MDS_BOOT_RESULT_EIO);
}

static MDS_BOOT_Result_t BOOT_CheckHash(MDS_BOOT_Device_t *src, uint32_t srcOfs,
                                        uint32_t size,
                                        uint8_t hash[CONFIG_MDS_BOOT_CHKHASH_SIZE])
{
#if (defined(CONFIG_MDS_BOOT_UPGRADE_HASH) && (CONFIG_MDS_BOOT_UPGRADE_HASH != 0))
    ALGO_SHA256_Context_t ctx;
    ALGO_SHA256_Digest_t digest;

    ALGO_SHA256_Init(&ctx);

    while (size > 0) {
        size_t len =
            (size > sizeof(g_bootCheckBuff)) ? (sizeof(g_bootCheckBuff)) : (size);
        int res = MDS_BOOT_DeviceRead(src, srcOfs, g_bootCheckBuff, len);
        if (res != 0) {
            return (MDS_BOOT_RESULT_EIO);
        }
        ALGO_SHA256_Update(&ctx, g_bootCheckBuff, len);
        srcOfs += len;
        size -= len;
    }

    ALGO_SHA256_Finish(&ctx, &digest);

    if (memcmp(hash, digest.hash, sizeof(digest.hash)) != 0) {
        return (MDS_BOOT_RESULT_ECHECK);
    }
#else
    (void)src;
    (void)srcOfs;
    (void)size;
    (void)hash;
#endif

    return (MDS_BOOT_RESULT_SUCCESS);
}

static MDS_BOOT_Result_t BOOT_CheckUpgradeInfo(MDS_BOOT_Device_t *src, uint32_t srcOfs,
                                               MDS_BOOT_UpgradeInfo_t *upgradeInfo)
{
    int res =
        MDS_BOOT_DeviceRead(src, srcOfs, (uint8_t *)(upgradeInfo), sizeof(*upgradeInfo));
    if (res != 0) {
        return (MDS_BOOT_RESULT_NONE); // jump to application
    }

    if (ALGO_GetU32BE(upgradeInfo->magic) != CONFIG_MDS_BOOT_UPGRADE_MAGIC) {
        return (MDS_BOOT_RESULT_ECHECK);
    }

    uint16_t check = ALGO_CRC16(0, (uint8_t *)(&(upgradeInfo->magic)),
                                sizeof(*upgradeInfo) - sizeof(upgradeInfo->check));
    if (check != ALGO_GetU16BE(upgradeInfo->check)) {
        return (MDS_BOOT_RESULT_ECHECK);
    }

    return (BOOT_CheckHash(src, sizeof(*upgradeInfo), ALGO_GetU32BE(upgradeInfo->size),
                           upgradeInfo->hash));
}

static MDS_BOOT_Result_t BOOT_CheckAllBinInfo(MDS_BOOT_Device_t *src, size_t srcOfs,
                                              const MDS_BOOT_UpgradeInfo_t *upgradeInfo)
{
    MDS_BOOT_Result_t result = MDS_BOOT_RESULT_SUCCESS;
    uint16_t cnt = ALGO_GetU16BE(upgradeInfo->count);

    for (uint16_t i = 0; i < cnt; i++) {
        MDS_BOOT_BinInfo_t binInfo = {0};

        int res =
            MDS_BOOT_DeviceRead(src, srcOfs, (uint8_t *)(&binInfo), sizeof(binInfo));
        if (res != 0) {
            return (MDS_BOOT_RESULT_EIO);
        }

        uint16_t check = ALGO_CRC16(0, (uint8_t *)(&(binInfo.flag)),
                                    sizeof(binInfo) - sizeof(binInfo.check));
        if (check != ALGO_GetU16BE(binInfo.check)) {
            return (MDS_BOOT_RESULT_ECHECK);
        }

        uint32_t srcSize = ALGO_GetU32BE(binInfo.srcSize);
        result = BOOT_CheckHash(src, srcOfs + sizeof(binInfo), srcSize, binInfo.hash);
        if (result != MDS_BOOT_RESULT_SUCCESS) {
            break;
        }

        srcOfs += sizeof(binInfo) + srcSize;
    }

    return (result);
}

static MDS_BOOT_Result_t BOOT_UpgradeSwtich(MDS_BOOT_Device_t *dst,
                                            MDS_BOOT_Device_t *src, uint32_t srcOfs,
                                            uint32_t srcSize, uint16_t flag)
{
    MDS_BOOT_Result_t result = MDS_BOOT_RESULT_NONE;

    switch (flag) {
#if (defined(CONFIG_MDS_BOOT_UPGRADE_WITH_COPY) &&                                       \
     (CONFIG_MDS_BOOT_UPGRADE_WITH_COPY != 0))
        case MDS_BOOT_FLAG_COPY:
            result = MDS_BOOT_UpgradeCopy(dst, src, srcOfs, srcSize);
            break;
#endif
#if (defined(CONFIG_MDS_BOOT_UPGRADE_WITH_LZMA) &&                                       \
     (CONFIG_MDS_BOOT_UPGRADE_WITH_LZMA != 0))
        case MDS_BOOT_FLAG_LZMA:
            result = MDS_BOOT_UpgradeLzma(dst, src, srcOfs, srcSize);
            break;
#endif
        default:
            break;
    }

    return (result);
}

static MDS_BOOT_Result_t BOOT_UpgradeBinInfo(MDS_BOOT_Device_t *dst,
                                             MDS_BOOT_Device_t *src, size_t srcOfs,
                                             const MDS_BOOT_UpgradeInfo_t *upgradeInfo)
{
    uint16_t cnt = ALGO_GetU16BE(upgradeInfo->count);

    for (uint16_t i = 0; i < cnt; i++) {
        MDS_BOOT_BinInfo_t binInfo = {0};

        int res =
            MDS_BOOT_DeviceRead(src, srcOfs, (uint8_t *)(&binInfo), sizeof(binInfo));
        if (res != 0) {
            return (MDS_BOOT_RESULT_EIO);
        }

        uint16_t flag = ALGO_GetU16BE(binInfo.flag);
        uint32_t srcSize = ALGO_GetU32BE(binInfo.srcSize);
        MDS_BOOT_Result_t result =
            BOOT_UpgradeSwtich(dst, src, srcOfs + sizeof(binInfo), srcSize, flag);
        if (result != MDS_BOOT_RESULT_SUCCESS) {
            return (result);
        }

        srcOfs += sizeof(binInfo) + srcSize;
    }

    return (MDS_BOOT_RESULT_SUCCESS);
}

MDS_BOOT_Result_t MDS_BOOT_UpgradeCheck(MDS_BOOT_SwapInfo_t *swapInfo,
                                        MDS_BOOT_Device_t *dst, MDS_BOOT_Device_t *src,
                                        const MDS_BOOT_UpgradeOps_t *ops)
{
    if (ops == NULL) {
        return (MDS_BOOT_RESULT_NONE);
    }

    if ((swapInfo != NULL) && (swapInfo->retry >= CONFIG_MDS_BOOT_UPGRADE_RETRY)) {
        return (MDS_BOOT_RESULT_ERETRY);
    }

    MDS_BOOT_Result_t result;
    MDS_BOOT_UpgradeInfo_t *upgradeInfo = &g_bootUpgradeInfo;
    g_bootUpgradeOps = ops;

    do {
        result = BOOT_CheckUpgradeInfo(src, 0, upgradeInfo);
        if (result != MDS_BOOT_RESULT_SUCCESS) {
            break;
        }

        result = BOOT_CheckAllBinInfo(src, sizeof(*upgradeInfo), upgradeInfo);
        if (result != MDS_BOOT_RESULT_SUCCESS) {
            break;
        }

        result = BOOT_UpgradeBinInfo(dst, src, sizeof(*upgradeInfo), upgradeInfo);
        if ((result == MDS_BOOT_RESULT_SUCCESS) || (result == MDS_BOOT_RESULT_NONE)) {
            MDS_BOOT_DeviceErase(src);
        }
    } while (0);

    if (swapInfo != NULL) {
        swapInfo->magic = ALGO_GetU32BE(upgradeInfo->magic);
        swapInfo->count = ALGO_GetU16BE(upgradeInfo->count);
        swapInfo->size = ALGO_GetU32BE(upgradeInfo->size);
        memcpy(swapInfo->hash, upgradeInfo->hash, sizeof(upgradeInfo->hash));

        swapInfo->result = result;
        if ((result != MDS_BOOT_RESULT_SUCCESS) && (result != MDS_BOOT_RESULT_NONE) &&
            (result != MDS_BOOT_RESULT_ECHECK)) {
            swapInfo->retry += 1;
        } else {
            swapInfo->retry = 0;
        }
        swapInfo->check = ALGO_CRC16(0, (uint8_t *)(&(swapInfo->magic)),
                                     sizeof(*swapInfo) - sizeof(swapInfo->check));
    }

    return (result);
}

MDS_BOOT_SwapInfo_t *MDS_BOOT_GetSwapInfo(void)
{
    MDS_BOOT_SwapInfo_t *swapInfo = &g_bootSwapInfo;

    if ((swapInfo->magic != CONFIG_MDS_BOOT_UPGRADE_MAGIC) ||
        (swapInfo->check != ALGO_CRC16(0, (uint8_t *)(&(swapInfo->magic)),
                                       sizeof(*swapInfo) - sizeof(swapInfo->check)))) {
        memset(swapInfo, 0, sizeof(*swapInfo));
    }

    return (swapInfo);
}

MDS_BOOT_Result_t MDS_BOOT_UpgradeCopy(MDS_BOOT_Device_t *dst, MDS_BOOT_Device_t *src,
                                       uint32_t srcOfs, uint32_t srcSize)
{
    int res = MDS_BOOT_DeviceErase(dst);
    if (res != 0) {
        return (MDS_BOOT_RESULT_EIO);
    }

    uint32_t readOfs = 0;
    while (readOfs < srcSize) {
        size_t single = ((srcSize - readOfs) > sizeof(g_bootCheckBuff)) ?
                            (sizeof(g_bootCheckBuff)) :
                            (srcSize - readOfs);

        res = MDS_BOOT_DeviceRead(src, srcOfs + readOfs, g_bootCheckBuff, single);
        if (res != 0) {
            return (MDS_BOOT_RESULT_EIO);
        }

        res = MDS_BOOT_DeviceWrite(dst, srcOfs + readOfs, g_bootCheckBuff, single);
        if (res != 0) {
            return (MDS_BOOT_RESULT_EIO);
        }

        readOfs += single;
    }

    return (MDS_BOOT_RESULT_SUCCESS);
}
