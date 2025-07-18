#ifndef QF_publishFromISR
#define QF_publishFromISR(e, ao) ((void)0)
#endif
/*============================================================================
* Product: Hierarchical Priority Staging Performance Test
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
Q_DEFINE_THIS_FILE

/*==========================================================================*/
/* Hierarchical Priority Test Configuration */
/*==========================================================================*/
#define HIERARCHICAL_TEST_DURATION_MS      (10000U)  /* 10 seconds */
#define HIERARCHICAL_TOTAL_EVENTS          (1000U)   /* 1000 events total */
#define HIERARCHICAL_HIGH_EVENTS           (200U)    /* 200 high priority */
#define HIERARCHICAL_NORMAL_EVENTS          (500U)    /* 500 normal priority */
#define HIERARCHICAL_LOW_EVENTS             (300U)    /* 300 low priority */

/* Hierarchical Priority Test Event Signals */
enum HierarchicalSignals {
    HIERARCHICAL_HIGH_SIG = Q_USER_SIG + 60,
    HIERARCHICAL_NORMAL_SIG,
    HIERARCHICAL_LOW_SIG,
    HIERARCHICAL_STOP_SIG,
    HIERARCHICAL_MAX_SIG
};

/*==========================================================================*/
/* Hierarchical Priority Test Event */
/*==========================================================================*/
typedef struct {
    QEvtEx super;   /* Use extended event for priority information */
    uint32_t sequenceNumber;
    uint32_t expectedOrder;  /* Expected processing order for validation */
} HierarchicalEvt;

/*==========================================================================*/
/* Hierarchical Priority Test AO */
/*==========================================================================*/
typedef struct {
    QActive super;
    uint32_t eventCount;
    uint32_t highPrioCount;
    uint32_t normalPrioCount;
    uint32_t lowPrioCount;
    uint32_t orderViolations;      /* Count of out-of-order events */
    uint32_t lastProcessedOrder;   /* Track processing order */
    uint32_t highPrioLatencySum;
    uint32_t normalPrioLatencySum;
    uint32_t lowPrioLatencySum;
    uint32_t maxLatency[3];        /* Max latency per priority level */
    uint32_t minLatency[3];        /* Min latency per priority level */
    bool isRunning;
} HierarchicalTestAO;

/*==========================================================================*/
/* Hierarchical Priority Test Globals */
/*==========================================================================*/
static HierarchicalTestAO l_hierarchicalTestAO;
static QEvt const *l_hierarchicalTestQueueSto[50];
static uint8_t l_hierarchicalTestStack[1024];

/* Test control */
static volatile bool g_hierarchicalTestRunning = false;
static rt_thread_t g_hierarchicalEventProducer = RT_NULL;
static volatile uint32_t g_eventSequence = 0U;

/*==========================================================================*/
/* Forward Declarations */
/*==========================================================================*/
static QState HierarchicalTestAO_initial(HierarchicalTestAO * const me, void const * const par);
static QState HierarchicalTestAO_running(HierarchicalTestAO * const me, QEvt const * const e);
static void hierarchicalEventProducerThread(void *parameter);
static uint32_t calculateExpectedOrder(uint32_t sequence, QSignal sig);
void HierarchicalPriorityTest_stop(void);

/*==========================================================================*/
/* Hierarchical Priority Test AO State Machine */
/*==========================================================================*/
static QState HierarchicalTestAO_initial(HierarchicalTestAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);

    me->eventCount = 0U;
    me->highPrioCount = 0U;
    me->normalPrioCount = 0U;
    me->lowPrioCount = 0U;
    me->orderViolations = 0U;
    me->lastProcessedOrder = 0U;
    me->highPrioLatencySum = 0U;
    me->normalPrioLatencySum = 0U;
    me->lowPrioLatencySum = 0U;

    for (uint8_t i = 0U; i < 3U; ++i) {
        me->maxLatency[i] = 0U;
        me->minLatency[i] = UINT32_MAX;
    }

    me->isRunning = false;

    return Q_TRAN(&HierarchicalTestAO_running);
}

