/*============================================================================
* Product: Logger Active Object Implementation
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
#include "logger_ao.h"
#include "bsp.h"
#include <rtthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/*==========================================================================*/
/* Global Logger AO Instance */
/*==========================================================================*/

/* Global Logger Active Object instance */
LoggerAO g_logger;

/*==========================================================================*/
/* Local (Private) State Machine Functions */
/*==========================================================================*/

/* Forward declarations of state functions */
static QState LoggerAO_initial(LoggerAO * const me, void const * const par);
static QState LoggerAO_idle(LoggerAO * const me, QEvt const * const e);
static QState LoggerAO_logging(LoggerAO * const me, QEvt const * const e);
static QState LoggerAO_flushing(LoggerAO * const me, QEvt const * const e);
static QState LoggerAO_error(LoggerAO * const me, QEvt const * const e);

/*==========================================================================*/
/* Local Helper Functions */
/*==========================================================================*/

/* Initialize a log entry to default values */
static void LoggerAO_initLogEntry(LogEntry *entry) {
    entry->sequence_id = 0U;
    entry->timestamp = 0U;
    entry->log_level = LOG_LEVEL_DEBUG;
    entry->source_id = 0U;
    memset(entry->message, 0, LOGGER_MAX_MESSAGE_LEN);
    entry->is_valid = RT_FALSE;
}

/* Add log entry to circular buffer */
static rt_bool_t LoggerAO_addLogEntry(LoggerAO *const me, uint8_t level, uint8_t source_id,
                                      const char *message, uint32_t timestamp) {
    /* Check if level meets current filter */
    if (level < me->current_log_level) {
        me->messages_filtered++;
        return RT_FALSE; /* Message filtered out */
    }

    /* Check if buffer is full */
    if (me->buffer_full) {
        me->messages_dropped++;
        me->buffer_overflows++;
        return RT_FALSE; /* Buffer full, message dropped */
    }

    /* Get current write entry */
    LogEntry *entry = &me->log_buffer[me->write_index];

    /* Initialize entry */
    entry->sequence_id = me->next_sequence_id++;
    entry->timestamp = timestamp;
    entry->log_level = level;
    entry->source_id = source_id;
    rt_strncpy(entry->message, message, LOGGER_MAX_MESSAGE_LEN - 1);
    entry->message[LOGGER_MAX_MESSAGE_LEN - 1] = '\0'; /* Ensure null termination */
    entry->is_valid = RT_TRUE;

    /* Update buffer pointers */
    me->write_index = (me->write_index + 1) % LOGGER_MAX_ENTRIES;

    /* Update entry count and check for buffer full condition */
    if (me->entry_count < LOGGER_MAX_ENTRIES) {
        me->entry_count++;
    } else {
        /* Buffer is full, will overwrite oldest entry */
        me->read_index = (me->read_index + 1) % LOGGER_MAX_ENTRIES;
        me->buffer_full = RT_TRUE;
        me->buffer_overflows++;
    }

    /* Update statistics */
    me->messages_logged[level]++;
    me->total_log_operations++;

    return RT_TRUE;
}

/* Output log entry to console */
static void LoggerAO_outputToConsole(LogEntry const *entry, rt_bool_t include_timestamp) {
    char timestamp_str[LOGGER_TIMESTAMP_SIZE];

    if (include_timestamp) {
        LoggerAO_formatTimestamp(entry->timestamp, timestamp_str, sizeof(timestamp_str));
        rt_kprintf("[%s][%s][%u] %s\n",
                   timestamp_str,
                   LoggerAO_levelToString(entry->log_level),
                   entry->source_id,
                   entry->message);
    } else {
        rt_kprintf("[%s][%u] %s\n",
                   LoggerAO_levelToString(entry->log_level),
                   entry->source_id,
                   entry->message);
    }
}

