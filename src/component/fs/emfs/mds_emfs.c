/**
 * Copyright (c) [2022] [pchom]
 * [MDS_FS] is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 **/
/* Include ----------------------------------------------------------------- */
#include "mds_emfs.h"

/* Define ------------------------------------------------------------------ */
#ifndef MDS_EMFS_LOCK_TIMEOUT
#define MDS_EMFS_LOCK_TIMEOUT MDS_CLOCK_TICK_FOREVER
#endif

#ifndef MDS_EMFS_WRITE_RETRY
#define MDS_EMFS_WRITE_RETRY 3
#endif

#define EMFS_FS_HEADER_MAGIC 0x533B
#define EMFS_FS_PAGE_SIZE    1024
#define EMFS_FILE_SPLIT_SIZE 16
#define EMFS_FILE_ID_MAX     0xFFFU

/* Typedef ----------------------------------------------------------------- */
// fs sector size = 1024B * (fs->sector + 1) , max 256K
typedef struct EMFS_FsHeader {
    uint8_t check[0x02];
    uint8_t magic[0x02];
    uint8_t pages[0x01];
    uint8_t index[0x01];
    uint8_t length[0x02];
} EMFS_FsHeader_t;

// file split size = 16B * (file->split + 1) , max 4K
// id => 0~4095, used => 0~4095
typedef struct EMFS_FileHeader {
    uint8_t check[0x02];
    uint8_t split[0x01];
    uint8_t id_used[0x03];
} EMFS_FileHeader_t;

/* Function ---------------------------------------------------------------- */
static uint16_t EMFS_FileSystemGetCheck(const EMFS_FsHeader_t *header)
{
    return (((uint16_t)(header->check[0x00]) << (MDS_BITS_OF_BYTE * 0x01)) |
            ((uint16_t)(header->check[0x01]) << (MDS_BITS_OF_BYTE * 0x00)));
}

static bool EMFS_FileSystemJudgMagic(const EMFS_FsHeader_t *header)
{
    uint16_t magic = (((uint16_t)(header->magic[0x00]) << (MDS_BITS_OF_BYTE * 0x01)) |
                      ((uint16_t)(header->magic[0x01]) << (MDS_BITS_OF_BYTE * 0x00)));

    return (magic == EMFS_FS_HEADER_MAGIC);
}

static size_t EMFS_FileSystemGetPageSize(const EMFS_FsHeader_t *header)
{
    size_t pages = 1 + ((uint16_t)(header->pages[0x00]));

    return (pages * EMFS_FS_PAGE_SIZE);
}

static size_t EMFS_FileSystemGetIndex(const EMFS_FsHeader_t *header)
{
    return ((size_t)(header->index[0x00]));
}

static size_t EMFS_FileSystemGetPageLength(const EMFS_FsHeader_t *header)
{
    return (((size_t)(header->length[0x00]) << (MDS_BITS_OF_BYTE * 0x01)) |
            ((size_t)(header->length[0x01]) << (MDS_BITS_OF_BYTE * 0x00)));
}

static void EMFS_FileSystemSetCheck(EMFS_FsHeader_t *header, uint16_t check)
{
    header->check[0x00] = (uint8_t)(check >> (MDS_BITS_OF_BYTE * 0x01));
    header->check[0x01] = (uint8_t)(check >> (MDS_BITS_OF_BYTE * 0x00));
}

static void EMFS_FileSystemSetMagic(EMFS_FsHeader_t *header)
{
    header->magic[0x00] = (uint8_t)(EMFS_FS_HEADER_MAGIC >> (MDS_BITS_OF_BYTE * 0x01));
    header->magic[0x01] = (uint8_t)(EMFS_FS_HEADER_MAGIC >> (MDS_BITS_OF_BYTE * 0x00));
}

static void EMFS_FileSystemSetPageSize(EMFS_FsHeader_t *header, size_t sector)
{
    header->pages[0x00] = (uint8_t)((sector / EMFS_FS_PAGE_SIZE) - 1);
}

static void EMFS_FileSystemSetIndex(EMFS_FsHeader_t *header, size_t index)
{
    header->index[0x00] = (uint8_t)(index);
}

static void EMFS_FileSystemSetPageLength(EMFS_FsHeader_t *header, size_t length)
{
    header->length[0x00] = (uint8_t)(length >> (MDS_BITS_OF_BYTE * 0x01));
    header->length[0x01] = (uint8_t)(length >> (MDS_BITS_OF_BYTE * 0x00));
}

static uint16_t EMFS_FileDataGetCheck(const EMFS_FileHeader_t *file)
{
    return (((uint16_t)(file->check[0x00]) << (MDS_BITS_OF_BYTE * 0x01)) |
            ((uint16_t)(file->check[0x01]) << (MDS_BITS_OF_BYTE * 0x00)));
}

