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
* @date Last updated on: 2025-07-22
* @version Last updated for: @ref qpc_7_3_0
*
* @file
* @brief QF/C RT-Thread Memory Pool Adapter for event pools.
*
* This file provides an adapter to use RT-Thread's native memory pools
* (`rt_mempool`) as the backend for QP/C's dynamic event pools.
*/
#ifndef QF_RTMPOOL_H
#define QF_RTMPOOL_H


#include <stdint.h>


#include <stdbool.h>    /* For bool type */
#include <rtthread.h>   /* RT-Thread API */
#include <rtdef.h>      /* For MISRA C fixed-width types */
#include <stdbool.h>    /* For bool type */
#include "qep_port.h"  /* QEP port, for QEvt, enum_t, etc. */
#include "qf_port.h"   /* QF port */

/*! @brief RT-Thread memory pool adapter for QP/C event pools.
*
* @details
* This structure adapts the RT-Thread memory pool (`rt_mempool`) to be used
* as an event pool in QP/C. It wraps `rt_mempool` and maintains additional
* information required by the QP/C framework, such as pool metadata and
* usage statistics.
*/
typedef struct {
    struct rt_mempool rtpool;    /*!< RT-Thread memory pool object */
    void    *storage;            /*!< Pointer to the memory storage buffer */
    char const *name;           /*!< Pointer to the pool name (for debugging) */
    uint16_t block_size;         /*!< Size of each memory block in bytes */
    uint16_t block_count;        /*!< Total number of blocks in the pool */
#ifdef QF_RTMPOOL_DEBUG
    uint16_t used_count;         /*!< Currently allocated blocks (for debug) */
    uint16_t max_used;           /*!< Maximum allocated blocks (for debug) */
#endif
    uint16_t nMin;               /*!< Minimum number of free blocks (low-water mark) */
    uint16_t margin;             /*!< Per-pool margin (minimum reserved free blocks) */
} QF_RTMemPool;

/*!
* @brief Initializes an RT-Thread memory pool adapter.
*
* @param[in,out] me      Pointer to a QF_RTMemPool struct to initialize.
* @param[in]     name    Name of the memory pool (for debugging).
* @param[in]     storage Pointer to the memory storage for the pool.
* @param[in]     n       Number of blocks in the pool.
* @param[in]     size    Size of each block in bytes.
*
* @returns RT_EOK on success, or an error code on failure.
*
* @note The provided storage buffer must be properly aligned. Use the
*       `ALIGN(RT_ALIGN_SIZE)` macro for static array definitions.
*/
rt_err_t QF_RTMemPool_init(QF_RTMemPool *const me, char const *name,
                          void *storage, rt_size_t n, rt_size_t size, uint16_t margin);

/*!
* @brief Allocates a block from the RT-Thread memory pool.
*
* @param[in,out] me      Pointer to the QF_RTMemPool instance.
* @param[in]     timeout Timeout for allocation (e.g., RT_WAITING_NO).
*
* @returns Pointer to the allocated block, or NULL if allocation failed.
*/
void *QF_RTMemPool_alloc(QF_RTMemPool *const me, rt_int32_t timeout);

/*!
* @brief Frees a previously allocated block back to the RT-Thread memory pool.
*
* @param[in,out] me      Pointer to the QF_RTMemPool instance.
* @param[in]     block   Pointer to the block to free.
*
* @returns RT_EOK on success.
*/
rt_err_t QF_RTMemPool_free(QF_RTMemPool *const me, void *block);

/*!
* @brief Gets the number of free blocks in the memory pool.
*
* @param[in] me Pointer to the QF_RTMemPool instance.
*
* @returns Number of free blocks.
*/
uint16_t QF_RTMemPool_getFreeCount(QF_RTMemPool const *const me);

#ifdef QF_RTMPOOL_DEBUG
/*!
* @brief Gets the number of used blocks in the memory pool.
*
* @param[in] me Pointer to the QF_RTMemPool instance.
*
* @returns Number of currently used blocks.
*/
uint16_t QF_RTMemPool_getUsedCount(QF_RTMemPool const *const me);

/*!
* @brief Gets the maximum number of blocks that have been simultaneously
*        allocated (high-water mark).
*
* @param[in] me Pointer to the QF_RTMemPool instance.
*
* @returns Maximum number of blocks used since initialization.
*/
uint16_t QF_RTMemPool_getMaxUsed(QF_RTMemPool const *const me);

/*!
* @brief Prints debug statistics for the memory pool.
*
* @param[in] me Pointer to the QF_RTMemPool instance.
*/
void QF_RTMemPool_printStats(QF_RTMemPool const *const me);
#endif /* QF_RTMPOOL_DEBUG */

/*================== Extended RT-Thread Memory Pool Manager =================*/
#if defined(QF_RTMPOOL_EXT) && (QF_RTMPOOL_EXT == 1)

/*!
* @brief Initializes the RT-Thread memory pool manager.
* @details This function must be called before any memory pools are registered
*          or used. It is typically called once during system startup.
*/
void QF_RTMemPoolMgr_init(void);

/*!
* @brief Registers an RT-Thread memory pool with the manager.
*
* @param[in] rtPool Pointer to the `QF_RTMemPool` instance to register.
*
* @returns The assigned pool ID, which is used internally by the framework.
*/
rt_uint8_t QF_RTMemPoolMgr_registerPool(QF_RTMemPool * const rtPool);

/*!
* @brief Allocates a new event with extended features like margin and fallback.
*
* @param[in] evtSize The size of the event to allocate.
* @param[in] margin  The minimum number of free blocks to leave in the ideal
*                    pool. If the allocation would violate the margin, the
*                    manager will attempt to fall back to a larger pool.
* @param[in] sig     The signal for the new event.
*
* @returns A pointer to the allocated event, or NULL if allocation fails.
*/
QEvt *QF_newX_RT(rt_uint16_t evtSize,
                 rt_uint16_t margin,
                 enum_t      sig);

/*!
* @brief Recycles a dynamic event back to its original memory pool.
* @details This function is the garbage collector for the RT-Thread pool
*          adapter. It is called automatically by the QP/C framework when
*          an event is no longer in use.
*
* @param[in] e Pointer to the event to be recycled.
*/
void QF_gc_RT(QEvt const * const e);

/*!
* @brief Retrieves statistics from the memory pool manager.
*
* @param[out] nPools    Pointer to store the number of registered pools.
* @param[out] allocs    Pointer to store the total number of successful allocations.
* @param[out] fails     Pointer to store the total number of failed allocations.
* @param[out] fallbacks Pointer to store the total number of fallback allocations.
*/
void QF_RTMemPoolMgr_getStats(rt_uint8_t  * nPools,
                             rt_uint32_t * allocs,
                             rt_uint32_t * fails,
                             rt_uint32_t * fallbacks);

/*!
* @brief Prints statistics for all registered memory pools.
*/
void QF_RTMemPoolMgr_printStats(void);

#endif /* defined(QF_RTMPOOL_EXT) && (QF_RTMPOOL_EXT == 1) */

#endif /* QF_RTMPOOL_H */
