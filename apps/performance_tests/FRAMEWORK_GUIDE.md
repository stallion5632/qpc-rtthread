# Performance Tests Framework 使用指南

## 重构完成

性能测试模块已经按照新框架重构完成，主要变化：

### 1. 目录结构
```
apps/performance_tests/
├── include/
│   ├── perf_test.h          # 核心测试框架头文件
│   ├── app_main.h           # 应用程序主头文件（简化）
│   └── bsp.h                # BSP头文件（简化）
├── src/
│   ├── perf_test_core.c     # 核心调度与管理逻辑
│   ├── perf_test_cmd.c      # MSH命令绑定
│   ├── app_main.c           # 框架初始化（简化）
│   └── bsp.c                # BSP实现（简化）
└── tests/
    ├── cpu_load.c           # CPU负载测试用例
    ├── counter_ao_test.c    # Counter AO测试用例
    ├── timer_ao_test.c      # Timer AO测试用例
    ├── mem_stress_test.c    # 内存压力测试用例
    └── multithread_test.c   # 多线程性能测试用例
```

### 2. 新测试用例

#### CPU负载测试 (cpu_load)
- 简单的循环计数测试
- 测试基本的CPU性能

#### Counter AO测试 (counter_ao)
- 基于QPC Active Object的计数器测试
- 使用定时器触发事件
- 测试事件处理性能

#### Timer AO测试 (timer_ao)
- 定时器性能测试
- 定期产生报告
- 测试定时器精度和开销

#### 内存压力测试 (mem_stress)
- 动态内存分配/释放测试
- 测试内存管理性能

#### 多线程测试 (multithread)
- 创建多个工作线程
- 测试线程同步和互斥
- 测试多线程环境下的性能

### 3. 使用方法

编译完成后，在终端中执行：

```bash
# 列出所有测试用例
perf list

# 启动特定测试
perf start cpu_load
perf start counter_ao
perf start timer_ao
perf start mem_stress
perf start multithread

# 查看测试报告
perf report

# 停止测试
perf stop cpu_load

# 重启测试
perf restart timer_ao
```

### 4. 框架特性

- **开放封闭原则**: 添加新测试用例只需创建新的 .c 文件并使用 `PERF_TEST_REG` 宏注册
- **统一管理**: 所有测试用例通过统一的命令行接口管理
- **自动化**: 支持自动测试运行、结果采集和统计分析
- **可扩展**: 框架可轻松扩展支持更多测试类型

### 5. 添加新测试用例

创建新的测试用例非常简单：

```c
#include "perf_test.h"

static int my_test_init(perf_test_case_t *tc) {
    // 初始化代码
    return 0;
}

static int my_test_run(perf_test_case_t *tc) {
    // 测试主逻辑
    for (int i = 0; i < 1000; i++) {
        tc->iterations++;
        // 执行测试操作...
    }
    return 0;
}

static int my_test_stop(perf_test_case_t *tc) {
    // 清理代码
    return 0;
}

// 注册测试用例
PERF_TEST_REG(my_test, my_test_init, my_test_run, my_test_stop);
```

框架将自动发现并集成新的测试用例。

### 6. 新增专业性能测试

基于backup_old_tests重构，新增了以下专业测试用例：

#### 延迟测试 (latency)
- 使用DWT硬件周期计数器进行精确测量
- 测试内存访问延迟
- 提供最小值、最大值、平均值统计
- 支持可配置的测量次数

#### 吞吐量测试 (throughput)
- 基于RT-Thread邮箱的生产者-消费者模式
- 测试数据包处理吞吐量
- 创建独立的生产者和消费者线程
- 计算单位时间内的数据处理能力

#### 抖动测试 (jitter)
- 使用RT-Thread定时器测试时序精度
- 测量定时器回调的时间抖动
- 与期望间隔进行比较分析
- 统计抖动的最小值、最大值和平均值

#### 空闲CPU测试 (idle_cpu)
- 测试系统空闲时间和CPU利用率
- 创建低优先级空闲任务和监控线程
- 统计空闲周期数和平均性能
- 评估系统整体效率

#### 内存测试 (memory)
- 动态内存分配和释放压力测试
- 追踪内存分配统计信息
- 检测内存泄露和分配失败
- 测量峰值内存使用量和碎片化程度

### 7. 增强的报告系统

新的报告系统提供详细的性能指标：

```
=== Latency Test Results ===
Measurements: 1000
Min latency: 45 cycles
Max latency: 123 cycles
Avg latency: 67 cycles
Total latency: 67000 cycles
```

每个测试都有专门的报告格式，包含相关的关键性能指标。框架自动识别测试类型并生成相应的详细报告。