static size_t EMFS_FileDataGetSplitSize(const EMFS_FileHeader_t *file)
{
    size_t split = 1 + file->split[0x00];

    return (split * EMFS_FILE_SPLIT_SIZE);
}

static size_t EMFS_FileDataGetId(const EMFS_FileHeader_t *file)
{
    size_t id_used = ((size_t)(file->id_used[0x00]) << (MDS_BITS_OF_BYTE * 0x02)) |
                     ((size_t)(file->id_used[0x01]) << (MDS_BITS_OF_BYTE * 0x01)) |
                     ((size_t)(file->id_used[0x02]) << (MDS_BITS_OF_BYTE * 0x00));

    return ((id_used >> 0x0C) & 0xFFFU);
}

static size_t EMFS_FileDataGetUsed(const EMFS_FileHeader_t *file)
{
    size_t id_used = ((size_t)(file->id_used[0x00]) << (MDS_BITS_OF_BYTE * 0x02)) |
                     ((size_t)(file->id_used[0x01]) << (MDS_BITS_OF_BYTE * 0x01)) |
                     ((size_t)(file->id_used[0x02]) << (MDS_BITS_OF_BYTE * 0x00));

    return ((id_used >> 0x00) & 0xFFFU);
}

static void EMFS_FileDataSetSplitNum(EMFS_FileHeader_t *file, size_t split)
{
    file->split[0x00] = (uint8_t)(split);
}

static void EMFS_FileDataSetId(EMFS_FileHeader_t *file, size_t id)
{
    size_t used = EMFS_FileDataGetUsed(file);
    size_t id_used = ((id & 0xFFFU) << 0x0C) | (used & 0xFFFU);

    file->id_used[0x00] = (uint8_t)(id_used >> (MDS_BITS_OF_BYTE * 0x02));
    file->id_used[0x01] = (uint8_t)(id_used >> (MDS_BITS_OF_BYTE * 0x01));
    file->id_used[0x02] = (uint8_t)(id_used >> (MDS_BITS_OF_BYTE * 0x00));
}

static void EMFS_FileDataSetUsed(EMFS_FileHeader_t *file, size_t used)
{
    size_t id = EMFS_FileDataGetId(file);
    size_t id_used = ((id & 0xFFFU) << 0x0C) | (used & 0xFFFU);

    file->id_used[0x00] = (uint8_t)(id_used >> (MDS_BITS_OF_BYTE * 0x02));
    file->id_used[0x01] = (uint8_t)(id_used >> (MDS_BITS_OF_BYTE * 0x01));
    file->id_used[0x02] = (uint8_t)(id_used >> (MDS_BITS_OF_BYTE * 0x00));
}

static void EMFS_FileDataSetCheck(EMFS_FileHeader_t *file, uint16_t check)
{
    file->check[0x00] = (uint8_t)(check >> (MDS_BITS_OF_BYTE * 0x01));
    file->check[0x01] = (uint8_t)(check >> (MDS_BITS_OF_BYTE * 0x00));
}

static uint16_t EMFS_DataCheckCalculate(uint16_t plus, const uint8_t *buff, size_t len)
{
    uint16_t crcValue = plus;

    for (size_t idx = 1; idx <= len; idx++) {
        uint16_t bitValue = buff[len - idx] << MDS_BITS_OF_BYTE;
        for (size_t bit = 0; bit < MDS_BITS_OF_BYTE; bit++) {
            crcValue = ((crcValue ^ bitValue) & 0x8000) ? ((crcValue << 1) ^ 0x1021) : (crcValue << 1);
            bitValue <<= 1;
        }
    }

    return (crcValue);
}

static uint16_t EMFS_FileSystemSectorCheck(const EMFS_FsHeader_t *header)
{
    return (EMFS_DataCheckCalculate(0, (uint8_t *)header + sizeof(header->check),
                                    EMFS_FileSystemGetPageSize(header) - sizeof(header->check)));
}

static uint16_t EMFS_FileDataSingleCheck(const EMFS_FileHeader_t *file)
{
    return (EMFS_DataCheckCalculate(0, (uint8_t *)file + sizeof(file->check),
                                    EMFS_FileDataGetSplitSize(file) - sizeof(file->check)));
}

static size_t EMFS_DeviceSearchPageSize(DEV_STORAGE_Periph_t *device, uint8_t *pageBuff, size_t buffSize)
{
    size_t pageSize = 0;
    size_t totalSize = DEV_STORAGE_PeriphTotalSize(device);

    MDS_Err_t err = DEV_STORAGE_PeriphOpen(device, MDS_CLOCK_TICK_FOREVER);
    if (err != MDS_EOK) {
        return (0);
    }

    for (size_t readOfs = 0; readOfs < totalSize; readOfs += EMFS_FS_PAGE_SIZE) {
        err = DEV_STORAGE_PeriphRead(device, readOfs, pageBuff, buffSize, NULL);
        if (err != MDS_EOK) {
            continue;
        }

        EMFS_FsHeader_t *header = (EMFS_FsHeader_t *)(pageBuff);
        if (EMFS_FileSystemJudgMagic(header) == false) {
            continue;
        }

        pageSize = EMFS_FileSystemGetPageSize(header);
        if ((pageSize < EMFS_FS_PAGE_SIZE) || ((pageSize % EMFS_FS_PAGE_SIZE) != 0) || (pageSize > buffSize)) {
            pageSize = 0;
            continue;
        }

        if (EMFS_FileSystemGetCheck(header) != EMFS_FileSystemSectorCheck(header)) {
            pageSize = 0;
            continue;
        }

        break;
    }

    DEV_STORAGE_PeriphClose(device);

    return (0);
}

