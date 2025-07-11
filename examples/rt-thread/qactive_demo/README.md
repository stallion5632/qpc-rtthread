# QXK Demo for RT-Thread

This directory contains a comprehensive demonstration of the QXK (preemptive dual-mode kernel) running on RT-Thread.

## Overview

The QXK demo showcases the integration of QPC's active objects (QActive) with QXK's extended threads (QXThread) in an RT-Thread environment. The demo implements a sensor data processing system with background monitoring.

## Components

### Active Objects (QActive)
1. **Sensor AO**: Periodically reads sensor data and publishes it
2. **Processor AO**: Processes received sensor data and generates results

### Extended Threads (QXThread)  
1. **Worker Thread**: Handles background processing work from the processor
2. **Monitor Thread**: Performs periodic system health monitoring

## Architecture

```
┌─────────────────┐    ┌─────────────────┐
│   Sensor AO     │───▶│  Processor AO   │
│  (Priority 1)   │    │  (Priority 2)   │
└─────────────────┘    └─────────────────┘
                                │
                                ▼
┌─────────────────┐    ┌─────────────────┐
│  Worker Thread  │◀───┤  Monitor Thread │
│  (Priority 3)   │    │  (Priority 4)   │
└─────────────────┘    └─────────────────┘
```

## Key Features

- **Dual-mode architecture**: Combines cooperative QActive objects with preemptive QXThread extensions
- **Event-driven communication**: Uses QPC's publish-subscribe mechanism for inter-object communication
- **Blocking operations**: Demonstrates QXThread's ability to perform blocking operations (QXThread_delay, QXThread_queueGet)
- **RT-Thread integration**: Automatic initialization via INIT_APP_EXPORT
- **Manual control**: MSH command support for manual start/stop

## Usage

### Automatic Startup
The demo automatically starts when RT-Thread boots if `QPC_USING_QXK_DEMO` is enabled in the configuration.

### Manual Control
Use the RT-Thread MSH (shell) command:
```
msh> qxk_demo_start
```

### Expected Output
```
QXK Demo: Started - 2 QActive objects + 2 QXThread extensions
Sensor: Starting periodic sensor readings
Processor: Idle, waiting for data
Worker: Thread started
Monitor: Thread started
Sensor: Reading 1, data = 266
Processor: Received sensor data = 266
Processor: Processing data (count: 1)
Worker: Processing work ID 1 (total: 1)
Monitor: System check #1 - All systems operational
Worker: Work ID 1 completed
...
```

## Configuration

To enable the QXK demo, add the following to your RT-Thread configuration:

```c
#define QPC_USING_QXK_DEMO          1
#define QPC_USING_QXK               1
#define PKG_USING_QPC               1
#define RT_USING_FINSH              1
#define RT_USING_MAILBOX            1
```

## Build Integration

The demo is integrated into the QPC build system via:
- **SConscript**: Automatically included when `QPC_USING_QXK_DEMO` is enabled
- **Dependencies**: Requires QPC with QXK kernel support
- **RT-Thread**: Uses standard RT-Thread build system

## Files

- `main.c`: Main demo implementation with all components
- `qxk_demo.h`: Header file with signal definitions and prototypes
- `SConscript`: Build configuration for RT-Thread SCons
- `README.md`: This documentation file

## Learning Points

This demo illustrates:
1. How to create and start QXThread extension threads
2. Proper use of QXK APIs (QXThread_ctor, QXTHREAD_START, QXThread_delay)
3. Event-driven communication between QActive objects and QXThread extensions
4. Integration with RT-Thread's initialization and shell systems
5. Best practices for mixed cooperative/preemptive scheduling

## See Also

- [QPC Documentation](https://www.state-machine.com/qpc)
- [QXK Kernel Guide](https://www.state-machine.com/qpc/qxk.html)
- [RT-Thread Documentation](https://www.rt-thread.org/document/site/)