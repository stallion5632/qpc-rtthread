/*============================================================================
* QP/C Real-Time Embedded Framework (RTEF)
* Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
*
* SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
*
* This software is dual-licensed under the terms of the open source GNU
* General Public License version 3 (or any later version), or alternatively,
* under the terms of one of the closed source Quantum Leaps commercial
* licenses.
*
* The terms of the open source GNU General Public License version 3
* can be found at: <www.gnu.org/licenses/gpl-3.0>
*
* The terms of the closed source Quantum Leaps commercial licenses
* can be found at: <www.state-machine.com/licensing>
*
* Redistributions in source code must retain this top-level comment block.
* Plagiarizing this software to sidestep the license obligations is illegal.
*
* Contact information:
* <www.state-machine.com>
* <info@state-machine.com>
============================================================================*/
/*!
* @date Last updated on: 2024-01-08
* @version Last updated for: @ref qpc_7_3_0
*
* @file
* @brief QF/C optimization layer for RT-Thread
*/
#define QP_IMPL           /* this is QP implementation */
#include "qf_port.h"      /* QF port */
#include "qf_pkg.h"       /* QF package-scope interface */
#include "qassert.h"
#ifndef Q_SPY              /* QS software tracing enabled? */
    #include "qs_dummy.h" /* disable the QS software tracing */
#endif /* Q_SPY */

Q_DEFINE_THIS_MODULE("qf_opt_layer")

/* Configuration macros */
#ifndef QF_STAGING_BUFFER_SIZE
#define QF_STAGING_BUFFER_SIZE 32U
#endif

/* Static dispatcher stack */
static uint8_t dispatcherStack[QF_DISPATCHER_STACK_SIZE] __attribute__((aligned(RT_ALIGN_SIZE)));
static struct rt_thread dispatcherThreadObj;

/* Partitioned staging buffers by priority */
static struct
{
    struct
    {
        QEvt const *evt;
        QActive *target;
        uint32_t timestamp;
    } buffer[QF_STAGING_BUFFER_SIZE];
    volatile uint32_t front;
    volatile uint32_t rear;
    volatile uint32_t count;
} l_stagingBuffers[QF_PRIO_LEVELS];

/* Dispatcher control */
static struct
{
    struct rt_semaphore sem;
    QF_DispatcherMetrics metrics;
    uint32_t totalBatchSize;
    uint32_t batchCount;
    bool enabled;
} l_dispatcher;

static QF_DispatcherStrategy const *l_policy = &QF_defaultStrategy;

/* Forward declarations */
static void dispatcherThreadEntry(void *parameter);
static void QF_idleHook(void);
static bool QF_addToStagingBuffer(QF_PrioLevel prioLevel, QEvt const *evt, QActive *target);
static uint32_t QF_popAllFromStagingBuffer(QF_PrioLevel prioLevel,
                                           QEvt const **eventBatch,
                                           QActive **targetBatch,
                                           uint32_t maxSize);
static void QF_processEventBatch(QEvt const **eventBatch,
                                 QActive **targetBatch,
                                 uint32_t batchSize);
static bool QF_retryEvent(QEvt const *evt, QActive *target);

/* Strategy implementations */
static bool defaultShouldMerge(QEvt const *prev, QEvt const *next);
static int defaultComparePriority(QEvt const *a, QEvt const *b);
static bool defaultShouldDrop(QEvt const *evt, QActive const *targetAO);
static QF_PrioLevel defaultGetPrioLevel(QEvt const *evt);

static bool highPerfShouldMerge(QEvt const *prev, QEvt const *next);
static int highPerfComparePriority(QEvt const *a, QEvt const *b);
static bool highPerfShouldDrop(QEvt const *evt, QActive const *targetAO);
static QF_PrioLevel highPerfGetPrioLevel(QEvt const *evt);

/* Default strategy implementations */
/**
 * @brief Default dispatcher strategy implementation
 */
QF_DispatcherStrategy const QF_defaultStrategy = {
    .shouldMerge = defaultShouldMerge,
    .comparePriority = defaultComparePriority,
    .shouldDrop = defaultShouldDrop,
    .getPrioLevel = defaultGetPrioLevel
};

/**
 * @brief High performance dispatcher strategy implementation
 */