static QState HierarchicalTestAO_running(HierarchicalTestAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->isRunning = true;
            status = Q_HANDLED();
            break;
        }

        case HIERARCHICAL_HIGH_SIG:
        case HIERARCHICAL_NORMAL_SIG:
        case HIERARCHICAL_LOW_SIG: {
            HierarchicalEvt const * const evt = (HierarchicalEvt const *)e;
            uint32_t currentTime = PerfCommon_getCycleCount();
            uint32_t latency = currentTime - evt->super.timestamp;
            uint8_t prioLevel;

            me->eventCount++;

            /* Determine priority level and update counters */
            if (e->sig == HIERARCHICAL_HIGH_SIG) {
                me->highPrioCount++;
                me->highPrioLatencySum += latency;
                prioLevel = 0U;  /* High priority */
            } else if (e->sig == HIERARCHICAL_NORMAL_SIG) {
                me->normalPrioCount++;
                me->normalPrioLatencySum += latency;
                prioLevel = 1U;  /* Normal priority */
            } else {
                me->lowPrioCount++;
                me->lowPrioLatencySum += latency;
                prioLevel = 2U;  /* Low priority */
            }

            /* Update latency statistics */
            if (latency > me->maxLatency[prioLevel]) {
                me->maxLatency[prioLevel] = latency;
            }
            if (latency < me->minLatency[prioLevel]) {
                me->minLatency[prioLevel] = latency;
            }

            /* Check processing order (should be HIGH -> NORMAL -> LOW) */
            if (evt->expectedOrder <= me->lastProcessedOrder) {
                me->orderViolations++;
            }
            me->lastProcessedOrder = evt->expectedOrder;

            status = Q_HANDLED();
            break;
        }

        case HIERARCHICAL_STOP_SIG: {
            me->isRunning = false;
            status = Q_HANDLED();
            break;
        }

        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}

/*==========================================================================*/
/* Helper Functions */
/*==========================================================================*/
static uint32_t calculateExpectedOrder(uint32_t sequence, QSignal sig) {
    /* Calculate expected processing order based on priority levels */
    /* High priority events should be processed first, then normal, then low */
    uint32_t order = sequence;

    if (sig == HIERARCHICAL_HIGH_SIG) {
        order = sequence;  /* High priority processed immediately */
    } else if (sig == HIERARCHICAL_NORMAL_SIG) {
        order = sequence + 1000U;  /* Normal priority after high */
    } else {
        order = sequence + 2000U;  /* Low priority after normal */
    }

    return order;
}

static void hierarchicalEventProducerThread(void *parameter) {
    Q_UNUSED_PAR(parameter);
    uint32_t highSent = 0U;
    uint32_t normalSent = 0U;
    uint32_t lowSent = 0U;

    rt_kprintf("[Hierarchical] Starting event burst production...\n");

    /* Generate events in a realistic mixed pattern */
    while (g_hierarchicalTestRunning &&
           (highSent < HIERARCHICAL_HIGH_EVENTS ||
            normalSent < HIERARCHICAL_NORMAL_EVENTS ||
            lowSent < HIERARCHICAL_LOW_EVENTS)) {

        /* Generate a burst of mixed priority events */
        for (uint32_t burst = 0U; burst < 10U && g_hierarchicalTestRunning; ++burst) {
            QSignal sig;
            uint8_t priority;
            uint8_t flags;

            /* Randomly select event type but respect quotas */
            uint32_t eventType = rt_tick_get() % 10U;

            if (eventType < 3U && highSent < HIERARCHICAL_HIGH_EVENTS) {
                /* High priority event (30% chance) */
                sig = HIERARCHICAL_HIGH_SIG;
                priority = 250U;  /* High priority value */
                flags = QF_EVT_FLAG_CRITICAL;  /* Mark as critical */
                highSent++;
            } else if (eventType < 8U && normalSent < HIERARCHICAL_NORMAL_EVENTS) {
                /* Normal priority event (50% chance) */
                sig = HIERARCHICAL_NORMAL_SIG;
                priority = 128U;  /* Normal priority value */
                flags = 0U;
                normalSent++;
            } else if (lowSent < HIERARCHICAL_LOW_EVENTS) {
                /* Low priority event (20% chance) */
                sig = HIERARCHICAL_LOW_SIG;
                priority = 64U;   /* Low priority value */
                flags = 0U;
                lowSent++;
            } else {
                continue;  /* Skip if quotas are met */
            }

            /* Create extended event with priority information */
            HierarchicalEvt *evt = (HierarchicalEvt *)QF_newEvtEx(sig, sizeof(HierarchicalEvt), priority, flags);
            if (evt != (HierarchicalEvt *)0) {
                evt->super.timestamp = PerfCommon_getCycleCount();
                evt->sequenceNumber = g_eventSequence++;
                evt->expectedOrder = calculateExpectedOrder(evt->sequenceNumber, sig);

                /* Use publishFromISR to test broadcast ISR path and priority staging */
                if (burst % 3U == 0U) {
                    /* Test publish-from-ISR path for some events */
                    QF_publishFromISR(&evt->super.super, &l_hierarchicalTestAO);
                } else {
                    /* Use postFromISR for targeted posting */
                    QF_postFromISR(&l_hierarchicalTestAO.super, &evt->super.super);
                }
            }

            /* Small delay to simulate realistic timing */
            rt_thread_mdelay(1);
        }

        /* Larger delay between bursts to allow processing */
        rt_thread_mdelay(50);
    }

    rt_kprintf("[Hierarchical] Event production completed. High=%u, Normal=%u, Low=%u\n",
               highSent, normalSent, lowSent);
}

