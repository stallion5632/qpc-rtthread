/*============================================================================
* Product: Timer Active Object Header
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
#ifndef TIMER_AO_H_
#define TIMER_AO_H_

#include "app_main.h"

/*==========================================================================*/
/* Timer AO Configuration */
/*==========================================================================*/
#define TIMER_QUEUE_SIZE        (128U)     /* Event queue size */
#define TIMER_STACK_SIZE        (2048U)    /* Stack size in bytes */

/*==========================================================================*/
/* Timer AO States */
/*==========================================================================*/
typedef enum {
    TIMER_STATE_STOPPED,
    TIMER_STATE_RUNNING,
    TIMER_STATE_REPORTING
} TimerState;

/*==========================================================================*/
/* Timer AO State Machine */
/*==========================================================================*/
typedef struct {
    QActive super;              /* Inherit from QActive */
    QTimeEvt tickTimeEvt;      /* Time event for tick generation */
    QTimeEvt reportTimeEvt;    /* Time event for periodic reporting */
    uint32_t tick_count;        /* Current tick count */
    uint32_t start_time_ms;     /* Test start timestamp */
    uint32_t last_report_time;  /* Last report timestamp */
    uint32_t report_count;      /* Number of reports generated */
    TimerState current_state;   /* Current state */
    rt_bool_t is_running;      /* Running state flag */
} TimerAO;

/*==========================================================================*/
/* Timer AO Public Interface */
/*==========================================================================*/

/* Constructor */
void TimerAO_ctor(void);

/* Get the singleton instance */
TimerAO *TimerAO_getInstance(void);

/* Get current tick count (thread-safe) */
uint32_t TimerAO_getTickCount(void);

/* Get elapsed time in milliseconds */
uint32_t TimerAO_getElapsedMs(void);

/* Get report count */
uint32_t TimerAO_getReportCount(void);

/* Check if timer is running */
rt_bool_t TimerAO_isRunning(void);

/* Get current state */
TimerState TimerAO_getCurrentState(void);

#endif /* TIMER_AO_H_ */