static MDS_Err_t EMFS_FileSystemReadCheckPage(MDS_EMFS_FileSystem_t *fs, size_t readOfs)
{
    MDS_Err_t err = DEV_STORAGE_PeriphOpen(fs->init.device, MDS_CLOCK_TICK_FOREVER);
    if (err != MDS_EOK) {
        return (err);
    }

    err = DEV_STORAGE_PeriphRead(fs->init.device, readOfs, fs->init.buff, fs->init.size, NULL);
    DEV_STORAGE_PeriphClose(fs->init.device);

    if (err != MDS_EOK) {
        return (err);
    }

    EMFS_FsHeader_t *header = (EMFS_FsHeader_t *)(fs->init.buff);
    if ((EMFS_FileSystemJudgMagic(header) == false) || (EMFS_FileSystemGetPageSize(header) > fs->init.size) ||
        (EMFS_FileSystemGetCheck(header) != EMFS_FileSystemSectorCheck(header))) {
        return (MDS_EAGAIN);
    }

    return (MDS_EOK);
}

static MDS_Err_t EMFS_FileSystemSearchLastPage(MDS_EMFS_FileSystem_t *fs)
{
    size_t totalSize = DEV_STORAGE_PeriphTotalSize(fs->init.device);
    if (totalSize < EMFS_FS_PAGE_SIZE) {
        return (MDS_EIO);
    }

    bool hasFind = false;
    bool hasZero = false;
    size_t findOfs = 0;
    size_t findIdx = 0;

    size_t tempOfs = ((fs->readOfs + fs->init.size) <= totalSize) ? (fs->readOfs) : (0);
    do {
        if (EMFS_FileSystemReadCheckPage(fs, tempOfs) == MDS_EOK) {
            EMFS_FsHeader_t *header = (EMFS_FsHeader_t *)(fs->init.buff);
            size_t tempIdx = EMFS_FileSystemGetIndex(header);
            if (tempIdx == 0) {
                hasZero = true;
            }
            if (((hasZero == false) && (tempIdx >= findIdx)) ||
                ((hasZero != false) && ((intptr_t)(tempIdx) >= (intptr_t)(findIdx)))) {
                findOfs = tempOfs;
                findIdx = tempIdx;
                hasFind = true;
            }
        }
        tempOfs = ((tempOfs + fs->init.size) < totalSize) ? (tempOfs + fs->init.size) : (0);
    } while (tempOfs != fs->readOfs);

    if (hasFind) {
        fs->readOfs = findOfs;
        return (EMFS_FileSystemReadCheckPage(fs, findOfs));
    }

    return (MDS_EIO);
}

static size_t EMFS_FileSystemGetBlockIndex(DEV_STORAGE_Periph_t *device, size_t blkNums, size_t offset)
{
    size_t blkOffset = 0;
    size_t blkIndex = 0;

    while (blkIndex < blkNums) {
        size_t blkLimit = blkOffset + DEV_STORAGE_PeriphSectorSize(device, blkOffset, NULL);
        if ((blkOffset <= offset) && (offset < blkLimit)) {
            break;
        }
        blkOffset = blkLimit;
        blkIndex++;
    }

    return (blkIndex);
}

static MDS_Err_t EMFS_FileSystemEraseNextPage(MDS_EMFS_FileSystem_t *fs, size_t nextOfs)
{
    size_t blkNums = DEV_STORAGE_PeriphSectorNums(fs->init.device);
    size_t totalSize = DEV_STORAGE_PeriphTotalSize(fs->init.device);

    size_t writeBlkS = EMFS_FileSystemGetBlockIndex(fs->init.device, blkNums, nextOfs);
    size_t writeBlkE = EMFS_FileSystemGetBlockIndex(fs->init.device, blkNums, nextOfs + fs->init.size - 1);

    if (totalSize >= (fs->init.size + fs->init.size)) {
        size_t readBlkS = EMFS_FileSystemGetBlockIndex(fs->init.device, blkNums, fs->readOfs);
        size_t readBlkE = EMFS_FileSystemGetBlockIndex(fs->init.device, blkNums, fs->readOfs + fs->init.size - 1);
        if (((readBlkS <= writeBlkS) && (writeBlkS <= readBlkE)) ||
            ((readBlkS <= writeBlkE) && (writeBlkE <= readBlkE))) {
            return (MDS_EACCES);
        }
    }

    MDS_Err_t err = DEV_STORAGE_PeriphOpen(fs->init.device, MDS_CLOCK_TICK_FOREVER);
    if (err != MDS_EOK) {
        return (err);
    }

    err = DEV_STORAGE_PeriphErase(fs->init.device, writeBlkS, writeBlkE - writeBlkS + 1, NULL);
    DEV_STORAGE_PeriphClose(fs->init.device);

    return (err);
}

