/*============================================================================
* Product: QActive Demo RT-Thread Integration Implementation
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

#include "rt_integration.h"
#include "config_proxy.h"
#include <finsh.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef QPC_USING_QACTIVE_DEMO_NONBLOCK

/*==========================================================================*/
/* Global RT-Thread Synchronization Objects - For system threads only */
/*==========================================================================*/
static rt_mutex_t g_system_mutex = RT_NULL;  /* Only for system threads */
static rt_sem_t g_storage_sem = RT_NULL;     /* Only for storage thread */
static rt_event_t g_system_event = RT_NULL;  /* Only for system threads */

/*==========================================================================*/
/* Shared Data Structures - Event-driven access only */
/*==========================================================================*/
SharedConfig g_shared_config = {
    .sensor_rate = 200U,       /* Default 2 second intervals */
    .storage_interval = 1000U, /* Default 10 second intervals */
    .system_flags = 0U
};

SystemStats g_system_stats = {0};

/*==========================================================================*/
/* RT-Thread Thread Handles */
/*==========================================================================*/
rt_thread_t storage_thread = RT_NULL;
rt_thread_t shell_thread = RT_NULL;

/*==========================================================================*/
/* Storage Thread Implementation - Independent of AO context */
/*==========================================================================*/
void storage_thread_entry(void *parameter) {
    rt_uint32_t events;
    rt_uint32_t save_counter = 0;

    (void)parameter;

    rt_kprintf("Storage: Thread started - Managing local data storage\n");

    /* Signal that storage is ready */
    if (g_system_event != RT_NULL) {
        rt_event_send(g_system_event, RT_EVENT_STORAGE_READY);
    }

    /* Main storage loop */
    while (1) {
        /* Wait for storage semaphore or timeout */
        if (rt_sem_take(g_storage_sem, g_shared_config.storage_interval) == RT_EOK) {
            rt_kprintf("Storage: Triggered save operation\n");
        }

        /* Periodic data save operation */
        save_counter++;
        rt_kprintf("Storage: Saving data batch %u to local storage\n", save_counter);

        /* Simulate file system operations */
        rt_thread_mdelay(30);

        /* Update statistics - no AO involvement */
        if (g_system_mutex != RT_NULL) {
            rt_mutex_take(g_system_mutex, RT_WAITING_FOREVER);
            g_system_stats.storage_saves++;
            rt_mutex_release(g_system_mutex);
        }

        rt_kprintf("Storage: Save completed (total: %u)\n",
                  g_system_stats.storage_saves);

        /* Send health update to system */
        if (g_system_event != RT_NULL) {
            rt_event_send(g_system_event, RT_EVENT_HEALTH_CHECK);
        }

        /* Check for system events */
        if (g_system_event != RT_NULL) {
            if (rt_event_recv(g_system_event, RT_EVENT_SYSTEM_ERROR,
                             RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                             0, &events) == RT_EOK) {
                rt_kprintf("Storage: System error detected, initiating recovery\n");

                if (g_system_mutex != RT_NULL) {
                    rt_mutex_take(g_system_mutex, RT_WAITING_FOREVER);
                    g_system_stats.errors++;
                    rt_mutex_release(g_system_mutex);
                }
            }
        }

        /* Small delay to prevent busy waiting */
        rt_thread_mdelay(50);
    }
}

/*==========================================================================*/
/* Shell Thread Implementation - Independent of AO context */
/*==========================================================================*/
void shell_thread_entry(void *parameter) {
    (void)parameter;

    rt_kprintf("Shell: Thread started - RT-Thread MSH commands available\n");

    /* Signal that shell is ready */
    if (g_system_event != RT_NULL) {
        rt_event_send(g_system_event, RT_EVENT_SHELL_READY);
    }

    /* Shell thread mainly handles command processing */
    /* The actual commands are handled by MSH exports */
    while (1) {
        /* Monitor system events and provide shell feedback */
        rt_uint32_t events;

        if (g_system_event != RT_NULL) {
            if (rt_event_recv(g_system_event,
                             RT_EVENT_STORAGE_READY |
                             RT_EVENT_QACTIVE_READY | RT_EVENT_HEALTH_CHECK,
                             RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                             1000, &events) == RT_EOK) {

                if (events & RT_EVENT_HEALTH_CHECK) {
                    rt_kprintf("Shell: System health check completed\n");
                }
            }
        }

        /* Periodic shell status update */
        rt_thread_mdelay(5000); /* 5 second intervals */
    }
}

