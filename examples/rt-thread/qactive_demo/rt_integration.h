/*============================================================================
* Product: QActive Demo RT-Thread Integration Header
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
#ifndef RT_INTEGRATION_H_
#define RT_INTEGRATION_H_

#include <rtthread.h>
#include "qactive_demo.h"

/*==========================================================================*/
/* RT-Thread Integration Signals */
/*==========================================================================*/
enum RTIntegrationSignals {
    /* Storage Thread signals */
    STORAGE_SAVE_SIG = MAX_DEMO_SIG,
    STORAGE_STATUS_SIG,
    
    /* Shell Thread signals */
    SHELL_COMMAND_SIG,
    SHELL_RESPONSE_SIG,
    
    /* System-wide notification signals */
    SYSTEM_HEALTH_SIG,
    SYSTEM_CONFIG_SIG,
    
    MAX_RT_INTEGRATION_SIG
};

/*==========================================================================*/
/* RT-Thread Integration Events */
/*==========================================================================*/
typedef struct {
    QEvt super;
    uint32_t data_id;
    uint32_t size;
    uint32_t checksum;
} StorageSaveEvt;

typedef struct {
    QEvt super;
    uint32_t command_id;
    char command_str[64];
    uint32_t param1;
    uint32_t param2;
} ShellCommandEvt;

typedef struct {
    QEvt super;
    uint32_t system_status;
    uint32_t qxk_health;
    uint32_t rt_health;
} SystemHealthEvt;

/*==========================================================================*/
/* RT-Thread Synchronization Objects */
/*==========================================================================*/
/* Global synchronization objects for RT-Thread/QXK communication */
extern rt_mutex_t g_config_mutex;      /* Mutex for shared configuration */
extern rt_sem_t g_storage_sem;         /* Semaphore for storage coordination */
extern rt_event_t g_system_event;      /* Event set for system notifications */

/*==========================================================================*/
/* Shared Data Structures */
/*==========================================================================*/
/* Configuration data protected by mutex */
typedef struct {
    uint32_t sensor_rate;
    uint32_t storage_interval;
    uint32_t system_flags;
} SharedConfig;

extern SharedConfig g_shared_config;

/* System statistics shared between QXK and RT-Thread */
typedef struct {
    uint32_t sensor_readings;
    uint32_t processed_data;
    uint32_t storage_saves;
    uint32_t health_checks;
    uint32_t errors;
} SystemStats;

extern SystemStats g_system_stats;

/*==========================================================================*/
/* RT-Thread Thread Declarations */
/*==========================================================================*/
/* Storage Thread - Manages local data storage */
void storage_thread_entry(void *parameter);
extern rt_thread_t storage_thread;

/* Shell Thread - Provides RT-Thread MSH commands */
void shell_thread_entry(void *parameter);
extern rt_thread_t shell_thread;

/*==========================================================================*/
/* RT-Thread Integration Functions */
/*==========================================================================*/
/* Initialize RT-Thread components */
int rt_integration_init(void);

/* Start RT-Thread components */
int rt_integration_start(void);

/* Stop RT-Thread components */
int rt_integration_stop(void);

/* Get system statistics */
void rt_integration_get_stats(SystemStats *stats);

/*==========================================================================*/
/* RT-Thread Event Flags */
/*==========================================================================*/
#define RT_EVENT_STORAGE_READY     (1 << 0)
#define RT_EVENT_SHELL_READY       (1 << 1)
#define RT_EVENT_QACTIVE_READY     (1 << 2)
#define RT_EVENT_SYSTEM_ERROR      (1 << 3)
#define RT_EVENT_CONFIG_UPDATED    (1 << 4)
#define RT_EVENT_DATA_AVAILABLE    (1 << 5)
#define RT_EVENT_HEALTH_CHECK      (1 << 6)

/*==========================================================================*/
/* MSH Command Prototypes */
/*==========================================================================*/
/* QActive control commands */
int qactive_start_cmd(int argc, char** argv);
int qactive_stop_cmd(int argc, char** argv);
int qactive_stats_cmd(int argc, char** argv);
int qactive_config_cmd(int argc, char** argv);

/* System control commands */
int system_status_cmd(int argc, char** argv);
int system_reset_cmd(int argc, char** argv);

#endif /* RT_INTEGRATION_H_ */