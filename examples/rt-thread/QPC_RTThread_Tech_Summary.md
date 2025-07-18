# QPC + RT-Thread 开发实战指南

## 1. 概述

本文档总结了在 RT-Thread 平台下使用 QPC 框架开发多活动对象（Active Object, AO）应用时的关键要点、常见问题及解决方案。包含完整的代码示例、最佳实践和故障排查指南。

## 2. 架构设计核心要点

### 2.1 事件池设计原则

**关键规则：**
- 事件池必须按事件大小**严格递增**顺序初始化
- 同一大小的事件只能使用**一个共享池**
- 建议统一事件结构体大小，减少池碎片

**错误示例：**
```c
// ❌ 错误：多个8字节事件使用独立池
QF_poolInit(sensorPool, sizeof(sensorPool), sizeof(SensorDataEvt));     // 8字节
QF_poolInit(resultPool, sizeof(resultPool), sizeof(ProcessorResultEvt)); // 8字节 - 断言失败！
```

**正确示例：**
```c
// ✅ 正确：同大小事件共享一个池
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(QEvt) basicEventPool[20];        // 4字节池
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(SensorDataEvt) shared8Pool[30];  // 8字节共享池

QF_poolInit(basicEventPool, sizeof(basicEventPool), sizeof(QEvt));           // 4字节
QF_poolInit(shared8Pool, sizeof(shared8Pool), sizeof(SensorDataEvt));        // 8字节
```

### 2.2 状态机编程规范

**关键信号处理：**
所有状态都必须显式处理 QPC 内部信号，否则会触发框架断言。

```c
static QState SensorAO_initial(SensorAO * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            // 必须处理：订阅信号、初始化资源
            QActive_subscribe(&me->super, SENSOR_READ_SIG);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            // 必须处理：状态迁移
            return Q_TRAN(&SensorAO_active);
        }
        default: {
            return Q_SUPER(&QHsm_top);
        }
    }
}

static QState SensorAO_active(SensorAO * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            // 进入状态时的操作
            QTimeEvt_armX(&me->timeEvt, 200, 200);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            // 退出状态时的清理
            QTimeEvt_disarm(&me->timeEvt);
            return Q_HANDLED();
        }
        case TIMEOUT_SIG: {
            // 业务逻辑处理
            // ...
            return Q_HANDLED();
        }
        default: {
            return Q_SUPER(&QHsm_top);
        }
    }
}
```


### 2.3 AO 初始化防重复机制

**问题背景：**
RT-Thread 自动初始化和手动调用可能导致资源重复注册，触发断言失败。

```c
void QActiveDemo_init(void) {
    static uint8_t isInitialized = 0;
    if (isInitialized != 0) {
        printf("QActive Demo: Already initialized, skipping...\n");
        return;
    }
    isInitialized = 1;

    // 框架初始化
    QF_init();
    QF_psInit(subscrSto, Q_DIM(subscrSto));

    // 事件池初始化（严格按大小递增）
    QF_poolInit(basicEventPool, sizeof(basicEventPool), sizeof(QEvt));
    QF_poolInit(shared8Pool, sizeof(shared8Pool), sizeof(SensorDataEvt));

    // AO 构造函数调用
    SensorAO_ctor();
    ProcessorAO_ctor();
    WorkerAO_ctor();
    MonitorAO_ctor();
}

int qactive_demo_start(void) {
    static uint8_t isStarted = 0;
    if (isStarted != 0) {
        printf("QActive Demo: Already started, skipping...\n");
        return 0;
    }
    isStarted = 1;

    // 启动 AO
    QACTIVE_START(AO_Sensor, 1U, sensorQueue, Q_DIM(sensorQueue),
                  sensorStack, sizeof(sensorStack), (void *)0);
    // ... 其他 AO 启动

    return QF_run();
}
```

### 2.4 内存对齐与栈配置

**关键注意事项：**
- 所有静态内存必须使用 `ALIGN(RT_ALIGN_SIZE)` 保证平台兼容性
- 栈大小需要考虑深度嵌套调用和中断处理

