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
#ifndef __MDS_FS_H__
#define __MDS_FS_H__

/* Include ----------------------------------------------------------------- */
#include "mds_sys.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
#ifndef MDS_FILESYSTEM_WITH_LARGE
#define MDS_FILESYSTEM_NAME_SIZE 255
typedef int32_t MDS_FileSize_t;
typedef int32_t MDS_FileOffset_t;
#else
#define MDS_FILESYSTEM_NAME_SIZE 511
typedef int64_t MDS_FileSize_t;
typedef int64_t MDS_FileOffset_t;
#endif

enum MDS_OpenFlag {
    MDS_OFLAG_NONE = 0x0000U,
    MDS_OFLAG_RDONLY = 0x0001U,
    MDS_OFLAG_WRONLY = 0x0002U,
    MDS_OFLAG_RDWR = 0x0003U,
    MDS_OFLAG_OPEN = 0x0008U,

    MDS_OFLAG_APPEND = 0x0010U,
    MDS_OFLAG_EXCL = 0x0020U,
    MDS_OFLAG_CREAT = 0x0040U,
    MDS_OFLAG_TRUNC = 0x0080U,
};

typedef enum MDS_FileWhence {
    MDS_FILE_SEEK_SET = 0,
    MDS_FILE_SEEK_CUR = 1,
    MDS_FILE_SEEK_END = 2,
} MDS_FileWhence_t;

typedef MDS_Arg_t MDS_FsDevice_t;
typedef struct MDS_FileSystem MDS_FileSystem_t;
typedef struct MDS_FileDesc MDS_FileDesc_t;

typedef struct MDS_FileStat {
    void *st_dev;
    uint32_t st_ino;
    uint32_t st_mode;
    void *st_rdev;
    MDS_FileOffset_t st_size;
    MDS_FileSize_t st_blksize;
    MDS_FileSize_t st_blocks;

    MDS_Time_t st_atime;
    MDS_Time_t st_mtime;
    MDS_Time_t st_ctime;
} MDS_FileStat_t;

typedef struct MDS_FsStat {
    MDS_FileSize_t f_blocks;
    MDS_FileSize_t f_bfree;
    MDS_FileSize_t f_bavail;
    MDS_FileSize_t f_files;
    MDS_FileSize_t f_ffree;

    uint32_t f_fsid;
    uint32_t f_namelen;
} MDS_FsStat_t;

typedef struct MDS_FileSystemOps {
    const char *name;

    MDS_Err_t (*mkfs)(MDS_FsDevice_t *device, const MDS_Arg_t *init);
    MDS_Err_t (*mount)(MDS_FileSystem_t *fs);
    MDS_Err_t (*unmount)(MDS_FileSystem_t *fs);
    MDS_Err_t (*statfs)(MDS_FileSystem_t *fs, MDS_FsStat_t *stat);

    MDS_Err_t (*mkdir)(MDS_FileSystem_t *fs, const char *path);
    MDS_Err_t (*remove)(MDS_FileSystem_t *fs, const char *path);
    MDS_Err_t (*rename)(MDS_FileSystem_t *fs, const char *oldpath, const char *newpath);
    MDS_Err_t (*link)(MDS_FileSystem_t *fs, const char *srcpath, const char *dstpath);
    MDS_Err_t (*stat)(MDS_FileSystem_t *fs, const char *path, MDS_FileStat_t *stat);

    MDS_Err_t (*open)(MDS_FileDesc_t *fd);
    MDS_Err_t (*close)(MDS_FileDesc_t *fd);
    MDS_Err_t (*flush)(MDS_FileDesc_t *fd);
    MDS_Err_t (*ioctl)(MDS_FileDesc_t *fd, MDS_Item_t cmd, MDS_Arg_t *args);
    MDS_FileOffset_t (*read)(MDS_FileDesc_t *fd, uint8_t *buff, MDS_FileSize_t len, MDS_FileOffset_t *pos);
    MDS_FileOffset_t (*write)(MDS_FileDesc_t *fd, const uint8_t *buff, MDS_FileSize_t len, MDS_FileOffset_t *pos);
    MDS_FileOffset_t (*seek)(MDS_FileDesc_t *fd, MDS_FileOffset_t ofs, MDS_FileWhence_t whence);
    MDS_FileOffset_t (*truncate)(MDS_FileDesc_t *fd, MDS_FileOffset_t len);
} MDS_FileSystemOps_t;

