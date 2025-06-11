#ifndef __MDS_CONFIG_H__
#define __MDS_CONFIG_H__

/* Base -------------------------------------------------------------------- */
#define CONFIG_MDS_TICK_U64             0
#define CONFIG_MDS_LIBRARY_MINIABLE     1
#define CONFIG_MDS_OBJECT_NAME_SIZE     7
#define CONFIG_MDS_CLOCK_TICK_FREQ_HZ   1000U
#define CONFIG_MDS_CORE_BACKTRACE_DEPTH 16
#define CONFIG_MDS_INIT_SECTION         ".init.mdsInit."

/* Log --------------------------------------------------------------------- */
#define CONFIG_MDS_LOG_ENABLE         1
#define CONFIG_MDS_LOG_BUILD_LEVEL    MDS_LOG_LEVEL_INF
#define CONFIG_MDS_LOG_FILTER_ENABLE  0
#define CONFIG_MDS_ASSERT_ENABLE      1
#define CONFIG_MDS_LOG_FORMAT_SECTION ".logfmt."

/* Kernel ------------------------------------------------------------------ */
#define CONFIG_MDS_KERNEL_LOG_LEVEL           MDS_LOG_LEVEL_WRN
#define CONFIG_MDS_KERNEL_SMP_CPUS            1
#define CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX 32

#define CONFIG_MDS_SYSMEM_HEAP_OPS G_MDS_MEMHEAP_OPS_LLFF

#define CONFIG_MDS_TIMER_SKIPLIST_LEVEL   1
#define CONFIG_MDS_TIMER_SKIPLIST_SHIFT   2

#define CONFIG_MDS_TIMER_INDEPENDENT      1
#define CONFIG_MDS_TIMER_THREAD_PRIORITY  0
#define CONFIG_MDS_TIMER_THREAD_STACKSIZE 256
#define CONFIG_MDS_TIMER_THREAD_TICKS     16

#define CONFIG_MDS_IDLE_THREAD_STACKSIZE 384
#define CONFIG_MDS_IDLE_THREAD_TICKS     16
#define CONFIG_MDS_IDLE_THREAD_HOOKS     0

#endif /* __MDS_CONFIG_H__ */
