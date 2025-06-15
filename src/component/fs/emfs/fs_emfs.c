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
#include "mds_fs.h"
#include "mds_emfs.h"

#if (defined(MDS_FILESYSTEM_WITH_FULL) && (MDS_FILESYSTEM_WITH_FULL != 0))

#else
static MDS_Err_t EMFS_Remove(MDS_FileSystem_t *fs, const char *path)
{
    MDS_EMFS_FileSystem_t *emfs = (MDS_EMFS_FileSystem_t *)(fs->data);
    MDS_EMFS_FileId_t id = (MDS_EMFS_FileId_t)(uintptr_t)(path);

    return (MDS_EMFS_Remove(emfs, id));
}

static MDS_Err_t EMFS_Open(MDS_FileDesc_t *fd)
{
    MDS_EMFS_FileDesc_t *emfd = (MDS_EMFS_FileDesc_t *)(fd->node->data);
    MDS_EMFS_FileSystem_t *emfs = (MDS_EMFS_FileSystem_t *)(fd->node->fs);
    MDS_EMFS_FileId_t id = (MDS_EMFS_FileId_t)(uintptr_t)(fd->node->path);

    MDS_Err_t err = MDS_EMFS_FileOpen(emfd, emfs, id);

    if ((fd->flags & MDS_FILE_OFLAG_CREAT) != 0U) {
        if ((err == MDS_EOK) && ((fd->flags & MDS_FILE_OFLAG_EXCL) != 0U)) {
            MDS_EMFS_FileClose(emfd);
            return (MDS_EEXIST);
        }
        if (err == MDS_ENOENT) {
            err = MDS_EMFS_FileCreate(emfd, emfs, id);
            if (err != MDS_EOK) {
                return (err);
            }
            err = MDS_EMFS_FileOpen(emfd, emfs, id);
        }
    }

    if (err == MDS_EOK) {
        if ((fd->flags & MDS_FILE_OFLAG_TRUNC) != 0U) {
            MDS_EMFS_FileTruncate(emfd, 0);
        }
        if ((fd->flags & MDS_FILE_OFLAG_APPEND) != 0U) {
            MDS_EMFS_FileSize(emfd, (size_t *)(&(fd->pos)), NULL);
        }
    }

    return (err);
}

static MDS_Err_t EMFS_Close(MDS_FileDesc_t *fd)
{
    MDS_EMFS_FileDesc_t *emfd = (MDS_EMFS_FileDesc_t *)(fd->node->data);

    return (MDS_EMFS_FileClose(emfd));
}

static MDS_FileOffset_t EMFS_Read(MDS_FileDesc_t *fd, uint8_t *buff, MDS_FileSize_t len, MDS_FileOffset_t *pos)
{
    size_t read = 0;
    MDS_EMFS_FileDesc_t *emfd = (MDS_EMFS_FileDesc_t *)(fd->node->data);

    MDS_EMFS_FileRead(emfd, *pos, buff, len, &read);

    return (read);
}

static MDS_FileOffset_t EMFS_Write(MDS_FileDesc_t *fd, const uint8_t *buff, MDS_FileSize_t len, MDS_FileOffset_t *pos)
{
    size_t write = 0;
    MDS_EMFS_FileDesc_t *emfd = (MDS_EMFS_FileDesc_t *)(fd->node->data);

    MDS_EMFS_FileWrite(emfd, *pos, buff, len, &write);

    return (write);
}

static MDS_FileOffset_t EMFS_Seek(MDS_FileDesc_t *fd, MDS_FileOffset_t pos, MDS_FileWhence_t whence)
{
    MDS_FileOffset_t ofs = -1;
    MDS_EMFS_FileDesc_t *emfd = (MDS_EMFS_FileDesc_t *)(fd->node->data);

    size_t used = 0;

    if (MDS_EMFS_FileSize(emfd, &used, NULL) == MDS_EOK) {
        switch (whence) {
            case MDS_FILE_SEEK_SET:
                ofs = ((size_t)pos > used) ? (-1) : (pos);
                break;
            case MDS_FILE_SEEK_CUR:
                ofs = ((size_t)(fd->pos + pos) > used) ? (-1) : (fd->pos + pos);
                break;
            case MDS_FILE_SEEK_END:
                ofs = used;
                break;
            default:
                break;
        }
    }

    return (ofs);
}

static MDS_FileOffset_t EMFS_Truncate(MDS_FileDesc_t *fd, MDS_FileOffset_t len)
{
    MDS_EMFS_FileDesc_t *emfd = (MDS_EMFS_FileDesc_t *)(fd->node->data);

    size_t pos = MDS_EMFS_FileTruncate(emfd, len);

    return (pos);
}

const MDS_FileSystemOps_t G_FILESYSTEM_EMFS_OPS = {
    .remove = EMFS_Remove,
    .open = EMFS_Open,
    .close = EMFS_Close,
    .flush = NULL,
    .ioctl = NULL,
    .read = EMFS_Read,
    .write = EMFS_Write,
    .seek = EMFS_Seek,
    .truncate = EMFS_Truncate,
};
#endif

