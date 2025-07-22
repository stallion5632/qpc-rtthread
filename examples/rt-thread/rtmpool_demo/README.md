
# RT-Thread 内存池集成 QP/C 事件池（margin/fallback 支持）

本方案通过 QF_EPOOL_* 宏机制，将 RT-Thread 的 rt_mempool 作为 QP/C 事件池后端，支持 margin 参数和自动 fallback，兼容原生 QP/C API。


## 快速使用

项目配置：
```c
#define QF_ENABLE_RT_MEMPOOL 1 // 启用 RT-Thread 内存池后端
#define QF_RTMPOOL_DEBUG 1     // 可选，启用调试
```

事件池初始化与分配：
```c
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(QEvt) l_smlPoolSto[10];
QF_poolInit(l_smlPoolSto, sizeof(l_smlPoolSto), sizeof(QEvt));
QEvt *evt = Q_NEW(QEvt, MY_SIGNAL); // 普通事件
```

调试统计：
```c
QF_RTMemPool_printStats(&QF_pool_[0]);
```


## 机制说明
通过 QF_EPOOL_* 宏适配，事件池分配支持 margin 检查和自动 fallback，兼容 QP/C 原有引用计数和生命周期。


---


# margin/fallback 机制与用法


1. margin/fallback：分配时优先小池，margin>0 时自动尝试更大池
2. 统计接口：可打印池分配/失败/峰值等信息


## 与 QP/C 原生事件池差异

| 特性 | QP/C 原生事件池 | RT-Thread 内存池 | 本次改进版 |
|------|---------------|-----------------|------------|
| 多级池 | 支持（最多15个） | 独立池 | 支持（使用多个 rt_mempool） |
| margin 功能 | 支持 | 不支持 | 支持（fallback 机制） |
| 块大小 | 严格递增 | 固定大小 | 严格递增（保持与 QP/C 兼容） |
| 内存对齐 | 自动处理 | 需手动对齐 | 需手动对齐（使用 ALIGN 宏） |
| 统计信息 | 基础统计 | 基础计数 | 增强统计（分配、失败、峰值等） |


## margin/fallback 实现原理

本集成方案通过以下策略保留了 QP/C 的 margin 功能：

1. **最佳匹配优先**：优先在最小满足事件大小的池中分配
2. **保留机制**：分配前检查池是否可满足 margin 要求
3. **自动 fallback**：当首选池不满足 margin 时，尝试使用更大的池
4. **严格递增**：保持与 QP/C 相同的池大小递增要求


### 分配流程（mermaid）

```mermaid
flowchart TD
    A[开始分配 (evtSize, margin)] --> B{找到最小满足大小的池 pid0}
    B --> C{margin = 0?}
    C -- 是 --> D[直接尝试分配]
    C -- 否 --> E[检查 free_count > margin]
    D --> F{分配成功?}
    E --> F
    F -- 是 --> G[返回事件]
    F -- 否 --> H{margin > 0?}
    H -- 否 --> I[返回失败]
    H -- 是 --> J[尝试后续更大的池]
    J --> K{任一池分配成功?}
    K -- 是 --> G
    K -- 否 --> I
```


## margin 参数与调试用法


### margin 参数用法

margin 参数允许你保证池中至少保留一定数量的块，提高关键事件的分配成功率：

```c
/* 普通事件分配 - margin=0 */
SensorDataEvt *evt = Q_NEW(SensorDataEvt, SENSOR_DATA_SIG);

/* 关键事件分配 - 保留至少 3 个块 */
CriticalAlarmEvt *alarm = (CriticalAlarmEvt *)QF_NEW(CriticalAlarmEvt,
                                                    3, /* margin */
                                                    ALARM_SIG);
```


### 统计接口

```c
/* 获取全部内存池统计 */
QF_PoolStats stats = QF_getPoolStats();
rt_kprintf("Allocations: %u, Failures: %u\n",
          stats.allocations, stats.failures);

/* 打印内存池统计到控制台 */
QF_printPoolStats();
```