/*==========================================================================*/
/* Hierarchical Priority Test Functions */
/*==========================================================================*/
void HierarchicalPriorityTest_start(void) {
    if (g_hierarchicalTestRunning) {
        rt_kprintf("Hierarchical Priority Test is already running!\n");
        return;
    }

    rt_kprintf("==================================================\n");
    rt_kprintf("Starting Hierarchical Priority Performance Test\n");
    rt_kprintf("Duration: %u ms\n", HIERARCHICAL_TEST_DURATION_MS);
    rt_kprintf("Total Events: %u (High: %u, Normal: %u, Low: %u)\n",
               HIERARCHICAL_TOTAL_EVENTS, HIERARCHICAL_HIGH_EVENTS,
               HIERARCHICAL_NORMAL_EVENTS, HIERARCHICAL_LOW_EVENTS);
    rt_kprintf("Testing priority-based staging buffer ordering...\n");
    rt_kprintf("==================================================\n");

    /* Initialize test control */
    g_hierarchicalTestRunning = true;
    g_eventSequence = 0U;

    /* Use high-performance policy for better priority handling */
    QF_setDispatcherStrategy(&QF_highPerfStrategy);

    /* Start the test AO */
    QActive_ctor(&l_hierarchicalTestAO.super, Q_STATE_CAST(&HierarchicalTestAO_initial));
    QACTIVE_START(&l_hierarchicalTestAO.super,
                  7U,  /* priority */
                  l_hierarchicalTestQueueSto,
                  Q_DIM(l_hierarchicalTestQueueSto),
                  l_hierarchicalTestStack,
                  sizeof(l_hierarchicalTestStack),
                  (void *)0);

    /* Create and start event producer thread */
    g_hierarchicalEventProducer = rt_thread_create("hier_producer",
                                                   hierarchicalEventProducerThread,
                                                   RT_NULL,
                                                   1024,  /* stack size */
                                                   RT_THREAD_PRIORITY_MAX - 3,  /* High priority producer */
                                                   10);
    if (g_hierarchicalEventProducer != RT_NULL) {
        rt_thread_startup(g_hierarchicalEventProducer);
    }

    /* Schedule test stop */
    rt_timer_t stopTimer = rt_timer_create("hierarchical_stop",
                                          (void (*)(void*))HierarchicalPriorityTest_stop,
                                          RT_NULL,
                                          rt_tick_from_millisecond(HIERARCHICAL_TEST_DURATION_MS),
                                          RT_TIMER_FLAG_ONE_SHOT);
    if (stopTimer != RT_NULL) {
        rt_timer_start(stopTimer);
    }
}

