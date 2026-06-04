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
#include "mds/fs/fs.h"

/* Define ------------------------------------------------------------------ */
MDS_LOG_MODULE_DEFINE(fs, CONFIG_MDS_FS_LOG_LEVEL);

/* Typedef ----------------------------------------------------------------- */
enum {
    MDS_FS_FLAG_OPEN = 0x8000U,
    MDS_FS_FLAG_READ = MDS_FS_FLAG_OPEN | MDS_FS_OFLAG_RDONLY,
    MDS_FS_FLAG_WRITE = MDS_FS_FLAG_OPEN | MDS_FS_OFLAG_WRONLY,
};

/* Variable ---------------------------------------------------------------- */
static MDS_Mutex_t g_fsLock;
static MDS_DListNode_t g_fsMntList = {.prev = &g_fsMntList, .next = &g_fsMntList};

/* Memory ------------------------------------------------------------------ */
__attribute__((weak)) void *MDS_FileSystemMalloc(size_t size)
{
    return (MDS_SysMemAlloc(size));
}

__attribute__((weak)) void MDS_FileSystemFree(void *ptr)
{
    return (MDS_SysMemFree(ptr));
}

/* Path -------------------------------------------------------------------- */
char *MDS_FileSystemNormalizePath(char *fullpath)
{
    if (fullpath == NULL) {
        return (NULL);
    }

    char *src = fullpath;
    char *dst = fullpath;

    while (*src != '\0') {
        if (src[0] == '.') {
            // "./"
            if ((src[1] == '/') || (src[1] == '\\') || (src[1] == '\0')) {
                src += (src[1] != '\0') ? 2 : 1;
                continue;
            }

            // "../"
            if ((src[1] == '.') && ((src[2] == '/') || (src[2] == '\\') || (src[2] == '\0'))) {
                if ((dst == fullpath) ||
                    (((dst - fullpath) > 2) && (dst[-1] == '/') && (dst[-2] == '.') &&
                     (dst[-3] == '.'))) {
                    *dst++ = '.';
                    *dst++ = '.';
                    if (src[2] != '\0') {
                        *dst++ = '/';
                    }
                } else if (((dst - fullpath) > 1) && (dst[-1] == '/')) {
                    do {
                        dst--;
                    } while ((dst > fullpath) && (dst[-1] != '/'));
                }
                src += (src[2] != '\0') ? 3 : 2;
                continue;
            }
        }

        while ((src[0] != '\0') && (src[0] != '/') && (src[0] != '\\')) {
            *dst++ = *src++;
        }

        if ((src[0] == '/') || (src[0] == '\\')) {
            if ((dst > fullpath) && (dst[-1] != '/')) {
                *dst++ = '/';
            } else if (dst == fullpath) {
                *dst++ = '/';
            }
            for (src++; (src[0] == '/') || (src[0] == '\\'); ++src) {
            }
        }
    }

    *dst = '\0';

    return (fullpath);
}

char *MDS_FileSystemJoinPath(const char *path, ...)
{
    if (path == NULL) {
        return (NULL);
    }

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
            const char *t = s;
            for (size_t slen = strlen(t); slen != 0; slen--) {
                fullpath[ofs++] = *t++;
            }
            s = va_arg(args, const char *);
        }
        fullpath[ofs] = '\0';
        va_end(args);
    }

    return (MDS_FileSystemNormalizePath(fullpath));
}

void MDS_FileSystemFreePath(char **path)
{
    if ((path != NULL) && (*path != NULL)) {
        MDS_FileSystemFree(*path);
        *path = NULL;
    }
}

/* FileSystem -------------------------------------------------------------- */
static const MDS_FileSystemOps_t *MDS_FileSystemFindOps(const char *fsName)
{
    static const __attribute__((section(MDS_FILE_SYSTEM_SECTION "\000"))) void *mdsFsOpsBegin =
        NULL;
    static const __attribute__((section(MDS_FILE_SYSTEM_SECTION "\177"))) void *mdsFsOpsLimit =
        NULL;
    const MDS_FileSystemOps_t *mdsFsOpsS =
        (const MDS_FileSystemOps_t *)((uintptr_t)(&mdsFsOpsBegin) + sizeof(void *));
    const MDS_FileSystemOps_t *mdsFsOPsE =
        (const MDS_FileSystemOps_t *)((uintptr_t)(&mdsFsOpsLimit));

    for (const MDS_FileSystemOps_t *ops = mdsFsOpsS; ops < mdsFsOPsE; ops++) {
        if (strcmp(ops->name, fsName) == 0) {
            return (ops);
        }
    }

    return (NULL);
}

