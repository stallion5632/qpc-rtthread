/*============================================================================
* Product: Counter Active Object Header
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
#ifndef COUNTER_AO_H_
#define COUNTER_AO_H_

#include "app_main.h"

/*==========================================================================*/
/* Counter AO Configuration */
/*==========================================================================*/
#define COUNTER_QUEUE_SIZE      (16U)     /* Event queue size */
#define COUNTER_STACK_SIZE      (512U)    /* Stack size in bytes */

/*==========================================================================*/
/* Counter AO State Machine */
/*==========================================================================*/
typedef struct {
    QActive super;              /* Inherit from QActive */
    QTimeEvt timeEvt;          /* Time event for periodic updates */
    uint32_t counter_value;     /* Current counter value */
    uint32_t update_count;      /* Number of updates performed */
    rt_bool_t is_running;      /* Running state flag */
} CounterAO;

/*==========================================================================*/
/* Counter AO Public Interface */
/*==========================================================================*/

/* Constructor */
void CounterAO_ctor(void);

/* Get the singleton instance */
CounterAO *CounterAO_getInstance(void);

/* Get current counter value (thread-safe) */
uint32_t CounterAO_getValue(void);

/* Get update count (thread-safe) */
uint32_t CounterAO_getUpdateCount(void);

/* Check if counter is running */
rt_bool_t CounterAO_isRunning(void);

#endif /* COUNTER_AO_H_ */