/* Update performance statistics */
static void LoggerAO_updatePerformanceStats(LoggerAO *const me, uint32_t processing_time) {
    /* Update min/max processing time */
    if (processing_time < me->min_log_time) {
        me->min_log_time = processing_time;
    }
    if (processing_time > me->max_log_time) {
        me->max_log_time = processing_time;
    }

    /* Update average processing time */
    me->total_log_time += processing_time;
    if (me->total_log_operations > 0U) {
        me->avg_log_time = me->total_log_time / me->total_log_operations;
    }
}

/*==========================================================================*/
/* Logger AO Constructor */
/*==========================================================================*/

void LoggerAO_ctor(LoggerAO *const me)
{
    /* Initialize the base QActive class */
    QActive_ctor(&me->super, Q_STATE_CAST(&LoggerAO_initial));

    /* Initialize circular buffer */
    for (uint8_t i = 0U; i < LOGGER_MAX_ENTRIES; i++) {
        LoggerAO_initLogEntry(&me->log_buffer[i]);
    }

    /* Initialize buffer management variables */
    me->write_index = 0U;
    me->read_index = 0U;
    me->entry_count = 0U;
    me->buffer_full = RT_FALSE;

    /* Initialize logger configuration with defaults */
    me->current_log_level = LOG_LEVEL_INFO;         /* Default to INFO level */
    me->logger_mode = LOGGER_MODE_SYNC;             /* Default to synchronous mode */
    me->output_targets = LOGGER_OUTPUT_CONSOLE;     /* Default to console output */
    me->timestamps_enabled = RT_TRUE;               /* Enable timestamps by default */

    /* Initialize performance tracking variables */
    me->total_log_operations = 0U;
    for (uint8_t i = 0U; i < LOG_LEVEL_OFF; i++) {
        me->messages_logged[i] = 0U;
    }
    me->messages_filtered = 0U;
    me->messages_dropped = 0U;
    me->buffer_overflows = 0U;

    /* Initialize timing and performance metrics */
    me->min_log_time = UINT32_MAX;                  /* Start with maximum value */
    me->max_log_time = 0U;
    me->avg_log_time = 0U;
    me->total_log_time = 0U;

    /* Initialize operation counters */
    me->flush_operations = 0U;
    me->clear_operations = 0U;
    me->next_sequence_id = 1U;                      /* Start sequence IDs from 1 */

    /* Initialize error tracking counters */
    me->format_errors = 0U;
    me->output_errors = 0U;
    me->buffer_errors = 0U;

    rt_kprintf("LoggerAO: Constructor completed, ready for logging operations\n");
}

/*==========================================================================*/
/* Logger AO State Machine Implementation */
/*==========================================================================*/

/* Initial pseudostate - sets up subscriptions and transitions to idle */
static QState LoggerAO_initial(LoggerAO * const me, void const * const par)
{
    (void)par; /* Unused parameter */

    rt_kprintf("LoggerAO: Entering initial state\n");

    /* Subscribe to relevant signals for logger operations */
    QActive_subscribe((QActive *)me, LOGGER_LOG_SIG);
    QActive_subscribe((QActive *)me, LOGGER_CLEAR_SIG);
    QActive_subscribe((QActive *)me, LOGGER_FLUSH_SIG);

    rt_kprintf("LoggerAO: Subscribed to logger signals\n");

    /* Transition to idle state and wait for events */
    return Q_TRAN(&LoggerAO_idle);
}

/* Idle state - waiting for logging operation events */
static QState LoggerAO_idle(LoggerAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("LoggerAO: Entering idle state, ready for logging operations\n");
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("LoggerAO: Exiting idle state\n");
            status = Q_HANDLED();
            break;
        }

        case LOGGER_LOG_SIG: {
            LoggerEvt const *evt = (LoggerEvt const *)e;
            rt_kprintf("LoggerAO: Received LOG signal (level %u)\n", evt->log_level);
            status = Q_TRAN(&LoggerAO_logging);
            break;
        }

        case LOGGER_FLUSH_SIG: {
            rt_kprintf("LoggerAO: Received FLUSH signal\n");
            status = Q_TRAN(&LoggerAO_flushing);
            break;
        }

        case LOGGER_CLEAR_SIG: {
            rt_kprintf("LoggerAO: Received CLEAR signal\n");

            /* Clear buffer immediately */
            for (uint8_t i = 0U; i < LOGGER_MAX_ENTRIES; i++) {
                LoggerAO_initLogEntry(&me->log_buffer[i]);
            }
            me->write_index = 0U;
            me->read_index = 0U;
            me->entry_count = 0U;
            me->buffer_full = RT_FALSE;
            me->clear_operations++;

            rt_kprintf("LoggerAO: Log buffer cleared\n");
            status = Q_HANDLED();
            break;
        }

        default: {
            rt_kprintf("LoggerAO: Unexpected signal %d in idle state\n", (int)e->sig);
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}