static void MDS_FileSystemLock(void)
{
    MDS_Err_t err;

    MDS_Lock_t lock = MDS_CriticalLock(NULL);

    MDS_ObjectType_t type = MDS_ObjectGetType(&(g_fsLock.object));
    if (type != MDS_OBJECT_TYPE_MUTEX) {
        err = MDS_MutexInit(&g_fsLock, "fs");
    }
#if (defined(CONFIG_MDS_FS_FILENODE_MAX) && (CONFIG_MDS_FS_FILENODE_MAX > 0))
    if (g_fsNodePool == NULL) {
        g_fsNodePool = MDS_MemPoolCreate("fs", sizeof(MDS_FsNode_t), CONFIG_MDS_FS_FILENODE_MAX);
    }
#endif
    MDS_CriticalRestore(NULL, lock);

    do {
        err = MDS_MutexAcquire(&g_fsLock, MDS_TIMEOUT_FOREVER);
    } while (!MDS_ErrIsSame(err, MDS_EOK));
}

static void MDS_FileSystemUnlock(void)
{
    MDS_MutexRelease(&g_fsLock);
}

static const MDS_FileSystem_t *MDS_FileSystemFindByDevice(MDS_FsDevice_t *device)
{
    MDS_FileSystem_t *iter = NULL;

    MDS_DLIST_CONTAINER_FOREACH_NEXT (iter, mntNode, &g_fsMntList) {
        if (iter->device != device) {
            continue;
        }
        return (iter);
    }

    return (NULL);
}

static void MDS_FileSystemInsert(MDS_FileSystem_t *fs)
{
    MDS_FileSystem_t *find = NULL;
    MDS_FileSystem_t *iter = NULL;

    MDS_DLIST_CONTAINER_FOREACH_NEXT (iter, mntNode, &g_fsMntList) {
        if (strcmp(fs->mntpath, iter->mntpath) < 0) {
            find = iter;
            break;
        }
    }

    if (find == NULL) {
        MDS_DListInsertNodePrev(&g_fsMntList, &(fs->mntNode));
    } else {
        MDS_DListInsertNodePrev(&(find->mntNode), &(fs->mntNode));
    }
}

static MDS_FileSystem_t *MDS_FileSystemLookupFs(const char *abspath)
{
    if ((abspath == NULL) || (abspath[0] != '/')) {
        return (NULL);
    }

    size_t slen = strlen(abspath);
    if ((slen > 0) && (abspath[slen - 1] == '/')) {
        slen -= 1;
    }

    MDS_FileSystem_t *iter = NULL;

    MDS_DLIST_CONTAINER_FOREACH_PREV (iter, mntNode, &g_fsMntList) {
        size_t plen = iter->mntplen; //strlen(iter->mntpath);
        if ((plen > 0) && (iter->mntpath[plen - 1] == '/')) {
            plen -= 1;
        }

        if (plen <= slen) {
            if ((plen == 0) && (slen == 0)) {
                return (iter);
            }
            if (((abspath[plen] == '/') || (abspath[plen] == '\0')) &&
                (strncmp(iter->mntpath, abspath, plen) == 0)) {
                return (iter);
            }
        }
    }

    return (NULL);
}

MDS_Err_t MDS_FsMkfs(MDS_FsDevice_t *device, const char *fsName, MDS_Arg_t init)
{
    MDS_ASSERT(device != NULL);
    MDS_ASSERT(fsName != NULL);

    MDS_Err_t err;

    do {
        MDS_FileSystemLock();
        const MDS_FileSystem_t *fs = MDS_FileSystemFindByDevice(device);
        MDS_FileSystemUnlock();

        if (fs != NULL) {
            err = MDS_EBUSY;
            break;
        }

        const MDS_FileSystemOps_t *ops = MDS_FileSystemFindOps(fsName);
        if (ops == NULL) {
            err = MDS_ENOENT;
            break;
        }

        if (ops->mkfs == NULL) {
            err = MDS_EIO;
            break;
        }

        err = ops->mkfs(device, init);
    } while (0);

    MDS_LOG_W("device(%p) mkfs err(%d)", device, err.errno);

    return (err);
}

