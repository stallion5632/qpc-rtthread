# RT-Thread 内存池集成到 QP/C 事件池

本示例演示了如何无侵入地将 RT-Thread 内存池（`rt_mempool`）作为 QP/C 事件池的后端，而无需修改 QP/C 核心代码。

## 集成概述

此集成利用了 QP/C 框架提供的事件池可插拔特性（通过 `QF_EPOOL_*` 宏系列），将 RT-Thread 的内存池作为事件池的实际后端实现。

核心优势：
1. 零入侵 - 不需要修改 QP/C 核心代码
2. 完全兼容现有代码 - 应用程序代码无需更改
3. 配置灵活 - 可通过宏轻松开启或关闭此功能

## 使用方法

### 1. 启用 RT-Thread 内存池集成

在项目配置文件（如 `rtconfig.h`）中添加：

```c
/* 启用 RT-Thread 内存池作为 QP/C 事件池后端 */
#define QF_ENABLE_RT_MEMPOOL 1

/* 可选：启用调试功能 */
#define QF_RTMPOOL_DEBUG 1
```

### 2. 使用事件池

应用程序代码无需任何改变！例如，以下是标准的 QP/C 事件池初始化和使用方法：

```c
/* 事件池定义（确保对齐） */
ALIGN(RT_ALIGN_SIZE)
static QF_MPOOL_EL(QEvt) l_smlPoolSto[10];

/* 初始化事件池 - 标准 QP/C API 不变 */
QF_poolInit(l_smlPoolSto, sizeof(l_smlPoolSto), sizeof(QEvt));

/* 分配事件 - 标准 QP/C API 不变 */
QEvt *evt = Q_NEW(QEvt, MY_SIGNAL);
```

### 3. 调试功能（当 QF_RTMPOOL_DEBUG 启用时）

可以使用以下函数查看内存池统计：

```c
/* 访问内部池（需谨慎使用） */
extern QF_RTMemPool QF_pool_[QF_MAX_EPOOL];

/* 打印内存池统计 */
QF_RTMemPool_printStats(&QF_pool_[0]); /* 小型事件池 */
```

## 工作原理

此集成方案通过以下方式工作：

1. 在 `qf_port.h` 中，根据 `QF_ENABLE_RT_MEMPOOL` 宏，选择性地重定义 QF 事件池操作宏：
   - `QF_EPOOL_TYPE_`
   - `QF_EPOOL_INIT_`
   - `QF_EPOOL_EVENT_SIZE_`
   - `QF_EPOOL_GET_`
   - `QF_EPOOL_PUT_`

2. 当启用 RT-Thread 内存池时，这些宏将映射到相应的 RT-Thread 内存池操作，而保持对外接口不变。

3. QP/C 的内存管理机制（如引用计数）保持不变，因此事件生命周期管理仍由 QP/C 处理。

## Demo执行结果和分析
本节说明典型的 demo 运行现象、常见错误输出及其原因分析。

### 1. 正常运行现象

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

### 2. 常见错误与排查

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


## 常见问题与最佳实践

### 1. 事件池断言 (qf_dyn.c:201)

如果遇到如下断言：

```
(qf_dyn) assertion failed at function:, line number:201
```

请检查：
- 所有 QF_poolInit 必须顺序递增，且每种事件大小只能有一个池。
- 每种 Q_NEW 的事件类型都必须有对应的池初始化。
- sizeof(QEvt) < sizeof(DataEvt) < ...，不能有重复。

### 2. 多 demo 并存导致冲突

QP/C 的事件池全局唯一，**同一固件/工程只能有一组事件池初始化**。如果多个 demo（如 rtmpool_demo 和 qactive_demo_lite）都初始化事件池，会导致断言或行为异常。

**建议：**
- 只保留一个 demo 的事件池初始化和 main/入口，其他 demo 只保留 AO 逻辑，不初始化事件池。
- 或用 menuconfig/宏控制只编译/运行一个 demo。

### 3. 事件池调试

QF_pool_ 是 QP/C 内部符号，不能直接在应用访问。调试池状态建议通过适配层暴露接口，或仅用 QF 提供的 API。

### 4. 其他注意事项

- 事件存储区域必须正确对齐（使用 `ALIGN(RT_ALIGN_SIZE)`）
- 确保事件块大小适合存储事件数据
- 在有限资源环境下，可关闭调试功能以节省内存

## 参考链接

- RT-Thread 内存池文档：https://www.rt-thread.org/document/api/group___pool.html
- QP/C 文档：https://www.state-machine.com/qpc/
