
# QP/C RT-Thread ISR-Safe Event Publishing

## 概述 (Overview)

本工程演示了QP/C在RT-Thread下的ISR安全事件发布机制。通过双缓冲区和高优先级中继线程，确保从中断服务例程（ISR）安全、可靠地发布事件到QP/C框架。

### 运行日志示例

```
=== QActive ISR Demo Auto-Initialize ===
[ISR] QF_isrRelayInit: Initializing ISR relay system
QActive ISR Demo: Started - 4 QActive objects
[System] Starting QF ISR demo application
[System] System startup completed
[AO] SensorAO TIMEOUT_SIG, trigger ISR relay for SENSOR_DATA_SIG=219
[ISR] QF_PUBLISH_FROM_ISR(SENSOR_DATA_SIG, 219)
[AO] MonitorAO MONITOR_TIMEOUT_SIG, trigger ISR relay for MONITOR_CHECK_SIG
[ISR] QF_PUBLISH_FROM_ISR(MONITOR_CHECK_SIG)
...（省略）...
```

日志中 [AO] 表示活动对象（Active Object）内部事件处理， [ISR] 表示通过ISR relay机制从中断上下文安全发布事件。


## 主要特性 (Key Features)

- **ISR relay双缓冲机制**：主缓冲区+溢出缓冲区，防止高峰丢失。
- **高优先级中继线程**：批量处理事件，低延迟，自动自适应批量大小。
- **详细统计信息**：事件处理数、丢失数、最大批量、最大处理时长、缓冲区使用率。
- **日志可追溯**：每次ISR事件发布和AO处理均有详细日志，便于调试和验证。

## 配置 (Configuration)

### 启用ISR中继功能

在 `qf_rtmpool_config.h` 中设置：

```c
#define QF_ENABLE_ISR_RELAY    1
```

### 可配置参数

在 `qf_isr_relay.h` 中调整以下参数：

```c
#define QF_ISR_MAIN_BUFFER_SIZE      64   /* 主缓冲区事件数量 */
#define QF_ISR_OVERFLOW_BUFFER_SIZE  16   /* 溢出缓冲区事件数量 */
#define QF_ISR_RELAY_THREAD_PRIO     3    /* 中继线程优先级 (高优先级) */
#define QF_ISR_RELAY_STACK_SIZE      1024 /* 中继线程栈大小 */
```


## 快速上手 (Quick Start)

1. **系统初始化**
   - 只需调用 `QF_init()` 和 `QF_run()`，ISR relay系统会自动初始化和启动。
   - 日志会显示 `[ISR] QF_isrRelayInit: Initializing ISR relay system`。

2. **事件发布与日志追踪**
   - 活动对象（如SensorAO）定时触发事件，通过 `isr_publish_xxx` 函数调用 `QF_PUBLISH_FROM_ISR`。
   - 日志如：
     - `[AO] SensorAO TIMEOUT_SIG, trigger ISR relay for SENSOR_DATA_SIG=219`
     - `[ISR] QF_PUBLISH_FROM_ISR(SENSOR_DATA_SIG, 219)`
   - 你可以在shell下直接看到事件流转和ISR relay机制。

3. **统计信息监控**
   - 调用 `QF_isrRelayPrintStats()` 可打印详细统计。
   - 也可通过 `QF_isrRelayGetStats()` 获取结构体进行程序化分析。


## 性能调优与调试建议 (Performance Tuning & Debugging)

- **缓冲区大小**：根据日志中 [ISR] 丢失统计和事件流量，调整 `QF_ISR_MAIN_BUFFER_SIZE` 和 `QF_ISR_OVERFLOW_BUFFER_SIZE`。
- **中继线程优先级**：确保 `QF_ISR_RELAY_THREAD_PRIO` 足够高，避免事件堆积。
- **日志分析**：通过 [AO] 和 [ISR] 日志可直观判断事件流转、系统瓶颈和中断响应。
- **常见问题**：如日志出现 events_lost 增加，建议增大缓冲区或优化事件处理。

## 故障排除 (Troubleshooting)

### 事件丢失问题

```bash
# 检查统计信息
msh />isr_stats

# 如果events_lost > 0，考虑以下解决方案：
# 1. 增加缓冲区大小
# 2. 提高中继线程优先级
# 3. 减少每批处理的事件数量
# 4. 优化事件处理器性能
```

### 测试ISR事件发布

```bash
# 模拟ISR事件发布进行测试
msh />test_isr_event 1 100    # 发布SENSOR_DATA事件，参数100
msh />test_isr_event 2        # 发布TIMER_EXPIRED事件
msh />isr_stats               # 检查统计信息
```

## 实现细节 (Implementation Details)

### 事件结构

每个ISR事件包含以下信息：

```c
typedef struct {
    QSignal sig;        /* 信号类型 */
    rt_uint8_t poolId;  /* 事件池ID（保留） */
    rt_uint16_t param;  /* 参数（保留） */
    rt_tick_t timestamp;/* 时间戳 */
} QF_ISREvent;
```

### 线程安全保证

- **原子操作**: 使用RT-Thread的中断禁用/使能确保原子性
- **无锁设计**: 环形缓冲区操作不使用互斥锁
- **统计保护**: 统计更新使用互斥锁保护

### 内存使用

典型配置的内存占用：
- 主缓冲区: 64 × 8字节 = 512字节
- 溢出缓冲区: 16 × 8字节 = 128字节
- 控制结构: 约100字节
- 中继线程栈: 1024字节
- **总计**: 约1.8KB
