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

#ifdef __cplusplus
extern "C" {
#endif

/* Clock -------------------------------------------------------------------- */
typedef size_t MDS_Tick_t;

#ifndef MDS_CLOCK_TICK_FREQ_HZ
#define MDS_CLOCK_TICK_FREQ_HZ 1000U
#endif

#define MDS_CLOCK_TICK_FOREVER ((MDS_Tick_t)(-1))

extern MDS_Tick_t MDS_ClockGetTickCount(void);
extern void MDS_ClockSetTickCount(MDS_Tick_t tickcnt);
extern void MDS_ClockIncTickCount(void);
extern MDS_Time_t MDS_ClockGetTimestamp(int8_t *tz);
extern void MDS_ClockSetTimestamp(MDS_Time_t ts, int8_t tz);

static inline MDS_Time_t MDS_ClockTickToMs(MDS_Tick_t tick)
{
    return ((MDS_Time_t)tick * MDS_TIME_MSEC_OF_SEC / MDS_CLOCK_TICK_FREQ_HZ);
}

static inline MDS_Time_t MDS_ClockTickFromMs(MDS_Time_t ms)
{
    return (ms * MDS_CLOCK_TICK_FREQ_HZ / MDS_TIME_MSEC_OF_SEC);
}

static inline void MDS_ClockDelayRawTick(MDS_Tick_t delay)
{
    MDS_Tick_t ticklimit = MDS_ClockGetTickCount() + delay;

    while (MDS_ClockGetTickCount() < ticklimit) {
    }
}

static inline void MDS_ClockDelayCount(MDS_Tick_t delay)
{
    for (volatile MDS_Tick_t count = delay; count > 0; count -= 1) {
    }
}

/* Core --------------------------------------------------------------------- */
extern void MDS_CoreIdleSleep(void);
extern MDS_Item_t MDS_CoreInterruptCurrent(void);
extern MDS_Item_t MDS_CoreInterruptLock(void);
extern void MDS_CoreInterruptRestore(MDS_Item_t lock);

/* SysMem ------------------------------------------------------------------- */
#ifndef MDS_SYSMEM_ALIGN_SIZE
#define MDS_SYSMEM_ALIGN_SIZE sizeof(uintptr_t)
#endif

extern void *MDS_SysMemAlloc(size_t size);
extern void *MDS_SysMemCalloc(size_t nmemb, size_t size);
extern void *MDS_SysMemRealloc(void *ptr, size_t size);
extern void MDS_SysMemFree(void *ptr);

/* Object -------------------------------------------------------------------- */
#ifndef MDS_OBJECT_NAME_SIZE
#define MDS_OBJECT_NAME_SIZE 7
#endif

typedef struct MDS_Object {
    MDS_ListNode_t node;
    uint8_t flags;
    char name[MDS_OBJECT_NAME_SIZE];
} MDS_Object_t;

typedef enum MDS_ObjectType {
    MDS_OBJECT_TYPE_NONE = 0,

    MDS_OBJECT_TYPE_DEVICE,

    MDS_OBJECT_TYPE_TIMER,

    MDS_OBJECT_TYPE_THREAD,

    MDS_OBJECT_TYPE_SEMAPHORE,

    MDS_OBJECT_TYPE_MUTEX,

    MDS_OBJECT_TYPE_EVENT,

    MDS_OBJECT_TYPE_MSGQUEUE,

    MDS_OBJECT_TYPE_MEMPOOL,

    MDS_OBJECT_TYPE_MEMHEAP,
} MDS_ObjectType_t;

typedef struct MDS_Timer MDS_Timer_t;
typedef struct MDS_Thread MDS_Thread_t;
typedef struct MDS_Semaphore MDS_Semaphore_t;
typedef struct MDS_Mutex MDS_Mutex_t;
typedef struct MDS_Event MDS_Event_t;
typedef struct MDS_MsgQueue MDS_MsgQueue_t;
typedef struct MDS_MemPool MDS_MemPool_t;
typedef struct MDS_MemHeap MDS_MemHeap_t;

extern MDS_Err_t MDS_ObjectInit(MDS_Object_t *object, MDS_ObjectType_t type, const char *name);
extern MDS_Err_t MDS_ObjectDeInit(MDS_Object_t *object);
extern MDS_Object_t *MDS_ObjectCreate(size_t typesz, MDS_ObjectType_t type, const char *name);
extern MDS_Err_t MDS_ObjectDestory(MDS_Object_t *object);
extern MDS_Object_t *MDS_ObjectFind(MDS_ObjectType_t type, const char *name);
extern MDS_Object_t *MDS_ObjectPrev(const MDS_Object_t *object);
extern MDS_Object_t *MDS_ObjectNext(const MDS_Object_t *object);
extern const MDS_ListNode_t *MDS_ObjectGetList(MDS_ObjectType_t type);
extern size_t MDS_ObjectGetCount(MDS_ObjectType_t type);
extern const char *MDS_ObjectGetName(const MDS_Object_t *object);
extern MDS_ObjectType_t MDS_ObjectGetType(const MDS_Object_t *object);
extern bool MDS_ObjectIsCreated(const MDS_Object_t *object);

/* Kernel ------------------------------------------------------------------ */
extern void MDS_KernelInit(void);
extern void MDS_KernelStartup(void);
extern MDS_Thread_t *MDS_KernelCurrentThread(void);
extern void MDS_KernelEnterCritical(void);
extern void MDS_KernelExitCritical(void);
extern size_t MDS_KernelGetCritical(void);
extern MDS_Tick_t MDS_KernelGetSleepTick(void);
extern void MDS_KernelCompensateTick(MDS_Tick_t tickcount);

/* Timer ------------------------------------------------------------------- */
#ifndef MDS_TIMER_SKIPLIST_LEVEL
#define MDS_TIMER_SKIPLIST_LEVEL 1
#endif

#ifndef MDS_TIMER_SKIPLIST_SHIFT
#define MDS_TIMER_SKIPLIST_SHIFT 2
#endif

#ifndef MDS_TIMER_THREAD_ENABLE
#define MDS_TIMER_THREAD_ENABLE 1
#endif

typedef void (*MDS_TimerEntry_t)(MDS_Arg_t *arg);

enum MDS_TimerType {
    MDS_TIMER_TYPE_ONCE = 0x00U,
    MDS_TIMER_TYPE_PERIOD = 0x01U,

    MDS_TIMER_TYPE_SYSTEM = 0x08U,
    MDS_TIMER_FLAG_ACTIVED = 0x80U,
};

struct MDS_Timer {
    MDS_Object_t object;

    MDS_ListNode_t node[MDS_TIMER_SKIPLIST_LEVEL];
    MDS_TimerEntry_t entry;
    MDS_Arg_t *arg;
    MDS_Mask_t flags;
    MDS_Tick_t tickstart;
    MDS_Tick_t ticklimit;
};

extern MDS_Err_t MDS_TimerInit(MDS_Timer_t *timer, const char *name, MDS_Mask_t type, MDS_TimerEntry_t entry,
                               MDS_Arg_t *arg);
extern MDS_Err_t MDS_TimerDeInit(MDS_Timer_t *timer);
extern MDS_Timer_t *MDS_TimerCreate(const char *name, MDS_Mask_t type, MDS_TimerEntry_t entry, MDS_Arg_t *arg);
extern MDS_Err_t MDS_TimerDestroy(MDS_Timer_t *timer);
extern MDS_Err_t MDS_TimerStart(MDS_Timer_t *timer, MDS_Tick_t timeout);
extern MDS_Err_t MDS_TimerStop(MDS_Timer_t *timer);
extern bool MDS_TimerIsActived(const MDS_Timer_t *timer);

/* Thread ------------------------------------------------------------------ */
#ifndef MDS_THREAD_PRIORITY_MAX
#define MDS_THREAD_PRIORITY_MAX 32
#endif

typedef void (*MDS_ThreadEntry_t)(MDS_Arg_t *arg);

typedef uint8_t MDS_ThreadPriority_t;

typedef enum MDS_ThreadState {
    MDS_THREAD_STATE_INACTIVED = 0x00U,
    MDS_THREAD_STATE_READY = 0x01U,
    MDS_THREAD_STATE_RUNNING = 0x02U,
    MDS_THREAD_STATE_TERMINATED = 0x03U,
    MDS_THREAD_STATE_BLOCKED = 0x04U,

    MDS_THREAD_STATE_MASK = 0x0FU,
    MDS_THREAD_FLAG_YIELD = 0x80U,
} MDS_ThreadState_t;

struct MDS_Thread {
    MDS_Object_t object;
    MDS_ListNode_t node;

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
    uint8_t state;
    uint8_t eventOpt;
    MDS_Mask_t eventMask;

#if (defined(MDS_KERNEL_STATS_ENABLE) && (MDS_KERNEL_STATS_ENABLE > 0))
    void *stackWater;
#endif
};

extern MDS_Err_t MDS_ThreadInit(MDS_Thread_t *thread, const char *name, MDS_ThreadEntry_t entry, MDS_Arg_t *arg,
                                void *stackPool, size_t stackSize, MDS_ThreadPriority_t priority, MDS_Tick_t ticks);
extern MDS_Err_t MDS_ThreadDeInit(MDS_Thread_t *thread);
extern MDS_Thread_t *MDS_ThreadCreate(const char *name, MDS_ThreadEntry_t entry, MDS_Arg_t *arg, size_t stackSize,
                                      MDS_ThreadPriority_t priority, MDS_Tick_t ticks);
extern MDS_Err_t MDS_ThreadDestroy(MDS_Thread_t *thread);
extern MDS_Err_t MDS_ThreadStartup(MDS_Thread_t *thread);
extern MDS_Err_t MDS_ThreadResume(MDS_Thread_t *thread);
extern MDS_Err_t MDS_ThreadSuspend(MDS_Thread_t *thread);
extern MDS_Err_t MDS_ThreadDelayTick(MDS_Tick_t delay);
extern MDS_Err_t MDS_ThreadChangePriority(MDS_Thread_t *thread, MDS_ThreadPriority_t priority);
extern MDS_ThreadState_t MDS_ThreadGetState(const MDS_Thread_t *thread);

static inline MDS_Err_t MDS_ThreadDelayMs(MDS_Tick_t timeout)
{
    return (MDS_ThreadDelayTick(MDS_ClockTickFromMs(timeout)));
}

/* Semaphore --------------------------------------------------------------- */
struct MDS_Semaphore {
    MDS_Object_t object;
    MDS_ListNode_t list;

    size_t value;
    size_t max;
};

extern MDS_Err_t MDS_SemaphoreInit(MDS_Semaphore_t *semaphore, const char *name, size_t init, size_t max);
extern MDS_Err_t MDS_SemaphoreDeInit(MDS_Semaphore_t *semaphore);
extern MDS_Semaphore_t *MDS_SemaphoreCreate(const char *name, size_t init, size_t max);
extern MDS_Err_t MDS_SemaphoreDestroy(MDS_Semaphore_t *semaphore);
extern MDS_Err_t MDS_SemaphoreAcquire(MDS_Semaphore_t *semaphore, MDS_Tick_t timeout);
extern MDS_Err_t MDS_SemaphoreRelease(MDS_Semaphore_t *semaphore);
extern size_t MDS_SemaphoreGetValue(const MDS_Semaphore_t *semaphore, size_t *max);

/* Condition --------------------------------------------------------------- */
typedef MDS_Semaphore_t MDS_Condition_t;

extern MDS_Err_t MDS_ConditionInit(MDS_Condition_t *condition, const char *name);
extern MDS_Err_t MDS_ConditionDeInit(MDS_Condition_t *condition);
extern MDS_Err_t MDS_ConditionBroadCast(MDS_Condition_t *condition);
extern MDS_Err_t MDS_ConditionSignal(MDS_Condition_t *condition);
extern MDS_Err_t MDS_ConditionWait(MDS_Condition_t *condition, MDS_Mutex_t *mutex, MDS_Tick_t timeout);

/* Mutex ------------------------------------------------------------------- */
struct MDS_Mutex {
    MDS_Object_t object;
    MDS_ListNode_t list;

    MDS_Thread_t *owner;
    MDS_ThreadPriority_t priority;
    uint8_t value;
    uint16_t nest;
};

extern MDS_Err_t MDS_MutexInit(MDS_Mutex_t *mutex, const char *name);
extern MDS_Err_t MDS_MutexDeInit(MDS_Mutex_t *mutex);
extern MDS_Mutex_t *MDS_MutexCreate(const char *name);
extern MDS_Err_t MDS_MutexDestroy(MDS_Mutex_t *mutex);
extern MDS_Err_t MDS_MutexAcquire(MDS_Mutex_t *mutex, MDS_Tick_t timeout);
extern MDS_Err_t MDS_MutexRelease(MDS_Mutex_t *mutex);
extern MDS_Thread_t *MDS_MutexGetOwner(const MDS_Mutex_t *mutex);

/* RwLock ------------------------------------------------------------------ */
typedef struct MDS_RwLock {
    MDS_Mutex_t mutex;
    MDS_Condition_t condRd;
    MDS_Condition_t condWr;

    int readers;
} MDS_RwLock_t;

extern MDS_Err_t MDS_RwLockInit(MDS_RwLock_t *rwlock, const char *name);
extern MDS_Err_t MDS_RwLockDeInit(MDS_RwLock_t *rwlock);
extern MDS_Err_t MDS_RwLockAcquireRead(MDS_RwLock_t *rwlock, MDS_Tick_t timeout);
extern MDS_Err_t MDS_RwLockAcquireWrite(MDS_RwLock_t *rwlock, MDS_Tick_t timeout);
extern MDS_Err_t MDS_RwLockRelease(MDS_RwLock_t *rwlock);

/* Event ------------------------------------------------------------------- */
enum MDS_EventOpt {
    MDS_EVENT_OPT_OR = 0x01u,
    MDS_EVENT_OPT_AND = 0x02u,
    MDS_EVENT_OPT_NOCLR = 0x08u,
};

struct MDS_Event {
    MDS_Object_t object;
    MDS_ListNode_t list;

    MDS_Mask_t value;
};

extern MDS_Err_t MDS_EventInit(MDS_Event_t *event, const char *name);
extern MDS_Err_t MDS_EventDeInit(MDS_Event_t *event);
extern MDS_Event_t *MDS_EventCreate(const char *name);
extern MDS_Err_t MDS_EventDestroy(MDS_Event_t *event);
extern MDS_Err_t MDS_EventWait(MDS_Event_t *event, MDS_Mask_t mask, MDS_Mask_t opt, MDS_Mask_t *recv,
                               MDS_Tick_t timeout);
extern MDS_Err_t MDS_EventSet(MDS_Event_t *event, MDS_Mask_t mask);
extern MDS_Err_t MDS_EventClr(MDS_Event_t *event, MDS_Mask_t mask);
extern MDS_Mask_t MDS_EventGetValue(const MDS_Event_t *event);

/* MsgQueue ---------------------------------------------------------------- */
struct MDS_MsgQueue {
    MDS_Object_t object;
    MDS_ListNode_t listRecv;
    MDS_ListNode_t listSend;

    void *queBuff;
    size_t msgSize;

    void *lfree;
    void *lhead;
    void *ltail;
};

extern MDS_Err_t MDS_MsgQueueInit(MDS_MsgQueue_t *msgQueue, const char *name, void *queBuff, size_t bufSize,
                                  size_t msgSize);
extern MDS_Err_t MDS_MsgQueueDeInit(MDS_MsgQueue_t *msgQueue);
extern MDS_MsgQueue_t *MDS_MsgQueueCreate(const char *name, size_t msgSize, size_t msgNums);
extern MDS_Err_t MDS_MsgQueueDestroy(MDS_MsgQueue_t *msgQueue);
extern MDS_Err_t MDS_MsgQueueRecvAcquire(MDS_MsgQueue_t *msgQueue, void *recv, size_t *len, MDS_Tick_t timeout);
extern MDS_Err_t MDS_MsgQueueRecvRelease(MDS_MsgQueue_t *msgQueue, void *recv);
extern MDS_Err_t MDS_MsgQueueRecvCopy(MDS_MsgQueue_t *msgQueue, void *buff, size_t size, size_t *len,
                                      MDS_Tick_t timeout);
extern MDS_Err_t MDS_MsgQueueSendMsg(MDS_MsgQueue_t *msgQueue, const MDS_MsgList_t *msgList, MDS_Tick_t timeout);
extern MDS_Err_t MDS_MsgQueueSend(MDS_MsgQueue_t *msgQueue, const void *buff, size_t len, MDS_Tick_t timeout);
extern MDS_Err_t MDS_MsgQueueUrgentMsg(MDS_MsgQueue_t *msgQueue, const MDS_MsgList_t *msgList);
extern MDS_Err_t MDS_MsgQueueUrgent(MDS_MsgQueue_t *msgQueue, const void *buff, size_t len);
extern size_t MDS_MsgQueueGetMsgSize(const MDS_MsgQueue_t *msgQueue);
extern size_t MDS_MsgQueueGetMsgCount(const MDS_MsgQueue_t *msgQueue);
extern size_t MDS_MsgQueueGetMsgFree(const MDS_MsgQueue_t *msgQueue);

/* MemPool --------------------------------------------------------------- */
struct MDS_MemPool {
    MDS_Object_t object;
    MDS_ListNode_t list;

    void *memBuff;
    size_t blkSize;

    void *lfree;
};

extern MDS_Err_t MDS_MemPoolInit(MDS_MemPool_t *memPool, const char *name, void *memBuff, size_t bufSize,
                                 size_t blkSize);
extern MDS_Err_t MDS_MemPoolDeInit(MDS_MemPool_t *memPool);
extern MDS_MemPool_t *MDS_MemPoolCreate(const char *name, size_t blkSize, size_t blkNums);
extern MDS_Err_t MDS_MemPoolDestroy(MDS_MemPool_t *memPool);
extern void *MDS_MemPoolAlloc(MDS_MemPool_t *memPool, MDS_Tick_t timeout);
extern void MDS_MemPoolFree(void *blkPtr);
extern size_t MDS_MemPoolGetBlkSize(const MDS_MemPool_t *memPool);
extern size_t MDS_MemPoolGetBlkFree(const MDS_MemPool_t *memPool);

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

typedef struct MDS_MemHeap {
    MDS_Object_t object;
    MDS_Semaphore_t lock;
    const MDS_MemHeapOps_t *ops;
    void *begin, *limit;
#if (defined(MDS_KERNEL_STATS_ENABLE) && (MDS_KERNEL_STATS_ENABLE > 0))
    MDS_MemHeapSize_t size;
#endif
} MDS_MemHeap_t;

extern MDS_Err_t MDS_MemHeapInit(MDS_MemHeap_t *memheap, const char *name, void *base, size_t size,
                                 const MDS_MemHeapOps_t *ops);
extern MDS_Err_t MDS_MemHeapDeInit(MDS_MemHeap_t *memheap);
extern void MDS_MemHeapFree(MDS_MemHeap_t *memheap, void *ptr);
extern void *MDS_MemHeapAlloc(MDS_MemHeap_t *memheap, size_t size);
extern void *MDS_MemHeapRealloc(MDS_MemHeap_t *memheap, void *ptr, size_t size);
extern void *MDS_MemHeapCalloc(MDS_MemHeap_t *memheap, size_t nmemb, size_t size);
extern void MDS_MemHeapSize(MDS_MemHeap_t *memheap, MDS_MemHeapSize_t *size);

extern const MDS_MemHeapOps_t G_MDS_MEMHEAP_OPS_LLFF;
extern const MDS_MemHeapOps_t G_MDS_MEMHEAP_OPS_TLSF;

/* Hook -------------------------------------------------------------------- */
#if (defined(MDS_KERNEL_HOOK_ENABLE) && (MDS_KERNEL_HOOK_ENABLE > 0))
extern void MDS_HOOK_SCHEDULER_SWITCH_Register(void (*hook)(MDS_Thread_t *toThread, MDS_Thread_t *fromThread));

extern void MDS_HOOK_TIMER_ENTER_Register(void (*hook)(MDS_Timer_t *timer));
extern void MDS_HOOK_TIMER_EXIT_Register(void (*hook)(MDS_Timer_t *timer));
extern void MDS_HOOK_TIMER_START_Register(void (*hook)(MDS_Timer_t *timer));
extern void MDS_HOOK_TIMER_STOP_Register(void (*hook)(MDS_Timer_t *timer));

extern void MDS_HOOK_THREAD_INIT_Register(void (*hook)(MDS_Thread_t *thread));
extern void MDS_HOOK_THREAD_EXIT_Register(void (*hook)(MDS_Thread_t *thread));
extern void MDS_HOOK_THREAD_RESUME_Register(void (*hook)(MDS_Thread_t *thread));
extern void MDS_HOOK_THREAD_SUSPEND_Register(void (*hook)(MDS_Thread_t *thread));

extern void MDS_HOOK_SEMAPHORE_TRY_ACQUIRE_Register(void (*hook)(MDS_Semaphore_t *semaphore, MDS_Tick_t timeout));
extern void MDS_HOOK_SEMAPHORE_HAS_ACQUIRE_Register(void (*hook)(MDS_Semaphore_t *semaphore, MDS_Err_t err));
extern void MDS_HOOK_SEMAPHORE_HAS_RELEASE_Register(void (*hook)(MDS_Semaphore_t *semaphore));

extern void MDS_HOOK_MUTEX_TRY_ACQUIRE_Register(void (*hook)(MDS_Mutex_t *mutex, MDS_Tick_t timeout));
extern void MDS_HOOK_MUTEX_HAS_ACQUIRE_Register(void (*hook)(MDS_Mutex_t *mutex, MDS_Err_t err));
extern void MDS_HOOK_MUTEX_HAS_RELEASE_Register(void (*hook)(MDS_Mutex_t *mutex));

extern void MDS_HOOK_EVENT_TRY_ACQUIRE_Register(void (*hook)(MDS_Event_t *event, MDS_Tick_t timeout));
extern void MDS_HOOK_EVENT_HAS_ACQUIRE_Register(void (*hook)(MDS_Event_t *event, MDS_Err_t err));
extern void MDS_HOOK_EVENT_HAS_SET_Register(void (*hook)(MDS_Event_t *event, MDS_Mask_t mask));
extern void MDS_HOOK_EVENT_HAS_CLR_Register(void (*hook)(MDS_Event_t *event, MDS_Mask_t mask));

extern void MDS_HOOK_MSGQUEUE_TRY_RECV_Register(void (*hook)(MDS_MsgQueue_t *msgQueue, MDS_Tick_t timeout));
extern void MDS_HOOK_MSGQUEUE_HAS_RECV_Register(void (*hook)(MDS_MsgQueue_t *msgQueue, MDS_Err_t err));
extern void MDS_HOOK_MSGQUEUE_TRY_SEND_Register(void (*hook)(MDS_MsgQueue_t *msgQueue, MDS_Tick_t timeout));
extern void MDS_HOOK_MSGQUEUE_HAS_SEND_Register(void (*hook)(MDS_MsgQueue_t *msgQueue, MDS_Err_t err));

extern void MDS_HOOK_MEMPOOL_TRY_ALLOC_Register(void (*hook)(MDS_MemPool_t *memPool, MDS_Tick_t timeout));
extern void MDS_HOOK_MEMPOOL_HAS_ALLOC_Register(void (*hook)(MDS_MemPool_t *memPool, void *ptr));
extern void MDS_HOOK_MEMPOOL_HAS_FREE_Register(void (*hook)(MDS_MemPool_t *memPool, void *ptr));

extern void MDS_HOOK_MEMHEAP_INIT_Register(void (*hook)(MDS_MemHeap_t *memheap, void *heapBegin, void *heapLimit,
                                                        size_t metaSize));
extern void MDS_HOOK_MEMHEAP_ALLOC_Register(void (*hook)(MDS_MemHeap_t *memheap, void *ptr, size_t size));
extern void MDS_HOOK_MEMHEAP_FREE_Register(void (*hook)(MDS_MemHeap_t *memheap, void *ptr));
extern void MDS_HOOK_MEMHEAP_REALLOC_Register(void (*hook)(MDS_MemHeap_t *memheap, void *old, void *new, size_t size));

#define MDS_HOOK_INIT(type, ...)                                                                                       \
    static void (*g_hook_##type)(__VA_ARGS__) = NULL;                                                                  \
    void MDS_HOOK_##type##_Register(void (*hook)(__VA_ARGS__))                                                         \
    {                                                                                                                  \
        g_hook_##type = hook;                                                                                          \
    }

#define MDS_HOOK_CALL(type, ...)                                                                                       \
    do {                                                                                                               \
        if (g_hook_##type != NULL) {                                                                                   \
            g_hook_##type(__VA_ARGS__);                                                                                \
        }                                                                                                              \
    } while (0)

#define MDS_HOOK_REGISTER(type, ...) MDS_HOOK_##type##_Register(__VA_ARGS__)

#else
#define MDS_HOOK_INIT(type, ...)
#define MDS_HOOK_CALL(type, ...)
#define MDS_HOOK_REGISTER(type, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __MDS_SYS_H__ */
