/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*            (c) 1995 - 2024 SEGGER Microcontroller GmbH             *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       SEGGER SystemView * Real-time application analysis           *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* SEGGER strongly recommends to not make any changes                 *
* to or modify the source code of this software in order to stay     *
* compatible with the SystemView and RTT protocol, and J-Link.       *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
*                                                                    *
*       SystemView version: 3.54                                    *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : mds_systemview.c
Purpose : Interface between embOS and System View.
Revision: $Rev: 25329 $
*/

#include "mds_sys.h"
#include "SEGGER_SYSVIEW.h"
#include "SEGGER_RTT.h"

/*********************************************************************
 *
 *       Local functions
 *
 **********************************************************************
 */
/*
+       0..  31 : Standard packets, known by SystemView.
+      32..1023 : OS-definable packets, described in a SystemView description file.
+    1024..2047 : User-definable packets, described in a SystemView description file.
+    2048..32767: Undefined.
*/
#define SYSVIEW_EVTID_SEMAPHORE_TRY_ACQUIRE 41U
#define SYSVIEW_EVTID_SEMAPHORE_HAS_ACQUIRE 42U
#define SYSVIEW_EVTID_SEMAPHORE_HAS_RELEASE 43U

#define SYSVIEW_EVTID_MUTEX_TRY_ACQUIRE 51U
#define SYSVIEW_EVTID_MUTEX_HAS_ACQUIRE 52U
#define SYSVIEW_EVTID_MUTEX_HAS_RELEASE 53U

#define SYSVIEW_EVTID_EVENT_TRY_ACQUIRE 61U
#define SYSVIEW_EVTID_EVENT_HAS_ACQUIRE 62U
#define SYSVIEW_EVTID_EVENT_HAS_SET     63U
#define SYSVIEW_EVTID_EVENT_HAS_CLR     64U

#define SYSVIEW_EVTID_MSGQUEUE_TRY_RECV 71U
#define SYSVIEW_EVTID_MSGQUEUE_HAS_RECV 72U
#define SYSVIEW_EVTID_MSGQUEUE_TRY_SEND 73U
#define SYSVIEW_EVTID_MSGQUEUE_HAS_SEND 74U

#define SYSVIEW_EVTID_MEMPOOL_TRY_ALLOC 81U
#define SYSVIEW_EVTID_MEMPOOL_HAS_ALLOC 82U
#define SYSVIEW_EVTID_MEMPOOL_HAS_FREE  83U

/*********************************************************************
 *
 *       _cbGetTime()
 *
 *  Function description
 *    This function is part of the link between MDS and SYSVIEW.
 *    Called from SystemView when asked by the host, returns the
 *    current system time in micro seconds.
 */
static U64 _cbGetTime(void)
{
    return (U64)(MDS_SysTickGetCount() * (MDS_TIME_USEC_OF_SEC / MDS_SYSTICK_FREQ_HZ));
}

/*********************************************************************
 *
 *       _cbSendTaskInfo()
 *
 *  Function description
 *    Sends task information to SystemView
 */
static void _cbSendTaskInfo(const MDS_Thread_t *thread)
{
    SEGGER_SYSVIEW_TASKINFO Info;

    MDS_MemBuffSet(&Info, 0, sizeof(Info));

    Info.TaskID = (U32)thread;
    Info.sName = thread->object.name;
    Info.Prio = thread->currPrio;
    Info.StackBase = (U32)thread->stackBase;
    Info.StackSize = (U32)thread->stackSize;

    SEGGER_SYSVIEW_SendTaskInfo(&Info);
}

/*********************************************************************
 *
 *       _cbSendTaskList()
 *
 *  Function description
 *    This function is part of the link between MDS and SYSVIEW.
 *    Called from SystemView when asked by the host, it uses SYSVIEW
 *    functions to send the entire task list to the host.
 */
static void _cbSendTaskList(void)
{
    const MDS_ListNode_t *list = MDS_ObjectGetList(MDS_OBJECT_TYPE_THREAD);
    MDS_Thread_t *iter = NULL;

    MDS_LIST_FOREACH_NEXT (iter, object.node, list) {
        _cbSendTaskInfo(iter);
    }
}