### 初始化与配置

与标准 QP/C 事件池初始化完全相同：

```c
/* 事件池定义 - 保持严格递增顺序 */
ALIGN(RT_ALIGN_SIZE)
static QF_MPOOL_EL(QEvt) l_smlPoolSto[10];

ALIGN(RT_ALIGN_SIZE)
static QF_MPOOL_EL(SensorDataEvt) l_medPoolSto[8];

/* 初始化事件池 */
QF_poolInit(l_smlPoolSto, sizeof(l_smlPoolSto), sizeof(QEvt));
QF_poolInit(l_medPoolSto, sizeof(l_medPoolSto), sizeof(SensorDataEvt));
```


## 最佳实践

1. **内存对齐**：所有事件池存储必须使用 `ALIGN(RT_ALIGN_SIZE)` 宏对齐
2. **严格递增**：池大小必须严格递增，且每种大小只初始化一次
3. **margin 使用**：
   - 对普通事件使用 margin=0
   - 对关键事件使用 margin>0，确保有足够空间保留
4. **避免过多 fallback**：虽然 fallback 机制很有用，但过度依赖会导致内存使用效率下降


## Demo运行现象与常见问题


### 正常运行现象

```
RT-Thread Memory Pool Demo for QP/C
QP/C version: 7.2.2
RT-Thread memory pool backend is ENABLED
sizeof(QEvt)=4, sizeof(DataEvt)=12
DemoAO: Entered running state
DemoAO: Created event with data=3
DemoAO: Received DataEvt with data=3
DemoAO: Created event with data=6
DemoAO: Received DataEvt with data=6
DemoAO: Created event with data=9
DemoAO: Received DataEvt with data=9
```


### 常见错误与排查

**断言失败示例：**
```
(qf_dyn) assertion failed at function:, line number:201
```
原因：事件池初始化顺序/大小不递增，或有事件类型未初始化对应池，或多个 demo 并存导致事件池冲突。

**多 demo 并存冲突示例：**
```
(qf_dyn) assertion failed at function:, line number:201
```
原因：同一固件/工程下有多个 demo 初始化事件池，导致 QP/C 全局事件池状态混乱。

**调试建议：**
- 打印 sizeof(QEvt)、sizeof(自定义事件)，确认池大小递增。
- 检查 QF_poolInit 顺序和数量，确保每种事件类型只初始化一次。
- 只保留一个 demo 的事件池初始化和 main/入口。



### 事件池断言 (qf_dyn.c:201)

如果遇到如下断言：

```
(qf_dyn) assertion failed at function:, line number:201
```

请检查：
- 所有 QF_poolInit 必须顺序递增，且每种事件大小只能有一个池。
- 每种 Q_NEW 的事件类型都必须有对应的池初始化。
- sizeof(QEvt) < sizeof(DataEvt) < ...，不能有重复。


### 多 demo 并存导致冲突

QP/C 的事件池全局唯一，**同一固件/工程只能有一组事件池初始化**。如果多个 demo（如 rtmpool_demo 和 qactive_demo_lite）都初始化事件池，会导致断言或行为异常。

**建议：**
- 只保留一个 demo 的事件池初始化和 main/入口，其他 demo 只保留 AO 逻辑，不初始化事件池。
- 或用 menuconfig/宏控制只编译/运行一个 demo。


### 事件池调试

QF_pool_ 是 QP/C 内部符号，不能直接在应用访问。调试池状态建议通过适配层暴露接口，或仅用 QF 提供的 API。


### 其他注意事项

- 事件存储区域必须正确对齐（使用 `ALIGN(RT_ALIGN_SIZE)`）
- 确保事件块大小适合存储事件数据
- 在有限资源环境下，可关闭调试功能以节省内存


## 参考链接

- RT-Thread 内存池文档：https://www.rt-thread.org/document/api/group___pool.html
- QP/C 文档：https://www.state-machine.com/qpc/