static MDS_Err_t EMFS_FileSystemCheckBlankPage(MDS_EMFS_FileSystem_t *fs, size_t nextOfs)
{
    MDS_Err_t err = DEV_STORAGE_PeriphOpen(fs->init.device, MDS_CLOCK_TICK_FOREVER);
    if (err != MDS_EOK) {
        return (err);
    }

    uintptr_t buff[EMFS_FS_PAGE_SIZE / EMFS_FILE_SPLIT_SIZE];
    size_t ofs = 0;

    while ((err == MDS_EOK) && (ofs < fs->init.size)) {
        size_t read = ((fs->init.size - ofs) > sizeof(buff)) ? (sizeof(buff)) : (fs->init.size - ofs);
        err = DEV_STORAGE_PeriphRead(fs->init.device, nextOfs + ofs, (uint8_t *)buff, read, NULL);
        for (size_t idx = 0; (err == MDS_EOK) && (idx < read); idx++) {
            if (buff[idx] != __UINTPTR_MAX__) {
                err = MDS_EFAULT;
            }
        }
        ofs += read;
    }

    DEV_STORAGE_PeriphClose(fs->init.device);

    return (err);
}

static MDS_Err_t EMFS_FileSystemFindNextPage(MDS_EMFS_FileSystem_t *fs, size_t nextOfs)
{
    size_t totalSize = DEV_STORAGE_PeriphTotalSize(fs->init.device);
    if (totalSize < fs->init.size) {
        return (MDS_EIO);
    }

    MDS_Err_t err = MDS_EIO;

    size_t tempOfs = ((nextOfs + fs->init.size) <= totalSize) ? (nextOfs) : (0);
    do {
        err = EMFS_FileSystemCheckBlankPage(fs, tempOfs);
        if (err == MDS_EOK) {
            fs->writeOfs = tempOfs;
            break;
        }
        tempOfs = ((tempOfs + fs->init.size) < totalSize) ? (tempOfs + fs->init.size) : (0);
    } while (tempOfs != fs->readOfs);

    if (err != MDS_EOK) {
        err = EMFS_FileSystemEraseNextPage(fs, nextOfs);
    }

    return (err);
}

static MDS_Err_t EMFS_FileSystemWriteFlush(MDS_EMFS_FileSystem_t *fs)
{
    MDS_Err_t err;
    EMFS_FsHeader_t *header = (EMFS_FsHeader_t *)(fs->init.buff);

    size_t index = EMFS_FileSystemGetIndex(header) + 1;
    EMFS_FileSystemSetIndex(header, index);

    uint16_t check = EMFS_FileSystemSectorCheck(header);
    EMFS_FileSystemSetCheck(header, check);

    for (size_t retry = 0; retry <= MDS_EMFS_WRITE_RETRY; retry++) {
        err = EMFS_FileSystemFindNextPage(fs, fs->writeOfs);
        if (err != MDS_EOK) {
            continue;
        }
        err = DEV_STORAGE_PeriphOpen(fs->init.device, MDS_CLOCK_TICK_FOREVER);
        if (err != MDS_EOK) {
            break;
        }
        err = DEV_STORAGE_PeriphWrite(fs->init.device, fs->writeOfs, fs->init.buff, fs->init.size, NULL);
        DEV_STORAGE_PeriphClose(fs->init.device);

        if ((err == MDS_EOK) && (EMFS_FileSystemReadCheckPage(fs, fs->writeOfs) == MDS_EOK)) {
            fs->readOfs = fs->writeOfs;
            break;
        }
    }

    if (err != MDS_EOK) {
        err = EMFS_FileSystemReadCheckPage(fs, fs->readOfs);
    }

    return (err);
}

