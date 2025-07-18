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
* @date Last updated on: 2023-01-07
* @version Last updated for: @ref qpc_7_2_0
*
* @file
* @brief QEP/C port, generic C99 compiler
*/
#ifndef QEP_PORT_H
#define QEP_PORT_H

#include <stdint.h>  /* Exact-width types. WG14/N843 C99 Standard */
#include <stdbool.h> /* Boolean type.      WG14/N843 C99 Standard */

/* Platform-specific QEvt extension */
#ifdef Q_EVT_TARGET
/*! QEvt extension for RT-Thread optimization layer */
#define Q_EVT_EXTENSION \
    void *target; /*!< Target AO pointer for fast-path dispatch */
#endif

#include "qep.h"     /* QEP platform-independent public interface */

#endif /* QEP_PORT_H */