/*==========================================================================*/
/* RT-Thread Integration Functions */
/*==========================================================================*/
int rt_integration_init(void) {
    rt_kprintf("RT-Integration: Initializing RT-Thread components\n");

    /* Create mutex for shared configuration - system threads only */
    g_system_mutex = rt_mutex_create("sys_mutex", RT_IPC_FLAG_FIFO);
    if (g_system_mutex == RT_NULL) {
        rt_kprintf("RT-Integration: Failed to create system mutex\n");
        return -1;
    }

    /* Create semaphore for storage coordination - system threads only */
    g_storage_sem = rt_sem_create("stor_sem", 0, RT_IPC_FLAG_FIFO);
    if (g_storage_sem == RT_NULL) {
        rt_kprintf("RT-Integration: Failed to create storage semaphore\n");
        return -1;
    }

    /* Create event set for system notifications - system threads only */
    g_system_event = rt_event_create("sys_event", RT_IPC_FLAG_FIFO);
    if (g_system_event == RT_NULL) {
        rt_kprintf("RT-Integration: Failed to create system event\n");
        return -1;
    }

    /* Initialize proxy threads */
    config_init();
    storage_init();

    rt_kprintf("RT-Integration: All synchronization objects created successfully\n");
    return 0;
}

int rt_integration_start(void) {
    rt_kprintf("RT-Integration: Starting RT-Thread components\n");

    /* Create and start storage thread */
    storage_thread = rt_thread_create("storage", storage_thread_entry, RT_NULL,
                                     2048, 10, 10);
    if (storage_thread != RT_NULL) {
        rt_thread_startup(storage_thread);
    } else {
        rt_kprintf("RT-Integration: Failed to create storage thread\n");
        return -1;
    }

    rt_kprintf("RT-Integration: Storage thread started successfully\n");

    /* Create and start shell thread */
    shell_thread = rt_thread_create("shell", shell_thread_entry, RT_NULL,
                                   1024, 11, 10);
    if (shell_thread != RT_NULL) {
        rt_thread_startup(shell_thread);
    } else {
        rt_kprintf("RT-Integration: Failed to create shell thread\n");
        return -1;
    }

    rt_kprintf("RT-Integration: All RT-Thread components started successfully\n");
    return 0;
}

int rt_integration_stop(void) {
    rt_kprintf("RT-Integration: Stopping RT-Thread components\n");

    /* Note: RT-Thread doesn't have a direct thread stop function */
    /* In a real implementation, threads would check a stop flag */

    return 0;
}

void rt_integration_get_stats(SystemStats *stats) {
    if (stats != RT_NULL) {
        if (g_system_mutex != RT_NULL) {
            rt_mutex_take(g_system_mutex, RT_WAITING_FOREVER);
            *stats = g_system_stats;
            rt_mutex_release(g_system_mutex);
        } else {
            *stats = g_system_stats;  /* Fallback if mutex not available */
        }
    }
}

/*==========================================================================*/
/* MSH Command Implementations - Using event-driven approach */
/*==========================================================================*/
int qactive_start_cmd(int argc, char** argv) {
    (void)argc;
    (void)argv;

    rt_kprintf("QActive: Starting QActive components\n");

    /* Send start signal to sensor */
    QACTIVE_POST(AO_Sensor, Q_NEW(QEvt, SENSOR_READ_SIG), 0U);

    /* Send start signal to processor */
    QACTIVE_POST(AO_Processor, Q_NEW(QEvt, PROCESSOR_START_SIG), 0U);

    rt_kprintf("QActive: Start commands sent to QActive components\n");
    return 0;
}

