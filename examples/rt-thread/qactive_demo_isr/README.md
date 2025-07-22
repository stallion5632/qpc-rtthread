
# QP/C RT-Thread ISR-Safe Event Publishing

## 概述 (Overview)

本工程演示了QP/C在RT-Thread下的ISR安全事件投递机制。对象投递事件。

### 运行日志示例


```
=== QActive ISR Demo Auto-Initialize ===
QActive ISR Demo: Started - 4 QActive objects
[System] Starting QF ISR demo application
[System] System startup completed
msh />[I/SDIO] SD card capacity 65536 KB.
[AO] SensorAO TIMEOUT_SIG, trigger ISR relay for SENSOR_DATA_SIG=216
[ISR] QF_postFromISR AO_Sensor SENSOR_DATA_SIG, value=216
[AO] MonitorAO MONITOR_TIMEOUT_SIG, trigger ISR relay for MONITOR_CHECK_SIG
[ISR] QF_postFromISR AO_Monitor MONITOR_CHECK_SIG
...（省略）...
```

日志中 [AO] 表示活动对象（Active Object）内部事件处理， [ISR] 表示通过ISR relay机制从中断上下文安全投递事件到指定活动对象。

### QF_postFromISR 的源码实现原理如下：

- QF_postFromISR 不是直接将事件投递到目标 AO 的消息队列，而是根据策略（优先级）将事件放入一个 staging buffer（分级缓冲区）。
- staging buffer 是环形队列，分为高/中/低优先级，事件和目标 AO 一起存储。
- 事件入 staging buffer 时，如果是动态事件会增加引用计数，防止提前释放。
- 每次有事件入 staging buffer，会通过信号量唤醒 dispatcher 线程。
- dispatcher 线程批量从 staging buffer 取出事件，按策略（可合并、丢弃、重试等）分发到目标 AO 的消息队列。
- 如果 AO 队列满，且事件支持重试（如带 NO_DROP 标志），事件会被重新放回低优先级 staging buffer，重试次数超限后丢弃。
- 所有事件流转、丢弃、重试等均有详细统计。

### QF_postFromISR 原理说明

- **事件分级缓冲**：QF_postFromISR 并不直接将事件投递到目标 AO，而是先根据事件优先级策略放入 staging buffer（分级环形缓冲区），支持高/中/低优先级。
- **批量中继线程**：专用 dispatcher 线程被信号量唤醒后，批量从 staging buffer 取出事件，按策略（合并、丢弃、重试）分发到目标 AO。
- **动态事件安全**：动态事件入 staging buffer 时自动增加引用计数，确保事件生命周期安全。
- **丢弃与重试机制**：如 AO 队列满，支持带 NO_DROP 标志的事件自动重试，重试超限后才丢弃，普通事件可按策略直接丢弃。
- **可插拔策略**：事件优先级、合并、丢弃等行为可通过策略接口自定义，适应不同实时场景。
- **详细统计**：系统自动统计事件处理、丢弃、重试、缓冲区溢出等信息，便于性能分析和调优。

### 在中断中使用是安全的
`QF_postFromISR` 专为中断上下文设计，安全性体现在：

- **操作极简**：在中断中只是将事件和目标 AO 指针写入 staging buffer（环形缓冲区），不涉及复杂逻辑和阻塞操作。
- **原子性**：入队操作通常用原子操作或临界区保护，避免数据竞争。
- **快速返回**：不会在中断中分配内存、等待信号量或执行耗时操作，保证中断响应时间。
- **事件分发异步**：事件实际处理由 dispatcher 线程在任务上下文完成，中断只负责“投递”。