MDS_Err_t MDS_FsMount(MDS_FileSystem_t *fs, const MDS_FsDevice_t *device, const char *path,
                      const char *fsName)
{
    MDS_ASSERT(path != NULL);
    MDS_ASSERT(fsName != NULL);

    MDS_Err_t err;
    char *mntpath = NULL;

    MDS_FileSystemLock();
    do {
        // if (!MDS_DListIsEmpty(&(fs->nodeList))) {
        //     err = MDS_EEXIST;
        //     break;
        // }

        const MDS_FileSystemOps_t *ops = MDS_FileSystemFindOps(fsName);
        if (ops == NULL) {
            err = MDS_ENOENT;
            break;
        }

        if ((path == NULL) || (path[0] != '/')) {
            err = MDS_EINVAL;
            break;
        }
        mntpath = MDS_FileSystemJoinPath("/", path, "./", NULL);
        if (mntpath == NULL) {
            err = MDS_ENOMEM;
            break;
        }

        MDS_MutexInit(&(fs->lock), "fs");
        MDS_DListInitNode(&(fs->mntNode));
        // MDS_DListInitNode(&(fs->nodeList));
        fs->device = device;
        fs->mntpath = mntpath;
        fs->mntplen = strlen(mntpath);
        fs->ops = ops;

        if ((ops != NULL) && (ops->mount != NULL)) {
            err = ops->mount(fs);
        } else {
            err = MDS_EIO;
        }

        if (MDS_ErrIsSame(err, MDS_EOK)) {
            MDS_FileSystemInsert(fs);
        }
    } while (0);
    MDS_FileSystemUnlock();

    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_FileSystemFreePath(&mntpath);
        MDS_LOG_E("filesytem(%p) mount err(%d)", fs, err.errno);
    } else {
        MDS_LOG_I("filesystem(%p) mount success", fs);
    }

    return (err);
}

MDS_Err_t MDS_FsUmount(MDS_FileSystem_t *fs)
{
    MDS_ASSERT(fs != NULL);

    MDS_Err_t err;

    MDS_FileSystemLock();
    do {
        MDS_MutexAcquire(&(fs->lock), MDS_TIMEOUT_FOREVER);
        // if (!MDS_DListIsEmpty(&(fs->nodeList))) {
        //     MDS_MutexRelease(&(fs->lock));
        //     err = MDS_EBUSY;
        //     break;
        // }

        if ((fs->ops != NULL) && (fs->ops->unmount != NULL)) {
            err = fs->ops->unmount(fs);
        } else {
            err = MDS_EIO;
        }

        MDS_MutexRelease(&(fs->lock));
        if (!MDS_ErrIsSame(err, MDS_EOK)) {
            break;
        }

        MDS_DListRemoveNode(&(fs->mntNode));
        MDS_MutexDeInit(&(fs->lock));
        MDS_FileSystemFreePath(&(fs->mntpath));

        fs->mntpath = NULL;
        fs->device = NULL;
        fs->ops = NULL;
        fs->data = MDS_ARG_WITH(NULL);
    } while (0);
    MDS_FileSystemUnlock();

    MDS_LOG_I("filesystem(%p) mount err(%d)", fs, err.errno);

    return (err);
}

MDS_Err_t MDS_FsGc(MDS_FileSystem_t *fs)
{
    MDS_ASSERT(fs != NULL);

    MDS_Err_t err;

    MDS_MutexAcquire(&(fs->lock), MDS_TIMEOUT_FOREVER);
    do {
        // if (!MDS_DListIsEmpty(&(fs->nodeList))) {
        //     err = MDS_EBUSY;
        //     break;
        // }

        if ((fs->ops != NULL) && (fs->ops->gc != NULL)) {
            err = fs->ops->gc(fs);
        } else {
            err = MDS_EIO;
        }

        MDS_LOG_I("filesystem(%p) gc err(%d)", fs, err.errno);
    } while (0);
    MDS_MutexRelease(&(fs->lock));

    return (err);
}

