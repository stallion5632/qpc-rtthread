#ifndef Q_TIMEOUT_SIG
#define Q_TIMEOUT_SIG 0xF0F0U
#endif
/*============================================================================
* Product: Counter Active Object Implementation
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
#include "counter_ao.h"
#include "bsp.h"
#include <rtthread.h>
#include "qpc.h"

/*==========================================================================*/
/* Global Counter AO Instance */
/*==========================================================================*/

/* Global Counter Active Object instance */
CounterAO g_counter;

/*==========================================================================*/
/* Local (Private) State Machine Functions */
/*==========================================================================*/

/* Forward declarations of state functions */
static QState CounterAO_initial(CounterAO * const me, void const * const par);
static QState CounterAO_idle(CounterAO * const me, QEvt const * const e);
static QState CounterAO_processing(CounterAO * const me, QEvt const * const e);
static QState CounterAO_error(CounterAO * const me, QEvt const * const e);
static QState CounterAO_reporting(CounterAO * const me, QEvt const * const e);

/*==========================================================================*/
/* Counter AO Constructor */
/*==========================================================================*/

void CounterAO_ctor(CounterAO *const me)
{
    /* Initialize the base QActive class */
    QActive_ctor(&me->super, Q_STATE_CAST(&CounterAO_initial));

    /* Initialize counter state variables to default values */
    me->current_value = COUNTER_DEFAULT_VALUE;
    me->increment_amount = 1U;                  /* Default increment amount */
    me->decrement_amount = 1U;                  /* Default decrement amount */
    me->operation_mode = COUNTER_MODE_NORMAL;   /* Default operation mode */

    /* Initialize performance tracking variables */
    me->total_operations = 0U;
    me->increment_operations = 0U;
    me->decrement_operations = 0U;
    me->reset_operations = 0U;

    /* Initialize timing and performance metrics */
    me->last_operation_time = 0U;
    me->min_operation_time = UINT32_MAX;        /* Start with maximum value */
    me->max_operation_time = 0U;
    me->avg_operation_time = 0U;

    /* Initialize error tracking counters */
    me->overflow_errors = 0U;
    me->underflow_errors = 0U;
    me->invalid_operation_errors = 0U;

    rt_kprintf("CounterAO: Constructor completed, ready for operation\n");
}

/*==========================================================================*/
/* Counter AO State Machine Implementation */
/*==========================================================================*/

/* Initial pseudostate - sets up subscriptions and transitions to idle */
static QState CounterAO_initial(CounterAO * const me, void const * const par)
{
    (void)par; /* Unused parameter */

    rt_kprintf("CounterAO: Entering initial state\n");

    /* Subscribe to relevant signals for counter operations */
    QActive_subscribe((QActive *)me, COUNTER_INCREMENT_SIG);
    QActive_subscribe((QActive *)me, COUNTER_DECREMENT_SIG);
    QActive_subscribe((QActive *)me, COUNTER_RESET_SIG);
    QActive_subscribe((QActive *)me, COUNTER_GET_VALUE_SIG);

    rt_kprintf("CounterAO: Subscribed to counter signals\n");

    /* Transition to idle state and wait for events */
    return Q_TRAN(&CounterAO_idle);
}

/* Idle state - waiting for counter operation events */
static QState CounterAO_idle(CounterAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("CounterAO: Entering idle state, ready for operations\n");
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("CounterAO: Exiting idle state\n");
            status = Q_HANDLED();
            break;
        }

        case COUNTER_INCREMENT_SIG: {
            rt_kprintf("CounterAO: Received INCREMENT signal\n");
            status = Q_TRAN(&CounterAO_processing);
            break;
        }

        case COUNTER_DECREMENT_SIG: {
            rt_kprintf("CounterAO: Received DECREMENT signal\n");
            status = Q_TRAN(&CounterAO_processing);
            break;
        }

        case COUNTER_RESET_SIG: {
            rt_kprintf("CounterAO: Received RESET signal\n");
            status = Q_TRAN(&CounterAO_processing);
            break;
        }

        case COUNTER_GET_VALUE_SIG: {
            rt_kprintf("CounterAO: Received GET_VALUE signal\n");
            status = Q_TRAN(&CounterAO_reporting);
            break;
        }

        default: {
            /* Handle unexpected signals gracefully */
            rt_kprintf("CounterAO: Unexpected signal %d in idle state\n", (int)e->sig);
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}

