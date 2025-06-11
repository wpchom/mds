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
#include "../kernel.h"

/* Define ------------------------------------------------------------------ */
MDS_LOG_MODULE_DECLARE(kernel, CONFIG_MDS_KERNEL_LOG_LEVEL);

/* Function ---------------------------------------------------------------- */
typedef struct MDS_MsgQueueHeader {
    struct MDS_MsgQueueHeader *next;
} MDS_MsgQueueHeader_t;

static void MDS_MsgQueueListInit(MDS_MsgQueue_t *msgQueue, size_t msgNums)
{
    msgQueue->lfree = NULL;
    msgQueue->lhead = NULL;
    msgQueue->ltail = NULL;

    for (size_t idx = 0; idx < msgNums; idx++) {
        MDS_MsgQueueHeader_t *list = (MDS_MsgQueueHeader_t *)(&(
            ((uint8_t *)(msgQueue->queBuff))[idx *
                                             (sizeof(MDS_MsgQueueHeader_t) + msgQueue->msgSize)]));
        list->next = (MDS_MsgQueueHeader_t *)(msgQueue->lfree);
        msgQueue->lfree = list;
    }
}

MDS_Err_t MDS_MsgQueueInit(MDS_MsgQueue_t *msgQueue, const char *name, void *queBuff,
                           size_t bufSize, size_t msgSize)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(bufSize > (sizeof(MDS_MsgQueueHeader_t) + msgSize));

    MDS_Err_t err = MDS_ObjectInit(&(msgQueue->object), MDS_OBJECT_TYPE_MSGQUEUE, name);
    if (err == MDS_EOK) {
        msgQueue->queBuff = queBuff;
        msgQueue->msgSize = VALUE_ALIGN(msgSize + MDS_SYSMEM_ALIGN_SIZE - 1,
                                        MDS_SYSMEM_ALIGN_SIZE);
        MDS_MsgQueueListInit(msgQueue,
                             bufSize / (msgQueue->msgSize + sizeof(MDS_MsgQueueHeader_t)));
        MDS_KernelWaitQueueInit(&(msgQueue->queueRecv));
        MDS_KernelWaitQueueInit(&(msgQueue->queueSend));
        MDS_SpinLockInit(&(msgQueue->spinlock));
    }

    return (err);
}

MDS_Err_t MDS_MsgQueueDeInit(MDS_MsgQueue_t *msgQueue)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(msgQueue->spinlock));
    if (err == MDS_EOK) {
        MDS_KernelWaitQueueDrain(&(msgQueue->queueRecv));
        MDS_KernelWaitQueueDrain(&(msgQueue->queueSend));

        err = MDS_ObjectDeInit(&(msgQueue->object));
    }
    if (err != MDS_EOK) {
        MDS_SpinLockRelease(&(msgQueue->spinlock));
    }
    MDS_CoreInterruptRestore(lock);

    return (err);
}

MDS_MsgQueue_t *MDS_MsgQueueCreate(const char *name, size_t msgSize, size_t msgNums)
{
    MDS_MsgQueue_t *msgQueue = (MDS_MsgQueue_t *)MDS_ObjectCreate(sizeof(MDS_MsgQueue_t),
                                                                  MDS_OBJECT_TYPE_MSGQUEUE, name);

    if (msgQueue != NULL) {
        msgQueue->msgSize = VALUE_ALIGN(msgSize + MDS_SYSMEM_ALIGN_SIZE - 1,
                                        MDS_SYSMEM_ALIGN_SIZE);
        msgQueue->queBuff = MDS_SysMemAlloc((msgQueue->msgSize + sizeof(MDS_MsgQueueHeader_t)) *
                                            msgNums);
        if (msgQueue->queBuff == NULL) {
            MDS_ObjectDestroy(&(msgQueue->object));
            return (NULL);
        }
        MDS_MsgQueueListInit(msgQueue, msgNums);
        MDS_KernelWaitQueueInit(&(msgQueue->queueRecv));
        MDS_KernelWaitQueueInit(&(msgQueue->queueSend));
        MDS_SpinLockInit(&(msgQueue->spinlock));
    }

    return (msgQueue);
}

