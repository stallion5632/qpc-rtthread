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
* @date Last updated on: 2025-07-21
* @version Last updated for: @ref qpc_7_3_0
*
* @file
* @brief QF/C RT-Thread Memory Pool Adapter Configuration
*
* @note
* This file should be included in your project's board configuration file
* or similar location to configure the RT-Thread memory pool adapter for QPC.
*/
#ifndef QF_RTMPOOL_CONFIG_H
#define QF_RTMPOOL_CONFIG_H

/**
 * @brief Enable RT-Thread memory pool integration for QF event pools
 *
 * Set to 1 to use RT-Thread's rt_mempool as the backend for QP/C event pools.
 * Set to 0 to use the native QP/C memory pool implementation.
 */
#undef QF_ENABLE_RT_MEMPOOL
#define QF_ENABLE_RT_MEMPOOL            1

/**
 * @brief Enable extended RT-Thread memory pool features
 *
 * Set to 1 to enable advanced features such as:
 * - Margin support for preventing pool exhaustion
 * - Fallback allocation to larger pools when a smaller pool cannot satisfy margin
 * - Memory pool manager for tracking multiple pools
 */
#define QF_RTMPOOL_EXT                 1

/**
 * @brief Enable debug features for RT-Thread memory pool adapter
 *
 * Set to 1 to enable additional debug features such as:
 * - Usage tracking (current and peak memory allocation)
 * - Debug output during allocation/free operations
 * - Memory statistics functions
 */
#define QF_RTMPOOL_DEBUG                1

#endif /* QF_RTMPOOL_CONFIG_H */
