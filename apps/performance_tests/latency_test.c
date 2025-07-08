/*============================================================================
* Product: Latency Performance Test
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
/* Latency Test Active Object */
/*==========================================================================*/

typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    uint32_t start_time;
    uint32_t end_time;
    uint32_t min_latency;
    uint32_t max_latency;
    uint32_t total_latency;
    uint32_t sequence_counter;
} LatencyAO;

static LatencyAO l_latencyAO;

/* State function declarations */
static QState LatencyAO_initial(LatencyAO * const me, QEvt const * const e);
static QState LatencyAO_idle(LatencyAO * const me, QEvt const * const e);
static QState LatencyAO_testing(LatencyAO * const me, QEvt const * const e);

/*==========================================================================*/
/* Active Object Constructor */
/*==========================================================================*/

static void LatencyAO_ctor(void) {
    LatencyAO *me = &l_latencyAO;
    
    QActive_ctor(&me->super, Q_STATE_CAST(&LatencyAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, LATENCY_TIMEOUT_SIG, 0U);
    
    me->start_time = 0;
    me->end_time = 0;
    me->min_latency = 0xFFFFFFFF;
    me->max_latency = 0;
    me->total_latency = 0;
    me->sequence_counter = 0;
}

/*==========================================================================*/
/* State Functions */
/*==========================================================================*/

static QState LatencyAO_initial(LatencyAO * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            /* Subscribe to latency test signals */
            QActive_subscribe(&me->super, LATENCY_START_SIG);
            QActive_subscribe(&me->super, LATENCY_END_SIG);
            QActive_subscribe(&me->super, LATENCY_MEASURE_SIG);
            QActive_subscribe(&me->super, LATENCY_STOP_SIG);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&LatencyAO_idle);
        }
        default: {
            return Q_SUPER(&QHsm_top);
        }
    }
}

