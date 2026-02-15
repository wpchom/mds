# MDS

一个自用的MCU开发框架，采用`gn`构建，详情可查看`BUILD.gn`中的配置项。

## 目录
```
./
├── kernel/          # RTOS内核
├── device/          # 设备抽象
├── components/      # 组件
├── include/         # 头文件
└── README.md        # 说明
```

## RTOS
针对于MCU的RTOS，暂时仅根据自身需求做的功能适配；

### 内核
`kernel/arch/*`，通过gn脚本的`mds_core_arch`的字段控制，当前支持:
- ARM Cortex-M
- RISC-V

配置项：
- `CONFIG_MDS_CORE_BACKTRACE_DEPTH` backtrace回溯深度，0为不回溯

### 对象
`kernel/object.c`，内核对下述对象进行抽象化管理，在初始化或创建是进行命名。
```
typedef enum MDS_ObjectType {
    MDS_OBJECT_TYPE_NONE = 0,
    MDS_OBJECT_TYPE_DEVICE,         // 设备
    MDS_OBJECT_TYPE_THREAD,         // 线程
    MDS_OBJECT_TYPE_WORKQUEUE,      // 工作队列
    MDS_OBJECT_TYPE_WORKNODE,       // 工作节点
    MDS_OBJECT_TYPE_SEMAPHORE,      // 信号量
    MDS_OBJECT_TYPE_MUTEX,          // 互斥锁
    MDS_OBJECT_TYPE_EVENT,          // 事件
    MDS_OBJECT_TYPE_MSGQUEUE,       // 消息队列
    MDS_OBJECT_TYPE_MEMPOOL,        // 内存池
    MDS_OBJECT_TYPE_MEMHEAP,        // 内存堆
} __attribute__((packed)) MDS_ObjectType_t;
```

配置项：
- `CONFIG_MDS_OBJECT_NAME_SIZE` 控制对象名字长度，默认为7字节，因对齐设计，尽可能为7、15、23等字节。

### 内存
`kernel/mm/*`，系统的内存堆默认采用LLFF的算法管理动态内存，通过`CONFIG_MDS_SYSMEM_HEAP_OPS`控制，暂未实现TLSF；可选主动初始化，或首次申请时自动初始化，通过`void MDS_SysMemHeapAddress(void **begin, void **limit)`获取内存堆起始和结束地址。

### 日志
`kernel/log.c`，模块化日志框架，不具备打印能力，日志等级分为：
```
#define MDS_LOG_LEVEL_OFF 0
#define MDS_LOG_LEVEL_FAT 1
#define MDS_LOG_LEVEL_ERR 2
#define MDS_LOG_LEVEL_WRN 3
#define MDS_LOG_LEVEL_INF 4
#define MDS_LOG_LEVEL_DBG 5
```

配置项：
- `CONFIG_MDS_LOG_ENABLE` 控制日志功能开关
- `CONFIG_MDS_LOG_BUILD_LEVEL` 控制编译期的日志等级
- `CONFIG_MDS_LOG_FILTER_ENABLE` 动态改变日志模块等级能力
- `CONFIG_MDS_LOG_ASSERT_ENABLE` 启用断言功能
- `CONFIG_MDS_LOG_FORMAT_SECTION` 日志格式化字符串段
- `CONFIG_MDS_LOG_MSGARGS_NUMS` 日志格式化参数个数控制
- `CONFIG_MDS_KERNEL_LOG_LEVEL` 内核日志等级，默认WRN

### 时钟
`kernel/sys/clock.c`，使用`MDS_Tick_t MDS_ClockGetTickCount(void)`获取tick数，默认1ms为1tick

配置项：
- `CONFIG_MDS_CLOCK_TICK_FREQ_HZ` tick频率

### 调度器
`kernel/sys/scheduler.c`，当前仅支持MLFQ的32级

配置项：
- `CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX` 线程最大优先级，默认32，即0～31等级。

### 定时器
`kernel/sys/timer.c`，支持独立定时器线程，其实现与工作队列`workq.c`一致，通过跳表或者时间轮实现。

配置项：
- `CONFIG_MDS_TIMER_INDEPENDENT` 使能独立定时器线程
- `CONFIG_MDS_TIMER_SKIPLIST_LEVEL` 时间跳表等级，=0则使用时间轮
- `CONFIG_MDS_TIMER_SKIPLIST_SHIFT` 时间跳表位移，开启跳表时有效
- `CONFIG_MDS_TIMER_WHEEL_TABLE` 时间轮等级表，关闭跳表时有效
- `CONFIG_MDS_TIMER_THREAD_PRIORITY` 独立定时器线程优先级，默认0为最高优先级
- `CONFIG_MDS_TIMER_THREAD_STACKSIZE` 独立定时器线程栈大小
- `CONFIG_MDS_TIMER_THREAD_TICKS` 定时器线程调度时间片

### 空闲线程
`kernel/sys/idle.c`，空闲线程，强制最低优先级，不可配置。

配置项：
- `CONFIG_MDS_IDLE_THREAD_STACKSIZE` 空闲线程栈大小
- `CONFIG_MDS_IDLE_THREAD_TICKS` 空闲线程调度时间片
- `CONFIG_MDS_IDLE_THREAD_HOOKS` 空闲线程钩子数
