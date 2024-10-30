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

/* Variable ---------------------------------------------------------------- */
static MDS_Mutex_t g_fsLock;
static MDS_ListNode_t g_fsList = {.prev = &g_fsList, .next = &g_fsList};

/* Memory ------------------------------------------------------------------ */
__attribute__((weak)) void *MDS_FileSystemMalloc(size_t size)
{
    return (MDS_SysMemAlloc(size));
}

__attribute__((weak)) void MDS_FileSystemFree(void *ptr)
{
    return (MDS_SysMemFree(ptr));
}

__attribute__((weak)) void *MDS_FileSystemCalloc(size_t numb, size_t size)
{
    return (MDS_SysMemCalloc(numb, size));
}

__attribute__((weak)) char *MDS_FileSystemStrdup(const char *str)
{
    size_t len = strlen(str) + 1;
    char *dup = MDS_FileSystemMalloc(len);
    if (dup != NULL) {
        MDS_MemBuffCopy(dup, len, str, len);
    }

    return (dup);
}

/* Path -------------------------------------------------------------------- */
static char *MDS_FileSystemNormalizePath(char *fullpath)
{
    char *src = fullpath;
    char *dst = fullpath;

    while (*src != '\0') {
        if (src[0] == '.') {
            if (src[1] == '/') {  // "./"
                for (src += 1; *src == '/'; src++) {
                }
                continue;
            } else if ((src[1] == '.') && (src[1 + 1] == '/')) {  // "../"
                for (src += 1 + 1; *src == '/'; src++) {
                }
                if (dst <= fullpath) {
                    return (NULL);
                }
                do {
                    dst--;
                } while ((fullpath < dst) && (dst[-1] != '/'));
            }
        }

        while ((*src != '\0') && (*src != '/')) {
            *dst++ = *src++;
        }
        if (*src == '/') {
            for (*dst++ = '/'; *src == '/'; src++) {
            }
        }
    }

    if (dst > fullpath) {
        dst[0] = '\0';
    }

    return (fullpath);
}

char *MDS_FileSystemJoinPath(const char *path, ...)
{
    va_list args;
    size_t len = 0;
    const char *s = path;

    va_start(args, path);
    for (const char *p = path; p != NULL; p = va_arg(args, const char *)) {
        if (p[0] == '/') {
            s = p;
            len = strlen(p) + 1;
        } else {
            len += strlen(p) + 1;
        }
    }
    va_end(args);

    char *fullpath = (len > 0) ? (MDS_FileSystemMalloc(len)) : (NULL);
    if (fullpath != NULL) {
        va_start(args, path);
        for (const char *p = path; p != s; p = va_arg(args, const char *)) {
        }
        size_t ofs = 0;
        while (s != NULL) {
            if (ofs != 0) {
                fullpath[ofs++] = '/';
            }
            for (size_t slen = strlen(s); slen != 0; slen--) {
                fullpath[ofs++] = *s++;
            }
            s = va_arg(args, const char *);
        }
        fullpath[ofs] = '\0';
        va_end(args);
    }

    return (MDS_FileSystemNormalizePath(fullpath));
}

/* FileSystem -------------------------------------------------------------- */
static const MDS_FileSystemOps_t *MDS_FileSystemFindOps(const char *fsName)
{
    static const void *mdsFsOpsBegin __attribute__((section(MDS_FILE_SYSTEM_SECTION "\000"))) = NULL;
    static const void *mdsFsOpsLimit __attribute__((section(MDS_FILE_SYSTEM_SECTION "\177"))) = NULL;
    const MDS_FileSystemOps_t *mdsFsOpsS = (const MDS_FileSystemOps_t *)((uintptr_t)(&mdsFsOpsBegin) + sizeof(void *));
    const MDS_FileSystemOps_t *mdsFsOPsE = (const MDS_FileSystemOps_t *)((uintptr_t)(&mdsFsOpsLimit));

    for (const MDS_FileSystemOps_t *ops = mdsFsOpsS; ops < mdsFsOPsE; ops++) {
        if (strcmp(ops->name, fsName) == 0) {
            return (ops);
        }
    }

    return (NULL);
}