MDS_Err_t MDS_MsgQueueDestroy(MDS_MsgQueue_t *msgQueue)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(msgQueue->spinlock));
    if (err == MDS_EOK) {
        void *queBuff = msgQueue->queBuff;
        MDS_KernelWaitQueueDrain(&(msgQueue->queueRecv));
        MDS_KernelWaitQueueDrain(&(msgQueue->queueSend));

        err = MDS_ObjectDestroy(&(msgQueue->object));
        if (err == MDS_EOK) {
            MDS_SysMemFree(queBuff);
        }
    }
    if (err != MDS_EOK) {
        MDS_SpinLockRelease(&(msgQueue->spinlock));
    }
    MDS_CoreInterruptRestore(lock);

    return (err);
}

MDS_Err_t MDS_MsgQueueRecvAcquire(MDS_MsgQueue_t *msgQueue, void *recv, MDS_Timeout_t timeout)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);

    MDS_HOOK_CALL(KERNEL, msgqueue,
                  (msgQueue, MDS_KERNEL_TRACE_MSGQUEUE_TRY_RECV, MDS_EOK, timeout));

    MDS_MsgQueueHeader_t *msg = NULL;

    MDS_Thread_t *thread = MDS_KernelCurrentThread();

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(msgQueue->spinlock));
    if (err == MDS_EOK) {
        if (msgQueue->lhead == NULL) {
            if (timeout.ticks == MDS_CLOCK_TICK_NO_WAIT) {
                err = MDS_ETIMEOUT;
            } else if (thread == NULL) {
                MDS_LOG_W("[msgqueue] thread is null try to recv msgqueue");
                err = MDS_EACCES;
            } else {
                MDS_LOG_D(
                    "[msgqueue] msgqueue(%p) recv suspend thread(%p) entry:%p timer wait:%lu",
                    msgQueue, thread, thread->entry, (unsigned long)timeout.ticks);
            }
        }

        while ((err == MDS_EOK) && (msgQueue->lhead == NULL)) {
            err = MDS_KernelWaitQueueUntil(&(msgQueue->queueRecv), thread, timeout, true, &lock,
                                           &(msgQueue->spinlock));
        }

        if (err == MDS_EOK) {
            msg = (MDS_MsgQueueHeader_t *)(msgQueue->lhead);
            msgQueue->lhead = (void *)(msg->next);
            if (msgQueue->lhead == NULL) {
                msgQueue->ltail = NULL;
            }
        }
    }
    MDS_SpinLockRelease(&(msgQueue->spinlock));
    MDS_CoreInterruptRestore(lock);

    MDS_HOOK_CALL(KERNEL, msgqueue, (msgQueue, MDS_KERNEL_TRACE_MSGQUEUE_HAS_RECV, err, timeout));

    if (err == MDS_EOK) {
        if (recv != NULL) {
            *((uintptr_t *)recv) = (uintptr_t)(msg + 1);
        }

        MDS_LOG_D("[msgqueue] thread[(%p) recv message from msgqueue(%p)", thread, msgQueue);
    }

    return (err);
}

MDS_Err_t MDS_MsgQueueRecvRelease(MDS_MsgQueue_t *msgQueue, void *recv)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);

    if (recv == NULL) {
        return (MDS_EINVAL);
    }

    MDS_MsgQueueHeader_t *msg = ((MDS_MsgQueueHeader_t *)(*(uintptr_t *)recv)) - 1;
    if ((((uintptr_t)(msg) - (uintptr_t)(msgQueue->queBuff)) %
         (sizeof(MDS_MsgQueueHeader_t) + msgQueue->msgSize)) != 0) {
        return (MDS_EINVAL);
    }

    MDS_Thread_t *thread = NULL;

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(msgQueue->spinlock));
    if (err == MDS_EOK) {
        msg->next = (MDS_MsgQueueHeader_t *)(msgQueue->lfree);
        msgQueue->lfree = (void *)msg;

        thread = MDS_KernelWaitQueueResume(&(msgQueue->queueSend));
    }
    MDS_SpinLockRelease(&(msgQueue->spinlock));
    MDS_CoreInterruptRestore(lock);

    if (thread != NULL) {
        MDS_KernelSchedulerCheck();
    }

    return (err);
}

MDS_Err_t MDS_MsgQueueRecvPeek(MDS_MsgQueue_t *msgQueue, void *buff, size_t size,
                               MDS_Timeout_t timeout)
{
    MDS_ASSERT(buff != NULL);
    MDS_ASSERT(size > 0);

    void *recv = NULL;

    MDS_Err_t err = MDS_MsgQueueRecvAcquire(msgQueue, &recv, timeout);
    if (err == MDS_EOK) {
        size_t rlen = (msgQueue->msgSize <= size) ? (msgQueue->msgSize) : (size);
        MDS_MemBuffCopy(buff, size, recv, rlen);
    }

    return (err);
}

