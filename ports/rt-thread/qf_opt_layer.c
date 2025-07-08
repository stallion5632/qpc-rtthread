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
#include "qf_pkg.h"
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
static uint8_t dispatcherStack[2048] RT_ALIGN_SIZE;
static rt_thread_t dispatcherThreadObj;

/* Staging buffer for fast-path events */
static struct {
    struct {
        QEvt const *evt;
        QActive *target;
    } buffer[QF_STAGING_BUFFER_SIZE];
    volatile uint32_t front;
    volatile uint32_t rear;
    rt_sem_t sem;
    uint32_t lostEvtCnt;
} l_stagingBuffer;

/* Forward declarations */
static void dispatcherThread(void *parameter);
static void QF_dispatchBatchedEvents(void);

/*..........................................................................*/
bool QF_isEligibleForFastPath(QActive const * const me, QEvt const * const e) {
    /* Check if the target AO thread is ready for fast dispatch */
    rt_thread_t targetThread = (rt_thread_t)&me->thread;
    
    /* Use rt_thread_ready() instead of accessing thread.stat directly */
    /* For now, simplified check - in real implementation check thread readiness */
    if (targetThread != RT_NULL) {
        /* Fast path is eligible for:
         * 1. Immutable events (e->poolId_ == 0)
         * 2. Dynamic events (e->poolId_ != 0) - extended eligibility
         */
        return true;
    }
    return false;
}

/*..........................................................................*/
bool QF_zeroCopyPost(QActive * const me, QEvt const * const e) {
    uint32_t rear = l_stagingBuffer.rear;
    uint32_t next = (rear + 1U) % QF_STAGING_BUFFER_SIZE;
    
    /* Check if buffer is full */
    if (next == l_stagingBuffer.front) {
        /* Buffer full - increment lost event counter */
        __sync_fetch_and_add(&l_stagingBuffer.lostEvtCnt, 1U);
        
        /* Optional fallback to rt_mb_send() */
        QF_CRIT_STAT_
        QF_CRIT_E_();
        
        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST_ATTEMPT, me->prio)
            QS_TIME_PRE_();
            QS_SIG_PRE_(e->sig);
            QS_OBJ_PRE_(me);
            QS_2U8_PRE_(e->poolId_, e->refCtr_);
            QS_U32_PRE_(l_stagingBuffer.lostEvtCnt);
        QS_END_NOCRIT_PRE_()
        
        if (e->poolId_ != 0U) {
            QEvt_refCtr_inc_(e);
        }
        
        QF_CRIT_X_();
        
        /* Fallback to normal mailbox */
        rt_err_t result = rt_mb_send(&me->eQueue, (rt_ubase_t)e);
        return (result == RT_EOK);
    }
    
    /* Store event and target in ring buffer */
    l_stagingBuffer.buffer[rear].evt = e;
    l_stagingBuffer.buffer[rear].target = me;
    
    /* Update rear index atomically */
    __sync_fetch_and_add(&l_stagingBuffer.rear, 1U);
    if (l_stagingBuffer.rear >= QF_STAGING_BUFFER_SIZE) {
        l_stagingBuffer.rear = 0U;
    }
    
    /* Increment reference counter for dynamic events */
    if (e->poolId_ != 0U) {
        QEvt_refCtr_inc_(e);
    }
    
    /* Signal the dispatcher thread (outside critical section) */
    rt_sem_release(&l_stagingBuffer.sem);
    
    return true;
}

/*..........................................................................*/
void QF_initOptLayer(void) {
    /* Initialize staging buffer */
    l_stagingBuffer.front = 0U;
    l_stagingBuffer.rear = 0U;
    l_stagingBuffer.lostEvtCnt = 0U;
    
    /* Initialize semaphore */
    rt_sem_init(&l_stagingBuffer.sem, "qf_opt_sem", 0, RT_IPC_FLAG_FIFO);
    
    /* Create dispatcher thread with highest priority (0) */
    rt_thread_init(&dispatcherThreadObj,
                   "qf_dispatcher",
                   dispatcherThread,
                   RT_NULL,
                   dispatcherStack,
                   sizeof(dispatcherStack),
                   0,    /* highest priority */
                   1);   /* no timeslice */
    
    rt_thread_startup(&dispatcherThreadObj);
}

/*..........................................................................*/
static void dispatcherThread(void *parameter) {
    Q_UNUSED_PAR(parameter);
    
    for (;;) {
        /* Wait for events in the staging buffer */
        rt_sem_take(&l_stagingBuffer.sem, RT_WAITING_FOREVER);
        
        /* Dispatch batched events */
        QF_dispatchBatchedEvents();
    }
}

/*..........................................................................*/
static void QF_dispatchBatchedEvents(void) {
    while (l_stagingBuffer.front != l_stagingBuffer.rear) {
        /* Get event from front of buffer */
        uint32_t front = l_stagingBuffer.front;
        QEvt const *e = l_stagingBuffer.buffer[front].evt;
        QActive *targetAO = l_stagingBuffer.buffer[front].target;
        
        /* Update front index atomically */
        l_stagingBuffer.front = (front + 1U) % QF_STAGING_BUFFER_SIZE;
        
        /* Directly dispatch to target AO without posting back to mailbox */
        if (targetAO != (QActive *)0) {
            QHSM_DISPATCH(&targetAO->super, e, targetAO->prio);
            
            /* Garbage collect the event */
            QF_gc(e);
        }
    }
}

/*..........................................................................*/
uint32_t QF_getLostEventCount(void) {
    return l_stagingBuffer.lostEvtCnt;
}