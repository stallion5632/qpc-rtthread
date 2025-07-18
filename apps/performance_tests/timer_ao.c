/*============================================================================
* Product: Timer Active Object Implementation
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
#include "timer_ao.h"
#include "bsp.h"
#include <rtthread.h>

/*==========================================================================*/
/* Global Timer AO Instance */
/*==========================================================================*/

/* Global Timer Active Object instance */
TimerAO g_timer;

/*==========================================================================*/
/* Local (Private) State Machine Functions */
/*==========================================================================*/

/* Forward declarations of state functions */
static QState TimerAO_initial(TimerAO * const me, void const * const par);
static QState TimerAO_idle(TimerAO * const me, QEvt const * const e);
static QState TimerAO_running(TimerAO * const me, QEvt const * const e);
static QState TimerAO_processing_timeout(TimerAO * const me, QEvt const * const e);
static QState TimerAO_error(TimerAO * const me, QEvt const * const e);
// static QState TimerAO_reporting(TimerAO * const me, QEvt const * const e);

/*==========================================================================*/
/* Local Helper Functions */
/*==========================================================================*/

/* Initialize a timer instance to default values */
static void TimerAO_initTimerInstance(TimerInstance *timer) {
    timer->timer_id = 0U;
    timer->duration_ms = 0U;
    timer->start_time = 0U;
    timer->expected_end_time = 0U;
    timer->actual_end_time = 0U;
    timer->mode = TIMER_MODE_ONESHOT;
    timer->precision = TIMER_PRECISION_NORMAL;
    timer->is_active = RT_FALSE;
    timer->is_expired = RT_FALSE;
    timer->expiration_count = 0U;
}

/* Find an available timer slot */
static TimerInstance* TimerAO_findAvailableSlot(TimerAO *const me) {
    for (uint8_t i = 0U; i < TIMER_MAX_INSTANCES; i++) {
        if (!me->timers[i].is_active) {
            return &me->timers[i];
        }
    }
    return (TimerInstance *)0; /* No available slots */
}

/* Update timing error statistics */
static void TimerAO_updateTimingStats(TimerAO *const me, uint32_t timing_error) {
    /* Update min/max timing error */
    if (timing_error < me->min_timing_error) {
        me->min_timing_error = timing_error;
    }
    if (timing_error > me->max_timing_error) {
        me->max_timing_error = timing_error;
    }

    /* Update average timing error */
    me->total_timing_error += timing_error;
    if (me->timers_expired > 0U) {
        me->avg_timing_error = me->total_timing_error / me->timers_expired;
    }
}

/*==========================================================================*/
/* Timer AO Constructor */
/*==========================================================================*/

void TimerAO_ctor(TimerAO *const me)
{
    /* Initialize the base QActive class */
    QActive_ctor(&me->super, Q_STATE_CAST(&TimerAO_initial));

    /* Initialize all timer instances */
    for (uint8_t i = 0U; i < TIMER_MAX_INSTANCES; i++) {
        TimerAO_initTimerInstance(&me->timers[i]);
    }

    /* Initialize timer management variables */
    me->active_timer_count = 0U;
    me->next_timer_id = 1U;                     /* Start timer IDs from 1 */

    /* Initialize performance tracking variables */
    me->total_timer_operations = 0U;
    me->timers_started = 0U;
    me->timers_stopped = 0U;
    me->timers_expired = 0U;
    me->timers_reset = 0U;

    /* Initialize timing accuracy metrics */
    me->min_timing_error = UINT32_MAX;          /* Start with maximum value */
    me->max_timing_error = 0U;
    me->avg_timing_error = 0U;
    me->total_timing_error = 0U;

    /* Initialize default settings */
    me->default_precision = TIMER_PRECISION_NORMAL;
    me->default_mode = TIMER_MODE_ONESHOT;

    /* Initialize error tracking counters */
    me->timer_creation_errors = 0U;
    me->timer_overflow_errors = 0U;
    me->invalid_timer_errors = 0U;

    rt_kprintf("TimerAO: Constructor completed, ready for timer operations\n");
}

