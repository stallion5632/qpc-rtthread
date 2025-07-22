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
 * @brief ISR-safe event publishing implementation for RT-Thread port
 */
#define QP_IMPL
#include <ipc/ringbuffer.h>
#include "qf_isr_relay.h"
#include "qf_port.h"
#include "qassert.h"

Q_DEFINE_THIS_MODULE("qf_isr_relay")

#ifndef rt_hw_interrupt_disable
#define rt_hw_interrupt_disable() (0)
#endif
#ifndef rt_hw_interrupt_enable
#define rt_hw_interrupt_enable(x) ((void)(x))
#endif

/* ISR event relay control structure */
typedef struct
{
    struct rt_ringbuffer main_rb;
    struct rt_ringbuffer overflow_rb;
    rt_uint8_t main_storage[sizeof(QF_ISREvent) * QF_ISR_MAIN_BUFFER_SIZE];
    rt_uint8_t overflow_storage[sizeof(QF_ISREvent) * QF_ISR_OVERFLOW_BUFFER_SIZE];
    rt_bool_t overflow_active;
    rt_uint32_t lost_count;
    struct rt_semaphore notify_sem;
    rt_thread_t relay_thread;
    struct rt_mutex stats_mutex;
    QF_ISRStats stats;
    rt_bool_t initialized;
} QF_ISRRelay;

/* Global ISR relay instance */
static QF_ISRRelay l_isrRelay;

/*
 * @brief ISR relay thread entry function
 *
 * This thread waits for ISR notifications and processes buffered events
 * in batches to minimize the number of thread wakeups and improve efficiency.
 */

/* MISRA C: Magic numbers replaced with named constants */
#define QF_ISR_BATCH_INIT_SIZE      (8U)
#define QF_ISR_BATCH_MAX_SIZE       (16U)
#define QF_ISR_BATCH_MIN_SIZE       (1U)
#define QF_ISR_BATCH_UPPER_THR      (5U)
#define QF_ISR_BATCH_LOWER_THR      (1U)

static void QF_isrRelayThreadEntry(void *parameter)
{
    QF_ISREvent evt;
    rt_size_t n;
    rt_uint32_t batch_count;
    rt_uint32_t max_batch = QF_ISR_BATCH_INIT_SIZE;
    rt_tick_t start_time;
    rt_tick_t process_time;

    (void)parameter; /* unused parameter */

    for (;;)
    {
        /* Wait for ISR notification */
        (void)rt_sem_take(&l_isrRelay.notify_sem, RT_WAITING_FOREVER);

        start_time = rt_tick_get();
        batch_count = 0U;

        /* Process overflow buffer first (higher priority) */
        if (l_isrRelay.overflow_active == RT_TRUE)
        {
            while ((batch_count < max_batch) &&
                   ((n = rt_ringbuffer_get(&l_isrRelay.overflow_rb,
                                           (rt_uint8_t *)&evt, sizeof(evt))) == sizeof(evt)))
            {
                /* Create and publish event through standard QP/C API */
                QEvt *qe = Q_NEW(QEvt, evt.sig);
                if (qe != (QEvt *)0)
                {
                    QF_PUBLISH(qe, (void *)0);
                    batch_count++;
                }
                else
                {
                    (void)rt_mutex_take(&l_isrRelay.stats_mutex, RT_WAITING_FOREVER);
                    l_isrRelay.stats.events_lost++;
                    (void)rt_mutex_release(&l_isrRelay.stats_mutex);
                }
            }

            /* Check if overflow buffer is empty */
            if (rt_ringbuffer_data_len(&l_isrRelay.overflow_rb) == 0U)
            {
                l_isrRelay.overflow_active = RT_FALSE;
            }
        }

        /* Process main buffer */
        while ((batch_count < max_batch) &&
               ((n = rt_ringbuffer_get(&l_isrRelay.main_rb,
                                       (rt_uint8_t *)&evt, sizeof(evt))) == sizeof(evt)))
        {
             /* Create and publish event through standard QP/C API */
            QEvt *qe = Q_NEW(QEvt, evt.sig);
            if (qe != (QEvt *)0)
            {
                /* Could extend QEvt to include param if needed */
                QF_PUBLISH(qe, (void *)0);
                ++batch_count;
            }
            else
            {
                /* Event allocation failed */
                (void)rt_mutex_take(&l_isrRelay.stats_mutex, RT_WAITING_FOREVER);
                l_isrRelay.stats.events_lost++;
                (void)rt_mutex_release(&l_isrRelay.stats_mutex);
            }
        }

        process_time = rt_tick_get() - start_time;

        (void)rt_mutex_take(&l_isrRelay.stats_mutex, RT_WAITING_FOREVER);
        l_isrRelay.stats.events_processed += batch_count;
        l_isrRelay.stats.relay_wakeups++;

        if (batch_count > l_isrRelay.stats.max_batch_size)
        {
            l_isrRelay.stats.max_batch_size = batch_count;
        }

        if (process_time > l_isrRelay.stats.max_process_time)
        {
            l_isrRelay.stats.max_process_time = process_time;
        }
        (void)rt_mutex_release(&l_isrRelay.stats_mutex);

        /* Simple adaptive batch sizing */
        if (process_time > QF_ISR_BATCH_UPPER_THR)
        {
            if (max_batch < QF_ISR_BATCH_MAX_SIZE)
            {
                max_batch++;
            }
        }
        else if (process_time < QF_ISR_BATCH_LOWER_THR)
        {
            if (max_batch > QF_ISR_BATCH_MIN_SIZE)
            {
                max_batch--;
            }
        }
    }
}

