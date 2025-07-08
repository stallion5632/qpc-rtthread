# QPC-RT-Thread Performance Test Suite

This directory contains a comprehensive performance test suite for the QPC (Quantum Platform for C) framework running on RT-Thread RTOS. The test suite addresses critical performance measurement concerns and implements best practices for thread safety, memory management, and measurement accuracy.

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