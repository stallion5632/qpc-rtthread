/*============================================================================
* Product: Idle CPU Performance Test
* Last updated for version 7.2.0
* Last updated on  2024-07-08
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
#include "perf_common.h"

Q_DEFINE_THIS_FILE

/*==========================================================================*/
/* Idle CPU Test Active Object */
/*==========================================================================*/

typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    uint32_t start_time;
    uint32_t end_time;
    uint32_t total_cycles;
    uint32_t measurement_count;
    uint32_t cpu_utilization;
} IdleCpuAO;

static IdleCpuAO l_idleCpuAO;

/* State function declarations */
static QState IdleCpuAO_initial(IdleCpuAO * const me, QEvt const * const e);
static QState IdleCpuAO_idle(IdleCpuAO * const me, QEvt const * const e);
static QState IdleCpuAO_measuring(IdleCpuAO * const me, QEvt const * const e);

/* Idle monitoring QXThread */
typedef struct {
    QXThread super;
    uint32_t monitor_count;
} IdleMonitorThread;

static IdleMonitorThread l_idleMonitorThread;
static void IdleMonitorThread_run(QXThread * const me);

/* CPU load QXThread */
typedef struct {
    QXThread super;
    uint32_t load_count;
} CpuLoadThread;

static CpuLoadThread l_cpuLoadThread;
static void CpuLoadThread_run(QXThread * const me);

/*==========================================================================*/
/* Active Object Constructor */
/*==========================================================================*/

static void IdleCpuAO_ctor(void) {
    IdleCpuAO *me = &l_idleCpuAO;
    
    QActive_ctor(&me->super, Q_STATE_CAST(&IdleCpuAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, IDLE_CPU_TIMEOUT_SIG, 0U);
    
    me->start_time = 0;
    me->end_time = 0;
    me->total_cycles = 0;
    me->measurement_count = 0;
    me->cpu_utilization = 0;
}

/*==========================================================================*/
/* QXThread Constructors */
/*==========================================================================*/

static void IdleMonitorThread_ctor(void) {
    IdleMonitorThread *me = &l_idleMonitorThread;
    
    QXThread_ctor(&me->super, &IdleMonitorThread_run, 0);
    me->monitor_count = 0;
}

static void CpuLoadThread_ctor(void) {
    CpuLoadThread *me = &l_cpuLoadThread;
    
    QXThread_ctor(&me->super, &CpuLoadThread_run, 0);
    me->load_count = 0;
}

/*==========================================================================*/
/* QXThread Functions */
/*==========================================================================*/

static void IdleMonitorThread_run(QXThread * const me) {
    IdleMonitorThread *imt = (IdleMonitorThread *)me;
    
    rt_kprintf("Idle Monitor QXThread: Started\n");
    
    uint32_t last_idle_count = 0;
    uint32_t last_time = 0;
    
    while (!g_stopLoadThreads) {
        uint32_t current_time = PerfCommon_getDWTCycles();
        uint32_t current_idle_count = g_idle_count;
        
        if (last_time != 0) {
            uint32_t idle_delta = current_idle_count - last_idle_count;
            
            /* Send idle measurement event */
            IdleCpuEvt *evt = Q_NEW(IdleCpuEvt, IDLE_CPU_MEASURE_SIG);
            evt->timestamp = current_time;
            evt->idle_count = idle_delta;
            
            QACTIVE_POST(&l_idleCpuAO.super, &evt->super, me);
            
            imt->monitor_count++;
        }
        
        last_time = current_time;
        last_idle_count = current_idle_count;
        
        /* Sample every 100ms */
        QXThread_delay(10); /* 100ms delay using QXK API */
    }
    
    rt_kprintf("Idle Monitor QXThread: Exiting, monitored %u times\n", imt->monitor_count);
}

static void CpuLoadThread_run(QXThread * const me) {
    CpuLoadThread *clt = (CpuLoadThread *)me;
    
    rt_kprintf("CPU Load QXThread: Started\n");
    
    volatile uint32_t dummy = 0;
    
    while (!g_stopLoadThreads) {
        /* Create varying CPU load */
        for (volatile int i = 0; i < 2000; i++) {
            dummy = dummy * 3 + i;
        }
        
        /* Pause to allow some idle time */
        QXThread_delay(5); /* 50ms delay using QXK API */
        
        /* More intensive load */
        for (volatile int i = 0; i < 1000; i++) {
            dummy = dummy ^ (dummy << 2);
        }
        
        QXThread_delay(3); /* 30ms delay using QXK API */
        
        clt->load_count++;
    }
    
    rt_kprintf("CPU Load QXThread: Exiting, load cycles: %u\n", clt->load_count);
}

/*==========================================================================*/
/* State Functions */
/*==========================================================================*/

static QState IdleCpuAO_initial(IdleCpuAO * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            /* Subscribe to idle CPU test signals */
            QActive_subscribe(&me->super, IDLE_CPU_START_SIG);
            QActive_subscribe(&me->super, IDLE_CPU_STOP_SIG);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&IdleCpuAO_idle);
        }
        default: {
            return Q_SUPER(&QHsm_top);
        }
    }
}

