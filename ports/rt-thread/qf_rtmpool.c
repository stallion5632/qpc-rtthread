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
* @brief QF/C RT-Thread Memory Pool Adapter implementation
*/
#define QP_IMPL        /* this is QP implementation */
#include "qf_port.h"   /* QF port */
#include "qf_rtmpool.h" /* RT-Thread memory pool adapter */
#include "qassert.h"   /* QP assertions */

Q_DEFINE_THIS_MODULE("qf_rtmpool")

/*!
* @brief Initialize the RT-Thread memory pool adapter
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
                          void *storage, rt_size_t n, rt_size_t size)
{
    Q_REQUIRE_ID(100, me != NULL);
    Q_REQUIRE_ID(101, storage != NULL);
    Q_REQUIRE_ID(102, n > 0U);
    Q_REQUIRE_ID(103, size >= sizeof(QEvt));

    rt_kprintf("[QPC] Initializing RT-Thread memory pool '%s' at %p: %u blocks x %u bytes\n",
               name, storage, (unsigned)n, (unsigned)size);

    /* Store the meta-information */
    me->storage = storage;
    me->block_size = (uint16_t)size;
    me->block_count = (uint16_t)n;

#ifdef QF_RTMPOOL_DEBUG
    me->used_count = 0U;
    me->max_used = 0U;
#endif

    /* Initialize the RT-Thread memory pool */
    return rt_mp_init(&me->rtpool, name, storage, n, size);
}

/*!
* @brief Allocate a block from the RT-Thread memory pool
*
* @param[in,out] me      pointer to the QF_RTMemPool
* @param[in]     timeout timeout for allocation (use RT_WAITING_NO for immediate return)
*
* @returns
* Pointer to the allocated block, or NULL if allocation failed
*/
void *QF_RTMemPool_alloc(QF_RTMemPool *const me, rt_int32_t timeout)
{
    void *block;

    Q_REQUIRE_ID(200, me != NULL);

    block = rt_mp_alloc(&me->rtpool, timeout);

#ifdef QF_RTMPOOL_DEBUG
    if (block != NULL) {
        ++me->used_count;
        if (me->used_count > me->max_used) {
            me->max_used = me->used_count;
        }
        rt_kprintf("[QPC] Allocating block at %p from pool '%s' (%u/%u used)\n",
                   block, me->rtpool.parent.name,
                   (unsigned)me->used_count, (unsigned)me->block_count);
    }
#endif

    return block;
}

/*!
* @brief Free a previously allocated block back to the RT-Thread memory pool
*
* @param[in,out] me      pointer to the QF_RTMemPool
* @param[in]     block   pointer to the block to free
*
* @returns
* RT_EOK on success, or error code on failure
*/
rt_err_t QF_RTMemPool_free(QF_RTMemPool *const me, void *block)
{
    Q_REQUIRE_ID(300, me != NULL);
    Q_REQUIRE_ID(301, block != NULL);

#ifdef QF_RTMPOOL_DEBUG
    rt_kprintf("[QPC] Freeing block at %p to pool '%s' (%u/%u used)\n",
               block, me->rtpool.parent.name,
               (unsigned)me->used_count, (unsigned)me->block_count);
    if (me->used_count > 0) {
        --me->used_count;
    }
#endif

    /* Free the block back to the RT-Thread memory pool */
    rt_mp_free(block);

    return RT_EOK;
}

/*!
* @brief Get the number of free blocks in the RT-Thread memory pool
*
* @param[in] me      pointer to the QF_RTMemPool
*
* @returns
* Number of free blocks in the memory pool
*/
uint16_t QF_RTMemPool_getFreeCount(QF_RTMemPool const *const me)
{
    Q_REQUIRE_ID(400, me != NULL);

    return (uint16_t)(me->rtpool.block_free_count);
}

#ifdef QF_RTMPOOL_DEBUG
/*!
* @brief Get the number of used blocks in the RT-Thread memory pool
*
* @param[in] me      pointer to the QF_RTMemPool
*
* @returns
* Number of currently used blocks in the memory pool
*/
uint16_t QF_RTMemPool_getUsedCount(QF_RTMemPool const *const me)
{
    Q_REQUIRE_ID(500, me != NULL);

    return me->used_count;
}

/*!
* @brief Get the maximum number of blocks that have been simultaneously allocated
*
* @param[in] me      pointer to the QF_RTMemPool
*
* @returns
* Maximum number of blocks simultaneously used since initialization
*/
uint16_t QF_RTMemPool_getMaxUsed(QF_RTMemPool const *const me)
{
    Q_REQUIRE_ID(600, me != NULL);

    return me->max_used;
}

/*!
* @brief Print debug information about the memory pool
*
* @param[in] me      pointer to the QF_RTMemPool
*/
void QF_RTMemPool_printStats(QF_RTMemPool const *const me)
{
    Q_REQUIRE_ID(700, me != NULL);

    rt_kprintf("[QPC] Memory pool '%s' stats:\n", me->rtpool.parent.name);
    rt_kprintf("  - Total blocks: %u\n", me->block_count);
    rt_kprintf("  - Block size: %u bytes\n", me->block_size);
    rt_kprintf("  - Free blocks: %u\n", me->rtpool.block_free_count);
    rt_kprintf("  - Used blocks: %u\n", me->used_count);
    rt_kprintf("  - Peak usage: %u blocks\n", me->max_used);
    rt_kprintf("  - Storage: %p\n", me->storage);
}
#endif /* QF_RTMPOOL_DEBUG */