static void MDS_FsLock(void)
{
    MDS_Err_t err;

    if (MDS_ObjectGetType(&(g_fsLock.object)) != MDS_OBJECT_TYPE_MUTEX) {
        err = MDS_MutexInit(&g_fsLock, "fs");
    }

    do {
        err = MDS_MutexAcquire(&g_fsLock, MDS_TICK_FOREVER);
    } while (err != MDS_EOK);
}

static void MDS_FsUnlock(void)
{
    MDS_MutexRelease(&g_fsLock);
}

static void MDS_FileSystemInsert(MDS_FileSystem_t *fs)
{
    MDS_FileSystem_t *find = NULL;
    MDS_FileSystem_t *iter = NULL;

    MDS_LIST_FOREACH_NEXT (iter, fsNode, &g_fsList) {
        if (strcmp(fs->path, iter->path) < 0) {
            find = iter;
            break;
        }
    }

    if (find == NULL) {
        MDS_ListInsertNodePrev(&g_fsList, &(fs->fsNode));
    } else {
        MDS_ListInsertNodePrev(&(find->fsNode), &(fs->fsNode));
    }
}

static MDS_FileSystem_t *MDS_FileSystemLookup(const char *abspath)
{
    MDS_FileSystem_t *iter = NULL;

    MDS_LIST_FOREACH_NEXT (iter, fsNode, &g_fsList) {
        if (strncmp(abspath, iter->path, strlen(iter->path)) == 0) {
            return (iter);
        }
    }

    return (NULL);
}

static const MDS_FileSystem_t *MDS_FileSystemFindMountDevice(MDS_FsDevice_t *device)
{
    MDS_FileSystem_t *iter = NULL;

    MDS_LIST_FOREACH_NEXT (iter, fsNode, &g_fsList) {
        if (iter->device != device) {
            continue;
        }
        return (iter);
    }

    return (NULL);
}

static const MDS_FileSystem_t *MDS_FileSystemFindMountPath(const char *abspath)
{
    size_t fplen = strlen(abspath);
    const MDS_FileSystem_t *iter = NULL;

    MDS_LIST_FOREACH_NEXT (iter, fsNode, &g_fsList) {
        size_t plen = strlen(iter->path);
        if (strncmp(abspath, iter->path, (fplen > plen) ? (plen) : (fplen))) {
            continue;
        }
        return (iter);
    }

    return (NULL);
}

MDS_Err_t MDS_FileSystemMkfs(MDS_FsDevice_t *device, const char *fsName, const MDS_Arg_t *init)
{
    MDS_ASSERT(device != NULL);
    MDS_ASSERT(fsName != NULL);

    MDS_Err_t err = MDS_EOK;

    MDS_FsLock();
    const MDS_FileSystem_t *fs = MDS_FileSystemFindMountDevice(device);
    if (fs != NULL) {
        err = MDS_EBUSY;
    }
    MDS_FsUnlock();

    if (err == MDS_EOK) {
        const MDS_FileSystemOps_t *ops = MDS_FileSystemFindOps(fsName);
        if (ops == NULL) {
            err = MDS_EIO;
        } else if (ops->mkfs != NULL) {
            err = ops->mkfs(device, init);
        }
    }

    return (err);
}

