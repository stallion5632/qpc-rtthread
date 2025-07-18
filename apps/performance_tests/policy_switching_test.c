/*============================================================================
* Product: Policy Switching Performance Test
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
/* Policy Switching Test Configuration */
/*==========================================================================*/
#define POLICY_SWITCH_TEST_DURATION_MS     (10000U)  /* 10 seconds */
#define POLICY_SWITCH_INTERVAL_MS          (1000U)   /* Switch every 1 second */
#define POLICY_SWITCH_EVENTS_PER_INTERVAL  (100U)    /* 100 events per interval */

/* Policy Switching Test Event Signals */
enum PolicySwitchSignals {
    POLICY_SWITCH_SIG = Q_USER_SIG + 50,
    POLICY_TEST_EVENT_SIG,
    POLICY_SWITCH_STOP_SIG,
    POLICY_SWITCH_MAX_SIG
};

/*==========================================================================*/
/* Policy Switching Test Event */
/*==========================================================================*/
typedef struct {
    QEvtEx super;   /* Use extended event for priority and flags */
    uint32_t sequenceNumber;
    uint8_t testPhase;    /* 0=default policy, 1=high-perf policy */
} PolicySwitchEvt;

/*==========================================================================*/
/* Policy Switching Test AO */
/*==========================================================================*/
typedef struct {
    QActive super;
    uint32_t eventCount;
    uint32_t defaultPolicyEvents;
    uint32_t highPerfPolicyEvents;
    uint32_t policySwitchCount;
    uint32_t totalLatencyDefault;
    uint32_t totalLatencyHighPerf;
    uint32_t maxLatencyDefault;
    uint32_t maxLatencyHighPerf;
    uint32_t minLatencyDefault;
    uint32_t minLatencyHighPerf;
    bool isRunning;
} PolicySwitchTestAO;

/*==========================================================================*/
/* Policy Switching Test Globals */
/*==========================================================================*/
static PolicySwitchTestAO l_policySwitchTestAO;
static QEvt const *l_policySwitchTestQueueSto[20];
static uint8_t l_policySwitchTestStack[512];

/* Test control */
static volatile bool g_policySwitchTestRunning = false;
static rt_timer_t g_policySwitchTimer;
static rt_thread_t g_policyEventProducer = RT_NULL;
static volatile uint32_t g_currentTestPhase = 0U;  /* 0=default, 1=high-perf */

/*==========================================================================*/
/* Forward Declarations */
/*==========================================================================*/
static QState PolicySwitchTestAO_initial(PolicySwitchTestAO * const me, void const * const par);
static QState PolicySwitchTestAO_running(PolicySwitchTestAO * const me, QEvt const * const e);
static void policySwitchTimerCallback(void *parameter);
static void policyEventProducerThread(void *parameter);
void PolicySwitchingTest_stop(void);

/*==========================================================================*/
/* Policy Switching Test AO State Machine */
/*==========================================================================*/
static QState PolicySwitchTestAO_initial(PolicySwitchTestAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);

    me->eventCount = 0U;
    me->defaultPolicyEvents = 0U;
    me->highPerfPolicyEvents = 0U;
    me->policySwitchCount = 0U;
    me->totalLatencyDefault = 0U;
    me->totalLatencyHighPerf = 0U;
    me->maxLatencyDefault = 0U;
    me->maxLatencyHighPerf = 0U;
    me->minLatencyDefault = UINT32_MAX;
    me->minLatencyHighPerf = UINT32_MAX;
    me->isRunning = false;

    return Q_TRAN(&PolicySwitchTestAO_running);
}

