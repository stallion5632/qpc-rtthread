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
* @date Last updated on: 2024-01-08
* @version Last updated for: @ref qpc_7_3_0
*
* @file
* @brief QF/C RT-Thread memory pool integration
*/
#define QP_IMPL           /* this is QP implementation */
#include "qf_port.h"      /* QF port */
#include "qf_mempool.h"
#include "qf_pkg.h"       /* QF package-scope interface */
#include "qassert.h"
#ifdef Q_SPY              /* QS software tracing enabled? */
    #include "qs_pkg.h"   /* QS package-scope internal interface */
#else
    #include "qs_dummy.h" /* disable the QS software tracing */
#endif /* Q_SPY */

Q_DEFINE_THIS_MODULE("qf_mempool")

#if QF_ENABLE_RT_MEMPOOL

/* Configuration constants */
#ifndef QF_RT_MEMPOOL_NUM_POOLS
#define QF_RT_MEMPOOL_NUM_POOLS 3U  /*!< Number of different event pools */
#endif

#ifndef QF_RT_MEMPOOL_SMALL_SIZE
#define QF_RT_MEMPOOL_SMALL_SIZE    64U   /*!< Small event size */
#endif

#ifndef QF_RT_MEMPOOL_MEDIUM_SIZE
#define QF_RT_MEMPOOL_MEDIUM_SIZE   128U  /*!< Medium event size */
#endif

#ifndef QF_RT_MEMPOOL_LARGE_SIZE
#define QF_RT_MEMPOOL_LARGE_SIZE    256U  /*!< Large event size */
#endif

#ifndef QF_RT_MEMPOOL_SMALL_COUNT
#define QF_RT_MEMPOOL_SMALL_COUNT   32U   /*!< Number of small events */
#endif

#ifndef QF_RT_MEMPOOL_MEDIUM_COUNT
#define QF_RT_MEMPOOL_MEDIUM_COUNT  16U   /*!< Number of medium events */
#endif

#ifndef QF_RT_MEMPOOL_LARGE_COUNT
#define QF_RT_MEMPOOL_LARGE_COUNT   8U    /*!< Number of large events */
#endif

/* RT-Thread memory pool structures */
typedef struct {
    struct rt_mempool pool;
    uint16_t block_size;
    uint16_t block_count;
    char *pool_memory;
    char name[RT_NAME_MAX];
} QF_RTMemPool;

/* Static memory pools */
static QF_RTMemPool l_memPools[QF_RT_MEMPOOL_NUM_POOLS];

/* Static memory for pools - aligned for better performance */
static char l_smallPoolMem[QF_RT_MEMPOOL_SMALL_COUNT * QF_RT_MEMPOOL_SMALL_SIZE]
    __attribute__((aligned(RT_ALIGN_SIZE)));
static char l_mediumPoolMem[QF_RT_MEMPOOL_MEDIUM_COUNT * QF_RT_MEMPOOL_MEDIUM_SIZE]
    __attribute__((aligned(RT_ALIGN_SIZE)));
static char l_largePoolMem[QF_RT_MEMPOOL_LARGE_COUNT * QF_RT_MEMPOOL_LARGE_SIZE]
    __attribute__((aligned(RT_ALIGN_SIZE)));

/* Pool statistics */
static struct {
    uint32_t allocations;
    uint32_t deallocations;
    uint32_t failures;
    uint32_t peak_usage[QF_RT_MEMPOOL_NUM_POOLS];
} l_poolStats;