```c
// 正确的内存对齐
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(QEvt) basicEventPool[20];
ALIGN(RT_ALIGN_SIZE) static QEvt const *sensorQueue[10];
ALIGN(RT_ALIGN_SIZE) static uint8_t sensorStack[1024];
```

## 3. 常见异常诊断与解决方案

### 3.1 事件池断言失败 (qf_dyn.c:201)

**错误信息：** `(qf_dyn) assertion failed at function:, line number:201`

**诊断步骤：**
1. 检查事件结构体大小
```c
printf("sizeof(QEvt)=%d\n", (int)sizeof(QEvt));
printf("sizeof(SensorDataEvt)=%d\n", (int)sizeof(SensorDataEvt));
printf("sizeof(ProcessorResultEvt)=%d\n", (int)sizeof(ProcessorResultEvt));
```

2. 检查初始化顺序
```c
// ❌ 错误：大小不递增
QF_poolInit(pool1, sizeof(pool1), 8);  // 8字节
QF_poolInit(pool2, sizeof(pool2), 4);  // 4字节 - 断言失败！

// ✅ 正确：严格递增
QF_poolInit(pool1, sizeof(pool1), 4);  // 4字节
QF_poolInit(pool2, sizeof(pool2), 8);  // 8字节
```

### 3.2 状态机信号处理断言 (qep_hsm.c:220)

**错误信息：** `(qep_hsm) assertion failed at function:, line number:220`

**根本原因：** 状态未处理 QPC 内部信号

**解决模板：**
```c
static QState MyAO_someState(MyAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            // 进入状态时的初始化
            // 订阅信号、启动定时器等
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            // 退出状态时的清理
            // 取消订阅、停止定时器等
            status = Q_HANDLED();
            break;
        }
        case Q_INIT_SIG: {
            // 初始状态迁移（仅 initial 状态需要）
            status = Q_TRAN(&MyAO_nextState);
            break;
        }
        case Q_EMPTY_SIG: {
            // 队列空时处理（可选）
            status = Q_HANDLED();
            break;
        }
        // 业务信号处理
        case MY_SIGNAL_SIG: {
            // 业务逻辑
            status = Q_HANDLED();
            break;
        }
        default: {
            // 未处理信号上抛给父状态
            status = Q_SUPER(&ParentState);
            break;
        }
    }

    return status;
}
```

### 3.3 AO 线程调度问题

**症状：** 事件处理在 idle 线程执行而非 AO 专用线程

**排查要点：**
```c
// 1. 检查线程创建方式
void QActive_start_(QActive * const me, uint_fast8_t prio,
                   QEvt const * * const qSto, uint_fast16_t qLen,
                   void * const stkSto, uint_fast16_t stkSize,
                   void const * const par) {

    // 使用 rt_thread_create 而非 rt_thread_init
    me->thread.type = RT_Object_Class_Thread;
    // ... 其他设置
}

// 2. 主线程让出 CPU
int main(void) {
    QActiveDemo_init();
    qactive_demo_start();

    // 关键：主线程让出控制权
    rt_thread_delay(RT_TICK_PER_SECOND);
    while (1) {
        rt_thread_delay(RT_TICK_PER_SECOND);
    }
}

// 3. 调试线程信息
static void debug_thread_info(QActive *ao, const char *name) {
    rt_kprintf("[%s] Thread: %s, Prio: %d, State: %d\n",
               name, rt_thread_self()->name,
               rt_thread_self()->current_priority,
               rt_thread_self()->stat);
}
```

### 3.3.1 AO 事件队列溢出与线程栈断言问题实战

**现象：**
- 日志持续出现 `[QPC][ERROR] AO event queue full, event drop! AO=..., sig=...`，说明 AO 事件队列溢出。
- 或遇到 `(stack_start != RT_NULL) assertion failed at function:rt_thread_init, line number:308`，说明 AO 线程栈未分配。

**排查与修正过程：**
1. 检查 AO 启动时事件队列和栈的分配：
   ```c
   // 错误写法（队列太小/栈为NULL）
   static QEvt *counterAOQueueSto[4];
   QActive_start_((QActive*)&s_counter_ao, 1, counterAOQueueSto, 4, NULL, 1024, NULL);
   ```
