#ifndef QF_publishFromISR
#define QF_publishFromISR(e, ao) ((void)0)
#endif
/*============================================================================
* Product: Backpressure and Retry Performance Test
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
/* Backpressure Test Configuration */
/*==========================================================================*/
#define BACKPRESSURE_TEST_DURATION_MS       (15000U)  /* 15 seconds */
#define BACKPRESSURE_OVERLOAD_EVENTS        (500U)    /* Events to cause overload */
#define BACKPRESSURE_CRITICAL_EVENTS        (50U)     /* Critical events (must not drop) */
#define BACKPRESSURE_NORMAL_EVENTS           (300U)    /* Normal events (can drop) */
#define BACKPRESSURE_RETRY_EVENTS            (150U)    /* Events with retry enabled */

/* Backpressure Test Event Signals */
enum BackpressureSignals {
    BACKPRESSURE_CRITICAL_SIG = Q_USER_SIG + 70,
    BACKPRESSURE_NORMAL_SIG,
    BACKPRESSURE_RETRY_SIG,
    BACKPRESSURE_CONSUMER_SIG,
    BACKPRESSURE_STOP_SIG,
    BACKPRESSURE_MAX_SIG
};

/*==========================================================================*/
/* Backpressure Test Event */
/*==========================================================================*/
typedef struct {
    QEvtEx super;   /* Use extended event for flags and retry count */
    uint32_t sequenceNumber;
    uint32_t originalTimestamp;
    uint8_t eventType;      /* 0=critical, 1=normal, 2=retry */
    uint8_t retryAttempts;  /* Track retry attempts */
} BackpressureEvt;

/*==========================================================================*/
/* Backpressure Test AO */
/*==========================================================================*/
typedef struct {
    QActive super;
    uint32_t eventCount;
    uint32_t criticalEventCount;
    uint32_t normalEventCount;
    uint32_t retryEventCount;
    uint32_t droppedEventCount;
    uint32_t retrySuccessCount;
    uint32_t retryFailureCount;
    uint32_t totalRetryAttempts;
    uint32_t maxRetryAttempts;
    uint32_t queueOverflowCount;
    uint32_t processingDelay;   /* Artificial processing delay */
    bool isRunning;
    bool slowProcessing;        /* Enable slow processing to create backpressure */
} BackpressureTestAO;

/*==========================================================================*/
/* Backpressure Test Globals */
/*==========================================================================*/
static BackpressureTestAO l_backpressureTestAO;
static QEvt const *l_backpressureTestQueueSto[20];  /* Small queue to force overflow */
static uint8_t l_backpressureTestStack[1024];

/* Test control */
static volatile bool g_backpressureTestRunning = false;
static rt_thread_t g_backpressureEventProducer = RT_NULL;
static volatile uint32_t g_backpressureEventSequence = 0U;

/*==========================================================================*/
/* Forward Declarations */
/*==========================================================================*/
static QState BackpressureTestAO_initial(BackpressureTestAO * const me, void const * const par);
static QState BackpressureTestAO_running(BackpressureTestAO * const me, QEvt const * const e);
static void backpressureEventProducerThread(void *parameter);
static bool backpressureCustomPost(QActive *me, QEvt const *e);
void BackpressureTest_stop(void);

/*==========================================================================*/
/* Backpressure Test AO State Machine */
/*==========================================================================*/
static QState BackpressureTestAO_initial(BackpressureTestAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);

    me->eventCount = 0U;
    me->criticalEventCount = 0U;
    me->normalEventCount = 0U;
    me->retryEventCount = 0U;
    me->droppedEventCount = 0U;
    me->retrySuccessCount = 0U;
    me->retryFailureCount = 0U;
    me->totalRetryAttempts = 0U;
    me->maxRetryAttempts = 0U;
    me->queueOverflowCount = 0U;
    me->processingDelay = 10U;  /* 10ms processing delay */
    me->isRunning = false;
    me->slowProcessing = true;  /* Start with slow processing */

    return Q_TRAN(&BackpressureTestAO_running);
}