#define MDS_FILE_SYSTEM_SECTION ".mds.fs."
#define MDS_FILE_SYSTEM_EXPORT(ops)                                                                                    \
    __attribute__((used,                                                                                               \
                   section(MDS_FILE_SYSTEM_SECTION))) static const MDS_FileSystemOps_t *__FsOps_##ops##__ = &(ops);

struct MDS_FileSystem {
    MDS_Mutex_t lock;
    MDS_ListNode_t fsNode;
    MDS_ListNode_t fileList;
    char *path;

    const MDS_FsDevice_t *device;
    const MDS_FileSystemOps_t *ops;
    MDS_Arg_t *data;
};

typedef struct MDS_FileNode {
    MDS_ListNode_t fileNode;
    char *path;

    int32_t refCount;

    MDS_FileSystem_t *fs;
    MDS_Arg_t *data;
} MDS_FileNode_t;

struct MDS_FileDesc {
    MDS_FileNode_t *node;

    MDS_FileOffset_t pos;
    MDS_Mask_t flags;
};

/* Function ---------------------------------------------------------------- */
extern void *MDS_FileSystemMalloc(size_t size);
extern void MDS_FileSystemFree(void *ptr);
extern char *MDS_FileSystemJoinPath(const char *path, ...);

extern MDS_Err_t MDS_FileSystemMkfs(MDS_FsDevice_t *device, const char *fsName, const MDS_Arg_t *init);
extern MDS_Err_t MDS_FileSystemMount(const MDS_FsDevice_t *device, const char *path, const char *fsName);
extern MDS_Err_t MDS_FileSystemUnmount(const char *path);
extern MDS_Err_t MDS_FileSystemStatfs(const char *path, MDS_FsStat_t *stat);

extern MDS_Err_t MDS_FileOpen(MDS_FileDesc_t *fd, const char *path, MDS_Mask_t flags);
extern MDS_Err_t MDS_FileClose(MDS_FileDesc_t *fd);
extern MDS_Err_t MDS_FileFlush(MDS_FileDesc_t *fd);
extern MDS_Err_t MDS_FileIoctl(MDS_FileDesc_t *fd, MDS_Item_t cmd, MDS_Arg_t *args);
extern MDS_FileOffset_t MDS_FilePosRead(MDS_FileDesc_t *fd, uint8_t *buff, MDS_FileSize_t len, MDS_FileOffset_t ofs);
extern MDS_FileOffset_t MDS_FileRead(MDS_FileDesc_t *fd, uint8_t *buff, MDS_FileSize_t len);
extern MDS_FileOffset_t MDS_FilePosWrite(MDS_FileDesc_t *fd, const uint8_t *buff, MDS_FileSize_t len,
                                         MDS_FileOffset_t ofs);
extern MDS_FileOffset_t MDS_FileWrite(MDS_FileDesc_t *fd, const uint8_t *buff, MDS_FileSize_t len);
extern MDS_FileOffset_t MDS_FilePosition(MDS_FileDesc_t *fd);
extern MDS_FileOffset_t MDS_FileSeek(MDS_FileDesc_t *fd, MDS_FileOffset_t offset, MDS_FileWhence_t whence);
extern MDS_FileOffset_t MDS_FileTruncate(MDS_FileDesc_t *fd, MDS_FileOffset_t len);

extern MDS_FileOffset_t MDS_FileTell(MDS_FileDesc_t *fd);
extern MDS_FileOffset_t MDS_FileRewind(MDS_FileDesc_t *fd);
extern MDS_FileSize_t MDS_FileSize(MDS_FileDesc_t *fd);

extern MDS_Err_t MDS_FileMkdir(const char *path);
extern MDS_Err_t MDS_FileRemove(const char *path);
extern MDS_Err_t MDS_FileRename(const char *oldpath, const char *newpath);
extern MDS_Err_t MDS_FileLink(const char *srcpath, const char *dstpath);
extern MDS_Err_t MDS_FileStat(const char *path, MDS_FileStat_t *stat);

#ifdef __cplusplus
}
#endif

#endif /* __MDS_FS_H__ */