/*==========================================================================*/
/* Timer AO State Machine Implementation */
/*==========================================================================*/

/* Initial pseudostate - sets up subscriptions and transitions to idle */
static QState TimerAO_initial(TimerAO * const me, void const * const par)
{
    (void)par; /* Unused parameter */

    rt_kprintf("TimerAO: Entering initial state\n");

    /* Subscribe to relevant signals for timer operations */
    QActive_subscribe((QActive *)me, TIMER_START_SIG);
    QActive_subscribe((QActive *)me, TIMER_STOP_SIG);
    QActive_subscribe((QActive *)me, TIMER_TIMEOUT_SIG);
    QActive_subscribe((QActive *)me, TIMER_RESET_SIG);

    rt_kprintf("TimerAO: Subscribed to timer signals\n");

    /* Transition to idle state and wait for events */
    return Q_TRAN(&TimerAO_idle);
}

/* Idle state - waiting for timer operation events */
static QState TimerAO_idle(TimerAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("TimerAO: Entering idle state, ready for timer operations\n");
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("TimerAO: Exiting idle state\n");
            status = Q_HANDLED();
            break;
        }

        case TIMER_START_SIG: {
            TimerEvt const *evt = (TimerEvt const *)e;
            rt_kprintf("TimerAO: Received START signal for %u ms timer\n", evt->duration_ms);

            /* Validate timer duration */
            if (!TimerAO_isDurationValid(evt->duration_ms)) {
                rt_kprintf("TimerAO: Invalid timer duration %u ms\n", evt->duration_ms);
                me->invalid_timer_errors++;
                status = Q_TRAN(&TimerAO_error);
            } else {
                /* Find available timer slot */
                TimerInstance *timer = TimerAO_findAvailableSlot(me);
                if (timer != (TimerInstance *)0) {
                    /* Initialize timer instance */
                    timer->timer_id = me->next_timer_id++;
                    timer->duration_ms = evt->duration_ms;
                    timer->start_time = BSP_getTimestamp();
                    timer->expected_end_time = timer->start_time + (evt->duration_ms * BSP_getTicksPerMs());
                    timer->mode = (evt->timer_id == 0U) ? me->default_mode : (evt->timer_id & 0xFFU);
                    timer->precision = me->default_precision;
                    timer->is_active = RT_TRUE;
                    timer->is_expired = RT_FALSE;
                    timer->expiration_count = 0U;

                    me->active_timer_count++;
                    me->timers_started++;
                    me->total_timer_operations++;

                    rt_kprintf("TimerAO: Timer %u started (%u ms), transitioning to running state\n",
                               timer->timer_id, timer->duration_ms);
                    status = Q_TRAN(&TimerAO_running);
                } else {
                    rt_kprintf("TimerAO: No available timer slots, max instances reached\n");
                    me->timer_creation_errors++;
                    status = Q_TRAN(&TimerAO_error);
                }
            }
            break;
        }

        case TIMER_STOP_SIG:
        case TIMER_RESET_SIG:
        case TIMER_TIMEOUT_SIG: {
            rt_kprintf("TimerAO: Received timer operation signal %d in idle state\n", (int)e->sig);
            status = Q_HANDLED(); /* Ignore these signals when idle */
            break;
        }

        default: {
            rt_kprintf("TimerAO: Unexpected signal %d in idle state\n", (int)e->sig);
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}

