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
* @brief QF Blocking Proxy Pattern implementation for RT-Thread
*/
#include "qf_port.h"      /* QF port */
#include "qf_pkg.h"
#include "qassert.h"

#ifdef QF_BLOCKING_PROXY_ENABLE

#include "qf_block_proxy.h"
#ifdef Q_SPY              /* QS software tracing enabled? */
    #include "qs_port.h"  /* QS port */
    #include "qs_pkg.h"   /* QS package-scope internal interface */
#else
    #include "qs_dummy.h" /* disable the QS software tracing */
#endif /* Q_SPY */

Q_DEFINE_THIS_MODULE("qf_block_proxy")

/*==========================================================================*/
/* Blocking Proxy Configuration */
/*==========================================================================*/

#ifndef QF_PROXY_QUEUE_SIZE
#define QF_PROXY_QUEUE_SIZE    32U  /* Number of blocking requests to queue */
#endif

#ifndef QF_PROXY_STACK_SIZE
#define QF_PROXY_STACK_SIZE    2048U  /* Stack size for proxy thread */
#endif

#ifndef QF_PROXY_PRIORITY
#define QF_PROXY_PRIORITY      1U   /* High priority for proxy thread */
#endif

/*==========================================================================*/
/* Blocking Proxy Internal Data */
/*==========================================================================*/

/*! Proxy thread data structure */
typedef struct {
    struct rt_thread thread;           /* RT-Thread control block */
    struct rt_messagequeue mq;         /* Message queue for requests */
    uint8_t stack[QF_PROXY_STACK_SIZE]; /* Thread stack */
    rt_ubase_t mqBuffer[QF_PROXY_QUEUE_SIZE]; /* Message queue buffer */
    bool initialized;                  /* Initialization flag */
} ProxyData;

static ProxyData l_proxyData;

/*==========================================================================*/
/* Proxy Thread Function */
/*==========================================================================*/

/*! Proxy thread entry point */
static void proxyThread(void *parameter) {
    (void)parameter; /* unused parameter */
    
    while (1) {
        BlockReqEvt *req;
        rt_err_t result;
        
        /* Wait for blocking request */
        result = rt_mq_recv(&l_proxyData.mq, 
                           (void *)&req, 
                           sizeof(BlockReqEvt *),
                           RT_WAITING_FOREVER);
        
        if (result == RT_EOK && req != NULL) {
            /* Perform the blocking operation */
            rt_err_t semResult = rt_sem_take(req->sem, req->timeout);
            
            /* Create completion event */
            BlockDoneEvt *done = Q_NEW(BlockDoneEvt, req->doneSig);
            if (done != NULL) {
                done->result = semResult;
                
                /* Post completion event to requestor */
                QACTIVE_POST(req->requestor, &done->super, &l_proxyData);
            }
            
            /* Clean up request event */
            QF_gc(&req->super);
        }
    }
}

/*==========================================================================*/
/* Public Interface Implementation */
/*==========================================================================*/

/*..........................................................................*/
void QF_proxyInit(void) {
    if (l_proxyData.initialized) {
        return; /* Already initialized */
    }
    
    /* Initialize message queue */
    rt_err_t result = rt_mq_init(&l_proxyData.mq,
                                "qf_proxy_mq",
                                l_proxyData.mqBuffer,
                                sizeof(BlockReqEvt *),
                                sizeof(l_proxyData.mqBuffer),
                                RT_IPC_FLAG_FIFO);
    Q_ASSERT(result == RT_EOK);
    
    /* Initialize and start proxy thread */
    result = rt_thread_init(&l_proxyData.thread,
                           "qf_proxy",
                           proxyThread,
                           RT_NULL,
                           l_proxyData.stack,
                           sizeof(l_proxyData.stack),
                           QF_PROXY_PRIORITY,
                           10); /* timeslice */
    Q_ASSERT(result == RT_EOK);
    
    result = rt_thread_startup(&l_proxyData.thread);
    Q_ASSERT(result == RT_EOK);
    
    l_proxyData.initialized = true;
}

/*..........................................................................*/
void QActive_blockOnSem(QActive * const me, 
                       struct rt_semaphore * const sem,
                       QSignal const doneSig,
                       rt_int32_t const timeout) {
    Q_REQUIRE_ID(300, me != NULL);
    Q_REQUIRE_ID(301, sem != NULL);
    Q_REQUIRE_ID(302, l_proxyData.initialized);
    
    /* Create blocking request event */
    BlockReqEvt *req = Q_NEW(BlockReqEvt, BLOCK_REQ_SIG);
    if (req != NULL) {
        req->requestor = me;
        req->sem = sem;
        req->doneSig = doneSig;
        req->timeout = timeout;
        
        /* Send request to proxy thread */
        rt_err_t result = rt_mq_send(&l_proxyData.mq, 
                                    &req, 
                                    sizeof(BlockReqEvt *));
        if (result != RT_EOK) {
            /* If send failed, clean up the event */
            QF_gc(&req->super);
        }
    }
}

#endif /* QF_BLOCKING_PROXY_ENABLE */