# QPC-RT-Thread 自动化性能测试套件

本目录包含运行在 RT-Thread RTOS 上的 QPC (Quantum Platform for C) 框架的全面自动化性能测试套件。测试套件解决了关键的性能测量问题，实现了线程安全、内存管理和测量精度的最佳实践。

## 核心特性

### 1. 自动化测试管理器 (`perf_test_manage.c`)
- **自动序列执行**: 5个测试用例按顺序自动运行，无需手动干预
- **统一初始化**: QF_init 和所有事件池只在管理器中初始化一次，避免重复导致的断言失败
- **定时控制**: 基于 RT-Thread 定时器的精确测试切换
- **Shell 命令集成**: 提供完整的命令行控制接口

### 2. QPC 状态机规范化
- **问题**: `(qep_hsm) assertion failed at function:, line number:220` 断言失败
- **解决方案**:
  - 修正所有 AO 初始状态函数，使用正确的 `void const * const par` 参数
  - 移除冗余的信号处理逻辑，简化状态机结构
  - 订阅操作在初始状态中完成，直接转换到运行状态

### 3. 框架初始化防重复机制
- **问题**: 多次调用 QF_init 导致状态机断言失败
- **解决方案**:
  - 集中式初始化：所有 QF_init 和事件池初始化在 `PerfTestManager_init` 中完成
  - 各测试启动函数仅负责 AO 构造和启动
  - 防重复机制确保初始化只执行一次

### 4. 独特信号定义防冲突
- **解决方案**: 为每种测试类型定义独立的 `Q_USER_SIG` 偏移:
  - `LATENCY_*_SIG`: Q_USER_SIG + 0-9
  - `THROUGHPUT_*_SIG`: Q_USER_SIG + 10-19
  - `JITTER_*_SIG`: Q_USER_SIG + 20-29
  - `IDLE_CPU_*_SIG`: Q_USER_SIG + 30-39
  - `MEMORY_*_SIG`: Q_USER_SIG + 40-49
  - `ISR_*_SIG`: Q_USER_SIG + 80-89

### 5. 安全事件时间戳处理
- **问题**: 误用 `poolId_` 字段存储时间戳导致内存损坏
- **解决方案**:
  - 自定义事件结构体 (`LatencyEvt`, `ThroughputEvt`, 等) 包含专用 `timestamp` 字段
  - 正确的 DWT (Data Watchpoint and Trace) 周期计数器初始化和重置
  - 每个测试开始时的安全时间戳管理

### 6. 活动对象清理和唯一优先级
- **问题**: 优先级冲突和测试间残留订阅
- **解决方案**:
  - 为每个测试 AO 分配唯一的 QP 优先级 (1-9)
  - 测试完成时正确取消信号订阅
  - 清洁的 AO 终止防止资源泄漏

### 7. 线程同步和终止
- **问题**: 定时器回调中不安全的线程删除导致崩溃
- **解决方案**:
  - `volatile` 标志 (`g_stopProducer`, `g_stopLoadThreads`) 安全的跨线程信号
  - 线程删除移到主测试函数中设置停止标志后
  - 线程删除前适当延迟 (`PerfCommon_waitForThreads()`)
  - 清洁的线程退出处理

## 测试类型

### 1. 延迟测试 (`latency_test.c`)
使用带专用时间戳字段的自定义 `LatencyEvt` 结构测量事件处理延迟。测试从发布到处理的事件往返时间。

### 2. 吞吐量测试 (`throughput_test.c`)
使用生产者-消费者模式和适当的线程同步测量系统吞吐量。测试负载下的事件处理速率。

### 3. 抖动测试 (`jitter_test.c`)
使用负载线程引入实际系统压力测量周期事件中的时序抖动。测试变化负载条件下的时序一致性。

### 4. 空闲 CPU 测试 (`idle_cpu_test.c`)
使用适当的空闲钩子机制测量 CPU 空闲时间和利用率。测试系统效率和资源使用。

### 5. 内存测试 (`memory_test.c`)
使用适当的跟踪和清理测量内存分配/释放性能。测试内存管理效率和泄漏检测。

### 6. ISR 发布测试 (`isr_publishing_test.c`)
测试 `QF_postFromISR` 和 `QF_publishFromISR` 路径性能，验证中断上下文的事件处理。

## 架构

### 自动化测试管理器 (`perf_test_manage.c`)
- **统一控制**: 协调所有测试的启动、停止和切换
- **定时管理**: 基于 RT-Thread 定时器的精确时序控制
- **状态跟踪**: 跟踪当前运行的测试和整体进度
- **Shell 集成**: 完整的命令行接口

### 公共基础设施 (`perf_common.c/h`)
- DWT 周期计数器管理
- 事件池初始化和清理
- 线程同步工具
- 内存管理包装器
- 共享 volatile 变量