static QState PolicySwitchTestAO_running(PolicySwitchTestAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->isRunning = true;
            status = Q_HANDLED();
            break;
        }

        case POLICY_SWITCH_SIG: {
            /* Switch between default and high-performance policies */
            if (g_currentTestPhase == 0U) {
                QF_setDispatcherStrategy(&QF_highPerfStrategy);
                g_currentTestPhase = 1U;
                rt_kprintf("[Policy Switch] Switched to HIGH-PERFORMANCE policy\n");
            } else {
                QF_setDispatcherStrategy(&QF_defaultStrategy);
                g_currentTestPhase = 0U;
                rt_kprintf("[Policy Switch] Switched to DEFAULT policy\n");
            }
            me->policySwitchCount++;
            status = Q_HANDLED();
            break;
        }

        case POLICY_TEST_EVENT_SIG: {
            PolicySwitchEvt const * const evt = (PolicySwitchEvt const *)e;
            uint32_t currentTime = PerfCommon_getCycleCount();
            uint32_t latency = currentTime - evt->super.timestamp;

            me->eventCount++;

            if (evt->testPhase == 0U) {
                /* Default policy event */
                me->defaultPolicyEvents++;
                me->totalLatencyDefault += latency;
                if (latency > me->maxLatencyDefault) {
                    me->maxLatencyDefault = latency;
                }
                if (latency < me->minLatencyDefault) {
                    me->minLatencyDefault = latency;
                }
            } else {
                /* High-performance policy event */
                me->highPerfPolicyEvents++;
                me->totalLatencyHighPerf += latency;
                if (latency > me->maxLatencyHighPerf) {
                    me->maxLatencyHighPerf = latency;
                }
                if (latency < me->minLatencyHighPerf) {
                    me->minLatencyHighPerf = latency;
                }
            }

            status = Q_HANDLED();
            break;
        }

        case POLICY_SWITCH_STOP_SIG: {
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
/* Timer and Thread Functions */
/*==========================================================================*/
static void policySwitchTimerCallback(void *parameter) {
    Q_UNUSED_PAR(parameter);

    if (g_policySwitchTestRunning) {
        static QEvt const switchEvt = { POLICY_SWITCH_SIG, 0U, 0U };
        QActive_post_(&l_policySwitchTestAO.super, &switchEvt, QF_NO_MARGIN, (void *)0);
    }
}

static void policyEventProducerThread(void *parameter) {
    Q_UNUSED_PAR(parameter);
    uint32_t sequenceNumber = 0U;

    while (g_policySwitchTestRunning) {
        for (uint32_t i = 0U; i < POLICY_SWITCH_EVENTS_PER_INTERVAL && g_policySwitchTestRunning; ++i) {
            /* Use extended events for better policy testing */
            PolicySwitchEvt *evt = (PolicySwitchEvt *)QF_newEvtEx(POLICY_TEST_EVENT_SIG, sizeof(PolicySwitchEvt),
                                                                 (g_currentTestPhase == 0U) ? 128U : 200U, 0U);
            if (evt != (PolicySwitchEvt *)0) {
                evt->super.timestamp = PerfCommon_getCycleCount();
                evt->sequenceNumber = sequenceNumber++;
                evt->testPhase = g_currentTestPhase;

                /* Use ISR posting to test interrupt path and dispatcher metrics */
                QF_postFromISR(&l_policySwitchTestAO.super, &evt->super.super);
            }

            rt_thread_mdelay(POLICY_SWITCH_INTERVAL_MS / POLICY_SWITCH_EVENTS_PER_INTERVAL);
        }
    }
}

/*==========================================================================*/
/* Policy Switching Test Functions */
/*==========================================================================*/
void PolicySwitchingTest_start(void) {
    if (g_policySwitchTestRunning) {
        rt_kprintf("Policy Switching Test is already running!\n");
        return;
    }

    rt_kprintf("==================================================\n");
    rt_kprintf("Starting Policy Switching Performance Test\n");
    rt_kprintf("Duration: %u ms\n", POLICY_SWITCH_TEST_DURATION_MS);
    rt_kprintf("Switch Interval: %u ms\n", POLICY_SWITCH_INTERVAL_MS);
    rt_kprintf("Events per Interval: %u\n", POLICY_SWITCH_EVENTS_PER_INTERVAL);
    rt_kprintf("Testing runtime policy switching capability...\n");
    rt_kprintf("==================================================\n");

    /* Initialize test control */
    g_policySwitchTestRunning = true;
    g_currentTestPhase = 0U;

    /* Reset to default policy and reset metrics */
    QF_setDispatcherStrategy(&QF_defaultStrategy);
    QF_resetDispatcherMetrics();

    /* Start the test AO */
    QActive_ctor(&l_policySwitchTestAO.super, Q_STATE_CAST(&PolicySwitchTestAO_initial));
    QACTIVE_START(&l_policySwitchTestAO.super,
                  6U,  /* priority */
                  l_policySwitchTestQueueSto,
                  Q_DIM(l_policySwitchTestQueueSto),
                  l_policySwitchTestStack,
                  sizeof(l_policySwitchTestStack),
                  (void *)0);

    /* Create and start the policy switch timer */
    g_policySwitchTimer = rt_timer_create("policy_switch",
                                         policySwitchTimerCallback,
                                         RT_NULL,
                                         rt_tick_from_millisecond(POLICY_SWITCH_INTERVAL_MS),
                                         RT_TIMER_FLAG_PERIODIC);
    if (g_policySwitchTimer != RT_NULL) {
        rt_timer_start(g_policySwitchTimer);
    }

    /* Create and start event producer thread */
    g_policyEventProducer = rt_thread_create("policy_producer",
                                            policyEventProducerThread,
                                            RT_NULL,
                                            512,   /* stack size */
                                            RT_THREAD_PRIORITY_MAX - 5,
                                            10);
    if (g_policyEventProducer != RT_NULL) {
        rt_thread_startup(g_policyEventProducer);
    }

    /* Schedule test stop */
    rt_timer_t stopTimer = rt_timer_create("policy_stop",
                                          (void (*)(void*))PolicySwitchingTest_stop,
                                          RT_NULL,
                                          rt_tick_from_millisecond(POLICY_SWITCH_TEST_DURATION_MS),
                                          RT_TIMER_FLAG_ONE_SHOT);
    if (stopTimer != RT_NULL) {
        rt_timer_start(stopTimer);
    }
}

void PolicySwitchingTest_stop(void) {
    if (!g_policySwitchTestRunning) {
        return;
    }

    rt_kprintf("\n==================================================\n");
    rt_kprintf("Stopping Policy Switching Performance Test\n");
    rt_kprintf("==================================================\n");

    /* Stop test control */
    g_policySwitchTestRunning = false;

    /* Send stop signal to AO */
    static QEvt const stopEvt = { POLICY_SWITCH_STOP_SIG, 0U, 0U };
    QActive_post_(&l_policySwitchTestAO.super, &stopEvt, QF_NO_MARGIN, (void *)0);

    /* Wait for threads to complete */
    PerfCommon_waitForThreads();

    /* Stop and delete timer */
    if (g_policySwitchTimer != RT_NULL) {
        rt_timer_stop(g_policySwitchTimer);
        rt_timer_delete(g_policySwitchTimer);
        g_policySwitchTimer = RT_NULL;
    }

    /* Delete event producer thread */
    if (g_policyEventProducer != RT_NULL) {
        rt_thread_delete(g_policyEventProducer);
        g_policyEventProducer = RT_NULL;
    }

    /* Calculate and display results */
    uint32_t avgLatencyDefault = (l_policySwitchTestAO.defaultPolicyEvents > 0U)
                                ? (l_policySwitchTestAO.totalLatencyDefault / l_policySwitchTestAO.defaultPolicyEvents)
                                : 0U;
    uint32_t avgLatencyHighPerf = (l_policySwitchTestAO.highPerfPolicyEvents > 0U)
                                 ? (l_policySwitchTestAO.totalLatencyHighPerf / l_policySwitchTestAO.highPerfPolicyEvents)
                                 : 0U;

    rt_kprintf("\n--- Policy Switching Test Results ---\n");
    rt_kprintf("Total Events Processed: %u\n", l_policySwitchTestAO.eventCount);
    rt_kprintf("Policy Switches: %u\n", l_policySwitchTestAO.policySwitchCount);
    rt_kprintf("\nDefault Policy Performance:\n");
    rt_kprintf("  Events: %u\n", l_policySwitchTestAO.defaultPolicyEvents);
    rt_kprintf("  Avg Latency: %u cycles\n", avgLatencyDefault);
    rt_kprintf("  Min Latency: %u cycles\n", l_policySwitchTestAO.minLatencyDefault);
    rt_kprintf("  Max Latency: %u cycles\n", l_policySwitchTestAO.maxLatencyDefault);
    rt_kprintf("\nHigh-Performance Policy Performance:\n");
    rt_kprintf("  Events: %u\n", l_policySwitchTestAO.highPerfPolicyEvents);
    rt_kprintf("  Avg Latency: %u cycles\n", avgLatencyHighPerf);
    rt_kprintf("  Min Latency: %u cycles\n", l_policySwitchTestAO.minLatencyHighPerf);
    rt_kprintf("  Max Latency: %u cycles\n", l_policySwitchTestAO.maxLatencyHighPerf);

    if (avgLatencyDefault > 0U && avgLatencyHighPerf > 0U) {
        uint32_t improvement = (avgLatencyDefault > avgLatencyHighPerf)
                              ? ((avgLatencyDefault - avgLatencyHighPerf) * 100U / avgLatencyDefault)
                              : 0U;
        rt_kprintf("\nPerformance Improvement: %u%% faster with high-perf policy\n", improvement);
    }

    /* Get dispatcher metrics to validate ISR path testing */
    QF_DispatcherMetrics const *metrics = QF_getDispatcherMetrics();
    rt_kprintf("\n--- ISR Path Dispatcher Metrics ---\n");
    rt_kprintf("Dispatch Cycles: %u\n", metrics->dispatchCycles);
    rt_kprintf("Events Processed via ISR path: %u\n", metrics->eventsProcessed);
    rt_kprintf("Events Merged: %u\n", metrics->eventsMerged);
    rt_kprintf("Events Dropped: %u\n", metrics->eventsDropped);
    rt_kprintf("Post Failures: %u\n", metrics->postFailures);
    rt_kprintf("Max Batch Size: %u\n", metrics->maxBatchSize);
    rt_kprintf("Staging Overflows (H/N/L): %u/%u/%u\n",
               metrics->stagingOverflows[0],
               metrics->stagingOverflows[1],
               metrics->stagingOverflows[2]);

    /* Validate ISR metrics were accumulated */
    if (metrics->eventsProcessed > 0U) {
        rt_kprintf("✓ ISR path metrics successfully accumulated\n");
    } else {
        rt_kprintf("⚠ No ISR path metrics accumulated - check QF_postFromISR implementation\n");
    }

    rt_kprintf("==================================================\n");

    /* Reset to default policy */
    QF_setDispatcherStrategy(&QF_defaultStrategy);
}