/* Logging state - handles log message processing */
static QState LoggerAO_logging(LoggerAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);
    uint32_t start_time = 0, end_time, processing_time;
    LoggerEvt const *evt;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("LoggerAO: Entering logging state\n");
            start_time = BSP_getTimestamp();
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("LoggerAO: Exiting logging state\n");

            /* Calculate and update performance metrics */
            end_time = BSP_getTimestamp();
            processing_time = LoggerAO_calculateLogTime(start_time, end_time);
            LoggerAO_updatePerformanceStats(me, processing_time);

            status = Q_HANDLED();
            break;
        }

        case LOGGER_LOG_SIG: {
            evt = (LoggerEvt const *)e;
            rt_kprintf("LoggerAO: Processing LOG message (level %u): %.32s\n",
                       evt->log_level, evt->message);

            /* Validate log level */
            if (!LoggerAO_isLevelValid(evt->log_level)) {
                rt_kprintf("LoggerAO: Invalid log level %u\n", evt->log_level);
                me->format_errors++;
                status = Q_TRAN(&LoggerAO_error);
                break;
            }

            /* Add message to buffer */
            rt_bool_t success = LoggerAO_addLogEntry(me, evt->log_level, 0U /* default source */,
                                                     evt->message, evt->timestamp);

            if (!success) {
                rt_kprintf("LoggerAO: Failed to add log entry (level: %u, filtered: %s)\n",
                           evt->log_level, (evt->log_level < me->current_log_level) ? "YES" : "NO");
                if (evt->log_level >= me->current_log_level) {
                    me->buffer_errors++;
                }
            } else {
                rt_kprintf("LoggerAO: Log entry added successfully (seq: %u)\n",
                           me->next_sequence_id - 1);

                /* Output immediately if in sync mode or force output requested */
                if (me->logger_mode == LOGGER_MODE_SYNC) {
                    LogEntry *entry = &me->log_buffer[(me->write_index - 1 + LOGGER_MAX_ENTRIES) % LOGGER_MAX_ENTRIES];
                    if (me->output_targets & LOGGER_OUTPUT_CONSOLE) {
                        LoggerAO_outputToConsole(entry, me->timestamps_enabled);
                    }
                }
            }

            /* Return to idle state */
            status = Q_TRAN(&LoggerAO_idle);
            break;
        }

        case LOGGER_FLUSH_SIG: {
            rt_kprintf("LoggerAO: Received FLUSH signal while logging\n");
            status = Q_TRAN(&LoggerAO_flushing);
            break;
        }

        case LOGGER_CLEAR_SIG: {
            rt_kprintf("LoggerAO: Received CLEAR signal while logging\n");
            /* Complete current log operation first, then clear will be handled in idle */
            status = Q_TRAN(&LoggerAO_idle);
            break;
        }

        default: {
            rt_kprintf("LoggerAO: Unexpected signal %d in logging state\n", (int)e->sig);
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}