/*
 * @brief Initialize ISR relay system
 *
 * This function must be called before any ISR event publishing.
 * It initializes ring buffers, synchronization objects, and statistics.
 */
void QF_isrRelayInit(void)
{
    if (l_isrRelay.initialized)
    {
        return; /* Already initialized */
    }
    else
    {
        rt_kprintf("[ISR] QF_isrRelayInit: Initializing ISR relay system\n");
    }

    /* Initialize ring buffers */
    rt_ringbuffer_init(&l_isrRelay.main_rb,
                       l_isrRelay.main_storage,
                       sizeof(l_isrRelay.main_storage));

    rt_ringbuffer_init(&l_isrRelay.overflow_rb,
                       l_isrRelay.overflow_storage,
                       sizeof(l_isrRelay.overflow_storage));

    /* Initialize control variables */
    l_isrRelay.overflow_active = RT_FALSE;
    l_isrRelay.lost_count = 0U;

    /* Initialize synchronization objects */
    (void)rt_sem_init(&l_isrRelay.notify_sem, "isr_sem", 0U, RT_IPC_FLAG_PRIO);
    (void)rt_mutex_init(&l_isrRelay.stats_mutex, "isr_stats", RT_IPC_FLAG_PRIO);

    /* Initialize statistics */
    l_isrRelay.stats.events_processed = 0U;
    l_isrRelay.stats.events_lost = 0U;
    l_isrRelay.stats.max_batch_size = 0U;
    l_isrRelay.stats.max_process_time = 0U;
    l_isrRelay.stats.relay_wakeups = 0U;

    l_isrRelay.initialized = RT_TRUE;
}

/*
 * @brief Start ISR relay thread
 *
 * This function creates and starts the relay thread that processes
 * ISR events. Must be called after QF_isrRelayInit().
 */

#define QF_ISR_RELAY_THREAD_SLICE   (10U)

void QF_isrRelayStart(void)
{
    Q_REQUIRE_ID(100, l_isrRelay.initialized);

    l_isrRelay.relay_thread = rt_thread_create("qf_isr_relay",
                                               QF_isrRelayThreadEntry,
                                               RT_NULL,
                                               QF_ISR_RELAY_STACK_SIZE,
                                               QF_ISR_RELAY_THREAD_PRIO,
                                               QF_ISR_RELAY_THREAD_SLICE);

    Q_ALLEGE_ID(101, l_isrRelay.relay_thread != RT_NULL);
    Q_ALLEGE_ID(102, rt_thread_startup(l_isrRelay.relay_thread) == RT_EOK);
}