MDS_Err_t MDS_EMFS_Mkfs(DEV_STORAGE_Periph_t *device, size_t pageSize)
{
    MDS_ASSERT(device != NULL);

    size_t totalSize = DEV_STORAGE_PeriphTotalSize(device);
    if ((pageSize < EMFS_FS_PAGE_SIZE) || ((pageSize % EMFS_FS_PAGE_SIZE) != 0) || (totalSize < pageSize)) {
        return (MDS_EINVAL);
    }

    uint8_t buff[EMFS_FS_PAGE_SIZE / EMFS_FILE_SPLIT_SIZE];
    MDS_MemBuffSet(buff, 0xFF, sizeof(buff));

    MDS_Err_t err = DEV_STORAGE_PeriphOpen(device, MDS_CLOCK_TICK_FOREVER);
    if (err != MDS_EOK) {
        return (err);
    }

    uint16_t check = 0;
    err = DEV_STORAGE_PeriphErase(device, 0, DEV_STORAGE_PeriphSectorNums(device), NULL);
    for (size_t ofs = pageSize; (err == MDS_EOK) && (ofs > 0); ofs -= sizeof(buff)) {
        if (ofs > sizeof(buff)) {
            check = EMFS_DataCheckCalculate(check, buff, sizeof(buff));
        } else {
            EMFS_FsHeader_t *header = (EMFS_FsHeader_t *)buff;
            EMFS_FileSystemSetMagic(header);
            EMFS_FileSystemSetIndex(header, 0);
            EMFS_FileSystemSetPageLength(header, 0);
            EMFS_FileSystemSetPageSize(header, pageSize);

            check = EMFS_DataCheckCalculate(check, buff + sizeof(header->check), sizeof(buff) - sizeof(header->check));
            EMFS_FileSystemSetCheck(header, check);
        }
        err = DEV_STORAGE_PeriphWrite(device, 0, buff, sizeof(buff), NULL);
    }
    DEV_STORAGE_PeriphClose(device);

    return (err);
}

MDS_Err_t MDS_EMFS_Mount(MDS_EMFS_FileSystem_t *fs, const MDS_EMFS_FsInitStruct_t *init)
{
    MDS_ASSERT(fs != NULL);
    MDS_ASSERT(init != NULL);
    MDS_ASSERT(init->buff != NULL);
    MDS_ASSERT(init->size >= EMFS_FS_PAGE_SIZE);

    size_t pageSize = EMFS_DeviceSearchPageSize(init->device, init->buff, init->size);
    if ((pageSize < EMFS_FS_PAGE_SIZE) || (pageSize > init->size)) {
        return (MDS_EINVAL);
    }

    MDS_Err_t err = MDS_MutexInit(&(fs->mutex), "emfs");
    if (err != MDS_EOK) {
        return (err);
    }

    err = MDS_MutexAcquire(&(fs->mutex), MDS_CLOCK_TICK_FOREVER);
    if (err == MDS_EOK) {
        MDS_ListInitNode(&(fs->list));
        fs->init.device = init->device;
        fs->init.buff = init->buff;
        fs->init.size = pageSize;

        err = EMFS_FileSystemSearchLastPage(fs);

        MDS_MutexRelease(&(fs->mutex));
    }

    if (err != MDS_EOK) {
        MDS_MutexDeInit(&(fs->mutex));
        fs->init.device = NULL;
        fs->init.buff = NULL;
        fs->init.size = 0;
    }

    return (err);
}

MDS_Err_t MDS_EMFS_Unmout(MDS_EMFS_FileSystem_t *fs)
{
    MDS_ASSERT(fs != NULL);

    MDS_Err_t err = MDS_MutexAcquire(&(fs->mutex), MDS_CLOCK_TICK_FOREVER);
    if (err != MDS_EOK) {
        return (err);
    }

    if (!MDS_ListIsEmpty(&(fs->list))) {
        err = MDS_EBUSY;
    } else {
        MDS_MutexDeInit(&(fs->mutex));
        fs->init.device = NULL;
        fs->init.buff = NULL;
        fs->init.size = 0;
    }

    return (MDS_EOK);
}

MDS_EMFS_FileDesc_t *MDS_EMFS_FileGetOpened(MDS_EMFS_FileSystem_t *fs, MDS_EMFS_FileId_t id)
{
    MDS_ASSERT(fs != NULL);
    MDS_ASSERT(id < EMFS_FILE_ID_MAX);

    MDS_EMFS_FileDesc_t *iter = NULL;

    MDS_LIST_FOREACH_NEXT (iter, node, &(fs->list)) {
        if (EMFS_FileDataGetId((EMFS_FileHeader_t *)(iter->data)) == id) {
            return (iter);
        }
    }

    return (NULL);
}

static EMFS_FileHeader_t *EMFS_FileSystemSearchFile(const MDS_EMFS_FileSystem_t *fs, MDS_EMFS_FileId_t id)
{
    EMFS_FsHeader_t *header = (EMFS_FsHeader_t *)(fs->init.buff);
    EMFS_FileHeader_t *find = (EMFS_FileHeader_t *)((uint8_t *)header + sizeof(EMFS_FsHeader_t));
    EMFS_FileHeader_t *limit = (EMFS_FileHeader_t *)((uint8_t *)find + EMFS_FileSystemGetPageLength(header));

    while (find < limit) {
        if (EMFS_FileDataGetId(find) == id) {
            return (find);
        }
        find = (EMFS_FileHeader_t *)((uint8_t *)find + EMFS_FileDataGetSplitSize(find));
    }

    return (NULL);
}

