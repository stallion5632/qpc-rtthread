# QActive Demo for RT-Thread

This directory contains a comprehensive demonstration of QPC Active Objects (QActive) running on RT-Thread with native RT-Thread integration.

## Overview

The QActive demo showcases the integration of QPC's active objects (QActive) with RT-Thread native threads in a real-world IoT gateway scenario. The demo implements an industrial data logging system that demonstrates practical integration patterns between QPC/QActive and RT-Thread domains.

## Architecture

### QActive Components (QPC Framework)
1. **Sensor AO**: Periodically reads sensor data and publishes it
2. **Processor AO**: Processes received sensor data, handles configuration updates
3. **Worker AO**: Handles background data compression and processing
4. **Monitor AO**: Performs periodic system health monitoring

### RT-Thread Components (Native RT-Thread)
1. **Storage Thread**: Manages local data storage operations
2. **Shell Thread**: Provides RT-Thread MSH commands for system control

### Synchronization Objects
- **Mutex**: RT-Thread mutex for shared configuration protection
- **Semaphore**: RT-Thread semaphore for storage coordination
- **Event Set**: RT-Thread event set for system-wide notifications

## Data Flow

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Sensor AO     │───▶│  Processor AO   │───▶│  Worker Thread  │
│  (QXK Priority  │    │  (QXK Priority  │    │  (QXK Priority  │
│      1)         │    │      2)         │    │      3)         │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                │                       │
                                │                       ▼
                                │              ┌─────────────────┐
                                │              │ Network Thread  │
                                │              │ (RT-Thread P10) │
                                │              └─────────────────┘
                                │                       │
                                │                       ▼
                                │              ┌─────────────────┐
                                │              │   "Cloud"       │
                                │              │  Transmission   │
                                │              └─────────────────┘
                                │
                                ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Monitor Thread │◀──▶│ Storage Thread  │◀───│  Shell Thread   │
│  (QXK Priority  │    │ (RT-Thread P11) │    │ (RT-Thread P12) │
│      4)         │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## Key Integration Features

### QXK → RT-Thread Communication
- **Data Transfer**: Worker thread sends processed data to Network thread via RT-Thread message queue
- **Storage Coordination**: Worker thread triggers storage operations via RT-Thread semaphore
- **System Events**: QXK components signal system-wide events via RT-Thread event set

### RT-Thread → QXK Communication  
- **Configuration Updates**: Network thread sends configuration changes to Processor AO via QPC events
- **Health Coordination**: Storage thread coordinates with Monitor thread via shared variables protected by RT-Thread mutex

### Bidirectional Integration
- **Statistics Sharing**: Both domains update shared statistics protected by RT-Thread mutex
- **Event Coordination**: System-wide events (health checks, errors) are coordinated via RT-Thread event set
- **MSH Commands**: RT-Thread shell commands control QXK components in real-time

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

To enable the QXK demo with RT-Thread integration, add the following to your RT-Thread configuration:

```c
#define QPC_USING_QXK_DEMO          1
#define QPC_USING_QXK               1
#define PKG_USING_QPC               1
#define RT_USING_FINSH              1
#define RT_USING_MAILBOX            1
#define RT_USING_MUTEX              1
#define RT_USING_SEMAPHORE          1
#define RT_USING_EVENT              1
#define RT_USING_MESSAGEQUEUE       1
```

## Build Integration

The demo is integrated into the QPC build system via:
- **SConscript**: Automatically included when `QPC_USING_QXK_DEMO` is enabled
- **Dependencies**: Requires QPC with QXK kernel support
- **RT-Thread**: Uses standard RT-Thread build system

## Files

- `main.c`: Main demo implementation with QXK components and RT-Thread integration
- `rt_integration.c`: RT-Thread native thread implementations and MSH commands
- `rt_integration.h`: RT-Thread integration interfaces and shared data structures
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
6. **RT-Thread Integration**: How to use RT-Thread synchronization objects (message queue, mutex, semaphore, event set)
7. **Bidirectional Communication**: QXK ↔ RT-Thread data exchange patterns
8. **Resource Sharing**: Safe sharing of data structures between QPC and RT-Thread domains
9. **Real-time Control**: MSH commands for runtime configuration and monitoring
10. **Industrial IoT Patterns**: Practical integration scenarios for embedded systems

## See Also

- [QPC Documentation](https://www.state-machine.com/qpc)
- [QXK Kernel Guide](https://www.state-machine.com/qpc/qxk.html)
- [RT-Thread Documentation](https://www.rt-thread.org/document/site/)

## Implementation Details

### Synchronization Architecture
```
QXK Domain                    RT-Thread Domain
┌─────────────────┐          ┌─────────────────┐
│   Sensor AO     │          │ Network Thread  │
│   Processor AO  │◄────────►│ Storage Thread  │
│   Worker Thread │          │ Shell Thread    │
│   Monitor Thread│          │                 │
└─────────────────┘          └─────────────────┘
        │                              │
        └──────────────────────────────┘
              Shared Resources:
              • RT-Thread Message Queue
              • RT-Thread Mutex  
              • RT-Thread Semaphore
              • RT-Thread Event Set
              • Protected Statistics
```

### Data Flow Example
1. **Sensor Reading**: Sensor AO generates data (e.g., temperature = 25.6°C)
2. **Processing**: Processor AO validates and processes data
3. **Compression**: Worker QXThread compresses data for transmission
4. **Network Transfer**: RT-Thread Network thread simulates cloud upload
5. **Storage**: RT-Thread Storage thread saves data locally
6. **Monitoring**: QXK Monitor and RT-Thread Storage coordinate health checks
7. **Configuration**: RT-Thread Network receives config updates, sends to QXK Processor

### Thread Priorities
- **QXK Sensor AO**: Priority 1 (highest)
- **QXK Processor AO**: Priority 2
- **QXK Worker Thread**: Priority 3
- **QXK Monitor Thread**: Priority 4
- **RT-Thread Network**: Priority 10 (lower)
- **RT-Thread Storage**: Priority 11
- **RT-Thread Shell**: Priority 12 (lowest)

This priority scheme ensures QXK components have higher priority for real-time processing, while RT-Thread components handle background operations.