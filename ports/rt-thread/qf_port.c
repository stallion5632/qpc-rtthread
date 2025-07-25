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
 * @date Last updated on: 2023-04-12
 * @version Last updated for: @ref qpc_7_2_2
 *
 * @file
 * @brief QF/C, port to RT-Thread
 */
#define QP_IMPL      /* this is QP implementation */
#include "qf_port.h" /* QF port */
#include "qf_pkg.h"
#include "qassert.h"
#include "qf_opt_layer.h" /* QF optimization layer */
#ifdef Q_SPY              /* QS software tracing enabled? */
#include "qs_port.h"      /* QS port */
#include "qs_pkg.h"       /* QS package-scope internal interface */
#else
#include "qs_dummy.h" /* disable the QS software tracing */
#endif                /* Q_SPY */

Q_DEFINE_THIS_MODULE("qf_port")


/**
 * @brief Initialize the QF framework (RT-Thread port)
 */
void QF_init(void)
{
}

/**
 * @brief Run the QF framework, start up and initialize optimization layer
 * @return 0 on success
 */
int_t QF_run(void)
{
    QS_CRIT_STAT_

    QF_onStartup(); /* QF callback to configure and start interrupts */

    /* Initialize the optimization layer */
    QF_initOptLayer();

    /* produce the QS_QF_RUN trace record */
    QS_BEGIN_PRE_(QS_QF_RUN, 0U)
    QS_END_PRE_()

    return 0; /* return success */
}

/**
 * @brief Stop the QF framework and perform cleanup
 */
void QF_stop(void)
{
    QF_onCleanup(); /* cleanup callback */
}

/**
 * @brief RT-Thread AO thread entry function, event loop dispatcher
 * @param parameter Pointer to QActive object
 */
static void thread_function(void *parameter)
{ /* RT-Thread signature */
    QActive *act = (QActive *)parameter;
#ifdef Q_RT_DEBUG
    rt_kprintf("[thread_function] AO thread started: %p, name: %s, prio: %d, stat: %d\n",
               act, rt_thread_self()->name, rt_thread_self()->current_priority, rt_thread_self()->stat);
#endif /* Q_RT_DEBUG */
    /* event-loop */
    for (;;)
    { /* for-ever */
        QEvt const *e = QActive_get_(act);
        QHSM_DISPATCH(&act->super, e, act->prio);
        QF_gc(e); /* check if the event is garbage, and collect it if so */
    }
}

/**
 * @brief Start an active object (AO) thread in RT-Thread
 * @param me Pointer to QActive object
 * @param prioSpec Priority specification
 * @param qSto Event queue storage
 * @param qLen Event queue length
 * @param stkSto Stack storage
 * @param stkSize Stack size
 * @param par Initialization parameter
 */
void QActive_start_(QActive *const me, QPrioSpec const prioSpec,
                    QEvt const **const qSto, uint_fast16_t const qLen,
                    void *const stkSto, uint_fast16_t const stkSize,
                    void const *const par)
{
    /* allege that the RT-Thread queue is created successfully */
    Q_ALLEGE_ID(210,
                rt_mb_init(&me->eQueue,
                           me->thread.name,
                           (void *)qSto,
                           (qLen),
                           RT_IPC_FLAG_FIFO) == RT_EOK);

    me->prio = (uint8_t)(prioSpec & 0xFFU); /* QF-priority */
    me->pthre = (uint8_t)(prioSpec >> 8U);  /* preemption-threshold */
    QActive_register_(me);                  /* register this AO */
    QHSM_INIT(&me->super, par, me->prio);   /* initial tran. (virtual) */
    QS_FLUSH();                             /* flush the trace buffer to the host */
#ifdef Q_RT_DEBUG
    rt_kprintf("[QActive_start_] AO: %p, name: %s, registered, QHSM: %p\n",
               me, me->thread.name ? me->thread.name : "NULL", me);
#endif /* Q_RT_DEBUG */
    Q_ALLEGE_ID(220,
                rt_thread_init(
                    &me->thread,              /* RT-Thread thread control block */
                    me->thread.name,          /* unique thread name */
                    &thread_function,         /* thread function */
                    me,                       /* thread parameter */
                    stkSto,                   /* stack start */
                    stkSize,                  /* stack size in bytes */
                    QF_MAX_ACTIVE - me->prio, /* RT-Thread priority */
                    5) == RT_EOK);
    rt_err_t startup_result = rt_thread_startup(&me->thread);
#ifdef Q_RT_DEBUG
    rt_kprintf("[QActive_start_] Thread startup result: %d, state: %d\n", startup_result, me->thread.stat);
#else
    if (startup_result != RT_EOK)
    {
        Q_ERROR_ID(521); /* thread startup failed */
    }
#endif /* Q_RT_DEBUG */
}

/**
 * @brief Set attributes for QActive object before start (e.g. thread name)
 * @param me Pointer to QActive object
 * @param attr1 Attribute type
 * @param attr2 Attribute value
 */
void QActive_setAttr(QActive *const me, uint32_t attr1, void const *attr2)
{
    /* this function must be called before QACTIVE_START(),
     * For RT-Thread, thread.name is a char array, not a pointer.
     * if first byte is 0, thread name has not been set.
     */
    Q_REQUIRE_ID(300, me->thread.name[0] == '\0');
    switch (attr1)
    {
    case THREAD_NAME_ATTR:
        rt_memset(me->thread.name, 0x00, RT_NAME_MAX);
        rt_strncpy(me->thread.name, (char *)attr2, RT_NAME_MAX - 1);
        break;
        /* ... */
    }
}