static EMFS_FileHeader_t *EMFS_FileCreateData(const MDS_EMFS_FileSystem_t *fs, MDS_EMFS_FileId_t id,
                                              const uint8_t *data, uint16_t len)
{
    EMFS_FileHeader_t *file = NULL;
    EMFS_FsHeader_t *header = (EMFS_FsHeader_t *)(fs->init.buff);

    size_t length = EMFS_FileSystemGetPageLength(header);
    size_t splitnm = (len + sizeof(EMFS_FileHeader_t) + EMFS_FILE_SPLIT_SIZE) / EMFS_FILE_SPLIT_SIZE;
    size_t splitsz = EMFS_FILE_SPLIT_SIZE * splitnm;

    if ((fs->init.size - length) >= splitsz) {
        file = (EMFS_FileHeader_t *)((uint8_t *)header + sizeof(EMFS_FsHeader_t) + length);
        EMFS_FileDataSetSplitNum(file, splitnm);
        EMFS_FileDataSetId(file, id);
        EMFS_FileDataSetUsed(file, sizeof(EMFS_FileHeader_t) + len);

        for (size_t idx = 0; idx < splitsz; idx++) {
            *((uint8_t *)file + sizeof(EMFS_FileHeader_t) + idx) = ((idx < len) && (data != NULL)) ? (data[idx])
                                                                                                   : (0xFF);
        }

        uint16_t check = EMFS_FileDataSingleCheck(file);
        EMFS_FileDataSetCheck(file, check);

        EMFS_FileSystemSetPageLength(header, length + splitsz);
    }

    return (file);
}

static void EMFS_FileRemoveData(MDS_EMFS_FileSystem_t *fs, EMFS_FileHeader_t *file)
{
    EMFS_FsHeader_t *header = (EMFS_FsHeader_t *)(fs->init.buff);
    size_t length = EMFS_FileSystemGetPageLength(header);
    size_t split = EMFS_FileDataGetSplitSize(file);

    uint16_t *end = (uint16_t *)((uint8_t *)header + sizeof(EMFS_FsHeader_t) + length);
    uint16_t *dst = (uint16_t *)((uint8_t *)file);
    uint16_t *src = (uint16_t *)((uint8_t *)file + split);

    while (dst < end) {
        *dst++ = (src < end) ? (*src++) : (__UINT16_MAX__);
    }

    EMFS_FileSystemSetPageLength(header, length - split);
}

MDS_Err_t MDS_EMFS_Remove(MDS_EMFS_FileSystem_t *fs, MDS_EMFS_FileId_t id)
{
    MDS_ASSERT(fs != NULL);

    MDS_Err_t err = MDS_MutexAcquire(&(fs->mutex), MDS_EMFS_LOCK_TIMEOUT);
    if (err != MDS_EOK) {
        return (err);
    }

    EMFS_FileHeader_t *file = EMFS_FileSystemSearchFile(fs, id);
    if (file == NULL) {
        err = MDS_ENOENT;
    } else if (MDS_EMFS_FileGetOpened(fs, id) != NULL) {
        err = MDS_EBUSY;
    }

    if (err == MDS_EOK) {
        EMFS_FileRemoveData(fs, file);
        EMFS_FileSystemWriteFlush(fs);
    }

    MDS_MutexRelease(&(fs->mutex));

    return (err);
}

MDS_Err_t MDS_EMFS_Rename(MDS_EMFS_FileSystem_t *fs, MDS_EMFS_FileId_t oldid, MDS_EMFS_FileId_t newid)
{
    MDS_ASSERT(fs != NULL);
    MDS_ASSERT(oldid < EMFS_FILE_ID_MAX);
    MDS_ASSERT(newid < EMFS_FILE_ID_MAX);

    MDS_Err_t err = MDS_MutexAcquire(&(fs->mutex), MDS_EMFS_LOCK_TIMEOUT);
    if (err != MDS_EOK) {
        return (err);
    }

    EMFS_FileHeader_t *file = EMFS_FileSystemSearchFile(fs, oldid);
    if (file == NULL) {
        err = MDS_ENOENT;
    } else if (MDS_EMFS_FileGetOpened(fs, oldid) != NULL) {
        err = MDS_EBUSY;
    }

    if (err == MDS_EOK) {
        EMFS_FileDataSetId(file, newid);

        uint16_t check = EMFS_FileDataSingleCheck(file);
        EMFS_FileDataSetCheck(file, check);

        EMFS_FileSystemWriteFlush(fs);
    }

    MDS_MutexRelease(&(fs->mutex));

    return (err);
}