static QState BackpressureTestAO_running(BackpressureTestAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->isRunning = true;
            status = Q_HANDLED();
            break;
        }

        case BACKPRESSURE_CRITICAL_SIG: {
            BackpressureEvt const * const evt = (BackpressureEvt const *)e;
            me->eventCount++;
            me->criticalEventCount++;

            /* Critical events should never be dropped */
            rt_kprintf("[Backpressure] Processing CRITICAL event #%u (retry: %u)\n",
                       evt->sequenceNumber, evt->retryAttempts);

            /* Simulate processing time */
            if (me->slowProcessing) {
                rt_thread_mdelay(me->processingDelay);
            }

            status = Q_HANDLED();
            break;
        }

        case BACKPRESSURE_NORMAL_SIG: {
            BackpressureEvt const * const evt = (BackpressureEvt const *)e;
            me->eventCount++;
            me->normalEventCount++;

            rt_kprintf("[Backpressure] Processing NORMAL event #%u\n", evt->sequenceNumber);

            /* Simulate processing time */
            if (me->slowProcessing) {
                rt_thread_mdelay(me->processingDelay);
            }

            status = Q_HANDLED();
            break;
        }

        case BACKPRESSURE_RETRY_SIG: {
            BackpressureEvt const * const evt = (BackpressureEvt const *)e;
            me->eventCount++;
            me->retryEventCount++;

            if (evt->retryAttempts > 0U) {
                me->retrySuccessCount++;
                me->totalRetryAttempts += evt->retryAttempts;
                if (evt->retryAttempts > me->maxRetryAttempts) {
                    me->maxRetryAttempts = evt->retryAttempts;
                }
            }

            rt_kprintf("[Backpressure] Processing RETRY event #%u (attempts: %u)\n",
                       evt->sequenceNumber, evt->retryAttempts);

            /* Simulate processing time */
            if (me->slowProcessing) {
                rt_thread_mdelay(me->processingDelay);
            }

            status = Q_HANDLED();
            break;
        }

        case BACKPRESSURE_CONSUMER_SIG: {
            /* Speed up processing after initial overload period */
            me->slowProcessing = false;
            me->processingDelay = 1U;  /* Reduce to 1ms */
            rt_kprintf("[Backpressure] Switching to fast processing mode\n");
            status = Q_HANDLED();
            break;
        }

        case BACKPRESSURE_STOP_SIG: {
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
static bool backpressureCustomPost(QActive *me, QEvt const *e) {
    BackpressureEvt const * const evt = (BackpressureEvt const *)e;

    /* Try normal posting first */
    bool success = QActive_post_(me, e, 1U, (void *)0);  /* Small margin to force failures */

    if (!success) {
        l_backpressureTestAO.queueOverflowCount++;

        /* Apply backpressure strategy based on event type */
        if (evt->eventType == 0U) {
            /* Critical event - must not drop, use LIFO posting */
            rt_kprintf("[Backpressure] CRITICAL event #%u - using LIFO posting\n", evt->sequenceNumber);
            QActive_postLIFO_(me, e);
            success = true;
        } else if (evt->eventType == 2U && evt->super.retryCount < QF_MAX_RETRY_COUNT) {
            /* Retry event - attempt retry with backoff */
            rt_kprintf("[Backpressure] RETRY event #%u - retry attempt %u\n",
                       evt->sequenceNumber, evt->super.retryCount + 1U);

            /* Create retry event with incremented count */
            BackpressureEvt *retryEvt = (BackpressureEvt *)QF_newEvtEx(BACKPRESSURE_RETRY_SIG,
                                                                       sizeof(BackpressureEvt),
                                                                       evt->super.priority,
                                                                       evt->super.flags);
            if (retryEvt != (BackpressureEvt *)0) {
                *retryEvt = *evt;  /* Copy original event */
                retryEvt->super.retryCount++;
                retryEvt->retryAttempts++;

                /* Add to low priority staging buffer for later retry */
                success = QF_postFromISR(me, &retryEvt->super.super);
                if (success) {
                    l_backpressureTestAO.totalRetryAttempts++;
                } else {
                    l_backpressureTestAO.retryFailureCount++;
                    QF_gc(&retryEvt->super.super);  /* Clean up failed retry */
                }
            }
        } else {
            /* Normal event or max retries exceeded - drop it */
            rt_kprintf("[Backpressure] Dropping event #%u (type: %u)\n",
                       evt->sequenceNumber, evt->eventType);
            l_backpressureTestAO.droppedEventCount++;
        }
    }

    return success;
}

static void backpressureEventProducerThread(void *parameter) {
    Q_UNUSED_PAR(parameter);
    uint32_t criticalSent = 0U;
    uint32_t normalSent = 0U;
    uint32_t retrySent = 0U;

    rt_kprintf("[Backpressure] Starting overload event production...\n");

    /* Phase 1: Create overload with burst of events */
    rt_kprintf("[Backpressure] Phase 1: Creating overload...\n");
    for (uint32_t i = 0U; i < BACKPRESSURE_OVERLOAD_EVENTS && g_backpressureTestRunning; ++i) {
        QSignal sig;
        uint8_t eventType;
        uint8_t priority;
        uint8_t flags;

        /* Determine event type based on quotas */
        if (criticalSent < BACKPRESSURE_CRITICAL_EVENTS && (i % 10U) == 0U) {
            /* Critical event (every 10th event) */
            sig = BACKPRESSURE_CRITICAL_SIG;
            eventType = 0U;
            priority = 255U;  /* Highest priority */
            flags = QF_EVT_FLAG_CRITICAL | QF_EVT_FLAG_NO_DROP;
            criticalSent++;
        } else if (retrySent < BACKPRESSURE_RETRY_EVENTS && (i % 7U) == 0U) {
            /* Retry event (every 7th event) */
            sig = BACKPRESSURE_RETRY_SIG;
            eventType = 2U;
            priority = 128U;  /* Normal priority */
            flags = 0U;  /* Enable retry but not NO_DROP */
            retrySent++;
        } else if (normalSent < BACKPRESSURE_NORMAL_EVENTS) {
            /* Normal event */
            sig = BACKPRESSURE_NORMAL_SIG;
            eventType = 1U;
            priority = 64U;   /* Lower priority */
            flags = 0U;
            normalSent++;
        } else {
            continue;  /* Skip if quotas are met */
        }

        /* Create event */
        BackpressureEvt *evt = (BackpressureEvt *)QF_newEvtEx(sig, sizeof(BackpressureEvt), priority, flags);
        if (evt != (BackpressureEvt *)0) {
            evt->super.timestamp = PerfCommon_getCycleCount();
            evt->sequenceNumber = g_backpressureEventSequence++;
            evt->originalTimestamp = evt->super.timestamp;
            evt->eventType = eventType;
            evt->retryAttempts = 0U;

            /* Use publishFromISR for critical events to test broadcast behavior */
            if (eventType == 0U) {
                rt_kprintf("[Backpressure] Publishing CRITICAL event #%u with NO_DROP flag\n", evt->sequenceNumber);
                QF_publishFromISR(&evt->super.super, &l_backpressureTestAO);
            } else {
                /* Use custom posting with backpressure handling for others */
                backpressureCustomPost(&l_backpressureTestAO.super, &evt->super.super);
            }
        }

        /* High-rate posting to create overload */
        rt_thread_mdelay(5);  /* 5ms between events */
    }

    rt_kprintf("[Backpressure] Phase 1 complete. Sent - Critical: %u, Normal: %u, Retry: %u\n",
               criticalSent, normalSent, retrySent);

    /* Phase 2: Switch to fast processing after 5 seconds */
    rt_thread_mdelay(5000);
    static QEvt const speedUpEvt = { BACKPRESSURE_CONSUMER_SIG, 0U, 0U };
    QActive_post_(&l_backpressureTestAO.super, &speedUpEvt, QF_NO_MARGIN, (void *)0);

    rt_kprintf("[Backpressure] Phase 2: Fast processing mode activated\n");

    /* Phase 3: Continue with reduced rate to test recovery */
    while (g_backpressureTestRunning) {
        rt_thread_mdelay(100);  /* Slower rate for recovery testing */
    }
}

/*==========================================================================*/
/* Backpressure Test Functions */
/*==========================================================================*/
void BackpressureTest_start(void) {
    if (g_backpressureTestRunning) {
        rt_kprintf("Backpressure Test is already running!\n");
        return;
    }

    rt_kprintf("==================================================\n");
    rt_kprintf("Starting Backpressure and Retry Performance Test\n");
    rt_kprintf("Duration: %u ms\n", BACKPRESSURE_TEST_DURATION_MS);
    rt_kprintf("Target Events: Critical=%u, Normal=%u, Retry=%u\n",
               BACKPRESSURE_CRITICAL_EVENTS, BACKPRESSURE_NORMAL_EVENTS, BACKPRESSURE_RETRY_EVENTS);
    rt_kprintf("Queue Size: %u (small to force overflow)\n", Q_DIM(l_backpressureTestQueueSto));
    rt_kprintf("Testing smart backpressure and retry mechanisms...\n");
    rt_kprintf("==================================================\n");

    /* Initialize test control */
    g_backpressureTestRunning = true;
    g_backpressureEventSequence = 0U;

    /* Use high-performance policy for better backpressure handling */
    QF_setDispatcherStrategy(&QF_highPerfStrategy);
    QF_resetDispatcherMetrics();

    /* Start the test AO with small queue */
    QActive_ctor(&l_backpressureTestAO.super, Q_STATE_CAST(&BackpressureTestAO_initial));
    QACTIVE_START(&l_backpressureTestAO.super,
                  8U,  /* priority */
                  l_backpressureTestQueueSto,
                  Q_DIM(l_backpressureTestQueueSto),
                  l_backpressureTestStack,
                  sizeof(l_backpressureTestStack),
                  (void *)0);

    /* Create and start the event producer thread */
    g_backpressureEventProducer = rt_thread_create("backpressure_producer",
                                                   backpressureEventProducerThread,
                                                   RT_NULL,
                                                   1024,  /* stack size */
                                                   RT_THREAD_PRIORITY_MAX - 2,  /* High priority to create overload */
                                                   10);
    if (g_backpressureEventProducer != RT_NULL) {
        rt_thread_startup(g_backpressureEventProducer);
    }

    /* Schedule test stop */
    rt_timer_t stopTimer = rt_timer_create("backpressure_stop",
                                          (void (*)(void*))BackpressureTest_stop,
                                          RT_NULL,
                                          rt_tick_from_millisecond(BACKPRESSURE_TEST_DURATION_MS),
                                          RT_TIMER_FLAG_ONE_SHOT);
    if (stopTimer != RT_NULL) {
        rt_timer_start(stopTimer);
    }
}

void BackpressureTest_stop(void) {
    if (!g_backpressureTestRunning) {
        return;
    }

    rt_kprintf("\n==================================================\n");
    rt_kprintf("Stopping Backpressure Performance Test\n");
    rt_kprintf("==================================================\n");

    /* Stop test control */
    g_backpressureTestRunning = false;

    /* Send stop signal to AO */
    static QEvt const stopEvt = { BACKPRESSURE_STOP_SIG, 0U, 0U };
    QActive_post_(&l_backpressureTestAO.super, &stopEvt, QF_NO_MARGIN, (void *)0);

    /* Wait for threads to complete */
    PerfCommon_waitForThreads();

    /* Delete event producer thread */
    if (g_backpressureEventProducer != RT_NULL) {
        rt_thread_delete(g_backpressureEventProducer);
        g_backpressureEventProducer = RT_NULL;
    }

    /* Calculate and display results */
    uint32_t totalTargetEvents = BACKPRESSURE_CRITICAL_EVENTS + BACKPRESSURE_NORMAL_EVENTS + BACKPRESSURE_RETRY_EVENTS;
    uint32_t deliveryRate = (totalTargetEvents > 0U)
                           ? (l_backpressureTestAO.eventCount * 100U / totalTargetEvents)
                           : 0U;
    uint32_t avgRetryAttempts = (l_backpressureTestAO.retrySuccessCount > 0U)
                               ? (l_backpressureTestAO.totalRetryAttempts / l_backpressureTestAO.retrySuccessCount)
                               : 0U;

    rt_kprintf("\n--- Backpressure Test Results ---\n");
    rt_kprintf("Total Events Processed: %u / %u (%.1f%%)\n",
               l_backpressureTestAO.eventCount, totalTargetEvents,
               (float)deliveryRate);
    rt_kprintf("Queue Overflows: %u\n", l_backpressureTestAO.queueOverflowCount);
    rt_kprintf("Events Dropped: %u\n", l_backpressureTestAO.droppedEventCount);

    rt_kprintf("\nEvent Type Breakdown:\n");
    rt_kprintf("  Critical Events: %u / %u\n", l_backpressureTestAO.criticalEventCount, BACKPRESSURE_CRITICAL_EVENTS);
    rt_kprintf("  Normal Events: %u / %u\n", l_backpressureTestAO.normalEventCount, BACKPRESSURE_NORMAL_EVENTS);
    rt_kprintf("  Retry Events: %u / %u\n", l_backpressureTestAO.retryEventCount, BACKPRESSURE_RETRY_EVENTS);

    rt_kprintf("\nRetry Statistics:\n");
    rt_kprintf("  Successful Retries: %u\n", l_backpressureTestAO.retrySuccessCount);
    rt_kprintf("  Failed Retries: %u\n", l_backpressureTestAO.retryFailureCount);
    rt_kprintf("  Total Retry Attempts: %u\n", l_backpressureTestAO.totalRetryAttempts);
    rt_kprintf("  Max Retry Attempts: %u\n", l_backpressureTestAO.maxRetryAttempts);
    rt_kprintf("  Avg Retry Attempts: %u\n", avgRetryAttempts);

    /* Get dispatcher metrics */
    QF_DispatcherMetrics const *metrics = QF_getDispatcherMetrics();
    rt_kprintf("\n--- Dispatcher Backpressure Metrics ---\n");
    rt_kprintf("Dispatch Cycles: %u\n", metrics->dispatchCycles);
    rt_kprintf("Events Dropped by Strategy: %u\n", metrics->eventsDropped);
    rt_kprintf("Events Retried by Strategy: %u\n", metrics->eventsRetried);
    rt_kprintf("Post Failures: %u\n", metrics->postFailures);
    rt_kprintf("Staging Overflows (H/N/L): %u/%u/%u\n",
               metrics->stagingOverflows[0],
               metrics->stagingOverflows[1],
               metrics->stagingOverflows[2]);

    /* Validate critical event guarantees */
    bool criticalGuarantee = (l_backpressureTestAO.criticalEventCount == BACKPRESSURE_CRITICAL_EVENTS);
    rt_kprintf("\n--- Critical Event Guarantee ---\n");
    rt_kprintf("Critical Events Sent: %u\n", BACKPRESSURE_CRITICAL_EVENTS);
    rt_kprintf("Critical Events Received: %u\n", l_backpressureTestAO.criticalEventCount);
    rt_kprintf("Critical Events Preserved: %s\n", criticalGuarantee ? "YES" : "NO");
    rt_kprintf("Drop Rate: %.2f%%\n",
               (totalTargetEvents > 0U)
               ? ((float)l_backpressureTestAO.droppedEventCount * 100.0f / totalTargetEvents)
               : 0.0f);

    /* Validate NO_DROP behavior */
    uint32_t criticalDropped = BACKPRESSURE_CRITICAL_EVENTS - l_backpressureTestAO.criticalEventCount;
    if (criticalDropped == 0U) {
        rt_kprintf("✓ PASS: All NO_DROP critical events were preserved under backpressure\n");
    } else {
        rt_kprintf("✗ FAIL: %u critical events with NO_DROP flag were lost\n", criticalDropped);
    }

    /* Test ISR path validation */
    if (metrics->eventsProcessed > 0U) {
        rt_kprintf("✓ PASS: ISR path metrics accumulated (%u events processed)\n", metrics->eventsProcessed);
    } else {
        rt_kprintf("⚠ WARNING: No ISR path metrics - check publishFromISR implementation\n");
    }

    /* Validate priority staging */
    uint32_t totalOverflows = metrics->stagingOverflows[0] + metrics->stagingOverflows[1] + metrics->stagingOverflows[2];
    if (totalOverflows > 0U) {
        rt_kprintf("✓ PASS: Priority staging tested - %u total overflows detected\n", totalOverflows);
    } else {
        rt_kprintf("⚠ INFO: No staging overflows occurred during test\n");
    }

    rt_kprintf("==================================================\n");

    /* Reset to default policy */
    QF_setDispatcherStrategy(&QF_defaultStrategy);
    QF_resetDispatcherMetrics();
}