/**
 * @brief Post an event to the AO's event queue (FIFO)
 * @param me Pointer to QActive object
 * @param e Event pointer
 * @param margin Minimum free slots required
 * @param sender Sender object pointer
 * @return true if posted, false if not
 */
bool QActive_post_(QActive *const me, QEvt const *const e,
                   uint_fast16_t const margin, void const *const sender)
{
    uint_fast16_t nFree;
    bool status;
    QF_CRIT_STAT_

    QF_CRIT_E_();
    nFree = (uint_fast16_t)(me->eQueue.size - me->eQueue.entry);

    if (margin == QF_NO_MARGIN)
    {
        if (nFree > 0U)
        {
            status = true; /* can post */
        }
        else
        {
            status = false; /* cannot post */
#ifdef Q_RT_DEBUG
            rt_kprintf("[QPC][ERROR] AO event queue full, event drop! AO=%p, sig=%u\n", me, e->sig);
#if (QF_MAX_EPOOL > 0U)
            QF_gc(e);
#endif
            QF_CRIT_X_();
            return false;
#else
            Q_ERROR_ID(510); /* must be able to post the event */
#endif /* Q_RT_DEBUG */
        }
    }
    else if (nFree > (QEQueueCtr)margin)
    {
        status = true; /* can post */
    }
    else
    {
        status = false; /* cannot post */
    }

    if (status)
    {   /* can post the event? */
        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST, me->prio)
        QS_TIME_PRE_();                      /* timestamp */
        QS_OBJ_PRE_(sender);                 /* the sender object */
        QS_SIG_PRE_(e->sig);                 /* the signal of the event */
        QS_OBJ_PRE_(me);                     /* this active object (recipient) */
        QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
        QS_EQC_PRE_(nFree);                  /* # free entries available */
        QS_EQC_PRE_(0U);                     /* min # free entries (unknown) */
        QS_END_NOCRIT_PRE_()

        if (e->poolId_ != 0U)
        {                        /* is it a pool event? */
            QEvt_refCtr_inc_(e); /* increment the reference counter */
        }

        QF_CRIT_X_();

        /* posting to the RT-Thread message queue must succeed */
        Q_ALLEGE_ID(520,
                    rt_mb_send(&me->eQueue, (rt_ubase_t)e) == RT_EOK);
    }
    else
    {
        QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST_ATTEMPT, me->prio)
        QS_TIME_PRE_();                      /* timestamp */
        QS_OBJ_PRE_(sender);                 /* the sender object */
        QS_SIG_PRE_(e->sig);                 /* the signal of the event */
        QS_OBJ_PRE_(me);                     /* this active object (recipient) */
        QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
        QS_EQC_PRE_(nFree);                  /* # free entries available */
        QS_EQC_PRE_(0U);                     /* min # free entries (unknown) */
        QS_END_NOCRIT_PRE_()

        QF_CRIT_X_();

#if (QF_MAX_EPOOL > 0U)
        QF_gc(e); /* recycle the event to avoid a leak */
#endif
    }

    return status;
}

/**
 * @brief Post an event to the AO's event queue (LIFO)
 * @param me Pointer to QActive object
 * @param e Event pointer
 */
void QActive_postLIFO_(QActive *const me, QEvt const *const e)
{
    QF_CRIT_STAT_
    QF_CRIT_E_();

    QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST_LIFO, me->prio)
    QS_TIME_PRE_();                                  /* timestamp */
    QS_SIG_PRE_(e->sig);                             /* the signal of this event */
    QS_OBJ_PRE_(me);                                 /* this active object */
    QS_2U8_PRE_(e->poolId_, e->refCtr_);             /* pool Id & ref Count */
    QS_EQC_PRE_(me->eQueue.size - me->eQueue.entry); /* # free */
    QS_EQC_PRE_(0U);                                 /* min # free entries (unknown) */
    QS_END_NOCRIT_PRE_()

    if (e->poolId_ != 0U)
    {                        /* is it a pool event? */
        QEvt_refCtr_inc_(e); /* increment the reference counter */
    }

    QF_CRIT_X_();

    /* LIFO posting must succeed */
    Q_ALLEGE_ID(610,
                rt_mb_urgent(&me->eQueue, (rt_ubase_t)e) == RT_EOK);
}

/**
 * @brief Get the next event from the AO's event queue
 * @param me Pointer to QActive object
 * @return Pointer to event
 */
QEvt const *QActive_get_(QActive *const me)
{
    QEvt const *e;
    QS_CRIT_STAT_

    Q_ALLEGE_ID(710,
                rt_mb_recv(&me->eQueue, (rt_ubase_t *)&e, RT_WAITING_FOREVER) == RT_EOK);

    QS_BEGIN_PRE_(QS_QF_ACTIVE_GET, me->prio)
    QS_TIME_PRE_();                                  /* timestamp */
    QS_SIG_PRE_(e->sig);                             /* the signal of this event */
    QS_OBJ_PRE_(me);                                 /* this active object */
    QS_2U8_PRE_(e->poolId_, e->refCtr_);             /* pool Id & ref Count */
    QS_EQC_PRE_(me->eQueue.size - me->eQueue.entry); /* # free */
    QS_END_PRE_()

    return e;
}