MDS_Err_t MDS_FileOpen(MDS_FsFile_t *file, const char *path, MDS_Mask_t oflag)
{
    MDS_ASSERT(file != NULL);
    MDS_ASSERT(path != NULL);

    MDS_Err_t err;
    MDS_FileSystem_t *fs = NULL;
    bool truncate = false;

    if ((file->fs != NULL) || (file->data.ptr != NULL) ||
        ((file->flag.mask & MDS_FS_FLAG_OPEN) == MDS_FS_FLAG_OPEN)) {
        return (MDS_EBUSY);
    }

    if ((path == NULL) || (path[0] != '/')) {
        return (MDS_EINVAL);
    }
    char *abspath = MDS_FileSystemJoinPath("/", path, NULL);
    if (abspath == NULL) {
        return (MDS_ENOMEM);
    }

    do {
        MDS_FileSystemLock();
        fs = MDS_FileSystemLookupFs(abspath);
        MDS_FileSystemUnlock();

        if (fs == NULL) {
            err = MDS_ENOENT;
            break;
        }

        if ((fs->ops == NULL) || (fs->ops->open == NULL)) {
            err = MDS_ENOTSUP;
            break;
        }

        if ((oflag.mask & MDS_FS_OFLAG_TRUNC) == MDS_FS_OFLAG_TRUNC) {
            if ((oflag.mask & MDS_FS_OFLAG_WRONLY) != MDS_FS_OFLAG_WRONLY) {
                err = MDS_EACCES;
                break;
            }
            if ((fs->ops == NULL) || (fs->ops->truncate == NULL)) {
                err = MDS_ENOTSUP;
                break;
            }
            truncate = true;
        }

        MDS_MutexAcquire(&(fs->lock), MDS_TIMEOUT_FOREVER);

        file->fs = fs;
        err = fs->ops->open(file, &(abspath[fs->mntplen]), oflag);
        if (MDS_ErrIsSame(err, MDS_EOK)) {
            file->flag = oflag;
        } else {
            file->fs = NULL;
            file->data = MDS_ARG_WITH(NULL);
        }

        if (truncate) {
            MDS_FsResult_t ret = fs->ops->truncate(file, (MDS_FsSize_t) {0});
            if (ret.len.size < 0) {
                err = ret.err;
            }
        }

        MDS_MutexRelease(&(fs->lock));
    } while (0);

    MDS_FileSystemFreePath(&abspath);

    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_LOG_E("fs(%p) file(%p) open flag(%u) err(%d)", fs, file, oflag.mask, err.errno);
    }

    return (err);
}

MDS_Err_t MDS_FileClose(MDS_FsFile_t *file)
{
    MDS_ASSERT(file != NULL);

    MDS_Err_t err;
    MDS_FileSystem_t *fs = file->fs;

    do {
        if (((file->flag.mask & MDS_FS_FLAG_OPEN) != MDS_FS_FLAG_OPEN) || (file->fs == NULL)) {
            err = MDS_EBADF;
            break;
        }

        if ((fs->ops == NULL) || (fs->ops->close == NULL)) {
            err = MDS_ENOTSUP;
            break;
        }

        MDS_MutexAcquire(&(fs->lock), MDS_TIMEOUT_FOREVER);

        err = fs->ops->close(file);
        if (MDS_ErrIsSame(err, MDS_EOK)) {
            file->fs = NULL;
            file->data = MDS_ARG_WITH(NULL);
            file->flag.mask = 0U;
        }

        MDS_MutexRelease(&(fs->lock));
    } while (0);

    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_LOG_E("fs(%p) file(%p) close err(%d)", fs, file, err.errno);
    }

    return (err);
}

MDS_FsResult_t MDS_FileRead(MDS_FsFile_t *file, void *buff, MDS_FsSize_t len)
{
    MDS_ASSERT(file != NULL);

    MDS_FsResult_t ret;
    MDS_FileSystem_t *fs = file->fs;

    do {
        if (((file->flag.mask & MDS_FS_FLAG_OPEN) != MDS_FS_FLAG_OPEN) || (file->fs == NULL)) {
            ret.err = MDS_EBADF;
            break;
        }

        if ((file->flag.mask & MDS_FS_OFLAG_RDONLY) != MDS_FS_OFLAG_RDONLY) {
            ret.err = MDS_EACCES;
            break;
        }

        if ((fs->ops == NULL) || (fs->ops->read == NULL)) {
            ret.err = MDS_ENOTSUP;
            break;
        }

        ret = fs->ops->read(file, buff, len);
    } while (0);

    if (ret.len.size < 0) {
        MDS_LOG_E("fs(%p) file(%p) read err(%d)", fs, file, ret.err.errno);
    }

    return (ret);
}

