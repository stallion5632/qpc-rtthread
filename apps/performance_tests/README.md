# QPC-RT-Thread Performance Test Suite

This directory contains a comprehensive performance test suite for the QPC (Quantum Platform for C) framework running on RT-Thread RTOS. The test suite has been restructured to integrate advanced performance tests from backup_old_tests with enhanced reporting capabilities.

## Test Suite Overview

The performance test suite now includes five specialized performance tests, each designed to measure different aspects of system performance:

### Available Tests

1. **Latency Test** (`latency_test.c`)
   - Measures memory access latency using DWT cycle counter
   - Performs 1000 measurements by default
   - Reports min, max, and average latency in cycles
   - Uses hardware cycle counting for precise timing

2. **Throughput Test** (`throughput_test.c`)
   - Tests packet processing throughput using RT-Thread mailboxes
   - Creates producer and consumer threads
   - Measures packets sent/received and processing duration
   - Calculates throughput as packets per cycle

3. **Jitter Test** (`jitter_test.c`)
   - Measures timing jitter using periodic timers
   - Tests timer accuracy with expected 100-cycle intervals
   - Records min, max, and average jitter values
   - Performs 100 measurements by default

4. **Idle CPU Test** (`idle_cpu_test.c`)
   - Measures idle CPU performance and utilization
   - Creates low-priority idle task and monitoring thread
   - Tracks idle counts over measurement periods
   - Reports total and average idle performance

5. **Memory Test** (`memory_test.c`)
   - Tests dynamic memory allocation and deallocation
   - Tracks allocation statistics and memory usage
   - Reports allocation failures and memory fragmentation
   - Measures peak memory usage

### Legacy Tests (Still Available)

6. **CPU Load Test** (`cpu_load.c`)
   - Simple loop-based CPU performance test
   - Baseline performance measurement

7. **Counter AO Test** (`counter_ao_test.c`)
   - QPC Active Object-based counter test
   - Event-driven performance measurement

8. **Timer AO Test** (`timer_ao_test.c`)
   - QPC Active Object timer performance test
   - Periodic event generation and handling

9. **Memory Stress Test** (`mem_stress_test.c`)
   - Memory pressure testing
   - Stress testing memory subsystem

10. **Multithread Test** (`multithread_test.c`)
    - Multi-threading performance test
    - Thread synchronization and coordination

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

### New Framework Commands

The restructured framework provides unified command interface:

```bash
# List all available test cases
perf list

# Start specific tests
perf start latency      # Start latency test
perf start throughput   # Start throughput test
perf start jitter       # Start jitter test
perf start idle_cpu     # Start idle CPU test
perf start memory       # Start memory test

# View detailed test reports
perf report

# Stop specific tests
perf stop latency
perf stop throughput

# Restart tests
perf restart jitter
```

### Enhanced Report Format

The new framework provides detailed reports with key performance metrics:

```
=== Latency Test Results ===
Measurements: 1000
Min latency: 45 cycles
Max latency: 123 cycles
Avg latency: 67 cycles
Total latency: 67000 cycles

=== Throughput Test Results ===
Packets sent: 850
Packets received: 850
Test duration: 1000000 cycles
Throughput: 0 packets/cycle

=== Jitter Test Results ===
Measurements: 100
Expected interval: 100 cycles
Min jitter: 2 cycles
Max jitter: 15 cycles
Avg jitter: 7 cycles

=== Idle CPU Test Results ===
Test duration: 1000000 cycles
Measurements: 100
Total idle count: 500000
Average idle per measurement: 5000

=== Memory Test Results ===
Total allocations: 200
Total frees: 180
Total allocated: 204800 bytes
Total freed: 184320 bytes
Max allocated: 20480 bytes
Allocation failures: 0
```

### Legacy RT-Thread MSH Commands (Deprecated)

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

## Troubleshooting and Known Issues

### Common Issues and Solutions

1. **DWT Counter Shows Zero Values**
   - **Cause**: DWT (Data Watchpoint and Trace) unit not properly initialized
   - **Solution**: Ensure CoreDebug_DEMCR register is set to enable DWT
   - **Check**: Look for DWT initialization logs: `[Test] DWT initialized, CTRL=0x########`

2. **Thread Assertion Failures**
   - **Cause**: Manually deleting threads that have already exited
   - **Solution**: Let threads exit naturally, avoid `rt_thread_delete` on completed threads
   - **Fixed**: All tests now use natural thread termination

3. **Memory Test Format Errors**
   - **Cause**: 64-bit format specifiers on 32-bit systems
   - **Solution**: Use 32-bit format with type casting
   - **Fixed**: Changed `%llu` to `%u` with `(rt_uint32_t)` casting

4. **Jitter Test Unrealistic Values**
   - **Cause**: Incorrect time interval calculations
   - **Solution**: Proper cycle-to-tick conversion and value normalization
   - **Fixed**: Added realistic jitter calculation with bounds checking

### Performance Optimization Tips

1. **DWT Precision**: Enable DWT for high-precision cycle counting
2. **Thread Priorities**: Adjust priorities based on test requirements
3. **Memory Configuration**: Increase heap size for memory-intensive tests
4. **Timing Intervals**: Adjust measurement intervals for system load

### Debug Information

Enable detailed logging by checking initialization messages:
```
[Latency Test] DWT initialized, CTRL=0x########
[Throughput Test] DWT initialized, CTRL=0x########
[Jitter Test] DWT initialized, CTRL=0x########
```

Valid DWT CTRL register should show non-zero value indicating proper initialization.
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
