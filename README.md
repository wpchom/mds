# MDS
MCU Develop System

---

## 内核
可选择性配置启用调度器内核

### 支持架构
- ARM Cortex-M
- RISC-V

### 任务管理
适配32bit架构的MCU，支持0-31优先级、事件片轮转，基于MLFQ的抢占式任务调度。

### 内存管理
支持对接多种内存管理算法，当前默认使用LLFF算法。

### 软定时器
可配置级数的跳表式定时器管理算法，

### 线程同步
- 信号量
- 完成量
- 互斥锁
- 读写锁
- 事件集
- 消息队列
- 内存池

### 日志记录
可对接串口、JLinkRTT、文件、Flash等接口，支持压缩格式输出。

---

## 设备
抽象设备层，屏蔽驱动差异，器件对象化操作，支持Fake驱动、远程驱动对接。

---

## 驱动
- 芯片
- 外设器件
- 模拟接口
  - LED
  - I2C
  - SPI

---

## 组件
- 按键
支持按键组，支持长按、短按、双击、三击等操作。

- 低功耗


- 文件系统
适合MCU的轻量化文件系统，支持掉电保护、擦写均衡。

- SystemView
对接SEGGER的SystemView

- 消息订阅

---

## 构建
[MDS_Build](https://github.com/wpchom/mds_build)基于Python+Gn+Ninja的包构建系统，支持跨平台的交叉编译环境。

[MDS_Demo](https://github.com/wpchom/mds_demo)基于MDS的示例程序，适配STM32F103、STM32H750等主流MCU的示例项目，支持Gtest测试框架。


---

## 待完善
- [ ] 文件系统
  - [ ] VFS
  - [ ] LittleFs
  - [ ] FatFs
- [ ] 芯片驱动+配套Demo
  - [ ] GD32VF103
  - [ ] STM32F103
  - [ ] STM32H750
- [ ] 外设驱动
  - [ ] SFUD兼容
  - [ ] LCD
- [ ] GUI
  - [ ] LVGL
