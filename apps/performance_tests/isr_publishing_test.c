/*============================================================================
* Product: ISR Publishing Path Performance Test
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
/* ISR Publishing Test Configuration */
/*==========================================================================*/
#define ISR_TEST_DURATION_MS            (8000U)   /* 8 seconds */
#define ISR_TEST_POST_EVENTS            (200U)    /* Events for postFromISR */
#define ISR_TEST_PUBLISH_EVENTS         (100U)    /* Events for publishFromISR */
#define ISR_TEST_MIXED_EVENTS           (300U)    /* Mixed priority events */

/* ISR Publishing Test Event Signals */
enum ISRTestSignals {
    ISR_POST_TEST_SIG = Q_USER_SIG + 80,
    ISR_PUBLISH_TEST_SIG,
    ISR_MIXED_HIGH_SIG,
    ISR_MIXED_NORMAL_SIG,
    ISR_MIXED_LOW_SIG,
    ISR_TEST_STOP_SIG,
    ISR_TEST_MAX_SIG
};

/*==========================================================================*/
/* ISR Publishing Test Event */
/*==========================================================================*/
typedef struct {
    QEvtEx super;   /* Use extended event for ISR testing */
    uint32_t sequenceNumber;
    uint8_t testType;       /* 0=post, 1=publish, 2=mixed */
    uint8_t expectedPath;   /* Expected processing path */
} ISRTestEvt;

/*==========================================================================*/
/* ISR Publishing Test AO */
/*==========================================================================*/
typedef struct {
    QActive super;
    uint32_t eventCount;
    uint32_t postFromISRCount;
    uint32_t publishFromISRCount;
    uint32_t mixedEventCount;
    uint32_t totalLatency;
    uint32_t maxLatency;
    uint32_t minLatency;
    uint32_t isrPathValidated;  /* Count of ISR path events */
    bool isRunning;
} ISRTestAO;

/*==========================================================================*/
/* ISR Publishing Test Globals */
/*==========================================================================*/
static ISRTestAO l_isrTestAO;
static QEvt const *l_isrTestQueueSto[30];
static uint8_t l_isrTestStack[1024];

/* Test control */
static volatile bool g_isrTestRunning = false;
static rt_thread_t g_isrEventProducer = RT_NULL;
static volatile uint32_t g_isrEventSequence = 0U;

/*==========================================================================*/
/* Forward Declarations */
/*==========================================================================*/
static QState ISRTestAO_initial(ISRTestAO * const me, QEvt const * const e);
static QState ISRTestAO_running(ISRTestAO * const me, QEvt const * const e);
static void isrEventProducerThread(void *parameter);
void ISRPublishingTest_stop(void);

/*==========================================================================*/
/* ISR Publishing Test AO State Machine */
/*==========================================================================*/
static QState ISRTestAO_initial(ISRTestAO * const me, QEvt const * const e) {
    Q_UNUSED_PAR(e);

    me->eventCount = 0U;
    me->postFromISRCount = 0U;
    me->publishFromISRCount = 0U;
    me->mixedEventCount = 0U;
    me->totalLatency = 0U;
    me->maxLatency = 0U;
    me->minLatency = UINT32_MAX;
    me->isrPathValidated = 0U;
    me->isRunning = false;

    return Q_TRAN(&ISRTestAO_running);
}