MDS_Err_t MDS_FileSystemMount(const MDS_FsDevice_t *device, const char *path, const char *fsName)
{
    MDS_ASSERT(path != NULL);
    MDS_ASSERT(fsName != NULL);

    MDS_Err_t err = MDS_EOK;

    const MDS_FileSystemOps_t *ops = MDS_FileSystemFindOps(fsName);
    if (ops == NULL) {
        return (MDS_ENOENT);
    }

    char *abspath = MDS_FileSystemJoinPath("/", path, "./", NULL);
    if (abspath == NULL) {
        return (MDS_ENOMEM);
    }

    MDS_FsLock();
    do {
        if (MDS_FileSystemFindMountPath(abspath) != NULL) {
            err = MDS_EEXIST;
            break;
        }

        MDS_FileSystem_t *fs = MDS_FileSystemCalloc(1, sizeof(MDS_FileSystem_t));
        if (fs == NULL) {
            err = MDS_ENOMEM;
            break;
        }

        MDS_MutexInit(&(fs->lock), "fs");
        MDS_ListInitNode(&(fs->fsNode));
        MDS_ListInitNode(&(fs->fileList));
        fs->path = abspath;
        fs->device = device;
        fs->ops = ops;

        if (ops->mount != NULL) {
            err = ops->mount(fs);
        }

        if (err == MDS_EOK) {
            MDS_FileSystemInsert(fs);
        } else {
            MDS_FileSystemFree(fs);
        }
    } while (0);
    MDS_FsUnlock();

    if (err != MDS_EOK) {
        MDS_FileSystemFree(abspath);
    }

    return (err);
}

MDS_Err_t MDS_FileSystemUnmount(const char *path)
{
    MDS_ASSERT(path != NULL);

    MDS_Err_t err = MDS_EOK;

    char *abspath = MDS_FileSystemJoinPath("/", path, "./", NULL);
    if (abspath == NULL) {
        return (MDS_ENOMEM);
    }

    MDS_FsLock();
    do {
        MDS_FileSystem_t *fs = MDS_FileSystemLookup(abspath);
        if (fs == NULL) {
            err = MDS_ENOENT;
            break;
        }

        MDS_MutexAcquire(&(fs->lock), MDS_TICK_FOREVER);
        if (!MDS_ListIsEmpty(&(fs->fileList))) {
            MDS_MutexRelease(&(fs->lock));
            err = MDS_EBUSY;
            break;
        }

        if ((fs->ops != NULL) && (fs->ops->unmount != NULL)) {
            err = fs->ops->unmount(fs);
        }

        if (err != MDS_EOK) {
            MDS_MutexRelease(&(fs->lock));
            break;
        }

        MDS_ListRemoveNode(&(fs->fsNode));
        MDS_MutexDeInit(&(fs->lock));
        MDS_FileSystemFree(fs->path);
        MDS_FileSystemFree(fs);
    } while (0);
    MDS_FsUnlock();

    MDS_FileSystemFree(abspath);

    return (err);
}

MDS_Err_t MDS_FileSystemStatfs(const char *path, MDS_FsStat_t *statfs)
{
    MDS_Err_t err = MDS_EIO;

    char *abspath = MDS_FileSystemJoinPath("/", path, "./", NULL);
    if (abspath == NULL) {
        return (MDS_ENOMEM);
    }

    MDS_FsLock();
    MDS_FileSystem_t *fs = MDS_FileSystemLookup(abspath);
    MDS_FsUnlock();

    if ((fs != NULL) && (fs->ops != NULL) && (fs->ops->statfs != NULL)) {
        err = fs->ops->statfs(fs, statfs);
    }

    MDS_FileSystemFree(abspath);

    return (err);
}

/* File -------------------------------------------------------------------- */
static MDS_FileNode_t *MDS_FileNodeLookup(MDS_FileSystem_t *fs, const char *path)
{
    MDS_FileNode_t *iter = NULL;
    MDS_LIST_FOREACH_NEXT (iter, fileNode, &(fs->fileList)) {
        if (strcmp(iter->path, path) == 0) {
            return (iter);
        }
    }

    return (NULL);
}

static MDS_FileNode_t *MDS_FileNodeCreate(MDS_FileSystem_t *fs, const char *abspath)
{
    MDS_FileNode_t *fnode = MDS_FileSystemCalloc(1, sizeof(MDS_FileNode_t));
    if (fnode != NULL) {
        fnode->path = MDS_FileSystemStrdup(&(abspath[strlen(fs->path)]));
        if (fnode->path == NULL) {
            MDS_FileSystemFree(fnode);
            fnode = NULL;
        } else {
            MDS_ListInitNode(&(fnode->fileNode));
            MDS_ListInsertNodePrev(&(fs->fileList), &(fnode->fileNode));
            fnode->refCount = 1;
            fnode->fs = fs;
        }
    }

    return (fnode);
}