MDS_Err_t MDS_EMFS_FileCreate(MDS_EMFS_FileDesc_t *fd, MDS_EMFS_FileSystem_t *fs, MDS_EMFS_FileId_t id)
{
    MDS_ASSERT(fd != NULL);
    MDS_ASSERT(fs != NULL);
    MDS_ASSERT(id < EMFS_FILE_ID_MAX);

    MDS_Err_t err = MDS_MutexAcquire(&(fs->mutex), MDS_EMFS_LOCK_TIMEOUT);
    if (err != MDS_EOK) {
        return (err);
    }

    EMFS_FileHeader_t *file = EMFS_FileSystemSearchFile(fs, id);
    if (file != NULL) {
        err = MDS_EEXIST;
    } else {
        file = EMFS_FileCreateData(fs, id, NULL, 0);
        if (file == NULL) {
            err = MDS_ENOMEM;
        } else {
            MDS_ListInitNode(&(fd->node));
            MDS_ListInsertNodePrev(&(fs->list), &(fd->node));
            fd->fs = fs;
            fd->data = (uint8_t *)file;
        }
    }

    MDS_MutexRelease(&(fs->mutex));

    return (err);
}

MDS_Err_t MDS_EMFS_FileOpen(MDS_EMFS_FileDesc_t *fd, MDS_EMFS_FileSystem_t *fs, MDS_EMFS_FileId_t id)
{
    MDS_ASSERT(fd != NULL);
    MDS_ASSERT(fs != NULL);
    MDS_ASSERT(id < EMFS_FILE_ID_MAX);

    MDS_Err_t err = MDS_MutexAcquire(&(fs->mutex), MDS_EMFS_LOCK_TIMEOUT);
    if (err != MDS_EOK) {
        return (err);
    }

    EMFS_FileHeader_t *file = EMFS_FileSystemSearchFile(fs, id);
    if (file == NULL) {
        err = MDS_ENOENT;
    } else {
        MDS_ListInitNode(&(fd->node));
        MDS_ListInsertNodePrev(&(fs->list), &(fd->node));
        fd->fs = fs;
        fd->data = (uint8_t *)file;
    }

    MDS_MutexRelease(&(fs->mutex));

    return (err);
}

MDS_Err_t MDS_EMFS_FileClose(MDS_EMFS_FileDesc_t *fd)
{
    MDS_ASSERT(fd != NULL);

    MDS_EMFS_FileSystem_t *fs = fd->fs;
    MDS_Err_t err = MDS_MutexAcquire(&(fs->mutex), MDS_EMFS_LOCK_TIMEOUT);
    if (err != MDS_EOK) {
        return (err);
    }

    EMFS_FileSystemWriteFlush(fd->fs);

    fd->data = NULL;
    fd->fs = NULL;
    MDS_ListRemoveNode(&(fd->node));

    MDS_MutexRelease(&(fs->mutex));

    return (err);
}

MDS_Err_t MDS_EMFS_FileSize(MDS_EMFS_FileDesc_t *fd, size_t *used, size_t *size)
{
    MDS_ASSERT(fd != NULL);

    if (MDS_ListIsEmpty(&(fd->node))) {
        return (MDS_EIO);
    }

    EMFS_FileHeader_t *file = (EMFS_FileHeader_t *)(fd->data);
    if ((fd->fs == NULL) || (file == NULL) || (EMFS_FileDataGetId(file) == EMFS_FILE_ID_MAX)) {
        return (MDS_EINVAL);
    }

    if (used != NULL) {
        *used = EMFS_FileDataGetUsed(file);
    }

    if (size != NULL) {
        *size = EMFS_FileDataGetSplitSize(file);
    }

    return (MDS_EOK);
}

static intptr_t EFMS_FileUsedCalcOfs(intptr_t used, intptr_t ofs)
{
    if (used == 0) {
        return (0);
    }

    while ((ofs < 0) || (ofs >= used)) {
        ofs = (ofs < 0) ? (ofs + used) : (ofs - used);
    }

    return (ofs);
}

size_t MDS_EMFS_FileTruncate(MDS_EMFS_FileDesc_t *fd, intptr_t ofs)
{
    MDS_ASSERT(fd != NULL);

    if (MDS_ListIsEmpty(&(fd->node))) {
        return (MDS_EIO);
    }

    EMFS_FileHeader_t *file = (EMFS_FileHeader_t *)(fd->data);
    size_t used = EMFS_FileDataGetUsed(file);

    if (ofs >= 0) {
        if ((size_t)ofs < used) {
            EMFS_FileDataSetUsed(file, used = ofs);
        }
    } else if ((size_t)(-ofs) < used) {
        ofs = used + ofs;
        used = MDS_MemBuffCopy((uint8_t *)file + sizeof(EMFS_FileHeader_t), used,
                               (uint8_t *)file + sizeof(EMFS_FileHeader_t) + ofs, used - ofs);
    }

    return (used);
}