static void hookTimerEnter(MDS_Timer_t *timer)
{
    SEGGER_SYSVIEW_RecordEnterTimer((U32)timer);
}

static void hookTimerExit(MDS_Timer_t *timer)
{
    UNUSED(timer);

    SEGGER_SYSVIEW_RecordExitTimer();
}

static void hookThreadInit(MDS_Thread_t *thread)
{
    SEGGER_SYSVIEW_OnTaskCreate((U32)thread);
}

static void hookThreadExit(MDS_Thread_t *thread)
{
    SEGGER_SYSVIEW_OnTaskTerminate((U32)thread);
}

static void hookThreadResume(MDS_Thread_t *thread)
{
    SEGGER_SYSVIEW_OnTaskStartReady((U32)thread);
}

static void hookThreadSuspend(MDS_Thread_t *thread)
{
    SEGGER_SYSVIEW_OnTaskStopReady((U32)thread, 0);
}

static void hookSchedulerSwitch(MDS_Thread_t *toThread, MDS_Thread_t *fromThread)
{
    SEGGER_SYSVIEW_OnTaskStopReady((U32)fromThread, 0);
    if (MDS_CoreInterruptCurrent() != 0) {
        SEGGER_SYSVIEW_OnTaskStartReady((U32)toThread);
        SEGGER_SYSVIEW_RecordEnterISR();
    } else if (toThread == MDS_KernelGetIdleThread()) {
        SEGGER_SYSVIEW_OnIdle();
    } else {
        SEGGER_SYSVIEW_OnTaskStartExec((U32)toThread);
    }
}

static void hookInterruptEnter(MDS_Item_t irq)
{
    UNUSED(irq);
    SEGGER_SYSVIEW_RecordEnterISR();
}

static void hookInterruptExit(MDS_Item_t irq)
{
    UNUSED(irq);
    if (MDS_CoreInterruptCurrent() != 0) {
        SEGGER_SYSVIEW_RecordExitISR();
        return;
    }

    SEGGER_SYSVIEW_RecordExitISRToScheduler();
    MDS_Thread_t *thread = MDS_KernelCurrentThread();
    if (thread == MDS_KernelGetIdleThread()) {
        SEGGER_SYSVIEW_OnIdle();
    } else {
        SEGGER_SYSVIEW_OnTaskStartExec((uintptr_t)thread);
    }
}

#if (defined(MDS_SYSTEMVIEW_TRACE_IPC) && (MDS_SYSTEMVIEW_TRACE_IPC > 0))
void SYSVIEW_RecordU32x2(unsigned Id, U32 Para0, U32 Para1)
{
    U8 aPacket[SEGGER_SYSVIEW_INFO_SIZE + SEGGER_SYSVIEW_QUANTA_U32 * sizeof(U16)];
    U8 *pPayload;
    //
    pPayload = SEGGER_SYSVIEW_PREPARE_PACKET(aPacket);     // Prepare the packet for SystemView
    pPayload = SEGGER_SYSVIEW_EncodeU32(pPayload, Para0);  // Add the first parameter to the packet
    pPayload = SEGGER_SYSVIEW_EncodeU32(pPayload, Para1);  // Add the second parameter to the packet
    //
    SEGGER_SYSVIEW_SendPacket(&aPacket[0], pPayload, Id);  // Send the packet
}

static void hookSemaphoreTryAcquire(MDS_Semaphore_t *semaphore, MDS_Tick_t timeout)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_SEMAPHORE_TRY_ACQUIRE, (uintptr_t)semaphore, (uint32_t)timeout);
}

static void hookSemaphoreHasAcquire(MDS_Semaphore_t *semaphore, MDS_Err_t err)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_SEMAPHORE_HAS_ACQUIRE, (uintptr_t)semaphore, (uint32_t)err);
}

static void hookSemaphoreHasRelease(MDS_Semaphore_t *semaphore)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_SEMAPHORE_HAS_RELEASE, (uintptr_t)semaphore, 0);
}

static void hookMutexTryAcquire(MDS_Mutex_t *mutex, MDS_Tick_t timeout)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_MUTEX_TRY_ACQUIRE, (uintptr_t)mutex, (uint32_t)timeout);
}

