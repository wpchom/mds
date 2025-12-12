/**
 * Copyright (c) [2022] [pchom]
 * [MDS] is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 *Mulan PSL v2 for more details.
 **/
#ifndef __MDS_ERR_H__
#define __MDS_ERR_H__

/* Include ----------------------------------------------------------------- */
#include <stdint.h>
#include <stdbool.h>
#include "mds_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define ------------------------------------------------------------------ */
#define MDS_ERR_MODULE_UNKOWN 0x0000
#define MDS_ERR_MODULE_OBJECT 0x0001
#define MDS_ERR_MODULE_DEVICE 0x0002
#define MDS_ERR_MODULE_SCHEDU 0x0003
#define MDS_ERR_MODULE_MEMHEAP
#define MDS_ERR_MODULE_THREAD
#define MDS_ERR_MODULE_WORKQ
#define MDS_ERR_MODULE_TIMER
#define MDS_ERR_MODULE_SEMAPHORE
#define MDS_ERR_MODULE_MUTEX
#define MDS_ERR_MODULE_EVENT
#define MDS_ERR_MODULE_MSGQUEUE
#define MDS_ERR_MODULE_MEMPOOL

/* Config ------------------------------------------------------------------ */
#ifndef MDS_ERR_MODULE
#define MDS_ERR_MODULE MDS_ERR_MODULE_UNKOWN
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct MDS_Err {
    int32_t errno : 16;
    uint32_t module : 16;
} MDS_Err_t;

static inline bool MDS_ErrIsSame(MDS_Err_t e1, MDS_Err_t e2)
{
    return (e1.errno == e2.errno);
}

#define MDS_EOK    ((MDS_Err_t) {0})
#define MDS_ERR(e) ((MDS_Err_t) {.module = MDS_ERR_MODULE, .errno = e})

#define MDS_EPERM   MDS_ERR(1)  /* Not owner */
#define MDS_ENOENT  MDS_ERR(2)  /* No such file or directory */
#define MDS_ESRCH   MDS_ERR(3)  /* No such process */
#define MDS_EINTR   MDS_ERR(4)  /* Interrupted system call */
#define MDS_EIO     MDS_ERR(5)  /* I/O error */
#define MDS_ENXIO   MDS_ERR(6)  /* No such device or address */
#define MDS_E2BIG   MDS_ERR(7)  /* Arg list too long */
#define MDS_ENOEXEC MDS_ERR(8)  /* Exec format error */
#define MDS_EBADF   MDS_ERR(9)  /* Bad file number */
#define MDS_ECHILD  MDS_ERR(10) /* No children */
#define MDS_EAGAIN  MDS_ERR(11) /* No more processes */
#define MDS_ENOMEM  MDS_ERR(12) /* Not enough space */
#define MDS_EACCES  MDS_ERR(13) /* Permission denied */
#define MDS_EFAULT  MDS_ERR(14) /* Bad address */
#define MDS_EBUSY   MDS_ERR(16) /* Device or resource busy */
#define MDS_EEXIST  MDS_ERR(17) /* File exists */
#define MDS_EXDEV   MDS_ERR(18) /* Cross-device link */
#define MDS_ENODEV  MDS_ERR(19) /* No such device */
#define MDS_ENOTDIR MDS_ERR(20) /* Not a directory */
#define MDS_EISDIR  MDS_ERR(21) /* Is a directory */
#define MDS_EINVAL  MDS_ERR(22) /* Invalid argument */
#define MDS_ENFILE  MDS_ERR(23) /* Too many open files in system */
#define MDS_EMFILE  MDS_ERR(24) /* File descriptor value too large */
#define MDS_ENOTTY  MDS_ERR(25) /* Not a character device */
#define MDS_ETXTBSY MDS_ERR(26) /* Text file busy */
#define MDS_EFBIG   MDS_ERR(27) /* File too large */
#define MDS_ENOSPC  MDS_ERR(28) /* No space left on device */
#define MDS_ESPIPE  MDS_ERR(29) /* Illegal seek */
#define MDS_EROFS   MDS_ERR(30) /* Read-only file system */
#define MDS_EMLINK  MDS_ERR(31) /* Too many links */
#define MDS_EPIPE   MDS_ERR(32) /* Broken pipe */
#define MDS_EDOM    MDS_ERR(33) /* Mathematics argument out of domain of function */
#define MDS_ERANGE  MDS_ERR(34) /* Result too large */
#define MDS_ENOMSG  MDS_ERR(35) /* No message of desired type */
#define MDS_EIDRM   MDS_ERR(36) /* Identifier removed */

#ifdef __cplusplus
}
#endif

#endif /* __MDS_ERR_H__ */
