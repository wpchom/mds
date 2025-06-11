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
#ifndef __MDS_SYS_H__
#define __MDS_SYS_H__

/* Include ----------------------------------------------------------------- */
#include "mds_def.h"
#include "mds_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Config ------------------------------------------------------------------ */
#ifndef CONFIG_MDS_CLOCK_TICK_FREQ_HZ
#define CONFIG_MDS_CLOCK_TICK_FREQ_HZ 1000U
#endif

#ifndef CONFIG_MDS_CORE_BACKTRACE_DEPTH
#define CONFIG_MDS_CORE_BACKTRACE_DEPTH 16
#endif

#ifndef CONFIG_MDS_OBJECT_NAME_SIZE
#define CONFIG_MDS_OBJECT_NAME_SIZE 7
#endif

#ifndef CONFIG_MDS_KERNEL_LOG_LEVEL
#define CONFIG_MDS_KERNEL_LOG_LEVEL MDS_LOG_LEVEL_WRN
#endif

#ifndef CONFIG_MDS_KERNEL_SMP_CPUS
#define CONFIG_MDS_KERNEL_SMP_CPUS 1
#endif

#ifndef CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX
#define CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX 32
#endif

#ifndef CONFIG_MDS_TIMER_SKIPLIST_LEVEL
#define CONFIG_MDS_TIMER_SKIPLIST_LEVEL 1
#endif

#ifndef CONFIG_MDS_TIMER_SKIPLIST_SHIFT
#define CONFIG_MDS_TIMER_SKIPLIST_SHIFT 2
#endif

#ifndef CONFIG_MDS_INIT_SECTION
#define CONFIG_MDS_INIT_SECTION ".init.mdsInit."
#endif

/* Init -------------------------------------------------------------------- */
#define MDS_INIT_PRIORITY_0 "0."
#define MDS_INIT_PRIORITY_1 "1."
#define MDS_INIT_PRIORITY_2 "2."
#define MDS_INIT_PRIORITY_3 "3."
#define MDS_INIT_PRIORITY_4 "4."
#define MDS_INIT_PRIORITY_5 "5."
#define MDS_INIT_PRIORITY_6 "6."
#define MDS_INIT_PRIORITY_7 "7."
#define MDS_INIT_PRIORITY_8 "8."
#define MDS_INIT_PRIORITY_9 "9."

typedef void (*MDS_InitEntry_t)(void);

