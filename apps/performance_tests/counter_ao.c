/*============================================================================
* Product: Counter Active Object Implementation
* Last updated for version 7.2.0
* Last updated on  2024-12-19
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
#include "counter_ao.h"
#include "app_main.h"
#include "bsp.h"
#include "inc/sys_log.h"

Q_DEFINE_THIS_MODULE("counter_ao")

/*==========================================================================*/
/* Counter AO Local Definitions */
/*==========================================================================*/

/* Counter AO states */
static QState CounterAO_initial (CounterAO * const me, QEvt const * const e);
static QState CounterAO_stopped (CounterAO * const me, QEvt const * const e);
static QState CounterAO_running  (CounterAO * const me, QEvt const * const e);

/* Singleton instance */
static CounterAO l_counterAO;

/*==========================================================================*/
/* Counter AO Public Interface */
/*==========================================================================*/
void CounterAO_ctor(void) {
    rt_kprintf("[QPC] module: %s\n", Q_this_module_);
    CounterAO * const me = &l_counterAO;

    /* Call base class constructor */
    QActive_ctor(&me->super, Q_STATE_CAST(&CounterAO_initial));

    /* Initialize time event */
    QTimeEvt_ctorX(&me->timeEvt, &me->super, COUNTER_UPDATE_SIG, 0U);

    /* Initialize instance variables */
    me->counter_value = 0U;
    me->update_count = 0U;
    me->is_running = RT_FALSE;
}

CounterAO *CounterAO_getInstance(void) {
    return &l_counterAO;
}

uint32_t CounterAO_getValue(void) {
    CounterAO * const me = &l_counterAO;
    return me->counter_value; /* Atomic read on 32-bit platforms */
}

uint32_t CounterAO_getUpdateCount(void) {
    CounterAO * const me = &l_counterAO;
    return me->update_count; /* Atomic read on 32-bit platforms */
}

rt_bool_t CounterAO_isRunning(void) {
    CounterAO * const me = &l_counterAO;
    return me->is_running; /* Atomic read for boolean */
}

/*==========================================================================*/
/* Counter AO State Machine Implementation */
/*==========================================================================*/
static QState CounterAO_initial(CounterAO * const me, QEvt const * const e) {
    (void)e; /* Suppress unused parameter warning */

    /* Initialize the Counter AO */
    me->counter_value = 0U;
    me->update_count = 0U;
    me->is_running = RT_FALSE;

    /* Reduce logging during startup to prevent event queue overflow */
    /* SYS_LOG_D("CounterAO: Initial state entered"); */

    /* Subscribe to relevant signals */
    QActive_subscribe((QActive *)me, APP_START_SIG);
    QActive_subscribe((QActive *)me, APP_STOP_SIG);
    QActive_subscribe((QActive *)me, COUNTER_START_SIG);
    QActive_subscribe((QActive *)me, COUNTER_STOP_SIG);
    QActive_subscribe((QActive *)me, TIMER_TICK_SIG); /* Subscribe to timer ticks for counting */

    return Q_TRAN(&CounterAO_stopped);
}

static QState CounterAO_stopped(CounterAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->is_running = RT_FALSE;
            SYS_LOG_I("CounterAO: Stopped state entered");
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            /* Reduce startup logging to prevent queue overflow */
            /* SYS_LOG_D("CounterAO: Exiting stopped state"); */
            status = Q_HANDLED();
            break;
        }
        case APP_START_SIG:
        case COUNTER_START_SIG: {
            SYS_LOG_I("CounterAO: Starting counter");
            status = Q_TRAN(&CounterAO_running);
            break;
        }
        case APP_STOP_SIG:
        case COUNTER_STOP_SIG: {
            /* Already stopped, just acknowledge */
            SYS_LOG_D("CounterAO: Stop signal received while stopped");
            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}

static QState CounterAO_running(CounterAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->is_running = RT_TRUE;

            /* Start the periodic timer for counter updates */
            QTimeEvt_armX(&me->timeEvt,
                         BSP_TICKS_PER_SEC * COUNTER_UPDATE_INTERVAL_MS / 1000U,
                         BSP_TICKS_PER_SEC * COUNTER_UPDATE_INTERVAL_MS / 1000U);

            SYS_LOG_I("CounterAO: Running state entered, timer started");
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            /* Disarm the timer */
            QTimeEvt_disarm(&me->timeEvt);
            me->is_running = RT_FALSE;
            SYS_LOG_I("CounterAO: Exiting running state, timer stopped");
            status = Q_HANDLED();
            break;
        }
        case COUNTER_UPDATE_SIG: {
            /* Increment counter value */
            ++me->counter_value;
            ++me->update_count;

            /* Update global statistics */
            rt_mutex_take(g_stats_mutex, RT_WAITING_FOREVER);
            ++g_perf_stats.counter_updates;
            rt_mutex_release(g_stats_mutex);

            /* Toggle LED to show activity */
            BSP_ledToggle();

            /* Log periodic updates (every 10th update) */
            if ((me->update_count % 10U) == 0U) {
                SYS_LOG_I("CounterAO: Counter value = %u, updates = %u",
                               me->counter_value, me->update_count);
            }

            /* Publish counter update event */
            CounterUpdateEvt *counterEvt = Q_NEW(CounterUpdateEvt, COUNTER_UPDATE_SIG);
            if (counterEvt != (CounterUpdateEvt *)0) {
                counterEvt->counter_value = me->counter_value;
                counterEvt->timestamp = BSP_getTimestampMs();
                QF_PUBLISH((QEvt *)counterEvt, (void *)0);
            }

            status = Q_HANDLED();
            break;
        }
        case TIMER_TICK_SIG: {
            /* Handle timer tick events from TimerAO */
            TimerTickEvt const *tickEvt = (TimerTickEvt const *)e;
            
            /* Increment counter on each timer tick */
            ++me->counter_value;
            ++me->update_count;

            /* Update global statistics */
            rt_mutex_take(g_stats_mutex, RT_WAITING_FOREVER);
            ++g_perf_stats.counter_updates;
            rt_mutex_release(g_stats_mutex);

            /* Toggle LED to show activity */
            BSP_ledToggle();

            /* Log periodic updates (every 50th tick to reduce log volume) */
            if ((tickEvt->tick_count % 50U) == 0U) {
                SYS_LOG_I("CounterAO: Timer tick #%u, counter = %u",
                               tickEvt->tick_count, me->counter_value);
            }

            status = Q_HANDLED();
            break;
        }
        case APP_STOP_SIG:
        case COUNTER_STOP_SIG: {
            SYS_LOG_I("CounterAO: Stopping counter");
            status = Q_TRAN(&CounterAO_stopped);
            break;
        }
        case COUNTER_TIMEOUT_SIG: {
            /* Test timeout - stop the counter */
            SYS_LOG_E("CounterAO: Test timeout reached");
            status = Q_TRAN(&CounterAO_stopped);
            break;
        }
        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}