## 使用方法

### 自动化测试套件 (推荐)
```c
// 初始化测试管理器 (仅需调用一次)
PerfTestManager_init();

// 启动自动化测试序列 - 所有5个测试将自动运行
PerfTestManager_startAll();

// 停止所有运行的测试
PerfTestManager_stopAll();

// 获取测试信息
PerfTestManager_info();
```

### RT-Thread MSH 命令 (推荐使用方式)
```bash
# 启动完整自动化测试套件 (5个测试，每个5秒，总计25秒)
perf_start_all

# 停止所有测试
perf_stop_all

# 获取测试套件信息
perf_info

# 手动运行单个测试 (如需要)
latency_start
throughput_start
jitter_start
idle_cpu_start
memory_start
```

### 单独测试控制 (高级用户)
```c
// 先初始化框架 (必须)
PerfTestManager_init();

// 然后运行单个测试
LatencyTest_start();
ThroughputTest_start();
JitterTest_start();
IdleCpuTest_start();
MemoryTest_start();
ISRPublishingTest_start();
```

## 构建集成

将性能测试添加到你的 RT-Thread 项目:

1. 在构建路径中包含 `apps/performance_tests/` 目录
2. 将源文件添加到你的 SConscript 或 Makefile
3. 启用 RT-Thread FINSH/MSH 命令行界面
4. 确保 QPC 框架正确集成
5. 在 `main.c` 中调用:
   ```c
   int main(void) {
       // RT-Thread 初始化...

       // 初始化性能测试管理器
       PerfTestManager_init();

       // 可选：启动自动化测试
       // PerfTestManager_startAll();

       return 0;
   }
   ```

## 关键修复和改进

### 1. QPC 状态机断言修复
根据 `examples\rt-thread\QPC_RTThread_Tech_Summary.md` 指导：
- 修正初始状态函数签名：`void const * const par` 而非 `QEvt const * const e`
- 简化状态处理：初始状态直接转换，无需复杂信号处理
- 避免重复 QF_init：集中在管理器中初始化一次

### 2. 防重复初始化机制
```c
void PerfTestManager_init(void) {
    if (s_framework_initialized != 0U) {
        return; // 防止重复初始化
    }

    QF_init(); // 只初始化一次
    // ... 事件池初始化

    s_framework_initialized = 1U;
}
```

### 3. 自动化测试序列
- 定时器驱动的自动切换机制
- 每个测试运行5秒，总计25秒完成
- 无需人工干预，结果自动报告

## 安全特性

- **内存安全**: 自定义事件结构防止 `poolId_` 误用
- **线程安全**: Volatile 标志用于安全的跨线程通信
- **资源清理**: 正确的取消订阅和线程终止
- **隔离性**: 唯一优先级和信号范围防止跨测试干扰
- **时序安全**: DWT 周期计数器初始化和重置确保准确测量
- **状态机安全**: 正确的 QPC 初始状态函数实现，避免断言失败

## 实现的最佳实践

1. **最小变更**: 针对特定问题的集中改进
2. **线程安全**: 正确的同步和终止模式
3. **内存安全**: 安全的事件处理和分配跟踪
4. **测量精度**: 正确的时序基础设施和信号隔离
5. **资源管理**: 清洁的启动/关闭过程
6. **错误处理**: 优雅的故障处理和恢复
7. **QPC 规范**: 遵循 QPC 框架的状态机编程最佳实践

## 测试结果

每个测试默认运行5秒并提供详细测量：
- **延迟测试**: 最小/最大/平均延迟 (周期数)
- **吞吐量测试**: 每周期包数，总处理数据量
- **抖动测试**: 时序变化统计
- **空闲 CPU**: CPU 利用率和空闲时间
- **内存测试**: 分配模式和效率
- **ISR 测试**: 中断路径性能和验证

## 故障排除

### QPC 状态机断言失败
```
(qep_hsm) assertion failed at function:, line number:220
```
**解决**: 已修复所有初始状态函数，使用正确的函数签名和实现模式。

### 重复初始化断言
```
(qf_dyn) assertion failed at function:, line number:201
```
**解决**: 集中式初始化机制确保 QF_init 和事件池只初始化一次。

### 测试间干扰
**解决**: 独特的信号定义和优先级分配，清洁的资源管理。

## 兼容性

- **QPC 框架**: 7.2.0+
- **RT-Thread**: 4.0+
- **ARM Cortex-M**: 支持 DWT
- **构建系统**: SConscript, Makefile, IAR, Keil

这个自动化性能测试套件为 QPC-RT-Thread 性能分析提供了坚实的基础，同时保持线程安全、内存安全和测量精度。通过修复 QPC 状态机问题和实现自动化管理，现在可以可靠地运行完整的性能评估。
