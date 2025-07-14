/*============================================================================
* Product: Performance Tests Application - Signal Definitions
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
#ifndef APP_SIGNALS_H_
#define APP_SIGNALS_H_

#include "qpc.h"

enum PerformanceAppSignals {
    /* Performance Test Control Signals */
    PERF_TEST_START_SIG = Q_USER_SIG,
    PERF_TEST_STOP_SIG,
    PERF_TEST_REPORT_SIG,

    /* Timer Signals */
    TIMER_START_SIG,
    TIMER_STOP_SIG,
    TIMER_TICK_SIG,
    TIMER_REPORT_SIG,
    TIMER_TIMEOUT_SIG,

    /* Counter Signals */
    COUNTER_START_SIG,
    COUNTER_STOP_SIG,
    COUNTER_UPDATE_SIG,
    COUNTER_TIMEOUT_SIG,

    /* Application control signals */
    APP_START_SIG,
    APP_STOP_SIG,
    APP_RESET_SIG,

    MAX_PERF_APP_SIG
};

#endif /* APP_SIGNALS_H_ */