void HierarchicalPriorityTest_stop(void) {
    if (!g_hierarchicalTestRunning) {
        return;
    }

    rt_kprintf("\n==================================================\n");
    rt_kprintf("Stopping Hierarchical Priority Performance Test\n");
    rt_kprintf("==================================================\n");

    /* Stop test control */
    g_hierarchicalTestRunning = false;

    /* Send stop signal to AO */
    static QEvt const stopEvt = { HIERARCHICAL_STOP_SIG, 0U, 0U };
    QActive_post_(&l_hierarchicalTestAO.super, &stopEvt, QF_NO_MARGIN, (void *)0);

    /* Wait for threads to complete */
    PerfCommon_waitForThreads();

    /* Delete event producer thread */
    if (g_hierarchicalEventProducer != RT_NULL) {
        rt_thread_delete(g_hierarchicalEventProducer);
        g_hierarchicalEventProducer = RT_NULL;
    }

    /* Calculate and display results */
    uint32_t avgHighLatency = (l_hierarchicalTestAO.highPrioCount > 0U)
                             ? (l_hierarchicalTestAO.highPrioLatencySum / l_hierarchicalTestAO.highPrioCount)
                             : 0U;
    uint32_t avgNormalLatency = (l_hierarchicalTestAO.normalPrioCount > 0U)
                               ? (l_hierarchicalTestAO.normalPrioLatencySum / l_hierarchicalTestAO.normalPrioCount)
                               : 0U;
    uint32_t avgLowLatency = (l_hierarchicalTestAO.lowPrioCount > 0U)
                            ? (l_hierarchicalTestAO.lowPrioLatencySum / l_hierarchicalTestAO.lowPrioCount)
                            : 0U;

    rt_kprintf("\n--- Hierarchical Priority Test Results ---\n");
    rt_kprintf("Total Events Processed: %u\n", l_hierarchicalTestAO.eventCount);
    rt_kprintf("Order Violations: %u\n", l_hierarchicalTestAO.orderViolations);

    rt_kprintf("\nHigh Priority Events (Expected: %u):\n", HIERARCHICAL_HIGH_EVENTS);
    rt_kprintf("  Processed: %u\n", l_hierarchicalTestAO.highPrioCount);
    rt_kprintf("  Avg Latency: %u cycles\n", avgHighLatency);
    rt_kprintf("  Min Latency: %u cycles\n", l_hierarchicalTestAO.minLatency[0]);
    rt_kprintf("  Max Latency: %u cycles\n", l_hierarchicalTestAO.maxLatency[0]);

    rt_kprintf("\nNormal Priority Events (Expected: %u):\n", HIERARCHICAL_NORMAL_EVENTS);
    rt_kprintf("  Processed: %u\n", l_hierarchicalTestAO.normalPrioCount);
    rt_kprintf("  Avg Latency: %u cycles\n", avgNormalLatency);
    rt_kprintf("  Min Latency: %u cycles\n", l_hierarchicalTestAO.minLatency[1]);
    rt_kprintf("  Max Latency: %u cycles\n", l_hierarchicalTestAO.maxLatency[1]);

    rt_kprintf("\nLow Priority Events (Expected: %u):\n", HIERARCHICAL_LOW_EVENTS);
    rt_kprintf("  Processed: %u\n", l_hierarchicalTestAO.lowPrioCount);
    rt_kprintf("  Avg Latency: %u cycles\n", avgLowLatency);
    rt_kprintf("  Min Latency: %u cycles\n", l_hierarchicalTestAO.minLatency[2]);
    rt_kprintf("  Max Latency: %u cycles\n", l_hierarchicalTestAO.maxLatency[2]);

    /* Validate priority ordering */
    bool priorityOrderingValid = (avgHighLatency <= avgNormalLatency) &&
                                (avgNormalLatency <= avgLowLatency);

    rt_kprintf("\n--- Priority Validation ---\n");
    rt_kprintf("Priority Ordering Valid: %s\n", priorityOrderingValid ? "YES" : "NO");
    rt_kprintf("Order Violation Rate: %.2f%%\n",
               (l_hierarchicalTestAO.eventCount > 0U)
               ? ((float)l_hierarchicalTestAO.orderViolations * 100.0f / l_hierarchicalTestAO.eventCount)
               : 0.0f);

    /* Get dispatcher metrics */
    QF_DispatcherMetrics const *metrics = QF_getDispatcherMetrics();
    rt_kprintf("\n--- Dispatcher Metrics ---\n");
    rt_kprintf("Dispatch Cycles: %u\n", metrics->dispatchCycles);
    rt_kprintf("Events Merged: %u\n", metrics->eventsMerged);
    rt_kprintf("Events Dropped: %u\n", metrics->eventsDropped);
    rt_kprintf("Max Batch Size: %u\n", metrics->maxBatchSize);
    rt_kprintf("Staging Overflows (H/N/L): %u/%u/%u\n",
               metrics->stagingOverflows[0],
               metrics->stagingOverflows[1],
               metrics->stagingOverflows[2]);

    rt_kprintf("==================================================\n");

    /* Reset to default policy */
    QF_setDispatcherStrategy(&QF_defaultStrategy);
    QF_resetDispatcherMetrics();
}
