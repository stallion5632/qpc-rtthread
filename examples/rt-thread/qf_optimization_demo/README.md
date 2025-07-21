# QP/C RT-Thread Optimization Features Demo

## 概述

这个演示程序全面展示了QP/C与RT-Thread集成的优化功能，包括：

1. **心跳和看门狗机制**: 展示超时等待和周期性心跳处理
2. **RT-Thread内存池集成**: 演示不同大小事件的内存池自动选择
3. **优化层功能**: 展示调度器优化和性能监控
4. **诊断和监控**: 展示实时系统状态监控

## 架构设计

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  DataProducer   │────│  DataConsumer   │    │  SystemMonitor  │
│     AO          │    │      AO         │    │      AO         │
│                 │    │                 │    │                 │
│ - 生成不同大小   │    │ - 处理数据事件   │    │ - 监控系统状态   │
│   的事件        │    │ - 心跳机制测试   │    │ - 内存池统计     │
│ - 内存池测试    │    │ - 超时处理      │    │ - 性能指标      │
│ - 压力测试      │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## 主要功能

### DataProducer AO
- **内存池测试**: 循环产生小、中、大三种不同大小的事件
- **自动池选择**: 验证RT-Thread内存池自动选择机制
- **压力测试**: 快速分配/释放内存以测试池性能
- **失败统计**: 记录内存分配失败次数

### DataConsumer AO
- **心跳机制**: 演示超时等待和心跳处理
- **数据处理**: 接收和处理不同类型的数据事件
- **状态转换**: 空闲↔处理状态转换
- **性能监控**: 统计处理次数和空闲时间

### SystemMonitor AO
- **系统监控**: 周期性输出系统状态
- **内存池统计**: 显示各内存池使用情况
- **优化层指标**: 显示调度器性能指标
- **诊断信息**: 综合系统健康状况

## 测试的API功能

### 1. RT-Thread内存池API
```c
QF_poolInit_RT()           // 初始化内存池
QF_newX_RT()               // 分配事件内存
QF_gc_RT()                 // 释放事件内存
QF_poolGetStat_RT()        // 获取池统计
QF_poolPrintStats_RT()     // 打印池状态
```

### 2. 优化层API
```c
QF_getDispatcherMetrics()  // 获取调度器指标
QF_resetDispatcherMetrics() // 重置指标
QF_enableOptLayer()        // 启用优化层
QF_disableOptLayer()       // 禁用优化层
```

### 3. 诊断API
```c
// Shell命令形式验证
qf_metrics                 // 显示调度器指标
qf_mempool                 // 显示内存池状态
qf_aos                     // 显示AO状态
```

### 4. 心跳机制
- 超时等待验证: `rt_mb_recv()` 带超时
- 看门狗集成: `feed_watchdog()` 调用
- 心跳统计: `perf_heartbeat()` 调用

## 使用方法

### 1. 启动演示
```bash
msh> qf_demo_start
```

### 2. 观察输出
演示会自动运行，输出包括：
- DataProducer产生的事件信息
- DataConsumer的处理和心跳信息
- SystemMonitor的周期性状态报告

### 3. 运行压力测试
```bash
msh> qf_demo_stress
```

### 4. 查看统计信息
```bash
# 查看内存池状态
msh> qf_mempool

# 查看调度器指标
msh> qf_metrics

# 查看AO状态
msh> qf_aos
```

### 5. 停止演示
```bash
msh> qf_demo_stop
```

## 预期输出示例

```
==== QP/C RT-Thread Optimization Demo ====
Framework initialized
[DataProducer] Initialized
[DataConsumer] Initialized and subscribed to DATA_SIG
[SystemMonitor] Initialized
All Active Objects started
===========================================

[DataProducer] Small event #3 published
[DataConsumer] Received data, processing...
[DataConsumer] Processing data item #1
[DataConsumer] Processing completed

[SystemMonitor] === Cycle #1 ===
[SystemMonitor] Memory pool status:
==== QF RT-Thread Memory Pool Statistics ====
Total allocations: 15
Total deallocations: 14
Total failures: 0
Outstanding events: 1
Pool 0 (qf_small): Free=31, Used=1, Peak=3/32
Pool 1 (qf_medium): Free=16, Used=0, Peak=5/16
Pool 2 (qf_large): Free=8, Used=0, Peak=2/8
=============================================
```

## 验证要点

1. **内存池功能**: 检查不同大小事件是否使用正确的内存池
2. **心跳机制**: 观察Consumer的心跳输出和超时处理
3. **压力测试**: 验证快速分配/释放时的性能和稳定性
4. **统计准确性**: 检查内存池统计数据是否正确
5. **Shell命令**: 验证所有诊断命令是否正常工作

这个演示全面验证了所有新增的优化功能，证明它们能够正确工作并提供预期的性能提升和监控能力。