static QState LatencyAO_idle(LatencyAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Latency Test: Idle state\n");
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
        
        case LATENCY_START_SIG: {
            rt_kprintf("Latency Test: Starting latency measurements\n");
            
            /* Reset measurement counters */
            g_latency_measurements = 0;
            me->min_latency = 0xFFFFFFFF;
            me->max_latency = 0;
            me->total_latency = 0;
            me->sequence_counter = 0;
            
            /* Reset and start DWT cycle counter */
            PerfCommon_resetDWT();
            
            /* Arm timeout timer (10 seconds) */
            QTimeEvt_armX(&me->timeEvt, 10 * 100, 0); /* 10 seconds */
            
            status = Q_TRAN(&LatencyAO_testing);
            break;
        }
        
        case LATENCY_STOP_SIG: {
            rt_kprintf("Latency Test: Stopping\n");
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

static QState LatencyAO_testing(LatencyAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Latency Test: Testing state\n");
            
            /* Start the first measurement */
            LatencyEvt *evt = Q_NEW(LatencyEvt, LATENCY_MEASURE_SIG);
            evt->timestamp = PerfCommon_getDWTCycles();
            evt->sequence_id = ++me->sequence_counter;
            
            QACTIVE_POST(&me->super, &evt->super, me);
            
            status = Q_HANDLED();
            break;
        }
        
        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->timeEvt);
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
        
        case LATENCY_MEASURE_SIG: {
            LatencyEvt const *evt = (LatencyEvt const *)e;
            uint32_t current_time = PerfCommon_getDWTCycles();
            uint32_t latency = current_time - evt->timestamp;
            
            /* Update statistics */
            if (latency < me->min_latency) {
                me->min_latency = latency;
            }
            if (latency > me->max_latency) {
                me->max_latency = latency;
            }
            me->total_latency += latency;
            g_latency_measurements++;
            
            /* Send next measurement if not stopping */
            if (g_latency_measurements < 1000) {
                LatencyEvt *next_evt = Q_NEW(LatencyEvt, LATENCY_MEASURE_SIG);
                next_evt->timestamp = current_time;
                next_evt->sequence_id = ++me->sequence_counter;
                
                QACTIVE_POST(&me->super, &next_evt->super, me);
            }
            
            status = Q_HANDLED();
            break;
        }
        
        case LATENCY_TIMEOUT_SIG: {
            rt_kprintf("Latency Test: Timeout reached\n");
            
            /* Calculate and print results */
            uint32_t avg_latency = (g_latency_measurements > 0) ? 
                                   (me->total_latency / g_latency_measurements) : 0;
            
            rt_kprintf("=== Latency Test Results ===\n");
            rt_kprintf("Measurements: %u\n", g_latency_measurements);
            rt_kprintf("Min latency: %u cycles\n", me->min_latency);
            rt_kprintf("Max latency: %u cycles\n", me->max_latency);
            rt_kprintf("Avg latency: %u cycles\n", avg_latency);
            rt_kprintf("Total latency: %u cycles\n", me->total_latency);
            
            status = Q_TRAN(&LatencyAO_idle);
            break;
        }
        
        case LATENCY_STOP_SIG: {
            rt_kprintf("Latency Test: Stopping test\n");
            QTimeEvt_disarm(&me->timeEvt);
            status = Q_TRAN(&LatencyAO_idle);
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
/* Latency Test Static Variables - Persistent for RT-Thread integration */
/*==========================================================================*/

static QEvt const *latency_queueSto[10];
static uint8_t latency_stack[1024];  /* Reduced stack size for embedded */
static rt_bool_t test_running = RT_FALSE;

/*==========================================================================*/
/* Latency Test Public Functions */
/*==========================================================================*/

void LatencyTest_start(void) {
    /* Prevent multiple simultaneous test instances */
    if (test_running) {
        rt_kprintf("Latency test already running\n");
        return;
    }
    
    /* Initialize common performance test infrastructure */
    PerfCommon_initTest();
    
    /* Initialize only the latency event pool */
    PerfCommon_initLatencyPool();
    
    /* Initialize QF if not already done - safe to call multiple times in RT-Thread */
    QF_init();
    
    /* Construct the latency AO */
    LatencyAO_ctor();
    
    /* Start the latency AO */
    QACTIVE_START(&l_latencyAO.super,
                  LATENCY_AO_PRIO,
                  latency_queueSto, Q_DIM(latency_queueSto),
                  latency_stack, sizeof(latency_stack),
                  (void *)0);
    
    /* Initialize QF framework (returns immediately in RT-Thread) */
    QF_run();
    
    /* Mark test as running */
    test_running = RT_TRUE;
    
    /* Send start signal */
    QACTIVE_POST(&l_latencyAO.super, Q_NEW(QEvt, LATENCY_START_SIG), &l_latencyAO);
    
    rt_kprintf("Latency test started successfully\n");
}

void LatencyTest_stop(void) {
    if (!test_running) {
        rt_kprintf("Latency test not running\n");
        return;
    }
    
    /* Send stop signal */
    QACTIVE_POST(&l_latencyAO.super, Q_NEW(QEvt, LATENCY_STOP_SIG), &l_latencyAO);
    
    /* Give time for stop signal to be processed */
    rt_thread_mdelay(100);
    
    /* Stop the Active Object */
    QActive_stop(&l_latencyAO.super);
    
    /* Unsubscribe from signals to prevent lingering subscriptions */
    QActive_unsubscribe(&l_latencyAO.super, LATENCY_START_SIG);
    QActive_unsubscribe(&l_latencyAO.super, LATENCY_END_SIG);
    QActive_unsubscribe(&l_latencyAO.super, LATENCY_MEASURE_SIG);
    QActive_unsubscribe(&l_latencyAO.super, LATENCY_STOP_SIG);
    
    /* Mark test as stopped */
    test_running = RT_FALSE;
    
    /* Cleanup common infrastructure */
    PerfCommon_cleanupTest();
    
    /* Print final results */
    PerfCommon_printResults("Latency", g_latency_measurements);
    
    rt_kprintf("Latency test stopped successfully\n");
}

/* RT-Thread MSH command exports */
#ifdef RT_USING_FINSH
#include <finsh.h>
MSH_CMD_EXPORT(LatencyTest_start, start latency performance test);
MSH_CMD_EXPORT(LatencyTest_stop, stop latency performance test);
#endif