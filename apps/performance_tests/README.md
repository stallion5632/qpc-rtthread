# QPC-RT-Thread Performance Test Suite

This directory contains a comprehensive performance test suite for the QPC (Quantum Platform for C) framework running on RT-Thread RTOS. The test suite is built using QPC Active Objects with proper signal handling, mutex protection, and MISRA C compliance.

## Architecture Overview

The performance test suite follows the QPC Active Object pattern with three main Active Objects:

### Active Objects

1. **CounterAO** (`counter_ao.c`, `counter_ao.h`)
   - Periodically updates a counter value every 100ms
   - Publishes counter update events
   - Tracks performance statistics
   - Implements proper state transitions with Q_ENTRY_SIG and Q_EXIT_SIG handling

2. **TimerAO** (`timer_ao.c`, `timer_ao.h`)
   - Generates timer ticks every 100ms
   - Implements a dedicated reporting state with proper transitions
   - Reports system performance every 1 second
   - Tracks elapsed time and generates periodic reports

3. **LoggerAO** (`logger_ao.c`, `logger_ao.h`)
   - Provides thread-safe logging using RT-Thread mutexes
   - Supports multiple log levels (DEBUG, INFO, WARN, ERROR)
   - Processes log events asynchronously
   - Maintains log statistics with counters

### Application Framework

4. **App Main** (`app_main.c`, `app_main.h`)
   - Ensures QF framework is initialized exactly once
   - Guarantees Active Objects are started only once with clear state checks
   - Provides RT-Thread MSH commands for test control
   - Manages shared statistics with mutex protection

5. **Board Support Package** (`bsp.c`, `bsp.h`)
   - Hardware abstraction layer
   - Performance monitoring using DWT cycle counter
   - LED control simulation
   - System information and memory management

## Key Features Implemented

### 1. Proper QP/C Framework Signal Handling
- **Q_ENTRY_SIG, Q_EXIT_SIG, Q_INIT_SIG**: Properly handled in all state machines
- **User Signals**: Well-defined signal ranges to prevent conflicts
- **Signal Subscription**: Active Objects subscribe only to relevant signals
- **Event Publishing**: Proper event publishing and consumption patterns

### 2. Framework Initialization Guarantees
- **QF Single Initialization**: `PerformanceApp_isQFInitialized()` ensures QF is initialized exactly once
- **AO Single Start**: `PerformanceApp_areAOsStarted()` ensures Active Objects are started only once
- **State Tracking**: Clear state checks prevent multiple initializations
- **Resource Management**: Proper cleanup and state management

### 3. TimerAO Reporting State Implementation
- **Dedicated Reporting State**: `TimerAO_reporting` state for report generation
- **Proper State Transitions**: Clean transitions between running and reporting states
- **Report Generation**: Structured timer reports with statistics
- **Transition Safety**: Proper handling of stop signals during reporting

### 4. Mutex-Protected Logging
- **RT-Thread Mutexes**: `g_log_mutex` for thread-safe console output
- **Statistics Protection**: `g_stats_mutex` for shared data access
- **Atomic Operations**: Safe counter updates and flag access
- **Deadlock Prevention**: Consistent mutex ordering and timeout handling

### 5. MISRA C Compliance
- **Explicit Casting**: All type conversions are explicit
- **No Magic Numbers**: All constants are properly defined
- **Consistent Naming**: CamelCase for functions, snake_case for variables
- **Bracket Style**: Consistent K&R bracket style
- **Parameter Validation**: Null pointer checks and parameter validation
- **Defensive Programming**: Default cases in switch statements

## Signal Definitions

```c
enum PerformanceAppSignals {
    /* Counter AO signals */
    COUNTER_START_SIG = Q_USER_SIG,
    COUNTER_STOP_SIG,
    COUNTER_UPDATE_SIG,
    COUNTER_TIMEOUT_SIG,
    
    /* Timer AO signals */
    TIMER_START_SIG,
    TIMER_STOP_SIG,
    TIMER_TICK_SIG,
    TIMER_REPORT_SIG,
    TIMER_TIMEOUT_SIG,
    
    /* Logger AO signals */
    LOGGER_LOG_SIG,
    LOGGER_FLUSH_SIG,
    LOGGER_TIMEOUT_SIG,
    
    /* Application control signals */
    APP_START_SIG,
    APP_STOP_SIG,
    APP_RESET_SIG
};
```