/* Processing state - handles counter operations with performance tracking */
static QState CounterAO_processing(CounterAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);
    uint32_t start_time, end_time, processing_time;
    CounterEvt const *evt;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("CounterAO: Entering processing state\n");
            start_time = BSP_getTimestamp();
            me->last_operation_time = start_time;
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("CounterAO: Exiting processing state\n");

            /* Calculate and update performance metrics */
            end_time = BSP_getTimestamp();
            processing_time = CounterAO_calculateProcessingTime(me->last_operation_time, end_time);

            /* Update timing statistics */
            if (processing_time < me->min_operation_time) {
                me->min_operation_time = processing_time;
            }
            if (processing_time > me->max_operation_time) {
                me->max_operation_time = processing_time;
            }

            /* Update average processing time using running average */
            if (me->total_operations > 0U) {
                me->avg_operation_time = ((me->avg_operation_time * (me->total_operations - 1U)) + processing_time) / me->total_operations;
            } else {
                me->avg_operation_time = processing_time;
            }

            status = Q_HANDLED();
            break;
        }

        case COUNTER_INCREMENT_SIG: {
            evt = (CounterEvt const *)e;
            rt_kprintf("CounterAO: Processing INCREMENT by %u\n", evt->value);

            /* Check for overflow before incrementing */
            if ((me->current_value + evt->value) > COUNTER_MAX_VALUE) {
                rt_kprintf("CounterAO: Overflow error, transitioning to error state\n");
                me->overflow_errors++;
                status = Q_TRAN(&CounterAO_error);
            } else {
                me->current_value += evt->value;
                me->increment_operations++;
                me->total_operations++;
                rt_kprintf("CounterAO: Counter incremented to %u\n", me->current_value);
                status = Q_TRAN(&CounterAO_idle);
            }
            break;
        }

        case COUNTER_DECREMENT_SIG: {
            evt = (CounterEvt const *)e;
            rt_kprintf("CounterAO: Processing DECREMENT by %u\n", evt->value);

            /* Check for underflow before decrementing */
            if (me->current_value < evt->value) {
                rt_kprintf("CounterAO: Underflow error, transitioning to error state\n");
                me->underflow_errors++;
                status = Q_TRAN(&CounterAO_error);
            } else {
                me->current_value -= evt->value;
                me->decrement_operations++;
                me->total_operations++;
                rt_kprintf("CounterAO: Counter decremented to %u\n", me->current_value);
                status = Q_TRAN(&CounterAO_idle);
            }
            break;
        }

        case COUNTER_RESET_SIG: {
            rt_kprintf("CounterAO: Processing RESET operation\n");
            me->current_value = COUNTER_DEFAULT_VALUE;
            me->reset_operations++;
            me->total_operations++;
            rt_kprintf("CounterAO: Counter reset to %u\n", me->current_value);
            status = Q_TRAN(&CounterAO_idle);
            break;
        }

        default: {
            rt_kprintf("CounterAO: Unexpected signal %d in processing state\n", (int)e->sig);
            me->invalid_operation_errors++;
            status = Q_TRAN(&CounterAO_error);
            break;
        }
    }

    return status;
}

/* Error state - handles error conditions and recovery */
static QState CounterAO_error(CounterAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("CounterAO: Entering error state\n");
            rt_kprintf("CounterAO: Error counts - Overflow: %u, Underflow: %u, Invalid: %u\n",
                       me->overflow_errors, me->underflow_errors, me->invalid_operation_errors);

            /* Auto-recovery: transition back to idle after error handling */
            static QEvt const recovery_evt = { Q_TIMEOUT_SIG, 0U, 0U };
            QActive_post_((QActive *)me, &recovery_evt, 0U, (void *)0);

            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("CounterAO: Exiting error state, recovering to normal operation\n");
            status = Q_HANDLED();
            break;
        }

        case Q_TIMEOUT_SIG: {
            rt_kprintf("CounterAO: Auto-recovery timeout, returning to idle state\n");
            status = Q_TRAN(&CounterAO_idle);
            break;
        }

        case COUNTER_RESET_SIG: {
            rt_kprintf("CounterAO: Reset received in error state, clearing errors\n");
            me->current_value = COUNTER_DEFAULT_VALUE;
            status = Q_TRAN(&CounterAO_idle);
            break;
        }

        default: {
            rt_kprintf("CounterAO: Signal %d ignored in error state\n", (int)e->sig);
            status = Q_HANDLED();
            break;
        }
    }

    return status;
}