/* Flushing state - handles buffer flush operations */
static QState LoggerAO_flushing(LoggerAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("LoggerAO: Entering flushing state\n");

            /* Flush all valid entries to console if enabled */
            if (me->output_targets & LOGGER_OUTPUT_CONSOLE) {
                rt_kprintf("=== Logger Buffer Flush ===\n");
                uint8_t entries_flushed = 0U;

                /* Flush entries from read_index to write_index */
                uint8_t current_index = me->read_index;
                for (uint8_t i = 0U; i < me->entry_count; i++) {
                    LogEntry *entry = &me->log_buffer[current_index];
                    if (entry->is_valid) {
                        LoggerAO_outputToConsole(entry, me->timestamps_enabled);
                        entries_flushed++;
                    }
                    current_index = (current_index + 1) % LOGGER_MAX_ENTRIES;
                }

                rt_kprintf("=== Flush Complete: %u entries ===\n", entries_flushed);
                me->flush_operations++;
            }

            /* Handle other output targets */
            if (me->output_targets & LOGGER_OUTPUT_FILE) {
                /* File output implementation would go here */
                rt_kprintf("LoggerAO: File output not implemented\n");
                me->output_errors++;
            }

            if (me->output_targets & LOGGER_OUTPUT_NETWORK) {
                /* Network output implementation would go here */
                rt_kprintf("LoggerAO: Network output not implemented\n");
                me->output_errors++;
            }

            /* Auto-transition back to idle after flush */
            static QEvt const done_evt = { TIMER_TIMEOUT_SIG, 0U, 0U };
            QActive_post_((QActive *)me, &done_evt, 0U, (void *)0);

            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("LoggerAO: Exiting flushing state\n");
            status = Q_HANDLED();
            break;
        }

        case TIMER_TIMEOUT_SIG: {
            rt_kprintf("LoggerAO: Flush operation complete, returning to idle\n");
            status = Q_TRAN(&LoggerAO_idle);
            break;
        }

        case LOGGER_LOG_SIG: {
            /* Queue log messages during flush - they will be processed after flush */
            rt_kprintf("LoggerAO: Log message queued during flush operation\n");
            status = Q_HANDLED();
            break;
        }

        default: {
            rt_kprintf("LoggerAO: Signal %d handled in flushing state\n", (int)e->sig);
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}

/* Error state - handles error conditions and recovery */
static QState LoggerAO_error(LoggerAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("LoggerAO: Entering error state\n");
            rt_kprintf("LoggerAO: Error counts - Format: %u, Output: %u, Buffer: %u\n",
                       me->format_errors, me->output_errors, me->buffer_errors);

            /* Auto-recovery: transition back to idle after error handling */
            static QEvt const recovery_evt = { TIMER_TIMEOUT_SIG, 0U, 0U };
            QActive_post_((QActive *)me, &recovery_evt, 0U, (void *)0);

            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("LoggerAO: Exiting error state, recovering to normal operation\n");
            status = Q_HANDLED();
            break;
        }

        case TIMER_TIMEOUT_SIG: {
            rt_kprintf("LoggerAO: Auto-recovery timeout, returning to idle state\n");
            status = Q_TRAN(&LoggerAO_idle);
            break;
        }

        case LOGGER_CLEAR_SIG: {
            rt_kprintf("LoggerAO: Clear received in error state, clearing errors\n");
            /* Clear buffer and error state */
            LoggerAO_clear(me);
            status = Q_TRAN(&LoggerAO_idle);
            break;
        }

        default: {
            rt_kprintf("LoggerAO: Signal %d ignored in error state\n", (int)e->sig);
            status = Q_HANDLED();
            break;
        }
    }

    return status;
}

