/*============================================================================
* Product: Jitter Performance Test
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
/* Jitter Test Active Object */
/*==========================================================================*/

typedef struct {
    QActive super;
    QTimeEvt timerEvt;
    QTimeEvt measureEvt;
    uint32_t expected_interval;
    uint32_t last_time;
    uint32_t min_jitter;
    uint32_t max_jitter;
    uint32_t total_jitter;
    uint32_t jitter_count;
    rt_bool_t first_measurement;
} JitterAO;

static JitterAO l_jitterAO;

/* State function declarations */
static QState JitterAO_initial(JitterAO * const me, QEvt const * const e);
static QState JitterAO_idle(JitterAO * const me, QEvt const * const e);
static QState JitterAO_measuring(JitterAO * const me, QEvt const * const e);

/* Load thread functions */
static void load_thread1_func(void *parameter);
static void load_thread2_func(void *parameter);
static rt_thread_t load_thread1 = RT_NULL;
static rt_thread_t load_thread2 = RT_NULL;

/*==========================================================================*/
/* Active Object Constructor */
/*==========================================================================*/

static void JitterAO_ctor(void) {
    JitterAO *me = &l_jitterAO;
    
    QActive_ctor(&me->super, Q_STATE_CAST(&JitterAO_initial));
    QTimeEvt_ctorX(&me->timerEvt, &me->super, JITTER_TIMER_SIG, 0U);
    QTimeEvt_ctorX(&me->measureEvt, &me->super, JITTER_TIMEOUT_SIG, 0U);
    
    me->expected_interval = 100; /* 100 ticks = 1 second */
    me->last_time = 0;
    me->min_jitter = 0xFFFFFFFF;
    me->max_jitter = 0;
    me->total_jitter = 0;
    me->jitter_count = 0;
    me->first_measurement = RT_TRUE;
}

/*==========================================================================*/
/* Load Thread Functions */
/*==========================================================================*/

static void load_thread1_func(void *parameter) {
    (void)parameter;
    
    volatile uint32_t dummy = 0;
    
    while (!g_stopLoadThreads) {
        /* Create some CPU load */
        for (volatile int i = 0; i < 1000; i++) {
            dummy = dummy * 2 + 1;
        }
        
        rt_thread_mdelay(10);
    }
    
    rt_kprintf("Load thread 1 exiting\n");
}

static void load_thread2_func(void *parameter) {
    (void)parameter;
    
    volatile uint32_t dummy = 0;
    
    while (!g_stopLoadThreads) {
        /* Create some CPU load */
        for (volatile int i = 0; i < 500; i++) {
            dummy = dummy ^ (dummy << 1);
        }
        
        rt_thread_mdelay(15);
    }
    
    rt_kprintf("Load thread 2 exiting\n");
}

/*==========================================================================*/
/* State Functions */
/*==========================================================================*/

static QState JitterAO_initial(JitterAO * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            /* Subscribe to jitter test signals */
            QActive_subscribe(&me->super, JITTER_START_SIG);
            QActive_subscribe(&me->super, JITTER_STOP_SIG);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&JitterAO_idle);
        }
        default: {
            return Q_SUPER(&QHsm_top);
        }
    }
}