/* Reporting state - handles statistics and value queries */
static QState CounterAO_reporting(CounterAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("CounterAO: Entering reporting state\n");

            /* Report current counter value and statistics */
            rt_kprintf("=== Counter Status Report ===\n");
            rt_kprintf("Current Value: %u\n", me->current_value);
            rt_kprintf("Operation Mode: %u\n", me->operation_mode);
            rt_kprintf("Total Operations: %u\n", me->total_operations);
            rt_kprintf("Increment Operations: %u\n", me->increment_operations);
            rt_kprintf("Decrement Operations: %u\n", me->decrement_operations);
            rt_kprintf("Reset Operations: %u\n", me->reset_operations);
            rt_kprintf("Min Processing Time: %u cycles\n", me->min_operation_time);
            rt_kprintf("Max Processing Time: %u cycles\n", me->max_operation_time);
            rt_kprintf("Avg Processing Time: %u cycles\n", me->avg_operation_time);
            rt_kprintf("Overflow Errors: %u\n", me->overflow_errors);
            rt_kprintf("Underflow Errors: %u\n", me->underflow_errors);
            rt_kprintf("Invalid Operation Errors: %u\n", me->invalid_operation_errors);
            rt_kprintf("=============================\n");

            /* Auto-transition back to idle after reporting */
            static QEvt const done_evt = { Q_TIMEOUT_SIG, 0U, 0U };
            QActive_post_((QActive *)me, &done_evt, 0U, (void *)0);

            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("CounterAO: Exiting reporting state\n");
            status = Q_HANDLED();
            break;
        }

        case Q_TIMEOUT_SIG: {
            rt_kprintf("CounterAO: Reporting complete, returning to idle\n");
            status = Q_TRAN(&CounterAO_idle);
            break;
        }

        default: {
            rt_kprintf("CounterAO: Signal %d handled in reporting state\n", (int)e->sig);
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}

/*==========================================================================*/
/* Counter AO Public Interface Functions */
/*==========================================================================*/

uint32_t CounterAO_getValue(CounterAO const *const me)
{
    return me->current_value;
}

void CounterAO_setMode(CounterAO *const me, uint8_t mode)
{
    if (mode <= COUNTER_MODE_PRECISE) {
        me->operation_mode = mode;
        rt_kprintf("CounterAO: Operation mode set to %u\n", mode);
    } else {
        rt_kprintf("CounterAO: Invalid operation mode %u\n", mode);
    }
}

uint8_t CounterAO_getMode(CounterAO const *const me)
{
    return me->operation_mode;
}

void CounterAO_getStats(CounterAO const *const me,
                        uint32_t *total_ops,
                        uint32_t *inc_ops,
                        uint32_t *dec_ops,
                        uint32_t *reset_ops)
{
    if (total_ops != (uint32_t *)0) *total_ops = me->total_operations;
    if (inc_ops != (uint32_t *)0) *inc_ops = me->increment_operations;
    if (dec_ops != (uint32_t *)0) *dec_ops = me->decrement_operations;
    if (reset_ops != (uint32_t *)0) *reset_ops = me->reset_operations;
}

void CounterAO_getPerformanceMetrics(CounterAO const *const me,
                                     uint32_t *min_time,
                                     uint32_t *max_time,
                                     uint32_t *avg_time)
{
    if (min_time != (uint32_t *)0) *min_time = me->min_operation_time;
    if (max_time != (uint32_t *)0) *max_time = me->max_operation_time;
    if (avg_time != (uint32_t *)0) *avg_time = me->avg_operation_time;
}

