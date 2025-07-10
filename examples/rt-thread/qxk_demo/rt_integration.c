/*============================================================================
* Product: QXK Demo RT-Thread Integration Implementation
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
#include <finsh.h>
#include <stdlib.h>

#ifdef QPC_USING_QXK_DEMO

Q_DEFINE_THIS_FILE

/*==========================================================================*/
/* Global RT-Thread Synchronization Objects */
/*==========================================================================*/
rt_mq_t g_network_queue = RT_NULL;
rt_mutex_t g_config_mutex = RT_NULL;
rt_sem_t g_storage_sem = RT_NULL;
rt_event_t g_system_event = RT_NULL;

/*==========================================================================*/
/* Shared Data Structures */
/*==========================================================================*/
SharedConfig g_shared_config = {
    .sensor_rate = 200,      /* Default 2 second intervals */
    .network_interval = 500, /* Default 5 second intervals */
    .storage_interval = 1000, /* Default 10 second intervals */
    .system_flags = 0
};

SystemStats g_system_stats = {0};

/*==========================================================================*/
/* RT-Thread Thread Handles */
/*==========================================================================*/
rt_thread_t network_thread = RT_NULL;
rt_thread_t storage_thread = RT_NULL;
rt_thread_t shell_thread = RT_NULL;

/*==========================================================================*/
/* Network Thread Implementation */
/*==========================================================================*/
void network_thread_entry(void *parameter) {
    NetworkDataEvt *msg;
    rt_uint32_t events;
    
    (void)parameter;
    
    rt_kprintf("Network: Thread started - Simulating Wi-Fi/Ethernet connectivity\n");
    
    /* Signal that network is ready */
    rt_event_send(g_system_event, RT_EVENT_NETWORK_READY);
    
    /* Main network loop */
    while (1) {
        /* Wait for data from message queue with timeout */
        if (rt_mq_recv(g_network_queue, (void*)&msg, sizeof(NetworkDataEvt*), 
                      g_shared_config.network_interval) == RT_EOK) {
            
            rt_kprintf("Network: Received data %u from source %u, transmitting to cloud\n", 
                      msg->data, msg->source_id);
            
            /* Simulate network transmission delay */
            rt_thread_mdelay(50);
            
            /* Update statistics */
            rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
            g_system_stats.network_transmissions++;
            rt_mutex_release(g_config_mutex);
            
            rt_kprintf("Network: Data transmitted successfully (total: %u)\n", 
                      g_system_stats.network_transmissions);
            
            /* Send notification that data was sent */
            rt_event_send(g_system_event, RT_EVENT_DATA_AVAILABLE);
            
            /* Free the message */
            rt_free(msg);
        }
        
        /* Check for configuration updates */
        if (rt_event_recv(g_system_event, RT_EVENT_CONFIG_UPDATED, 
                         RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 
                         0, &events) == RT_EOK) {
            rt_kprintf("Network: Configuration updated, new interval = %u\n", 
                      g_shared_config.network_interval);
            
            /* Send config update to QXK Processor */
            NetworkConfigEvt *config_evt = Q_NEW(NetworkConfigEvt, NETWORK_CONFIG_SIG);
            config_evt->sensor_rate = g_shared_config.sensor_rate;
            config_evt->network_interval = g_shared_config.network_interval;
            config_evt->storage_interval = g_shared_config.storage_interval;
            
            QACTIVE_POST(AO_Processor, &config_evt->super, network_thread);
        }
        
        /* Periodic health check */
        rt_thread_mdelay(100);
    }
}

