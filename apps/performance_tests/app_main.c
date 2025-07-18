/*============================================================================
* Product: Performance Test Suite Application Main Implementation
* Last updated for version 7.2.0
* Last updated on  2024-07-13
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2024 Quantum Leaps, LLC. All rights reserved.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <www.gnu.org/licenses/>.
*
* Contact information:
* <www.state-machine.com/licensing>
* <info@state-machine.com>
============================================================================*/
#include "app_main.h"
#include <rtthread.h>
#include "qpc.h"
#include "bsp.h"

#include "counter_ao.h"
#include "timer_ao.h"
#include "logger_ao.h"
#include "qf.h"

/*==========================================================================*/
/* Static Event Queue Storage for Each Active Object */
/*==========================================================================*/

/* Static event queue storage for Counter AO - thread-safe event handling */
static QEvt const *counterQueueSto[COUNTER_QUEUE_SIZE];

/* Static event queue storage for Timer AO - timer event management */
static QEvt const *timerQueueSto[TIMER_QUEUE_SIZE];

/* Static event queue storage for Logger AO - message logging */
static QEvt const *loggerQueueSto[LOGGER_QUEUE_SIZE];

/*==========================================================================*/
/* Active Object Instance Table */
/*==========================================================================*/

/* Table of Active Object pointers for centralized management */
static QActive * const activeObjects[] = {
    &g_counter.super,    /* Counter Active Object instance */
    &g_timer.super,      /* Timer Active Object instance */
    &g_logger.super      /* Logger Active Object instance */
};

/*==========================================================================*/
/* Global Subscription Table for Publish-Subscribe System */
/*==========================================================================*/

/* Global subscription storage - enables publish-subscribe event routing */
QSubscrList subscriptions[MAX_APP_SIG];

/*==========================================================================*/
/* Application State Variables */
/*==========================================================================*/

/* Application initialization state flag - prevents double initialization */
static rt_bool_t qf_initialized = RT_FALSE;

/* Application running state flag - tracks if AOs are started */
static rt_bool_t qf_started = RT_FALSE;

/* Application statistics counters for monitoring */
static uint32_t g_app_event_count = 0U;      /* Total events processed */
static uint32_t g_app_error_count = 0U;      /* Total error count */
static uint32_t g_app_runtime_ms = 0U;       /* Application runtime in ms */

/*==========================================================================*/
/* Application Initialization Function */
/*==========================================================================*/

void app_main_init(void)
{
    /* Prevent double initialization */
    if (qf_initialized)
    {
        rt_kprintf("Performance Tests: Already initialized, skipping...\n");
        return;
    }

    rt_kprintf("=== Performance Tests Auto-Initialize ===\n");
    rt_kprintf("Performance Tests: Initializing...\n");

    /* Initialize QF framework - core event handling system */
    rt_kprintf("Performance Tests: Initializing QF framework...\n");
    QF_init();

    /* Initialize publish-subscribe system with global subscription table */
    rt_kprintf("Performance Tests: Initializing publish-subscribe system...\n");
    QF_psInit(subscriptions, Q_DIM(subscriptions));

    /* Print event struct sizes for debugging and memory optimization */
    rt_kprintf("sizeof(QEvt)=%d\n", (int)sizeof(QEvt));
    rt_kprintf("sizeof(CounterEvt)=%d\n", (int)sizeof(CounterEvt));
    rt_kprintf("sizeof(TimerEvt)=%d\n", (int)sizeof(TimerEvt));
    rt_kprintf("sizeof(LoggerEvt)=%d\n", (int)sizeof(LoggerEvt));
    rt_kprintf("sizeof(PerfTestEvt)=%d\n", (int)sizeof(PerfTestEvt));

    /* Initialize Board Support Package - hardware abstraction layer */
    rt_kprintf("Performance Tests: Initializing BSP...\n");
    BSP_init();

    /* Initialize individual Active Objects with their respective configurations */
    rt_kprintf("Performance Tests: Initializing Active Objects...\n");

    /* Initialize Counter AO - handles counting operations */
    CounterAO_ctor(&g_counter);
    rt_kprintf("Performance Tests: Counter AO initialized\n");

    /* Initialize Timer AO - handles timing operations */
    TimerAO_ctor(&g_timer);
    rt_kprintf("Performance Tests: Timer AO initialized\n");

    /* Initialize Logger AO - handles message logging */
    LoggerAO_ctor(&g_logger);
    rt_kprintf("Performance Tests: Logger AO initialized\n");

    /* Mark initialization as complete */
    qf_initialized = RT_TRUE;
    g_app_runtime_ms = rt_tick_get() * 1000U / RT_TICK_PER_SECOND;

    rt_kprintf("Performance Tests: Initialization complete\n");
}

