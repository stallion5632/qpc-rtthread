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
#ifdef Q_SPY              /* QS software tracing enabled? */
    #include "qs_port.h"  /* QS port */
    #include "qs_pkg.h"   /* QS package-scope internal interface */
#else
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
static struct {
    struct {
        QEvt const *evt;
        QActive *target;
        uint32_t timestamp;
    } buffer[QF_STAGING_BUFFER_SIZE];
    volatile uint32_t front;
    volatile uint32_t rear;
    volatile uint32_t count;
} l_stagingBuffers[QF_PRIO_LEVELS];

/* Dispatcher control */
static struct {
    struct rt_semaphore sem;
    QF_DispatcherMetrics metrics;
    uint32_t totalBatchSize;
    uint32_t batchCount;
    bool enabled;
} l_dispatcher;

/* Strategy pointer - const qualified for MISRA C compliance */
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
QF_DispatcherStrategy const QF_defaultStrategy = {
    .shouldMerge = defaultShouldMerge,
    .comparePriority = defaultComparePriority,
    .shouldDrop = defaultShouldDrop,
    .getPrioLevel = defaultGetPrioLevel
};

QF_DispatcherStrategy const QF_highPerfStrategy = {
    .shouldMerge = highPerfShouldMerge,
    .comparePriority = highPerfComparePriority,
    .shouldDrop = highPerfShouldDrop,
    .getPrioLevel = highPerfGetPrioLevel
};