static QState IdleCpuAO_idle(IdleCpuAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Idle CPU Test: Idle state\n");
            status = Q_HANDLED();
            break;
        }
        
        case Q_EXIT_SIG: {
            status = Q_HANDLED();
            break;
        }
        
        case Q_INIT_SIG: {
            status = Q_HANDLED();
            break;
        }
        
        case Q_EMPTY_SIG: {
            status = Q_HANDLED();
            break;
        }
        
        case IDLE_CPU_START_SIG: {
            rt_kprintf("Idle CPU Test: Starting idle CPU measurements\n");
            
            /* Reset measurement counters */
            g_idle_count = 0;
            me->measurement_count = 0;
            me->total_cycles = 0;
            me->cpu_utilization = 0;
            g_stopLoadThreads = RT_FALSE;
            
            /* Reset and start DWT cycle counter */
            PerfCommon_resetDWT();
            me->start_time = PerfCommon_getDWTCycles();
            
            /* Arm timeout timer (10 seconds) */
            QTimeEvt_armX(&me->timeEvt, 10 * 100, 0); /* 10 seconds */
            
            /* Create idle monitor QXThread with smaller stack for embedded */
            IdleMonitorThread_ctor();
            
            /* Create CPU load QXThread */
            CpuLoadThread_ctor();
            
            /* Start QXThreads */
            static QEvt const *idleMonitorQueue[10];
            static uint8_t idleMonitorStack[1024];
            static QEvt const *cpuLoadQueue[10];
            static uint8_t cpuLoadStack[2048];
            
            QXTHREAD_START(&l_idleMonitorThread.super,
                          LOAD_THREAD_PRIO,
                          idleMonitorQueue, Q_DIM(idleMonitorQueue),
                          idleMonitorStack, sizeof(idleMonitorStack),
                          (void *)0);
            
            QXTHREAD_START(&l_cpuLoadThread.super,
                          LOAD_THREAD_PRIO + 1,
                          cpuLoadQueue, Q_DIM(cpuLoadQueue),
                          cpuLoadStack, sizeof(cpuLoadStack),
                          (void *)0);
            
            status = Q_TRAN(&IdleCpuAO_measuring);
            break;
        }
        
        case IDLE_CPU_STOP_SIG: {
            rt_kprintf("Idle CPU Test: Stopping\n");
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

static QState IdleCpuAO_measuring(IdleCpuAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Idle CPU Test: Measuring state\n");
            status = Q_HANDLED();
            break;
        }
        
        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->timeEvt);
            g_stopLoadThreads = RT_TRUE;
            status = Q_HANDLED();
            break;
        }
        
        case Q_INIT_SIG: {
            status = Q_HANDLED();
            break;
        }
        
        case Q_EMPTY_SIG: {
            status = Q_HANDLED();
            break;
        }
        
        case IDLE_CPU_MEASURE_SIG: {
            IdleCpuEvt const *evt = (IdleCpuEvt const *)e;
            
            me->measurement_count++;
            me->total_cycles += evt->idle_count;
            
            /* Calculate CPU utilization (simplified) */
            if (me->measurement_count % 10 == 0) {
                uint32_t avg_idle = me->total_cycles / me->measurement_count;
                rt_kprintf("Idle CPU measurement %u: avg_idle=%u\n",
                          me->measurement_count, avg_idle);
            }
            
            status = Q_HANDLED();
            break;
        }
        
        case IDLE_CPU_TIMEOUT_SIG: {
            rt_kprintf("Idle CPU Test: Timeout reached\n");
            
            /* Set stop flag for threads */
            g_stopLoadThreads = RT_TRUE;
            
            /* Wait for threads to stop and then delete them */
            PerfCommon_waitForThreads();
            if (idle_monitor_thread != RT_NULL) {
                rt_thread_delete(idle_monitor_thread);
                idle_monitor_thread = RT_NULL;
            }
            if (cpu_load_thread != RT_NULL) {
                rt_thread_delete(cpu_load_thread);
                cpu_load_thread = RT_NULL;
            }
            
            me->end_time = PerfCommon_getDWTCycles();
            
            /* Calculate and print results */
            uint32_t test_duration = me->end_time - me->start_time;
            uint32_t avg_idle = (me->measurement_count > 0) ? 
                               (me->total_cycles / me->measurement_count) : 0;
            
            rt_kprintf("=== Idle CPU Test Results ===\n");
            rt_kprintf("Test duration: %u cycles\n", test_duration);
            rt_kprintf("Measurements: %u\n", me->measurement_count);
            rt_kprintf("Total idle count: %u\n", g_idle_count);
            rt_kprintf("Average idle per measurement: %u\n", avg_idle);
            rt_kprintf("Total cycles measured: %u\n", me->total_cycles);
            
            status = Q_TRAN(&IdleCpuAO_idle);
            break;
        }
        
        case IDLE_CPU_STOP_SIG: {
            rt_kprintf("Idle CPU Test: Stopping test\n");
            QTimeEvt_disarm(&me->timeEvt);
            
            g_stopLoadThreads = RT_TRUE;
            
            /* Wait for threads to stop and then delete them */
            PerfCommon_waitForThreads();
            if (idle_monitor_thread != RT_NULL) {
                rt_thread_delete(idle_monitor_thread);
                idle_monitor_thread = RT_NULL;
            }
            if (cpu_load_thread != RT_NULL) {
                rt_thread_delete(cpu_load_thread);
                cpu_load_thread = RT_NULL;
            }
            
            status = Q_TRAN(&IdleCpuAO_idle);
            break;
        }
        
        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }
    
    return status;
}