/*..........................................................................*/
void QF_poolInit_RT(void) {
    /* Initialize small event pool */
    l_memPools[0].block_size = QF_RT_MEMPOOL_SMALL_SIZE;
    l_memPools[0].block_count = QF_RT_MEMPOOL_SMALL_COUNT;
    l_memPools[0].pool_memory = l_smallPoolMem;
    rt_strncpy(l_memPools[0].name, "qf_small", RT_NAME_MAX - 1);

    rt_err_t result = rt_mp_init(&l_memPools[0].pool,
                                 l_memPools[0].name,
                                 l_memPools[0].pool_memory,
                                 QF_RT_MEMPOOL_SMALL_COUNT,
                                 QF_RT_MEMPOOL_SMALL_SIZE);
    Q_ALLEGE_ID(801, result == RT_EOK);

    /* Initialize medium event pool */
    l_memPools[1].block_size = QF_RT_MEMPOOL_MEDIUM_SIZE;
    l_memPools[1].block_count = QF_RT_MEMPOOL_MEDIUM_COUNT;
    l_memPools[1].pool_memory = l_mediumPoolMem;
    rt_strncpy(l_memPools[1].name, "qf_medium", RT_NAME_MAX - 1);

    result = rt_mp_init(&l_memPools[1].pool,
                        l_memPools[1].name,
                        l_memPools[1].pool_memory,
                        QF_RT_MEMPOOL_MEDIUM_COUNT,
                        QF_RT_MEMPOOL_MEDIUM_SIZE);
    Q_ALLEGE_ID(802, result == RT_EOK);

    /* Initialize large event pool */
    l_memPools[2].block_size = QF_RT_MEMPOOL_LARGE_SIZE;
    l_memPools[2].block_count = QF_RT_MEMPOOL_LARGE_COUNT;
    l_memPools[2].pool_memory = l_largePoolMem;
    rt_strncpy(l_memPools[2].name, "qf_large", RT_NAME_MAX - 1);

    result = rt_mp_init(&l_memPools[2].pool,
                        l_memPools[2].name,
                        l_memPools[2].pool_memory,
                        QF_RT_MEMPOOL_LARGE_COUNT,
                        QF_RT_MEMPOOL_LARGE_SIZE);
    Q_ALLEGE_ID(803, result == RT_EOK);

    /* Initialize statistics */
    rt_memset(&l_poolStats, 0, sizeof(l_poolStats));

#ifdef Q_RT_DEBUG
    rt_kprintf("[QF_MEMPOOL] RT-Thread memory pools initialized:\n");
    rt_kprintf("  Small:  %u blocks x %u bytes\n",
               QF_RT_MEMPOOL_SMALL_COUNT, QF_RT_MEMPOOL_SMALL_SIZE);
    rt_kprintf("  Medium: %u blocks x %u bytes\n",
               QF_RT_MEMPOOL_MEDIUM_COUNT, QF_RT_MEMPOOL_MEDIUM_SIZE);
    rt_kprintf("  Large:  %u blocks x %u bytes\n",
               QF_RT_MEMPOOL_LARGE_COUNT, QF_RT_MEMPOOL_LARGE_SIZE);
#endif /* Q_RT_DEBUG */
}

/*..........................................................................*/
static uint8_t QF_selectPool(uint_fast16_t evtSize) {
    if (evtSize <= QF_RT_MEMPOOL_SMALL_SIZE) {
        return 0U; /* small pool */
    }
    else if (evtSize <= QF_RT_MEMPOOL_MEDIUM_SIZE) {
        return 1U; /* medium pool */
    }
    else if (evtSize <= QF_RT_MEMPOOL_LARGE_SIZE) {
        return 2U; /* large pool */
    }
    else {
        return 0xFFU; /* no suitable pool */
    }
}

