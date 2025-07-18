/*============================================================================
* Product: Timer Active Object Header
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
#ifndef TIMER_AO_H_
#define TIMER_AO_H_

#include "qpc.h"
#include "app_main.h"

/*==========================================================================*/
/* Timer Active Object Configuration */
/*==========================================================================*/

/* Timer AO priority - must be unique among all AOs */
#define TIMER_AO_PRIO            4U

/* Timer configuration constants */
#define TIMER_MAX_DURATION_MS    60000U     /* Maximum timer duration: 60 seconds */
#define TIMER_MIN_DURATION_MS    1U         /* Minimum timer duration: 1 ms */
#define TIMER_DEFAULT_DURATION   1000U     /* Default timer duration: 1 second */
#define TIMER_MAX_INSTANCES      8U         /* Maximum concurrent timer instances */

/* Timer precision modes */
#define TIMER_PRECISION_NORMAL   0U         /* Normal precision timing */
#define TIMER_PRECISION_HIGH     1U         /* High precision timing */
#define TIMER_PRECISION_ULTRA    2U         /* Ultra-high precision timing */

/* Timer operation modes */
#define TIMER_MODE_ONESHOT       0U         /* One-shot timer mode */
#define TIMER_MODE_PERIODIC      1U         /* Periodic timer mode */
#define TIMER_MODE_BURST         2U         /* Burst timer mode */

/*==========================================================================*/
/* Timer Instance Structure */
/*==========================================================================*/

/* Individual timer instance data structure */
typedef struct {
    uint32_t timer_id;                      /* Unique timer identifier */
    uint32_t duration_ms;                   /* Timer duration in milliseconds */
    uint32_t start_time;                    /* Timer start timestamp */
    uint32_t expected_end_time;             /* Expected completion timestamp */
    uint32_t actual_end_time;               /* Actual completion timestamp */
    uint8_t mode;                           /* Timer operation mode */
    uint8_t precision;                      /* Timer precision level */
    rt_bool_t is_active;                    /* Timer active status flag */
    rt_bool_t is_expired;                   /* Timer expiration status flag */
    uint32_t expiration_count;              /* Number of expirations (for periodic) */
} TimerInstance;

/*==========================================================================*/
/* Timer Active Object Structure */
/*==========================================================================*/

/* Timer Active Object class definition */
typedef struct {
    QActive super;                          /* Base QActive class */
    
    /* Timer management */
    TimerInstance timers[TIMER_MAX_INSTANCES]; /* Array of timer instances */
    uint8_t active_timer_count;             /* Number of currently active timers */
    uint32_t next_timer_id;                 /* Next available timer ID */
    
    /* Performance tracking variables */
    uint32_t total_timer_operations;        /* Total timer operations performed */
    uint32_t timers_started;                /* Number of timers started */
    uint32_t timers_stopped;                /* Number of timers stopped */
    uint32_t timers_expired;                /* Number of timers that expired */
    uint32_t timers_reset;                  /* Number of timers reset */
    
    /* Timing accuracy metrics */
    uint32_t min_timing_error;              /* Minimum timing error in cycles */
    uint32_t max_timing_error;              /* Maximum timing error in cycles */
    uint32_t avg_timing_error;              /* Average timing error in cycles */
    uint32_t total_timing_error;            /* Cumulative timing error */
    
    /* Timer precision and performance settings */
    uint8_t default_precision;              /* Default timer precision mode */
    uint8_t default_mode;                   /* Default timer operation mode */
    
    /* Error tracking */
    uint32_t timer_creation_errors;         /* Timer creation failure count */
    uint32_t timer_overflow_errors;         /* Timer overflow error count */
    uint32_t invalid_timer_errors;          /* Invalid timer operation count */
    
} TimerAO;

/*==========================================================================*/
/* Timer States Enumeration */
/*==========================================================================*/

/* Timer AO state enumeration for state machine implementation */
typedef enum {
    TIMER_STATE_INITIAL,                    /* Initial state before operation */
    TIMER_STATE_IDLE,                       /* Idle state waiting for commands */
    TIMER_STATE_RUNNING,                    /* Running state with active timers */
    TIMER_STATE_PROCESSING_TIMEOUT,         /* Processing timer timeout events */
    TIMER_STATE_ERROR,                      /* Error state for invalid operations */
    TIMER_STATE_REPORTING,                  /* Reporting timer statistics */
    TIMER_STATE_MAX                         /* Maximum state value */
} TimerState;

/*==========================================================================*/
/* Timer Event Structures */
/*==========================================================================*/

