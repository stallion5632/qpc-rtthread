# QP/C RT-Thread Integration Optimizations

本文档说明了QP/C框架与RT-Thread实时操作系统的两个主要优化功能：

## 1. 超时等待与心跳机制

### 功能概述
- **超时等待**: AO(Active Object)线程使用带超时的事件等待，避免无限阻塞
- **心跳机制**: 在无事件时执行周期性心跳任务
- **看门狗喂狗**: 自动喂养硬件看门狗，防止系统复位
- **后台维护**: 执行性能统计、内存清理等后台任务

### 配置选项
```c
/* 启用心跳和看门狗功能 */
#define QF_ENABLE_HEARTBEAT 1

/* 心跳间隔，默认100ms */
#define QF_HEARTBEAT_TICKS (RT_TICK_PER_SECOND/10)

/* 启用看门狗喂狗 */
#define QF_ENABLE_WATCHDOG_FEED 1
```

### 实现原理
1. **事件循环修改**: AO线程的事件循环从无限等待改为超时等待
2. **超时处理**: 超时时执行心跳和看门狗任务
3. **看门狗集成**: 使用RT-Thread设备框架操作硬件看门狗
4. **统计更新**: 更新运行时统计信息

### 代码示例
```c
/* 在AO线程中 */
for (;;) {
    rt_err_t result = rt_mb_recv(&act->eQueue, (rt_ubase_t *)&e, QF_HEARTBEAT_TICKS);

    if (result == RT_EOK) {
        /* 处理接收到的事件 */
        QHSM_DISPATCH(&act->super, e, act->prio);
        QF_gc(e);
    }
    else if (result == -RT_ETIMEOUT) {
        /* 超时 - 执行心跳任务 */
        feed_watchdog(act);      /* 喂狗 */
        perf_heartbeat(act);     /* 性能心跳 */
    }
}
```

## 2. 内存池与RT-Thread mempool对接

### 功能概述
- **统一内存管理**: 使用RT-Thread内存池替代QP/C原生事件池
- **分级内存池**: 小、中、大三种规格的内存池，提高内存利用率
- **统计监控**: 详细的内存分配/释放统计信息
- **Shell监控**: 通过RT-Thread shell命令监控内存使用情况

### 配置选项
```c
/* 启用RT-Thread内存池集成 */
#define QF_ENABLE_RT_MEMPOOL 1

/* 内存池配置 */
#define QF_RT_MEMPOOL_SMALL_SIZE    64U    /* 小事件大小 */
#define QF_RT_MEMPOOL_SMALL_COUNT   32U    /* 小事件数量 */
#define QF_RT_MEMPOOL_MEDIUM_SIZE   128U   /* 中等事件大小 */
#define QF_RT_MEMPOOL_MEDIUM_COUNT  16U    /* 中等事件数量 */
#define QF_RT_MEMPOOL_LARGE_SIZE    256U   /* 大事件大小 */
#define QF_RT_MEMPOOL_LARGE_COUNT   8U     /* 大事件数量 */
```

### 内存池架构
```
┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│   Small     │  │   Medium    │  │   Large     │
│   Pool      │  │   Pool      │  │   Pool      │
│             │  │             │  │             │
│ 32×64 bytes │  │ 16×128 bytes│  │ 8×256 bytes │
└─────────────┘  └─────────────┘  └─────────────┘
       ↑                ↑                ↑
       └────────────────┼────────────────┘
                        │
                ┌───────▼────────┐
                │ Pool Selector  │
                │ (Auto-select   │
                │  best fit)     │
                └────────────────┘
```

### 实现原理
1. **池选择算法**: 根据事件大小自动选择最适合的内存池
2. **替换QF函数**: 重写QF_new()和QF_gc()函数
3. **统计收集**: 记录分配/释放次数、峰值使用量等
4. **错误处理**: 内存不足时的降级处理

### 使用示例
```c
/* 分配事件 - 自动选择合适的内存池 */
MyEvt *evt = Q_NEW(MyEvt, MY_SIGNAL);
if (evt != (MyEvt *)0) {
    evt->data = 42;
    QACTIVE_POST(target_ao, &evt->super, sender);
}
/* 事件会在处理完成后自动释放到对应内存池 */
```

## Shell监控命令

启用shell命令后，可以使用以下命令监控系统状态：

```bash
# 显示调度器指标
msh> qf_metrics

# 显示内存池状态
msh> qf_mempool
==== QF RT-Thread Memory Pool Statistics ====
Total allocations: 1250
Total deallocations: 1248
Total failures: 0
Outstanding events: 2
Pool 0 (qf_small): Free=30, Used=2, Peak=8/32
Pool 1 (qf_medium): Free=16, Used=0, Peak=3/16
Pool 2 (qf_large): Free=8, Used=0, Peak=1/8
=============================================

# 显示Active Object状态
msh> qf_aos

# 运行集成示例
msh> qf_test

# 显示帮助信息
msh> qf_help
```

## 性能优势

### 心跳机制优势
- **系统可靠性**: 自动看门狗喂狗，防止系统挂起
- **实时监控**: 周期性统计更新，便于性能分析
- **资源管理**: 定期清理和维护，提高系统稳定性

### 内存池优势
- **统一管理**: 所有事件内存通过RT-Thread统一管理
- **监控友好**: 可通过RT-Thread工具链监控内存使用
- **分级优化**: 三级内存池减少内存碎片
- **统计详细**: 完整的分配/释放统计信息

## 兼容性

这些优化功能设计为可选功能，通过宏开关控制：

- `QF_ENABLE_HEARTBEAT`: 控制心跳功能
- `QF_ENABLE_RT_MEMPOOL`: 控制内存池集成
- 关闭时回退到原始QP/C行为
- 不影响现有应用程序

## 注意事项

1. **内存配置**: 根据应用需求调整内存池大小和数量
2. **心跳间隔**: 根据系统要求调整心跳间隔
3. **调试输出**: 生产环境建议关闭`Q_RT_DEBUG`以减少console输出
4. **看门狗配置**: 确保硬件看门狗超时时间大于心跳间隔

## 文件结构

```
ports/rt-thread/
├── qf_rt_config.h      # 配置头文件
├── qf_port.h           # QF端口头文件（已修改）
├── qf_port.c           # QF端口实现（已修改）
├── qf_mempool.c        # RT-Thread内存池集成
├── qf_hooks.c          # QF回调函数（已修改）
├── qf_diagnostics.c    # 诊断和监控（已修改）
└── qf_example.c        # 使用示例
```

这些优化显著提升了QP/C在RT-Thread环境下的可靠性、监控性和资源利用效率。
