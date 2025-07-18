/*============================================================================
* Product: QF Dispatcher Metrics and Policy Management Utilities
* Last updated for version 7.3.0
* Last updated on  2024-07-11
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

/*==========================================================================*/
/* QF Metrics and Policy Management Functions */
/*==========================================================================*/

void QF_printDispatcherMetrics(void) {
    QF_DispatcherMetrics const *metrics = QF_getDispatcherMetrics();
    QF_DispatcherStrategy const *policy = QF_getDispatcherPolicy();
    
    rt_kprintf("==================================================\n");
    rt_kprintf("QF Dispatcher Metrics and Policy Information\n");
    rt_kprintf("==================================================\n");
    
    /* Policy Information */
    rt_kprintf("Current Policy: %s\n", 
               (policy == &QF_defaultStrategy) ? "DEFAULT" :
               (policy == &QF_highPerfStrategy) ? "HIGH_PERFORMANCE" : "CUSTOM");
    
    /* Dispatcher Metrics */
    rt_kprintf("\n--- Dispatcher Performance ---\n");
    rt_kprintf("Dispatch Cycles: %u\n", metrics->dispatchCycles);
    rt_kprintf("Events Processed: %u\n", metrics->eventsProcessed);
    rt_kprintf("Max Batch Size: %u\n", metrics->maxBatchSize);
    rt_kprintf("Avg Batch Size: %u\n", metrics->avgBatchSize);
    rt_kprintf("Max Queue Depth: %u\n", metrics->maxQueueDepth);
    
    /* Event Processing Statistics */
    rt_kprintf("\n--- Event Processing ---\n");
    rt_kprintf("Events Merged: %u\n", metrics->eventsMerged);
    rt_kprintf("Events Dropped: %u\n", metrics->eventsDropped);
    rt_kprintf("Events Retried: %u\n", metrics->eventsRetried);
    rt_kprintf("Post Failures: %u\n", metrics->postFailures);
    
    /* Staging Buffer Statistics */
    rt_kprintf("\n--- Staging Buffer Overflows ---\n");
    rt_kprintf("High Priority: %u\n", metrics->stagingOverflows[0]);
    rt_kprintf("Normal Priority: %u\n", metrics->stagingOverflows[1]);
    rt_kprintf("Low Priority: %u\n", metrics->stagingOverflows[2]);
    rt_kprintf("Total Overflows: %u\n", 
               metrics->stagingOverflows[0] + metrics->stagingOverflows[1] + metrics->stagingOverflows[2]);
    
    /* Calculate efficiency metrics */
    if (metrics->eventsProcessed > 0U) {
        uint32_t mergeRate = (metrics->eventsMerged * 100U) / metrics->eventsProcessed;
        uint32_t dropRate = (metrics->eventsDropped * 100U) / metrics->eventsProcessed;
        uint32_t retryRate = (metrics->eventsRetried * 100U) / metrics->eventsProcessed;
        
        rt_kprintf("\n--- Efficiency Metrics ---\n");
        rt_kprintf("Merge Rate: %u.%u%%\n", mergeRate / 10U, mergeRate % 10U);
        rt_kprintf("Drop Rate: %u.%u%%\n", dropRate / 10U, dropRate % 10U);
        rt_kprintf("Retry Rate: %u.%u%%\n", retryRate / 10U, retryRate % 10U);
    }
    
    rt_kprintf("==================================================\n");
}

void QF_resetDispatcherMetricsCmd(void) {
    QF_resetDispatcherMetrics();
    rt_kprintf("Dispatcher metrics reset successfully.\n");
}

void QF_switchToDefaultPolicy(void) {
    QF_setDispatcherStrategy(&QF_defaultStrategy);
    rt_kprintf("Switched to DEFAULT dispatcher policy.\n");
}

void QF_switchToHighPerfPolicy(void) {
    QF_setDispatcherStrategy(&QF_highPerfStrategy);
    rt_kprintf("Switched to HIGH_PERFORMANCE dispatcher policy.\n");
}

void QF_showPolicyComparison(void) {
    rt_kprintf("==================================================\n");
    rt_kprintf("QF Dispatcher Policy Comparison\n");
    rt_kprintf("==================================================\n");
    
    rt_kprintf("DEFAULT Policy:\n");
    rt_kprintf("  - Merge: Events with same signal\n");
    rt_kprintf("  - Priority: Based on signal value\n");
    rt_kprintf("  - Drop: Never drops events\n");
    rt_kprintf("  - Staging: All events to NORMAL priority\n");
    rt_kprintf("  - Best for: Reliability, simplicity\n");
    
    rt_kprintf("\nHIGH_PERFORMANCE Policy:\n");
    rt_kprintf("  - Merge: Only explicitly marked mergeable events\n");
    rt_kprintf("  - Priority: Uses explicit priority field\n");
    rt_kprintf("  - Drop: Drops non-critical events when queue >80%% full\n");
    rt_kprintf("  - Staging: HIGH/NORMAL/LOW based on priority and flags\n");
    rt_kprintf("  - Best for: Low latency, high throughput\n");
    
    rt_kprintf("\nCommands:\n");
    rt_kprintf("  QF_switchToDefaultPolicy()    - Switch to default policy\n");
    rt_kprintf("  QF_switchToHighPerfPolicy()   - Switch to high-perf policy\n");
    rt_kprintf("  QF_printDispatcherMetrics()   - Show current metrics\n");
    rt_kprintf("  QF_resetDispatcherMetricsCmd() - Reset metrics counters\n");
    rt_kprintf("==================================================\n");
}

