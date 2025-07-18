/*============================================================================
* Product: Performance Tests Framework - Main Header
* Last updated for version 7.3.0
* Last updated on  2025-07-18
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2025 Quantum Leaps, LLC. All rights reserved.
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
#ifndef APP_MAIN_H_
#define APP_MAIN_H_

#include "qpc.h"
#include <rtthread.h>

/*==========================================================================*/
/* Performance Test Framework Constants */
/*==========================================================================*/
#define PERF_FRAMEWORK_VERSION      "7.3.0"

/*==========================================================================*/
/* Performance Test Signals (for legacy test compatibility) */
/*==========================================================================*/
enum PerformanceAppSignals {
    /* Counter AO signals */
    COUNTER_START_SIG = Q_USER_SIG,
    COUNTER_STOP_SIG,
    COUNTER_UPDATE_SIG,
    COUNTER_TIMEOUT_SIG,

    /* Timer AO signals */
    TIMER_START_SIG,
    TIMER_STOP_SIG,
    TIMER_TICK_SIG,
    TIMER_REPORT_SIG,
    TIMER_TIMEOUT_SIG,

    /* Application control signals */
    APP_START_SIG,
    APP_STOP_SIG,
    APP_RESET_SIG,

    MAX_PERF_APP_SIG
};

/*==========================================================================*/
/* Performance Test Events */
/*==========================================================================*/

/*==========================================================================*/
/* Global Synchronization Objects */
/*==========================================================================*/
extern rt_mutex_t g_log_mutex;    /* Mutex for thread-safe logging */
extern rt_mutex_t g_stats_mutex;  /* Mutex for statistics access */

/*==========================================================================*/
/* Performance Test Framework API */
/*==========================================================================*/
void PerformanceFramework_init(void);

#endif /* APP_MAIN_H_ */
