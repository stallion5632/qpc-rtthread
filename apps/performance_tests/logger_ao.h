/*============================================================================
* Product: Logger Active Object Header
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
#ifndef LOGGER_AO_H_
#define LOGGER_AO_H_

#include "qpc.h"
#include "app_main.h"

/*==========================================================================*/
/* Logger Active Object Configuration */
/*==========================================================================*/

/* Logger AO priority - must be unique among all AOs */
#define LOGGER_AO_PRIO           5U

/* Logger buffer and message configuration */
#define LOGGER_MAX_MESSAGE_LEN   128U       /* Maximum length of a single log message */
#define LOGGER_BUFFER_SIZE       4096U      /* Total circular buffer size */
#define LOGGER_MAX_ENTRIES       64U        /* Maximum number of log entries */
#define LOGGER_TIMESTAMP_SIZE    24U        /* Timestamp string buffer size */

/* Log levels - priority-based filtering */
#define LOG_LEVEL_DEBUG         0U          /* Debug level messages */
#define LOG_LEVEL_INFO          1U          /* Information level messages */
#define LOG_LEVEL_WARN          2U          /* Warning level messages */
#define LOG_LEVEL_ERROR         3U          /* Error level messages */
#define LOG_LEVEL_CRITICAL      4U          /* Critical level messages */
#define LOG_LEVEL_OFF           5U          /* Logging disabled */

/* Logger modes */
#define LOGGER_MODE_SYNC        0U          /* Synchronous logging mode */
#define LOGGER_MODE_ASYNC       1U          /* Asynchronous logging mode */
#define LOGGER_MODE_BATCH       2U          /* Batch logging mode */

/* Logger output targets */
#define LOGGER_OUTPUT_CONSOLE   0x01U       /* Output to console */
#define LOGGER_OUTPUT_FILE      0x02U       /* Output to file */
#define LOGGER_OUTPUT_NETWORK   0x04U       /* Output to network */
#define LOGGER_OUTPUT_ALL       0x07U       /* Output to all targets */

/*==========================================================================*/
/* Logger Entry Structure */
/*==========================================================================*/

/* Individual log entry structure */
typedef struct {
    uint32_t sequence_id;                   /* Sequential entry identifier */
    uint32_t timestamp;                     /* Entry timestamp in cycles */
    uint8_t log_level;                      /* Log level of this entry */
    uint8_t source_id;                      /* Source AO or module ID */
    char message[LOGGER_MAX_MESSAGE_LEN];   /* Log message text */
    rt_bool_t is_valid;                     /* Entry validity flag */
} LogEntry;

/*==========================================================================*/
/* Logger Active Object Structure */
/*==========================================================================*/

/* Logger Active Object class definition */
typedef struct {
    QActive super;                          /* Base QActive class */
    
    /* Circular buffer for log entries */
    LogEntry log_buffer[LOGGER_MAX_ENTRIES]; /* Circular buffer for log entries */
    uint8_t write_index;                    /* Current write position in buffer */
    uint8_t read_index;                     /* Current read position in buffer */
    uint8_t entry_count;                    /* Number of valid entries in buffer */
    rt_bool_t buffer_full;                  /* Flag indicating buffer overflow */
    
    /* Logger configuration */
    uint8_t current_log_level;              /* Current minimum log level */
    uint8_t logger_mode;                    /* Current logger operation mode */
    uint8_t output_targets;                 /* Active output target bitmask */
    rt_bool_t timestamps_enabled;           /* Timestamp inclusion flag */
    
    /* Performance tracking variables */
    uint32_t total_log_operations;          /* Total logging operations performed */
    uint32_t messages_logged[LOG_LEVEL_OFF]; /* Messages logged per level */
    uint32_t messages_filtered;             /* Messages filtered due to level */
    uint32_t messages_dropped;              /* Messages dropped due to buffer full */
    uint32_t buffer_overflows;              /* Number of buffer overflow events */
    
    /* Timing and performance metrics */
    uint32_t min_log_time;                  /* Minimum logging processing time */
    uint32_t max_log_time;                  /* Maximum logging processing time */
    uint32_t avg_log_time;                  /* Average logging processing time */
    uint32_t total_log_time;                /* Cumulative logging time */
    
    /* Logger statistics */
    uint32_t flush_operations;              /* Number of flush operations */
    uint32_t clear_operations;              /* Number of clear operations */
    uint32_t next_sequence_id;              /* Next sequence ID to assign */
    
    /* Error tracking */
    uint32_t format_errors;                 /* Message formatting errors */
    uint32_t output_errors;                 /* Output operation errors */
    uint32_t buffer_errors;                 /* Buffer management errors */
    
} LoggerAO;

