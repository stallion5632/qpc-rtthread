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
 * @brief QF/C RT-Thread Memory Pool Adapter implementation.
 */
#define QP_IMPL /* this is QP implementation */

#include "qf_port.h" /* QF port */
#include "qep.h"     /* For QEvt, enum_t, QSignal, etc. */
#include "qf.h"      /* QF/C main header, for QEvt definition */
#include <qf_pkg.h>
#include "qf_rtmpool_config.h" /* RT-Thread mempool configuration */
#include "qf_rtmpool.h"        /* RT-Thread memory pool adapter */
#include "qassert.h"           /* QP assertions */
#include <rtdef.h>

Q_DEFINE_THIS_MODULE("qf_rtmpool")

#ifndef rt_hw_interrupt_disable
#define rt_hw_interrupt_disable() (0)
#endif
#ifndef rt_hw_interrupt_enable
#define rt_hw_interrupt_enable(x) ((void)(x))
#endif

#if defined(QF_RTMPOOL_EXT) && (QF_RTMPOOL_EXT == 1)
typedef struct
{
    QF_RTMemPool *pools[QF_MAX_EPOOL];
    rt_uint8_t nPools;
    struct
    {
        rt_uint32_t allocations;
        rt_uint32_t failures;
        rt_uint32_t fallbacks;
    } stats;
} QF_RTMemPoolMgr;

static QF_RTMemPoolMgr l_poolMgr;
static rt_uint8_t l_initialized = (rt_uint8_t)0;
#endif /* defined(QF_RTMPOOL_EXT) && (QF_RTMPOOL_EXT == 1) */

/*
 * @brief Recycles a dynamic event to its original RT-Thread memory pool.
 * @note This implementation must be defined after l_poolMgr.
 */
void QF_gc_RT(QEvt const *const e)
{
#if defined(QF_RTMPOOL_EXT) && (QF_RTMPOOL_EXT == 1)
    if (e != RT_NULL)
    {
        rt_uint8_t id;
        if (e->poolId_ != (uint8_t)0)
        {
            QF_CRIT_STAT_
            QF_CRIT_E_();
            id = (rt_uint8_t)(e->poolId_ - (uint8_t)1);
            if (id < l_poolMgr.nPools) /* Check boundary */
            {
                (void)QF_RTMemPool_free(l_poolMgr.pools[id], (void *)e);
            }
            else
            {
                Q_ERROR_ID(301); /* Invalid pool ID */
            }
            QF_CRIT_X_();
        }
    }
#else
    /* If extended manager is not used, this function should not be called. */
    Q_ERROR_ID(302);
#endif /* defined(QF_RTMPOOL_EXT) && (QF_RTMPOOL_EXT == 1) */
}

/*!
 * @brief Initializes an RT-Thread memory pool adapter.
 */
rt_err_t QF_RTMemPool_init(QF_RTMemPool *const me, char const *name,
                           void *storage, rt_size_t n, rt_size_t size, uint16_t margin)
{
    rt_err_t ret;

    Q_REQUIRE_ID(100, me != RT_NULL);
    Q_REQUIRE_ID(101, storage != RT_NULL);
    Q_REQUIRE_ID(102, n > 0U);
    Q_REQUIRE_ID(103, size >= sizeof(QEvt));

    /* Store the meta-information */
    me->name = name;
    me->storage = storage;
    me->block_size = (uint16_t)size;
    me->block_count = (uint16_t)n;
#ifdef QF_RTMPOOL_DEBUG
    me->used_count = 0U;
    me->max_used = 0U;
#endif
    me->nMin = (uint16_t)n; /* Initially, all blocks are free */
    me->margin = margin;

    /* Initialize the RT-Thread memory pool */
    ret = rt_mp_init(&me->rtpool, name, storage, n, size);

    return ret;
}

/*!
 * @brief Allocates a block from the RT-Thread memory pool.
 */