/*..........................................................................*/
QEvt *QF_newX_RT(uint_fast16_t const evtSize,
                 uint_fast16_t const margin,
                 enum_t const sig) {
    uint8_t poolId = QF_selectPool(evtSize);

    if (poolId >= QF_RT_MEMPOOL_NUM_POOLS) {
        /* Event too large for any pool */
#ifdef Q_RT_DEBUG
        rt_kprintf("[QF_ERROR] Event size %u too large for memory pools\n", evtSize);
#endif
        ++l_poolStats.failures;
        return (QEvt *)0;
    }

    /* Try to allocate from the selected pool */
    QEvt *e = (QEvt *)rt_mp_alloc(&l_memPools[poolId].pool, RT_WAITING_NO);

    if (e != RT_NULL) {
        /* Initialize the event */
        e->sig = (QSignal)sig;
        e->poolId_ = poolId + 1U;  /* QPC uses 1-based pool IDs */
        e->refCtr_ = 0U;

        /* Update statistics */
        ++l_poolStats.allocations;
        uint32_t used = l_memPools[poolId].block_count -
                       l_memPools[poolId].pool.block_free_count;
        if (used > l_poolStats.peak_usage[poolId]) {
            l_poolStats.peak_usage[poolId] = used;
        }

#ifdef Q_RT_DEBUG
        rt_kprintf("[QF_MEMPOOL] Allocated event: sig=%d, size=%u, pool=%u, ptr=%p\n",
                   sig, evtSize, poolId, e);
#endif /* Q_RT_DEBUG */
    }
    else {
        /* Allocation failed */
        ++l_poolStats.failures;
#ifdef Q_RT_DEBUG
        rt_kprintf("[QF_ERROR] Failed to allocate event: sig=%d, size=%u, pool=%u\n",
                   sig, evtSize, poolId);
#endif /* Q_RT_DEBUG */
    }

    return e;
}

/*..........................................................................*/
void QF_gc_RT(QEvt const * const e) {
    if ((e != (QEvt *)0) && (e->poolId_ != 0U)) {
        uint8_t poolId = e->poolId_ - 1U; /* Convert to 0-based index */
        Q_ASSERT(poolId < QF_RT_MEMPOOL_NUM_POOLS);
        Q_ASSERT(e->refCtr_ == 0U || e->refCtr_ == 1U);

#ifdef Q_RT_DEBUG
        rt_kprintf("[QF_MEMPOOL][GC] Freeing event: pool=%u, ptr=%p, sig=%u, refCtr=%u\n", poolId, e, e->sig, e->refCtr_);
#endif /* Q_RT_DEBUG */

        rt_mp_free((void *)e);
        ++l_poolStats.deallocations;

#ifdef Q_RT_DEBUG
        rt_kprintf("[QF_MEMPOOL][GC] Event freed successfully: pool=%u, ptr=%p\n", poolId, e);
#endif /* Q_RT_DEBUG */
    } else {
#ifdef Q_RT_DEBUG
        rt_kprintf("[QF_MEMPOOL][GC][FATAL] Attempt to free invalid or static event: ptr=%p, poolId_=%u\n", e, e ? e->poolId_ : 0);
#endif
        Q_ASSERT(0); /* 直接断言，强制定位非法释放 */
    }
}

/*..........................................................................*/
void QF_poolGetStat_RT(uint8_t poolId,
                       uint16_t *pFree,
                       uint16_t *pUsed,
                       uint16_t *pPeak) {
    if (poolId < QF_RT_MEMPOOL_NUM_POOLS) {
        uint16_t available = l_memPools[poolId].pool.block_free_count;
        *pFree = available;
        *pUsed = l_memPools[poolId].block_count - available;
        *pPeak = l_poolStats.peak_usage[poolId];
    }
    else {
        *pFree = 0U;
        *pUsed = 0U;
        *pPeak = 0U;
    }
}

/*..........................................................................*/
void QF_poolPrintStats_RT(void) {
    rt_kprintf("\n==== QF RT-Thread Memory Pool Statistics ====\n");
    rt_kprintf("Total allocations: %lu\n", l_poolStats.allocations);
    rt_kprintf("Total deallocations: %lu\n", l_poolStats.deallocations);
    rt_kprintf("Total failures: %lu\n", l_poolStats.failures);
    rt_kprintf("Outstanding events: %lu\n",
               l_poolStats.allocations - l_poolStats.deallocations);

    for (uint8_t i = 0U; i < QF_RT_MEMPOOL_NUM_POOLS; ++i) {
        uint16_t free, used, peak;
        QF_poolGetStat_RT(i, &free, &used, &peak);
        rt_kprintf("Pool %u (%s): Free=%u, Used=%u, Peak=%u/%u\n",
                   i, l_memPools[i].name, free, used, peak, l_memPools[i].block_count);
    }
    rt_kprintf("=============================================\n");
}

#endif /* QF_ENABLE_RT_MEMPOOL */