static void MDS_FileNodeDestroy(MDS_FileNode_t *fnode)
{
    fnode->refCount -= 1;

    if (fnode->refCount < 0) {
        MDS_LOG_E("[fs][FileNodeDestory] fnode refCount %d < 0", fnode->refCount);
    }

    if (fnode->refCount == 0) {
        MDS_ListRemoveNode(&(fnode->fileNode));
        MDS_FileSystemFree(fnode->path);
        MDS_FileSystemFree(fnode);
    }
}

MDS_Err_t MDS_FileOpen(MDS_FileDesc_t *fd, const char *path, MDS_Mask_t flags)
{
    MDS_ASSERT(fd != NULL);
    MDS_ASSERT(path != NULL);

    MDS_Err_t err = MDS_EOK;

    if (fd->node != NULL) {
        return (MDS_EEXIST);
    }

    char *abspath = MDS_FileSystemJoinPath("/", path, NULL);
    if (abspath == NULL) {
        return (MDS_EINVAL);
    }

    MDS_FsLock();
    MDS_FileSystem_t *fs = MDS_FileSystemLookup(abspath);
    MDS_FsUnlock();
    if ((fs == NULL) || (fs->ops == NULL) || (fs->ops->open == NULL)) {
        MDS_FileSystemFree(abspath);
        return (MDS_ENOENT);
    }

    MDS_MutexAcquire(&(fs->lock), MDS_TICK_FOREVER);
    do {
        MDS_FileNode_t *fnode = MDS_FileNodeLookup(fs, abspath);
        if (fnode != NULL) {
            fnode->refCount += 1;
        } else {
            fnode = MDS_FileNodeCreate(fs, abspath);
            if (fnode == NULL) {
                err = MDS_ENOMEM;
                break;
            }
        }

        fd->node = fnode;
        fd->pos = 0;
        fd->flags = flags | MDS_OFLAG_OPEN;

        err = fs->ops->open(fd);
        if (err != MDS_EOK) {
            MDS_FileNodeDestroy(fnode);
            fd->node = NULL;
            fd->pos = 0;
            fd->flags = MDS_OFLAG_NONE;
        }
    } while (0);
    MDS_MutexRelease(&(fs->lock));

    MDS_FileSystemFree(abspath);

    return (err);
}

MDS_Err_t MDS_FileClose(MDS_FileDesc_t *fd)
{
    MDS_ASSERT(fd != NULL);
    MDS_ASSERT(fd->node != NULL);
    MDS_ASSERT(fd->node->fs != NULL);

    MDS_Err_t err = MDS_EOK;
    MDS_FileNode_t *fnode = fd->node;
    MDS_FileSystem_t *fs = fnode->fs;

    MDS_MutexAcquire(&(fs->lock), MDS_TICK_FOREVER);
    do {
        if ((fs->ops != NULL) && (fs->ops->close != NULL)) {
            err = fs->ops->close(fd);
        }
        if (err != MDS_EOK) {
            break;
        }

        MDS_FileNodeDestroy(fnode);
        fd->node = NULL;
        fd->pos = 0;
        fd->flags = MDS_OFLAG_NONE;
    } while (0);
    MDS_MutexRelease(&(fs->lock));

    return (err);
}

MDS_Err_t MDS_FileIoctl(MDS_FileDesc_t *fd, MDS_Item_t cmd, MDS_Arg_t *args)
{
    MDS_ASSERT(fd != NULL);

    if ((fd->node == NULL) || ((fd->flags & MDS_OFLAG_OPEN) == 0U)) {
        return (MDS_EACCES);
    }

    if ((fd->node->fs == NULL) || (fd->node->fs->ops == NULL) || (fd->node->fs->ops->ioctl == NULL)) {
        return (MDS_EPERM);
    }

    return (fd->node->fs->ops->ioctl(fd, cmd, args));
}