MDS_Err_t MDS_MsgQueueRecvCopy(MDS_MsgQueue_t *msgQueue, void *buff, size_t size,
                               MDS_Timeout_t timeout)
{
    MDS_ASSERT(buff != NULL);
    MDS_ASSERT(size > 0);

    void *recv = NULL;

    MDS_Err_t err = MDS_MsgQueueRecvAcquire(msgQueue, &recv, timeout);
    if (err == MDS_EOK) {
        size_t rlen = (msgQueue->msgSize <= size) ? (msgQueue->msgSize) : (size);
        MDS_MemBuffCopy(buff, size, recv, rlen);
        err = MDS_MsgQueueRecvRelease(msgQueue, &recv);
    }

    return (err);
}

MDS_Err_t MDS_MsgQueueSendMsg(MDS_MsgQueue_t *msgQueue, const MDS_MsgList_t *msgList,
                              MDS_Timeout_t timeout)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);
    MDS_ASSERT(msgList != NULL);

    size_t len = MDS_MsgListGetLength(msgList);
    if ((len == 0) || (len > msgQueue->msgSize)) {
        return (MDS_EINVAL);
    }

    MDS_HOOK_CALL(KERNEL, msgqueue,
                  (msgQueue, MDS_KERNEL_TRACE_MSGQUEUE_TRY_SEND, MDS_EOK, timeout));

    MDS_MsgQueueHeader_t *msg = NULL;

    MDS_Thread_t *thread = MDS_KernelCurrentThread();

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(msgQueue->spinlock));
    if (err == MDS_EOK) {
        if (msgQueue->lfree == NULL) {
            if (timeout.ticks == MDS_CLOCK_TICK_NO_WAIT) {
                err = MDS_ERANGE;
            } else if (thread == NULL) {
                MDS_LOG_W("[msgqueue] thread is null try to send msgqueue");
                err = MDS_EACCES;
            } else {
                MDS_LOG_D(
                    "[msgqueue] msgqueue(%p) send suspend thread(%p) entry:%p timer wait:%lu",
                    msgQueue, thread, thread->entry, (unsigned long)timeout.ticks);
            }
        }

        while ((err == MDS_EOK) && (msgQueue->lfree == NULL)) {
            err = MDS_KernelWaitQueueUntil(&(msgQueue->queueSend), thread, timeout, true, &lock,
                                           &(msgQueue->spinlock));
        }

        if (err == MDS_EOK) {
            msg = (MDS_MsgQueueHeader_t *)(msgQueue->lfree);
            msgQueue->lfree = (void *)(msg->next);
            msg->next = NULL;
        }
    }
    MDS_SpinLockRelease(&(msgQueue->spinlock));
    MDS_CoreInterruptRestore(lock);

    MDS_HOOK_CALL(KERNEL, msgqueue, (msgQueue, MDS_KERNEL_TRACE_MSGQUEUE_HAS_SEND, err, timeout));

    if (err != MDS_EOK) {
        return ((err == MDS_ETIMEOUT) ? (MDS_ERANGE) : (err));
    }

    MDS_MsgListCopyBuff(msg + 1, msgQueue->msgSize, msgList);

    MDS_LOG_D("[msgqueue] send message to msgqueue(%p) len:%zu", msgQueue, len);

    lock = MDS_CoreInterruptLock();
    err = MDS_SpinLockAcquire(&(msgQueue->spinlock));
    if (err == MDS_EOK) {
        if (msgQueue->ltail != NULL) {
            ((MDS_MsgQueueHeader_t *)(msgQueue->ltail))->next = msg;
        }
        msgQueue->ltail = msg;
        if (msgQueue->lhead == NULL) {
            msgQueue->lhead = msg;
        }
        thread = MDS_KernelWaitQueueResume(&(msgQueue->queueRecv));
    } else {
        thread = NULL;
    }
    MDS_SpinLockRelease(&(msgQueue->spinlock));
    MDS_CoreInterruptRestore(lock);

    if (thread != NULL) {
        MDS_KernelSchedulerCheck();
    }

    return (err);
}

