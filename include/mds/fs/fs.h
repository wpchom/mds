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
#include "mds/sys.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define ------------------------------------------------------------------ */
#ifndef CONFIG_MDS_FS_LOG_LEVEL
#define CONFIG_MDS_FS_LOG_LEVEL MDS_LOG_LEVEL_INF
#endif

#ifndef CONFIG_MDS_FS_FILESIZE_MAX
#define CONFIG_MDS_FS_FILESIZE_MAX __INT32_MAX__
#endif

#ifndef CONFIG_MDS_FS_FILENAME_SIZE
#define CONFIG_MDS_FS_FILENAME_SIZE 63
#endif

/* Typedef ----------------------------------------------------------------- */
#if (defined(CONFIG_MDS_FS_FILESIZE_MAX) && (CONFIG_MDS_FS_FILESIZE_MAX <= __INT32_MAX__))
typedef struct {
    int32_t size;
} MDS_FsSize_t;
#elif (defined(CONFIG_MDS_FS_FILESIZE_MAX) && (CONFIG_MDS_FS_FILESIZE_MAX <= __INT64_MAX__))
typedef struct {
    int64_t size;
} MDS_FsSize_t;
#else
#error "CONFIG_MDS_FS_FILESIZE_MAX is too large"
#endif

typedef union {
    MDS_Err_t err;
    MDS_FsSize_t ofs;
    MDS_FsSize_t len;
} MDS_FsResult_t;

enum MDS_FileOpenFlag {
    MDS_FS_OFLAG_RDONLY = 0x0001U,
    MDS_FS_OFLAG_WRONLY = 0x0002U,
    MDS_FS_OFLAG_RDWR = 0x0003U,

    MDS_FS_OFLAG_CREAT = 0x0010U,
    MDS_FS_OFLAG_EXCL = 0x0020U,
    MDS_FS_OFLAG_TRUNC = 0x0040U,
    MDS_FS_OFLAG_APPEND = 0x0080U,
};

typedef enum MDS_FileType {
    MDS_FILE_TYPE_REG = 0x00U,
    MDS_FILE_TYPE_DIR = 0x01U,
    MDS_FILE_TYPE_DEV = 0x02U,
    MDS_FILE_TYPE_LNK = 0x03U,
    MDS_FILE_TYPE_SOCK = 0x04U,
} __attribute__((packed)) MDS_FileType_t;

typedef enum MDS_FileLock {
    MDS_FILE_LOCK_UN = 0x0000U,
    MDS_FILE_LOCK_SH = 0x0001U,
    MDS_FILE_LOCK_EX = 0x0002U,
} __attribute__((packed)) MDS_FileLock_t;

typedef enum MDS_FileWhence {
    MDS_FILE_SEEK_SET = 0,
    MDS_FILE_SEEK_CUR = 1,
    MDS_FILE_SEEK_END = 2,
} __attribute__((packed)) MDS_FileWhence_t;

typedef struct MDS_FsDirent {
    MDS_FileType_t type;
    char name[CONFIG_MDS_FS_FILENAME_SIZE + 1];
    MDS_FsSize_t size;
} MDS_FsDirent_t;

// TODO:
typedef struct MDS_FileStat {
    void *st_dev;
    uint32_t st_ino;
    uint32_t st_mode;
    void *st_rdev;
    MDS_FsSize_t st_size;
    MDS_FsSize_t st_blksize;
    MDS_FsSize_t st_blocks;

    MDS_TimeStamp_t st_atime;
    MDS_TimeStamp_t st_mtime;
    MDS_TimeStamp_t st_ctime;
} MDS_FileStat_t;

typedef struct MDS_FsStat {
    MDS_FsSize_t f_blocks;
    MDS_FsSize_t f_bfree;
    MDS_FsSize_t f_bavail;
    MDS_FsSize_t f_files;
    MDS_FsSize_t f_ffree;

    uint32_t f_namelen;
} MDS_FsStat_t;

typedef int MDS_FsCmd_t;
enum MDS_FILESYSTEM_Cmd {
    MDS_FS_CMD_GETINODE = 0, // same as device get id
};

typedef MDS_Arg_t MDS_FsDevice_t;
typedef struct MDS_FileSystem MDS_FileSystem_t;
typedef struct MDS_FsFile MDS_FsFile_t;
typedef struct MDS_FsDir MDS_FsDir_t;

