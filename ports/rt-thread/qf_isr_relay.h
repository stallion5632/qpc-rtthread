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
* @date Last updated on: 2025-07-22
* @version Last updated for: @ref qpc_7_3_0
*
* @file
* @brief ISR-safe event publishing for RT-Thread port
*/
#ifndef QF_ISR_RELAY_H
#define QF_ISR_RELAY_H

#include <stdint.h>  /* Exact-width types. WG14/N843 C99 Standard */
#include <stdbool.h> /* Boolean type.      WG14/N843 C99 Standard */
#include "qep.h"
#include "qf_port.h"
#include <rtthread.h>

/* ISR event relay configuration */
#ifndef QF_ISR_MAIN_BUFFER_SIZE
#define QF_ISR_MAIN_BUFFER_SIZE    32U
#endif

#ifndef QF_ISR_OVERFLOW_BUFFER_SIZE
#define QF_ISR_OVERFLOW_BUFFER_SIZE 16U
#endif

#ifndef QF_ISR_RELAY_THREAD_PRIO
#define QF_ISR_RELAY_THREAD_PRIO   5U   /* Higher than typical AOs */
#endif

#ifndef QF_ISR_RELAY_STACK_SIZE
#define QF_ISR_RELAY_STACK_SIZE    2048U
#endif

/* Event descriptor for ISR buffering */
typedef struct QF_ISREvent_tag {
    QSignal sig;
    rt_uint8_t poolId;
    rt_uint16_t param;
    rt_tick_t timestamp;
} QF_ISREvent;

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
typedef QF_ISREvent qf_isrrelay_typedef_dummy_t;
#pragma GCC diagnostic pop
#endif
/* Statistics structure */
typedef struct {
    rt_uint32_t events_processed;
    rt_uint32_t events_lost;
    rt_uint32_t max_batch_size;
    rt_tick_t max_process_time;
    rt_uint32_t relay_wakeups;
} QF_ISRStats;

/* Public API */
void QF_isrRelayInit(void);
void QF_isrRelayStart(void);
void QF_publishFromISR(QSignal sig, rt_uint8_t poolId, rt_uint16_t param);
void QF_isrRelayPrintStats(void);
QF_ISRStats const *QF_isrRelayGetStats(void);

#if 0
/* Convenience macro for simple signal publishing from ISR */
#define QF_PUBLISH_FROM_ISR(sig_) \
    QF_publishFromISR((sig_), 0U, 0U)

/* Convenience macro for parameterized event publishing from ISR */
#define QF_PUBLISH_PARAM_FROM_ISR(sig_, param_) \
    QF_publishFromISR((sig_), 0U, (param_))
#endif /* 0 */
#endif /* QF_ISR_RELAY_H */
