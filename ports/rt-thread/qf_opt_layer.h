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
* @brief QF/C optimization layer header for RT-Thread
*/
#ifndef QF_OPT_LAYER_H_
#define QF_OPT_LAYER_H_

#include "qf_port.h"

/* Configuration constants */
#ifndef QF_STAGING_BUFFER_SIZE
#define QF_STAGING_BUFFER_SIZE 32U
#endif

#ifndef QF_MAX_RETRY_COUNT
#define QF_MAX_RETRY_COUNT 3U
#endif

/* Extended event structure with metadata */
typedef struct {
    QEvt super;                 /* Base event structure */
    uint32_t timestamp;         /* Event arrival timestamp */
    uint8_t priority;           /* Event priority (0-255) */
    uint8_t flags;              /* Event flags (bit0=mergeable, bit1=critical, etc.) */
    uint8_t retryCount;         /* Retry count for backpressure */
    uint8_t reserved;           /* Reserved for future use */
} QEvtEx;

/* Event flags */
#define QF_EVT_FLAG_MERGEABLE   0x01U
#define QF_EVT_FLAG_CRITICAL    0x02U
#define QF_EVT_FLAG_NO_DROP     0x04U

/* Priority levels for staging buffers */
typedef enum {
    QF_PRIO_HIGH = 0U,
    QF_PRIO_NORMAL = 1U,
    QF_PRIO_LOW = 2U,
    QF_PRIO_LEVELS = 3U
} QF_PrioLevel;

/* Dispatcher strategy interface */
typedef struct {
    bool (*shouldMerge)(QEvt const *prev, QEvt const *next);
    int (*comparePriority)(QEvt const *a, QEvt const *b);
    bool (*shouldDrop)(QEvt const *evt, QActive const *targetAO);
    QF_PrioLevel (*getPrioLevel)(QEvt const *evt);
} QF_DispatcherStrategy;

/* Runtime metrics structure */
typedef struct {
    uint32_t dispatchCycles;        /* Number of dispatch cycles */
    uint32_t eventsProcessed;       /* Total events processed */
    uint32_t eventsMerged;          /* Events merged */
    uint32_t eventsDropped;         /* Events dropped */
    uint32_t eventsRetried;         /* Events retried */
    uint32_t maxBatchSize;          /* Maximum batch size processed */
    uint32_t avgBatchSize;          /* Average batch size */
    uint32_t maxQueueDepth;         /* Maximum queue depth observed */
    uint32_t postFailures;          /* Failed post attempts */
    uint32_t stagingOverflows[QF_PRIO_LEVELS];  /* Overflows per priority level */
} QF_DispatcherMetrics;

/* Function prototypes */
void QF_initOptLayer(void);
void QF_setDispatcherStrategy(QF_DispatcherStrategy const *strategy);
bool QF_postFromISR(QActive * const me, QEvt const * const e);
uint32_t QF_getLostEventCount(void);
void QF_enableOptLayer(void);
void QF_disableOptLayer(void);
QF_DispatcherMetrics const *QF_getDispatcherMetrics(void);
void QF_resetDispatcherMetrics(void);

/* Extended event functions */
QEvtEx *QF_newEvtEx(enum_t const sig, uint16_t const evtSize, uint8_t priority, uint8_t flags);
uint32_t QF_getTimestamp(void);

/* Default strategy implementations */
extern QF_DispatcherStrategy const QF_defaultStrategy;
extern QF_DispatcherStrategy const QF_highPerfStrategy;

#endif /* QF_OPT_LAYER_H_ */