#define MDS_INIT_IMPORT(priority, func)                                                           \
    static __attribute__((used, section(CONFIG_MDS_INIT_SECTION priority #func)))                 \
    const MDS_InitEntry_t __MDS_INIT_##func = (func)

static inline void MDS_InitExport(void)
{
    static __attribute__((section(CONFIG_MDS_INIT_SECTION " "))) const void *initBegin = NULL;
    static __attribute__((section(CONFIG_MDS_INIT_SECTION "~"))) const void *initLimit = NULL;

    for (const MDS_InitEntry_t *init =
             (MDS_InitEntry_t *)((uintptr_t)(&initBegin) + sizeof(void *));
         init < (MDS_InitEntry_t *)(&initLimit); init++) {
        if (*init != NULL) {
            (*init)();
        }
    }
}

/* Clock ------------------------------------------------------------------- */
#define MDS_CLOCK_TICK_NO_WAIT   ((MDS_Tick_t)(0))
#define MDS_CLOCK_TICK_FOREVER   ((MDS_Tick_t)(-1))
#define MDS_CLOCK_TICK_TIMER_MAX ((MDS_Tick_t)(MDS_CLOCK_TICK_FOREVER / 2))

#define MDS_CLOCK_TICK_TO_MS(t) ((t) * MDS_TIME_MSEC_OF_SEC / CONFIG_MDS_CLOCK_TICK_FREQ_HZ)
#define MDS_CLOCK_TICK_TO_US(t) ((t) * MDS_TIME_USEC_OF_SEC / CONFIG_MDS_CLOCK_TICK_FREQ_HZ)

#define MDS_TIMEOUT_TICKS(t) ((MDS_Timeout_t) {.ticks = (MDS_Tick_t)(t)})
#define MDS_TIMEOUT_NO_WAIT  MDS_TIMEOUT_TICKS(MDS_CLOCK_TICK_NO_WAIT)
#define MDS_TIMEOUT_FOREVER  MDS_TIMEOUT_TICKS(MDS_CLOCK_TICK_FOREVER)

#define MDS_TIMEOUT_SEC(t) MDS_TIMEOUT_TICKS(((t) * CONFIG_MDS_CLOCK_TICK_FREQ_HZ))
#define MDS_TIMEOUT_MS(t)                                                                         \
    MDS_TIMEOUT_TICKS(((t) * CONFIG_MDS_CLOCK_TICK_FREQ_HZ / MDS_TIME_MSEC_OF_SEC))
#define MDS_TIMEOUT_US(t)                                                                         \
    MDS_TIMEOUT_TICKS(((t) * CONFIG_MDS_CLOCK_TICK_FREQ_HZ / MDS_TIME_USEC_OF_SEC))

void MDS_SysTickHandler(void);
MDS_Tick_t MDS_ClockGetTickCount(void);
void MDS_ClockIncTickCount(MDS_Tick_t ticks);

static inline void MDS_ClockDelayRawTick(MDS_Timeout_t timeout)
{
    MDS_Tick_t ticklimit = MDS_ClockGetTickCount() + timeout.ticks;

    while (MDS_ClockGetTickCount() < ticklimit) {
    }
}

static inline void MDS_ClockDelayCount(MDS_Timeout_t timeout)
{
    for (volatile MDS_Tick_t count = timeout.ticks; count > 0; count -= 1) {
    }
}

/* Core -------------------------------------------------------------------- */
void MDS_CoreIdleSleep(void);
MDS_Lock_t MDS_CoreInterruptLock(void);
void MDS_CoreInterruptRestore(MDS_Lock_t lock);

/* Spinlock ---------------------------------------------------------------- */
typedef struct MDS_SpinLock {
#if defined(CONFIG_MDS_KERNEL_SMP_CPUS) && (CONFIG_MDS_KERNEL_SMP_CPUS > 1)
    intptr_t locked;
#endif
} MDS_SpinLock_t;

MDS_Err_t MDS_SpinLockInit(MDS_SpinLock_t *spinlock);
MDS_Err_t MDS_SpinLockAcquire(MDS_SpinLock_t *spinlock);
MDS_Err_t MDS_SpinLockRelease(MDS_SpinLock_t *spinlock);

/* Object ------------------------------------------------------------------ */
typedef enum MDS_ObjectType {
    MDS_OBJECT_TYPE_NONE = 0,
    MDS_OBJECT_TYPE_DEVICE,
    MDS_OBJECT_TYPE_THREAD,
    MDS_OBJECT_TYPE_WORKQUEUE,
    MDS_OBJECT_TYPE_WORKNODE,
    MDS_OBJECT_TYPE_SEMAPHORE,
    MDS_OBJECT_TYPE_MUTEX,
    MDS_OBJECT_TYPE_EVENT,
    MDS_OBJECT_TYPE_POLL,
    MDS_OBJECT_TYPE_MSGQUEUE,
    MDS_OBJECT_TYPE_MEMPOOL,
    MDS_OBJECT_TYPE_MEMHEAP,
} __attribute__((packed)) MDS_ObjectType_t;

typedef struct MDS_Thread MDS_Thread_t;
typedef struct MDS_WorkQueue MDS_WorkQueue_t;
typedef struct MDS_WorkNode MDS_WorkNode_t;
typedef struct MDS_WorkNode MDS_Timer_t;
typedef struct MDS_Semaphore MDS_Semaphore_t;
typedef struct MDS_Mutex MDS_Mutex_t;
typedef struct MDS_Event MDS_Event_t;
typedef struct MDS_Poll MDS_Poll_t;
typedef struct MDS_MsgQueue MDS_MsgQueue_t;
typedef struct MDS_MemPool MDS_MemPool_t;
typedef struct MDS_MemHeap MDS_MemHeap_t;

typedef struct MDS_ObjectInfo {
    MDS_DListNode_t list;
    MDS_SpinLock_t spinlock;
} MDS_ObjectInfo_t;

typedef struct MDS_Object {
    MDS_DListNode_t node;
    MDS_ObjectType_t type : 7;
    bool created          : 1;
    char name[CONFIG_MDS_OBJECT_NAME_SIZE];
} MDS_Object_t;

MDS_Err_t MDS_ObjectInit(MDS_Object_t *object, MDS_ObjectType_t type, const char *name);
MDS_Err_t MDS_ObjectDeInit(MDS_Object_t *object);
MDS_Object_t *MDS_ObjectCreate(size_t typesz, MDS_ObjectType_t type, const char *name);
MDS_Err_t MDS_ObjectDestroy(MDS_Object_t *object);
MDS_Object_t *MDS_ObjectFind(MDS_ObjectType_t type, const char *name);
MDS_ObjectInfo_t *MDS_ObjectGetInfo(MDS_ObjectType_t type);
size_t MDS_ObjectGetCount(MDS_ObjectType_t type);
const char *MDS_ObjectGetName(const MDS_Object_t *object);
MDS_ObjectType_t MDS_ObjectGetType(const MDS_Object_t *object);
bool MDS_ObjectIsCreated(const MDS_Object_t *object);

/* Kernel ------------------------------------------------------------------ */
typedef union MDS_WaitQueue {
    MDS_DListNode_t list;
    MDS_DListNode_t node;
} MDS_WaitQueue_t;

typedef struct MDS_ThreadPriority {
    int8_t priority;
} __attribute__((packed)) MDS_ThreadPriority_t;

void MDS_KernelInit(void);
void MDS_KernelStartup(void);
MDS_Thread_t *MDS_KernelCurrentThread(void);
MDS_Tick_t MDS_KernelGetSleepTick(void);
void MDS_KernelCompensateTick(MDS_Tick_t ticks);

/* WorkQueue --------------------------------------------------------------- */
typedef void (*MDS_WorkEntry_t)(const MDS_WorkNode_t *workn, MDS_Arg_t *arg);

struct MDS_WorkQueue {
    MDS_Object_t object;

    MDS_Thread_t *thread;
    MDS_DListNode_t list[CONFIG_MDS_TIMER_SKIPLIST_LEVEL];

    MDS_SpinLock_t spinlock;
};

struct MDS_WorkNode {
    MDS_Object_t object;

    MDS_DListNode_t node[CONFIG_MDS_TIMER_SKIPLIST_LEVEL];
    MDS_WorkQueue_t *queue;
    MDS_Tick_t tickout;
    MDS_Tick_t tperiod;

    // callback
    MDS_WorkEntry_t entry;
    MDS_WorkEntry_t stop;
    MDS_Arg_t *arg;
};

MDS_Err_t MDS_WorkQueueInit(MDS_WorkQueue_t *workq, const char *name, MDS_Thread_t *thread,
                            void *stackPool, size_t stackSize, MDS_ThreadPriority_t priority,
                            MDS_Timeout_t timeout);
MDS_Err_t MDS_WorkQueueDeInit(MDS_WorkQueue_t *workq);
MDS_WorkQueue_t *MDS_WorkQueueCreate(const char *name, size_t stackSize,
                                     MDS_ThreadPriority_t priority, MDS_Timeout_t timeout);
MDS_Err_t MDS_WorkQueueDestroy(MDS_WorkQueue_t *workq);
MDS_Err_t MDS_WorkQueueStart(MDS_WorkQueue_t *workq);
MDS_Err_t MDS_WorkQueueStop(MDS_WorkQueue_t *workq);
MDS_Tick_t MDS_WorkQueueNextTick(MDS_WorkQueue_t *workq);

MDS_Err_t MDS_WorkNodeInit(MDS_WorkNode_t *workn, const char *name, MDS_WorkEntry_t entry,
                           MDS_WorkEntry_t stop, MDS_Arg_t *arg);
MDS_Err_t MDS_WorkNodeDeInit(MDS_WorkNode_t *workn);
MDS_WorkNode_t *MDS_WorkNodeCreate(const char *name, MDS_WorkEntry_t entry, MDS_WorkEntry_t stop,
                                   MDS_Arg_t *arg);
MDS_Err_t MDS_WorkNodeDestroy(MDS_WorkNode_t *workn);
MDS_Err_t MDS_WorkNodeSubmit(MDS_WorkQueue_t *workq, MDS_WorkNode_t *workn, MDS_Timeout_t delay,
                             MDS_Timeout_t period);
MDS_Err_t MDS_WorkNodeCancle(MDS_WorkNode_t *workn);
bool MDS_WorkNodeIsSubmit(const MDS_WorkNode_t *workn);

/* Timer ------------------------------------------------------------------- */
typedef MDS_WorkEntry_t MDS_TimerEntry_t;

MDS_Err_t MDS_TimerInit(MDS_Timer_t *timer, const char *name, MDS_TimerEntry_t entry,
                        MDS_TimerEntry_t stop, MDS_Arg_t *arg);
MDS_Err_t MDS_TimerDeInit(MDS_Timer_t *timer);
MDS_Timer_t *MDS_TimerCreate(const char *name, MDS_TimerEntry_t entry, MDS_TimerEntry_t stop,
                             MDS_Arg_t *arg);
MDS_Err_t MDS_TimerDestroy(MDS_Timer_t *timer);
MDS_Err_t MDS_TimerStart(MDS_Timer_t *timer, MDS_Timeout_t duration, MDS_Timeout_t period);
MDS_Err_t MDS_TimerStop(MDS_Timer_t *timer);
bool MDS_TimerIsActive(const MDS_Timer_t *timer);

/* Thread ------------------------------------------------------------------ */
typedef void (*MDS_ThreadEntry_t)(MDS_Arg_t *arg);

#define MDS_THREAD_PRIORITY(n) ((MDS_ThreadPriority_t) {n})

typedef enum MDS_ThreadState {
    MDS_THREAD_STATE_INACTIVED = 0x00U,
    MDS_THREAD_STATE_TERMINATED = 0x01U,
    MDS_THREAD_STATE_READY = 0x02U,
    MDS_THREAD_STATE_RUNNING = 0x03U,
    MDS_THREAD_STATE_SUSPENDED = 0x04U,

    MDS_THREAD_STATE_MASK = 0x0FU,
    MDS_THREAD_FLAG_YIELD = 0x80U,
} __attribute__((packed)) MDS_ThreadState_t;

typedef enum MDS_EventOpt {
    MDS_EVENT_OPT_NONE = 0x00,
    MDS_EVENT_OPT_OR = 0x01,
    MDS_EVENT_OPT_AND = 0x02,
    MDS_EVENT_OPT_NOCLR = 0x08,
    MDS_EVENT_OPT_OR_NOCLR = MDS_EVENT_OPT_OR | MDS_EVENT_OPT_NOCLR,
    MDS_EVENT_OPR_AND_NOCLR = MDS_EVENT_OPT_AND | MDS_EVENT_OPT_NOCLR,
} __attribute__((packed)) MDS_EventOpt_t;

struct MDS_Thread {
    MDS_Object_t object;
    MDS_WaitQueue_t nodeWait;

    MDS_ThreadEntry_t entry;
    MDS_Arg_t *arg;

    size_t stackSize;
    void *stackBase;
    void *stackPoint;

    MDS_Tick_t initTick;
    MDS_Tick_t remainTick;
    MDS_Timer_t timer;

    volatile MDS_Err_t err;
    MDS_ThreadPriority_t initPrio;
    MDS_ThreadPriority_t currPrio;
    MDS_ThreadState_t state;
    MDS_EventOpt_t eventOpt;
    MDS_Mask_t eventMask;

#if (defined(CONFIG_MDS_KERNEL_STATS_ENABLE) && (CONFIG_MDS_KERNEL_STATS_ENABLE != 0))
    void *stackWater;
#endif

    MDS_SpinLock_t spinlock;
};

MDS_Err_t MDS_ThreadInit(MDS_Thread_t *thread, const char *name, MDS_ThreadEntry_t entry,
                         MDS_Arg_t *arg, void *stackPool, size_t stackSize,
                         MDS_ThreadPriority_t priority, MDS_Timeout_t timeout);
MDS_Err_t MDS_ThreadDeInit(MDS_Thread_t *thread);
MDS_Thread_t *MDS_ThreadCreate(const char *name, MDS_ThreadEntry_t entry, MDS_Arg_t *arg,
                               size_t stackSize, MDS_ThreadPriority_t priority,
                               MDS_Timeout_t timeout);
MDS_Err_t MDS_ThreadDestroy(MDS_Thread_t *thread);
MDS_Err_t MDS_ThreadStartup(MDS_Thread_t *thread);
MDS_Err_t MDS_ThreadResume(MDS_Thread_t *thread);
MDS_Err_t MDS_ThreadSuspend(MDS_Thread_t *thread);
MDS_Err_t MDS_ThreadSetPriority(MDS_Thread_t *thread, MDS_ThreadPriority_t priority);
MDS_ThreadState_t MDS_ThreadGetState(const MDS_Thread_t *thread);
MDS_Err_t MDS_ThreadDelay(MDS_Timeout_t timeout);
MDS_Err_t MDS_ThreadYield(void);

/* Semaphore --------------------------------------------------------------- */
struct MDS_Semaphore {
    MDS_Object_t object;
    MDS_WaitQueue_t queueWait;

    size_t value;
    size_t max;

    MDS_SpinLock_t spinlock;
};

MDS_Err_t MDS_SemaphoreInit(MDS_Semaphore_t *semaphore, const char *name, size_t init, size_t max);
MDS_Err_t MDS_SemaphoreDeInit(MDS_Semaphore_t *semaphore);
MDS_Semaphore_t *MDS_SemaphoreCreate(const char *name, size_t init, size_t max);
MDS_Err_t MDS_SemaphoreDestroy(MDS_Semaphore_t *semaphore);
MDS_Err_t MDS_SemaphoreAcquire(MDS_Semaphore_t *semaphore, MDS_Timeout_t timeout);
MDS_Err_t MDS_SemaphoreRelease(MDS_Semaphore_t *semaphore);
size_t MDS_SemaphoreGetValue(const MDS_Semaphore_t *semaphore, size_t *max);

/* Condition --------------------------------------------------------------- */
typedef MDS_Semaphore_t MDS_Condition_t;

MDS_Err_t MDS_ConditionInit(MDS_Condition_t *condition, const char *name);
MDS_Err_t MDS_ConditionDeInit(MDS_Condition_t *condition);
MDS_Condition_t *MDS_ConditionCreate(const char *name);
MDS_Err_t MDS_ConditionDestroy(MDS_Condition_t *condition);
MDS_Err_t MDS_ConditionBroadCast(MDS_Condition_t *condition);
MDS_Err_t MDS_ConditionSignal(MDS_Condition_t *condition);
MDS_Err_t MDS_ConditionWait(MDS_Condition_t *condition, MDS_Mutex_t *mutex, MDS_Timeout_t timeout);

/* Mutex ------------------------------------------------------------------- */
struct MDS_Mutex {
    MDS_Object_t object;
    MDS_WaitQueue_t queueWait;

    MDS_Thread_t *owner;
    MDS_ThreadPriority_t priority;
    int8_t value;
    int16_t nest;

    MDS_SpinLock_t spinlock;
};

MDS_Err_t MDS_MutexInit(MDS_Mutex_t *mutex, const char *name);
MDS_Err_t MDS_MutexDeInit(MDS_Mutex_t *mutex);
MDS_Mutex_t *MDS_MutexCreate(const char *name);
MDS_Err_t MDS_MutexDestroy(MDS_Mutex_t *mutex);
MDS_Err_t MDS_MutexAcquire(MDS_Mutex_t *mutex, MDS_Timeout_t timeout);
MDS_Err_t MDS_MutexRelease(MDS_Mutex_t *mutex);
MDS_Thread_t *MDS_MutexGetOwner(const MDS_Mutex_t *mutex);

/* RwLock ------------------------------------------------------------------ */
typedef struct MDS_RwLock {
    MDS_Mutex_t mutex;
    MDS_Condition_t condRd;
    MDS_Condition_t condWr;

    int readers;
} MDS_RwLock_t;

MDS_Err_t MDS_RwLockInit(MDS_RwLock_t *rwlock, const char *name);
MDS_Err_t MDS_RwLockDeInit(MDS_RwLock_t *rwlock);
MDS_Err_t MDS_RwLockAcquireRead(MDS_RwLock_t *rwlock, MDS_Timeout_t timeout);
MDS_Err_t MDS_RwLockAcquireWrite(MDS_RwLock_t *rwlock, MDS_Timeout_t timeout);
MDS_Err_t MDS_RwLockRelease(MDS_RwLock_t *rwlock);

/* Event ------------------------------------------------------------------- */
struct MDS_Event {
    MDS_Object_t object;
    MDS_WaitQueue_t queueWait;

    MDS_Mask_t value;

    MDS_SpinLock_t spinlock;
};

MDS_Err_t MDS_EventInit(MDS_Event_t *event, const char *name);
MDS_Err_t MDS_EventDeInit(MDS_Event_t *event);
MDS_Event_t *MDS_EventCreate(const char *name);
MDS_Err_t MDS_EventDestroy(MDS_Event_t *event);
MDS_Err_t MDS_EventWait(MDS_Event_t *event, MDS_Mask_t mask, MDS_EventOpt_t opt, MDS_Mask_t *recv,
                        MDS_Timeout_t timeout);
MDS_Err_t MDS_EventSet(MDS_Event_t *event, MDS_Mask_t mask);
MDS_Err_t MDS_EventClr(MDS_Event_t *event, MDS_Mask_t mask);
MDS_Mask_t MDS_EventGetValue(const MDS_Event_t *event);

/* MsgQueue ---------------------------------------------------------------- */
struct MDS_MsgQueue {
    MDS_Object_t object;
    MDS_WaitQueue_t queueRecv;
    MDS_WaitQueue_t queueSend;

    void *queBuff;
    size_t msgSize;

    void *lfree;
    void *lhead;
    void *ltail;

    MDS_SpinLock_t spinlock;
};

MDS_Err_t MDS_MsgQueueInit(MDS_MsgQueue_t *msgQueue, const char *name, void *queBuff,
                           size_t bufSize, size_t msgSize);
MDS_Err_t MDS_MsgQueueDeInit(MDS_MsgQueue_t *msgQueue);
MDS_MsgQueue_t *MDS_MsgQueueCreate(const char *name, size_t msgSize, size_t msgNums);
MDS_Err_t MDS_MsgQueueDestroy(MDS_MsgQueue_t *msgQueue);
MDS_Err_t MDS_MsgQueueRecvAcquire(MDS_MsgQueue_t *msgQueue, void *recv, MDS_Timeout_t timeout);
MDS_Err_t MDS_MsgQueueRecvRelease(MDS_MsgQueue_t *msgQueue, void *recv);
MDS_Err_t MDS_MsgQueueRecvPeek(MDS_MsgQueue_t *msgQueue, void *buff, size_t size,
                               MDS_Timeout_t timeout);
MDS_Err_t MDS_MsgQueueRecvCopy(MDS_MsgQueue_t *msgQueue, void *buff, size_t size,
                               MDS_Timeout_t timeout);
MDS_Err_t MDS_MsgQueueSendMsg(MDS_MsgQueue_t *msgQueue, const MDS_MsgList_t *msgList,
                              MDS_Timeout_t timeout);
MDS_Err_t MDS_MsgQueueSend(MDS_MsgQueue_t *msgQueue, const void *buff, size_t len,
                           MDS_Timeout_t timeout);
MDS_Err_t MDS_MsgQueueUrgentMsg(MDS_MsgQueue_t *msgQueue, const MDS_MsgList_t *msgList);
MDS_Err_t MDS_MsgQueueUrgent(MDS_MsgQueue_t *msgQueue, const void *buff, size_t len);
size_t MDS_MsgQueueGetMsgSize(MDS_MsgQueue_t *msgQueue);
size_t MDS_MsgQueueGetMsgCount(MDS_MsgQueue_t *msgQueue);
size_t MDS_MsgQueueGetMsgFree(MDS_MsgQueue_t *msgQueue);

/* MemPool ----------------------------------------------------------------- */
struct MDS_MemPool {
    MDS_Object_t object;
    MDS_WaitQueue_t queueWait;

    void *memBuff;
    size_t blkSize;
    void *lfree;

    MDS_SpinLock_t spinlock;
};

MDS_Err_t MDS_MemPoolInit(MDS_MemPool_t *memPool, const char *name, void *memBuff, size_t bufSize,
                          size_t blkSize);
MDS_Err_t MDS_MemPoolDeInit(MDS_MemPool_t *memPool);
MDS_MemPool_t *MDS_MemPoolCreate(const char *name, size_t blkSize, size_t blkNums);
MDS_Err_t MDS_MemPoolDestroy(MDS_MemPool_t *memPool);
void *MDS_MemPoolAlloc(MDS_MemPool_t *memPool, MDS_Timeout_t timeout);
void MDS_MemPoolFree(void *blkPtr);
size_t MDS_MemPoolGetBlkSize(MDS_MemPool_t *memPool);
size_t MDS_MemPoolGetBlkFree(MDS_MemPool_t *memPool);

/* MemHeap ----------------------------------------------------------------- */
typedef struct MDS_MemHeapSize {
    size_t cur, max, total;
} MDS_MemHeapSize_t;

typedef struct MDS_MemHeapOps {
    MDS_Err_t (*setup)(MDS_MemHeap_t *memheap, void *heapBase, size_t heapSize);
    void (*free)(MDS_MemHeap_t *memheap, void *ptr);
    void *(*alloc)(MDS_MemHeap_t *memheap, size_t size);
    void *(*realloc)(MDS_MemHeap_t *memheap, void *ptr, size_t size);
    void (*size)(MDS_MemHeap_t *memheap, MDS_MemHeapSize_t *size);
} MDS_MemHeapOps_t;

struct MDS_MemHeap {
    MDS_Object_t object;

    MDS_Semaphore_t lock;

    const MDS_MemHeapOps_t *ops;
    void *begin, *limit;
#if (defined(CONFIG_MDS_KERNEL_STATS_ENABLE) && (CONFIG_MDS_KERNEL_STATS_ENABLE != 0))
    MDS_MemHeapSize_t size;
#endif
};

MDS_Err_t MDS_MemHeapInit(MDS_MemHeap_t *memheap, const char *name, void *base, size_t size,
                          const MDS_MemHeapOps_t *ops);
MDS_Err_t MDS_MemHeapDeInit(MDS_MemHeap_t *memheap);
void MDS_MemHeapFree(MDS_MemHeap_t *memheap, void *ptr);
void *MDS_MemHeapAlloc(MDS_MemHeap_t *memheap, size_t size);
void *MDS_MemHeapRealloc(MDS_MemHeap_t *memheap, void *ptr, size_t size);
void *MDS_MemHeapCalloc(MDS_MemHeap_t *memheap, size_t nmemb, size_t size);
void MDS_MemHeapSize(MDS_MemHeap_t *memheap, MDS_MemHeapSize_t *size);

extern const MDS_MemHeapOps_t G_MDS_MEMHEAP_OPS_LLFF;
extern const MDS_MemHeapOps_t G_MDS_MEMHEAP_OPS_TLSF;

/* SysMem ------------------------------------------------------------------ */
#define MDS_SYSMEM_ALIGN_SIZE sizeof(uintptr_t)

void *MDS_SysMemAlloc(size_t size);
void *MDS_SysMemCalloc(size_t nmemb, size_t size);
void *MDS_SysMemRealloc(void *ptr, size_t size);
void MDS_SysMemFree(void *ptr);

/* Hook -------------------------------------------------------------------- */
#ifndef CONFIG_MDS_HOOK_ENABLE_KERNEL
#define CONFIG_MDS_HOOK_ENABLE_KERNEL 0
#endif

typedef enum MDS_KERNEL_Trace {
    MDS_KERNEL_TRACE_SCHEDULER_SWITCH,

    MDS_KERNEL_TRACE_THREAD_INIT,
    MDS_KERNEL_TRACE_THREAD_EXIT,
    MDS_KERNEL_TRACE_THREAD_RESUME,
    MDS_KERNEL_TRACE_THREAD_SUSPEND,

    MDS_KERNEL_TRACE_TIMER_ENTER,
    MDS_KERNEL_TRACE_TIMER_EXIT,
    MDS_KERNEL_TRACE_TIMER_START,
    MDS_KERNEL_TRACE_TIMER_STOP,

    MDS_KERNEL_TRACE_MEMHEAP_INIT,
    MDS_KERNEL_TRACE_MEMHEAP_ALLOC,
    MDS_KERNEL_TRACE_MEMHEAP_FREE,
    MDS_KERNEL_TRACE_MEMHEAP_REALLOC,

    MDS_KERNEL_TRACE_SEMAPHORE_TRY_ACQUIRE,
    MDS_KERNEL_TRACE_SEMAPHORE_HAS_ACQUIRE,
    MDS_KERNEL_TRACE_SEMAPHORE_HAS_RELEASE,

    MDS_KERNEL_TRACE_MUTEX_TRY_ACQUIRE,
    MDS_KERNEL_TRACE_MUTEX_HAS_ACQUIRE,
    MDS_KERNEL_TRACE_MUTEX_HAS_RELEASE,

    MDS_KERNEL_TRACE_EVENT_TRY_ACQUIRE,
    MDS_KERNEL_TRACE_EVENT_HAS_ACQUIRE,
    MDS_KERNEL_TRACE_EVENT_HAS_SET,
    MDS_KERNEL_TRACE_EVENT_HAS_CLR,

    MDS_KERNEL_TRACE_MSGQUEUE_TRY_RECV,
    MDS_KERNEL_TRACE_MSGQUEUE_HAS_RECV,
    MDS_KERNEL_TRACE_MSGQUEUE_TRY_SEND,
    MDS_KERNEL_TRACE_MSGQUEUE_HAS_SEND,

    MDS_KERNEL_TRACE_MEMPOOL_TRY_ALLOC,
    MDS_KERNEL_TRACE_MEMPOOL_HAS_ALLOC,
    MDS_KERNEL_TRACE_MEMPOOL_HAS_FREE,

} MDS_KERNEL_Trace_t;

typedef struct MDS_HOOK_Kernel {
    void (*scheduler)(const MDS_Thread_t *toThread, const MDS_Thread_t *fromThread);
    void (*thread)(const MDS_Thread_t *thread, MDS_KERNEL_Trace_t id);
    void (*timer)(const MDS_Timer_t *timer, MDS_KERNEL_Trace_t id);
    void (*memheap)(const MDS_MemHeap_t *memheap, MDS_KERNEL_Trace_t id, void *free_begin,
                    void *alloc_limit, size_t size);
    void (*semaphore)(const MDS_Semaphore_t *semaphore, MDS_KERNEL_Trace_t id, MDS_Err_t err,
                      MDS_Timeout_t timeout);
    void (*mutex)(const MDS_Mutex_t *mutex, MDS_KERNEL_Trace_t id, MDS_Err_t err,
                  MDS_Timeout_t timeout);
    void (*event)(const MDS_Event_t *event, MDS_KERNEL_Trace_t id, MDS_Err_t err,
                  MDS_Timeout_t timeout);
    void (*msgqueue)(const MDS_MsgQueue_t *msgqueue, MDS_KERNEL_Trace_t id, MDS_Err_t err,
                     MDS_Timeout_t timeout);
    void (*mempool)(const MDS_MemPool_t *mempool, MDS_KERNEL_Trace_t id, MDS_Err_t err,
                    MDS_Timeout_t timeout, void *blk);
} MDS_HOOK_Kernel_t;

MDS_HOOK_DECLARE(KERNEL, MDS_HOOK_Kernel_t);

#ifdef __cplusplus
}
#endif

#endif /* __MDS_SYS_H__ */