/*==========================================================================*/
/* Application Start Function */
/*==========================================================================*/

void app_main_start(void)
{
    /* Ensure application is initialized before starting */
    if (!qf_initialized) {
        rt_kprintf("Performance Tests: Not initialized, calling init first...\n");
        app_main_init();
    }

    /* Prevent double start */
    if (qf_started) {
        rt_kprintf("Performance Tests: Already started, skipping...\n");
        return;
    }

    rt_kprintf("=== Performance Tests Starting Active Objects ===\n");

    /* Start Counter AO with dedicated priority and queue */
    QActive_start_(&g_counter.super,
                   COUNTER_AO_PRIO,              /* Priority level */
                   counterQueueSto,              /* Event queue storage */
                   Q_DIM(counterQueueSto),       /* Queue size */
                   (void *)0,                    /* No stack (uses main stack) */
                   0U,                           /* Stack size = 0 */
                   (void *)0);                   /* No initialization parameter */
    rt_kprintf("Performance Tests: Counter AO started with priority %d\n", COUNTER_AO_PRIO);

    /* Start Timer AO with dedicated priority and queue */
    QActive_start_(&g_timer.super,
                   TIMER_AO_PRIO,                /* Priority level */
                   timerQueueSto,                /* Event queue storage */
                   Q_DIM(timerQueueSto),         /* Queue size */
                   (void *)0,                    /* No stack (uses main stack) */
                   0U,                           /* Stack size = 0 */
                   (void *)0);                   /* No initialization parameter */
    rt_kprintf("Performance Tests: Timer AO started with priority %d\n", TIMER_AO_PRIO);

    /* Start Logger AO with dedicated priority and queue */
    QActive_start_(&g_logger.super,
                   LOGGER_AO_PRIO,               /* Priority level */
                   loggerQueueSto,               /* Event queue storage */
                   Q_DIM(loggerQueueSto),        /* Queue size */
                   (void *)0,                    /* No stack (uses main stack) */
                   0U,                           /* Stack size = 0 */
                   (void *)0);                   /* No initialization parameter */
    rt_kprintf("Performance Tests: Logger AO started with priority %d\n", LOGGER_AO_PRIO);

    /* Mark application as started */
    qf_started = RT_TRUE;

    rt_kprintf("Performance Tests: All Active Objects started successfully\n");
    rt_kprintf("Performance Tests: Application ready for testing\n");
}

/*==========================================================================*/
/* Application Stop Function */
/*==========================================================================*/

void app_main_stop(void)
{
    if (!qf_started) {
        rt_kprintf("Performance Tests: Not started, nothing to stop\n");
        return;
    }

    rt_kprintf("=== Performance Tests Stopping Active Objects ===\n");

    /* Stop all Active Objects gracefully by unsubscribing and stopping */
    for (uint8_t i = 0U; i < Q_DIM(activeObjects); ++i) {
        if (activeObjects[i] != (QActive *)0) {
            /* Unsubscribe from all signals to prevent event delivery */
            QActive_unsubscribeAll(activeObjects[i]);
            rt_kprintf("Performance Tests: AO[%d] unsubscribed from all signals\n", i);

            /* RT-Thread port doesn't support QActive_stop, use alternative approach */
            /* Send a termination signal to the AO instead */
            QEvt const *termEvt = Q_NEW(QEvt, PERF_TEST_STOP_SIG);
            if (termEvt != NULL) {
                QACTIVE_POST(activeObjects[i], termEvt, &l_appMain);
                rt_kprintf("Performance Tests: AO[%d] termination signal sent\n", i);
            }
        }
    }

    /* Mark application as stopped */
    qf_started = RT_FALSE;

    rt_kprintf("Performance Tests: All Active Objects stopped\n");
}