MDS_FsResult_t MDS_FileWrite(MDS_FsFile_t *file, const void *buff, MDS_FsSize_t len)
{
    MDS_ASSERT(file != NULL);

    MDS_FsResult_t ret;
    MDS_FileSystem_t *fs = file->fs;

    do {
        if (((file->flag.mask & MDS_FS_FLAG_OPEN) != MDS_FS_FLAG_OPEN) || (file->fs == NULL)) {
            ret.err = MDS_EBADF;
            break;
        }

        if ((file->flag.mask & MDS_FS_OFLAG_WRONLY) != MDS_FS_OFLAG_WRONLY) {
            ret.err = MDS_EACCES;
            break;
        }

        if ((fs->ops == NULL) || (fs->ops->write == NULL)) {
            ret.err = MDS_ENOTSUP;
            break;
        }

        ret = fs->ops->write(file, buff, len);
    } while (0);

    if (ret.len.size < 0) {
        MDS_LOG_E("fs(%p) file(%p) write  err(%d)", fs, file, ret.err.errno);
    }

    return (ret);
}

MDS_FsResult_t MDS_FileSeek(MDS_FsFile_t *file, MDS_FsSize_t pos, MDS_FileWhence_t whence)
{
    MDS_ASSERT(file != NULL);

    MDS_FsResult_t ret;
    MDS_FileSystem_t *fs = file->fs;

    do {
        if (((file->flag.mask & MDS_FS_FLAG_OPEN) != MDS_FS_FLAG_OPEN) || (file->fs == NULL)) {
            ret.err = MDS_EBADF;
            break;
        }

        if ((fs->ops == NULL) || (fs->ops->seek == NULL)) {
            ret.err = MDS_ENOTSUP;
            break;
        }

        ret = fs->ops->seek(file, pos, whence);
    } while (0);

    if (ret.len.size < 0) {
        MDS_LOG_E("fs(%p) file(%p) seek err(%d)", fs, file, ret.err.errno);
    }

    return (ret);
}

MDS_FsResult_t MDS_FileTell(MDS_FsFile_t *file)
{
    MDS_ASSERT(file != NULL);

    MDS_FsResult_t ret;
    MDS_FileSystem_t *fs = file->fs;

    do {
        if (((file->flag.mask & MDS_FS_FLAG_OPEN) != MDS_FS_FLAG_OPEN) || (file->fs == NULL)) {
            ret.err = MDS_EBADF;
            break;
        }

        if ((fs->ops != NULL) && (fs->ops->tell != NULL)) {
            ret.err = MDS_ENOTSUP;
            break;
        }

        ret = fs->ops->tell(file);
    } while (0);

    if (ret.len.size < 0) {
        MDS_LOG_E("fs(%p) file(%p) tell err(%d)", fs, file, ret.err.errno);
    }

    return (ret);
}

MDS_FsResult_t MDS_FileTruncate(MDS_FsFile_t *file, MDS_FsSize_t len)
{
    MDS_ASSERT(file != NULL);

    MDS_FsResult_t ret;
    MDS_FileSystem_t *fs = file->fs;

    do {
        if (((file->flag.mask & MDS_FS_FLAG_OPEN) != MDS_FS_FLAG_OPEN) || (file->fs == NULL)) {
            ret.err = MDS_EBADF;
            break;
        }

        if ((file->flag.mask & (MDS_FS_OFLAG_WRONLY)) != MDS_FS_OFLAG_WRONLY) {
            ret.err = MDS_EACCES;
            break;
        }

        if ((fs->ops == NULL) || (fs->ops->truncate == NULL)) {
            ret.err = MDS_ENOTSUP;
            break;
        }

        ret = fs->ops->truncate(file, len);
    } while (0);

    if (ret.len.size < 0) {
        MDS_LOG_E("fs(%p) file(%p) truncate  err(%d)", fs, file, ret.err.errno);
    }

    return (ret);
}

MDS_Err_t MDS_FileSync(MDS_FsFile_t *file)
{
    MDS_ASSERT(file != NULL);

    MDS_Err_t err;
    MDS_FileSystem_t *fs = file->fs;

    do {
        if (((file->flag.mask & MDS_FS_FLAG_OPEN) != MDS_FS_FLAG_OPEN) || (file->fs == NULL)) {
            err = MDS_EBADF;
            break;
        }

        if ((fs->ops == NULL) || (fs->ops->sync == NULL)) {
            err = MDS_ENOTSUP;
            break;
        }

        err = fs->ops->sync(file);
    } while (0);

    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_LOG_E("fs(%p) file(%p) sync err(%d)", fs, file, err.errno);
    }

    return (err);
}