static void hookMutexHasAcquire(MDS_Mutex_t *mutex, MDS_Err_t err)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_MUTEX_HAS_ACQUIRE, (uintptr_t)mutex, (uint32_t)err);
}

static void hookMutexHasRelease(MDS_Mutex_t *mutex)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_MUTEX_HAS_RELEASE, (uintptr_t)mutex, 0);
}

static void hookEventTryAcquire(MDS_Event_t *event, MDS_Tick_t timeout)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_EVENT_TRY_ACQUIRE, (uintptr_t)event, (uint32_t)timeout);
}

static void hookEventHasAcquire(MDS_Event_t *event, MDS_Err_t err)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_EVENT_HAS_ACQUIRE, (uintptr_t)event, (uint32_t)err);
}

static void hookEventHasSet(MDS_Event_t *event, MDS_Mask_t mask)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_EVENT_HAS_SET, (uintptr_t)event, (uint32_t)mask);
}

static void hookEventHasClr(MDS_Event_t *event, MDS_Mask_t mask)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_EVENT_HAS_CLR, (uintptr_t)event, (uint32_t)mask);
}

static void hookMsgQueueTryRecv(MDS_MsgQueue_t *msgQueue, MDS_Tick_t timeout)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_MSGQUEUE_TRY_RECV, (uintptr_t)msgQueue, (uint32_t)timeout);
}

static void hookMsgQueueHasRecv(MDS_MsgQueue_t *msgQueue, MDS_Err_t err)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_MSGQUEUE_HAS_RECV, (uintptr_t)msgQueue, (uint32_t)err);
}

static void hookMsgQueueTrySend(MDS_MsgQueue_t *msgQueue, MDS_Tick_t timeout)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_MSGQUEUE_TRY_SEND, (uintptr_t)msgQueue, (uint32_t)timeout);
}

static void hookMsgQueueHasSend(MDS_MsgQueue_t *msgQueue, MDS_Err_t err)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_MSGQUEUE_HAS_SEND, (uintptr_t)msgQueue, (uint32_t)err);
}

static void hookMemPoolTryAlloc(MDS_MemPool_t *memPool, MDS_Tick_t timeout)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_MEMPOOL_TRY_ALLOC, (uintptr_t)memPool, (uint32_t)timeout);
}

static void hookMemPoolHasAlloc(MDS_MemPool_t *memPool, void *ptr)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_MEMPOOL_HAS_ALLOC, (uintptr_t)memPool, (uintptr_t)ptr);
}

static void hookMemPoolHasFree(MDS_MemPool_t *memPool, void *ptr)
{
    SYSVIEW_RecordU32x2(SYSVIEW_EVTID_MEMPOOL_HAS_FREE, (uintptr_t)memPool, (uintptr_t)ptr);
}
#endif

#if (defined(MDS_SYSTEMVIEW_TRACE_MEM) && (MDS_SYSTEMVIEW_TRACE_MEM > 0))
static void hookMemheapInit(MDS_MemHeap_t *memheap, void *heapBegin, void *heapLimit, size_t metaSize)
{
    SEGGER_SYSVIEW_HeapDefine(memheap, heapBegin, (uintptr_t)heapLimit - (uintptr_t)heapBegin, metaSize);
}

static void hookMemheapAlloc(MDS_MemHeap_t *memheap, void *ptr, size_t size)
{
    SEGGER_SYSVIEW_HeapAlloc(memheap, ptr, size);
}

static void hookMemheapFree(MDS_MemHeap_t *memheap, void *ptr)
{
    SEGGER_SYSVIEW_HeapFree(memheap, ptr);
}

static void hookMemheapRealloc(MDS_MemHeap_t *memheap, void *old, void *new, size_t size)
{
    SEGGER_SYSVIEW_HeapFree(memheap, old);
    SEGGER_SYSVIEW_HeapAlloc(memheap, new, size);
}
#endif

// Services provided to SYSVIEW by MDS
static const SEGGER_SYSVIEW_OS_API SYSVIEW_X_OS_TraceAPI = {
    _cbGetTime,
    _cbSendTaskList,
};