void *QF_RTMemPool_alloc(QF_RTMemPool *const me, rt_int32_t timeout)
{
    void *block = RT_NULL;
    rt_base_t level = 0;
    Q_REQUIRE_ID(200, me != RT_NULL);

    /* Strict margin: check, allocate, and update statistics in critical section */
    level = rt_hw_interrupt_disable();
    if (me->rtpool.block_free_count > me->margin)
    {
        block = rt_mp_alloc(&me->rtpool, timeout);
        if (block != RT_NULL)
        {
#ifdef QF_RTMPOOL_DEBUG
            me->used_count++;
            if (me->used_count > me->max_used)
            {
                me->max_used = me->used_count;
            }
#endif
            if (me->rtpool.block_free_count < me->nMin)
            {
                me->nMin = me->rtpool.block_free_count;
            }
        }
    }
    rt_hw_interrupt_enable(level);
    return block;
}

/*!
 * @brief Frees a previously allocated block back to the RT-Thread memory pool.
 */
rt_err_t QF_RTMemPool_free(QF_RTMemPool *const me, void *block)
{
    Q_REQUIRE_ID(300, me != RT_NULL);
    Q_REQUIRE_ID(301, block != RT_NULL);

    /* Update statistics in critical section if debug enabled */
#ifdef QF_RTMPOOL_DEBUG
    rt_base_t level = 0;
    level = rt_hw_interrupt_disable();
    if (me->used_count > 0U)
    {
        me->used_count--;
        /* Update min watermark after free */
        if (me->rtpool.block_free_count < me->nMin)
        {
            me->nMin = me->rtpool.block_free_count;
        }
    }
    rt_hw_interrupt_enable(level);
#endif

    rt_mp_free(block);
    return RT_EOK;
}

/*!
 * @brief Gets the number of free blocks in the memory pool.
 */
uint16_t QF_RTMemPool_getFreeCount(QF_RTMemPool const *const me)
{
    Q_REQUIRE_ID(400, me != RT_NULL);

    return (uint16_t)(me->rtpool.block_free_count);
}

#ifdef QF_RTMPOOL_DEBUG
/*!
 * @brief Gets the number of used blocks in the memory pool.
 */
uint16_t QF_RTMemPool_getUsedCount(QF_RTMemPool const *const me)
{
    Q_REQUIRE_ID(500, me != RT_NULL);
    return me->used_count;
}

/*!
 * @brief Gets the maximum number of blocks that have been simultaneously
 *        allocated (high-water mark).
 */
uint16_t QF_RTMemPool_getMaxUsed(QF_RTMemPool const *const me)
{
    Q_REQUIRE_ID(600, me != RT_NULL);
    return me->max_used;
}

/*!
 * @brief Prints debug statistics for the memory pool.
 */
void QF_RTMemPool_printStats(QF_RTMemPool const *const me)
{
    Q_REQUIRE_ID(700, me != RT_NULL);

    rt_kprintf("Pool '%s': %u/%u blocks used, peak %u, min_free %u\n",
               me->name,
               me->used_count,
               me->block_count,
               me->max_used,
               me->nMin);
}
#endif /* QF_RTMPOOL_DEBUG */

#if defined(QF_RTMPOOL_EXT) && (QF_RTMPOOL_EXT == 1)

/* Selects the best-fit pool for a given event size. */
static rt_uint8_t QF_RTSelectPool(rt_uint16_t evtSize)
{
    rt_uint8_t p;
    rt_uint8_t best = l_poolMgr.nPools;
    rt_uint16_t minBlock = 0xFFFFU;

    for (p = 0U; p < l_poolMgr.nPools; ++p)
    {
        if ((l_poolMgr.pools[p]->block_size >= evtSize) &&
            (l_poolMgr.pools[p]->block_size < minBlock))
        {
            minBlock = l_poolMgr.pools[p]->block_size;
            best = p;
        }
    }
    return best;
}

/* Initializes the memory pool manager. */
void QF_RTMemPoolMgr_init(void)
{
    if (l_initialized == (rt_uint8_t)0)
    {
        l_poolMgr.nPools = (rt_uint8_t)0;
        l_poolMgr.stats.allocations = (rt_uint32_t)0;
        l_poolMgr.stats.failures = (rt_uint32_t)0;
        l_poolMgr.stats.fallbacks = (rt_uint32_t)0;
        l_initialized = (rt_uint8_t)1;
    }
}

