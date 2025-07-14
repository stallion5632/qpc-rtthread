/*============================================================================
* Product: Performance Tests Application - Main Implementation
* Last updated for version 7.2.0
* Last updated on  2024-12-19
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
#include "app_main.h"
#include "counter_ao.h"
#include "timer_ao.h"
#include "logger_ao.h"
#include "bsp.h"
#include <finsh.h>
#include <stdio.h>

Q_DEFINE_THIS_FILE

/*==========================================================================*/
/* Module-level definitions */
/*==========================================================================*/

/* QF publish-subscribe table */
#define MAX_PUB_SIG     (32U)
static QSubscrList l_subscrSto[MAX_PUB_SIG];

/* Event storage for Active Objects */
static QEvt const *l_counterQueueSto[COUNTER_QUEUE_SIZE];
static QEvt const *l_timerQueueSto[TIMER_QUEUE_SIZE];
static QEvt const *l_loggerQueueSto[LOGGER_QUEUE_SIZE];

/* Event pools */
#define SMALL_EVENT_SIZE    sizeof(QEvt)
#define MEDIUM_EVENT_SIZE   sizeof(CounterUpdateEvt)
#define LARGE_EVENT_SIZE    sizeof(LogEvt)

static QF_MPOOL_EL(QEvt) l_smlPoolSto[20U];
static QF_MPOOL_EL(CounterUpdateEvt) l_medPoolSto[10U];
static QF_MPOOL_EL(LogEvt) l_lrgPoolSto[15U];

/* Application state tracking */
static rt_bool_t l_qf_initialized = RT_FALSE;
static rt_bool_t l_aos_started = RT_FALSE;

/*==========================================================================*/
/* Global Synchronization Objects */
/*==========================================================================*/
rt_mutex_t g_log_mutex = RT_NULL;      /* Mutex for thread-safe logging */
rt_mutex_t g_stats_mutex = RT_NULL;    /* Mutex for statistics access */

/*==========================================================================*/
/* Shared Statistics Structure */
/*==========================================================================*/
PerformanceStats g_perf_stats = {
    .counter_updates = 0U,
    .timer_ticks = 0U,
    .timer_reports = 0U,
    .log_messages = 0U,
    .test_duration_ms = 0U,
    .test_running = RT_FALSE
};

/*==========================================================================*/
/* Active Object Instances */
/*==========================================================================*/
QActive * const AO_Counter = (QActive *)CounterAO_getInstance();
QActive * const AO_Timer   = (QActive *)TimerAO_getInstance();
QActive * const AO_Logger  = (QActive *)LoggerAO_getInstance();

/*==========================================================================*/
/* Application Initialization */
/*==========================================================================*/
void PerformanceApp_init(void) {
    /* Initialize BSP first */
    BSP_init();
    
    /* Create synchronization objects */
    if (g_log_mutex == RT_NULL) {
        g_log_mutex = rt_mutex_create("log_mtx", RT_IPC_FLAG_PRIO);
        Q_ASSERT(g_log_mutex != RT_NULL);
    }
    
    if (g_stats_mutex == RT_NULL) {
        g_stats_mutex = rt_mutex_create("stats_mtx", RT_IPC_FLAG_PRIO);
        Q_ASSERT(g_stats_mutex != RT_NULL);
    }
    
    /* Initialize QF only once */
    if (!l_qf_initialized) {
        QF_init();
        
        /* Initialize publish-subscribe */
        QF_psInit(l_subscrSto, Q_DIM(l_subscrSto));
        
        /* Initialize event pools */
        QF_poolInit(l_smlPoolSto, sizeof(l_smlPoolSto), SMALL_EVENT_SIZE);
        QF_poolInit(l_medPoolSto, sizeof(l_medPoolSto), MEDIUM_EVENT_SIZE);
        QF_poolInit(l_lrgPoolSto, sizeof(l_lrgPoolSto), LARGE_EVENT_SIZE);
        
        l_qf_initialized = RT_TRUE;
    }
    
    /* Construct Active Objects */
    CounterAO_ctor();
    TimerAO_ctor();
    LoggerAO_ctor();
    
    /* Reset statistics */
    PerformanceApp_resetStats();
}

/*==========================================================================*/
/* Application Start */
/*==========================================================================*/
int PerformanceApp_start(void) {
    /* Ensure initialization is complete */
    if (!l_qf_initialized) {
        PerformanceApp_init();
    }
    
    /* Start Active Objects only once */
    if (!l_aos_started) {
        /* Start Active Objects with their priorities and event queues */
        QACTIVE_START(AO_Counter,
                      (uint_fast8_t)COUNTER_AO_PRIO,
                      l_counterQueueSto,
                      Q_DIM(l_counterQueueSto),
                      (void *)0,
                      COUNTER_STACK_SIZE,
                      (QEvt *)0);
        
        QACTIVE_START(AO_Timer,
                      (uint_fast8_t)TIMER_AO_PRIO,
                      l_timerQueueSto,
                      Q_DIM(l_timerQueueSto),
                      (void *)0,
                      TIMER_STACK_SIZE,
                      (QEvt *)0);
        
        QACTIVE_START(AO_Logger,
                      (uint_fast8_t)LOGGER_AO_PRIO,
                      l_loggerQueueSto,
                      Q_DIM(l_loggerQueueSto),
                      (void *)0,
                      LOGGER_STACK_SIZE,
                      (QEvt *)0);
        
        l_aos_started = RT_TRUE;
    }
    
    /* Mark test as running */
    rt_mutex_take(g_stats_mutex, RT_WAITING_FOREVER);
    g_perf_stats.test_running = RT_TRUE;
    rt_mutex_release(g_stats_mutex);
    
    /* Post start signals to Active Objects */
    static QEvt const startEvt = { APP_START_SIG, 0U, 0U };
    QACTIVE_POST(AO_Counter, &startEvt, (void *)0);
    QACTIVE_POST(AO_Timer, &startEvt, (void *)0);
    QACTIVE_POST(AO_Logger, &startEvt, (void *)0);
    
    /* Log the test start */
    LoggerAO_logInfo("Performance test started");
    
    return 0;
}

