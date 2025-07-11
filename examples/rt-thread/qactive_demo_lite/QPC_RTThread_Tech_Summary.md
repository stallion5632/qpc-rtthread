# QPC + RT-Thread Event Pool & AO Initialization Technical Summary

## 1. 问题背景

在 RT-Thread 平台下移植 QPC (Quantum Platform for C) 框架，应用包含多个 QActive 活动对象（Sensor、Processor、Worker、Monitor），并使用 QF 动态事件池进行事件分配。系统启动时出现断言失败，导致无法正常运行。

## 2. 异常现象

### 2.1 QF_poolInit 断言失败
```
(qf_dyn) assertion failed at function:, line number:201
```
- 位置：`QF_poolInit` (qf_dyn.c, line 201)
- 触发时机：事件池初始化阶段

### 2.2 RT-Thread 对象初始化断言失败
```
(obj != object) assertion failed at function:rt_object_init, line number:358
```
- 位置：`rt_object_init` (RT-Thread 内核)
- 触发时机：QActive 对象重复注册时

## 3. 原因分析

### 3.1 QF_poolInit 断言失败原因
- QPC 框架要求事件池必须按事件大小递增初始化。
- 同样大小的事件只能用一个池，不能为每个事件类型单独建池。
- 原代码为每个 8 字节事件（SensorDataEvt、ProcessorResultEvt、WorkerWorkEvt）都建了独立池，导致断言失败。

### 3.2 RT-Thread 对象初始化断言失败原因
- QActiveDemo_init() 和 qactive_demo_start() 被多次调用，导致 QActive 对象和 RT-Thread 资源重复注册。
- RT-Thread 检测到对象重复，触发断言。

## 4. 解决过程

### 4.1 事件池设计优化
- 只保留一个 8 字节事件池 shared8Pool，供所有 8 字节事件共用。
- pool 命名和注释优化，明确所有 8 字节事件共用同一个池。
- 事件池初始化顺序严格递增：QEvt（4字节）→ shared8Pool（8字节）。

### 4.2 防止重复初始化
- QActiveDemo_init() 增加静态标志 isInitialized，确保只初始化一次。
- qactive_demo_start() 增加静态标志 isStarted，确保只启动一次。
- main() 和自动初始化流程只会有一次有效启动。

### 4.3 代码风格与注释规范
- 所有事件池、队列、栈等静态分配内存均加 ALIGN(RT_ALIGN_SIZE)，保证平台兼容性。
- 注释全部采用 MISRA C 风格英文，便于团队协作和代码规范化。

## 5. 关键代码片段

```c
/* Event pools must be initialized in ascending order of event size. */
/* Only one pool is allowed for events of the same size. */
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(QEvt) basicEventPool[20];              /* 4-byte event pool for QEvt only */
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(SensorDataEvt) shared8Pool[30];        /* 8-byte event pool shared by SensorDataEvt, ProcessorResultEvt, WorkerWorkEvt */

void QActiveDemo_init(void) {
    static uint8_t isInitialized = 0;
    if (isInitialized != 0) {
        printf("QActive Demo: Already initialized, skipping...\n");
        return;
    }
    isInitialized = 1;
    // ...existing code...
}

int qactive_demo_start(void) {
    static uint8_t isStarted = 0;
    if (isStarted != 0) {
        printf("QActive Demo: Already started, skipping...\n");
        return 0;
    }
    isStarted = 1;
    // ...existing code...
}
```

## 6. 总结与建议

- QPC 框架事件池必须按大小递增初始化，同大小事件只能用一个池。
- 结构体设计时建议让事件大小一致，提升池复用率和系统效率。
- 静态分配内存建议加 ALIGN(RT_ALIGN_SIZE)，保证平台兼容。
- 代码注释建议采用英文 MISRA C 风格，便于团队协作。
- 初始化流程需防止重复注册，避免 RT-Thread 对象断言失败。

## 7. 参考文档
- QPC 官方文档：https://www.state-machine.com/qpc
- RT-Thread 官方文档：https://www.rt-thread.io/
- MISRA C 注释规范：https://www.misra.org.uk/