/*==========================================================================*/
/* Storage Thread Implementation */
/*==========================================================================*/
void storage_thread_entry(void *parameter) {
    rt_uint32_t events;
    rt_uint32_t save_counter = 0;
    
    (void)parameter;
    
    rt_kprintf("Storage: Thread started - Managing local data storage\n");
    
    /* Signal that storage is ready */
    rt_event_send(g_system_event, RT_EVENT_STORAGE_READY);
    
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
        
        /* Update statistics */
        rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
        g_system_stats.storage_saves++;
        
        /* Coordinate with QXK Monitor for health check */
        SystemHealthEvt *health_evt = Q_NEW(SystemHealthEvt, SYSTEM_HEALTH_SIG);
        health_evt->system_status = (g_system_stats.errors == 0) ? 1 : 0;
        health_evt->qxk_health = 1; /* Assume QXK is healthy */
        health_evt->rt_health = 1;  /* RT-Thread is healthy */
        rt_mutex_release(g_config_mutex);
        
        rt_kprintf("Storage: Save completed (total: %u), health status: %u\n", 
                  g_system_stats.storage_saves, health_evt->system_status);
        
        /* Send health update to system */
        rt_event_send(g_system_event, RT_EVENT_HEALTH_CHECK);
        
        /* Free the health event */
        Q_GC(&health_evt->super);
        
        /* Check for system events */
        if (rt_event_recv(g_system_event, RT_EVENT_SYSTEM_ERROR, 
                         RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 
                         0, &events) == RT_EOK) {
            rt_kprintf("Storage: System error detected, initiating recovery\n");
            
            rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
            g_system_stats.errors++;
            rt_mutex_release(g_config_mutex);
        }
        
        /* Small delay to prevent busy waiting */
        rt_thread_mdelay(50);
    }
}