#if 0
/* Reporting state - handles statistics and status queries */
static QState LoggerAO_reporting(LoggerAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("LoggerAO: Entering reporting state\n");

            /* Report logger status and statistics */
            rt_kprintf("=== Logger Status Report ===\n");
            rt_kprintf("Configuration:\n");
            rt_kprintf("  Log Level: %s (%u)\n", LoggerAO_levelToString(me->current_log_level), me->current_log_level);
            rt_kprintf("  Mode: %s (%u)\n", LoggerAO_modeToString(me->logger_mode), me->logger_mode);
            rt_kprintf("  Output Targets: 0x%02X\n", me->output_targets);
            rt_kprintf("  Timestamps: %s\n", me->timestamps_enabled ? "ENABLED" : "DISABLED");

            rt_kprintf("Buffer Status:\n");
            rt_kprintf("  Entries: %u/%u (%.1f%% full)\n", me->entry_count, LOGGER_MAX_ENTRIES,
                       (float)me->entry_count * 100.0f / LOGGER_MAX_ENTRIES);
            rt_kprintf("  Write Index: %u\n", me->write_index);
            rt_kprintf("  Read Index: %u\n", me->read_index);
            rt_kprintf("  Buffer Full: %s\n", me->buffer_full ? "YES" : "NO");

            rt_kprintf("Operations:\n");
            rt_kprintf("  Total Log Operations: %u\n", me->total_log_operations);
            rt_kprintf("  Flush Operations: %u\n", me->flush_operations);
            rt_kprintf("  Clear Operations: %u\n", me->clear_operations);
            rt_kprintf("  Next Sequence ID: %u\n", me->next_sequence_id);

            rt_kprintf("Message Statistics:\n");
            for (uint8_t i = 0U; i < LOG_LEVEL_OFF; i++) {
                rt_kprintf("  %s: %u\n", LoggerAO_levelToString(i), me->messages_logged[i]);
            }
            rt_kprintf("  Filtered: %u\n", me->messages_filtered);
            rt_kprintf("  Dropped: %u\n", me->messages_dropped);
            rt_kprintf("  Buffer Overflows: %u\n", me->buffer_overflows);

            rt_kprintf("Performance Metrics:\n");
            rt_kprintf("  Min Log Time: %u cycles\n", me->min_log_time);
            rt_kprintf("  Max Log Time: %u cycles\n", me->max_log_time);
            rt_kprintf("  Avg Log Time: %u cycles\n", me->avg_log_time);

            rt_kprintf("Error Statistics:\n");
            rt_kprintf("  Format Errors: %u\n", me->format_errors);
            rt_kprintf("  Output Errors: %u\n", me->output_errors);
            rt_kprintf("  Buffer Errors: %u\n", me->buffer_errors);
            rt_kprintf("============================\n");

            /* Auto-transition back to idle after reporting */
            static QEvt const done_evt = { TIMER_TIMEOUT_SIG, 0U, 0U };
            QActive_post_((QActive *)me, &done_evt, 0U, (void *)0);

            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("LoggerAO: Exiting reporting state\n");
            status = Q_HANDLED();
            break;
        }

        case TIMER_TIMEOUT_SIG: {
            rt_kprintf("LoggerAO: Reporting complete, returning to idle state\n");
            status = Q_TRAN(&LoggerAO_idle);
            break;
        }

        default: {
            rt_kprintf("LoggerAO: Signal %d handled in reporting state\n", (int)e->sig);
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}
#endif

/*==========================================================================*/
/* Logger AO Public Interface Functions */
/*==========================================================================*/

rt_bool_t LoggerAO_log(LoggerAO *const me, uint8_t level, uint8_t source_id, const char *message)
{
    if (message == (char *)0 || !LoggerAO_isLevelValid(level)) {
        return RT_FALSE;
    }

    static LoggerEvt evt;
    evt.super.sig = LOGGER_LOG_SIG;
    evt.log_level = level;
    rt_strncpy(evt.message, message, LOGGER_MAX_MESSAGE_LEN - 1);
    evt.message[LOGGER_MAX_MESSAGE_LEN - 1] = '\0';
    evt.timestamp = BSP_getTimestamp();

    QActive_post_(&me->super, &evt.super, 0U, (void *)0);

    return RT_TRUE;
}

rt_bool_t LoggerAO_logf(LoggerAO *const me, uint8_t level, uint8_t source_id, const char *format, ...)
{
    if (format == (char *)0 || !LoggerAO_isLevelValid(level)) {
        return RT_FALSE;
    }

    char message_buffer[LOGGER_MAX_MESSAGE_LEN];
    va_list args;

    va_start(args, format);
    int result = rt_vsnprintf(message_buffer, sizeof(message_buffer), format, args);
    va_end(args);

    if (result < 0) {
        me->format_errors++;
        return RT_FALSE;
    }

    return LoggerAO_log(me, level, source_id, message_buffer);
}