MDS_Err_t MDS_MsgQueueSend(MDS_MsgQueue_t *msgQueue, const void *buff, size_t len,
                           MDS_Timeout_t timeout)
{
    MDS_ASSERT(buff != NULL);

    const MDS_MsgList_t msgList = {.buff = buff, .len = len, .next = NULL};

    return (MDS_MsgQueueSendMsg(msgQueue, &msgList, timeout));
}

MDS_Err_t MDS_MsgQueueUrgentMsg(MDS_MsgQueue_t *msgQueue, const MDS_MsgList_t *msgList)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);
    MDS_ASSERT(msgList != NULL);

    size_t len = MDS_MsgListGetLength(msgList);
    if ((len == 0) || (len > msgQueue->msgSize)) {
        return (MDS_EINVAL);
    }

    MDS_HOOK_CALL(KERNEL, msgqueue,
                  (msgQueue, MDS_KERNEL_TRACE_MSGQUEUE_TRY_SEND, MDS_EOK, MDS_TIMEOUT_NO_WAIT));

    MDS_MsgQueueHeader_t *msg = NULL;

    MDS_Thread_t *thread = NULL;

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(msgQueue->spinlock));
    if (err == MDS_EOK) {
        msg = (MDS_MsgQueueHeader_t *)(msgQueue->lfree);
        if (msg != NULL) {
            msgQueue->lfree = (void *)(msg->next);
        }
    }
    MDS_SpinLockRelease(&(msgQueue->spinlock));
    MDS_CoreInterruptRestore(lock);

    MDS_HOOK_CALL(KERNEL, msgqueue,
                  (msgQueue, MDS_KERNEL_TRACE_MSGQUEUE_HAS_SEND, err, MDS_TIMEOUT_NO_WAIT));

    if (msg == NULL) {
        return (MDS_ERANGE);
    }

    MDS_MsgListCopyBuff(msg + 1, msgQueue->msgSize, msgList);

    MDS_LOG_D("[msgqueue] send urgent message to msgqueue(%p) len:%zu", msgQueue, len);

    lock = MDS_CoreInterruptLock();
    err = MDS_SpinLockAcquire(&(msgQueue->spinlock));
    if (err == MDS_EOK) {
        msg->next = (MDS_MsgQueueHeader_t *)(msgQueue->lhead);
        msgQueue->lhead = msg;
        if (msgQueue->ltail == NULL) {
            msgQueue->ltail = msg;
        }

        thread = MDS_KernelWaitQueueResume(&(msgQueue->queueRecv));
    }
    MDS_SpinLockRelease(&(msgQueue->spinlock));
    MDS_CoreInterruptRestore(lock);

    if (thread != NULL) {
        MDS_KernelSchedulerCheck();
    }

    return (err);
}

MDS_Err_t MDS_MsgQueueUrgent(MDS_MsgQueue_t *msgQueue, const void *buff, size_t len)
{
    MDS_ASSERT(buff == NULL);

    const MDS_MsgList_t msgList = {.buff = buff, .len = len, .next = NULL};

    return (MDS_MsgQueueUrgentMsg(msgQueue, &msgList));
}

size_t MDS_MsgQueueGetMsgSize(MDS_MsgQueue_t *msgQueue)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);

    return (msgQueue->msgSize);
}

size_t MDS_MsgQueueGetMsgCount(MDS_MsgQueue_t *msgQueue)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);

    size_t cnt = 0;

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(msgQueue->spinlock));
    if (err == MDS_EOK) {
        for (MDS_MsgQueueHeader_t *msg = (MDS_MsgQueueHeader_t *)(msgQueue->lhead); msg != NULL;
             msg = msg->next) {
            cnt += 1;
        }
    }
    MDS_SpinLockRelease(&(msgQueue->spinlock));
    MDS_CoreInterruptRestore(lock);

    return (cnt);
}

size_t MDS_MsgQueueGetMsgFree(MDS_MsgQueue_t *msgQueue)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);

    size_t cnt = 0;

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(msgQueue->spinlock));
    if (err == MDS_EOK) {
        for (MDS_MsgQueueHeader_t *msg = (MDS_MsgQueueHeader_t *)(msgQueue->lfree); msg != NULL;
             msg = msg->next) {
            cnt += 1;
        }
    }
    MDS_SpinLockRelease(&(msgQueue->spinlock));
    MDS_CoreInterruptRestore(lock);

    return (cnt);
}