MDS_FileOffset_t MDS_FileRead(MDS_FileDesc_t *fd, uint8_t *buff, MDS_FileSize_t len)
{
    MDS_ASSERT(fd != NULL);

    if ((fd->node == NULL) ||
        ((fd->flags & (MDS_OFLAG_OPEN | MDS_OFLAG_RDWR)) == (MDS_OFLAG_OPEN | MDS_OFLAG_RDONLY))) {
        return (MDS_EACCES);
    }

    if ((fd->node->fs == NULL) || (fd->node->fs->ops == NULL) || (fd->node->fs->ops->read == NULL)) {
        return (MDS_EPERM);
    }

    return (fd->node->fs->ops->read(fd, buff, len, &(fd->pos)));
}

MDS_FileOffset_t MDS_FilePosRead(MDS_FileDesc_t *fd, uint8_t *buff, MDS_FileSize_t len, MDS_FileOffset_t pos)
{
    MDS_ASSERT(fd != NULL);

    if ((fd->node == NULL) ||
        ((fd->flags & (MDS_OFLAG_OPEN | MDS_OFLAG_RDWR)) == (MDS_OFLAG_OPEN | MDS_OFLAG_RDONLY))) {
        return (MDS_EACCES);
    }

    if ((fd->node->fs == NULL) || (fd->node->fs->ops == NULL) || (fd->node->fs->ops->read == NULL)) {
        return (MDS_EPERM);
    }

    return (fd->node->fs->ops->read(fd, buff, len, &pos));
}

MDS_FileOffset_t MDS_FileWrite(MDS_FileDesc_t *fd, const uint8_t *buff, MDS_FileSize_t len)
{
    MDS_ASSERT(fd != NULL);

    if ((fd->node == NULL) ||
        ((fd->flags & (MDS_OFLAG_OPEN | MDS_OFLAG_RDWR)) == (MDS_OFLAG_OPEN | MDS_OFLAG_WRONLY))) {
        return (MDS_EACCES);
    }

    if ((fd->node->fs == NULL) || (fd->node->fs->ops == NULL) || (fd->node->fs->ops->write == NULL)) {
        return (MDS_EPERM);
    }

    return (fd->node->fs->ops->write(fd, buff, len, &(fd->pos)));
}

MDS_FileOffset_t MDS_FilPosWrite(MDS_FileDesc_t *fd, const uint8_t *buff, MDS_FileSize_t len, MDS_FileOffset_t pos)
{
    MDS_ASSERT(fd != NULL);

    if ((fd->node == NULL) ||
        ((fd->flags & (MDS_OFLAG_OPEN | MDS_OFLAG_RDWR)) == (MDS_OFLAG_OPEN | MDS_OFLAG_WRONLY))) {
        return (MDS_EACCES);
    }

    if ((fd->node->fs == NULL) || (fd->node->fs->ops == NULL) || (fd->node->fs->ops->write == NULL)) {
        return (MDS_EPERM);
    }

    return (fd->node->fs->ops->write(fd, buff, len, &pos));
}

MDS_Err_t MDS_FileFlush(MDS_FileDesc_t *fd)
{
    MDS_ASSERT(fd != NULL);

    if ((fd->node == NULL) || ((fd->flags & MDS_OFLAG_OPEN) == MDS_OFLAG_NONE)) {
        return (MDS_EACCES);
    }

    if ((fd->node->fs == NULL) || (fd->node->fs->ops == NULL) || (fd->node->fs->ops->flush == NULL)) {
        return (MDS_EPERM);
    }

    return (fd->node->fs->ops->flush(fd));
}

MDS_FileOffset_t MDS_FilePosition(MDS_FileDesc_t *fd)
{
    MDS_ASSERT(fd != NULL);

    return (fd->pos);
}

MDS_FileOffset_t MDS_FileSeek(MDS_FileDesc_t *fd, MDS_FileOffset_t offset, MDS_FileWhence_t whence)
{
    MDS_ASSERT(fd != NULL);

    if ((fd->node == NULL) || ((fd->flags & MDS_OFLAG_OPEN) == MDS_OFLAG_NONE)) {
        return (MDS_EACCES);
    }

    if ((fd->node->fs == NULL) || (fd->node->fs->ops == NULL) || (fd->node->fs->ops->seek == NULL)) {
        return (MDS_EPERM);
    }

    fd->pos = fd->node->fs->ops->seek(fd, offset, whence);

    return (fd->pos);
}

