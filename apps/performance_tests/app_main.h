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

    /* Application control signals */
    APP_START_SIG,
    APP_STOP_SIG,
    APP_RESET_SIG,

    MAX_PERF_APP_SIG
};

/*==========================================================================*/
/* Performance Test Events */
/*==========================================================================*/

/* Structure for counter update event */
typedef struct {
    QEvt super;           /* Base event structure */
    uint32_t counter_value; /* Value of the counter */
    uint32_t timestamp;     /* Timestamp of the event (ms) */
    uint32_t margin;
} CounterUpdateEvt;

/* Structure for timer tick event */
typedef struct {
    QEvt super;           /* Base event structure */
    uint32_t tick_count;  /* Number of timer ticks */
    uint32_t timestamp;   /* Timestamp of the event (ms) */
    uint32_t margin;
} TimerTickEvt;

/* Structure for timer report event */
typedef struct {
    QEvt super;           /* Base event structure */
    uint32_t elapsed_ms;  /* Elapsed time in milliseconds */
    uint32_t tick_count;  /* Number of timer ticks */
    uint32_t counter_value; /* Value of the counter at report */
} TimerReportEvt;

/* Enumeration for performance test AO priorities */
enum PerformanceAppPriorities {
    TIMER_AO_PRIO   = 30U, /* Priority for Timer AO - higher priority */
    COUNTER_AO_PRIO = 31U  /* Priority for Counter AO - lower priority */
};

/* Pointers to active object instances */
extern QActive *AO_Counter; /* Counter AO instance pointer */
extern QActive *AO_Timer;   /* Timer AO instance pointer */

/* Mutex for thread-safe logging */
extern rt_mutex_t g_log_mutex;
/* Mutex for statistics access */
extern rt_mutex_t g_stats_mutex;

/* Structure for shared performance test statistics */
typedef struct {
    volatile uint32_t counter_updates;   /* Number of counter updates */
    volatile uint32_t timer_ticks;       /* Number of timer ticks */
    volatile uint32_t timer_reports;     /* Number of timer reports */
    volatile uint32_t log_messages;      /* Number of log messages */
    volatile uint32_t test_duration_ms;  /* Test duration in milliseconds */
    volatile rt_bool_t test_running;     /* Test running status flag */
} PerformanceStats;

extern PerformanceStats g_perf_stats; /* Global statistics instance */

/* Function to initialize the performance test application */
void PerformanceApp_init(void);

/* Function to start the performance test */
int PerformanceApp_start(void);

/* Function to stop the performance test */
void PerformanceApp_stop(void);

/* Function to get current performance statistics */
void PerformanceApp_getStats(PerformanceStats *stats);

/* Function to reset performance statistics */
void PerformanceApp_resetStats(void);

/* Function to check if QF framework is already initialized */
rt_bool_t PerformanceApp_isQFInitialized(void);

/* Function to check if Active Objects are already started */
rt_bool_t PerformanceApp_areAOsStarted(void);

/* BSP initialization function */
void BSP_init(void);
/* Function to get current timestamp in milliseconds */
uint32_t BSP_getTimestampMs(void);
/* Function to turn on the LED */
void BSP_ledOn(void);
/* Function to turn off the LED */
void BSP_ledOff(void);
/* Function to toggle the LED state */
void BSP_ledToggle(void);

/* RT-Thread MSH command: start performance test */
int perf_test_start_cmd(int argc, char** argv);
/* RT-Thread MSH command: stop performance test */
int perf_test_stop_cmd(int argc, char** argv);
/* RT-Thread MSH command: show performance test statistics */
int perf_test_stats_cmd(int argc, char** argv);
/* RT-Thread MSH command: reset performance test statistics */
int perf_test_reset_cmd(int argc, char** argv);

#endif /* APP_MAIN_H_ */
