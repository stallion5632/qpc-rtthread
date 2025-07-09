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
* @brief QF Extensions for RT-Thread
*/
#ifndef QF_EXT_H
#define QF_EXT_H

/* Include core QF port functionality */
#include "qf_port.h"

/* Include blocking proxy pattern functionality */
#ifdef QF_BLOCKING_PROXY_ENABLE
#include "qf_block_proxy.h"
#endif

/* Additional QF extensions for RT-Thread can be added here */

#endif /* QF_EXT_H */