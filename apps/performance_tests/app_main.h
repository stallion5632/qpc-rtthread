/*============================================================================
* Product: Performance Test Suite Application Main Header
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
#ifndef APP_MAIN_H_
#define APP_MAIN_H_

#include "qpc.h"
#include <rtthread.h>

/*==========================================================================*/
/* Application Configuration Constants */
/*==========================================================================*/

/* Queue sizes for each Active Object */
#define COUNTER_QUEUE_SIZE    10U   /* Queue size for Counter AO */
#define TIMER_QUEUE_SIZE      8U    /* Queue size for Timer AO */  
#define LOGGER_QUEUE_SIZE     16U   /* Queue size for Logger AO */

/* Number of subscription entries for publish-subscribe system */
#define NUM_SUBSCRIPTIONS     32U   /* Total subscription table size */

/* Maximum number of Active Objects in the system */
#define MAX_ACTIVE_OBJECTS    8U    /* Maximum AOs that can be registered */

/*==========================================================================*/
/* Application Signal Definitions */
/*==========================================================================*/

enum AppSignals {
    /* Counter signals */
    COUNTER_INCREMENT_SIG = Q_USER_SIG,  /* Signal to increment counter */
    COUNTER_DECREMENT_SIG,               /* Signal to decrement counter */
    COUNTER_RESET_SIG,                   /* Signal to reset counter */
    COUNTER_GET_VALUE_SIG,               /* Signal to get counter value */
    
    /* Timer signals */
    TIMER_START_SIG,                     /* Signal to start timer */
    TIMER_STOP_SIG,                      /* Signal to stop timer */
    TIMER_TIMEOUT_SIG,                   /* Signal for timer timeout */
    TIMER_RESET_SIG,                     /* Signal to reset timer */
    
    /* Logger signals */
    LOGGER_LOG_SIG,                      /* Signal to log message */
    LOGGER_CLEAR_SIG,                    /* Signal to clear log */
    LOGGER_FLUSH_SIG,                    /* Signal to flush log */
    
    /* Performance test signals */
    PERF_TEST_START_SIG,                 /* Signal to start performance test */
    PERF_TEST_STOP_SIG,                  /* Signal to stop performance test */
    PERF_TEST_RESULT_SIG,                /* Signal for test results */
    
    MAX_APP_SIG                          /* Maximum signal value */
};

/*==========================================================================*/
/* Application Event Structures */
/*==========================================================================*/

/* Counter event structure with value parameter */
typedef struct {
    QEvt super;                          /* Base QEvt structure */
    uint32_t value;                      /* Counter value or increment amount */
    uint32_t timestamp;                  /* Event timestamp for performance tracking */
} CounterEvt;

/* Timer event structure with duration parameter */
typedef struct {
    QEvt super;                          /* Base QEvt structure */
    uint32_t duration_ms;                /* Timer duration in milliseconds */
    uint32_t timer_id;                   /* Timer identifier for multiple timers */
    uint32_t timestamp;                  /* Event timestamp for performance tracking */
} TimerEvt;

/* Logger event structure with message data */
typedef struct {
    QEvt super;                          /* Base QEvt structure */
    char message[64];                    /* Log message string buffer */
    uint8_t log_level;                   /* Log level (INFO, WARN, ERROR) */
    uint32_t timestamp;                  /* Event timestamp for performance tracking */
} LoggerEvt;

/* Performance test event structure */
typedef struct {
    QEvt super;                          /* Base QEvt structure */
    uint32_t test_id;                    /* Test identifier */
    uint32_t iteration_count;            /* Number of test iterations */
    uint32_t result_data;                /* Test result data */
    uint32_t timestamp;                  /* Event timestamp for performance tracking */
} PerfTestEvt;

/*==========================================================================*/
/* Global Subscription Table */
/*==========================================================================*/

/* Global subscription storage for publish-subscribe system */
extern QSubscrList subscriptions[MAX_APP_SIG];

/*==========================================================================*/
/* Application Initialization and Management Functions */
/*==========================================================================*/

/* Initialize the entire application framework */
void app_main_init(void);

/* Start all Active Objects in the application */
void app_main_start(void);

/* Stop all Active Objects gracefully */
void app_main_stop(void);

/* Check if application is properly initialized */
rt_bool_t app_main_is_initialized(void);

/* Check if application is currently running */
rt_bool_t app_main_is_running(void);

/* Get application runtime statistics */
void app_main_get_stats(void);

/*==========================================================================*/
/* MSH Shell Command Functions */
/*==========================================================================*/

/* MSH command to initialize performance tests */
void app_init_cmd(void);

/* MSH command to start performance tests */
void app_start_cmd(void);

/* MSH command to stop performance tests */
void app_stop_cmd(void);

/* MSH command to show application status */
void app_status_cmd(void);

/* MSH command to show application statistics */
void app_stats_cmd(void);

#endif /* APP_MAIN_H_ */