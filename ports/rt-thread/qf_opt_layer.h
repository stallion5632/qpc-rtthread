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
* @brief QF/C optimization layer for RT-Thread - header file
*/
#ifndef QF_OPT_LAYER_H_
#define QF_OPT_LAYER_H_

/*! Configuration macro for staging buffer size */
#ifndef QF_STAGING_BUFFER_SIZE
#define QF_STAGING_BUFFER_SIZE 32U
#endif

/*! Macro to get thread pointer from QActive */
#define QF_THREAD_PTR(me_) ((rt_thread_t)&(me_)->thread)

/* Public API */
/*! Initialize the optimization layer */
void QF_initOptLayer(void);


/*! ISR wrapper for fast-path dispatch */
bool QF_postFromISR(QActive * const me, QEvt const * const e);

/*! Get lost event count */
uint32_t QF_getLostEventCount(void);

/*! Enable optimization layer */
void QF_enableOptLayer(void);

/*! Disable optimization layer */
void QF_disableOptLayer(void);

#endif /* QF_OPT_LAYER_H_ */