/* Running state - managing active timers */
static QState TimerAO_running(TimerAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);
    uint32_t current_time;
    TimerInstance *timer;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("TimerAO: Entering running state with %u active timers\n", me->active_timer_count);

            /* Set up periodic checking for timer expirations */
            static QEvt const check_evt = { TIMER_TIMEOUT_SIG, 0U, 0U };
            QActive_post_((QActive *)me, &check_evt, 0U, (void *)0);

            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("TimerAO: Exiting running state\n");
            status = Q_HANDLED();
            break;
        }

        case TIMER_TIMEOUT_SIG: {
            /* Check for timer expirations */
            current_time = BSP_getTimestamp();
            rt_bool_t found_expired = RT_FALSE;

            for (uint8_t i = 0U; i < TIMER_MAX_INSTANCES; i++) {
                timer = &me->timers[i];
                if (timer->is_active && !timer->is_expired) {
                    if (current_time >= timer->expected_end_time) {
                        /* Timer has expired */
                        timer->actual_end_time = current_time;
                        timer->is_expired = RT_TRUE;
                        timer->expiration_count++;
                        found_expired = RT_TRUE;

                        /* Calculate timing error */
                        uint32_t timing_error = TimerAO_calculateTimingError(timer->expected_end_time, timer->actual_end_time);
                        TimerAO_updateTimingStats(me, timing_error);

                        rt_kprintf("TimerAO: Timer %u expired (error: %u cycles)\n", timer->timer_id, timing_error);

                        /* Handle periodic timers */
                        if (timer->mode == TIMER_MODE_PERIODIC) {
                            timer->start_time = current_time;
                            timer->expected_end_time = current_time + (timer->duration_ms * BSP_getTicksPerMs());
                            timer->is_expired = RT_FALSE;
                            rt_kprintf("TimerAO: Periodic timer %u restarted\n", timer->timer_id);
                        } else {
                            timer->is_active = RT_FALSE;
                            me->active_timer_count--;
                            me->timers_expired++;
                        }
                    }
                }
            }

            if (found_expired) {
                status = Q_TRAN(&TimerAO_processing_timeout);
            } else {
                /* Continue checking if there are active timers */
                if (me->active_timer_count > 0U) {
                    static QEvt const recheck_evt = { TIMER_TIMEOUT_SIG, 0U, 0U };
                    QActive_post_((QActive *)me, &recheck_evt, 0U, (void *)0);
                }
                status = Q_HANDLED();
            }
            break;
        }

        case TIMER_START_SIG: {
            /* Handle new timer start while running */
            TimerEvt const *evt = (TimerEvt const *)e;
            rt_kprintf("TimerAO: Starting additional timer for %u ms\n", evt->duration_ms);

            TimerInstance *new_timer = TimerAO_findAvailableSlot(me);
            if (new_timer != (TimerInstance *)0) {
                new_timer->timer_id = me->next_timer_id++;
                new_timer->duration_ms = evt->duration_ms;
                new_timer->start_time = BSP_getTimestamp();
                new_timer->expected_end_time = new_timer->start_time + (evt->duration_ms * BSP_getTicksPerMs());
                new_timer->mode = me->default_mode;
                new_timer->precision = me->default_precision;
                new_timer->is_active = RT_TRUE;
                new_timer->is_expired = RT_FALSE;
                new_timer->expiration_count = 0U;

                me->active_timer_count++;
                me->timers_started++;
                me->total_timer_operations++;

                rt_kprintf("TimerAO: Additional timer %u started\n", new_timer->timer_id);
            } else {
                rt_kprintf("TimerAO: Cannot start additional timer, no slots available\n");
                me->timer_creation_errors++;
            }
            status = Q_HANDLED();
            break;
        }

        case TIMER_STOP_SIG: {
            TimerEvt const *evt = (TimerEvt const *)e;
            timer = TimerAO_findTimer(me, evt->timer_id);
            if (timer != (TimerInstance *)0 && timer->is_active) {
                timer->is_active = RT_FALSE;
                me->active_timer_count--;
                me->timers_stopped++;
                me->total_timer_operations++;
                rt_kprintf("TimerAO: Timer %u stopped\n", evt->timer_id);

                /* Transition to idle if no more active timers */
                if (me->active_timer_count == 0U) {
                    status = Q_TRAN(&TimerAO_idle);
                } else {
                    status = Q_HANDLED();
                }
            } else {
                rt_kprintf("TimerAO: Timer %u not found or not active\n", evt->timer_id);
                status = Q_HANDLED();
            }
            break;
        }

        case TIMER_RESET_SIG: {
            TimerEvt const *evt = (TimerEvt const *)e;
            timer = TimerAO_findTimer(me, evt->timer_id);
            if (timer != (TimerInstance *)0 && timer->is_active) {
                timer->start_time = BSP_getTimestamp();
                timer->expected_end_time = timer->start_time + (timer->duration_ms * BSP_getTicksPerMs());
                timer->is_expired = RT_FALSE;
                me->timers_reset++;
                me->total_timer_operations++;
                rt_kprintf("TimerAO: Timer %u reset\n", evt->timer_id);
            } else {
                rt_kprintf("TimerAO: Timer %u not found or not active for reset\n", evt->timer_id);
            }
            status = Q_HANDLED();
            break;
        }

        default: {
            rt_kprintf("TimerAO: Unexpected signal %d in running state\n", (int)e->sig);
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}