int qactive_stop_cmd(int argc, char** argv) {
    (void)argc;
    (void)argv;

    rt_kprintf("QActive: Stopping QActive components (simulation)\n");

    /* In a real implementation, this would send stop signals */
    rt_kprintf("QActive: Stop simulation completed\n");
    return 0;
}

int qactive_stats_cmd(int argc, char** argv) {
    (void)argc;
    (void)argv;

    SystemStats stats;
    rt_integration_get_stats(&stats);

    rt_kprintf("=== QActive Demo System Statistics ===\n");
    rt_kprintf("Sensor Readings:       %u\n", stats.sensor_readings);
    rt_kprintf("Processed Data:        %u\n", stats.processed_data);
    rt_kprintf("Storage Saves:         %u\n", stats.storage_saves);
    rt_kprintf("Health Checks:         %u\n", stats.health_checks);
    rt_kprintf("Errors:                %u\n", stats.errors);
    rt_kprintf("================================\n");

    return 0;
}

int qactive_config_cmd(int argc, char** argv) {
    if (argc < 3) {
        rt_kprintf("Usage: qactive_control config <sensor_rate> <storage_interval> [flags]\n");
        rt_kprintf("Current config: sensor=%u, storage=%u\n",
                  g_shared_config.sensor_rate,
                  g_shared_config.storage_interval);
        return 0;
    }

    if (g_system_mutex != RT_NULL) {
        rt_mutex_take(g_system_mutex, RT_WAITING_FOREVER);
    }

    if (argc >= 3) {
        g_shared_config.sensor_rate = atoi(argv[2]);
    }
    if (argc >= 4) {
        g_shared_config.storage_interval = atoi(argv[3]);
    }
    if (argc >= 5) {
        g_shared_config.system_flags = atoi(argv[4]);
    }

    if (g_system_mutex != RT_NULL) {
        rt_mutex_release(g_system_mutex);
    }

    /* Signal configuration update */
    if (g_system_event != RT_NULL) {
        rt_event_send(g_system_event, RT_EVENT_CONFIG_UPDATED);
    }

    rt_kprintf("QActive: Configuration updated - sensor=%u, storage=%u, flags=%u\n",
              g_shared_config.sensor_rate,
              g_shared_config.storage_interval,
              g_shared_config.system_flags);

    return 0;
}

int system_status_cmd(int argc, char** argv) {
    (void)argc;
    (void)argv;

    rt_kprintf("=== System Status ===\n");
    rt_kprintf("Storage Thread:  %s\n", (storage_thread != RT_NULL) ? "Running" : "Stopped");
    rt_kprintf("Shell Thread:    %s\n", (shell_thread != RT_NULL) ? "Running" : "Stopped");

    SystemStats stats;
    rt_integration_get_stats(&stats);
    rt_kprintf("System Health:   %s\n", (stats.errors == 0) ? "Healthy" : "Errors Detected");

    rt_kprintf("====================\n");

    return 0;
}

int system_reset_cmd(int argc, char** argv) {
    (void)argc;
    (void)argv;

    rt_kprintf("System: Resetting statistics\n");

    if (g_system_mutex != RT_NULL) {
        rt_mutex_take(g_system_mutex, RT_WAITING_FOREVER);
        rt_memset(&g_system_stats, 0, sizeof(SystemStats));
        rt_mutex_release(g_system_mutex);
    } else {
        rt_memset(&g_system_stats, 0, sizeof(SystemStats));
    }

    rt_kprintf("System: Statistics reset completed\n");
    return 0;
}

/* RT-Thread MSH command exports */
MSH_CMD_EXPORT(qactive_start_cmd, Start QActive components);
MSH_CMD_EXPORT(qactive_stop_cmd, Stop QActive components);
MSH_CMD_EXPORT(qactive_stats_cmd, Show QActive system statistics);
MSH_CMD_EXPORT(qactive_config_cmd, Configure QActive parameters);
MSH_CMD_EXPORT(system_status_cmd, Show system status);
MSH_CMD_EXPORT(system_reset_cmd, Reset system statistics);

#endif /* QPC_USING_QACTIVE_DEMO_NONBLOCK */
