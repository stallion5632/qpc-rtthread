## blinky - QP/C + RT-Thread 最小示例

**核心特征**：
- **最小可运行**：只包含一个Active Object（AO），代码极简
- **定时事件驱动**：利用QTimeEvt周期性切换LED状态
- **无信号订阅/发布**：不涉及复杂的事件池、信号订阅、发布机制
- **无RT-Thread阻塞API**：完全事件驱动，主线程让出CPU

**关键代码结构**：
```c
// AO结构体
typedef struct {
    QActive super;
    QTimeEvt blinkEvt;
    uint8_t led_state;
} BlinkyAO;

static BlinkyAO l_blinkyAO;
QActive * const AO_Blinky = &l_blinkyAO.super;

// 构造函数
static void BlinkyAO_ctor(void) {
    BlinkyAO *me = &l_blinkyAO;
    QActive_ctor(&me->super, Q_STATE_CAST(&BlinkyAO_initial));
    QTimeEvt_ctorX(&me->blinkEvt, &me->super, BLINK_TIMEOUT_SIG, 0U);
    me->led_state = 0;
}

// 状态机实现
static QState BlinkyAO_initial(BlinkyAO * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&BlinkyAO_blinking);
}

static QState BlinkyAO_blinking(BlinkyAO * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            QTimeEvt_armX(&me->blinkEvt, 50, 50); // 50 tick周期
            return Q_HANDLED();
        }
        case BLINK_TIMEOUT_SIG: {
            me->led_state = !me->led_state;
            rt_pin_write(LED_PIN, me->led_state);
            return Q_HANDLED();
        }
        default:
            return Q_SUPER(&QHsm_top);
    }
}
```

**main函数与启动流程**：
```c
int main(void) {
    QF_init();
    // ... RT-Thread硬件初始化 ...
    BlinkyAO_ctor();
    static QEvt const *blinkyQueue[5];
    static uint8_t blinkyStack[512];
    QACTIVE_START(AO_Blinky, 1U, blinkyQueue, Q_DIM(blinkyQueue), blinkyStack, sizeof(blinkyStack), 0);
    return QF_run();
}
```

**开发注意事项**：
- QTimeEvt定时事件推荐用作周期性任务驱动，避免阻塞API
- main函数最后必须让出CPU（QF_run），否则AO无法调度
- 适合初学者理解事件驱动、状态机、AO基本概念


---

## 四个Demo的详细对比分析

### 1. **qactive_demo_lite** - 纯事件驱动基础示例

**核心特征**：
- **纯QP/C事件驱动**：完全不使用任何RT-Thread阻塞API（如`rt_sem_take`, `rt_mutex_take`, `rt_thread_mdelay`等）
- **简单信号订阅**：每个AO只订阅自己的基本信号
- **基础事件池**：只有简单的事件池设计
- **4个AO**：Sensor、Processor、Worker、Monitor，功能简单明了

**优点**：理解QP/C框架的最佳入门示例，代码清晰
**缺点**：缺乏实际工程中的复杂需求处理


### 2. **qactive_demo_block** - AO内部使用阻塞API示例

**核心特征**：
- **破坏Run-to-Completion**：AO状态机内直接调用`rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER)`等阻塞API
- **混合模型**：QP/C事件驱动 + RT-Thread阻塞同步
- **架构缺陷明确**：README明确指出"破坏了QActive run-to-complete事件处理语义"

**实际问题**：
```c
// 在AO状态机内直接阻塞
rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);  // 会挂起AO线程
g_shared_config.sensor_rate = new_rate;
rt_mutex_release(g_config_mutex);
```

**设计目的**：演示错误的做法，展示阻塞API对AO响应性的负面影响


### 3. **qactive_demo_nonblock** - 代理线程分离阻塞操作

**核心特征**：
- **AO保持非阻塞**：AO状态机内绝不调用RT-Thread阻塞API
- **代理线程模式**：引入ConfigProxy、StorageProxy等独立线程处理阻塞操作
- **消息队列通信**：AO与代理线程通过`rt_mq`异步通信
- **事件确认机制**：代理线程处理完毕后通过事件回传结果给AO

**关键实现**：
```c
// AO通过非阻塞方式请求配置
post_config_request(key, buf, sender_ao);  // 立即返回，不阻塞

// 代理线程阻塞等待并处理
rt_mq_recv(config_mq, &req, size, RT_WAITING_FOREVER);  // 只有代理线程阻塞
```

**技术亮点**：完美解决了AO响应性与阻塞操作的矛盾


### 4. **advanced_dispatcher_demo** (实际是performance_tests) - 工程化复杂应用

**核心特征**：
- **复杂信号管理**：信号分组（COUNTER_*、TIMER_*、LOGGER_*、APP_*），防止冲突
- **多重订阅**：AO可订阅多个不同功能的信号，支持动态订阅/退订
- **发布-订阅广播**：真正的`QF_PUBLISH`广播机制
- **工程化模块**：日志系统、统计模块、互斥保护、状态报告等
- **TimerAO报告状态**：专门的`TimerAO_reporting`状态处理复杂报告逻辑
- **MISRA C合规**：代码规范、类型安全、防御性编程

**架构复杂度**：
```c
// 复杂的信号订阅
QActive_subscribe(&me->super, COUNTER_UPDATE_SIG);
QActive_subscribe(&me->super, TIMER_REPORT_SIG);
QActive_subscribe(&me->super, LOGGER_LOG_SIG);
// 支持动态订阅管理

// 广播机制
QF_PUBLISH(&counterEvt, me);  // 广播给所有订阅者
```


## 总结对比表

| 特性 | qactive_demo_lite | qactive_demo_block | qactive_demo_nonblock | advanced_dispatcher_demo |
|------|-------------------|-------------------|----------------------|-------------------------|
| **AO内阻塞API** | ❌ 无 | ✅ 有（设计缺陷） | ❌ 无 | ❌ 无 |
| **代理线程** | ❌ 无 | ❌ 无 | ✅ 有 | ✅ 有 |
| **信号复杂度** | 简单 | 简单 | 中等 | 复杂（分组管理） |
| **事件池** | 基础 | 基础 | 多层次 | 精细化 |
| **工程化程度** | 低 | 低 | 中 | 高 |
| **设计目的** | 入门演示 | 反面教材 | 最佳实践 | 工程应用 |

**技术演进路径**：
1. **lite** → 理解QP/C基础
2. **block** → 认识错误做法的危害
3. **nonblock** → 掌握正确的架构模式
4. **advanced** → 应用到复杂工程项目