/* Processing timeout state - handles timer expiration events */
static QState TimerAO_processing_timeout(TimerAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("TimerAO: Processing timer timeout events\n");

            /* Publish timeout events for expired timers */
            for (uint8_t i = 0U; i < TIMER_MAX_INSTANCES; i++) {
                TimerInstance *timer = &me->timers[i];
                if (timer->is_expired && timer->mode != TIMER_MODE_PERIODIC) {
                    static TimerTimeoutEvt timeout_evt;
                    timeout_evt.super.sig = TIMER_TIMEOUT_SIG;
                    timeout_evt.timer_id = timer->timer_id;
                    timeout_evt.actual_time = timer->actual_end_time;
                    timeout_evt.expected_time = timer->expected_end_time;
                    timeout_evt.timing_error = TimerAO_calculateTimingError(timer->expected_end_time, timer->actual_end_time);

                    /* Publish timeout event to interested subscribers */
                    QF_PUBLISH(&timeout_evt.super, (void *)0);
                    rt_kprintf("TimerAO: Published timeout event for timer %u\n", timer->timer_id);
                }
            }

            /* Auto-transition back to appropriate state */
            static QEvt const done_evt = { TIMER_TIMEOUT_SIG, 0U, 0U };
            QActive_post_((QActive *)me, &done_evt, 0U, (void *)0);

            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("TimerAO: Exiting timeout processing state\n");
            status = Q_HANDLED();
            break;
        }

        case TIMER_TIMEOUT_SIG: {
            /* Transition back to running if there are active timers, otherwise idle */
            if (me->active_timer_count > 0U) {
                rt_kprintf("TimerAO: Timeout processing complete, returning to running state\n");
                status = Q_TRAN(&TimerAO_running);
            } else {
                rt_kprintf("TimerAO: Timeout processing complete, returning to idle state\n");
                status = Q_TRAN(&TimerAO_idle);
            }
            break;
        }

        default: {
            rt_kprintf("TimerAO: Signal %d handled in timeout processing state\n", (int)e->sig);
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}