MDS_Err_t MDS_FileIoctl(MDS_FsFile_t *file, MDS_FsCmd_t cmd, MDS_Arg_t arg)
{
    MDS_ASSERT(file != NULL);

    MDS_Err_t err;
    MDS_FileSystem_t *fs = file->fs;

    do {
        if (((file->flag.mask & MDS_FS_FLAG_OPEN) != MDS_FS_FLAG_OPEN) || (file->fs == NULL)) {
            err = MDS_EBADF;
            break;
        }

        if ((fs->ops == NULL) || (fs->ops->ioctl == NULL)) {
            err = MDS_ENOTSUP;
            break;
        }

        err = fs->ops->ioctl(file, cmd, arg);
    } while (0);

    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_LOG_E("fs(%p) file ioctl cmd(%d) err(%d)", fs, cmd, err.errno);
    }

    return (err);
}

#if (defined(CONFIG_MDS_FS_LOCKNODE_NUMS) && (CONFIG_MDS_FS_LOCKNODE_NUMS > 0))
MDS_Err_t MDS_FileLock(MDS_FsFile_t *file, MDS_FileLock_t lock, MDS_Timeout_t timeout)
{
    MDS_ASSERT(file != NULL);

    MDS_Err_t err;

    do {
        if (((file->flag.mask & MDS_FS_FLAG_OPEN) != MDS_FS_FLAG_OPEN) || (file->fs == NULL) ||
            (file->node == NULL)) {
            err = MDS_EBADF;
            break;
        }

        MDS_FsNode_t *node = file->node;

        switch (lock) {
            case MDS_FILE_LOCK_UN:
                err = MDS_RwLockRelease(&(node->lock));
                break;
            case MDS_FILE_LOCK_SH:
                err = MDS_RwLockAcquireRead(&(node->lock), timeout);
                break;
            case MDS_FILE_LOCK_EX:
                err = MDS_RwLockAcquireWrite(&(node->lock), timeout);
                break;
            default:
                err = MDS_EINVAL;
                break;
        }
    } while (0);

    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_LOG_E("file(%p) lock(%d) err(%d)", file, lock, err.errno);
    }

    return (err);
}
#endif

MDS_Err_t MDS_FsOpenDir(MDS_FsDir_t *dir, const char *path)
{
    MDS_ASSERT(dir != NULL);

    MDS_Err_t err;
    MDS_FileSystem_t *fs = NULL;

    if ((dir->fs != NULL) || (dir->data.ptr != NULL)) {
        return (MDS_EBUSY);
    }

    if ((path == NULL) || (path[0] != '/')) {
        return (MDS_EINVAL);
    }
    char *abspath = MDS_FileSystemJoinPath("/", path, NULL);
    if (abspath == NULL) {
        return (MDS_ENOMEM);
    }

    do {
        if (strcmp(abspath, "/") == 0) {
            dir->fs = NULL;
            dir->data.ptr = g_fsMntList.next;
            err = MDS_EOK;
            break;
        }

        MDS_FileSystemLock();
        fs = MDS_FileSystemLookupFs(abspath);
        MDS_FileSystemUnlock();

        if (fs == NULL) {
            err = MDS_ENOENT;
            break;
        }

        if ((fs->ops == NULL) || (fs->ops->opendir == NULL)) {
            err = MDS_ENOTSUP;
            break;
        }

        dir->fs = fs;
        err = fs->ops->opendir(dir, &(abspath[fs->mntplen]));
    } while (0);

    MDS_FileSystemFreePath(&abspath);

    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        dir->fs = NULL;
        dir->data = MDS_ARG_WITH(NULL);
        MDS_LOG_E("fs(%p) dir(%p) open error(%d)", fs, dir, err.errno);
    }

    return (err);
}

MDS_Err_t MDS_FsCloseDir(MDS_FsDir_t *dir)
{
    MDS_ASSERT(dir != NULL);

    MDS_Err_t err;
    MDS_FileSystem_t *fs = dir->fs;

    do {
        if (dir->fs == NULL) {
            err = MDS_EBADF;
            break;
        }

        if ((fs->ops == NULL) || (fs->ops->closedir == NULL)) {
            err = MDS_ENOTSUP;
            break;
        }

        err = fs->ops->closedir(dir);
    } while (0);

    if (MDS_ErrIsSame(err, MDS_EOK)) {
        dir->fs = NULL;
        dir->data = MDS_ARG_WITH(NULL);
    } else {
        MDS_LOG_E("fs(%p) dir(%p) close err(%d)", fs, dir, err.errno);
    }

    return (err);
}

