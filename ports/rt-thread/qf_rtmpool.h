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
* @brief QF/C RT-Thread Memory Pool Adapter for event pools
*/
#ifndef QF_RTMPOOL_H
#define QF_RTMPOOL_H

#include <rtthread.h>   /* RT-Thread API */

/*! @brief RT-Thread memory pool adapter for QP/C event pools
*
* This structure adapts the RT-Thread memory pool (rt_mempool) to be used
* as an event pool in QP/C. It wraps rt_mempool and maintains additional
* information needed by QP/C.
*/
typedef struct {
    struct rt_mempool rtpool;    /*!< RT-Thread memory pool object */
    void *storage;               /*!< pointer to the memory storage */
    uint16_t block_size;         /*!< size of memory blocks in bytes */
    uint16_t block_count;        /*!< total number of blocks */
#ifdef QF_RTMPOOL_DEBUG
    uint16_t used_count;         /*!< currently allocated blocks (for debug) */
    uint16_t max_used;           /*!< maximum allocated blocks (for debug) */
#endif
} QF_RTMemPool;

/*! @brief Initialize the RT-Thread memory pool adapter
*
* @param[in,out] me       pointer to a QF_RTMemPool struct to initialize
* @param[in]     name     name of the memory pool (for debugging)
* @param[in]     storage  pointer to the memory storage for the pool
* @param[in]     n        number of blocks
* @param[in]     size     block size in bytes
*
* @returns
* RT_EOK on success, or error code on failure
*
* @note
* The storage must be properly aligned for the memory blocks.
* Use ALIGN(RT_ALIGN_SIZE) macro for static arrays.
*/
rt_err_t QF_RTMemPool_init(QF_RTMemPool *const me, const char *name,
                          void *storage, rt_size_t n, rt_size_t size);

/*! @brief Allocate a block from the RT-Thread memory pool
*
* @param[in,out] me      pointer to the QF_RTMemPool
* @param[in]     timeout timeout for allocation (use RT_WAITING_NO for immediate return)
*
* @returns
* Pointer to the allocated block, or NULL if allocation failed
*/
void *QF_RTMemPool_alloc(QF_RTMemPool *const me, rt_int32_t timeout);

/*! @brief Free a previously allocated block back to the RT-Thread memory pool
*
* @param[in,out] me      pointer to the QF_RTMemPool
* @param[in]     block   pointer to the block to free
*
* @returns
* RT_EOK on success, or error code on failure
*/
rt_err_t QF_RTMemPool_free(QF_RTMemPool *const me, void *block);

/*! @brief Get the number of free blocks in the RT-Thread memory pool
*
* @param[in] me      pointer to the QF_RTMemPool
*
* @returns
* Number of free blocks in the memory pool
*/
uint16_t QF_RTMemPool_getFreeCount(QF_RTMemPool const *const me);

#ifdef QF_RTMPOOL_DEBUG
/*! @brief Get the number of used blocks in the RT-Thread memory pool
*
* @param[in] me      pointer to the QF_RTMemPool
*
* @returns
* Number of currently used blocks in the memory pool
*/
uint16_t QF_RTMemPool_getUsedCount(QF_RTMemPool const *const me);

/*! @brief Get the maximum number of blocks that have been simultaneously allocated
*
* @param[in] me      pointer to the QF_RTMemPool
*
* @returns
* Maximum number of blocks simultaneously used since initialization
*/
uint16_t QF_RTMemPool_getMaxUsed(QF_RTMemPool const *const me);

/*! @brief Print debug information about the memory pool
*
* @param[in] me      pointer to the QF_RTMemPool
*/
void QF_RTMemPool_printStats(QF_RTMemPool const *const me);
#endif /* QF_RTMPOOL_DEBUG */

#endif /* QF_RTMPOOL_H */