/* Error state - handles error conditions and recovery */
static QState TimerAO_error(TimerAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("TimerAO: Entering error state\n");
            rt_kprintf("TimerAO: Error counts - Creation: %u, Overflow: %u, Invalid: %u\n",
                       me->timer_creation_errors, me->timer_overflow_errors, me->invalid_timer_errors);

            /* Auto-recovery: transition back to appropriate state after error handling */
            static QEvt const recovery_evt = { TIMER_TIMEOUT_SIG, 0U, 0U };
            QActive_post_((QActive *)me, &recovery_evt, 0U, (void *)0);

            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("TimerAO: Exiting error state, recovering to normal operation\n");
            status = Q_HANDLED();
            break;
        }

        case TIMER_TIMEOUT_SIG: {
            /* Recovery: return to appropriate state based on active timers */
            if (me->active_timer_count > 0U) {
                rt_kprintf("TimerAO: Auto-recovery to running state\n");
                status = Q_TRAN(&TimerAO_running);
            } else {
                rt_kprintf("TimerAO: Auto-recovery to idle state\n");
                status = Q_TRAN(&TimerAO_idle);
            }
            break;
        }

        case TIMER_RESET_SIG: {
            rt_kprintf("TimerAO: Reset received in error state, clearing errors\n");
            /* Stop all timers and clear error state */
            TimerAO_stopAllTimers(me);
            status = Q_TRAN(&TimerAO_idle);
            break;
        }

        default: {
            rt_kprintf("TimerAO: Signal %d ignored in error state\n", (int)e->sig);
            status = Q_HANDLED();
            break;
        }
    }

    return status;
}

#if 0

/* Reporting state - handles statistics and status queries */
static QState TimerAO_reporting(TimerAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("TimerAO: Entering reporting state\n");

            /* Report timer status and statistics */
            rt_kprintf("=== Timer Status Report ===\n");
            rt_kprintf("Active Timers: %u/%u\n", me->active_timer_count, TIMER_MAX_INSTANCES);
            rt_kprintf("Next Timer ID: %u\n", me->next_timer_id);
            rt_kprintf("Total Operations: %u\n", me->total_timer_operations);
            rt_kprintf("Timers Started: %u\n", me->timers_started);
            rt_kprintf("Timers Stopped: %u\n", me->timers_stopped);
            rt_kprintf("Timers Expired: %u\n", me->timers_expired);
            rt_kprintf("Timers Reset: %u\n", me->timers_reset);
            rt_kprintf("Min Timing Error: %u cycles\n", me->min_timing_error);
            rt_kprintf("Max Timing Error: %u cycles\n", me->max_timing_error);
            rt_kprintf("Avg Timing Error: %u cycles\n", me->avg_timing_error);
            rt_kprintf("Creation Errors: %u\n", me->timer_creation_errors);
            rt_kprintf("Overflow Errors: %u\n", me->timer_overflow_errors);
            rt_kprintf("Invalid Errors: %u\n", me->invalid_timer_errors);

            /* List active timers */
            rt_kprintf("\n--- Active Timers ---\n");
            for (uint8_t i = 0U; i < TIMER_MAX_INSTANCES; i++) {
                TimerInstance *timer = &me->timers[i];
                if (timer->is_active) {
                    uint32_t current_time = BSP_getTimestamp();
                    uint32_t remaining_time = (timer->expected_end_time > current_time) ?
                                              (timer->expected_end_time - current_time) : 0U;
                    rt_kprintf("Timer %u: %u ms, remaining: %u cycles, mode: %u\n",
                               timer->timer_id, timer->duration_ms, remaining_time, timer->mode);
                }
            }
            rt_kprintf("====================\n");

            /* Auto-transition back to appropriate state after reporting */
            static QEvt const done_evt = { TIMER_TIMEOUT_SIG, 0U, 0U };
            QActive_post_((QActive *)me, &done_evt, 0U, (void *)0);

            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("TimerAO: Exiting reporting state\n");
            status = Q_HANDLED();
            break;
        }

        case TIMER_TIMEOUT_SIG: {
            /* Return to appropriate state based on active timers */
            if (me->active_timer_count > 0U) {
                rt_kprintf("TimerAO: Reporting complete, returning to running state\n");
                status = Q_TRAN(&TimerAO_running);
            } else {
                rt_kprintf("TimerAO: Reporting complete, returning to idle state\n");
                status = Q_TRAN(&TimerAO_idle);
            }
            break;
        }

        default: {
            rt_kprintf("TimerAO: Signal %d handled in reporting state\n", (int)e->sig);
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}
#endif