2. 修正为：
   ```c
   // 正确写法：增大队列容量，静态分配线程栈
   static QEvt const *counterAOQueueSto[32]; // 增大队列容量为32
   static rt_uint8_t counterAOStack[1024];   // 静态分配 AO 线程栈
   QActive_start_((QActive*)&s_counter_ao,
                  1, // prioSpec
                  counterAOQueueSto, 32, // 队列存储和长度
                  counterAOStack, sizeof(counterAOStack), // 栈指针和大小
                  NULL); // par
   ```
3. 这样可彻底消除事件丢失和线程栈断言问题。

**经验总结：**
- AO 事件队列建议不少于 16~32，根据事件高峰量调整。
- AO 线程栈必须静态分配并传递给 QActive_start_，不能为 NULL。
- 相关代码建议加注释，便于团队成员理解和复用。

### 3.4 QActive_setAttr 断言失败

**错误信息：** `(qf_port) assertion failed at function:, line number:300`

**修复代码：**
```c
void QActive_setAttr(QActive *const me, uint32_t attr1, void const *attr2) {
    // RT-Thread thread.name 是数组，检查首字节
    Q_REQUIRE_ID(300, me->thread.name[0] == '\0');

    switch (attr1) {
        case THREAD_NAME_ATTR:
            rt_memset(me->thread.name, 0x00, RT_NAME_MAX);
            rt_strncpy(me->thread.name, (char *)attr2, RT_NAME_MAX - 1);
            break;
    }
}
```

### 3.5 发布-订阅机制配置

**配置要点：**
```c
// 1. 全局订阅表分配
#define MAX_PUB_SIG     32U
static QSubscrList subscrSto[MAX_PUB_SIG];

// 2. 系统初始化时配置（仅一次）
void QActiveDemo_init(void) {
    QF_psInit(subscrSto, Q_DIM(subscrSto));
}

// 3. AO 订阅信号（在 Q_ENTRY_SIG 中）
static QState SensorAO_initial(SensorAO * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            QActive_subscribe(&me->super, SENSOR_READ_SIG);
            return Q_HANDLED();
        }
    }
}

// 4. 发布事件
SensorDataEvt *evt = Q_NEW(SensorDataEvt, SENSOR_DATA_SIG);
evt->data = sensor_value;
QF_PUBLISH(&evt->super, me);  // 一对多发布
```

## 4. 关键代码片段与开发注意事项

### 4.1 完整的 AO 构造函数模板

```c
typedef struct {
    QActive super;          // 继承 QActive
    QTimeEvt timeEvt;       // 定时器事件
    uint32_t counter;       // 业务数据
    // ... 其他成员
} MyAO;

static MyAO l_myAO;
QActive * const AO_My = &l_myAO.super;

static void MyAO_ctor(void) {
    MyAO *me = &l_myAO;

    // 1. 调用基类构造函数
    QActive_ctor(&me->super, Q_STATE_CAST(&MyAO_initial));

    // 2. 初始化定时器事件
    QTimeEvt_ctorX(&me->timeEvt, &me->super, MY_TIMEOUT_SIG, 0U);

    // 3. 初始化业务数据
    me->counter = 0;
}
```

### 4.2 事件动态分配与释放

```c
// 动态事件分配
static void send_sensor_data(uint32_t data) {
    SensorDataEvt *evt = Q_NEW(SensorDataEvt, SENSOR_DATA_SIG);
    if (evt != NULL) {  // 检查分配是否成功
        evt->data = data;
        QF_PUBLISH(&evt->super, &l_sensorAO);  // 框架负责释放
    } else {
        // 处理内存分配失败
        rt_kprintf("ERROR: Failed to allocate SensorDataEvt\n");
    }
}

// 事件处理中的类型转换
case SENSOR_DATA_SIG: {
    SensorDataEvt const *evt = (SensorDataEvt const *)e;
    rt_kprintf("Received sensor data: %u\n", evt->data);
    status = Q_HANDLED();
    break;
}
```

### 4.3 定时器使用模式