MDS_FileOffset_t MDS_FileTruncate(MDS_FileDesc_t *fd, MDS_FileOffset_t len)
{
    MDS_ASSERT(fd != NULL);

    if ((fd->node == NULL) ||
        ((fd->flags & (MDS_OFLAG_OPEN | MDS_OFLAG_RDWR)) == (MDS_OFLAG_OPEN | MDS_OFLAG_WRONLY))) {
        return (MDS_EACCES);
    }

    if ((fd->node->fs == NULL) || (fd->node->fs->ops == NULL) || (fd->node->fs->ops->truncate == NULL)) {
        return (MDS_EPERM);
    }

    MDS_Err_t err = fd->node->fs->ops->truncate(fd, len);
    if ((err == MDS_EOK) && (fd->pos > len)) {
        fd->pos = len;
    }

    return (err);
}

MDS_FileOffset_t MDS_FileTell(MDS_FileDesc_t *fd)
{
    return (MDS_FileSeek(fd, 0, MDS_FILE_SEEK_CUR));
}

MDS_FileOffset_t MDS_FileRewind(MDS_FileDesc_t *fd)
{
    return (MDS_FileSeek(fd, 0, MDS_FILE_SEEK_SET));
}

MDS_FileSize_t MDS_FileSize(MDS_FileDesc_t *fd)
{
    return (MDS_FileSeek(fd, 0, MDS_FILE_SEEK_END));
}

MDS_Err_t MDS_FileMkdir(const char *path)
{
    MDS_Err_t err = MDS_EPERM;

    char *abspath = MDS_FileSystemJoinPath("/", path, "./", NULL);
    if (abspath == NULL) {
        return (MDS_ENOMEM);
    }

    MDS_FsLock();
    MDS_FileSystem_t *fs = MDS_FileSystemLookup(abspath);
    MDS_FsUnlock();

    do {
        if ((fs == NULL) || (fs->ops == NULL) || (fs->ops->mkdir == NULL)) {
            break;
        }

        const char *fspath = &(abspath[strlen(fs->path)]);

        MDS_MutexAcquire(&(fs->lock), MDS_TICK_FOREVER);
        MDS_FileNode_t *fnode = MDS_FileNodeLookup(fs, fspath);
        if (fnode == NULL) {
            err = fs->ops->mkdir(fs, fspath);
        } else {
            err = MDS_EBUSY;
        }
        MDS_MutexRelease(&(fs->lock));
    } while (0);

    MDS_FileSystemFree(abspath);

    return (err);
}

MDS_Err_t MDS_FileRemove(const char *path)
{
    MDS_Err_t err = MDS_EPERM;

    char *abspath = MDS_FileSystemJoinPath("/", path, NULL);
    if (abspath == NULL) {
        return (MDS_ENOMEM);
    }

    MDS_FsLock();
    MDS_FileSystem_t *fs = MDS_FileSystemLookup(abspath);
    MDS_FsUnlock();

    do {
        if ((fs == NULL) || (fs->ops == NULL) || (fs->ops->remove == NULL)) {
            break;
        }

        const char *fspath = &(abspath[strlen(fs->path)]);

        MDS_MutexAcquire(&(fs->lock), MDS_TICK_FOREVER);
        MDS_FileNode_t *fnode = MDS_FileNodeLookup(fs, fspath);
        if (fnode == NULL) {
            err = fs->ops->remove(fs, fspath);
        } else {
            err = MDS_EBUSY;
        }
        MDS_MutexRelease(&(fs->lock));
    } while (0);

    MDS_FileSystemFree(abspath);

    return (err);
}