MDS_Err_t MDS_FsReadDir(MDS_FsDir_t *dir, MDS_FsDirent_t *dirent)
{
    MDS_ASSERT(dir != NULL);
    MDS_ASSERT(dirent != NULL);

    MDS_Err_t err;
    MDS_FileSystem_t *fs = dir->fs;

    if (dir->fs != NULL) {
        if ((fs->ops == NULL) || (fs->ops->readdir == NULL)) {
            err = MDS_ENOTSUP;
        }
        while (MDS_ErrIsSame(err, MDS_EOK)) {
            err = fs->ops->readdir(dir, dirent);
            if (!MDS_ErrIsSame(err, MDS_EOK)) {
                break;
            }
            if (dirent->name[0] == 0) {
                break;
            }
            if (dirent->type != MDS_FILE_TYPE_DIR) {
                break;
            }
            if ((strcmp(dirent->name, ".") != 0) && (strcmp(dirent->name, "..") != 0)) {
                break;
            }
        }
        if (!MDS_ErrIsSame(err, MDS_EOK)) {
            MDS_LOG_E("fs(%p) dir(%p) readdir err(%d)", fs, dir, err.errno);
        }
        return (err);
    }

    if (dir->data.ptr == NULL) {
        dirent->name[0] = 0;
        return (MDS_EOK);
    }

    MDS_DListNode_t *next = NULL;

    MDS_FileSystemLock();
    for (MDS_DListNode_t *iter = g_fsMntList.next; iter != &g_fsMntList; iter = iter->next) {
        if (iter != dir->data.ptr) {
            continue;
        }

        fs = CONTAINER_OF(iter, MDS_FileSystem_t, mntNode);

        dirent->type = MDS_FILE_TYPE_DIR;
        strncpy(dirent->name, fs->mntpath + 1, sizeof(dirent->name) - 1);
        dirent->name[sizeof(dirent->name) - 1] = 0;
        dirent->size = (MDS_FsSize_t) {0};

        next = iter->next;
        break;
    }
    MDS_FileSystemUnlock();

    if (fs == NULL) {
        err = MDS_ENOENT;
    } else {
        dir->data.ptr = next;
    }

    return (err);
}

MDS_Err_t MDS_FsMkdir(const char *path)
{
    MDS_ASSERT(path != NULL);

    MDS_Err_t err;
    MDS_FileSystem_t *fs = NULL;

    if ((path == NULL) || (path[0] != '/')) {
        return (MDS_EINVAL);
    }
    char *abspath = MDS_FileSystemJoinPath("/", path, NULL);
    if (abspath == NULL) {
        return (MDS_ENOMEM);
    }

    do {

        MDS_FileSystemLock();
        fs = MDS_FileSystemLookupFs(abspath);
        MDS_FileSystemUnlock();

        if (fs == NULL) {
            err = MDS_ENOENT;
            break;
        }

        if ((fs->ops == NULL) || (fs->ops->mkdir == NULL)) {
            err = MDS_ENOTSUP;
            break;
        }

        err = fs->ops->mkdir(fs, &(abspath[fs->mntplen]));
    } while (0);

    MDS_FileSystemFreePath(&abspath);

    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_LOG_E("fs(%p) mkdir err(%d)", fs, err.errno);
    }

    return (err);
}

MDS_Err_t MDS_FsUnlink(const char *path)
{
    MDS_ASSERT(path != NULL);

    MDS_Err_t err;
    MDS_FileSystem_t *fs = NULL;

    if ((path == NULL) || (path[0] != '/')) {
        return (MDS_EINVAL);
    }
    char *abspath = MDS_FileSystemJoinPath("/", path, NULL);
    if (abspath == NULL) {
        return (MDS_ENOMEM);
    }

    do {
        MDS_FileSystemLock();
        fs = MDS_FileSystemLookupFs(abspath);
        MDS_FileSystemUnlock();

        if (fs == NULL) {
            err = MDS_ENOENT;
            break;
        }

        if ((fs->ops == NULL) || (fs->ops->unlink == NULL)) {
            err = MDS_ENOTSUP;
            break;
        }

        err = fs->ops->unlink(fs, &abspath[fs->mntplen]);
    } while (0);

    MDS_FileSystemFreePath(&abspath);

    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_LOG_E("fs(%p) unlink err(%d)", fs, err.errno);
    }

    return (err);
}