```c
// 一次性定时器
QTimeEvt_armX(&me->timeEvt, BSP_TICKS_PER_SEC, 0U);  // 1秒后触发一次

// 周期定时器
QTimeEvt_armX(&me->timeEvt, BSP_TICKS_PER_SEC, BSP_TICKS_PER_SEC);  // 每秒触发

// 停止定时器
QTimeEvt_disarm(&me->timeEvt);

// 定时器事件处理
case MY_TIMEOUT_SIG: {
    // 定时器触发处理
    me->counter++;
    if (me->counter >= MAX_COUNT) {
        QTimeEvt_disarm(&me->timeEvt);  // 停止定时器
    }
    status = Q_HANDLED();
    break;
}
```

### 4.4 跨 AO 通信模式

```c
// 点对点发送
WorkerWorkEvt *work_evt = Q_NEW(WorkerWorkEvt, WORKER_WORK_SIG);
work_evt->work_id = task_id;
QACTIVE_POST(AO_Worker, &work_evt->super, me);  // 指定目标 AO

// 一对多广播
SensorDataEvt *evt = Q_NEW(SensorDataEvt, SENSOR_DATA_SIG);
evt->data = sensor_reading;
QF_PUBLISH(&evt->super, me);  // 所有订阅者都会收到

// 延迟发送
QACTIVE_POST_LIFO(AO_Target, &evt->super, me);  // LIFO 队列插入
```

### 4.5 调试与诊断代码

```c
// 事件池状态监控
void debug_pool_status(void) {
    rt_kprintf("Event pool usage:\n");
    rt_kprintf("  Basic pool free blocks: %d\n", QF_getPoolMin(0));
    rt_kprintf("  Shared8 pool free blocks: %d\n", QF_getPoolMin(1));
}

// AO 状态监控
void debug_ao_status(QActive *ao, const char *name) {
    rt_kprintf("AO %s: Queue free=%d, Thread=%s, Prio=%d\n",
               name,
               QActive_getQueueMin(ao),
               rt_thread_self()->name,
               rt_thread_self()->current_priority);
}

// 断言钩子定制
void Q_onAssert(char const * const module, int_t loc) {
    rt_kprintf("ASSERTION FAILED in %s:%d\n", module, loc);
    rt_kprintf("System state: pools=%d/%d, active_objects=%d\n",
               QF_getPoolMin(0), QF_getPoolMin(1), QF_getMaxActive());

    // 调试信息收集后再断言
    RT_ASSERT(0);
}
```

### 4.6 系统集成最佳实践

```c
// 系统启动序列
int main(void) {
    // 1. RT-Thread 初始化（已由内核完成）

    // 2. QPC 框架初始化
    QActiveDemo_init();

    // 3. 启动应用
    int ret = qactive_demo_start();

    // 4. 主线程让出控制权
    rt_thread_delay(10);  // 给 AO 线程调度机会

    // 5. 主线程循环（可选）
    while (1) {
        rt_thread_delay(RT_TICK_PER_SECOND);
        debug_pool_status();  // 定期监控
    }

    return ret;
}

// MSH 命令集成
static void cmd_qactive_status(int argc, char **argv) {
    debug_pool_status();
    rt_kprintf("QF active objects: %d\n", QF_getMaxActive());
}
MSH_CMD_EXPORT(qactive_status, show QActive demo status);

// RT-Thread 组件自动初始化
INIT_APP_EXPORT(qactive_demo_init);
```

## 5. 故障排查流程图

```
应用启动失败
├── 断言失败？
│   ├── qf_dyn.c:201 → 检查事件池大小顺序
│   ├── qep_hsm.c:220 → 检查状态机信号处理
│   ├── qf_port.c:300 → 检查 QActive_setAttr 调用
│   └── rt_object_init:358 → 检查重复初始化
├── 线程调度异常？
│   ├── 事件在 idle 线程处理 → 检查 AO 线程创建
│   ├── 主线程不让出 CPU → 添加 rt_thread_delay
│   └── Fast-path 机制 → 禁用快速路径
└── 内存相关？
    ├── data abort → 检查指针和栈大小
    ├── 内存泄漏 → 检查事件分配/释放
    └── 对齐问题 → 使用 ALIGN(RT_ALIGN_SIZE)
```

## 6. 常见错误速查表

