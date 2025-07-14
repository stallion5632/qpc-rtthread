/*============================================================================
* Product: Timer Active Object Implementation
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
#include "timer_ao.h"
#include "counter_ao.h"
#include "bsp.h"
#include "sys_log.h"

Q_DEFINE_THIS_MODULE("timer_ao")

/*==========================================================================*/
/* Timer AO Local Definitions */
/*==========================================================================*/

/* Timer AO states */
static QState TimerAO_initial   (TimerAO * const me, QEvt const * const e);
static QState TimerAO_stopped   (TimerAO * const me, QEvt const * const e);
static QState TimerAO_running   (TimerAO * const me, QEvt const * const e);
static QState TimerAO_reporting (TimerAO * const me, QEvt const * const e);

/* Singleton instance */
static TimerAO l_timerAO;

/*==========================================================================*/
/* Timer AO Public Interface */
/*==========================================================================*/
void TimerAO_ctor(void) {
    rt_kprintf("[QPC] module: %s\n", Q_this_module_);
    TimerAO * const me = &l_timerAO;

    /* Call base class constructor */
    QActive_ctor(&me->super, Q_STATE_CAST(&TimerAO_initial));

    /* Initialize time events */
    QTimeEvt_ctorX(&me->tickTimeEvt, &me->super, TIMER_TICK_SIG, 0U);
    QTimeEvt_ctorX(&me->reportTimeEvt, &me->super, TIMER_REPORT_SIG, 0U);

    /* Initialize instance variables */
    me->tick_count = 0U;
    me->start_time_ms = 0U;
    me->last_report_time = 0U;
    me->report_count = 0U;
    me->current_state = TIMER_STATE_STOPPED;
    me->is_running = RT_FALSE;
}

TimerAO *TimerAO_getInstance(void) {
    return &l_timerAO;
}

uint32_t TimerAO_getTickCount(void) {
    TimerAO * const me = &l_timerAO;
    return me->tick_count; /* Atomic read on 32-bit platforms */
}

uint32_t TimerAO_getElapsedMs(void) {
    TimerAO * const me = &l_timerAO;
    if (me->start_time_ms == 0U) {
        return 0U;
    }
    return BSP_getTimestampMs() - me->start_time_ms;
}

uint32_t TimerAO_getReportCount(void) {
    TimerAO * const me = &l_timerAO;
    return me->report_count; /* Atomic read on 32-bit platforms */
}

rt_bool_t TimerAO_isRunning(void) {
    TimerAO * const me = &l_timerAO;
    return me->is_running; /* Atomic read for boolean */
}

TimerState TimerAO_getCurrentState(void) {
    TimerAO * const me = &l_timerAO;
    return me->current_state; /* Atomic read for enum */
}

/*==========================================================================*/
/* Timer AO State Machine Implementation */
/*==========================================================================*/
static QState TimerAO_initial(TimerAO * const me, QEvt const * const e) {
    (void)e; /* Suppress unused parameter warning */

    /* Initialize the Timer AO */
    me->tick_count = 0U;
    me->start_time_ms = 0U;
    me->last_report_time = 0U;
    me->report_count = 0U;
    me->current_state = TIMER_STATE_STOPPED;
    me->is_running = RT_FALSE;

    /* Reduce logging during startup to prevent event queue overflow */
    /* SYS_LOG_D("TimerAO: Initial state entered"); */

    /* Subscribe to relevant signals */
    QActive_subscribe((QActive *)me, APP_START_SIG);
    QActive_subscribe((QActive *)me, APP_STOP_SIG);
    QActive_subscribe((QActive *)me, TIMER_START_SIG);
    QActive_subscribe((QActive *)me, TIMER_STOP_SIG);
    QActive_subscribe((QActive *)me, COUNTER_UPDATE_SIG);

    return Q_TRAN(&TimerAO_stopped);
}