static QState JitterAO_idle(JitterAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Jitter Test: Idle state\n");
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
        
        case JITTER_START_SIG: {
            rt_kprintf("Jitter Test: Starting jitter measurements\n");
            
            /* Reset measurement counters */
            g_jitter_measurements = 0;
            me->min_jitter = 0xFFFFFFFF;
            me->max_jitter = 0;
            me->total_jitter = 0;
            me->jitter_count = 0;
            me->first_measurement = RT_TRUE;
            g_stopLoadThreads = RT_FALSE;
            
            /* Reset and start DWT cycle counter */
            PerfCommon_resetDWT();
            me->last_time = PerfCommon_getDWTCycles();
            
            /* Arm timeout timer (10 seconds) */
            QTimeEvt_armX(&me->measureEvt, 10 * 100, 0); /* 10 seconds */
            
            /* Start periodic timer for jitter measurement */
            QTimeEvt_armX(&me->timerEvt, me->expected_interval, me->expected_interval);
            
            /* Create load threads to introduce jitter */
            load_thread1 = rt_thread_create("load1",
                                           load_thread1_func,
                                           RT_NULL,
                                           2048,
                                           LOAD_THREAD_PRIO,
                                           20);
            if (load_thread1 != RT_NULL) {
                rt_thread_startup(load_thread1);
            }
            
            load_thread2 = rt_thread_create("load2",
                                           load_thread2_func,
                                           RT_NULL,
                                           2048,
                                           LOAD_THREAD_PRIO + 1,
                                           20);
            if (load_thread2 != RT_NULL) {
                rt_thread_startup(load_thread2);
            }
            
            status = Q_TRAN(&JitterAO_measuring);
            break;
        }
        
        case JITTER_STOP_SIG: {
            rt_kprintf("Jitter Test: Stopping\n");
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

static QState JitterAO_measuring(JitterAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Jitter Test: Measuring state\n");
            status = Q_HANDLED();
            break;
        }
        
        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->timerEvt);
            QTimeEvt_disarm(&me->measureEvt);
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
        
        case JITTER_TIMER_SIG: {
            uint32_t current_time = PerfCommon_getDWTCycles();
            
            if (!me->first_measurement) {
                uint32_t actual_interval = current_time - me->last_time;
                uint32_t jitter;
                
                if (actual_interval > me->expected_interval) {
                    jitter = actual_interval - me->expected_interval;
                } else {
                    jitter = me->expected_interval - actual_interval;
                }
                
                /* Update statistics */
                if (jitter < me->min_jitter) {
                    me->min_jitter = jitter;
                }
                if (jitter > me->max_jitter) {
                    me->max_jitter = jitter;
                }
                me->total_jitter += jitter;
                me->jitter_count++;
                g_jitter_measurements++;
                
                /* Send jitter measurement event */
                JitterEvt *evt = Q_NEW(JitterEvt, JITTER_MEASURE_SIG);
                evt->timestamp = current_time;
                evt->expected_time = me->expected_interval;
                evt->actual_time = actual_interval;
                
                QACTIVE_POST(&me->super, &evt->super, me);
            } else {
                me->first_measurement = RT_FALSE;
            }
            
            me->last_time = current_time;
            status = Q_HANDLED();
            break;
        }
        
        case JITTER_MEASURE_SIG: {
            JitterEvt const *evt = (JitterEvt const *)e;
            
            /* Process jitter measurement */
            if (g_jitter_measurements % 100 == 0) {
                rt_kprintf("Jitter measurement %u: expected=%u, actual=%u\n",
                          g_jitter_measurements, evt->expected_time, evt->actual_time);
            }
            
            status = Q_HANDLED();
            break;
        }
        
        case JITTER_TIMEOUT_SIG: {
            rt_kprintf("Jitter Test: Timeout reached\n");
            
            /* Set stop flag for load threads */
            g_stopLoadThreads = RT_TRUE;
            
            /* Wait for threads to stop and then delete them */
            PerfCommon_waitForThreads();
            if (load_thread1 != RT_NULL) {
                rt_thread_delete(load_thread1);
                load_thread1 = RT_NULL;
            }
            if (load_thread2 != RT_NULL) {
                rt_thread_delete(load_thread2);
                load_thread2 = RT_NULL;
            }
            
            /* Calculate and print results */
            uint32_t avg_jitter = (me->jitter_count > 0) ? 
                                  (me->total_jitter / me->jitter_count) : 0;
            
            rt_kprintf("=== Jitter Test Results ===\n");
            rt_kprintf("Measurements: %u\n", g_jitter_measurements);
            rt_kprintf("Expected interval: %u cycles\n", me->expected_interval);
            rt_kprintf("Min jitter: %u cycles\n", me->min_jitter);
            rt_kprintf("Max jitter: %u cycles\n", me->max_jitter);
            rt_kprintf("Avg jitter: %u cycles\n", avg_jitter);
            rt_kprintf("Total jitter: %u cycles\n", me->total_jitter);
            
            status = Q_TRAN(&JitterAO_idle);
            break;
        }
        
        case JITTER_STOP_SIG: {
            rt_kprintf("Jitter Test: Stopping test\n");
            QTimeEvt_disarm(&me->timerEvt);
            QTimeEvt_disarm(&me->measureEvt);
            
            g_stopLoadThreads = RT_TRUE;
            
            /* Wait for threads to stop and then delete them */
            PerfCommon_waitForThreads();
            if (load_thread1 != RT_NULL) {
                rt_thread_delete(load_thread1);
                load_thread1 = RT_NULL;
            }
            if (load_thread2 != RT_NULL) {
                rt_thread_delete(load_thread2);
                load_thread2 = RT_NULL;
            }
            
            status = Q_TRAN(&JitterAO_idle);
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
/* Jitter Test Public Functions */
/*==========================================================================*/

void JitterTest_start(void) {
    static QEvt const *jitter_queueSto[15];
    static uint8_t jitter_stack[2048];
    
    /* Initialize common performance test infrastructure */
    PerfCommon_initTest();
    
    /* Initialize only the jitter event pool */
    PerfCommon_initJitterPool();
    
    /* Initialize QF */
    QF_init();
    
    /* Construct the jitter AO */
    JitterAO_ctor();
    
    /* Start the jitter AO */
    QACTIVE_START(&l_jitterAO.super,
                  JITTER_AO_PRIO,
                  jitter_queueSto, Q_DIM(jitter_queueSto),
                  jitter_stack, sizeof(jitter_stack),
                  (void *)0);
    
    /* Send start signal */
    QACTIVE_POST(&l_jitterAO.super, Q_NEW(QEvt, JITTER_START_SIG), &l_jitterAO);
    
    /* Run the test */
    QF_run();
}

void JitterTest_stop(void) {
    /* Send stop signal */
    QACTIVE_POST(&l_jitterAO.super, Q_NEW(QEvt, JITTER_STOP_SIG), &l_jitterAO);
    
    /* Unsubscribe from signals to prevent lingering subscriptions */
    QActive_unsubscribe(&l_jitterAO.super, JITTER_START_SIG);
    QActive_unsubscribe(&l_jitterAO.super, JITTER_STOP_SIG);
    
    /* Cleanup common infrastructure */
    PerfCommon_cleanupTest();
    
    /* Print final results */
    PerfCommon_printResults("Jitter", g_jitter_measurements);
}

/* RT-Thread MSH command exports */
#ifdef RT_USING_FINSH
#include <finsh.h>
MSH_CMD_EXPORT(JitterTest_start, start jitter performance test);
MSH_CMD_EXPORT(JitterTest_stop, stop jitter performance test);
#endif