MDS_Err_t MDS_FileRename(const char *oldpath, const char *newpath)
{
    MDS_ASSERT(oldpath != NULL);
    MDS_ASSERT(newpath != NULL);

    MDS_Err_t err = MDS_EPERM;
    char *oldabspath = NULL;
    char *newabspath = NULL;

    do {
        oldabspath = MDS_FileSystemJoinPath("/", oldpath, NULL);
        if (oldabspath == NULL) {
            err = MDS_ENOMEM;
            break;
        }

        newabspath = MDS_FileSystemJoinPath("/", newpath, NULL);
        if (newabspath == NULL) {
            err = MDS_ENOMEM;
            break;
        }

        MDS_FsLock();
        MDS_FileSystem_t *oldfs = MDS_FileSystemLookup(oldabspath);
        MDS_FileSystem_t *newfs = MDS_FileSystemLookup(newabspath);
        MDS_FsUnlock();
        if (oldfs != newfs) {
            // copy
            break;
        }

        if ((oldfs == NULL) || (oldfs->ops == NULL) || (oldfs->ops->rename == NULL)) {
            break;
        }

        const char *oldfspath = &(oldabspath[strlen(oldfs->path)]);
        const char *newfspath = &(newabspath[strlen(newfs->path)]);

        MDS_MutexAcquire(&(oldfs->lock), MDS_TICK_FOREVER);
        MDS_FileNode_t *fnode = MDS_FileNodeLookup(oldfs, oldfspath);
        if (fnode == NULL) {
            err = oldfs->ops->rename(oldfs, oldfspath, newfspath);
        } else {
            err = MDS_EBUSY;
        }
        MDS_MutexRelease(&(oldfs->lock));
    } while (0);

    if (oldabspath != NULL) {
        MDS_FileSystemFree(oldabspath);
    }
    if (newabspath != NULL) {
        MDS_FileSystemFree(newabspath);
    }

    return (err);
}

MDS_Err_t MDS_FileLink(const char *srcpath, const char *dstpath)
{
    MDS_ASSERT(srcpath != NULL);
    MDS_ASSERT(dstpath != NULL);

    MDS_Err_t err = MDS_EPERM;
    char *srcabspath = NULL;
    char *dstabspath = NULL;

    do {
        srcabspath = MDS_FileSystemJoinPath("/", srcpath, NULL);
        if (srcabspath == NULL) {
            err = MDS_ENOMEM;
            break;
        }

        dstabspath = MDS_FileSystemJoinPath("/", dstpath, NULL);
        if (dstabspath == NULL) {
            err = MDS_ENOMEM;
            break;
        }

        MDS_FsLock();
        MDS_FileSystem_t *srcfs = MDS_FileSystemLookup(srcabspath);
        MDS_FileSystem_t *dstfs = MDS_FileSystemLookup(dstabspath);
        MDS_FsUnlock();
        if (srcfs != dstfs) {
            // cross
            break;
        }

        if ((srcfs == NULL) || (srcfs->ops == NULL) || (srcfs->ops->link == NULL)) {
            break;
        }

        const char *srcfspath = &(srcabspath[strlen(srcfs->path)]);
        const char *dstfspath = &(dstabspath[strlen(dstfs->path)]);

        err = srcfs->ops->link(srcfs, srcfspath, dstfspath);
    } while (0);

    if (srcabspath != NULL) {
        MDS_FileSystemFree(srcabspath);
    }
    if (dstabspath != NULL) {
        MDS_FileSystemFree(dstabspath);
    }

    return (err);
}

MDS_Err_t MDS_FileStat(const char *path, MDS_FileStat_t *stat)
{
    MDS_ASSERT(path != NULL);

    MDS_Err_t err = MDS_EPERM;

    char *abspath = MDS_FileSystemJoinPath("/", path, NULL);
    if (abspath == NULL) {
        return (MDS_ENOMEM);
    }

    MDS_FsLock();
    MDS_FileSystem_t *fs = MDS_FileSystemLookup(abspath);
    MDS_FsUnlock();

    if ((fs != NULL) && (fs->ops != NULL) && (fs->ops->stat != NULL)) {
        err = fs->ops->stat(fs, &(abspath[strlen(fs->path)]), stat);
    }

    MDS_FileSystemFree(abspath);

    return (err);
}