QF_DispatcherStrategy const QF_highPerfStrategy = {
    .shouldMerge = highPerfShouldMerge,
    .comparePriority = highPerfComparePriority,
    .shouldDrop = highPerfShouldDrop,
    .getPrioLevel = highPerfGetPrioLevel
};


/**
 * @brief Initialize the QF optimization layer, including buffers, dispatcher, and thread
 */
void QF_initOptLayer(void)
{
    /* Initialize all priority staging buffers */
    for (uint8_t i = 0; i < QF_PRIO_LEVELS; ++i)
    {
        l_stagingBuffers[i].front = 0U;
        l_stagingBuffers[i].rear = 0U;
        l_stagingBuffers[i].count = 0U;
    }

    /* Initialize dispatcher control */
    l_dispatcher.enabled = true;
    rt_memset(&l_dispatcher.metrics, 0, sizeof(l_dispatcher.metrics));
    l_dispatcher.totalBatchSize = 0U;
    l_dispatcher.batchCount = 0U;

    /* Initialize semaphore */
    rt_sem_init(&l_dispatcher.sem, "qf_disp_sem", 0, RT_IPC_FLAG_FIFO);

    /* Create dispatcher thread with highest priority */
    rt_thread_init(&dispatcherThreadObj,
                   "qf_dispatcher",
                   dispatcherThreadEntry,
                   RT_NULL,
                   dispatcherStack,
                   sizeof(dispatcherStack),
                   QF_DISPATCHER_PRIORITY,
                   1);

    rt_thread_startup(&dispatcherThreadObj);

    /* Set idle hook to check staging buffers */
    rt_thread_idle_sethook(QF_idleHook);
}


/**
 * @brief Set the dispatcher strategy
 * @param strategy Pointer to dispatcher strategy
 */
void QF_setDispatcherStrategy(QF_DispatcherStrategy const *strategy)
{
    Q_REQUIRE_ID(100, strategy != (QF_DispatcherStrategy *)0);
    l_policy = strategy;
}

/**
 * @brief Get the current dispatcher strategy
 * @return Pointer to dispatcher strategy
 */
QF_DispatcherStrategy const *QF_getDispatcherPolicy(void)
{
    return l_policy;
}

/**
 * @brief Dispatcher thread entry, processes event batches in priority order
 * @param parameter Thread parameter (unused)
 */
static void dispatcherThreadEntry(void *parameter)
{
    Q_UNUSED_PAR(parameter);

    /* Local batch arrays for processing */
    QEvt const *eventBatch[QF_STAGING_BUFFER_SIZE];
    QActive *targetBatch[QF_STAGING_BUFFER_SIZE];
    uint32_t batchSize;

    for (;;)
    {
        /* Wait for events to be available */
        rt_sem_take(&l_dispatcher.sem, RT_WAITING_FOREVER);

        /* Update dispatch cycle count */
        l_dispatcher.metrics.dispatchCycles++;

        /* Process events by priority: HIGH -> NORMAL -> LOW */
        for (uint8_t prio = QF_PRIO_HIGH; prio < QF_PRIO_LEVELS; ++prio)
        {
            batchSize = QF_popAllFromStagingBuffer((QF_PrioLevel)prio,
                                                   eventBatch,
                                                   targetBatch,
                                                   QF_STAGING_BUFFER_SIZE);

            if (batchSize > 0U)
            {
                /* Update metrics */
                l_dispatcher.metrics.eventsProcessed += batchSize;
                l_dispatcher.totalBatchSize += batchSize;
                l_dispatcher.batchCount++;

                if (batchSize > l_dispatcher.metrics.maxBatchSize)
                {
                    l_dispatcher.metrics.maxBatchSize = batchSize;
                }

                /* Process the batch */
                QF_processEventBatch(eventBatch, targetBatch, batchSize);
            }
        }

        /* Update average batch size */
        if (l_dispatcher.batchCount > 0U)
        {
            l_dispatcher.metrics.avgBatchSize = l_dispatcher.totalBatchSize / l_dispatcher.batchCount;
        }
    }
}

/**
 * @brief Process a batch of events, applying strategy for drop/merge/post
 * @param eventBatch Array of events
 * @param targetBatch Array of target active objects
 * @param batchSize Number of events in batch
 */
