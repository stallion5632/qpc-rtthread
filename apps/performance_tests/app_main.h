/*============================================================================
* Product: Performance Tests Application - Main Header
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
#ifndef APP_MAIN_H_
#define APP_MAIN_H_

#include "qpc.h"
#include <rtthread.h>

/*==========================================================================*/
/* Performance Test Application Constants */
/*==========================================================================*/
#define PERF_TEST_TIMEOUT_MS        (10000U)  /* 10 second test duration */
#define COUNTER_UPDATE_INTERVAL_MS  (100U)    /* Counter update every 100ms */
#define TIMER_REPORT_INTERVAL_MS    (1000U)   /* Timer report every 1 second */
#define LOG_BUFFER_SIZE             (128U)    /* Log message buffer size */

/*==========================================================================*/
/* Performance Test Signals */
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
    
    /* Logger AO signals */
    LOGGER_LOG_SIG,
    LOGGER_FLUSH_SIG,
    LOGGER_TIMEOUT_SIG,
    
    /* Application control signals */
    APP_START_SIG,
    APP_STOP_SIG,
    APP_RESET_SIG,
    
    MAX_PERF_APP_SIG
};

/*==========================================================================*/
/* Performance Test Events */
/*==========================================================================*/

/* Counter update event */
typedef struct {
    QEvt super;
    uint32_t counter_value;
    uint32_t timestamp;
} CounterUpdateEvt;

/* Timer tick event */
typedef struct {
    QEvt super;
    uint32_t tick_count;
    uint32_t timestamp;
} TimerTickEvt;

/* Timer report event */
typedef struct {
    QEvt super;
    uint32_t elapsed_ms;
    uint32_t tick_count;
    uint32_t counter_value;
} TimerReportEvt;

/* Log event */
typedef struct {
    QEvt super;
    char message[LOG_BUFFER_SIZE];
    uint32_t timestamp;
    uint8_t log_level;
} LogEvt;

/*==========================================================================*/
/* Performance Test Priorities */
/*==========================================================================*/
enum PerformanceAppPriorities {
    COUNTER_AO_PRIO = 1U,
    TIMER_AO_PRIO   = 2U,
    LOGGER_AO_PRIO  = 3U
};

/*==========================================================================*/
/* Active Object Handles */
/*==========================================================================*/
extern QActive * const AO_Counter;
extern QActive * const AO_Timer;
extern QActive * const AO_Logger;

/*==========================================================================*/
/* Global Synchronization Objects */
/*==========================================================================*/
extern rt_mutex_t g_log_mutex;      /* Mutex for thread-safe logging */
extern rt_mutex_t g_stats_mutex;    /* Mutex for statistics access */

/*==========================================================================*/
/* Shared Statistics Structure */
/*==========================================================================*/
typedef struct {
    volatile uint32_t counter_updates;
    volatile uint32_t timer_ticks;
    volatile uint32_t timer_reports;
    volatile uint32_t log_messages;
    volatile uint32_t test_duration_ms;
    volatile rt_bool_t test_running;
} PerformanceStats;

extern PerformanceStats g_perf_stats;

/*==========================================================================*/
/* Application Functions */
/*==========================================================================*/

/* Initialize the performance test application */
void PerformanceApp_init(void);

/* Start the performance test */
int PerformanceApp_start(void);

/* Stop the performance test */
void PerformanceApp_stop(void);

/* Get current performance statistics */
void PerformanceApp_getStats(PerformanceStats *stats);

/* Reset performance statistics */
void PerformanceApp_resetStats(void);

/* Check if QF framework is already initialized */
rt_bool_t PerformanceApp_isQFInitialized(void);

/* Check if Active Objects are already started */
rt_bool_t PerformanceApp_areAOsStarted(void);

/*==========================================================================*/
/* BSP Function Declarations */
/*==========================================================================*/
void BSP_init(void);
uint32_t BSP_getTimestampMs(void);
void BSP_ledOn(void);
void BSP_ledOff(void);
void BSP_ledToggle(void);

/*==========================================================================*/
/* RT-Thread MSH Command Prototypes */
/*==========================================================================*/
int perf_test_start_cmd(int argc, char** argv);
int perf_test_stop_cmd(int argc, char** argv);
int perf_test_stats_cmd(int argc, char** argv);
int perf_test_reset_cmd(int argc, char** argv);

#endif /* APP_MAIN_H_ */