# QPC/RT-Thread 性能测试 data abort 问题排查与修复总结

## 1. 问题背景
- 目标：在 RT-Thread 上运行 QPC 性能测试（如 latency_test），要求无 data abort/assertion 错误。
- 现象：启动 Latency Test 时频繁出现 data abort，PC 指向 AO 事件处理相关代码。

## 2. 主要排查与修复措施（精炼版）

1. **事件池类型与事件分配一致性**
   - 多次切换事件池类型（QEvt/LatencyEvt），最终统一为 LatencyEvt，并确保所有 Q_NEW 分配、QACTIVE_POST 投递都严格使用 LatencyEvt 类型，防止类型不匹配导致内存越界和 data abort。

2. **事件池、队列、堆栈资源加大**
   - 多次将 LATENCY_POOL_SIZE、事件队列长度、AO 线程堆栈空间大幅提升，确保极端压力下不会因资源耗尽导致事件分配失败或栈溢出。

3. **发布-订阅表初始化与信号范围对齐**
   - 明确 QF_psInit(subscrSto, MAX_PUB_SIG) 调用，且 MAX_PUB_SIG 由固定值改为 MAX_PERF_SIG，保证所有信号订阅都在表内，防止越界破坏内存。

4. **QF/QF_psInit 初始化时机与唯一性**
   - 在 LatencyTest_start 前强制调用 LatencyTest_frameworkInit，确保 QF_init 和 QF_psInit 只初始化一次且在任何订阅前完成，避免 publish-subscribe 机制未初始化时被使用。

5. **事件池初始化方式规范化**
   - 先用本地静态 latencyPool，后统一调用 PerfCommon_initLatencyPool()，与全局池一致，避免多重池定义或未初始化导致的分配异常。

6. **信号定义、事件结构体对齐与头文件规范**
   - 检查所有信号定义，确保无重复、无遗漏，事件结构体全部 __attribute__((aligned(RT_ALIGN_SIZE)))，头文件引用顺序规范，避免隐式声明和类型冲突。

7. **防御性编程与异常输出**
   - 所有 Q_NEW 分配均有 NULL 检查和错误输出，QACTIVE_POST、QActive_subscribe 等调用前后均有判空和异常处理，提升健壮性，便于定位问题。

8. **编译警告与未实现接口清理**
   - 删除未使用的静态变量和函数，消除 unused-variable/unused-function 警告。对未实现的 RT-Thread 底层接口采用宏空实现，保证编译通过。

9. **AO 启动参数与资源分配复查**
   - 多次核查 AO 启动参数（优先级、队列、堆栈、事件池），确保与实际资源分配完全一致，避免参数不符导致的异常。

10. **异常与断言钩子实现及端口适配层复查**
    - 检查并实现 Q_onAssert 钩子，便于捕获断言和异常信息。多次复查 QPC/RT-Thread 端口适配层，确保 AO 线程、事件分发、发布订阅等机制与 RT-Thread 兼容。

## 3. 结果与后续建议
- 以上所有措施均已落实，data abort 问题依然存在。
- 建议结合 map 文件和异常 PC 地址，定位具体崩溃点；检查 RT-Thread 线程栈溢出、内存保护区、QPC 端口适配层实现；逐步最小化测试用例，排查最小可复现路径。