static void QF_processEventBatch(QEvt const **eventBatch,
                                 QActive **targetBatch,
                                 uint32_t batchSize)
{
    QF_DispatcherStrategy const *strategy = l_policy;

    /* Apply strategy-based optimizations */
    for (uint32_t i = 0; i < batchSize; ++i)
    {
        QEvt const *evt = eventBatch[i];
        QActive *target = targetBatch[i];

        if (evt == (QEvt const *)0 || target == (QActive *)0)
        {
            continue;
        }

        /* Check if event should be dropped */
        if (strategy->shouldDrop(evt, target))
        {
            l_dispatcher.metrics.eventsDropped++;
            QF_gc(evt);
            continue;
        }

        /* Check for merge opportunities with next events */
        bool merged = false;
        if (strategy->shouldMerge != (bool (*)(QEvt const *, QEvt const *))0)
        {
            for (uint32_t j = i + 1; j < batchSize; ++j)
            {
                if (eventBatch[j] != (QEvt const *)0 &&
                    targetBatch[j] == target &&
                    strategy->shouldMerge(evt, eventBatch[j]))
                {
                    /* Merge events - keep the later one, discard the earlier */
                    QF_gc(evt);
                    eventBatch[i] = (QEvt const *)0; /* Mark as processed */
                    l_dispatcher.metrics.eventsMerged++;
                    merged = true;
                    break;
                }
            }
        }

        if (!merged)
        {
            /* Try to post the event */
            rt_err_t result = rt_mb_send(&target->eQueue, (rt_ubase_t)evt);

            if (result != RT_EOK)
            {
                /* Post failed - apply backpressure strategy */
                if (QF_retryEvent(evt, target))
                {
                    l_dispatcher.metrics.eventsRetried++;
                }
                else
                {
                    l_dispatcher.metrics.eventsDropped++;
                    l_dispatcher.metrics.postFailures++;
                    QF_gc(evt);
                }
            }
        }
    }
}

/**
 * @brief Retry posting an event if allowed, put back to low priority buffer
 * @param evt Event pointer
 * @param target Target active object
 * @return true if retried, false otherwise
 */
static bool QF_retryEvent(QEvt const *evt, QActive *target)
{
    bool retVal = false;
    QEvtEx *evtEx = (QEvtEx *)evt;

    /* Check if event supports retry */
    if (evtEx->super.sig != 0U)
    {
        /* Check retry count */
        if (evtEx->retryCount < QF_MAX_RETRY_COUNT)
        {
            /* Check if event is marked as critical (must not be dropped) */
            if ((evtEx->flags & QF_EVT_FLAG_NO_DROP) != 0U)
            {
                /* Put back in low priority queue for retry */
                evtEx->retryCount++;
                retVal = QF_addToStagingBuffer(QF_PRIO_LOW, evt, target);
            }
        }
    }

    return retVal;
}

/**
 * @brief Post an event from ISR context to the staging buffer
 * @param me Target active object
 * @param e Event pointer
 * @return true if posted successfully, false otherwise
 */
bool QF_postFromISR(QActive *const me, QEvt const *const e)
{
    bool retVal = false;
    if (l_dispatcher.enabled)
    {
        /* Determine priority level using strategy */
        QF_PrioLevel prioLevel = l_policy->getPrioLevel(e);

        /* Add to appropriate staging buffer */
        if (QF_addToStagingBuffer(prioLevel, e, me))
        {
            /* Increment reference counter for dynamic events */
            if (e->poolId_ != 0U)
            {
                QEvt_refCtr_inc_(e);
            }
            /* Signal the dispatcher thread */
            rt_sem_release(&l_dispatcher.sem);
            retVal = true;
        }
    }
    return retVal;
}

/**
 * @brief Add an event to the staging buffer of specified priority
 * @param prioLevel Priority level
 * @param evt Event pointer
 * @param target Target active object
 * @return true if added successfully, false if buffer full
 */
static bool QF_addToStagingBuffer(QF_PrioLevel prioLevel, QEvt const *evt, QActive *target)
{
    volatile uint32_t rear = l_stagingBuffers[prioLevel].rear;
    volatile uint32_t next = (rear + 1U) % QF_STAGING_BUFFER_SIZE;

    /* Check if buffer is full */
    if (next == l_stagingBuffers[prioLevel].front)
    {
        /* Buffer overflow */
        l_dispatcher.metrics.stagingOverflows[prioLevel]++;
        return false;
    }

    /* Store event in staging buffer */
    l_stagingBuffers[prioLevel].buffer[rear].evt = evt;
    l_stagingBuffers[prioLevel].buffer[rear].target = target;
    l_stagingBuffers[prioLevel].buffer[rear].timestamp = QF_getTimestamp();

    /* Update rear index atomically */
    l_stagingBuffers[prioLevel].rear = next;
    l_stagingBuffers[prioLevel].count++;

    return true;
}

