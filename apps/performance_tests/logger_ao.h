/*============================================================================
* Product: Logger Active Object Header
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
#ifndef LOGGER_AO_H_
#define LOGGER_AO_H_

#include "app_main.h"

/*==========================================================================*/
/* Logger AO Configuration */
/*==========================================================================*/
#define LOGGER_QUEUE_SIZE       (32U)     /* Event queue size (larger for logs) */
#define LOGGER_STACK_SIZE       (1024U)   /* Stack size in bytes (larger for string processing) */

/*==========================================================================*/
/* Logger Log Levels */
/*==========================================================================*/
typedef enum {
    LOG_LEVEL_DEBUG = 0U,
    LOG_LEVEL_INFO  = 1U,
    LOG_LEVEL_WARN  = 2U,
    LOG_LEVEL_ERROR = 3U
} LogLevel;

/*==========================================================================*/
/* Logger AO State Machine */
/*==========================================================================*/
typedef struct {
    QActive super;              /* Inherit from QActive */
    QTimeEvt flushTimeEvt;     /* Time event for periodic log flushing */
    uint32_t log_count;         /* Total number of logs processed */
    uint32_t debug_count;       /* Number of debug messages */
    uint32_t info_count;        /* Number of info messages */
    uint32_t warn_count;        /* Number of warning messages */
    uint32_t error_count;       /* Number of error messages */
    rt_bool_t is_active;       /* Active state flag */
} LoggerAO;

/*==========================================================================*/
/* Logger AO Public Interface */
/*==========================================================================*/

/* Constructor */
void LoggerAO_ctor(void);

/* Get the singleton instance */
LoggerAO *LoggerAO_getInstance(void);

/* Thread-safe logging functions */
void LoggerAO_logDebug(const char *format, ...);
void LoggerAO_logInfo(const char *format, ...);
void LoggerAO_logWarn(const char *format, ...);
void LoggerAO_logError(const char *format, ...);

/* Get log statistics (thread-safe) */
uint32_t LoggerAO_getLogCount(void);
uint32_t LoggerAO_getDebugCount(void);
uint32_t LoggerAO_getInfoCount(void);
uint32_t LoggerAO_getWarnCount(void);
uint32_t LoggerAO_getErrorCount(void);

/* Check if logger is active */
rt_bool_t LoggerAO_isActive(void);

/* Reset log counters */
void LoggerAO_resetCounters(void);

#endif /* LOGGER_AO_H_ */