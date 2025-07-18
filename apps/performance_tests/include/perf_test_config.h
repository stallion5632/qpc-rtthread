/*============================================================================
* Product: Performance Test Configuration
* Last updated for version 7.2.0
*
* This file contains configuration parameters for all performance tests
============================================================================*/
#ifndef PERF_TEST_CONFIG_H_
#define PERF_TEST_CONFIG_H_

/*==========================================================================*/
/* Test Configuration Parameters */
/*==========================================================================*/

/* Latency Test Configuration */
#define LATENCY_TEST_MEASUREMENTS       1000
#define LATENCY_TEST_MIN_EXPECTED       45
#define LATENCY_TEST_MAX_EXPECTED       123
#define LATENCY_TEST_AVG_EXPECTED       67

/* Throughput Test Configuration */
#define THROUGHPUT_TEST_PACKETS         850
#define THROUGHPUT_TEST_DURATION_CYCLES 1000000

/* Jitter Test Configuration */
#define JITTER_TEST_MEASUREMENTS        100
#define JITTER_TEST_EXPECTED_INTERVAL   100
#define JITTER_TEST_MIN_EXPECTED        2
#define JITTER_TEST_MAX_EXPECTED        15
#define JITTER_TEST_AVG_EXPECTED        7

/* Idle CPU Test Configuration */
#define IDLE_CPU_TEST_DURATION_CYCLES   1000000
#define IDLE_CPU_TEST_MEASUREMENTS      100
#define IDLE_CPU_EXPECTED_TOTAL         500000
#define IDLE_CPU_EXPECTED_AVG           5000

/* Memory Test Configuration */
#define MEMORY_TEST_ALLOCATIONS         200
#define MEMORY_TEST_FREES               180
#define MEMORY_TEST_TOTAL_ALLOCATED     204800
#define MEMORY_TEST_TOTAL_FREED         184320
#define MEMORY_TEST_MAX_ALLOCATED       20480
#define MEMORY_TEST_FAILURES            0

/* DWT (Data Watchpoint and Trace) Configuration */
#define DWT_CTRL_ADDR                   0xE0001000
#define DWT_CYCCNT_ADDR                 0xE0001004

/* Thread Configuration */
#define PERF_TEST_STACK_SIZE            2048
#define PERF_TEST_PRIORITY              (RT_THREAD_PRIORITY_MAX / 2)
#define PERF_TEST_TIMESLICE             10

/* Test Timeouts */
#define PERF_TEST_TIMEOUT_MS            10000
#define PERF_TEST_CHECK_INTERVAL_MS     100

#endif /* PERF_TEST_CONFIG_H_ */