/*
 * @brief Publish event from ISR context
 *
 * This is the main API for publishing events from interrupt service routines.
 * It uses a lightweight ring buffer mechanism with overflow protection.
 *
 * @param sig Signal to publish
 * @param poolId Event pool ID (for future extension)
 * @param param Optional parameter (for future extension)
 */
void QF_publishFromISR(QSignal sig, rt_uint8_t poolId, rt_uint16_t param)
{
    QF_ISREvent evt;
    rt_base_t level;
    rt_bool_t notify_needed = RT_FALSE;

    Q_REQUIRE_ID(200, l_isrRelay.initialized);

    /* Prepare event descriptor */
    evt.sig = sig;
    evt.poolId = poolId;
    evt.param = param;
    evt.timestamp = rt_tick_get();

    /* Critical section to access ring buffers atomically */
    level = rt_hw_interrupt_disable();

    /* Try to put event in main buffer first */
    if (rt_ringbuffer_put(&l_isrRelay.main_rb, (const rt_uint8_t *)&evt, sizeof(evt)) != sizeof(evt))
    {
        /* Main buffer full, try overflow buffer */
        if (rt_ringbuffer_put(&l_isrRelay.overflow_rb, (const rt_uint8_t *)&evt, sizeof(evt)) == sizeof(evt))
        {
            l_isrRelay.overflow_active = RT_TRUE;
            notify_needed = RT_TRUE;
        }
        else
        {
            /* Both buffers full, event lost */
            l_isrRelay.lost_count++;
        }
    }
    else
    {
        notify_needed = RT_TRUE;
    }

    rt_hw_interrupt_enable(level);

    /* Notify relay thread if event was buffered */
    if (notify_needed)
    {
        (void)rt_sem_release(&l_isrRelay.notify_sem);
    }
}

/*
 * @brief Print ISR relay statistics
 *
 * This function prints detailed statistics about ISR event processing
 * including throughput, loss rate, and performance metrics.
 */
void QF_isrRelayPrintStats(void)
{
    QF_ISRStats stats_copy;
    rt_uint32_t main_usage;
    rt_uint32_t overflow_usage;
    rt_uint32_t total_lost;

    if (!l_isrRelay.initialized)
    {
        rt_kprintf("ISR Relay not initialized\n");
        return;
    }

    /* Get snapshot of statistics */
    (void)rt_mutex_take(&l_isrRelay.stats_mutex, RT_WAITING_FOREVER);
    stats_copy = l_isrRelay.stats;
    total_lost = l_isrRelay.stats.events_lost + l_isrRelay.lost_count;
    (void)rt_mutex_release(&l_isrRelay.stats_mutex);

    /* Get buffer usage */
    main_usage = rt_ringbuffer_data_len(&l_isrRelay.main_rb) / sizeof(QF_ISREvent);
    overflow_usage = rt_ringbuffer_data_len(&l_isrRelay.overflow_rb) / sizeof(QF_ISREvent);

    rt_kprintf("\n=== QF ISR Relay Statistics ===\n");
    rt_kprintf("Events processed: %lu\n", stats_copy.events_processed);
    rt_kprintf("Events lost:      %lu\n", total_lost);
    rt_kprintf("Relay wakeups:    %lu\n", stats_copy.relay_wakeups);
    rt_kprintf("Max batch size:   %lu\n", stats_copy.max_batch_size);
    rt_kprintf("Max process time: %lu ticks\n", stats_copy.max_process_time);
    rt_kprintf("Buffer usage:     main=%lu/%u, overflow=%lu/%u\n",
               main_usage, QF_ISR_MAIN_BUFFER_SIZE,
               overflow_usage, QF_ISR_OVERFLOW_BUFFER_SIZE);
    rt_kprintf("Overflow active:  %s\n",
               l_isrRelay.overflow_active ? "YES" : "NO");
    rt_kprintf("===============================\n\n");
}

/*
 * @brief Get ISR relay statistics
 *
 * @return Pointer to current statistics structure (read-only)
 */
QF_ISRStats const *QF_isrRelayGetStats(void)
{
    return &l_isrRelay.stats;
}