void SEGGER_SYSVIEW_HookRegister(void)
{
    MDS_HOOK_REGISTER(INTERRUPT_ENTER, hookInterruptEnter);
    MDS_HOOK_REGISTER(INTERRUPT_EXIT, hookInterruptExit);
    MDS_HOOK_REGISTER(SCHEDULER_SWITCH, hookSchedulerSwitch);

    MDS_HOOK_REGISTER(TIMER_ENTER, hookTimerEnter);
    MDS_HOOK_REGISTER(TIMER_EXIT, hookTimerExit);

    MDS_HOOK_REGISTER(THREAD_INIT, hookThreadInit);
    MDS_HOOK_REGISTER(THREAD_EXIT, hookThreadExit);
    MDS_HOOK_REGISTER(THREAD_RESUME, hookThreadResume);
    MDS_HOOK_REGISTER(THREAD_SUSPEND, hookThreadSuspend);

#if (defined(MDS_SYSTEMVIEW_TRACE_IPC) && (MDS_SYSTEMVIEW_TRACE_IPC > 0))
    MDS_HOOK_REGISTER(SEMAPHORE_TRY_ACQUIRE, hookSemaphoreTryAcquire);
    MDS_HOOK_REGISTER(SEMAPHORE_HAS_ACQUIRE, hookSemaphoreHasAcquire);
    MDS_HOOK_REGISTER(SEMAPHORE_HAS_RELEASE, hookSemaphoreHasRelease);

    MDS_HOOK_REGISTER(MUTEX_TRY_ACQUIRE, hookMutexTryAcquire);
    MDS_HOOK_REGISTER(MUTEX_HAS_ACQUIRE, hookMutexHasAcquire);
    MDS_HOOK_REGISTER(MUTEX_HAS_RELEASE, hookMutexHasRelease);

    MDS_HOOK_REGISTER(EVENT_TRY_ACQUIRE, hookEventTryAcquire);
    MDS_HOOK_REGISTER(EVENT_HAS_ACQUIRE, hookEventHasAcquire);
    MDS_HOOK_REGISTER(EVENT_HAS_SET, hookEventHasSet);
    MDS_HOOK_REGISTER(EVENT_HAS_CLR, hookEventHasClr);

    MDS_HOOK_REGISTER(MSGQUEUE_TRY_RECV, hookMsgQueueTryRecv);
    MDS_HOOK_REGISTER(MSGQUEUE_HAS_RECV, hookMsgQueueHasRecv);
    MDS_HOOK_REGISTER(MSGQUEUE_TRY_SEND, hookMsgQueueTrySend);
    MDS_HOOK_REGISTER(MSGQUEUE_HAS_SEND, hookMsgQueueHasSend);

    MDS_HOOK_REGISTER(MEMPOOL_TRY_ALLOC, hookMemPoolTryAlloc);
    MDS_HOOK_REGISTER(MEMPOOL_HAS_ALLOC, hookMemPoolHasAlloc);
    MDS_HOOK_REGISTER(MEMPOOL_HAS_FREE, hookMemPoolHasFree);
#endif

#if (defined(MDS_SYSTEMVIEW_TRACE_MEM) && (MDS_SYSTEMVIEW_TRACE_MEM > 0))
    MDS_HOOK_REGISTER(MEMHEAP_INIT, hookMemheapInit);
    MDS_HOOK_REGISTER(MEMHEAP_ALLOC, hookMemheapAlloc);
    MDS_HOOK_REGISTER(MEMHEAP_FREE, hookMemheapFree);
    MDS_HOOK_REGISTER(MEMHEAP_REALLOC, hookMemheapRealloc);
#endif
}

__attribute__((weak)) void SEGGER_SYSVIEW_Conf(void)
{
    SEGGER_SYSVIEW_HookRegister();

    SEGGER_SYSVIEW_Init(MDS_SYSTICK_FREQ_HZ, MDS_SYSTICK_FREQ_HZ, &SYSVIEW_X_OS_TraceAPI, NULL);

    SEGGER_SYSVIEW_Start();
}

U32 SEGGER_SYSVIEW_X_GetInterruptId(void)
{
    return ((U32)(MDS_CoreInterruptCurrent()));
}

U32 SEGGER_SYSVIEW_X_GetTimestamp(void)
{
    // get cpu clock
    return (0);
}

/*************************** End of file ****************************/
