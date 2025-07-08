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

/* Optional: Enable specific performance test features */
#define QPC_PERF_USE_DWT_CYCLES        /* Use DWT cycle counter for timing */
#define QPC_PERF_ENABLE_IDLE_HOOK      /* Enable idle hook for CPU measurement */
#define QPC_PERF_MAX_THREADS    10     /* Maximum concurrent test threads */

/* Memory pool sizes for performance events */
#define QPC_PERF_LATENCY_POOL_SIZE     10
#define QPC_PERF_THROUGHPUT_POOL_SIZE  20
#define QPC_PERF_JITTER_POOL_SIZE      15
#define QPC_PERF_IDLE_CPU_POOL_SIZE    10
#define QPC_PERF_MEMORY_POOL_SIZE      15
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