| 错误信息 | 文件位置 | 根本原因 | 解决方案 |
|---------|---------|---------|---------|
| `(qf_dyn) assertion failed... line:201` | qf_dyn.c | 事件池大小不递增或重复 | 重新设计事件池，按大小严格递增 |
| `(qep_hsm) assertion failed... line:220` | qep_hsm.c | 状态未处理内部信号 | 添加 Q_ENTRY/EXIT/INIT_SIG 处理 |
| `(obj != object) assertion failed... line:358` | rt_object_init | 对象重复注册 | 添加静态标志防重复初始化 |
| `(qf_port) assertion failed... line:300` | qf_port.c | QActive_setAttr 调用问题 | 检查 thread.name[0] 而非指针 |
| `(qf_ps) assertion failed... line:300` | qf_ps.c | 发布订阅未初始化 | 初始化 subscrSto 且在 AO 启动后订阅 |
| `Function[rt_mutex_take] shall not be used in ISR` | RT-Thread内核 | 在 ISR 上下文使用阻塞API | 使用原子操作或关中断保护 |
| `data abort:Exception...` | 硬件层 | 非法内存访问 | 检查指针、栈大小、对齐 |
| 事件在 idle 线程处理 | - | AO 线程未正确创建/调度 | 使用 rt_thread_create，主线程让出CPU |

## 7. 开发检查清单

### 启动前检查
- [ ] 事件结构体大小打印确认
- [ ] 事件池按大小严格递增初始化
- [ ] 静态内存使用 `ALIGN(RT_ALIGN_SIZE)`
- [ ] 防重复初始化标志设置
- [ ] 发布订阅表 `subscrSto` 分配

### 状态机设计检查
- [ ] 每个状态处理 `Q_ENTRY_SIG`
- [ ] 每个状态处理 `Q_EXIT_SIG`
- [ ] initial 状态处理 `Q_INIT_SIG`
- [ ] default 分支返回父状态
- [ ] 订阅操作在 `Q_ENTRY_SIG` 中执行

### 运行时检查
- [ ] AO 线程名称和优先级正确
- [ ] 事件处理在正确线程上下文
- [ ] 主线程让出 CPU 控制权
- [ ] 事件池使用率监控
- [ ] 断言钩子 `Q_onAssert` 实现

## 8. 性能优化建议

### 8.1 内存使用优化
```c
// 事件结构体对齐优化
typedef struct {
    QEvt super;         // 4字节
    uint32_t data;      // 4字节，总计8字节对齐
} __attribute__((packed)) SensorDataEvt;

// 避免过多事件池
#define SMALL_EVENTS    20  // 基础事件池大小
#define MEDIUM_EVENTS   30  // 业务事件池大小
#define LARGE_EVENTS    10  // 大事件池大小（谨慎使用）
```

### 8.2 实时性优化
```c
// AO 优先级设计
#define SENSOR_PRIO     1   // 最高优先级（数据采集）
#define PROCESSOR_PRIO  2   // 中等优先级（数据处理）
#define WORKER_PRIO     3   // 较低优先级（后台任务）
#define MONITOR_PRIO    4   // 最低优先级（监控统计）

// 栈大小经验值
#define AO_STACK_SIZE   1024    // 基础AO栈大小
#define COMPLEX_AO_STACK 2048   // 复杂处理AO栈大小
```

### 8.3 调试支持优化
```c
// 编译时调试开关
#ifdef QPC_DEBUG
    #define QPC_LOG(fmt, ...) rt_kprintf("[QPC] " fmt "\n", ##__VA_ARGS__)
    #define QPC_ASSERT(cond) do { if (!(cond)) { \
        rt_kprintf("QPC_ASSERT failed: %s:%d\n", __FILE__, __LINE__); \
        RT_ASSERT(0); }} while(0)
#else
    #define QPC_LOG(fmt, ...)
    #define QPC_ASSERT(cond)
#endif

// 运行时统计
typedef struct {
    uint32_t events_processed;
    uint32_t state_transitions;
    uint32_t pool_allocation_failures;
} AOStats;
```


> **参考资源**
> - QPC 官方文档：https://www.state-machine.com/qpc/
> - RT-Thread 文档：https://www.rt-thread.org/document/site/
> - 本文档对应示例代码：`examples/rt-thread/qactive_demo_lite/`