typedef struct MDS_FileSystemOps {
    const char *name;

    // file
    MDS_Err_t (*open)(MDS_FsFile_t *file, const char *path, MDS_Mask_t flag);
    MDS_Err_t (*close)(MDS_FsFile_t *file);
    MDS_FsResult_t (*read)(MDS_FsFile_t *file, void *dst, MDS_FsSize_t len);
    MDS_FsResult_t (*write)(MDS_FsFile_t *file, const void *src, MDS_FsSize_t len);
    MDS_FsResult_t (*seek)(MDS_FsFile_t *file, MDS_FsSize_t ofs, MDS_FileWhence_t whence);
    MDS_FsResult_t (*tell)(MDS_FsFile_t *file);
    MDS_FsResult_t (*truncate)(MDS_FsFile_t *file, MDS_FsSize_t len);
    MDS_Err_t (*sync)(MDS_FsFile_t *file);
    MDS_Err_t (*ioctl)(MDS_FsFile_t *file, MDS_FsCmd_t cmd, MDS_Arg_t arg);

    // dir
    MDS_Err_t (*opendir)(MDS_FsDir_t *dir, const char *path);
    MDS_Err_t (*readdir)(MDS_FsDir_t *dir, MDS_FsDirent_t *entry);
    MDS_Err_t (*closedir)(MDS_FsDir_t *dir);

    // fs
    MDS_Err_t (*mkfs)(MDS_FsDevice_t *device, MDS_Arg_t init);
    MDS_Err_t (*mount)(MDS_FileSystem_t *fs);
    MDS_Err_t (*unmount)(MDS_FileSystem_t *fs);
    MDS_Err_t (*gc)(MDS_FileSystem_t *fs);

    MDS_Err_t (*unlink)(MDS_FileSystem_t *fs, const char *path);
    MDS_Err_t (*rename)(MDS_FileSystem_t *fs, const char *src, const char *dst);
    MDS_Err_t (*mkdir)(MDS_FileSystem_t *fs, const char *path);
    MDS_Err_t (*statfs)(MDS_FileSystem_t *fs, const char *path, MDS_FsStat_t *statfs);
    MDS_Err_t (*stat)(MDS_FileSystem_t *fs, const char *path, MDS_FileStat_t *stat);
} MDS_FileSystemOps_t;

#define MDS_FILE_SYSTEM_SECTION ".mds.fs."
#define MDS_FILE_SYSTEM_EXPORT(ops)                                                                \
    static const __attribute__((used, section(MDS_FILE_SYSTEM_SECTION)))                           \
    MDS_FileSystemOps_t *__FsOps_##ops##__ = &(ops);

struct MDS_FileSystem {
    MDS_DListNode_t mntNode;
    MDS_Mutex_t lock;

    const MDS_FsDevice_t *device;
    char *mntpath;
    size_t mntplen;

    const MDS_FileSystemOps_t *ops;
    MDS_Arg_t data;
};

struct MDS_FsFile {
    MDS_FileSystem_t *fs;
    MDS_Arg_t data;

    MDS_Mask_t flag;
};

struct MDS_FsDir {
    MDS_FileSystem_t *fs;
    MDS_Arg_t data;
};

/* Function ---------------------------------------------------------------- */
char *MDS_FileSystemNormalizePath(char *fullpath);
char *MDS_FileSystemJoinPath(const char *path, ...);
void MDS_FileSystemFreePath(char **path);

#define MDS_FS_JOIN_PATH(path, ...) MDS_FileSystemJoinPath(path, __VA_ARGS__, NULL)
#define MDS_FS_FREE_PATH(path)      MDS_FileSystemFreePath(&(path))

MDS_Err_t MDS_FsMkfs(MDS_FsDevice_t *device, const char *fsName, MDS_Arg_t init);
MDS_Err_t MDS_FsMount(MDS_FileSystem_t *fs, const MDS_FsDevice_t *device, const char *path,
                      const char *fsName);
MDS_Err_t MDS_FsUmount(MDS_FileSystem_t *fs);
MDS_Err_t MDS_FsGc(MDS_FileSystem_t *fs);

MDS_Err_t MDS_FileOpen(MDS_FsFile_t *file, const char *path, MDS_Mask_t oflag);
MDS_Err_t MDS_FileClose(MDS_FsFile_t *file);
MDS_FsResult_t MDS_FileRead(MDS_FsFile_t *file, void *buff, MDS_FsSize_t len);
MDS_FsResult_t MDS_FileWrite(MDS_FsFile_t *file, const void *buff, MDS_FsSize_t len);
MDS_FsResult_t MDS_FileSeek(MDS_FsFile_t *file, MDS_FsSize_t pos, MDS_FileWhence_t whence);
MDS_FsResult_t MDS_FileTell(MDS_FsFile_t *file);
MDS_FsResult_t MDS_FileTruncate(MDS_FsFile_t *file, MDS_FsSize_t len);
MDS_Err_t MDS_FileSync(MDS_FsFile_t *file);
MDS_Err_t MDS_FileIoctl(MDS_FsFile_t *file, MDS_FsCmd_t cmd, MDS_Arg_t arg);

MDS_Err_t MDS_FsOpenDir(MDS_FsDir_t *dir, const char *path);
MDS_Err_t MDS_FsCloseDir(MDS_FsDir_t *dir);
MDS_Err_t MDS_FsReadDir(MDS_FsDir_t *dir, MDS_FsDirent_t *dirent);

MDS_Err_t MDS_FsMkdir(const char *path);
MDS_Err_t MDS_FsUnlink(const char *path);
MDS_Err_t MDS_FsRename(const char *oldpath, const char *newpath);
MDS_Err_t MDS_FsSymLink(const char *srcpath, const char *dstpath);
MDS_Err_t MDS_FsStatFs(const char *path, MDS_FsStat_t *statfs);
MDS_Err_t MDS_FsStatFile(const char *path, MDS_FileStat_t *stat);

#ifdef __cplusplus
}
#endif

#endif /* __MDS_FS_H__ */