## Usage

### RT-Thread MSH Commands

```bash
# Start the performance test
perf_test_start_cmd

# Stop the performance test
perf_test_stop_cmd

# Show performance statistics
perf_test_stats_cmd

# Reset performance statistics
perf_test_reset_cmd
```

### Programmatic Interface

```c
#include "app_main.h"

// Initialize the application
PerformanceApp_init();

// Start the performance test
int result = PerformanceApp_start();

// Get statistics
PerformanceStats stats;
PerformanceApp_getStats(&stats);

// Stop the test
PerformanceApp_stop();
```

### Thread-Safe Logging

```c
#include "logger_ao.h"

// Thread-safe logging from any context
LoggerAO_logInfo("Counter value: %u", counter_value);
LoggerAO_logWarn("Performance warning detected");
LoggerAO_logError("Critical error occurred");
```

## Performance Metrics

The test suite tracks the following metrics:

- **Counter Updates**: Number of counter increments
- **Timer Ticks**: Number of timer tick events
- **Timer Reports**: Number of performance reports generated
- **Log Messages**: Total number of log messages processed
- **Test Duration**: Elapsed time in milliseconds
- **Test Status**: Current running state

## Build Configuration

The performance test suite requires the following RT-Thread configuration:

```c
#define PKG_USING_QPC                1
#define RT_USING_MAILBOX            1
#define RT_USING_FINSH              1
#define RT_USING_MUTEX              1
```

## Safety Features

### Memory Safety
- **Event Pool Management**: Proper event allocation and deallocation
- **Null Pointer Checks**: Defensive programming against null pointers
- **Buffer Overflow Protection**: Safe string operations with bounds checking

### Thread Safety
- **Mutex Protection**: All shared data protected by mutexes
- **Atomic Operations**: Safe access to simple data types
- **Event-Driven Communication**: No direct shared memory between AOs

### Resource Management
- **Single Initialization**: Framework initialized exactly once
- **Proper Cleanup**: Clean shutdown and resource deallocation
- **State Management**: Clear state tracking and transitions

## Testing and Validation

The test suite provides comprehensive validation:

1. **State Machine Testing**: All states and transitions are tested
2. **Signal Handling**: Proper signal processing validation
3. **Mutex Protection**: Thread safety verification
4. **Resource Management**: Memory leak detection
5. **Performance Monitoring**: Real-time performance metrics

## Compatibility

- **QPC Framework**: 7.2.0+
- **RT-Thread**: 4.0+ with mutex and mailbox support
- **ARM Cortex-M**: With DWT support for performance monitoring
- **Build Systems**: SCons (RT-Thread standard)

This performance test suite demonstrates best practices for QPC Active Object design with RT-Thread integration, emphasizing safety, reliability, and maintainability.

## Features Implemented

### 1. Unique Signal Definitions
- **Problem**: Cross-test signal collisions causing unreliable measurements
- **Solution**: Defined distinct `Q_USER_SIG` offsets for each test type:
  - `LATENCY_*_SIG`: Q_USER_SIG + 0-9
  - `THROUGHPUT_*_SIG`: Q_USER_SIG + 10-19
  - `JITTER_*_SIG`: Q_USER_SIG + 20-29
  - `IDLE_CPU_*_SIG`: Q_USER_SIG + 30-39
  - `MEMORY_*_SIG`: Q_USER_SIG + 40-49

### 2. Safe Event Timestamp Handling
- **Problem**: Misuse of `poolId_` field for timestamps causing memory corruption
- **Solution**: 
  - Custom event structures (`LatencyEvt`, `ThroughputEvt`, etc.) with dedicated `timestamp` fields
  - Proper DWT (Data Watchpoint and Trace) cycle counter initialization and reset
  - Safe timestamp management at the start of each test

### 3. Active Object Cleanup and Unique Priorities
- **Problem**: Priority conflicts and lingering subscriptions between tests
- **Solution**:
  - Unique QP priorities assigned to each test AO (1-6)
  - Proper unsubscription from signals at test completion
  - Clean AO termination preventing resource leaks