/*==========================================================================*/
/* Logger States Enumeration */
/*==========================================================================*/

/* Logger AO state enumeration for state machine implementation */
typedef enum {
    LOGGER_STATE_INITIAL,                   /* Initial state before operation */
    LOGGER_STATE_IDLE,                      /* Idle state waiting for log events */
    LOGGER_STATE_LOGGING,                   /* Active logging state */
    LOGGER_STATE_FLUSHING,                  /* Flushing log buffer to output */
    LOGGER_STATE_ERROR,                     /* Error state for exception handling */
    LOGGER_STATE_REPORTING,                 /* Reporting logger statistics */
    LOGGER_STATE_MAX                        /* Maximum state value */
} LoggerState;

/*==========================================================================*/
/* Logger Event Structures */
/*==========================================================================*/

/* Enhanced logger event with message and metadata */
typedef struct {
    QEvt super;                             /* Base QEvt structure */
    char message[LOGGER_MAX_MESSAGE_LEN];   /* Log message text */
    uint8_t log_level;                      /* Message log level */
    uint8_t source_id;                      /* Source identifier */
    uint32_t timestamp;                     /* Event timestamp */
    rt_bool_t force_output;                 /* Force immediate output flag */
} LoggerEvtEx;

/* Logger flush event structure */
typedef struct {
    QEvt super;                             /* Base QEvt structure */
    uint8_t output_targets;                 /* Target outputs for flush */
    rt_bool_t clear_after_flush;            /* Clear buffer after flush */
    uint32_t timestamp;                     /* Event timestamp */
} LoggerFlushEvt;

/* Logger configuration event structure */
typedef struct {
    QEvt super;                             /* Base QEvt structure */
    uint8_t new_log_level;                  /* New minimum log level */
    uint8_t new_mode;                       /* New logger operation mode */
    uint8_t new_output_targets;             /* New output target configuration */
    rt_bool_t enable_timestamps;            /* Timestamp enable/disable */
} LoggerConfigEvt;

/* Logger statistics request event */
typedef struct {
    QEvt super;                             /* Base QEvt structure */
    uint32_t request_id;                    /* Statistics request identifier */
    rt_bool_t include_entries;              /* Include log entries in report */
    rt_bool_t reset_after_report;           /* Reset statistics after reporting */
} LoggerStatsEvt;

/*==========================================================================*/
/* Global Logger AO Instance */
/*==========================================================================*/

/* Global Logger Active Object instance - accessible from other modules */
extern LoggerAO g_logger;

/*==========================================================================*/
/* Logger AO Public Interface Functions */
/*==========================================================================*/

/* Constructor function to initialize Logger AO */
void LoggerAO_ctor(LoggerAO *const me);

/* Log a message with specified level and source */
rt_bool_t LoggerAO_log(LoggerAO *const me, uint8_t level, uint8_t source_id, const char *message);

/* Log a formatted message with variable arguments */
rt_bool_t LoggerAO_logf(LoggerAO *const me, uint8_t level, uint8_t source_id, const char *format, ...);

/* Set logger configuration parameters */
void LoggerAO_setConfig(LoggerAO *const me, uint8_t log_level, uint8_t mode, uint8_t output_targets);

/* Get current logger configuration */
void LoggerAO_getConfig(LoggerAO const *const me, uint8_t *log_level, uint8_t *mode, uint8_t *output_targets);

/* Flush log buffer to output targets */
void LoggerAO_flush(LoggerAO *const me, uint8_t output_targets);

/* Clear log buffer */
void LoggerAO_clear(LoggerAO *const me);

/* Get number of log entries in buffer */
uint8_t LoggerAO_getEntryCount(LoggerAO const *const me);

/* Check if buffer is full */
rt_bool_t LoggerAO_isBufferFull(LoggerAO const *const me);

/* Get logger operation statistics */
void LoggerAO_getStats(LoggerAO const *const me,
                       uint32_t *total_ops,
                       uint32_t *filtered,
                       uint32_t *dropped,
                       uint32_t *overflows);