/*==========================================================================*/
/* Timer AO Public Interface Functions */
/*==========================================================================*/

uint32_t TimerAO_startTimer(TimerAO *const me, uint32_t duration_ms, uint8_t mode, uint8_t precision)
{
    if (!TimerAO_isDurationValid(duration_ms)) {
        return 0U; /* Invalid duration */
    }

    TimerInstance *timer = TimerAO_findAvailableSlot(me);
    if (timer == (TimerInstance *)0) {
        me->timer_creation_errors++;
        return 0U; /* No available slots */
    }

    static TimerEvt start_evt;
    start_evt.super.sig = TIMER_START_SIG;
    start_evt.duration_ms = duration_ms;
    start_evt.timer_id = me->next_timer_id;
    start_evt.timestamp = BSP_getTimestamp();

    QActive_post_(&me->super, &start_evt.super, 0U, (void *)0);

    return me->next_timer_id; /* Return the timer ID that will be assigned */
}

rt_bool_t TimerAO_stopTimer(TimerAO *const me, uint32_t timer_id)
{
    TimerInstance *timer = TimerAO_findTimer(me, timer_id);
    if (timer == (TimerInstance *)0 || !timer->is_active) {
        return RT_FALSE;
    }

    static TimerEvt stop_evt;
    stop_evt.super.sig = TIMER_STOP_SIG;
    stop_evt.timer_id = timer_id;
    stop_evt.timestamp = BSP_getTimestamp();

    QActive_post_(&me->super, &stop_evt.super, 0U, (void *)0);
    return RT_TRUE;
}

/* Stop all timers in the Timer AO */
void TimerAO_stopAllTimers(TimerAO *const me)
{
    rt_kprintf("TimerAO: Stopping all active timers\n");
    for (uint8_t i = 0U; i < TIMER_MAX_INSTANCES; i++) {
        if (me->timers[i].is_active) {
            TimerEvt stop_evt;
            stop_evt.super.sig = TIMER_STOP_SIG;
            stop_evt.timer_id = me->timers[i].timer_id;
            stop_evt.timestamp = BSP_getTimestamp();

            QActive_post_(&me->super, &stop_evt.super, 0U, (void *)0);
        }
    }
}

/* Check if the timer duration is valid (non-zero and within acceptable range) */
rt_bool_t TimerAO_isDurationValid(uint32_t duration_ms) {
    return (duration_ms > 0U && duration_ms <= TIMER_MAX_DURATION_MS);
}

/* Find a timer instance by ID */
TimerInstance* TimerAO_findTimer(TimerAO *const me, uint32_t timer_id) {
    for (uint8_t i = 0U; i < TIMER_MAX_INSTANCES; i++) {
        if (me->timers[i].is_active && me->timers[i].timer_id == timer_id) {
            return &me->timers[i];
        }
    }
    return (TimerInstance *)0; /* Not found */
}

/* Calculate the timing error between expected and actual end times */
uint32_t TimerAO_calculateTimingError(uint32_t expected_time, uint32_t actual_time) {
    if (actual_time >= expected_time) {
        return actual_time - expected_time;
    } else {
        /* Underflow, return maximum error */
        return UINT32_MAX - expected_time + actual_time;
    }
}

/*==========================================================================*/
/* BSP (Board Support Package) Interface - to be implemented in BSP layer */
/*==========================================================================*/

/* BSP_getTimestamp and BSP_getTicksPerMs are implemented in bsp.c, remove duplicates */
//#if 0
//uint32_t BSP_getTimestamp(void) {
//    return rt_tick_get() * (1000 / RT_TICK_PER_SECOND);
//}
//uint32_t BSP_getTicksPerMs(void) {
//    return RT_TICK_PER_SECOND / 1000;
//}
//#endif