/* Registers an RT-Thread memory pool with the manager. */
rt_uint8_t QF_RTMemPoolMgr_registerPool(QF_RTMemPool *const rtPool)
{
    rt_uint8_t id;
    QF_CRIT_STAT_
    QF_CRIT_E_();

    Q_ASSERT_ID(800, l_poolMgr.nPools < QF_MAX_EPOOL);
    id = l_poolMgr.nPools;
    l_poolMgr.pools[id] = rtPool;
    l_poolMgr.nPools++;

    QF_CRIT_X_();
    return id;
}

/* Allocates an event with margin and fallback support. */
QEvt *QF_newX_RT(rt_uint16_t evtSize,
                 rt_uint16_t margin,
                 enum_t sig)
{
    QEvt *e = RT_NULL;
    rt_uint8_t pid;

    if (l_initialized == (rt_uint8_t)0)
    {
        QF_RTMemPoolMgr_init();
    }

    pid = QF_RTSelectPool(evtSize);

    if (pid < l_poolMgr.nPools)
    {
        uint16_t use_margin = (margin != 0xFFFFU) ? margin : l_poolMgr.pools[pid]->margin;
        e = (QEvt *)QF_RTMemPool_alloc(l_poolMgr.pools[pid], RT_WAITING_NO);
        /* Fallback to larger pools if the primary pool is empty or below margin */
        if (e == RT_NULL)
        {
            rt_uint8_t p;
            for (p = (rt_uint8_t)(pid + 1U); p < l_poolMgr.nPools; ++p)
            {
                use_margin = (margin != 0xFFFFU) ? margin : l_poolMgr.pools[p]->margin;
                e = (QEvt *)QF_RTMemPool_alloc(l_poolMgr.pools[p], RT_WAITING_NO);
                if (e != RT_NULL)
                {
                    pid = p; /* Update pool ID to the fallback pool */
                    l_poolMgr.stats.fallbacks++;
                    break;
                }
            }
        }
        if (e != RT_NULL)
        {
            e->sig = (QSignal)sig;
            e->poolId_ = (uint8_t)(pid + 1U);
            e->refCtr_ = 0U;
            l_poolMgr.stats.allocations++;
        }
        else
        {
            l_poolMgr.stats.failures++;
            rt_kprintf("[QF_newX_RT] Allocation failed: evtSize=%u, pool=%u, margin=%u, free=%u\n",
                       evtSize, pid, use_margin, QF_RTMemPool_getFreeCount(l_poolMgr.pools[pid]));
            Q_ERROR_ID(801); /* Allocation failed */
        }
    }
    else
    {
        Q_ERROR_ID(802); /* No suitable pool found */
    }

    return e;
}

/* Retrieves memory pool manager statistics. */
void QF_RTMemPoolMgr_getStats(rt_uint8_t *const nPools,
                              rt_uint32_t *const allocs,
                              rt_uint32_t *const fails,
                              rt_uint32_t *const fallbacks)
{
    QF_CRIT_STAT_
    QF_CRIT_E_();
    if (nPools != RT_NULL)
    {
        *nPools = l_poolMgr.nPools;
    }
    if (allocs != RT_NULL)
    {
        *allocs = l_poolMgr.stats.allocations;
    }
    if (fails != RT_NULL)
    {
        *fails = l_poolMgr.stats.failures;
    }
    if (fallbacks != RT_NULL)
    {
        *fallbacks = l_poolMgr.stats.fallbacks;
    }
    QF_CRIT_X_();
}

/*
 * @brief Prints memory pool manager statistics.
 *
 * This function prints the number of pools, allocations, failures, and fallbacks.
 * It also prints statistics for each registered pool.
 */
void QF_RTMemPoolMgr_printStats(void)
{
    rt_uint8_t np = 0U;
    rt_uint32_t allocs = 0U;
    rt_uint32_t fails = 0U;
    rt_uint32_t fallbacks = 0U;
    rt_uint8_t i = 0U;

    QF_RTMemPoolMgr_getStats(&np, &allocs, &fails, &fallbacks);
    (void)rt_kprintf("Pools:%u Allocs:%u Fails:%u Fallbacks:%u\n",
                     np, allocs, fails, fallbacks);

    for (i = 0U; i < np; ++i)
    {
        QF_RTMemPool_printStats(l_poolMgr.pools[i]);
    }
}

#endif /* defined(QF_RTMPOOL_EXT) && (QF_RTMPOOL_EXT == 1) */