/* Get logger performance metrics */
void LoggerAO_getPerformanceMetrics(LoggerAO const *const me,
                                    uint32_t *min_time,
                                    uint32_t *max_time,
                                    uint32_t *avg_time);

/* Get logger error statistics */
void LoggerAO_getErrorStats(LoggerAO const *const me,
                            uint32_t *format_errors,
                            uint32_t *output_errors,
                            uint32_t *buffer_errors);

/* Get log messages for specific level */
uint32_t LoggerAO_getMessagesForLevel(LoggerAO const *const me, uint8_t level);

/* Reset logger statistics and performance metrics */
void LoggerAO_resetStats(LoggerAO *const me);

/*==========================================================================*/
/* Logger AO Utility Functions */
/*==========================================================================*/

/* Convert log level to string representation */
const char* LoggerAO_levelToString(uint8_t level);

/* Convert log mode to string representation */
const char* LoggerAO_modeToString(uint8_t mode);

/* Validate log level value */
rt_bool_t LoggerAO_isLevelValid(uint8_t level);

/* Format timestamp for log entries */
void LoggerAO_formatTimestamp(uint32_t timestamp, char *buffer, rt_size_t buffer_size);

/* Format log entry for output */
void LoggerAO_formatLogEntry(LogEntry const *entry, char *buffer, rt_size_t buffer_size);

/* Calculate log processing time */
uint32_t LoggerAO_calculateLogTime(uint32_t start_time, uint32_t end_time);

/* Print buffer contents to console */
void LoggerAO_printBuffer(LoggerAO const *const me);

/* Get buffer usage percentage */
uint8_t LoggerAO_getBufferUsage(LoggerAO const *const me);

/*==========================================================================*/
/* Convenience Logging Macros */
/*==========================================================================*/

/* Convenience macros for different log levels */
#define LOG_DEBUG(source, msg)      LoggerAO_log(&g_logger, LOG_LEVEL_DEBUG, source, msg)
#define LOG_INFO(source, msg)       LoggerAO_log(&g_logger, LOG_LEVEL_INFO, source, msg)
#define LOG_WARN(source, msg)       LoggerAO_log(&g_logger, LOG_LEVEL_WARN, source, msg)
#define LOG_ERROR(source, msg)      LoggerAO_log(&g_logger, LOG_LEVEL_ERROR, source, msg)
#define LOG_CRITICAL(source, msg)   LoggerAO_log(&g_logger, LOG_LEVEL_CRITICAL, source, msg)

/* Formatted logging macros */
#define LOGF_DEBUG(source, fmt, ...)    LoggerAO_logf(&g_logger, LOG_LEVEL_DEBUG, source, fmt, ##__VA_ARGS__)
#define LOGF_INFO(source, fmt, ...)     LoggerAO_logf(&g_logger, LOG_LEVEL_INFO, source, fmt, ##__VA_ARGS__)
#define LOGF_WARN(source, fmt, ...)     LoggerAO_logf(&g_logger, LOG_LEVEL_WARN, source, fmt, ##__VA_ARGS__)
#define LOGF_ERROR(source, fmt, ...)    LoggerAO_logf(&g_logger, LOG_LEVEL_ERROR, source, fmt, ##__VA_ARGS__)
#define LOGF_CRITICAL(source, fmt, ...) LoggerAO_logf(&g_logger, LOG_LEVEL_CRITICAL, source, fmt, ##__VA_ARGS__)

/*==========================================================================*/
/* MSH Shell Command Functions for Logger AO */
/*==========================================================================*/

/* MSH command to log a test message */
void logger_log_cmd(int argc, char *argv[]);

/* MSH command to set log level */
void logger_set_level_cmd(int argc, char *argv[]);

/* MSH command to flush log buffer */
void logger_flush_cmd(void);

/* MSH command to clear log buffer */
void logger_clear_cmd(void);

/* MSH command to show logger buffer contents */
void logger_show_cmd(void);

/* MSH command to show logger statistics */
void logger_stats_cmd(void);

/* MSH command to reset logger statistics */
void logger_reset_stats_cmd(void);

/* MSH command to show logger performance metrics */
void logger_perf_cmd(void);

/* MSH command to show logger configuration */
void logger_config_cmd(void);

/* MSH command to set logger mode */
void logger_set_mode_cmd(int argc, char *argv[]);

#endif /* LOGGER_AO_H_ */