void LoggerAO_flush(LoggerAO *const me, uint8_t output_targets)
{
    static LoggerFlushEvt evt;
    evt.super.sig = LOGGER_FLUSH_SIG;
    evt.output_targets = output_targets;
    evt.clear_after_flush = RT_FALSE;
    evt.timestamp = BSP_getTimestamp();

    QActive_post_(&me->super, &evt.super, 0U, (void *)0);
}

void LoggerAO_clear(LoggerAO *const me)
{
    static QEvt const evt = { LOGGER_CLEAR_SIG, 0U, 0U };
    QActive_post_(&me->super, &evt, 0U, (void *)0);
}

uint8_t LoggerAO_getEntryCount(LoggerAO const *const me)
{
    return me->entry_count;
}

rt_bool_t LoggerAO_isBufferFull(LoggerAO const *const me)
{
    return me->buffer_full;
}

void LoggerAO_getStats(LoggerAO const *const me,
                       uint32_t *total_ops,
                       uint32_t *filtered,
                       uint32_t *dropped,
                       uint32_t *overflows)
{
    if (total_ops != (uint32_t *)0) *total_ops = me->total_log_operations;
    if (filtered != (uint32_t *)0) *filtered = me->messages_filtered;
    if (dropped != (uint32_t *)0) *dropped = me->messages_dropped;
    if (overflows != (uint32_t *)0) *overflows = me->buffer_overflows;
}

void LoggerAO_getPerformanceMetrics(LoggerAO const *const me,
                                    uint32_t *min_time,
                                    uint32_t *max_time,
                                    uint32_t *avg_time)
{
    if (min_time != (uint32_t *)0) *min_time = me->min_log_time;
    if (max_time != (uint32_t *)0) *max_time = me->max_log_time;
    if (avg_time != (uint32_t *)0) *avg_time = me->avg_log_time;
}

uint32_t LoggerAO_getMessagesForLevel(LoggerAO const *const me, uint8_t level)
{
    if (level < LOG_LEVEL_OFF) {
        return me->messages_logged[level];
    }
    return 0U;
}

/*==========================================================================*/
/* Logger AO Utility Functions */
/*==========================================================================*/

