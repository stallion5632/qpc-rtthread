![QP Framework](doxygen/images/qp_banner.jpg)

# QPC-RTThread 端口（QP/C on RT-Thread）

QPC-RTThread 是基于 Quantum Leaps QP/C 实时事件框架的 RT-Thread 操作系统移植包。它将 QP/C 的事件驱动、Active Object（AO）和层级状态机模型，融入 RT-Thread 的高效调度环境，为嵌入式应用提供轻量、响应式且可维护的架构。


## 1. 关于 QP/C 实时事件框架

QP/C（Quantum Platform in C）是一款轻量级、开源的软件框架／RTOS，用于构建由事件驱动的 Active Objects（演员对象）协同工作的实时嵌入式系统。它支持 UML 层级状态机规范，能够在裸机或多种 RTOS 上实现确定性执行。

### QP 框架家族

- QP/C
  语言：C (C11)
  许可：GPLv3 / 商业闭源
- QP/C++
  语言：C++ (C++17)
  许可：GPLv3 / 商业闭源
- SafeQP/C
  语言：C (C11)
  特性：全套安全功能、认证工具包
  许可：商业闭源
- SafeQP/C++
  语言：C++ (C++17)
  特性：全套安全功能、认证工具包
  许可：商业闭源

> 查看 QP/C 更新日志：https://www.state-machine.com/qpc/history.html


## 2. QPC-RTThread 特性

- 支持 QP/C 最新稳定版本
- 集成 RT-Thread 4.x 内核，兼容 ARM Cortex-M0/M3/M4/M7/A
- 基于 Active Object 模式的模块化任务管理
- 丰富的事件队列、时间事件（Time Event）和多端口消息传递
- 动态与静态内存配置选项，减少运行时开销
- 提供事件分级、批量分发、零拷贝及运行时监控等优化层
- 简单易用的 API，与现有 RT-Thread 驱动无缝集成


## 3. 仓库结构

```text
qpc-rtthread/
├── 3rd_party/           # 第三方依赖与适配
├── apps/                # 性能测试与基准
├── doxygen/             # API 文档与示例图
├── examples/            # QP/C + RT-Thread 工程模板
├── include/             # QPC 公共头文件
├── ports/               # RT-Thread 移植层与优化层
│   └── rt-thread/
│       ├── bsp/         # 板级支持包示例
│       ├── include/     # 移植层头文件
│       ├── src/         # 移植层实现
│       └── toolchain.cmake
├── src/                 # QPC 核心源码
├── test/                # 单元测试与验证
├── LICENSE              # MIT 许可（QP/C 双授权另见原仓库）
└── README.md            # 本说明文档
```


## 4. 快速开始

1. 克隆仓库并初始化子模块

   ```bash
   git clone https://github.com/stallion5632/qpc-rtthread.git
   cd qpc-rtthread
   git submodule update --init --recursive
   ```

2. 配置 RT-Thread 工程，启用宏（示例）：

   ```c
   #define QPC_USING_PERFORMANCE_TESTS
   #define RT_USING_MAILBOX
   #define RT_USING_HEAP
   #define RT_USING_TIMER
   #define RT_USING_MUTEX
   #define PKG_USING_QPC
   ```



3. 运行并验证
   通过串口和 LED 指示，观察状态机行为与性能测试报告。


## 5. 移植层概览

位于 `ports/rt-thread/`，核心文件：

- **qp_port.h / qp_port.c**：QP/C 与 RT-Thread 内核接口
- **qf_opt_layer.c/h**：事件分级、批量分发、零拷贝等优化
- **线程封装**：将 QActive 绑定到 RT-Thread 线程/信号量
- **时间事件**：利用 RT-Thread 定时器驱动 QP/C 时序事件
- **内存管理桥接**：支持 QP/C 事件池和动态队列分配


## 6. 性能测试与示例

- **apps/performance_tests/**：延迟、吞吐、抖动、内存占用等一键性能报告
- **examples/rt-thread/**：
  - blinky：LED 翻转示例
  - qactive_demo_lite：轻量级事件驱动模板
  - qactive_demo_nonblock：异步代理线程架构


## 7. 常见问题

- 如何对齐 AO 与线程优先级？
  建议将 QActive 的优先级映射到 RT-Thread 线程优先级，避免相互抢占冲突。

- 时间事件精度？
  由 RT-Thread 系统滴答决定，可修改 `RT_TICK_PER_SECOND` 调整。

- DWT 计数器为零？
  仿真环境（QEMU）中 DWT 不可用，已自动降级为滴答计数。

- 事件池断言失败？
  确保事件池按照大小递增初始化，详见 `ports/rt-thread/QPC_RTThread_Tech_Summary.md`。


## 8. QP/C 原始仓库资源

- GitHub 仓库：https://github.com/QuantumLeaps/qpc
- QP/C 在线文档：https://www.state-machine.com/qpc
- Getting Started 指南：https://state-machine.com/doc/AN_Getting_Started_with_QPC.pdf


## 9. 许可与支持

QP/C 部分采用双重许可：GPLv3（开源）或 Quantum Leaps 商业闭源授权。
SafeQP 系列仅提供商业授权。

本移植包源码遵循 MIT 许可协议，详见 [LICENSE](./LICENSE)。

如需商业授权、技术支持或认证工具包，请联系 info@state-machine.com。