/*==========================================================================*/
/* Shell Thread Implementation */
/*==========================================================================*/
void shell_thread_entry(void *parameter) {
    (void)parameter;
    
    rt_kprintf("Shell: Thread started - RT-Thread MSH commands available\n");
    
    /* Signal that shell is ready */
    rt_event_send(g_system_event, RT_EVENT_SHELL_READY);
    
    /* Shell thread mainly handles command processing */
    /* The actual commands are handled by MSH exports */
    while (1) {
        /* Monitor system events and provide shell feedback */
        rt_uint32_t events;
        
        if (rt_event_recv(g_system_event, 
                         RT_EVENT_NETWORK_READY | RT_EVENT_STORAGE_READY | 
                         RT_EVENT_QXK_READY | RT_EVENT_HEALTH_CHECK,
                         RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                         1000, &events) == RT_EOK) {
            
            if (events & RT_EVENT_HEALTH_CHECK) {
                rt_kprintf("Shell: System health check completed\n");
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
    
    /* Create message queue for network data */
    g_network_queue = rt_mq_create("net_mq", sizeof(NetworkDataEvt*), 10, RT_IPC_FLAG_FIFO);
    if (g_network_queue == RT_NULL) {
        rt_kprintf("RT-Integration: Failed to create network message queue\n");
        return -1;
    }
    
    /* Create mutex for shared configuration */
    g_config_mutex = rt_mutex_create("cfg_mutex", RT_IPC_FLAG_FIFO);
    if (g_config_mutex == RT_NULL) {
        rt_kprintf("RT-Integration: Failed to create configuration mutex\n");
        return -1;
    }
    
    /* Create semaphore for storage coordination */
    g_storage_sem = rt_sem_create("stor_sem", 0, RT_IPC_FLAG_FIFO);
    if (g_storage_sem == RT_NULL) {
        rt_kprintf("RT-Integration: Failed to create storage semaphore\n");
        return -1;
    }
    
    /* Create event set for system notifications */
    g_system_event = rt_event_create("sys_event", RT_IPC_FLAG_FIFO);
    if (g_system_event == RT_NULL) {
        rt_kprintf("RT-Integration: Failed to create system event\n");
        return -1;
    }
    
    rt_kprintf("RT-Integration: All synchronization objects created successfully\n");
    return 0;
}

int rt_integration_start(void) {
    rt_kprintf("RT-Integration: Starting RT-Thread components\n");
    
    /* Create and start network thread */
    network_thread = rt_thread_create("network", network_thread_entry, RT_NULL,
                                     2048, 10, 10);
    if (network_thread != RT_NULL) {
        rt_thread_startup(network_thread);
    } else {
        rt_kprintf("RT-Integration: Failed to create network thread\n");
        return -1;
    }
    
    /* Create and start storage thread */
    storage_thread = rt_thread_create("storage", storage_thread_entry, RT_NULL,
                                     2048, 11, 10);
    if (storage_thread != RT_NULL) {
        rt_thread_startup(storage_thread);
    } else {
        rt_kprintf("RT-Integration: Failed to create storage thread\n");
        return -1;
    }
    
    /* Create and start shell thread */
    shell_thread = rt_thread_create("shell", shell_thread_entry, RT_NULL,
                                   1024, 12, 10);
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
        rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
        *stats = g_system_stats;
        rt_mutex_release(g_config_mutex);
    }
}

/*==========================================================================*/
/* MSH Command Implementations */
/*==========================================================================*/
int qxk_start_cmd(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    rt_kprintf("QXK: Starting QXK components\n");
    
    /* Send start signal to sensor */
    QACTIVE_POST(AO_Sensor, Q_NEW(QEvt, SENSOR_READ_SIG), shell_thread);
    
    /* Send start signal to processor */
    QACTIVE_POST(AO_Processor, Q_NEW(QEvt, PROCESSOR_START_SIG), shell_thread);
    
    rt_kprintf("QXK: Start commands sent to QXK components\n");
    return 0;
}

int qxk_stop_cmd(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    rt_kprintf("QXK: Stopping QXK components (simulation)\n");
    
    /* In a real implementation, this would send stop signals */
    rt_kprintf("QXK: Stop simulation completed\n");
    return 0;
}

int qxk_stats_cmd(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    SystemStats stats;
    rt_integration_get_stats(&stats);
    
    rt_kprintf("=== QXK Demo System Statistics ===\n");
    rt_kprintf("Sensor Readings:       %u\n", stats.sensor_readings);
    rt_kprintf("Processed Data:        %u\n", stats.processed_data);
    rt_kprintf("Network Transmissions: %u\n", stats.network_transmissions);
    rt_kprintf("Storage Saves:         %u\n", stats.storage_saves);
    rt_kprintf("Health Checks:         %u\n", stats.health_checks);
    rt_kprintf("Errors:                %u\n", stats.errors);
    rt_kprintf("================================\n");
    
    return 0;
}

int qxk_config_cmd(int argc, char** argv) {
    if (argc < 2) {
        rt_kprintf("Usage: qxk_config <sensor_rate> [network_interval] [storage_interval]\n");
        rt_kprintf("Current config: sensor=%u, network=%u, storage=%u\n",
                  g_shared_config.sensor_rate,
                  g_shared_config.network_interval,
                  g_shared_config.storage_interval);
        return 0;
    }
    
    rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    
    if (argc >= 2) {
        g_shared_config.sensor_rate = atoi(argv[1]);
    }
    if (argc >= 3) {
        g_shared_config.network_interval = atoi(argv[2]);
    }
    if (argc >= 4) {
        g_shared_config.storage_interval = atoi(argv[3]);
    }
    
    rt_mutex_release(g_config_mutex);
    
    /* Signal configuration update */
    rt_event_send(g_system_event, RT_EVENT_CONFIG_UPDATED);
    
    rt_kprintf("QXK: Configuration updated - sensor=%u, network=%u, storage=%u\n",
              g_shared_config.sensor_rate,
              g_shared_config.network_interval,
              g_shared_config.storage_interval);
    
    return 0;
}

int system_status_cmd(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    rt_kprintf("=== System Status ===\n");
    rt_kprintf("Network Thread:  %s\n", (network_thread != RT_NULL) ? "Running" : "Stopped");
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
    
    rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
    rt_memset(&g_system_stats, 0, sizeof(SystemStats));
    rt_mutex_release(g_config_mutex);
    
    rt_kprintf("System: Statistics reset completed\n");
    return 0;
}

/* RT-Thread MSH command exports */
MSH_CMD_EXPORT(qxk_start_cmd, Start QXK components);
MSH_CMD_EXPORT(qxk_stop_cmd, Stop QXK components);
MSH_CMD_EXPORT(qxk_stats_cmd, Show QXK system statistics);
MSH_CMD_EXPORT(qxk_config_cmd, Configure QXK parameters);
MSH_CMD_EXPORT(system_status_cmd, Show system status);
MSH_CMD_EXPORT(system_reset_cmd, Reset system statistics);

#endif /* QPC_USING_QXK_DEMO */