/*==========================================================================*/
/* Idle Hook Function */
/*==========================================================================*/

/* This function is called by the RT-Thread idle thread */
void rt_hw_idle_hook(void) {
    /* Increment idle counter - volatile ensures atomic visibility */
    g_idle_count++;
}

/*==========================================================================*/
/* Idle CPU Test Static Variables - Persistent for RT-Thread integration */
/*==========================================================================*/

static QEvt const *idle_cpu_queueSto[10];    /* Reduced queue size */
static uint8_t idle_cpu_stack[1024];         /* Reduced stack size for embedded */
static rt_bool_t idle_cpu_test_running = RT_FALSE;

/*==========================================================================*/
/* Idle CPU Test Public Functions */
/*==========================================================================*/

void IdleCpuTest_start(void) {
    /* Prevent multiple simultaneous test instances */
    if (idle_cpu_test_running) {
        rt_kprintf("Idle CPU test already running\n");
        return;
    }
    
    /* Initialize common performance test infrastructure */
    PerfCommon_initTest();
    
    /* Initialize only the idle CPU event pool */
    PerfCommon_initIdleCpuPool();
    
    /* Initialize QF if not already done - safe to call multiple times in RT-Thread */
    QF_init();
    
    /* Construct the idle CPU AO */
    IdleCpuAO_ctor();
    
    /* Start the idle CPU AO */
    QACTIVE_START(&l_idleCpuAO.super,
                  IDLE_CPU_AO_PRIO,
                  idle_cpu_queueSto, Q_DIM(idle_cpu_queueSto),
                  idle_cpu_stack, sizeof(idle_cpu_stack),
                  (void *)0);
    
    /* Initialize QF framework (returns immediately in RT-Thread) */
    QF_run();
    
    /* Mark test as running */
    idle_cpu_test_running = RT_TRUE;
    
    /* Send start signal */
    QACTIVE_POST(&l_idleCpuAO.super, Q_NEW(QEvt, IDLE_CPU_START_SIG), &l_idleCpuAO);
    
    rt_kprintf("Idle CPU test started successfully\n");
}

void IdleCpuTest_stop(void) {
    if (!idle_cpu_test_running) {
        rt_kprintf("Idle CPU test not running\n");
        return;
    }
    
    /* Send stop signal */
    QACTIVE_POST(&l_idleCpuAO.super, Q_NEW(QEvt, IDLE_CPU_STOP_SIG), &l_idleCpuAO);
    
    /* Give time for stop signal to be processed */
    rt_thread_mdelay(200);
    
    /* Unsubscribe from signals to prevent lingering subscriptions */
    QActive_unsubscribe(&l_idleCpuAO.super, IDLE_CPU_START_SIG);
    QActive_unsubscribe(&l_idleCpuAO.super, IDLE_CPU_STOP_SIG);
    
    /* Mark test as stopped */
    idle_cpu_test_running = RT_FALSE;
    
    /* Cleanup common infrastructure */
    PerfCommon_cleanupTest();
    
    /* Print final results */
    PerfCommon_printResults("Idle CPU", g_idle_count);
    
    rt_kprintf("Idle CPU test stopped successfully\n");
}

/* RT-Thread MSH command exports */
#ifdef RT_USING_FINSH
#include <finsh.h>
MSH_CMD_EXPORT(IdleCpuTest_start, start idle CPU performance test);
MSH_CMD_EXPORT(IdleCpuTest_stop, stop idle CPU performance test);
#endif