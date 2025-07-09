/*============================================================================
* Product: Performance Test Suite for QPC-RT-Thread
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
#ifndef PERF_COMMON_H_
#define PERF_COMMON_H_

#include "qpc.h"
#include <rtthread.h>

/*==========================================================================*/
/* Performance Test Signal Definitions - Using distinct Q_USER_SIG offsets */
/*==========================================================================*/

enum PerformanceTestSignals {
    /* Latency test signals */
    LATENCY_START_SIG = Q_USER_SIG,
    LATENCY_END_SIG,
    LATENCY_MEASURE_SIG,
    LATENCY_TIMEOUT_SIG,
    LATENCY_STOP_SIG,
    LATENCY_SYNC_SIG,         /* Signal to trigger semaphore wait */
    LATENCY_SYNC_DONE_SIG,    /* Signal when semaphore is available */
    
    /* Throughput test signals */
    THROUGHPUT_START_SIG = Q_USER_SIG + 10,
    THROUGHPUT_SEND_SIG,
    THROUGHPUT_RECV_SIG, 
    THROUGHPUT_TIMEOUT_SIG,
    THROUGHPUT_STOP_SIG,
    THROUGHPUT_SYNC_SIG,         /* Signal to trigger semaphore wait */
    THROUGHPUT_SYNC_DONE_SIG,    /* Signal when semaphore is available */
    THROUGHPUT_READY_SIG,        /* Signal to check consumer readiness */
    THROUGHPUT_READY_DONE_SIG,   /* Signal when consumer is ready */
    
    /* Jitter test signals */
    JITTER_START_SIG = Q_USER_SIG + 20,
    JITTER_MEASURE_SIG,
    JITTER_TIMER_SIG,
    JITTER_TIMEOUT_SIG,
    JITTER_STOP_SIG,
    
    /* Idle CPU test signals */
    IDLE_CPU_START_SIG = Q_USER_SIG + 30,
    IDLE_CPU_MEASURE_SIG,
    IDLE_CPU_TIMEOUT_SIG,
    IDLE_CPU_STOP_SIG,
    
    /* Memory test signals */
    MEMORY_START_SIG = Q_USER_SIG + 40,
    MEMORY_ALLOC_SIG,
    MEMORY_FREE_SIG,
    MEMORY_MEASURE_SIG,
    MEMORY_TIMEOUT_SIG,
    MEMORY_STOP_SIG,
    
    MAX_PERF_SIG
};

/*==========================================================================*/
/* Performance Test Event Structures - Safe timestamp handling */
/*==========================================================================*/

/* Custom event structure for latency measurements */
typedef struct {
    QEvt super;
    uint32_t timestamp;     /* dedicated timestamp field */
    uint32_t sequence_id;   /* for tracking event order */
} LatencyEvt;

/* Custom event structure for throughput measurements */
typedef struct {
    QEvt super;
    uint32_t timestamp;
    uint32_t data_size;
    uint32_t packet_id;
} ThroughputEvt;

/* Custom event structure for jitter measurements */
typedef struct {
    QEvt super;
    uint32_t timestamp;
    uint32_t expected_time;
    uint32_t actual_time;
} JitterEvt;

/* Custom event structure for idle CPU measurements */
typedef struct {
    QEvt super;
    uint32_t timestamp;
    uint32_t idle_count;
} IdleCpuEvt;

/* Custom event structure for memory measurements */
typedef struct {
    QEvt super;
    uint32_t timestamp;
    uint32_t alloc_size;
    void *ptr;
} MemoryEvt;

/*==========================================================================*/
/* Performance Test Priorities - Unique QP priorities for each test AO */
/*==========================================================================*/

enum PerformanceTestPriorities {
    LATENCY_AO_PRIO = 1,
    THROUGHPUT_PRODUCER_PRIO = 2,
    THROUGHPUT_CONSUMER_PRIO = 3,
    JITTER_AO_PRIO = 4,
    IDLE_CPU_AO_PRIO = 5,
    MEMORY_AO_PRIO = 6,
    
    /* Load test threads - lower priority */
    LOAD_THREAD_PRIO = 10
};

/*==========================================================================*/
/* Common Performance Test Definitions */
/*==========================================================================*/

/* DWT (Data Watchpoint and Trace) cycle counter definitions */
#define DWT_CTRL        (*(volatile uint32_t*)0xE0001000)
#define DWT_CYCCNT      (*(volatile uint32_t*)0xE0001004)
#define DWT_DEMCR       (*(volatile uint32_t*)0xE000EDFC)

#define DWT_CTRL_CYCCNTENA_Pos      0U
#define DWT_CTRL_CYCCNTENA_Msk      (1UL << DWT_CTRL_CYCCNTENA_Pos)
#define DWT_DEMCR_TRCENA_Pos        24U
#define DWT_DEMCR_TRCENA_Msk        (1UL << DWT_DEMCR_TRCENA_Pos)

/* Thread synchronization flags - volatile for safe cross-thread signaling */
extern volatile rt_bool_t g_stopProducer;
extern volatile rt_bool_t g_stopLoadThreads;

/* Atomic visibility variables - volatile for shared access */
extern volatile uint32_t g_idle_count;
extern volatile uint32_t g_latency_measurements;
extern volatile uint32_t g_throughput_measurements;
extern volatile uint32_t g_jitter_measurements;
extern volatile uint32_t g_memory_measurements;

/*==========================================================================*/
/* Common Performance Test Functions */
/*==========================================================================*/

/* DWT cycle counter functions */
void PerfCommon_initDWT(void);
void PerfCommon_resetDWT(void);
uint32_t PerfCommon_getDWTCycles(void);

/* Performance test utility functions */
void PerfCommon_initTest(void);
void PerfCommon_cleanupTest(void);
void PerfCommon_printResults(const char *test_name, uint32_t measurements);

/* Thread management functions */
void PerfCommon_setStopFlags(rt_bool_t stop);
void PerfCommon_waitForThreads(void);

/* Event pool management - selective initialization */
void PerfCommon_initEventPools(void);           /* Initialize all pools (legacy) */
void PerfCommon_initLatencyPool(void);          /* Initialize latency pool only */
void PerfCommon_initThroughputPool(void);       /* Initialize throughput pool only */
void PerfCommon_initJitterPool(void);           /* Initialize jitter pool only */
void PerfCommon_initIdleCpuPool(void);          /* Initialize idle CPU pool only */
void PerfCommon_initMemoryPool(void);           /* Initialize memory pool only */
void PerfCommon_cleanupEventPools(void);

/* Memory management utilities */
void* PerfCommon_malloc(rt_size_t size);
void PerfCommon_free(void *ptr);

#endif /* PERF_COMMON_H_ */