/*==========================================================================*/
/* Application Stop */
/*==========================================================================*/
void PerformanceApp_stop(void) {
    /* Mark test as stopped */
    rt_mutex_take(g_stats_mutex, RT_WAITING_FOREVER);
    g_perf_stats.test_running = RT_FALSE;
    rt_mutex_release(g_stats_mutex);
    
    /* Post stop signals to Active Objects */
    static QEvt const stopEvt = { APP_STOP_SIG, 0U, 0U };
    QACTIVE_POST(AO_Counter, &stopEvt, (void *)0);
    QACTIVE_POST(AO_Timer, &stopEvt, (void *)0);
    QACTIVE_POST(AO_Logger, &stopEvt, (void *)0);
    
    /* Log the test stop */
    LoggerAO_logInfo("Performance test stopped");
}

/*==========================================================================*/
/* Statistics Management */
/*==========================================================================*/
void PerformanceApp_getStats(PerformanceStats *stats) {
    Q_REQUIRE(stats != (PerformanceStats *)0);
    
    rt_mutex_take(g_stats_mutex, RT_WAITING_FOREVER);
    *stats = g_perf_stats;
    rt_mutex_release(g_stats_mutex);
}

void PerformanceApp_resetStats(void) {
    rt_mutex_take(g_stats_mutex, RT_WAITING_FOREVER);
    g_perf_stats.counter_updates = 0U;
    g_perf_stats.timer_ticks = 0U;
    g_perf_stats.timer_reports = 0U;
    g_perf_stats.log_messages = 0U;
    g_perf_stats.test_duration_ms = 0U;
    g_perf_stats.test_running = RT_FALSE;
    rt_mutex_release(g_stats_mutex);
}

/*==========================================================================*/
/* State Query Functions */
/*==========================================================================*/
rt_bool_t PerformanceApp_isQFInitialized(void) {
    return l_qf_initialized;
}

rt_bool_t PerformanceApp_areAOsStarted(void) {
    return l_aos_started;
}

/*==========================================================================*/
/* RT-Thread MSH Commands */
/*==========================================================================*/
int perf_test_start_cmd(int argc, char** argv) {
    (void)argc; /* Suppress unused parameter warning */
    (void)argv; /* Suppress unused parameter warning */
    
    rt_kprintf("Starting performance test...\n");
    int result = PerformanceApp_start();
    if (result == 0) {
        rt_kprintf("Performance test started successfully\n");
    } else {
        rt_kprintf("Failed to start performance test\n");
    }
    return result;
}
MSH_CMD_EXPORT(perf_test_start_cmd, Start performance test);

int perf_test_stop_cmd(int argc, char** argv) {
    (void)argc; /* Suppress unused parameter warning */
    (void)argv; /* Suppress unused parameter warning */
    
    rt_kprintf("Stopping performance test...\n");
    PerformanceApp_stop();
    rt_kprintf("Performance test stopped\n");
    return 0;
}
MSH_CMD_EXPORT(perf_test_stop_cmd, Stop performance test);

int perf_test_stats_cmd(int argc, char** argv) {
    (void)argc; /* Suppress unused parameter warning */
    (void)argv; /* Suppress unused parameter warning */
    
    PerformanceStats stats;
    PerformanceApp_getStats(&stats);
    
    rt_kprintf("=== Performance Test Statistics ===\n");
    rt_kprintf("Test running: %s\n", stats.test_running ? "Yes" : "No");
    rt_kprintf("Test duration: %u ms\n", stats.test_duration_ms);
    rt_kprintf("Counter updates: %u\n", stats.counter_updates);
    rt_kprintf("Timer ticks: %u\n", stats.timer_ticks);
    rt_kprintf("Timer reports: %u\n", stats.timer_reports);
    rt_kprintf("Log messages: %u\n", stats.log_messages);
    rt_kprintf("QF initialized: %s\n", PerformanceApp_isQFInitialized() ? "Yes" : "No");
    rt_kprintf("AOs started: %s\n", PerformanceApp_areAOsStarted() ? "Yes" : "No");
    
    return 0;
}
MSH_CMD_EXPORT(perf_test_stats_cmd, Show performance test statistics);

int perf_test_reset_cmd(int argc, char** argv) {
    (void)argc; /* Suppress unused parameter warning */
    (void)argv; /* Suppress unused parameter warning */
    
    rt_kprintf("Resetting performance test statistics...\n");
    PerformanceApp_resetStats();
    LoggerAO_resetCounters();
    rt_kprintf("Statistics reset complete\n");
    return 0;
}
MSH_CMD_EXPORT(perf_test_reset_cmd, Reset performance test statistics);