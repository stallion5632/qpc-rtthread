/*============================================================================
* Product: Performance Tests Framework - Main Implementation
* Last updated for version 7.3.0
* Last updated on  2025-07-18
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2025 Quantum Leaps, LLC. All rights reserved.
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
#include "perf_test.h"
#include "app_main.h"
#include "bsp.h"
#include <finsh.h>
#include <stdio.h>

Q_DEFINE_THIS_MODULE("perf_main")

/*==========================================================================*/
/* Performance Test Framework Initialization */
/*==========================================================================*/

/* QF event pools for performance testing (used for event allocation) */
static QF_MPOOL_EL(QEvt) l_smlPoolSto[100U];

/* Framework initialization flag (ensures single init) */
static rt_bool_t l_framework_initialized = RT_FALSE;

/* Global mutexes for thread safety (protects log/stats) */
rt_mutex_t g_log_mutex = RT_NULL;
rt_mutex_t g_stats_mutex = RT_NULL;

/*==========================================================================*/
/* Performance Test Framework API */
/*==========================================================================*/

/*
 * Initialize the performance test framework.
 * Sets up BSP, event pools, and global mutexes.
 */
void PerformanceFramework_init(void)
{
    rt_kprintf("[Perf Framework] Initializing performance test framework...\n");

    /* Initialize BSP */
    BSP_init();

    /* Create global mutexes for thread safety */
    if (g_log_mutex == RT_NULL)
    {
        g_log_mutex = rt_mutex_create("log_mtx", RT_IPC_FLAG_PRIO);
        Q_ASSERT(g_log_mutex != RT_NULL);
    }

    if (g_stats_mutex == RT_NULL)
    {
        g_stats_mutex = rt_mutex_create("stats_mtx", RT_IPC_FLAG_PRIO);
        Q_ASSERT(g_stats_mutex != RT_NULL);
    }

    /* Initialize QF framework only once */
    if (!l_framework_initialized)
    {
        QF_init();

        /* Initialize event pools */
        QF_poolInit(l_smlPoolSto, sizeof(l_smlPoolSto), sizeof(QEvt));

        l_framework_initialized = RT_TRUE;
    }

    rt_kprintf("[Perf Framework] Initialization complete\n");
}

/*
 * MSH command to initialize framework (manual trigger)
 */
static int cmd_perf_init(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    PerformanceFramework_init();
    rt_kprintf("Performance test framework initialized\n");

    return 0;
}

/*
 * Auto-initialization function called during RT-Thread startup
 */
static int perf_framework_auto_init(void)
{
    PerformanceFramework_init();
    rt_kprintf("[Perf Framework] Auto-initialization complete\n");
    return 0;
}

/* Export MSH commands for manual framework init */
MSH_CMD_EXPORT(cmd_perf_init, Initialize performance test framework);

/* Auto-initialize during system startup */
INIT_APP_EXPORT(perf_framework_auto_init);
