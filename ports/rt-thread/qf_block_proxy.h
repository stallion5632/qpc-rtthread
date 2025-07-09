/*============================================================================
* QP/C Real-Time Embedded Framework (RTEF)
* Copyright (C) 2024 Quantum Leaps, LLC. All rights reserved.
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
* @date Last updated on: 2024-07-09
* @version Last updated for: @ref qpc_7_3_0
*
* @file
* @brief QF Blocking Proxy Pattern for RT-Thread
*/
#ifndef QF_BLOCK_PROXY_H
#define QF_BLOCK_PROXY_H

#ifdef QF_BLOCKING_PROXY_ENABLE

/*==========================================================================*/
/* Blocking Proxy Signal Definitions */
/*==========================================================================*/

enum BlockingProxySignals {
    BLOCK_REQ_SIG = Q_USER_SIG + 100,  /* Signal for blocking request */
    BLOCK_DONE_SIG                     /* Signal for blocking completion */
};

/*==========================================================================*/
/* Blocking Proxy Event Structures */
/*==========================================================================*/

/*! Event structure for blocking request */
typedef struct {
    QEvt super;                    /* Inherit from QEvt */
    QActive *requestor;            /* AO that made the blocking request */
    struct rt_semaphore *sem;      /* Semaphore to wait on */
    QSignal doneSig;               /* Signal to send when done */
    rt_int32_t timeout;            /* Timeout value for rt_sem_take */
} BlockReqEvt;

/*! Event structure for blocking completion */
typedef struct {
    QEvt super;                    /* Inherit from QEvt */
    rt_err_t result;               /* Result of the blocking operation */
} BlockDoneEvt;

/*==========================================================================*/
/* Blocking Proxy Public Interface */
/*==========================================================================*/

/*! Initialize the blocking proxy */
void QF_proxyInit(void);

/*! Block on a semaphore using the proxy pattern */
void QActive_blockOnSem(QActive * const me, 
                       struct rt_semaphore * const sem,
                       QSignal const doneSig,
                       rt_int32_t const timeout);

#endif /* QF_BLOCKING_PROXY_ENABLE */

#endif /* QF_BLOCK_PROXY_H */