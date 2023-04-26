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

enum RT_Thread_ThreadAttrs {
    THREAD_NAME_ATTR
};

#include <rtthread.h>   /* RT-Thread API */

#include "qep_port.h"  /* QEP port */
#include "qequeue.h"   /* used for event deferral */
#include "qf.h"        /* QF platform-independent public interface */
#ifdef Q_SPY
#include "qmpool.h"    /* needed only for QS-RX */
#endif

/*****************************************************************************
* interface used only inside QF, but not in applications
*/
#ifdef QP_IMPL

    #define QF_SCHED_STAT_
    #define QF_SCHED_LOCK_(prio_)   rt_enter_critical()
    #define QF_SCHED_UNLOCK_()      rt_exit_critical()

    /* TreadX block pool operations... */
    #define QF_EPOOL_TYPE_              struct rt_mempool
    #define QF_EPOOL_INIT_(pool_, poolSto_, poolSize_, evtSize_)            \
        Q_ALLEGE(rt_mp_init(&(pool_), (char *)"QP",                         \
                 (poolSto_), (poolSize_), (evtSize_)) == RT_EOK)

    #define QF_EPOOL_EVENT_SIZE_(pool_)                                     \
        ((uint_fast16_t)(pool_).block_size)

    #define QF_EPOOL_GET_(pool_, e_, margin_, qs_id_) do {                  \
        if ((pool_).block_free_count > (margin_)) {                         \
            e_ = rt_mp_alloc(&(pool_), RT_WAITING_NO);                      \
        }                                                                   \
        else {                                                              \
            (e_) = (QEvt *)0;                                               \
        }                                                                   \
    } while (false)

    #define QF_EPOOL_PUT_(dummy, e_, qs_id_)                                \
        rt_mp_free((void *)(e_));

#endif /* ifdef QP_IMPL */

#endif /* QF_PORT_H */