void QF_runPolicyBenchmark(void) {
    rt_kprintf("==================================================\n");
    rt_kprintf("QF Policy Performance Benchmark\n");
    rt_kprintf("==================================================\n");
    
    /* Save current policy */
    QF_DispatcherStrategy const *originalPolicy = QF_getDispatcherPolicy();
    
    /* Benchmark default policy */
    rt_kprintf("Testing DEFAULT policy for 3 seconds...\n");
    QF_setDispatcherStrategy(&QF_defaultStrategy);
    QF_resetDispatcherMetrics();
    
    uint32_t startTime = rt_tick_get();
    uint32_t endTime = startTime + rt_tick_from_millisecond(3000);
    
    /* Generate test events for default policy */
    while (rt_tick_get() < endTime) {
        /* Create simple test events */
        QEvt *evt = (QEvt *)QF_newX_(sizeof(QEvt), 0U, Q_USER_SIG + 1);
        if (evt != (QEvt *)0) {
            /* Would normally post to an active object */
            QF_gc(evt);  /* Just clean up for now */
        }
        rt_thread_mdelay(1);
    }
    
    QF_DispatcherMetrics const *defaultMetrics = QF_getDispatcherMetrics();
    uint32_t defaultCycles = defaultMetrics->dispatchCycles;
    uint32_t defaultEvents = defaultMetrics->eventsProcessed;
    
    /* Benchmark high-performance policy */
    rt_kprintf("Testing HIGH_PERFORMANCE policy for 3 seconds...\n");
    QF_setDispatcherStrategy(&QF_highPerfStrategy);
    QF_resetDispatcherMetrics();
    
    startTime = rt_tick_get();
    endTime = startTime + rt_tick_from_millisecond(3000);
    
    /* Generate test events for high-perf policy */
    while (rt_tick_get() < endTime) {
        /* Create extended test events */
        QEvtEx *evt = QF_newEvtEx(Q_USER_SIG + 2, sizeof(QEvtEx), 128U, 0U);
        if (evt != (QEvtEx *)0) {
            /* Would normally post to an active object */
            QF_gc(&evt->super);  /* Just clean up for now */
        }
        rt_thread_mdelay(1);
    }
    
    QF_DispatcherMetrics const *highPerfMetrics = QF_getDispatcherMetrics();
    uint32_t highPerfCycles = highPerfMetrics->dispatchCycles;
    uint32_t highPerfEvents = highPerfMetrics->eventsProcessed;
    
    /* Display comparison results */
    rt_kprintf("\n--- Benchmark Results ---\n");
    rt_kprintf("DEFAULT Policy:\n");
    rt_kprintf("  Dispatch Cycles: %u\n", defaultCycles);
    rt_kprintf("  Events Processed: %u\n", defaultEvents);
    
    rt_kprintf("HIGH_PERFORMANCE Policy:\n");
    rt_kprintf("  Dispatch Cycles: %u\n", highPerfCycles);
    rt_kprintf("  Events Processed: %u\n", highPerfEvents);
    
    if (defaultCycles > 0U && highPerfCycles > 0U) {
        uint32_t cycleImprovement = (defaultCycles > highPerfCycles) 
                                   ? ((defaultCycles - highPerfCycles) * 100U / defaultCycles)
                                   : 0U;
        rt_kprintf("\nPerformance Improvement: %u%% fewer cycles with high-perf policy\n", cycleImprovement);
    }
    
    /* Restore original policy */
    QF_setDispatcherStrategy(originalPolicy);
    rt_kprintf("\nRestored original policy.\n");
    rt_kprintf("==================================================\n");
}

/* RT-Thread MSH command exports */
#ifdef RT_USING_FINSH
#include <finsh.h>
MSH_CMD_EXPORT(QF_printDispatcherMetrics, show QF dispatcher metrics);
MSH_CMD_EXPORT(QF_resetDispatcherMetricsCmd, reset QF dispatcher metrics);
MSH_CMD_EXPORT(QF_switchToDefaultPolicy, switch to default dispatcher policy);
MSH_CMD_EXPORT(QF_switchToHighPerfPolicy, switch to high performance policy);
MSH_CMD_EXPORT(QF_showPolicyComparison, show policy comparison information);
MSH_CMD_EXPORT(QF_runPolicyBenchmark, run quick policy performance benchmark);
#endif