MDS_Err_t MDS_FsRename(const char *oldpath, const char *newpath)
{
    MDS_ASSERT(oldpath != NULL);
    MDS_ASSERT(newpath != NULL);

    MDS_Err_t err;
    MDS_FileSystem_t *fs = NULL;

    if ((oldpath == NULL) || (oldpath[0] != '/') || (newpath == NULL) || (newpath[0] != '/')) {
        return (MDS_EINVAL);
    }

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

        MDS_FileSystemLock();
        MDS_FileSystem_t *oldfs = MDS_FileSystemLookupFs(oldabspath);
        MDS_FileSystem_t *newfs = MDS_FileSystemLookupFs(newabspath);
        MDS_FileSystemUnlock();
        if ((oldfs != newfs) || (oldfs == NULL)) {
            err = MDS_EXDEV;
            break;
        }

        // check fs flag with write
        fs = oldfs;
        if ((fs->ops == NULL) || (fs->ops->rename == NULL)) {
            err = MDS_ENOTSUP;
            break;
        }

        MDS_MutexAcquire(&(fs->lock), MDS_TIMEOUT_FOREVER);

        const char *oldfspath = &(oldabspath[oldfs->mntplen]);
        const char *newfspath = &(newabspath[newfs->mntplen]);
        err = fs->ops->rename(fs, oldfspath, newfspath);

        MDS_MutexRelease(&(fs->lock));
    } while (0);

    MDS_FileSystemFreePath(&oldabspath);
    MDS_FileSystemFreePath(&newabspath);

    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_LOG_E("fs(%p) rename err(%d)", fs, err.errno);
    }

    return (err);
}

MDS_Err_t MDS_FsSymLink(const char *srcpath, const char *dstpath)
{
    MDS_ASSERT(srcpath != NULL);
    MDS_ASSERT(dstpath != NULL);

    MDS_Err_t err = MDS_EIO;

    // TODO:

    return (err);
}

MDS_Err_t MDS_FsStatfs(const char *path, MDS_FsStat_t *statfs)
{
    MDS_ASSERT(path != NULL);

    MDS_Err_t err;
    MDS_FileSystem_t *fs = NULL;

    if ((path == NULL) || (path[0] != '/')) {
        return (MDS_EINVAL);
    }
    char *abspath = MDS_FileSystemJoinPath("/", path, NULL);
    if (abspath == NULL) {
        return (MDS_ENOMEM);
    }

    do {
        MDS_FileSystemLock();
        fs = MDS_FileSystemLookupFs(abspath);
        MDS_FileSystemUnlock();

        if (fs == NULL) {
            err = MDS_ENOENT;
            break;
        }

        if ((fs->ops == NULL) || (fs->ops->statfs == NULL)) {
            err = MDS_ENOTSUP;
            break;
        }

        err = fs->ops->statfs(fs, &(abspath[fs->mntplen]), statfs);
    } while (0);

    MDS_FileSystemFreePath(&abspath);

    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_LOG_E("fs(%p) statfs err(%d)", fs, err.errno);
    }

    return (err);
}

MDS_Err_t MDS_FsStatFile(const char *path, MDS_FileStat_t *stat)
{
    MDS_ASSERT(path != NULL);

    MDS_Err_t err;
    MDS_FileSystem_t *fs = NULL;

    if ((path == NULL) || (path[0] != '/')) {
        return (MDS_EINVAL);
    }
    char *abspath = MDS_FileSystemJoinPath("/", path, NULL);
    if (abspath == NULL) {
        return (MDS_ENOMEM);
    }

    do {
        MDS_FileSystemLock();
        fs = MDS_FileSystemLookupFs(abspath);
        MDS_FileSystemUnlock();

        if (fs == NULL) {
            err = MDS_ENOENT;
            break;
        }

        if ((fs->ops == NULL) || (fs->ops->statfs == NULL)) {
            err = MDS_ENOTSUP;
            break;
        }

        err = fs->ops->stat(fs, &(abspath[fs->mntplen]), stat);
    } while (0);

    MDS_FileSystemFreePath(&abspath);

    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_LOG_E("fs(%p) stat err(%d)", fs, err.errno);
    }

    return (err);
}