static QState TimerAO_stopped(TimerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->is_running = RT_FALSE;
            me->current_state = TIMER_STATE_STOPPED;
            SYS_LOG_I("TimerAO: Stopped state entered");
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            /* Reduce startup logging to prevent queue overflow */
            /* SYS_LOG_D("TimerAO: Exiting stopped state"); */
            status = Q_HANDLED();
            break;
        }
        case APP_START_SIG:
        case TIMER_START_SIG: {
            SYS_LOG_I("TimerAO: Starting timer");
            status = Q_TRAN(&TimerAO_running);
            break;
        }
        case APP_STOP_SIG:
        case TIMER_STOP_SIG: {
            /* Already stopped, just acknowledge */
            SYS_LOG_D("TimerAO: Stop signal received while stopped");
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

static QState TimerAO_running(TimerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->is_running = RT_TRUE;
            me->current_state = TIMER_STATE_RUNNING;
            me->start_time_ms = BSP_getTimestampMs();
            me->last_report_time = me->start_time_ms;

            /* Start the tick timer (100ms intervals) */
            QTimeEvt_armX(&me->tickTimeEvt,
                         BSP_TICKS_PER_SEC / 10U,  /* 100ms */
                         BSP_TICKS_PER_SEC / 10U); /* Periodic */

            /* Start the report timer (1 second intervals) */
            QTimeEvt_armX(&me->reportTimeEvt,
                         BSP_TICKS_PER_SEC * TIMER_REPORT_INTERVAL_MS / 1000U,
                         BSP_TICKS_PER_SEC * TIMER_REPORT_INTERVAL_MS / 1000U);

            SYS_LOG_I("TimerAO: Running state entered, timers started");
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            /* Disarm all timers */
            QTimeEvt_disarm(&me->tickTimeEvt);
            QTimeEvt_disarm(&me->reportTimeEvt);
            me->is_running = RT_FALSE;
            SYS_LOG_I("TimerAO: Exiting running state, timers stopped");
            status = Q_HANDLED();
            break;
        }
        case TIMER_TICK_SIG: {
            /* Increment tick count */
            ++me->tick_count;

            /* Update global statistics */
            rt_mutex_take(g_stats_mutex, RT_WAITING_FOREVER);
            ++g_perf_stats.timer_ticks;
            g_perf_stats.test_duration_ms = TimerAO_getElapsedMs();
            rt_mutex_release(g_stats_mutex);

            /* Publish timer tick event */
            TimerTickEvt *tickEvt = Q_NEW(TimerTickEvt, TIMER_TICK_SIG);
            if (tickEvt != (TimerTickEvt *)0) {
                tickEvt->tick_count = me->tick_count;
                tickEvt->timestamp = BSP_getTimestampMs();
                QF_PUBLISH((QEvt *)tickEvt, (void *)0);
            }

            status = Q_HANDLED();
            break;
        }
        case TIMER_REPORT_SIG: {
            /* Transition to reporting state */
            status = Q_TRAN(&TimerAO_reporting);
            break;
        }
        case APP_STOP_SIG:
        case TIMER_STOP_SIG: {
            SYS_LOG_I("TimerAO: Stopping timer");
            status = Q_TRAN(&TimerAO_stopped);
            break;
        }
        case TIMER_TIMEOUT_SIG: {
            /* Test timeout - stop the timer */
            SYS_LOG_E("TimerAO: Test timeout reached");
            status = Q_TRAN(&TimerAO_stopped);
            break;
        }
        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}

static QState TimerAO_reporting(TimerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->current_state = TIMER_STATE_REPORTING;

            /* Generate report */
            uint32_t current_time = BSP_getTimestampMs();
            uint32_t elapsed_since_report = current_time - me->last_report_time;
            uint32_t counter_value = CounterAO_getValue();

            ++me->report_count;
            me->last_report_time = current_time;

            /* Update global statistics */
            rt_mutex_take(g_stats_mutex, RT_WAITING_FOREVER);
            ++g_perf_stats.timer_reports;
            g_perf_stats.test_duration_ms = TimerAO_getElapsedMs();
            rt_mutex_release(g_stats_mutex);

            /* Create and publish timer report event */
            TimerReportEvt *reportEvt = Q_NEW(TimerReportEvt, TIMER_REPORT_SIG);
            if (reportEvt != (TimerReportEvt *)0) {
                reportEvt->elapsed_ms = elapsed_since_report;
                reportEvt->tick_count = me->tick_count;
                reportEvt->counter_value = counter_value;
                QF_PUBLISH((QEvt *)reportEvt, (void *)0);
            }

            /* Log report with thread-safe logging */
            SYS_LOG_I("TimerAO: Report #%u - Elapsed: %u ms, Ticks: %u, Counter: %u",
                           me->report_count, elapsed_since_report, me->tick_count, counter_value);

            /* Immediately transition back to running */
            status = Q_TRAN(&TimerAO_running);
            break;
        }
        case Q_EXIT_SIG: {
            SYS_LOG_D("TimerAO: Exiting reporting state");
            status = Q_HANDLED();
            break;
        }
        case APP_STOP_SIG:
        case TIMER_STOP_SIG: {
            SYS_LOG_I("TimerAO: Stopping timer from reporting state");
            status = Q_TRAN(&TimerAO_stopped);
            break;
        }
        default: {
            /* Handle other events in parent state */
            status = Q_SUPER(&TimerAO_running);
            break;
        }
    }

    return status;
}