void CounterAO_getErrorStats(CounterAO const *const me,
                             uint32_t *overflow_errors,
                             uint32_t *underflow_errors,
                             uint32_t *invalid_errors)
{
    if (overflow_errors != (uint32_t *)0) *overflow_errors = me->overflow_errors;
    if (underflow_errors != (uint32_t *)0) *underflow_errors = me->underflow_errors;
    if (invalid_errors != (uint32_t *)0) *invalid_errors = me->invalid_operation_errors;
}

/*==========================================================================*/
/* Counter AO Utility Functions */
/*==========================================================================*/

rt_bool_t CounterAO_isValueValid(uint32_t value)
{
    return (value >= COUNTER_MIN_VALUE && value <= COUNTER_MAX_VALUE);
}

uint32_t CounterAO_calculateProcessingTime(uint32_t start_time, uint32_t end_time)
{
    /* Handle timer wraparound case */
    if (end_time >= start_time) {
        return (end_time - start_time);
    } else {
        return (UINT32_MAX - start_time + end_time + 1U);
    }
}

/*==========================================================================*/
/* MSH Shell Command Implementations */
/*==========================================================================*/

void counter_increment_cmd(void)
{
    static CounterEvt evt = { { COUNTER_INCREMENT_SIG, 0U, 0U }, 1U, 0U };
    evt.timestamp = BSP_getTimestamp();

    rt_kprintf("Sending counter increment command...\n");
    QActive_post_(&g_counter.super, &evt.super, 0U, (void *)0);
}

void counter_decrement_cmd(void)
{
    static CounterEvt evt = { { COUNTER_DECREMENT_SIG, 0U, 0U }, 1U, 0U };
    evt.timestamp = BSP_getTimestamp();

    rt_kprintf("Sending counter decrement command...\n");
    QActive_post_(&g_counter.super, &evt.super, 0U, (void *)0);
}

void counter_reset_cmd(void)
{
    static QEvt const evt = { COUNTER_RESET_SIG, 0U, 0U };

    rt_kprintf("Sending counter reset command...\n");
    QActive_post_(&g_counter.super, &evt, 0U, (void *)0);
}

void counter_get_cmd(void)
{
    static QEvt const evt = { COUNTER_GET_VALUE_SIG, 0U, 0U };

    rt_kprintf("Requesting counter value report...\n");
    QActive_post_(&g_counter.super, &evt, 0U, (void *)0);
}

void counter_stats_cmd(void)
{
    uint32_t total, inc, dec, reset;
    CounterAO_getStats(&g_counter, &total, &inc, &dec, &reset);

    rt_kprintf("=== Counter Statistics ===\n");
    rt_kprintf("Total Operations: %u\n", total);
    rt_kprintf("Increment Operations: %u\n", inc);
    rt_kprintf("Decrement Operations: %u\n", dec);
    rt_kprintf("Reset Operations: %u\n", reset);
    rt_kprintf("==========================\n");
}

void counter_perf_cmd(void)
{
    uint32_t min_time, max_time, avg_time;
    CounterAO_getPerformanceMetrics(&g_counter, &min_time, &max_time, &avg_time);

    rt_kprintf("=== Counter Performance Metrics ===\n");
    rt_kprintf("Minimum Processing Time: %u cycles\n", min_time);
    rt_kprintf("Maximum Processing Time: %u cycles\n", max_time);
    rt_kprintf("Average Processing Time: %u cycles\n", avg_time);
    rt_kprintf("===================================\n");
}

/* Export MSH commands for interactive testing */
MSH_CMD_EXPORT(counter_increment_cmd, Increment counter by 1);
MSH_CMD_EXPORT(counter_decrement_cmd, Decrement counter by 1);
MSH_CMD_EXPORT(counter_reset_cmd, Reset counter to 0);
MSH_CMD_EXPORT(counter_get_cmd, Get counter value and statistics);
MSH_CMD_EXPORT(counter_stats_cmd, Show counter operation statistics);
MSH_CMD_EXPORT(counter_perf_cmd, Show counter performance metrics);
