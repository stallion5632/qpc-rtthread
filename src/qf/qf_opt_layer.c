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
* Contact information:
* <www.state-machine.com>
* <info@state-machine.com>
============================================================================*/
/*!
* @date Last updated on: 2025-07-07
* @version Last updated for: @ref qpc_7_3_0
* @author Contributed by: stallion5632
*
* @file
* @brief QF/C optimization layer implementation for RT-Thread port
*/
#define QP_IMPL           /* this is QP implementation */
#include "qf_pkg.h"       /* QF package-scope interface */
#include "qf_opt_layer.h" /* QF optimization layer interface */
#include "qassert.h"      /* QP embedded systems-friendly assertions */
#ifdef Q_SPY              /* QS software tracing enabled? */
    #include "qs_port.h"  /* QS port */
    #include "qs_pkg.h"   /* QS package-scope internal interface */
#else
    #include "qs_dummy.h" /* disable the QS software tracing */
#endif /* Q_SPY */

Q_DEFINE_THIS_MODULE("qf_opt_layer")

static QF_StagingBuffer l_stagingBuffer;
static struct rt_thread l_dispatcherThread;
static void *l_dispatcherStack;
#define DISPATCHER_STACK_SIZE 2048

/* Fast path eligibility check */
bool QF_isEligibleForFastPath(QActive const * const me,
                             QEvt const * const e) {
    rt_base_t cur_prio;
    rt_thread_t cur_thread;
    
    /* Get current thread priority */
    cur_thread = rt_thread_self();
    cur_prio = cur_thread->current_priority;
    
    /* Check if target AO has higher priority than current context */
    if (me->prio > cur_prio && 
        me->thread.stat == RT_THREAD_READY &&
        (e->poolId_ == 0)) { /* Only static events eligible for fast path */
        return true;
    }
    return false;
}

/* Zero-copy event posting */
void QF_zeroCopyPost(QActive * const me, QEvt const * const e,
                     void const * const sender) {
    QF_CRIT_STAT_
    
    QF_CRIT_E_();
    if (e->poolId_ != 0U) { /* Is it a pool event? */
        QEvt_refCtr_inc_(e); /* Increment reference counter */
    }
    
    /* Post pointer directly to mailbox without copying event data */
    rt_err_t err = rt_mb_send((rt_mailbox_t *)&me->eQueue, (rt_ubase_t)e);
    QF_CRIT_X_();
    
    Q_ASSERT_ID(710, err == RT_EOK);
}

/* Staging buffer initialization */
void QF_stagingBufferInit(void) {
    /* Initialize ring buffer */
    l_stagingBuffer.front = 0;
    l_stagingBuffer.rear = 0;
    
    /* Initialize notification semaphore */
    rt_sem_init(&l_stagingBuffer.notify_sem, "staged_evt",
                0, RT_IPC_FLAG_FIFO);
                
    /* Create and start dispatcher thread */
    l_dispatcherStack = rt_malloc(DISPATCHER_STACK_SIZE);
    Q_ASSERT(l_dispatcherStack != RT_NULL);
    
    rt_err_t err = rt_thread_init(&l_dispatcherThread,
                                 "qf_dispatch",
                                 &QF_dispatcherThread,
                                 RT_NULL,
                                 l_dispatcherStack,
                                 DISPATCHER_STACK_SIZE,
                                 RT_THREAD_PRIORITY_MAX - 1,
                                 10);
    Q_ASSERT(err == RT_EOK);
    rt_thread_startup(&l_dispatcherThread);
}

/* Post to staging buffer from ISR */
void QF_stagingBufferPost(QEvt const * const e) {
    QF_CRIT_STAT_
    QF_CRIT_E_();
    rt_base_t next = (l_stagingBuffer.rear + 1) % QF_STAGING_BUFFER_SIZE;
    
    if (next != l_stagingBuffer.front) { /* Buffer not full */
        l_stagingBuffer.events[l_stagingBuffer.rear] = e;
        l_stagingBuffer.rear = next;
        rt_sem_release(&l_stagingBuffer.notify_sem);
    }
    QF_CRIT_X_();
}

/* Dispatcher thread function */
static void QF_dispatcherThread(void *param) {
    while (1) {
        rt_sem_take(&l_stagingBuffer.notify_sem, RT_WAITING_FOREVER);
        
        QF_CRIT_STAT_
        QF_CRIT_E_();
        
        /* Process all events in staging buffer */
        while (l_stagingBuffer.front != l_stagingBuffer.rear) {
            QEvt const *e = l_stagingBuffer.events[l_stagingBuffer.front];
            QActive *target = (QActive *)(((QEvt *)e)->target);
            
            /* Post to target AO using zero-copy mechanism */
            QF_zeroCopyPost(target, e, 0);
            
            l_stagingBuffer.front = 
                (l_stagingBuffer.front + 1) % QF_STAGING_BUFFER_SIZE;
        }
        
        QF_CRIT_X_();
    }
}

/* Implementation of QF_dispatchBatchedEvents */
void QF_dispatchBatchedEvents(void) {
    QF_CRIT_STAT_
    QF_CRIT_E_();
    
    /* Process all events in the staging buffer */
    while (l_stagingBuffer.front != l_stagingBuffer.rear) {
        QEvt const *e = l_stagingBuffer.events[l_stagingBuffer.front];
        QActive *target = (QActive *)(((QEvt *)e)->target);
        
        /* Process event immediately */
        QHSM_DISPATCH(&target->super, e, target->prio);
        QF_gc(e); /* check if the event is garbage, and collect it if so */
        
        l_stagingBuffer.front = 
            (l_stagingBuffer.front + 1) % QF_STAGING_BUFFER_SIZE;
    }
    
    QF_CRIT_X_();
}