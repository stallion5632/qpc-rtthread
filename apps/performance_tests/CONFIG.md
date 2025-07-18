# QPC Performance Tests Configuration Example

This file shows how to enable the QPC performance tests in your RT-Thread project.

## RT-Thread Configuration (rtconfig.h)

Add the following configuration options to enable the performance tests:

```c
/* Enable QPC performance tests */
#define QPC_USING_PERFORMANCE_TESTS

/* Required RT-Thread features */
#define RT_USING_MAILBOX
#define RT_USING_FINSH
#define RT_USING_HEAP

# Performance Tests Configuration Guide

## Overview

The performance test suite has been enhanced with professional-grade tests integrated from backup_old_tests. This document provides configuration and usage guidance.

## Test Configuration

### Hardware Requirements

- **ARM Cortex-M with DWT**: Tests use Data Watchpoint and Trace (DWT) cycle counter for precise timing
- **Minimum RAM**: 64KB recommended for memory tests
- **RT-Thread Support**: Mailbox, Timer, and Thread APIs required

### Software Dependencies

```c
// Required RT-Thread components
#define RT_USING_MAILBOX        1
#define RT_USING_TIMER          1
#define RT_USING_FINSH          1
#define RT_USING_MUTEX          1

// QPC Framework
#define PKG_USING_QPC           1
```

## Test Parameters

### Latency Test Configuration
```c
#define LATENCY_TEST_MEASUREMENTS       1000    // Number of measurements
#define LATENCY_TEST_MIN_EXPECTED       45      // Expected min latency (cycles)
#define LATENCY_TEST_MAX_EXPECTED       123     // Expected max latency (cycles)
#define LATENCY_TEST_AVG_EXPECTED       67      // Expected avg latency (cycles)
```

### Throughput Test Configuration
```c
#define THROUGHPUT_TEST_PACKETS         850     // Target packet count
#define THROUGHPUT_TEST_DURATION_CYCLES 1000000 // Test duration (cycles)
```

### Jitter Test Configuration
```c
#define JITTER_TEST_MEASUREMENTS        100     // Number of measurements
#define JITTER_TEST_EXPECTED_INTERVAL   100     // Expected interval (cycles)
#define JITTER_TEST_MIN_EXPECTED        2       // Expected min jitter (cycles)
#define JITTER_TEST_MAX_EXPECTED        15      // Expected max jitter (cycles)
#define JITTER_TEST_AVG_EXPECTED        7       // Expected avg jitter (cycles)
```

### Idle CPU Test Configuration
```c
#define IDLE_CPU_TEST_DURATION_CYCLES   1000000 // Test duration (cycles)
#define IDLE_CPU_TEST_MEASUREMENTS      100     // Number of measurements
#define IDLE_CPU_EXPECTED_TOTAL         500000  // Expected total idle count
#define IDLE_CPU_EXPECTED_AVG           5000    // Expected avg idle per measurement
```

### Memory Test Configuration
```c
#define MEMORY_TEST_ALLOCATIONS         200     // Total allocations
#define MEMORY_TEST_FREES               180     // Total frees
#define MEMORY_TEST_TOTAL_ALLOCATED     204800  // Total allocated bytes
#define MEMORY_TEST_TOTAL_FREED         184320  // Total freed bytes
#define MEMORY_TEST_MAX_ALLOCATED       20480   // Max allocated bytes
#define MEMORY_TEST_FAILURES            0       // Expected allocation failures
```

## Usage Examples

### Basic Test Execution
```bash
# List available tests
perf list

# Run individual tests
perf start latency
perf start throughput
perf start jitter
perf start idle_cpu
perf start memory

# View detailed reports
perf report

# Stop running tests
perf stop latency
```

### Validation Commands
```bash
# Validate test integration
cmd_perf_validate

# Run quick validation test
cmd_perf_quick_test
```

### Expected Report Format
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

## Troubleshooting

### Common Issues

1. **DWT Not Available**: Ensure target supports ARM Cortex-M DWT unit
2. **Memory Allocation Failures**: Increase heap size or reduce test parameters
3. **Thread Creation Failures**: Check available system resources
4. **Timer Precision Issues**: Verify system tick configuration

### Debug Commands
```bash
# Check test registration
cmd_perf_validate

# Run quick verification
cmd_perf_quick_test

# Check system resources
free
ps
```

## Integration Notes

- Tests automatically register using `PERF_TEST_REG()` macro
- Framework supports up to 16 concurrent test cases
- Each test runs in its own thread with configurable priority
- Statistics are automatically collected and reported
- Thread-safe operation with mutex protection

## Performance Tuning

### Thread Priorities
```c
#define PERF_TEST_PRIORITY              (RT_THREAD_PRIORITY_MAX / 2)
#define IDLE_TASK_PRIORITY              (RT_THREAD_PRIORITY_MAX - 1)
```

### Memory Configuration
```c
#define PERF_TEST_STACK_SIZE            2048
#define MAILBOX_SIZE                    32
```

### Timing Configuration
```c
#define PERF_TEST_TIMEOUT_MS            10000
#define PERF_TEST_CHECK_INTERVAL_MS     100
```
```

## Build Configuration (SConscript)

The main SConscript file already includes the performance tests when `QPC_USING_PERFORMANCE_TESTS` is defined. No additional build configuration is needed.

## Usage

### Command Line (RT-Thread MSH)

```bash
# Get test information
PerfTestSuite_printInfo

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
```

### Programmatic Usage

```c
#include "perf_common.h"

void run_performance_tests(void) {
    // Initialize test infrastructure
    PerfCommon_initTest();

    // Run specific test
    LatencyTest_start();

    // Wait for completion or stop manually
    LatencyTest_stop();

    // Cleanup
    PerfCommon_cleanupTest();
}
```

## Hardware Requirements

- ARM Cortex-M processor with DWT (Data Watchpoint and Trace) support
- Minimum 32KB RAM for test execution
- RT-Thread RTOS with mailbox and heap support

## Expected Output

Each test provides detailed performance metrics:

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

## Troubleshooting

### Common Issues

1. **Compilation errors**: Ensure RT-Thread headers are in include path
2. **Missing symbols**: Enable required RT-Thread features (mailbox, heap, finsh)
3. **Runtime crashes**: Check stack sizes for test threads (minimum 2KB recommended)
4. **No DWT support**: Some ARM cores don't support DWT; timing will use RT-Thread ticks
5. **Memory allocation failures**: Increase heap size in RT-Thread configuration

### Debug Features

Enable debug output by defining:
```c
#define QPC_PERF_DEBUG_OUTPUT
```

This will provide additional diagnostic information during test execution.