/**
 * @brief Pop all events from the staging buffer of specified priority
 * @param prioLevel Priority level
 * @param eventBatch Output array for events
 * @param targetBatch Output array for targets
 * @param maxSize Maximum number to pop
 * @return Number of events popped
 */
static uint32_t QF_popAllFromStagingBuffer(QF_PrioLevel prioLevel,
                                           QEvt const **eventBatch,
                                           QActive **targetBatch,
                                           uint32_t maxSize)
{
    uint32_t count = 0;

    /* Extract all events from the staging buffer */
    while (l_stagingBuffers[prioLevel].front != l_stagingBuffers[prioLevel].rear &&
           count < maxSize)
    {
        uint32_t front = l_stagingBuffers[prioLevel].front;

        eventBatch[count] = l_stagingBuffers[prioLevel].buffer[front].evt;
        targetBatch[count] = l_stagingBuffers[prioLevel].buffer[front].target;

        /* Update front index */
        l_stagingBuffers[prioLevel].front = (front + 1U) % QF_STAGING_BUFFER_SIZE;
        l_stagingBuffers[prioLevel].count--;

        count++;
    }

    return count;
}

/**
 * @brief Get current timestamp (tick count)
 * @return Current tick
 */
uint32_t QF_getTimestamp(void)
{
    return rt_tick_get();
}

/**
 * @brief Create a new extended event with timestamp, priority, flags
 * @param sig Signal
 * @param evtSize Event size
 * @param priority Event priority
 * @param flags Event flags
 * @return Pointer to new extended event
 */
QEvtEx *QF_newEvtEx(enum_t const sig, uint16_t const evtSize, uint8_t priority, uint8_t flags)
{
    QEvtEx *evtEx = (QEvtEx *)QF_newX_(evtSize, 0U, sig);

    if (evtEx != (QEvtEx *)0)
    {
        evtEx->timestamp = QF_getTimestamp();
        evtEx->priority = priority;
        evtEx->flags = flags;
        evtEx->retryCount = 0U;
        evtEx->reserved = 0U;
    }

    return evtEx;
}

/**
 * @brief Get total lost event count due to buffer overflow
 * @return Total lost events
 */
uint32_t QF_getLostEventCount(void)
{
    uint32_t total = 0;
    for (uint8_t i = 0; i < QF_PRIO_LEVELS; ++i)
    {
        total += l_dispatcher.metrics.stagingOverflows[i];
    }
    return total;
}

/**
 * @brief Enable the QF optimization layer
 */
void QF_enableOptLayer(void)
{
    l_dispatcher.enabled = true;
}

/**
 * @brief Disable the QF optimization layer
 */
void QF_disableOptLayer(void)
{
    l_dispatcher.enabled = false;
}

/**
 * @brief Get pointer to dispatcher metrics
 * @return Pointer to metrics struct
 */
QF_DispatcherMetrics const *QF_getDispatcherMetrics(void)
{
    return &l_dispatcher.metrics;
}

/**
 * @brief Reset dispatcher metrics and batch counters
 */
void QF_resetDispatcherMetrics(void)
{
    rt_memset(&l_dispatcher.metrics, 0, sizeof(l_dispatcher.metrics));
    l_dispatcher.totalBatchSize = 0U;
    l_dispatcher.batchCount = 0U;
}

/**
 * @brief Idle hook to check and signal dispatcher if events are pending
 */
static void QF_idleHook(void)
{
    /* Check if any staging buffer has pending events */
    if (l_dispatcher.enabled)
    {
        for (uint8_t i = 0; i < QF_PRIO_LEVELS; ++i)
        {
            if (l_stagingBuffers[i].front != l_stagingBuffers[i].rear)
            {
                rt_sem_release(&l_dispatcher.sem);
                break;
            }
        }
    }
}

/* Default strategy implementations */

/**
 * @brief Default strategy: merge events with same signal
 * @param prev Previous event
 * @param next Next event
 * @return true if should merge
 */