MDS_Err_t MDS_EMFS_FileRead(MDS_EMFS_FileDesc_t *fd, intptr_t ofs, uint8_t *buff, size_t len, size_t *read)
{
    MDS_ASSERT(fd != NULL);
    MDS_ASSERT(buff != NULL);

    if (MDS_ListIsEmpty(&(fd->node))) {
        return (MDS_EIO);
    }

    EMFS_FileHeader_t *file = (EMFS_FileHeader_t *)(fd->data);
    if ((fd->fs == NULL) || (file == NULL) || (EMFS_FileDataGetId(file) == EMFS_FILE_ID_MAX)) {
        return (MDS_EINVAL);
    }

    MDS_Err_t err;
    size_t count = 0;

    do {
        if (EMFS_FileDataGetCheck(file) == EMFS_FileDataSingleCheck(file)) {
            err = MDS_EOK;
            break;
        }

        err = MDS_MutexAcquire(&(fd->fs->mutex), MDS_EMFS_LOCK_TIMEOUT);
        if (err != MDS_EOK) {
            break;
        }

        err = EMFS_FileSystemReadCheckPage(fd->fs, fd->fs->readOfs);
        if (err != MDS_EOK) {
            MDS_LOG_E("[emfs] read file err:%d try to reload on fs:%p ofs:%u", err, fd->fs, fd->fs->readOfs);
            err = EMFS_FileSystemSearchLastPage(fd->fs);
            if (err != MDS_EOK) {
                MDS_LOG_E("[emfs] read file err:%d try to research on fs:%p device:%p", err, fd->fs,
                          fd->fs->init.device);
            }
        }

        if (EMFS_FileDataGetCheck(file) != EMFS_FileDataSingleCheck(file)) {
            err = MDS_EIO;
        }

        MDS_MutexRelease(&(fd->fs->mutex));
    } while (0);

    if (err == MDS_EOK) {
        size_t used = EMFS_FileDataGetUsed(file);
        ofs = EFMS_FileUsedCalcOfs(used, ofs);
        count = MDS_MemBuffCopy(buff, len, (uint8_t *)file + sizeof(EMFS_FileHeader_t) + ofs, used - ofs);
    }

    if (read != NULL) {
        *read = count;
    }

    return (err);
}

MDS_Err_t MDS_EMFS_FileWrite(MDS_EMFS_FileDesc_t *fd, intptr_t ofs, const uint8_t *buff, size_t len, size_t *write)
{
    MDS_ASSERT(fd != NULL);
    MDS_ASSERT(buff != NULL);

    if (MDS_ListIsEmpty(&(fd->node))) {
        return (MDS_EIO);
    }

    EMFS_FileHeader_t *file = (EMFS_FileHeader_t *)(fd->data);
    if ((fd->fs == NULL) || (file == NULL) || (EMFS_FileDataGetId(file) == EMFS_FILE_ID_MAX)) {
        return (MDS_EINVAL);
    }

    MDS_Err_t err;
    size_t count = 0;
    size_t used = EMFS_FileDataGetUsed(file);
    ofs = EFMS_FileUsedCalcOfs(used, ofs);

    do {
        if (((ofs + len) <= used) && (memcmp((uint8_t *)file + sizeof(EMFS_FileHeader_t) + ofs, buff, len) == 0)) {
            err = MDS_EOK;
            break;
        }

        err = MDS_MutexAcquire(&(fd->fs->mutex), MDS_EMFS_LOCK_TIMEOUT);
        if (err != MDS_EOK) {
            break;
        }

        EMFS_FsHeader_t *header = (EMFS_FsHeader_t *)(fd->fs->init.buff);
        size_t pageused = sizeof(EMFS_FsHeader_t) + EMFS_FileSystemGetPageLength(header);
        size_t splitsz = EMFS_FileDataGetSplitSize(file);
        size_t spacesz = EMFS_FileSystemGetPageSize(header) - pageused;
        if ((splitsz < (len + sizeof(EMFS_FileHeader_t))) && (spacesz < (len + sizeof(EMFS_FileHeader_t) - splitsz))) {
            err = MDS_ERANGE;
        } else if ((ofs + len) < splitsz) {
            count = MDS_MemBuffCopy((uint8_t *)file + sizeof(EMFS_FileHeader_t) + ofs, splitsz - ofs, buff, len);
        } else {
            MDS_MemBuffCopy((uint8_t *)(header) + pageused, spacesz, (uint8_t *)file, sizeof(EMFS_FileHeader_t) + used);
            EMFS_FileRemoveData(fd->fs, file);
            file = (EMFS_FileHeader_t *)((uint8_t *)(header) + pageused - sizeof(EMFS_FileHeader_t) - used);
            count = MDS_MemBuffCopy((uint8_t *)file + sizeof(EMFS_FileHeader_t) + ofs, splitsz - ofs, buff, len);
        }

        MDS_MutexRelease(&(fd->fs->mutex));
    } while (0);

    if (write != NULL) {
        *write = count;
    }

    return (err);
}