### 4. Thread Synchronization and Termination
- **Problem**: Unsafe thread deletion in timer callbacks causing crashes
- **Solution**:
  - `volatile` flags (`g_stopProducer`, `g_stopLoadThreads`) for safe cross-thread signaling
  - Thread deletion moved to main test function after setting stop flags
  - Proper delay (`PerfCommon_waitForThreads()`) before thread deletion
  - Clean thread exit handling

### 5. Atomic Variable Visibility
- **Problem**: Race conditions in shared variable access
- **Solution**: Variables requiring atomic visibility declared `volatile`:
  - `g_idle_count`
  - `g_latency_measurements`
  - `g_throughput_measurements`
  - `g_jitter_measurements`
  - `g_memory_measurements`

## Test Types

### 1. Latency Test (`latency_test.c`)
Measures event processing latency using custom `LatencyEvt` structures with dedicated timestamp fields. Tests event round-trip time from posting to processing.

### 2. Throughput Test (`throughput_test.c`)
Measures system throughput using producer-consumer pattern with proper thread synchronization. Tests event processing rate under load.

### 3. Jitter Test (`jitter_test.c`)
Measures timing jitter in periodic events using load threads to introduce realistic system stress. Tests timing consistency under varying load conditions.

### 4. Idle CPU Test (`idle_cpu_test.c`)
Measures CPU idle time and utilization using a proper idle hook mechanism. Tests system efficiency and resource usage.

### 5. Memory Test (`memory_test.c`)
Measures memory allocation/deallocation performance with proper tracking and cleanup. Tests memory management efficiency and leak detection.

## Architecture

### Common Infrastructure (`perf_common.c/h`)
- DWT cycle counter management
- Event pool initialization and cleanup
- Thread synchronization utilities
- Memory management wrappers
- Shared volatile variables

### Test Runner (`perf_test_suite.c`)
- Unified test execution interface
- Individual test control
- Result reporting
- RT-Thread MSH command integration

## Usage

### Individual Tests
```c
// Run individual tests
LatencyTest_start();
ThroughputTest_start();
JitterTest_start();
IdleCpuTest_start();
MemoryTest_start();
```

### Test Suite
```c
// Run all tests sequentially
PerfTestSuite_runAll();

// Get test information
PerfTestSuite_printInfo();

// Stop all running tests
PerfTestSuite_stopAll();
```

### RT-Thread MSH Commands
```bash
# Run all tests
PerfTestSuite_runAll

# Run individual tests
PerfTestSuite_runLatency
PerfTestSuite_runThroughput
PerfTestSuite_runJitter
PerfTestSuite_runIdleCpu
PerfTestSuite_runMemory

# Stop all tests
PerfTestSuite_stopAll

# Get help
PerfTestSuite_printInfo
```

## Build Integration

Add the performance test files to your RT-Thread project:

1. Include the `apps/performance_tests/` directory in your build path
2. Add the source files to your SConscript or Makefile
3. Enable RT-Thread FINSH/MSH for command-line interface
4. Ensure QPC framework is properly integrated

## Safety Features

- **Memory Safety**: Custom event structures prevent `poolId_` misuse
- **Thread Safety**: Volatile flags for safe cross-thread communication
- **Resource Cleanup**: Proper unsubscription and thread termination
- **Isolation**: Unique priorities and signal ranges prevent cross-test interference
- **Timing Safety**: DWT cycle counter initialization and reset for accurate measurements

## Best Practices Implemented

1. **Minimal Changes**: Focused improvements addressing specific issues
2. **Thread Safety**: Proper synchronization and termination patterns
3. **Memory Safety**: Safe event handling and allocation tracking
4. **Measurement Accuracy**: Proper timing infrastructure and signal isolation
5. **Resource Management**: Clean startup/shutdown procedures
6. **Error Handling**: Graceful failure handling and recovery

## Testing

Each test runs for 10 seconds by default and provides detailed measurements:
- Latency: Min/Max/Average latency in cycles
- Throughput: Packets per cycle, total data processed
- Jitter: Timing variation statistics
- Idle CPU: CPU utilization and idle time
- Memory: Allocation patterns and efficiency

## Compatibility

- **QPC Framework**: 7.2.0+
- **RT-Thread**: 4.0+
- **ARM Cortex-M**: With DWT support
- **Build Systems**: SConscript, Makefile, IAR, Keil

This performance test suite provides a solid foundation for QPC-RT-Thread performance analysis while maintaining thread safety, memory safety, and measurement accuracy.