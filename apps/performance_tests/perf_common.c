/*============================================================================
* Product: Performance Test Suite Common Implementation
* Last updated for version 7.2.0
* Last updated on  2024-07-08
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2024 Quantum Leaps, LLC. All rights reserved.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <www.gnu.org/licenses/>.
*
* Contact information:
* <www.state-machine.com/licensing>
* <info@state-machine.com>
============================================================================*/
#include "perf_common.h"

Q_DEFINE_THIS_FILE

/*==========================================================================*/
/* Global Variables - volatile for atomic visibility */
/*==========================================================================*/

/* Thread synchronization flags - volatile for safe cross-thread signaling */
volatile rt_bool_t g_stopProducer = RT_FALSE;
volatile rt_bool_t g_stopLoadThreads = RT_FALSE;

/* Atomic visibility variables - volatile for shared access */
volatile uint32_t g_idle_count = 0;
volatile uint32_t g_latency_measurements = 0;
volatile uint32_t g_throughput_measurements = 0;
volatile uint32_t g_jitter_measurements = 0;
volatile uint32_t g_memory_measurements = 0;

/*==========================================================================*/
/* Configurable Event Pool Sizes */
/*==========================================================================*/

#ifndef QPC_PERF_LATENCY_POOL_SIZE
#define QPC_PERF_LATENCY_POOL_SIZE      20
#endif

#ifndef QPC_PERF_THROUGHPUT_POOL_SIZE
#define QPC_PERF_THROUGHPUT_POOL_SIZE   40
#endif

#ifndef QPC_PERF_JITTER_POOL_SIZE
#define QPC_PERF_JITTER_POOL_SIZE       30
#endif

#ifndef QPC_PERF_IDLE_CPU_POOL_SIZE
#define QPC_PERF_IDLE_CPU_POOL_SIZE     20
#endif

#ifndef QPC_PERF_MEMORY_POOL_SIZE
#define QPC_PERF_MEMORY_POOL_SIZE       25
#endif

/*==========================================================================*/
/* Static Event Pools for Performance Tests */
/*==========================================================================*/

static QF_MPOOL_EL(LatencyEvt) l_latencyPool[QPC_PERF_LATENCY_POOL_SIZE];
static QF_MPOOL_EL(ThroughputEvt) l_throughputPool[QPC_PERF_THROUGHPUT_POOL_SIZE];
static QF_MPOOL_EL(JitterEvt) l_jitterPool[QPC_PERF_JITTER_POOL_SIZE];
static QF_MPOOL_EL(IdleCpuEvt) l_idleCpuPool[QPC_PERF_IDLE_CPU_POOL_SIZE];
static QF_MPOOL_EL(MemoryEvt) l_memoryPool[QPC_PERF_MEMORY_POOL_SIZE];

/*==========================================================================*/
/* DWT Cycle Counter Functions */
/*==========================================================================*/

void PerfCommon_initDWT(void) {
    /* Enable trace in DEMCR */
    DWT_DEMCR |= DWT_DEMCR_TRCENA_Msk;
    
    /* Reset cycle counter */
    DWT_CYCCNT = 0;
    
    /* Enable cycle counter */
    DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void PerfCommon_resetDWT(void) {
    /* Reset cycle counter */
    DWT_CYCCNT = 0;
}

uint32_t PerfCommon_getDWTCycles(void) {
    return DWT_CYCCNT;
}

/*==========================================================================*/
/* Performance Test Utility Functions */
/*==========================================================================*/

void PerfCommon_initTest(void) {
    /* Initialize DWT cycle counter */
    PerfCommon_initDWT();
    
    /* Reset all measurement counters */
    g_latency_measurements = 0;
    g_throughput_measurements = 0;
    g_jitter_measurements = 0;
    g_memory_measurements = 0;
    g_idle_count = 0;
    
    /* Reset thread synchronization flags */
    g_stopProducer = RT_FALSE;
    g_stopLoadThreads = RT_FALSE;
    
    /* Initialize event pools */
    PerfCommon_initEventPools();
}

void PerfCommon_cleanupTest(void) {
    /* Set stop flags to ensure all threads terminate */
    PerfCommon_setStopFlags(RT_TRUE);
    
    /* Wait for threads to complete */
    PerfCommon_waitForThreads();
    
    /* Cleanup event pools */
    PerfCommon_cleanupEventPools();
}

void PerfCommon_printResults(const char *test_name, uint32_t measurements) {
    rt_kprintf("=== %s Performance Test Results ===\n", test_name);
    rt_kprintf("Total measurements: %u\n", measurements);
    rt_kprintf("Idle count: %u\n", g_idle_count);
    rt_kprintf("Test completed successfully\n\n");
}

/*==========================================================================*/
/* Thread Management Functions */
/*==========================================================================*/

void PerfCommon_setStopFlags(rt_bool_t stop) {
    g_stopProducer = stop;
    g_stopLoadThreads = stop;
}

void PerfCommon_waitForThreads(void) {
    /* Give threads time to see stop flags and exit cleanly */
    rt_thread_mdelay(100);
}

/*==========================================================================*/
/* Event Pool Management */
/*==========================================================================*/

void PerfCommon_initEventPools(void) {
    /* Initialize event pools for different test types */
    QF_poolInit(l_latencyPool, sizeof(l_latencyPool), sizeof(LatencyEvt));
    QF_poolInit(l_throughputPool, sizeof(l_throughputPool), sizeof(ThroughputEvt));
    QF_poolInit(l_jitterPool, sizeof(l_jitterPool), sizeof(JitterEvt));
    QF_poolInit(l_idleCpuPool, sizeof(l_idleCpuPool), sizeof(IdleCpuEvt));
    QF_poolInit(l_memoryPool, sizeof(l_memoryPool), sizeof(MemoryEvt));
}

void PerfCommon_cleanupEventPools(void) {
    /* Event pools are automatically cleaned up by QF */
    /* No explicit cleanup needed */
}

/*==========================================================================*/
/* Memory Management Utilities */
/*==========================================================================*/

void* PerfCommon_malloc(rt_size_t size) {
    return rt_malloc(size);
}

void PerfCommon_free(void *ptr) {
    if (ptr != RT_NULL) {
        rt_free(ptr);
    }
}