static bool defaultShouldMerge(QEvt const *prev, QEvt const *next)
{
    /* Default: merge events with same signal */
    return (prev->sig == next->sig);
}

/**
 * @brief Default strategy: compare priority by signal value
 * @param a First event
 * @param b Second event
 * @return Comparison result
 */
static int defaultComparePriority(QEvt const *a, QEvt const *b)
{
    /* Default: compare by signal value */
    return (int)(a->sig - b->sig);
}

/**
 * @brief Default strategy: never drop events
 * @param evt Event pointer
 * @param targetAO Target active object
 * @return false always
 */
static bool defaultShouldDrop(QEvt const *evt, QActive const *targetAO)
{
    Q_UNUSED_PAR(targetAO);
    /* Default: don't drop any events */
    return false;
}

/**
 * @brief Default strategy: all events go to normal priority
 * @param evt Event pointer
 * @return Normal priority
 */
static QF_PrioLevel defaultGetPrioLevel(QEvt const *evt)
{
    /* Default: all events go to normal priority */
    Q_UNUSED_PAR(evt);
    return QF_PRIO_NORMAL;
}

/* High performance strategy implementations */

/**
 * @brief High performance strategy: merge only if marked mergeable
 * @param prev Previous event
 * @param next Next event
 * @return true if should merge
 */
static bool highPerfShouldMerge(QEvt const *prev, QEvt const *next)
{
    QEvtEx const *prevEx = (QEvtEx const *)prev;
    QEvtEx const *nextEx = (QEvtEx const *)next;

    /* High perf: merge only if explicitly marked as mergeable */
    if (prevEx->super.sig != 0U && nextEx->super.sig != 0U)
    {
        return (prev->sig == next->sig &&
                (prevEx->flags & QF_EVT_FLAG_MERGEABLE) != 0U &&
                (nextEx->flags & QF_EVT_FLAG_MERGEABLE) != 0U);
    }

    return false;
}

/**
 * @brief High performance strategy: compare by explicit priority if available
 * @param a First event
 * @param b Second event
 * @return Comparison result
 */
static int highPerfComparePriority(QEvt const *a, QEvt const *b)
{
    QEvtEx const *aEx = (QEvtEx const *)a;
    QEvtEx const *bEx = (QEvtEx const *)b;

    /* High perf: use explicit priority if available */
    if (aEx->super.sig != 0U && bEx->super.sig != 0U)
    {
        return (int)(aEx->priority - bEx->priority);
    }

    /* Fallback to signal comparison */
    return (int)(a->sig - b->sig);
}

/**
 * @brief High performance strategy: drop non-critical events if queue is full
 * @param evt Event pointer
 * @param targetAO Target active object
 * @return true if should drop
 */
static bool highPerfShouldDrop(QEvt const *evt, QActive const *targetAO)
{
    bool retVal = false;
    QEvtEx const *evtEx = (QEvtEx const *)evt;

    /* High perf: drop events that are not critical and queue is getting full */
    if ((evtEx->super.sig != 0U) && ((evtEx->flags & QF_EVT_FLAG_CRITICAL) == 0U))
    {
        /* Check target queue depth */
        uint32_t queueDepth = targetAO->eQueue.entry;
        uint32_t queueSize = targetAO->eQueue.size;

        /* Drop if queue is more than 80% full */
        if (queueDepth > (queueSize * 4U / 5U))
        {
            retVal = true;
        }
    }

    return retVal;
}

/**
 * @brief High performance strategy: map event to priority level
 * @param evt Event pointer
 * @return Priority level
 */
static QF_PrioLevel highPerfGetPrioLevel(QEvt const *evt)
{
    QF_PrioLevel prio = QF_PRIO_NORMAL;
    QEvtEx const *evtEx = (QEvtEx const *)evt;

    /* High perf: use explicit priority mapping */
    if (evtEx->super.sig != 0U)
    {
        if ((evtEx->flags & QF_EVT_FLAG_CRITICAL) != 0U)
        {
            prio = QF_PRIO_HIGH;
        }
        else if (evtEx->priority > 128U)
        {
            prio = QF_PRIO_HIGH;
        }
        else if (evtEx->priority > 64U)
        {
            prio = QF_PRIO_NORMAL;
        }
        else
        {
            prio = QF_PRIO_LOW;
        }
    }
    /* Default for regular events is QF_PRIO_NORMAL */
    return prio;
}