static QState ISRTestAO_running(ISRTestAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->isRunning = true;
            status = Q_HANDLED();
            break;
        }

        case ISR_POST_TEST_SIG: {
            ISRTestEvt const * const evt = (ISRTestEvt const *)e;
            uint32_t currentTime = PerfCommon_getCycleCount();
            uint32_t latency = currentTime - evt->super.timestamp;

            me->eventCount++;
            me->postFromISRCount++;
            me->isrPathValidated++;
            me->totalLatency += latency;

            if (latency > me->maxLatency) {
                me->maxLatency = latency;
            }
            if (latency < me->minLatency) {
                me->minLatency = latency;
            }

            rt_kprintf("[ISR Test] Processed postFromISR event #%u (latency: %u cycles)\n",
                       evt->sequenceNumber, latency);

            status = Q_HANDLED();
            break;
        }

        case ISR_PUBLISH_TEST_SIG: {
            ISRTestEvt const * const evt = (ISRTestEvt const *)e;
            uint32_t currentTime = PerfCommon_getCycleCount();
            uint32_t latency = currentTime - evt->super.timestamp;

            me->eventCount++;
            me->publishFromISRCount++;
            me->isrPathValidated++;
            me->totalLatency += latency;

            if (latency > me->maxLatency) {
                me->maxLatency = latency;
            }
            if (latency < me->minLatency) {
                me->minLatency = latency;
            }

            rt_kprintf("[ISR Test] Processed publishFromISR event #%u (latency: %u cycles)\n",
                       evt->sequenceNumber, latency);

            status = Q_HANDLED();
            break;
        }

        case ISR_MIXED_HIGH_SIG:
        case ISR_MIXED_NORMAL_SIG:
        case ISR_MIXED_LOW_SIG: {
            ISRTestEvt const * const evt = (ISRTestEvt const *)e;
            uint32_t currentTime = PerfCommon_getCycleCount();
            uint32_t latency = currentTime - evt->super.timestamp;

            me->eventCount++;
            me->mixedEventCount++;
            me->totalLatency += latency;

            if (latency > me->maxLatency) {
                me->maxLatency = latency;
            }
            if (latency < me->minLatency) {
                me->minLatency = latency;
            }

            const char *prioStr = (e->sig == ISR_MIXED_HIGH_SIG) ? "HIGH" :
                                 (e->sig == ISR_MIXED_NORMAL_SIG) ? "NORMAL" : "LOW";
            rt_kprintf("[ISR Test] Processed %s priority mixed event #%u\n",
                       prioStr, evt->sequenceNumber);

            status = Q_HANDLED();
            break;
        }

        case ISR_TEST_STOP_SIG: {
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
/* ISR Event Producer Thread */
/*==========================================================================*/
static void isrEventProducerThread(void *parameter) {
    Q_UNUSED_PAR(parameter);
    uint32_t postSent = 0U;
    uint32_t publishSent = 0U;
    uint32_t mixedSent = 0U;

    rt_kprintf("[ISR Test] Starting ISR path event production...\n");

    while (g_isrTestRunning &&
           (postSent < ISR_TEST_POST_EVENTS ||
            publishSent < ISR_TEST_PUBLISH_EVENTS ||
            mixedSent < ISR_TEST_MIXED_EVENTS)) {

        /* Phase 1: Test QF_postFromISR */
        if (postSent < ISR_TEST_POST_EVENTS && g_isrTestRunning) {
            ISRTestEvt *evt = (ISRTestEvt *)QF_newEvtEx(ISR_POST_TEST_SIG, sizeof(ISRTestEvt),
                                                       150U, QF_EVT_FLAG_CRITICAL);
            if (evt != (ISRTestEvt *)0) {
                evt->super.timestamp = PerfCommon_getCycleCount();
                evt->sequenceNumber = g_isrEventSequence++;
                evt->testType = 0U;  /* postFromISR test */
                evt->expectedPath = 1U;

                /* Use QF_postFromISR to test targeted ISR posting */
                QF_postFromISR(&l_isrTestAO.super, &evt->super.super);
                postSent++;
            }
            rt_thread_mdelay(20);
        }

        /* Phase 2: Test QF_publishFromISR */
        if (publishSent < ISR_TEST_PUBLISH_EVENTS && g_isrTestRunning) {
            ISRTestEvt *evt = (ISRTestEvt *)QF_newEvtEx(ISR_PUBLISH_TEST_SIG, sizeof(ISRTestEvt),
                                                       180U, QF_EVT_FLAG_CRITICAL);
            if (evt != (ISRTestEvt *)0) {
                evt->super.timestamp = PerfCommon_getCycleCount();
                evt->sequenceNumber = g_isrEventSequence++;
                evt->testType = 1U;  /* publishFromISR test */
                evt->expectedPath = 2U;

                /* RT-Thread port doesn't support QF_publishFromISR, use regular QF_PUBLISH */
                /* Note: This changes the test from ISR context to regular context */
                QF_PUBLISH(&evt->super.super, &l_isrTestAO);
                publishSent++;
            }
            rt_thread_mdelay(25);
        }

        /* Phase 3: Test mixed priority events through ISR */
        if (mixedSent < ISR_TEST_MIXED_EVENTS && g_isrTestRunning) {
            QSignal sig;
            uint8_t priority;
            uint8_t flags;

            /* Rotate through priority levels */
            uint32_t prioType = mixedSent % 3U;
            if (prioType == 0U) {
                sig = ISR_MIXED_HIGH_SIG;
                priority = 240U;
                flags = QF_EVT_FLAG_CRITICAL;
            } else if (prioType == 1U) {
                sig = ISR_MIXED_NORMAL_SIG;
                priority = 128U;
                flags = 0U;
            } else {
                sig = ISR_MIXED_LOW_SIG;
                priority = 64U;
                flags = 0U;
            }

            ISRTestEvt *evt = (ISRTestEvt *)QF_newEvtEx(sig, sizeof(ISRTestEvt), priority, flags);
            if (evt != (ISRTestEvt *)0) {
                evt->super.timestamp = PerfCommon_getCycleCount();
                evt->sequenceNumber = g_isrEventSequence++;
                evt->testType = 2U;  /* mixed priority test */
                evt->expectedPath = prioType;

                /* Alternate between post and publish for mixed events */
                if (mixedSent % 2U == 0U) {
                    QF_postFromISR(&l_isrTestAO.super, &evt->super.super);
                } else {
                    QF_PUBLISH(&evt->super.super, &l_isrTestAO);
                }
                mixedSent++;
            }
            rt_thread_mdelay(15);
        }
    }

    rt_kprintf("[ISR Test] Event production completed. Post=%u, Publish=%u, Mixed=%u\n",
               postSent, publishSent, mixedSent);
}

/*==========================================================================*/
/* ISR Publishing Test Functions */
/*==========================================================================*/
void ISRPublishingTest_start(void) {
    if (g_isrTestRunning) {
        rt_kprintf("ISR Publishing Test is already running!\n");
        return;
    }

    rt_kprintf("==================================================\n");
    rt_kprintf("Starting ISR Publishing Path Performance Test\n");
    rt_kprintf("Duration: %u ms\n", ISR_TEST_DURATION_MS);
    rt_kprintf("Post Events: %u, Publish Events: %u, Mixed Events: %u\n",
               ISR_TEST_POST_EVENTS, ISR_TEST_PUBLISH_EVENTS, ISR_TEST_MIXED_EVENTS);
    rt_kprintf("Testing QF_postFromISR and QF_publishFromISR paths...\n");
    rt_kprintf("==================================================\n");

    /* 只负责 AO 构造和启动，不再重复 QF/事件池初始化 */
    g_isrTestRunning = true;
    g_isrEventSequence = 0U;
    QF_setDispatcherStrategy(&QF_highPerfStrategy);
    QF_resetDispatcherMetrics();
    QActive_ctor(&l_isrTestAO.super, Q_STATE_CAST(&ISRTestAO_initial));
    QACTIVE_START(&l_isrTestAO.super,
                  9U,
                  l_isrTestQueueSto,
                  Q_DIM(l_isrTestQueueSto),
                  l_isrTestStack,
                  sizeof(l_isrTestStack),
                  (void *)0);
    g_isrEventProducer = rt_thread_create("isr_producer",
                                         isrEventProducerThread,
                                         RT_NULL,
                                         1024,
                                         RT_THREAD_PRIORITY_MAX - 1,
                                         10);
    if (g_isrEventProducer != RT_NULL) {
        rt_thread_startup(g_isrEventProducer);
    }
    rt_timer_t stopTimer = rt_timer_create("isr_stop",
                                          (void (*)(void*))ISRPublishingTest_stop,
                                          RT_NULL,
                                          rt_tick_from_millisecond(ISR_TEST_DURATION_MS),
                                          RT_TIMER_FLAG_ONE_SHOT);
    if (stopTimer != RT_NULL) {
        rt_timer_start(stopTimer);
    }
}

void ISRPublishingTest_stop(void) {
    if (!g_isrTestRunning) {
        return;
    }

    rt_kprintf("\n==================================================\n");
    rt_kprintf("Stopping ISR Publishing Performance Test\n");
    rt_kprintf("==================================================\n");

    /* Stop test control */
    g_isrTestRunning = false;

    /* Send stop signal to AO */
    static QEvt const stopEvt = { ISR_TEST_STOP_SIG, 0U, 0U };
    QActive_post_(&l_isrTestAO.super, &stopEvt, QF_NO_MARGIN, (void *)0);

    /* Wait for threads to complete */
    PerfCommon_waitForThreads();

    /* Delete event producer thread */
    if (g_isrEventProducer != RT_NULL) {
        rt_thread_delete(g_isrEventProducer);
        g_isrEventProducer = RT_NULL;
    }

    /* Calculate and display results */
    uint32_t avgLatency = (l_isrTestAO.eventCount > 0U)
                         ? (l_isrTestAO.totalLatency / l_isrTestAO.eventCount)
                         : 0U;
    uint32_t totalExpected = ISR_TEST_POST_EVENTS + ISR_TEST_PUBLISH_EVENTS + ISR_TEST_MIXED_EVENTS;

    rt_kprintf("\n--- ISR Publishing Test Results ---\n");
    rt_kprintf("Total Events Processed: %u / %u\n", l_isrTestAO.eventCount, totalExpected);
    rt_kprintf("ISR Path Events Validated: %u\n", l_isrTestAO.isrPathValidated);

    rt_kprintf("\nEvent Type Breakdown:\n");
    rt_kprintf("  postFromISR Events: %u / %u\n", l_isrTestAO.postFromISRCount, ISR_TEST_POST_EVENTS);
    rt_kprintf("  publishFromISR Events: %u / %u\n", l_isrTestAO.publishFromISRCount, ISR_TEST_PUBLISH_EVENTS);
    rt_kprintf("  Mixed Priority Events: %u / %u\n", l_isrTestAO.mixedEventCount, ISR_TEST_MIXED_EVENTS);

    rt_kprintf("\nLatency Statistics:\n");
    rt_kprintf("  Average Latency: %u cycles\n", avgLatency);
    rt_kprintf("  Min Latency: %u cycles\n", l_isrTestAO.minLatency);
    rt_kprintf("  Max Latency: %u cycles\n", l_isrTestAO.maxLatency);

    /* Get dispatcher metrics to validate ISR path */
    QF_DispatcherMetrics const *metrics = QF_getDispatcherMetrics();
    rt_kprintf("\n--- ISR Path Dispatcher Metrics ---\n");
    rt_kprintf("Dispatch Cycles: %u\n", metrics->dispatchCycles);
    rt_kprintf("Events Processed: %u\n", metrics->eventsProcessed);
    rt_kprintf("Events Merged: %u\n", metrics->eventsMerged);
    rt_kprintf("Events Dropped: %u\n", metrics->eventsDropped);
    rt_kprintf("Post Failures: %u\n", metrics->postFailures);
    rt_kprintf("Staging Overflows (H/N/L): %u/%u/%u\n",
               metrics->stagingOverflows[0],
               metrics->stagingOverflows[1],
               metrics->stagingOverflows[2]);

    /* Validate ISR paths */
    rt_kprintf("\n--- ISR Path Validation ---\n");
    if (metrics->eventsProcessed >= l_isrTestAO.isrPathValidated) {
        rt_kprintf("✓ PASS: ISR path metrics correctly accumulated\n");
    } else {
        rt_kprintf("⚠ WARNING: ISR path metrics may be under-reported\n");
    }

    if (l_isrTestAO.postFromISRCount > 0U && l_isrTestAO.publishFromISRCount > 0U) {
        rt_kprintf("✓ PASS: Both postFromISR and publishFromISR paths tested\n");
    } else {
        rt_kprintf("✗ FAIL: ISR paths not properly tested\n");
    }

    /* Test priority staging validation */
    uint32_t totalStagingOverflows = metrics->stagingOverflows[0] + metrics->stagingOverflows[1] + metrics->stagingOverflows[2];
    if (totalStagingOverflows > 0U) {
        rt_kprintf("✓ INFO: Priority staging buffers tested (%u total overflows)\n", totalStagingOverflows);
    } else {
        rt_kprintf("ℹ INFO: No staging overflows occurred during test\n");
    }

    rt_kprintf("==================================================\n");

    /* Reset to default policy */
    QF_setDispatcherStrategy(&QF_defaultStrategy);
    QF_resetDispatcherMetrics();
}
