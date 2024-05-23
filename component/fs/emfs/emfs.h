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
#ifndef __MDS_EMFS_H__
#define __MDS_EMFS_H__

/* Include ----------------------------------------------------------------- */
#include "dev_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef uint16_t MDS_EMFS_FileId_t;

typedef struct MDS_EMFS_FsInitStruct {
    DEV_STORAGE_Periph_t *device;
    uint8_t *buff;
    size_t size;
} MDS_EMFS_FsInitStruct_t;

typedef struct MDS_EMFS_FileSystem {
    MDS_ListNode_t list;
    MDS_Mutex_t mutex;
    size_t pageSize;
    size_t readOfs;
    size_t writeOfs;

    MDS_EMFS_FsInitStruct_t init;
} MDS_EMFS_FileSystem_t;

typedef struct MDS_EMFS_FileDesc {
    MDS_ListNode_t node;
    MDS_EMFS_FileSystem_t *fs;
    uint8_t *data;
} MDS_EMFS_FileDesc_t;

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t MDS_EMFS_Mkfs(DEV_STORAGE_Periph_t *device, size_t sctsz);
extern MDS_Err_t MDS_EMFS_Mount(MDS_EMFS_FileSystem_t *fs, const MDS_EMFS_FsInitStruct_t *init);
extern MDS_Err_t MDS_EMFS_Unmout(MDS_EMFS_FileSystem_t *fs);

extern MDS_EMFS_FileDesc_t *MDS_EMFS_FileGetOpened(MDS_EMFS_FileSystem_t *fs, MDS_EMFS_FileId_t id);
extern MDS_Err_t MDS_EMFS_Unlink(MDS_EMFS_FileSystem_t *fs, MDS_EMFS_FileId_t id);
extern MDS_Err_t MDS_EMFS_Rename(MDS_EMFS_FileSystem_t *fs, MDS_EMFS_FileId_t oldid, MDS_EMFS_FileId_t newid);

extern MDS_Err_t MDS_EMFS_FileCreate(MDS_EMFS_FileDesc_t *fd, MDS_EMFS_FileSystem_t *fs, MDS_EMFS_FileId_t id);
extern MDS_Err_t MDS_EMFS_FileOpen(MDS_EMFS_FileDesc_t *fd, MDS_EMFS_FileSystem_t *fs, MDS_EMFS_FileId_t id);
extern MDS_Err_t MDS_EMFS_FileClose(MDS_EMFS_FileDesc_t *fd);
extern MDS_Err_t MDS_EMFS_FileSize(MDS_EMFS_FileDesc_t *fd, size_t *datasz, size_t *size);
extern size_t MDS_EMFS_FileTruncate(MDS_EMFS_FileDesc_t *fd, intptr_t ofs);
extern MDS_Err_t MDS_EMFS_FileRead(MDS_EMFS_FileDesc_t *fd, intptr_t ofs, uint8_t *buff, size_t size, size_t *read);
extern MDS_Err_t MDS_EMFS_FileWrite(MDS_EMFS_FileDesc_t *fd, intptr_t ofs, const uint8_t *buff, size_t size,
                                    size_t *write);

#ifdef __cplusplus
}
#endif

#endif /* __MDS_EMFS_H__ */