const char* LoggerAO_levelToString(uint8_t level)
{
    switch (level) {
        case LOG_LEVEL_DEBUG:    return "DEBUG";
        case LOG_LEVEL_INFO:     return "INFO";
        case LOG_LEVEL_WARN:     return "WARN";
        case LOG_LEVEL_ERROR:    return "ERROR";
        case LOG_LEVEL_CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

const char* LoggerAO_modeToString(uint8_t mode)
{
    switch (mode) {
        case LOGGER_MODE_SYNC:  return "SYNC";
        case LOGGER_MODE_ASYNC: return "ASYNC";
        case LOGGER_MODE_BATCH: return "BATCH";
        default:                return "UNKNOWN";
    }
}

rt_bool_t LoggerAO_isLevelValid(uint8_t level)
{
    return (level < LOG_LEVEL_OFF);
}

void LoggerAO_formatTimestamp(uint32_t timestamp, char *buffer, rt_size_t buffer_size)
{
    if (buffer != (char *)0 && buffer_size > 0) {
        rt_snprintf(buffer, buffer_size, "%010u", timestamp);
    }
}

uint32_t LoggerAO_calculateLogTime(uint32_t start_time, uint32_t end_time)
{
    /* Handle timer wraparound case */
    if (end_time >= start_time) {
        return (end_time - start_time);
    } else {
        return (UINT32_MAX - start_time + end_time + 1U);
    }
}

uint8_t LoggerAO_getBufferUsage(LoggerAO const *const me)
{
    return (uint8_t)((me->entry_count * 100U) / LOGGER_MAX_ENTRIES);
}

/*==========================================================================*/
/* MSH Shell Command Implementations */
/*==========================================================================*/

void logger_log_cmd(int argc, char *argv[])
{
    uint8_t level = LOG_LEVEL_INFO;
    const char *message = "Test log message";

    if (argc > 1) {
        level = (uint8_t)atoi(argv[1]);
    }
    if (argc > 2) {
        message = argv[2];
    }

    rt_kprintf("Sending log message (level %u): %s\n", level, message);
    LoggerAO_log(&g_logger, level, 0U, message);
}

void logger_set_level_cmd(int argc, char *argv[])
{
    if (argc > 1) {
        uint8_t new_level = (uint8_t)atoi(argv[1]);
        if (LoggerAO_isLevelValid(new_level)) {
            g_logger.current_log_level = new_level;
            rt_kprintf("Logger level set to %u (%s)\n", new_level, LoggerAO_levelToString(new_level));
        } else {
            rt_kprintf("Invalid log level %u. Valid range: 0-%u\n", new_level, LOG_LEVEL_OFF - 1);
        }
    } else {
        rt_kprintf("Current log level: %u (%s)\n",
                   g_logger.current_log_level, LoggerAO_levelToString(g_logger.current_log_level));
    }
}

void logger_flush_cmd(void)
{
    rt_kprintf("Flushing logger buffer...\n");
    LoggerAO_flush(&g_logger, LOGGER_OUTPUT_CONSOLE);
}

void logger_clear_cmd(void)
{
    rt_kprintf("Clearing logger buffer...\n");
    LoggerAO_clear(&g_logger);
}

void logger_stats_cmd(void)
{
    uint32_t total, filtered, dropped, overflows;
    LoggerAO_getStats(&g_logger, &total, &filtered, &dropped, &overflows);

    rt_kprintf("=== Logger Statistics ===\n");
    rt_kprintf("Total Operations: %u\n", total);
    rt_kprintf("Messages Filtered: %u\n", filtered);
    rt_kprintf("Messages Dropped: %u\n", dropped);
    rt_kprintf("Buffer Overflows: %u\n", overflows);
    rt_kprintf("Buffer Usage: %u/%u (%u%%)\n",
               LoggerAO_getEntryCount(&g_logger), LOGGER_MAX_ENTRIES,
               LoggerAO_getBufferUsage(&g_logger));
    rt_kprintf("=========================\n");
}

void logger_perf_cmd(void)
{
    uint32_t min_time, max_time, avg_time;
    LoggerAO_getPerformanceMetrics(&g_logger, &min_time, &max_time, &avg_time);

    rt_kprintf("=== Logger Performance Metrics ===\n");
    rt_kprintf("Minimum Log Time: %u cycles\n", min_time);
    rt_kprintf("Maximum Log Time: %u cycles\n", max_time);
    rt_kprintf("Average Log Time: %u cycles\n", avg_time);
    rt_kprintf("==================================\n");
}

void logger_config_cmd(void)
{
    rt_kprintf("=== Logger Configuration ===\n");
    rt_kprintf("Log Level: %s (%u)\n", LoggerAO_levelToString(g_logger.current_log_level), g_logger.current_log_level);
    rt_kprintf("Mode: %s (%u)\n", LoggerAO_modeToString(g_logger.logger_mode), g_logger.logger_mode);
    rt_kprintf("Output Targets: 0x%02X\n", g_logger.output_targets);
    rt_kprintf("Timestamps: %s\n", g_logger.timestamps_enabled ? "ENABLED" : "DISABLED");
    rt_kprintf("=============================\n");
}

/* Export MSH commands for interactive testing */
MSH_CMD_EXPORT_ALIAS(logger_log_cmd, logger_log, Log a test message [level] [message]);
MSH_CMD_EXPORT_ALIAS(logger_set_level_cmd, logger_set_level, Set logger level [0-4]);
MSH_CMD_EXPORT(logger_flush_cmd, Flush logger buffer to console);
MSH_CMD_EXPORT(logger_clear_cmd, Clear logger buffer);
MSH_CMD_EXPORT(logger_stats_cmd, Show logger statistics);
MSH_CMD_EXPORT(logger_perf_cmd, Show logger performance metrics);
MSH_CMD_EXPORT(logger_config_cmd, Show logger configuration);
