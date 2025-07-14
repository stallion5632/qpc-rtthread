/*============================================================================
* Product: Logger Active Object Implementation
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
#include "logger_ao.h"
#include "bsp.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

Q_DEFINE_THIS_MODULE("logger_ao")

/*==========================================================================*/
/* Logger AO Local Definitions */
/*==========================================================================*/

/* Logger AO states */
static QState LoggerAO_initial (LoggerAO * const me, QEvt const * const e);
static QState LoggerAO_idle    (LoggerAO * const me, QEvt const * const e);
static QState LoggerAO_active  (LoggerAO * const me, QEvt const * const e);

/* Singleton instance */
static LoggerAO l_loggerAO;

/* Helper function for creating log events */
static LogEvt *LoggerAO_createLogEvt(LogLevel level, const char *format, va_list args);

/* Log level strings for output */
static const char * const l_logLevelStr[] = {
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR"
};

/*==========================================================================*/
/* Logger AO Public Interface */
/*==========================================================================*/
void LoggerAO_ctor(void) {
    rt_kprintf("[QPC] module: %s\n", Q_this_module_);
    LoggerAO * const me = &l_loggerAO;

    /* Call base class constructor */
    QActive_ctor(&me->super, Q_STATE_CAST(&LoggerAO_initial));

    /* Initialize time event */
    QTimeEvt_ctorX(&me->flushTimeEvt, &me->super, LOGGER_FLUSH_SIG, 0U);

    /* Initialize instance variables */
    me->log_count = 0U;
    me->debug_count = 0U;
    me->info_count = 0U;
    me->warn_count = 0U;
    me->error_count = 0U;
    me->is_active = RT_FALSE;
}

LoggerAO *LoggerAO_getInstance(void) {
    return &l_loggerAO;
}

/*==========================================================================*/
/* Thread-safe Logging Functions */
/*==========================================================================*/
void LoggerAO_logDebug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    LogEvt *logEvt = LoggerAO_createLogEvt(LOG_LEVEL_DEBUG, format, args);
    va_end(args);

    if (logEvt != (LogEvt *)0) {
        QACTIVE_POST((QActive *)&l_loggerAO, (QEvt *)logEvt, (void *)0);
    }
}

void LoggerAO_logInfo(const char *format, ...) {
    va_list args;
    va_start(args, format);
    LogEvt *logEvt = LoggerAO_createLogEvt(LOG_LEVEL_INFO, format, args);
    va_end(args);

    if (logEvt != (LogEvt *)0) {
        QACTIVE_POST((QActive *)&l_loggerAO, (QEvt *)logEvt, (void *)0);
    }
}

void LoggerAO_logWarn(const char *format, ...) {
    va_list args;
    va_start(args, format);
    LogEvt *logEvt = LoggerAO_createLogEvt(LOG_LEVEL_WARN, format, args);
    va_end(args);

    if (logEvt != (LogEvt *)0) {
        QACTIVE_POST((QActive *)&l_loggerAO, (QEvt *)logEvt, (void *)0);
    }
}

void LoggerAO_logError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    LogEvt *logEvt = LoggerAO_createLogEvt(LOG_LEVEL_ERROR, format, args);
    va_end(args);

    if (logEvt != (LogEvt *)0) {
        QACTIVE_POST((QActive *)&l_loggerAO, (QEvt *)logEvt, (void *)0);
    }
}

/*==========================================================================*/
/* Statistics Functions */
/*==========================================================================*/
uint32_t LoggerAO_getLogCount(void) {
    LoggerAO * const me = &l_loggerAO;
    return me->log_count; /* Atomic read on 32-bit platforms */
}

uint32_t LoggerAO_getDebugCount(void) {
    LoggerAO * const me = &l_loggerAO;
    return me->debug_count; /* Atomic read on 32-bit platforms */
}

uint32_t LoggerAO_getInfoCount(void) {
    LoggerAO * const me = &l_loggerAO;
    return me->info_count; /* Atomic read on 32-bit platforms */
}

uint32_t LoggerAO_getWarnCount(void) {
    LoggerAO * const me = &l_loggerAO;
    return me->warn_count; /* Atomic read on 32-bit platforms */
}

