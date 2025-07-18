/*============================================================================
* Product: Counter Active Object Header
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
#ifndef COUNTER_AO_H_
#define COUNTER_AO_H_

#include "qpc.h"
#include "app_main.h"

/*==========================================================================*/
/* Counter Active Object Configuration */
/*==========================================================================*/

/* Counter AO priority - must be unique among all AOs */
#define COUNTER_AO_PRIO          3U

/* Counter value limits for safety and overflow prevention */
#define COUNTER_MAX_VALUE        1000000U   /* Maximum counter value */
#define COUNTER_MIN_VALUE        0U         /* Minimum counter value */
#define COUNTER_DEFAULT_VALUE    0U         /* Default starting value */

/* Counter operation modes */
#define COUNTER_MODE_NORMAL      0U         /* Normal counting mode */
#define COUNTER_MODE_FAST        1U         /* Fast counting mode */
#define COUNTER_MODE_PRECISE     2U         /* Precise counting mode */

/*==========================================================================*/
/* Counter Active Object Structure */
/*==========================================================================*/

/* Counter Active Object class definition */
typedef struct {
    QActive super;                          /* Base QActive class */
    
    /* Counter state variables */
    uint32_t current_value;                 /* Current counter value */
    uint32_t increment_amount;              /* Amount to increment per operation */
    uint32_t decrement_amount;              /* Amount to decrement per operation */
    uint8_t operation_mode;                 /* Current operation mode */
    
    /* Performance tracking variables */
    uint32_t total_operations;              /* Total operations performed */
    uint32_t increment_operations;          /* Number of increment operations */
    uint32_t decrement_operations;          /* Number of decrement operations */
    uint32_t reset_operations;              /* Number of reset operations */
    
    /* Timing and performance metrics */
    uint32_t last_operation_time;           /* Timestamp of last operation */
    uint32_t min_operation_time;            /* Minimum operation processing time */
    uint32_t max_operation_time;            /* Maximum operation processing time */
    uint32_t avg_operation_time;            /* Average operation processing time */
    
    /* Error tracking */
    uint32_t overflow_errors;               /* Number of overflow errors */
    uint32_t underflow_errors;              /* Number of underflow errors */
    uint32_t invalid_operation_errors;      /* Number of invalid operations */
    
} CounterAO;

/*==========================================================================*/
/* Counter States Enumeration */
/*==========================================================================*/

/* Counter AO state enumeration for state machine implementation */
typedef enum {
    COUNTER_STATE_INITIAL,                  /* Initial state before operation */
    COUNTER_STATE_IDLE,                     /* Idle state waiting for events */
    COUNTER_STATE_PROCESSING,               /* Processing counter operations */
    COUNTER_STATE_ERROR,                    /* Error state for invalid operations */
    COUNTER_STATE_REPORTING,                /* Reporting statistics state */
    COUNTER_STATE_MAX                       /* Maximum state value */
} CounterState;

/*==========================================================================*/
/* Counter Event Structures */
/*==========================================================================*/

/* Enhanced counter event with operation parameters */
typedef struct {
    QEvt super;                             /* Base QEvt structure */
    uint32_t value;                         /* Operation value parameter */
    uint32_t timestamp;                     /* Event timestamp */
    uint8_t operation_mode;                 /* Requested operation mode */
    uint8_t priority_level;                 /* Event priority level */
} CounterEvtEx;

/* Counter statistics event for reporting */
typedef struct {
    QEvt super;                             /* Base QEvt structure */
    uint32_t request_id;                    /* Statistics request identifier */
    rt_bool_t reset_after_report;           /* Reset statistics after reporting */
    uint32_t timestamp;                     /* Event timestamp */
} CounterStatsEvt;

/*==========================================================================*/
/* Global Counter AO Instance */
/*==========================================================================*/

/* Global Counter Active Object instance - accessible from other modules */
extern CounterAO g_counter;

/*==========================================================================*/
/* Counter AO Public Interface Functions */
/*==========================================================================*/

/* Constructor function to initialize Counter AO */
void CounterAO_ctor(CounterAO *const me);

/* Get current counter value - thread-safe read operation */
uint32_t CounterAO_getValue(CounterAO const *const me);

/* Set counter operation mode */
void CounterAO_setMode(CounterAO *const me, uint8_t mode);

/* Get counter operation mode */
uint8_t CounterAO_getMode(CounterAO const *const me);

/* Get counter statistics - thread-safe statistics read */
void CounterAO_getStats(CounterAO const *const me, 
                        uint32_t *total_ops,
                        uint32_t *inc_ops, 
                        uint32_t *dec_ops,
                        uint32_t *reset_ops);

/* Get counter performance metrics */
void CounterAO_getPerformanceMetrics(CounterAO const *const me,
                                     uint32_t *min_time,
                                     uint32_t *max_time,
                                     uint32_t *avg_time);

/* Get counter error statistics */
void CounterAO_getErrorStats(CounterAO const *const me,
                             uint32_t *overflow_errors,
                             uint32_t *underflow_errors,
                             uint32_t *invalid_errors);

/* Reset counter statistics and performance metrics */
void CounterAO_resetStats(CounterAO *const me);

/* Reset counter value to default */
void CounterAO_resetValue(CounterAO *const me);

/*==========================================================================*/
/* Counter AO Utility Functions */
/*==========================================================================*/

/* Validate counter value within acceptable range */
rt_bool_t CounterAO_isValueValid(uint32_t value);

/* Calculate operation processing time */
uint32_t CounterAO_calculateProcessingTime(uint32_t start_time, uint32_t end_time);

/* Format counter statistics for display */
void CounterAO_formatStats(CounterAO const *const me, char *buffer, rt_size_t buffer_size);

/*==========================================================================*/
/* MSH Shell Command Functions for Counter AO */
/*==========================================================================*/

/* MSH command to increment counter */
void counter_increment_cmd(void);

/* MSH command to decrement counter */
void counter_decrement_cmd(void);

/* MSH command to reset counter */
void counter_reset_cmd(void);

/* MSH command to get counter value */
void counter_get_cmd(void);

/* MSH command to show counter statistics */
void counter_stats_cmd(void);

/* MSH command to reset counter statistics */
void counter_reset_stats_cmd(void);

/* MSH command to set counter mode */
void counter_set_mode_cmd(int argc, char *argv[]);

/* MSH command to show counter performance metrics */
void counter_perf_cmd(void);

#endif /* COUNTER_AO_H_ */