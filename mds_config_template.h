#ifndef __MDS_CONFIG_H__
#define __MDS_CONFIG_H__

/* Tick */
#ifndef CONFIG_MDS_TICK_U64
#define CONFIG_MDS_TICK_U64 0
#endif

/* Object */
#ifndef CONFIG_MDS_OBJECT_NAME_SIZE
#define CONFIG_MDS_OBJECT_NAME_SIZE 7
#endif

/* Init */
#ifndef CONFIG_MDS_INIT_SECTION
#define CONFIG_MDS_INIT_SECTION ".init.mds."
#endif

/* Sysmem */
#ifndef CONFIG_MDS_SYSMEM_HEAP_SECTION
#define CONFIG_MDS_SYSMEM_HEAP_SECTION ".noinit.sysheap."
#endif

#ifndef CONFIG_MDS_SYSMEM_HEAP_SIZE
#define CONFIG_MDS_SYSMEM_HEAP_SIZE 0
#endif

#if (defined(CONFIG_MDS_SYSMEM_HEAP_SIZE) && (CONFIG_MDS_SYSMEM_HEAP_SIZE > 0))
#ifndef CONFIG_MDS_SYSMEM_HEAP_OPS
#define CONFIG_MDS_SYSMEM_HEAP_OPS MDS_MEMHEAP_OPS_LLFF
#endif
#endif

/* Clock */
#ifndef CONFIG_MDS_CLOCK_TICK_FREQ_HZ
#define CONFIG_MDS_CLOCK_TICK_FREQ_HZ 1000U
#endif

/* Arch */
#ifndef CONFIG_MDS_BACKTRACE_DEPTH
#define CONFIG_MDS_BACKTRACE_DEPTH 16
#endif

/* Kernel Smp cpus */
#ifndef CONFIG_MDS_KERNEL_SMP_CPUS
#define CONFIG_MDS_KERNEL_SMP_CPUS 1
#endif

/* Kernel thread priority */
#ifndef CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX
#define CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX 32
#endif

/* Kernel timer */
#ifndef CONFIG_MDS_TIMER_INDEPENDENT
#define CONFIG_MDS_TIMER_INDEPENDENT 0
#endif

#ifndef CONFIG_MDS_TIMER_SKIPLIST_LEVEL
#define CONFIG_MDS_TIMER_SKIPLIST_LEVEL 1
#endif

#ifndef CONFIG_MDS_TIMER_SKIPLIST_SHIFT
#define CONFIG_MDS_TIMER_SKIPLIST_SHIFT 2
#endif

#if (CONFIG_MDS_TIMER_SKIPLIST_LEVEL == 0)
#ifndef CONFIG_MDS_TIMER_WHEEL_TABLE
#define CONFIG_MDS_TIMER_WHEEL_TABLE 6, 5, 4
#endif
#endif

#define CONFIG_MDS_TIMER_THREAD_PRIORITY  0
#define CONFIG_MDS_TIMER_THREAD_STACKSIZE 512
#define CONFIG_MDS_TIMER_THREAD_TICKS     16

/* Kernel idle */
#define CONFIG_MDS_IDLE_THREAD_STACKSIZE 512
#define CONFIG_MDS_IDLE_THREAD_TICKS     16
#define CONFIG_MDS_IDLE_THREAD_HOOKS     0

/* Kernel log */
#ifndef CONFIG_MDS_KERNEL_LOG_LEVEL
#define CONFIG_MDS_KERNEL_LOG_LEVEL MDS_LOG_LEVEL_WRN
#endif

#ifndef CONFIG_MDS_KERNEL_STATS_ENABLE
#define CONFIG_MDS_KERNEL_STATS_ENABLE 0
#endif

#ifndef CONFIG_MDS_HOOK_ENABLE_KERNEL
#define CONFIG_MDS_HOOK_ENABLE_KERNEL 0
#endif

#ifndef CONFIG_MDS_LOG_MSGQUEUE_NUMS
#define CONFIG_MDS_LOG_MSGQUEUE_NUMS 0
#endif

#ifndef CONFIG_MDS_LOG_THREAD_STACKSIZE
#define CONFIG_MDS_LOG_THREAD_STACKSIZE 512
#endif

#ifndef CONFIG_MDS_LOG_THREAD_PRIORITY
#define CONFIG_MDS_LOG_THREAD_PRIORITY (CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX - 1)
#endif

#ifndef CONFIG_MDS_LOG_THREAD_TICKS
#define CONFIG_MDS_LOG_THREAD_TICKS 32
#endif

/* Log */
#ifndef CONFIG_MDS_LOG_ENABLE
#define CONFIG_MDS_LOG_ENABLE 1
#endif

#ifndef CONFIG_MDS_LOG_ASSERT_ENABLE
#define CONFIG_MDS_LOG_ASSERT_ENABLE 0
#endif

#ifndef CONFIG_MDS_LOG_FILTER_ENABLE
#define CONFIG_MDS_LOG_FILTER_ENABLE 0
#endif

#ifndef CONFIG_MDS_LOG_BUILD_LEVEL
#define CONFIG_MDS_LOG_BUILD_LEVEL MDS_LOG_LEVEL_INF
#endif

#ifndef CONFIG_MDS_LOG_FORMAT_SECTION
#define CONFIG_MDS_LOG_FORMAT_SECTION ".logstr."
#endif

#ifndef CONFIG_MDS_LOG_MSGARGS_NUMS
#define CONFIG_MDS_LOG_MSGARGS_NUMS 7
#endif

/* Lpc */
#ifndef CONFIG_MDS_LPC_THREAD_IDLE
#define CONFIG_MDS_LPC_THREAD_IDLE 0
#endif

#ifndef CONFIG_MDS_LPC_VOTE_TYPE
#define CONFIG_MDS_LPC_VOTE_TYPE uint8_t
#endif

#ifndef CONFIG_MDS_LPC_LOG_LEVEL
#define CONFIG_MDS_LPC_LOG_LEVEL MDS_LOG_LEVEL_WRN
#endif

#ifndef CONFIG_MDS_LPC_LIST_OF_SLEEP
#define CONFIG_MDS_LPC_LIST_OF_SLEEP                                                               \
    MDS_LPC_SLEEP(LIGHT)                                                                           \
    MDS_LPC_SLEEP(DEEP)                                                                            \
    MDS_LPC_SLEEP(RESET)                                                                           \
    MDS_LPC_SLEEP(SHUTDOWN)
#endif

#ifndef CONFIG_MDS_LPC_LIST_OF_RUN
#define CONFIG_MDS_LPC_LIST_OF_RUN                                                                 \
    MDS_LPC_RUN(LOW)                                                                               \
    MDS_LPC_RUN(NORMAL)                                                                            \
    MDS_LPC_RUN(HIGH)
#endif

#endif