uint32_t LoggerAO_getErrorCount(void) {
    LoggerAO * const me = &l_loggerAO;
    return me->error_count; /* Atomic read on 32-bit platforms */
}

rt_bool_t LoggerAO_isActive(void) {
    LoggerAO * const me = &l_loggerAO;
    return me->is_active; /* Atomic read for boolean */
}

void LoggerAO_resetCounters(void) {
    LoggerAO * const me = &l_loggerAO;
    me->log_count = 0U;
    me->debug_count = 0U;
    me->info_count = 0U;
    me->warn_count = 0U;
    me->error_count = 0U;
}

/*==========================================================================*/
/* Helper Functions */
/*==========================================================================*/
static LogEvt *LoggerAO_createLogEvt(LogLevel level, const char *format, va_list args) {
    LogEvt *logEvt = Q_NEW(LogEvt, LOGGER_LOG_SIG);
    if (logEvt != (LogEvt *)0) {
        logEvt->log_level = (uint8_t)level;
        logEvt->timestamp = BSP_getTimestampMs();

        /* Format the log message */
        vsnprintf(logEvt->message, sizeof(logEvt->message), format, args);
        logEvt->message[sizeof(logEvt->message) - 1U] = '\0'; /* Ensure null termination */
    }
    return logEvt;
}

/*==========================================================================*/
/* Logger AO State Machine Implementation */
/*==========================================================================*/
static QState LoggerAO_initial(LoggerAO * const me, QEvt const * const e) {
    (void)e; /* Suppress unused parameter warning */

    /* Initialize the Logger AO */
    me->log_count = 0U;
    me->debug_count = 0U;
    me->info_count = 0U;
    me->warn_count = 0U;
    me->error_count = 0U;
    me->is_active = RT_FALSE;

    /* Subscribe to relevant signals */
    QActive_subscribe((QActive *)me, APP_START_SIG);
    QActive_subscribe((QActive *)me, APP_STOP_SIG);
    QActive_subscribe((QActive *)me, TIMER_REPORT_SIG);
    QActive_subscribe((QActive *)me, COUNTER_UPDATE_SIG);

    /* Directly output initial message to console (before logging is active) */
    rt_kprintf("[%s] LoggerAO: Initial state entered\n", l_logLevelStr[LOG_LEVEL_DEBUG]);

    return Q_TRAN(&LoggerAO_idle);
}

static QState LoggerAO_idle(LoggerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->is_active = RT_FALSE;
            /* Direct console output for state entry */
            rt_kprintf("[%s] LoggerAO: Idle state entered\n", l_logLevelStr[LOG_LEVEL_INFO]);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            /* Direct console output for state exit */
            rt_kprintf("[%s] LoggerAO: Exiting idle state\n", l_logLevelStr[LOG_LEVEL_DEBUG]);
            status = Q_HANDLED();
            break;
        }
        case APP_START_SIG: {
            /* Direct console output for transition */
            rt_kprintf("[%s] LoggerAO: Starting logging service\n", l_logLevelStr[LOG_LEVEL_INFO]);
            status = Q_TRAN(&LoggerAO_active);
            break;
        }
        case LOGGER_LOG_SIG: {
            /* Process log messages even when idle, but don't start flush timer */
            LogEvt const *logEvt = (LogEvt const *)e;

            /* Thread-safe console output using mutex */
            rt_mutex_take(g_log_mutex, RT_WAITING_FOREVER);
            rt_kprintf("[%u][%s] %s\n",
                      logEvt->timestamp,
                      l_logLevelStr[logEvt->log_level],
                      logEvt->message);
            rt_mutex_release(g_log_mutex);

            /* Update counters */
            ++me->log_count;
            switch (logEvt->log_level) {
                case LOG_LEVEL_DEBUG: ++me->debug_count; break;
                case LOG_LEVEL_INFO:  ++me->info_count;  break;
                case LOG_LEVEL_WARN:  ++me->warn_count;  break;
                case LOG_LEVEL_ERROR: ++me->error_count; break;
                default: break; /* Defensive programming */
            }

            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}