/*..........................................................................*/
void QF_initOptLayer(void) {
    /* Initialize all priority staging buffers */
    for (uint8_t i = 0; i < QF_PRIO_LEVELS; ++i) {
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

/*..........................................................................*/
void QF_setDispatcherStrategy(QF_DispatcherStrategy const *strategy) {
    Q_REQUIRE_ID(100, strategy != (QF_DispatcherStrategy *)0);
    l_policy = strategy;
}

/*..........................................................................*/
QF_DispatcherStrategy const *QF_getDispatcherPolicy(void) {
    return l_policy;
}

/*..........................................................................*/
static void dispatcherThreadEntry(void *parameter) {
    Q_UNUSED_PAR(parameter);
    
    /* Local batch arrays for processing */
    QEvt const *eventBatch[QF_STAGING_BUFFER_SIZE];
    QActive *targetBatch[QF_STAGING_BUFFER_SIZE];
    uint32_t batchSize;
    
    for (;;) {
        /* Wait for events to be available */
        rt_sem_take(&l_dispatcher.sem, RT_WAITING_FOREVER);
        
        /* Update dispatch cycle count */
        l_dispatcher.metrics.dispatchCycles++;
        
        /* Process events by priority: HIGH -> NORMAL -> LOW */
        for (uint8_t prio = QF_PRIO_HIGH; prio < QF_PRIO_LEVELS; ++prio) {
            batchSize = QF_popAllFromStagingBuffer((QF_PrioLevel)prio, 
                                                  eventBatch, 
                                                  targetBatch, 
                                                  QF_STAGING_BUFFER_SIZE);
            
            if (batchSize > 0U) {
                /* Update metrics */
                l_dispatcher.metrics.eventsProcessed += batchSize;
                l_dispatcher.totalBatchSize += batchSize;
                l_dispatcher.batchCount++;
                
                if (batchSize > l_dispatcher.metrics.maxBatchSize) {
                    l_dispatcher.metrics.maxBatchSize = batchSize;
                }
                
                /* Process the batch */
                QF_processEventBatch(eventBatch, targetBatch, batchSize);
            }
        }
        
        /* Update average batch size */
        if (l_dispatcher.batchCount > 0U) {
            l_dispatcher.metrics.avgBatchSize = l_dispatcher.totalBatchSize / l_dispatcher.batchCount;
        }
    }
}

/*..........................................................................*/
static void QF_processEventBatch(QEvt const **eventBatch, 
                                QActive **targetBatch, 
                                uint32_t batchSize) {
    QF_DispatcherStrategy const *strategy = l_policy;
    
    /* Apply strategy-based optimizations */
    for (uint32_t i = 0; i < batchSize; ++i) {
        QEvt const *evt = eventBatch[i];
        QActive *target = targetBatch[i];
        
        if (evt == (QEvt const *)0 || target == (QActive *)0) {
            continue;
        }
        
        /* Check if event should be dropped */
        if (strategy->shouldDrop(evt, target)) {
            l_dispatcher.metrics.eventsDropped++;
            QF_gc(evt);
            continue;
        }
        
        /* Check for merge opportunities with next events */
        bool merged = false;
        if (strategy->shouldMerge != (bool (*)(QEvt const *, QEvt const *))0) {
            for (uint32_t j = i + 1; j < batchSize; ++j) {
                if (eventBatch[j] != (QEvt const *)0 && 
                    targetBatch[j] == target &&
                    strategy->shouldMerge(evt, eventBatch[j])) {
                    /* Merge events - keep the later one, discard the earlier */
                    QF_gc(evt);
                    eventBatch[i] = (QEvt const *)0;  /* Mark as processed */
                    l_dispatcher.metrics.eventsMerged++;
                    merged = true;
                    break;
                }
            }
        }
        
        if (!merged) {
            /* Try to post the event */
            rt_err_t result = rt_mb_send(&target->eQueue, (rt_ubase_t)evt);
            
            if (result != RT_EOK) {
                /* Post failed - apply backpressure strategy */
                if (QF_retryEvent(evt, target)) {
                    l_dispatcher.metrics.eventsRetried++;
                } else {
                    l_dispatcher.metrics.eventsDropped++;
                    l_dispatcher.metrics.postFailures++;
                    QF_gc(evt);
                }
            }
        }
    }
}

/*..........................................................................*/
static bool QF_retryEvent(QEvt const *evt, QActive *target) {
    QEvtEx *evtEx = (QEvtEx *)evt;
    
    /* Check if event supports retry */
    if (evtEx->super.sig == 0U) {
        /* Not an extended event - cannot retry */
        return false;
    }
    
    /* Check retry count */
    if (evtEx->retryCount >= QF_MAX_RETRY_COUNT) {
        /* Maximum retries exceeded */
        return false;
    }
    
    /* Check if event is marked as critical (must not be dropped) */
    if ((evtEx->flags & QF_EVT_FLAG_NO_DROP) != 0U) {
        /* Put back in low priority queue for retry */
        evtEx->retryCount++;
        return QF_addToStagingBuffer(QF_PRIO_LOW, evt, target);
    }
    
    return false;
}

/*..........................................................................*/
bool QF_postFromISR(QActive * const me, QEvt const * const e) {
    if (!l_dispatcher.enabled) {
        return false;
    }
    
    /* Determine priority level using strategy */
    QF_PrioLevel prioLevel = l_policy->getPrioLevel(e);
    
    /* Add to appropriate staging buffer */
    if (QF_addToStagingBuffer(prioLevel, e, me)) {
        /* Increment reference counter for dynamic events */
        if (e->poolId_ != 0U) {
            QEvt_refCtr_inc_(e);
        }
        
        /* Notify kernel: entering ISR */
        rt_interrupt_enter();
        
        /* Signal the dispatcher thread (PendSV defers switching) */
        rt_sem_release(&l_dispatcher.sem);
        
        /* Notify kernel: leaving ISR */
        rt_interrupt_leave();
        
        return true;
    }
    
    return false;
}

/*..........................................................................*/
void QF_publishFromISR(QEvt const * const e, void const * const sender) {
    Q_UNUSED_PAR(sender);
    
    if (!l_dispatcher.enabled) {
        return;
    }
    
    /* Determine priority level using strategy */
    QF_PrioLevel prioLevel = l_policy->getPrioLevel(e);
    
    /* Process all active objects that might be interested in this event */
    for (uint8_t p = 1U; p <= QF_MAX_ACTIVE; ++p) {
        QActive * const a = QActive_registry_[p];
        if (a != (QActive *)0) {
            /* Check if this AO subscribes to this signal (simplified) */
            /* In a full implementation, you'd check the subscription table */
            
            /* Add to appropriate staging buffer for each subscribed AO */
            if (QF_addToStagingBuffer(prioLevel, e, a)) {
                /* Increment reference counter for dynamic events */
                if (e->poolId_ != 0U) {
                    QEvt_refCtr_inc_(e);
                }
            }
        }
    }
    
    /* Notify kernel: entering ISR */
    rt_interrupt_enter();
    
    /* Release semaphore to signal dispatcher thread (PendSV defers switching) */
    rt_sem_release(&l_dispatcher.sem);
    
    /* Notify kernel: leaving ISR */
    rt_interrupt_leave();
}

/*..........................................................................*/
static bool QF_addToStagingBuffer(QF_PrioLevel prioLevel, QEvt const *evt, QActive *target) {
    volatile uint32_t rear = l_stagingBuffers[prioLevel].rear;
    volatile uint32_t next = (rear + 1U) % QF_STAGING_BUFFER_SIZE;
    
    /* Check if buffer is full */
    if (next == l_stagingBuffers[prioLevel].front) {
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

/*..........................................................................*/
static uint32_t QF_popAllFromStagingBuffer(QF_PrioLevel prioLevel, 
                                           QEvt const **eventBatch, 
                                           QActive **targetBatch, 
                                           uint32_t maxSize) {
    uint32_t count = 0;
    
    /* Extract all events from the staging buffer */
    while (l_stagingBuffers[prioLevel].front != l_stagingBuffers[prioLevel].rear && 
           count < maxSize) {
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

/*..........................................................................*/
uint32_t QF_getTimestamp(void) {
    return rt_tick_get();
}

/*..........................................................................*/
QEvtEx *QF_newEvtEx(enum_t const sig, uint16_t const evtSize, uint8_t priority, uint8_t flags) {
    QEvtEx *evtEx = (QEvtEx *)QF_newX_(evtSize, 0U, sig);
    
    if (evtEx != (QEvtEx *)0) {
        evtEx->timestamp = QF_getTimestamp();
        evtEx->priority = priority;
        evtEx->flags = flags;
        evtEx->retryCount = 0U;     /* Clear residual retry count */
        evtEx->reserved = 0U;       /* Clear reserved field */
    }
    
    return evtEx;
}

/*..........................................................................*/
uint32_t QF_getLostEventCount(void) {
    uint32_t total = 0;
    for (uint8_t i = 0; i < QF_PRIO_LEVELS; ++i) {
        total += l_dispatcher.metrics.stagingOverflows[i];
    }
    return total;
}

/*..........................................................................*/
void QF_enableOptLayer(void) {
    l_dispatcher.enabled = true;
}

/*..........................................................................*/
void QF_disableOptLayer(void) {
    l_dispatcher.enabled = false;
}

/*..........................................................................*/
QF_DispatcherMetrics const *QF_getDispatcherMetrics(void) {
    return &l_dispatcher.metrics;
}

/*..........................................................................*/
void QF_resetDispatcherMetrics(void) {
    rt_memset(&l_dispatcher.metrics, 0, sizeof(l_dispatcher.metrics));
    l_dispatcher.totalBatchSize = 0U;
    l_dispatcher.batchCount = 0U;
}

/*..........................................................................*/
static void QF_idleHook(void) {
    /* Check if any staging buffer has pending events */
    if (l_dispatcher.enabled) {
        for (uint8_t i = 0; i < QF_PRIO_LEVELS; ++i) {
            if (l_stagingBuffers[i].front != l_stagingBuffers[i].rear) {
                rt_sem_release(&l_dispatcher.sem);
                break;
            }
        }
    }
}

/*..........................................................................*/
/* Default strategy implementations */
/*..........................................................................*/
static bool defaultShouldMerge(QEvt const *prev, QEvt const *next) {
    /* Default: merge events with same signal */
    return (prev->sig == next->sig);
}

static int defaultComparePriority(QEvt const *a, QEvt const *b) {
    /* Default: compare by signal value */
    return (int)(a->sig - b->sig);
}

static bool defaultShouldDrop(QEvt const *evt, QActive const *targetAO) {
    Q_UNUSED_PAR(targetAO);
    /* Default: don't drop any events */
    return false;
}

static QF_PrioLevel defaultGetPrioLevel(QEvt const *evt) {
    /* Default: all events go to normal priority */
    Q_UNUSED_PAR(evt);
    return QF_PRIO_NORMAL;
}

/*..........................................................................*/
/* High performance strategy implementations */
/*..........................................................................*/
static bool highPerfShouldMerge(QEvt const *prev, QEvt const *next) {
    QEvtEx const *prevEx = (QEvtEx const *)prev;
    QEvtEx const *nextEx = (QEvtEx const *)next;
    
    /* High perf: merge only if explicitly marked as mergeable */
    if (prevEx->super.sig != 0U && nextEx->super.sig != 0U) {
        return (prev->sig == next->sig && 
                (prevEx->flags & QF_EVT_FLAG_MERGEABLE) != 0U &&
                (nextEx->flags & QF_EVT_FLAG_MERGEABLE) != 0U);
    }
    
    return false;
}

static int highPerfComparePriority(QEvt const *a, QEvt const *b) {
    QEvtEx const *aEx = (QEvtEx const *)a;
    QEvtEx const *bEx = (QEvtEx const *)b;
    
    /* High perf: use explicit priority if available */
    if (aEx->super.sig != 0U && bEx->super.sig != 0U) {
        return (int)(aEx->priority - bEx->priority);
    }
    
    /* Fallback to signal comparison */
    return (int)(a->sig - b->sig);
}

static bool highPerfShouldDrop(QEvt const *evt, QActive const *targetAO) {
    QEvtEx const *evtEx = (QEvtEx const *)evt;
    
    /* High perf: drop events that are not critical and queue is getting full */
    if (evtEx->super.sig != 0U && (evtEx->flags & QF_EVT_FLAG_CRITICAL) == 0U) {
        /* Check target queue depth */
        uint32_t queueDepth = targetAO->eQueue.entry;
        uint32_t queueSize = targetAO->eQueue.size;
        
        /* Drop if queue is more than 80% full */
        if (queueDepth > (queueSize * 4U / 5U)) {
            return true;
        }
    }
    
    return false;
}

static QF_PrioLevel highPerfGetPrioLevel(QEvt const *evt) {
    QEvtEx const *evtEx = (QEvtEx const *)evt;
    
    /* High perf: use explicit priority mapping */
    if (evtEx->super.sig != 0U) {
        if ((evtEx->flags & QF_EVT_FLAG_CRITICAL) != 0U) {
            return QF_PRIO_HIGH;
        } else if (evtEx->priority > 128U) {
            return QF_PRIO_HIGH;
        } else if (evtEx->priority > 64U) {
            return QF_PRIO_NORMAL;
        } else {
            return QF_PRIO_LOW;
        }
    }
    
    /* Default for regular events */
    return QF_PRIO_NORMAL;
}