/*==========================================================================*/
/* Application Status Query Functions */
/*==========================================================================*/

rt_bool_t app_main_is_initialized(void)
{
    return qf_initialized;
}

rt_bool_t app_main_is_running(void)
{
    return qf_started;
}

/*==========================================================================*/
/* Application Statistics Function */
/*==========================================================================*/

void app_main_get_stats(void)
{
    uint32_t current_time_ms = rt_tick_get() * 1000U / RT_TICK_PER_SECOND;
    uint32_t uptime_ms = current_time_ms - g_app_runtime_ms;

    rt_kprintf("\n=== Performance Test Application Statistics ===\n");
    rt_kprintf("Initialization Status: %s\n", qf_initialized ? "INITIALIZED" : "NOT_INITIALIZED");
    rt_kprintf("Running Status: %s\n", qf_started ? "RUNNING" : "STOPPED");
    rt_kprintf("Application Uptime: %u ms\n", uptime_ms);
    rt_kprintf("Total Events Processed: %u\n", g_app_event_count);
    rt_kprintf("Total Errors: %u\n", g_app_error_count);
    rt_kprintf("Active Objects Count: %d\n", (int)Q_DIM(activeObjects));
    rt_kprintf("Subscription Table Size: %d\n", (int)Q_DIM(subscriptions));

    /* Print individual AO status */
    rt_kprintf("\n--- Active Object Status ---\n");
    rt_kprintf("Counter AO Priority: %d\n", COUNTER_AO_PRIO);
    rt_kprintf("Timer AO Priority: %d\n", TIMER_AO_PRIO);
    rt_kprintf("Logger AO Priority: %d\n", LOGGER_AO_PRIO);

    /* Print memory usage information */
    rt_kprintf("\n--- Memory Usage ---\n");
    rt_kprintf("Counter Queue Size: %d events\n", COUNTER_QUEUE_SIZE);
    rt_kprintf("Timer Queue Size: %d events\n", TIMER_QUEUE_SIZE);
    rt_kprintf("Logger Queue Size: %d events\n", LOGGER_QUEUE_SIZE);
    rt_kprintf("Total Queue Memory: %d bytes\n",
               (int)(sizeof(counterQueueSto) + sizeof(timerQueueSto) + sizeof(loggerQueueSto)));
    rt_kprintf("================================================\n\n");
}

/*==========================================================================*/
/* MSH Shell Command Implementations */
/*==========================================================================*/

void app_init_cmd(void)
{
    rt_kprintf("Initializing Performance Test Application...\n");
    app_main_init();
}

void app_start_cmd(void)
{
    rt_kprintf("Starting Performance Test Application...\n");
    app_main_start();
}

void app_stop_cmd(void)
{
    rt_kprintf("Stopping Performance Test Application...\n");
    app_main_stop();
}

void app_status_cmd(void)
{
    rt_kprintf("Performance Test Application Status:\n");
    rt_kprintf("  Initialized: %s\n", app_main_is_initialized() ? "YES" : "NO");
    rt_kprintf("  Running: %s\n", app_main_is_running() ? "YES" : "NO");
}

void app_stats_cmd(void)
{
    app_main_get_stats();
}

/*==========================================================================*/
/* MSH Shell Command Exports */
/*==========================================================================*/

/* Export shell commands for interactive testing */
MSH_CMD_EXPORT(app_init_cmd, Initialize performance test application);
MSH_CMD_EXPORT(app_start_cmd, Start performance test application);
MSH_CMD_EXPORT(app_stop_cmd, Stop performance test application);
MSH_CMD_EXPORT(app_status_cmd, Show application status);
MSH_CMD_EXPORT(app_stats_cmd, Show application statistics);
