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
* @date Last updated on: 2022-06-12
* @version Last updated for: @ref qpc_7_0_1
*
* @file
* @brief QF/C, port to RT-Thread
*/
#ifndef QF_PORT_H
#define QF_PORT_H

/* RT-Thread event queue and thread types */
#define QF_EQUEUE_TYPE      struct rt_mailbox
#define QF_THREAD_TYPE      struct rt_thread

/* The maximum number of active objects in the application, see NOTE2 */
#define QF_MAX_ACTIVE       (RT_THREAD_PRIORITY_MAX)

/* QF critical section for RT-Thread, see NOTE3 */
#define QF_CRIT_ENTRY(stat_)  (rt_enter_critical())
#define QF_CRIT_EXIT(stat_)   (rt_exit_critical())

/* RT-Thread memory pool integration for QF event pools */
#ifndef QF_ENABLE_RT_MEMPOOL
#define QF_ENABLE_RT_MEMPOOL 0
#endif

#if QF_ENABLE_RT_MEMPOOL
/* Debug support for RT-Thread memory pool adapter */
#ifndef QF_RTMPOOL_DEBUG
#define QF_RTMPOOL_DEBUG 0
#endif
#endif

/* QF optimization layer configuration */
#ifndef QF_STAGING_BUFFER_SIZE
#define QF_STAGING_BUFFER_SIZE 32U  /*!< Configurable staging buffer size */
#endif

#ifndef QF_DISPATCHER_STACK_SIZE
#define QF_DISPATCHER_STACK_SIZE 2048U  /*!< Configurable dispatcher stack size */
#endif

#ifndef QF_DISPATCHER_PRIORITY
#define QF_DISPATCHER_PRIORITY 0U  /*!< Highest priority for dispatcher */
#endif

enum RT_Thread_ThreadAttrs {
    THREAD_NAME_ATTR
};

#include <rtthread.h>   /* RT-Thread API */

#include "qep_port.h" /* QEP port */
#include "qequeue.h"  /* native QF event queue for deferring events */
#include "qmpool.h"   /* native QF event pool */
#include "qf.h"       /* QF platform-independent public interface */
#include "qf_opt_layer.h" /* QF optimization layer */

/*****************************************************************************
* interface used only inside QF, but not in applications
*/
#ifdef QP_IMPL

    #define QF_SCHED_STAT_
    #define QF_SCHED_LOCK_(prio_)   rt_enter_critical()
    #define QF_SCHED_UNLOCK_()      rt_exit_critical()

#if QF_ENABLE_RT_MEMPOOL
    #include "qf_rtmpool.h"  /* RT-Thread memory pool adapter */

    /* Override the native QF event pool operations with RT-Thread memory pool */
    #define QF_EPOOL_TYPE_            QF_RTMemPool
    #define QF_EPOOL_INIT_(p_, poolSto_, poolSize_, evtSize_) \
        (QF_RTMemPool_init(&(p_), #p_, (poolSto_), ((poolSize_) / (evtSize_)), (evtSize_)))
    #define QF_EPOOL_EVENT_SIZE_(p_)  ((uint_fast16_t)(p_).block_size)
    #define QF_EPOOL_GET_(p_, e_, m_, qs_id_) \
        do { \
            QS_CRIT_STAT_ \
            QS_BEGIN_(QS_QF_MPOOL_GET, (qs_id_)) \
                QS_TIME_(); \
                QS_MPC_((QF_MPOOL_CTR)QF_RTMemPool_getFreeCount(&(p_))); \
                QS_MPC_((QF_MPOOL_CTR)(m_)); \
            QS_END_() \
            (e_) = (QEvt *)QF_RTMemPool_alloc(&(p_), RT_WAITING_NO); \
            if ((e_) != (QEvt *)0) { \
                (e_)->refCtr_ = 0U; \
                QS_BEGIN_(QS_QF_MPOOL_GET_ATTEMPT, (qs_id_)) \
                    QS_TIME_(); \
                    QS_MPC_((QF_MPOOL_CTR)QF_RTMemPool_getFreeCount(&(p_))); \
                    QS_MPC_((QF_MPOOL_CTR)(m_)); \
                QS_END_() \
            } \
        } while (0)
    #define QF_EPOOL_PUT_(p_, e_, qs_id_) \
        do { \
            QS_CRIT_STAT_ \
            QS_BEGIN_(QS_QF_MPOOL_PUT, (qs_id_)) \
                QS_TIME_(); \
                QS_MPC_((QF_MPOOL_CTR)QF_RTMemPool_getFreeCount(&(p_))); \
            QS_END_() \
            QF_RTMemPool_free(&(p_), (e_)); \
        } while (0)
#else
    /* Native QF event pool operations (default) */
    #define QF_EPOOL_TYPE_            QMPool
    #define QF_EPOOL_INIT_(p_, poolSto_, poolSize_, evtSize_) \
        (QMPool_init(&(p_), (poolSto_), (poolSize_), (evtSize_)))
    #define QF_EPOOL_EVENT_SIZE_(p_)  ((uint_fast16_t)(p_).blockSize)
    #define QF_EPOOL_GET_(p_, e_, m_, qs_id_) \
        ((e_) = (QEvt *)QMPool_get(&(p_), (m_), (qs_id_)))
    #define QF_EPOOL_PUT_(p_, e_, qs_id_) \
        (QMPool_put(&(p_), (e_), (qs_id_)))
#endif /* QF_ENABLE_RT_MEMPOOL */

#endif /* ifdef QP_IMPL */

#endif /* QF_PORT_H */