static QState LoggerAO_active(LoggerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->is_active = RT_TRUE;

            /* Start periodic flush timer (5 seconds) */
            QTimeEvt_armX(&me->flushTimeEvt,
                         BSP_TICKS_PER_SEC * 5U,  /* 5 seconds */
                         BSP_TICKS_PER_SEC * 5U); /* Periodic */

            /* Thread-safe console output */
            rt_mutex_take(g_log_mutex, RT_WAITING_FOREVER);
            rt_kprintf("[%s] LoggerAO: Active state entered, flush timer started\n",
                      l_logLevelStr[LOG_LEVEL_INFO]);
            rt_mutex_release(g_log_mutex);

            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            /* Stop flush timer */
            QTimeEvt_disarm(&me->flushTimeEvt);
            me->is_active = RT_FALSE;

            /* Thread-safe console output */
            rt_mutex_take(g_log_mutex, RT_WAITING_FOREVER);
            rt_kprintf("[%s] LoggerAO: Exiting active state, flush timer stopped\n",
                      l_logLevelStr[LOG_LEVEL_INFO]);
            rt_mutex_release(g_log_mutex);

            status = Q_HANDLED();
            break;
        }
        case LOGGER_LOG_SIG: {
            LogEvt const *logEvt = (LogEvt const *)e;

            /* Thread-safe console output using mutex */
            rt_mutex_take(g_log_mutex, RT_WAITING_FOREVER);
            rt_kprintf("[%u][%s] %s\n",
                      logEvt->timestamp,
                      l_logLevelStr[logEvt->log_level],
                      logEvt->message);
            rt_mutex_release(g_log_mutex);

            /* Update counters */
            ++me->log_count;
            switch (logEvt->log_level) {
                case LOG_LEVEL_DEBUG: ++me->debug_count; break;
                case LOG_LEVEL_INFO:  ++me->info_count;  break;
                case LOG_LEVEL_WARN:  ++me->warn_count;  break;
                case LOG_LEVEL_ERROR: ++me->error_count; break;
                default: break; /* Defensive programming */
            }

            /* Update global statistics */
            rt_mutex_take(g_stats_mutex, RT_WAITING_FOREVER);
            ++g_perf_stats.log_messages;
            rt_mutex_release(g_stats_mutex);

            status = Q_HANDLED();
            break;
        }
        case LOGGER_FLUSH_SIG: {
            /* Periodic flush - output statistics */
            rt_mutex_take(g_log_mutex, RT_WAITING_FOREVER);
            rt_kprintf("[%s] LoggerAO: Log statistics - Total: %u, Debug: %u, Info: %u, Warn: %u, Error: %u\n",
                      l_logLevelStr[LOG_LEVEL_INFO],
                      me->log_count, me->debug_count, me->info_count,
                      me->warn_count, me->error_count);
            rt_mutex_release(g_log_mutex);

            status = Q_HANDLED();
            break;
        }
        case APP_STOP_SIG: {
            rt_mutex_take(g_log_mutex, RT_WAITING_FOREVER);
            rt_kprintf("[%s] LoggerAO: Stopping logging service\n", l_logLevelStr[LOG_LEVEL_INFO]);
            rt_mutex_release(g_log_mutex);

            status = Q_TRAN(&LoggerAO_idle);
            break;
        }
        case TIMER_REPORT_SIG: {
            /* Log timer report events */
            TimerReportEvt const *reportEvt = (TimerReportEvt const *)e;

            rt_mutex_take(g_log_mutex, RT_WAITING_FOREVER);
            rt_kprintf("[%s] LoggerAO: Timer Report - Elapsed: %u ms, Ticks: %u, Counter: %u\n",
                      l_logLevelStr[LOG_LEVEL_INFO],
                      reportEvt->elapsed_ms, reportEvt->tick_count, reportEvt->counter_value);
            rt_mutex_release(g_log_mutex);

            status = Q_HANDLED();
            break;
        }
        case LOGGER_TIMEOUT_SIG: {
            rt_mutex_take(g_log_mutex, RT_WAITING_FOREVER);
            rt_kprintf("[%s] LoggerAO: Test timeout reached\n", l_logLevelStr[LOG_LEVEL_WARN]);
            rt_mutex_release(g_log_mutex);

            status = Q_TRAN(&LoggerAO_idle);
            break;
        }
        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}