/* Enhanced timer event with operation parameters */
typedef struct {
    QEvt super;                             /* Base QEvt structure */
    uint32_t duration_ms;                   /* Timer duration in milliseconds */
    uint32_t timer_id;                      /* Timer identifier */
    uint32_t timestamp;                     /* Event timestamp */
    uint8_t mode;                           /* Timer operation mode */
    uint8_t precision;                      /* Timer precision level */
} TimerEvtEx;

/* Timer timeout event structure */
typedef struct {
    QEvt super;                             /* Base QEvt structure */
    uint32_t timer_id;                      /* Timer that expired */
    uint32_t actual_time;                   /* Actual expiration time */
    uint32_t expected_time;                 /* Expected expiration time */
    uint32_t timing_error;                  /* Timing error in cycles */
} TimerTimeoutEvt;

/* Timer statistics request event */
typedef struct {
    QEvt super;                             /* Base QEvt structure */
    uint32_t request_id;                    /* Statistics request identifier */
    rt_bool_t include_active_timers;        /* Include active timer details */
    rt_bool_t reset_after_report;           /* Reset statistics after reporting */
} TimerStatsEvt;

/*==========================================================================*/
/* Global Timer AO Instance */
/*==========================================================================*/

/* Global Timer Active Object instance - accessible from other modules */
extern TimerAO g_timer;

/*==========================================================================*/
/* Timer AO Public Interface Functions */
/*==========================================================================*/

/* Constructor function to initialize Timer AO */
void TimerAO_ctor(TimerAO *const me);

/* Start a new timer with specified parameters */
uint32_t TimerAO_startTimer(TimerAO *const me, uint32_t duration_ms, uint8_t mode, uint8_t precision);

/* Stop a specific timer by ID */
rt_bool_t TimerAO_stopTimer(TimerAO *const me, uint32_t timer_id);

/* Reset a specific timer by ID */
rt_bool_t TimerAO_resetTimer(TimerAO *const me, uint32_t timer_id);

/* Stop all active timers */
void TimerAO_stopAllTimers(TimerAO *const me);

/* Get timer status by ID */
rt_bool_t TimerAO_getTimerStatus(TimerAO const *const me, uint32_t timer_id, TimerInstance *status);

/* Get number of active timers */
uint8_t TimerAO_getActiveTimerCount(TimerAO const *const me);

/* Get timer operation statistics */
void TimerAO_getStats(TimerAO const *const me, 
                      uint32_t *total_ops,
                      uint32_t *started,
                      uint32_t *stopped,
                      uint32_t *expired,
                      uint32_t *reset);

/* Get timer timing accuracy metrics */
void TimerAO_getTimingMetrics(TimerAO const *const me,
                              uint32_t *min_error,
                              uint32_t *max_error,
                              uint32_t *avg_error);

/* Get timer error statistics */
void TimerAO_getErrorStats(TimerAO const *const me,
                           uint32_t *creation_errors,
                           uint32_t *overflow_errors,
                           uint32_t *invalid_errors);

/* Reset timer statistics and performance metrics */
void TimerAO_resetStats(TimerAO *const me);

/* Set default timer parameters */
void TimerAO_setDefaults(TimerAO *const me, uint8_t mode, uint8_t precision);

/*==========================================================================*/
/* Timer AO Utility Functions */
/*==========================================================================*/

/* Validate timer duration within acceptable range */
rt_bool_t TimerAO_isDurationValid(uint32_t duration_ms);

/* Calculate timing error between expected and actual times */
uint32_t TimerAO_calculateTimingError(uint32_t expected_time, uint32_t actual_time);

/* Find timer instance by ID */
TimerInstance* TimerAO_findTimer(TimerAO *const me, uint32_t timer_id);

/* Format timer statistics for display */
void TimerAO_formatStats(TimerAO const *const me, char *buffer, rt_size_t buffer_size);

/* Format active timers list for display */
void TimerAO_formatActiveTimers(TimerAO const *const me, char *buffer, rt_size_t buffer_size);

/*==========================================================================*/
/* MSH Shell Command Functions for Timer AO */
/*==========================================================================*/

/* MSH command to start a timer */
void timer_start_cmd(int argc, char *argv[]);

/* MSH command to stop a timer */
void timer_stop_cmd(int argc, char *argv[]);

/* MSH command to reset a timer */
void timer_reset_cmd(int argc, char *argv[]);

/* MSH command to stop all timers */
void timer_stop_all_cmd(void);

/* MSH command to list active timers */
void timer_list_cmd(void);

/* MSH command to show timer statistics */
void timer_stats_cmd(void);

/* MSH command to reset timer statistics */
void timer_reset_stats_cmd(void);

/* MSH command to show timer performance metrics */
void timer_perf_cmd(void);

/* MSH command to set timer defaults */
void timer_set_defaults_cmd(int argc, char *argv[]);

#endif /* TIMER_AO_H_ */