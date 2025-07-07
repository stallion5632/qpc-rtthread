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
* @brief QF/C optimization layer interface for RT-Thread port
*/
#ifndef QF_OPT_LAYER_H
#define QF_OPT_LAYER_H

#include "qpc.h"        /* QP/C public interface */
#include <rtthread.h>   /* RT-Thread types */

/* Configuration options */
#define QF_FAST_PATH_THRESHOLD   3U  /* Priority threshold for fast path */
#define QF_STAGING_BUFFER_SIZE   32U /* Size of interrupt staging buffer */

/* Staging buffer for batched interrupt events */
typedef struct {
    rt_base_t  front;
    rt_base_t  rear;
    QEvt const *events[QF_STAGING_BUFFER_SIZE];
    struct rt_semaphore notify_sem;
} QF_StagingBuffer;

// 注意：QEvt 结构体需包含 target 指针，否则实现需调整
// 建议在 QEvt 定义中添加 void *target; 字段，或在此处说明

/* Fast path eligibility check */
bool QF_isEligibleForFastPath(QActive const * const me, 
                             QEvt const * const e);

/* Zero-copy event queue operations */
void QF_zeroCopyPost(QActive * const me, QEvt const * const e,
                     void const * const sender);

/* Batch processing operations */
void QF_stagingBufferInit(void);
void QF_stagingBufferPost(QEvt const * const e);
void QF_dispatchBatchedEvents(void);

#endif /* QF_OPT_LAYER_H */