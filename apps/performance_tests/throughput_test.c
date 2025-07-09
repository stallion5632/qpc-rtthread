/*============================================================================
* Product: Throughput Performance Test
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
/* Throughput Test Active Objects */
/*==========================================================================*/

typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    uint32_t packets_sent;
    uint32_t start_time;
    uint32_t packet_counter;
} ThroughputProducerAO;

typedef struct {
    QActive super;
    uint32_t packets_received;
    uint32_t total_data_received;
    uint32_t start_time;
    uint32_t end_time;
} ThroughputConsumerAO;

static ThroughputProducerAO l_producerAO;
static ThroughputConsumerAO l_consumerAO;

/* State function declarations */
static QState ThroughputProducerAO_initial(ThroughputProducerAO * const me, QEvt const * const e);
static QState ThroughputProducerAO_idle(ThroughputProducerAO * const me, QEvt const * const e);
static QState ThroughputProducerAO_producing(ThroughputProducerAO * const me, QEvt const * const e);

static QState ThroughputConsumerAO_initial(ThroughputConsumerAO * const me, QEvt const * const e);
static QState ThroughputConsumerAO_idle(ThroughputConsumerAO * const me, QEvt const * const e);
static QState ThroughputConsumerAO_consuming(ThroughputConsumerAO * const me, QEvt const * const e);

/* Producer thread function */
static void producer_thread_func(void *parameter);
static rt_thread_t producer_thread = RT_NULL;

/*==========================================================================*/
/* Active Object Constructors */
/*==========================================================================*/

static void ThroughputProducerAO_ctor(void) {
    ThroughputProducerAO *me = &l_producerAO;
    
    QActive_ctor(&me->super, Q_STATE_CAST(&ThroughputProducerAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, THROUGHPUT_TIMEOUT_SIG, 0U);
    
    me->packets_sent = 0;
    me->start_time = 0;
    me->packet_counter = 0;
}

static void ThroughputConsumerAO_ctor(void) {
    ThroughputConsumerAO *me = &l_consumerAO;
    
    QActive_ctor(&me->super, Q_STATE_CAST(&ThroughputConsumerAO_initial));
    
    me->packets_received = 0;
    me->total_data_received = 0;
    me->start_time = 0;
    me->end_time = 0;
}

/*==========================================================================*/
/* Producer State Functions */
/*==========================================================================*/

static QState ThroughputProducerAO_initial(ThroughputProducerAO * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            /* Subscribe to throughput test signals */
            QActive_subscribe(&me->super, THROUGHPUT_START_SIG);
            QActive_subscribe(&me->super, THROUGHPUT_STOP_SIG);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&ThroughputProducerAO_idle);
        }
        default: {
            return Q_SUPER(&QHsm_top);
        }
    }
}

static QState ThroughputProducerAO_idle(ThroughputProducerAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Throughput Producer: Idle state\n");
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
        
        case THROUGHPUT_START_SIG: {
            rt_kprintf("Throughput Producer: Starting throughput test\n");
            
            /* Reset counters */
            me->packets_sent = 0;
            me->packet_counter = 0;
            g_throughput_measurements = 0;
            g_stopProducer = RT_FALSE;
            
            /* Reset and start DWT cycle counter */
            PerfCommon_resetDWT();
            me->start_time = PerfCommon_getDWTCycles();
            
            /* Arm timeout timer (10 seconds) */
            QTimeEvt_armX(&me->timeEvt, 10 * 100, 0); /* 10 seconds */
            
            /* Create producer thread with smaller stack for embedded */
            producer_thread = rt_thread_create("producer",
                                               producer_thread_func,
                                               RT_NULL,
                                               1024,  /* Reduced from 2048 */
                                               LOAD_THREAD_PRIO,
                                               20);
            if (producer_thread != RT_NULL) {
                rt_thread_startup(producer_thread);
            }
            
            status = Q_TRAN(&ThroughputProducerAO_producing);
            break;
        }
        
        case THROUGHPUT_STOP_SIG: {
            rt_kprintf("Throughput Producer: Stopping\n");
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

static QState ThroughputProducerAO_producing(ThroughputProducerAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Throughput Producer: Producing state\n");
            status = Q_HANDLED();
            break;
        }
        
        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->timeEvt);
            g_stopProducer = RT_TRUE;
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
        
        case THROUGHPUT_SEND_SIG: {
            ThroughputEvt const *evt = (ThroughputEvt const *)e;
            me->packets_sent++;
            g_throughput_measurements++;
            
            /* Forward to consumer */
            ThroughputEvt *fwd_evt = Q_NEW(ThroughputEvt, THROUGHPUT_RECV_SIG);
            fwd_evt->timestamp = PerfCommon_getDWTCycles();
            fwd_evt->data_size = evt->data_size;
            fwd_evt->packet_id = evt->packet_id;
            
            QACTIVE_POST(&l_consumerAO.super, &fwd_evt->super, me);
            
            status = Q_HANDLED();
            break;
        }
        
        case THROUGHPUT_TIMEOUT_SIG: {
            rt_kprintf("Throughput Producer: Timeout reached\n");
            
            /* Set stop flag for producer thread */
            g_stopProducer = RT_TRUE;
            
            /* Wait for thread to stop and then delete it */
            PerfCommon_waitForThreads();
            if (producer_thread != RT_NULL) {
                rt_thread_delete(producer_thread);
                producer_thread = RT_NULL;
            }
            
            /* Calculate and print results */
            uint32_t end_time = PerfCommon_getDWTCycles();
            uint32_t duration = end_time - me->start_time;
            
            rt_kprintf("=== Throughput Producer Results ===\n");
            rt_kprintf("Packets sent: %u\n", me->packets_sent);
            rt_kprintf("Test duration: %u cycles\n", duration);
            rt_kprintf("Throughput: %u packets/cycle\n", 
                      (duration > 0) ? (me->packets_sent / duration) : 0);
            
            status = Q_TRAN(&ThroughputProducerAO_idle);
            break;
        }
        
        case THROUGHPUT_STOP_SIG: {
            rt_kprintf("Throughput Producer: Stopping test\n");
            QTimeEvt_disarm(&me->timeEvt);
            g_stopProducer = RT_TRUE;
            
            /* Wait for thread to stop and then delete it */
            PerfCommon_waitForThreads();
            if (producer_thread != RT_NULL) {
                rt_thread_delete(producer_thread);
                producer_thread = RT_NULL;
            }
            
            status = Q_TRAN(&ThroughputProducerAO_idle);
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
/* Consumer State Functions */
/*==========================================================================*/

static QState ThroughputConsumerAO_initial(ThroughputConsumerAO * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            /* Subscribe to throughput test signals */
            QActive_subscribe(&me->super, THROUGHPUT_START_SIG);
            QActive_subscribe(&me->super, THROUGHPUT_RECV_SIG);
            QActive_subscribe(&me->super, THROUGHPUT_STOP_SIG);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&ThroughputConsumerAO_idle);
        }
        default: {
            return Q_SUPER(&QHsm_top);
        }
    }
}

static QState ThroughputConsumerAO_idle(ThroughputConsumerAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Throughput Consumer: Idle state\n");
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
        
        case THROUGHPUT_START_SIG: {
            rt_kprintf("Throughput Consumer: Starting to consume\n");
            
            /* Reset counters */
            me->packets_received = 0;
            me->total_data_received = 0;
            me->start_time = PerfCommon_getDWTCycles();
            
            status = Q_TRAN(&ThroughputConsumerAO_consuming);
            break;
        }
        
        case THROUGHPUT_STOP_SIG: {
            rt_kprintf("Throughput Consumer: Stopping\n");
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

static QState ThroughputConsumerAO_consuming(ThroughputConsumerAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Throughput Consumer: Consuming state\n");
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
        
        case THROUGHPUT_RECV_SIG: {
            ThroughputEvt const *evt = (ThroughputEvt const *)e;
            
            me->packets_received++;
            me->total_data_received += evt->data_size;
            me->end_time = PerfCommon_getDWTCycles();
            
            status = Q_HANDLED();
            break;
        }
        
        case THROUGHPUT_STOP_SIG: {
            rt_kprintf("Throughput Consumer: Stopping test\n");
            
            /* Calculate and print results */
            uint32_t duration = me->end_time - me->start_time;
            
            rt_kprintf("=== Throughput Consumer Results ===\n");
            rt_kprintf("Packets received: %u\n", me->packets_received);
            rt_kprintf("Total data received: %u bytes\n", me->total_data_received);
            rt_kprintf("Test duration: %u cycles\n", duration);
            rt_kprintf("Throughput: %u packets/cycle\n", 
                      (duration > 0) ? (me->packets_received / duration) : 0);
            
            status = Q_TRAN(&ThroughputConsumerAO_idle);
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
/* Producer Thread Function */
/*==========================================================================*/

static void producer_thread_func(void *parameter) {
    (void)parameter;
    
    uint32_t packet_id = 0;
    
    while (!g_stopProducer) {
        /* Create and send throughput event */
        ThroughputEvt *evt = Q_NEW(ThroughputEvt, THROUGHPUT_SEND_SIG);
        evt->timestamp = PerfCommon_getDWTCycles();
        evt->data_size = 1024; /* Simulate 1KB data packets */
        evt->packet_id = ++packet_id;
        
        QACTIVE_POST(&l_producerAO.super, &evt->super, &l_producerAO);
        
        /* Small delay to avoid overwhelming the system */
        rt_thread_mdelay(1);
    }
    
    rt_kprintf("Producer thread exiting\n");
}

/*==========================================================================*/
/* Throughput Test Static Variables - Persistent for RT-Thread integration */
/*==========================================================================*/

static QEvt const *producer_queueSto[15];    /* Reduced queue sizes */
static QEvt const *consumer_queueSto[15];
static uint8_t producer_stack[1024];         /* Reduced stack sizes for embedded */
static uint8_t consumer_stack[1024];
static rt_bool_t throughput_test_running = RT_FALSE;

/*==========================================================================*/
/* Throughput Test Public Functions */
/*==========================================================================*/

void ThroughputTest_start(void) {
    /* Prevent multiple simultaneous test instances */
    if (throughput_test_running) {
        rt_kprintf("Throughput test already running\n");
        return;
    }
    
    /* Initialize common performance test infrastructure */
    PerfCommon_initTest();
    
    /* Initialize only the throughput event pool */
    PerfCommon_initThroughputPool();
    
    /* Initialize QF if not already done - safe to call multiple times in RT-Thread */
    QF_init();
    
    /* Construct the AOs */
    ThroughputProducerAO_ctor();
    ThroughputConsumerAO_ctor();
    
    /* Start the AOs */
    QACTIVE_START(&l_producerAO.super,
                  THROUGHPUT_PRODUCER_PRIO,
                  producer_queueSto, Q_DIM(producer_queueSto),
                  producer_stack, sizeof(producer_stack),
                  (void *)0);
    
    QACTIVE_START(&l_consumerAO.super,
                  THROUGHPUT_CONSUMER_PRIO,
                  consumer_queueSto, Q_DIM(consumer_queueSto),
                  consumer_stack, sizeof(consumer_stack),
                  (void *)0);
    
    /* Initialize QF framework (returns immediately in RT-Thread) */
    QF_run();
    
    /* Mark test as running */
    throughput_test_running = RT_TRUE;
    
    /* Send start signals */
    QACTIVE_POST(&l_producerAO.super, Q_NEW(QEvt, THROUGHPUT_START_SIG), &l_producerAO);
    QACTIVE_POST(&l_consumerAO.super, Q_NEW(QEvt, THROUGHPUT_START_SIG), &l_consumerAO);
    
    rt_kprintf("Throughput test started successfully\n");
}

void ThroughputTest_stop(void) {
    if (!throughput_test_running) {
        rt_kprintf("Throughput test not running\n");
        return;
    }
    
    /* Send stop signals */
    QACTIVE_POST(&l_producerAO.super, Q_NEW(QEvt, THROUGHPUT_STOP_SIG), &l_producerAO);
    QACTIVE_POST(&l_consumerAO.super, Q_NEW(QEvt, THROUGHPUT_STOP_SIG), &l_consumerAO);
    
    /* Give time for stop signals to be processed */
    rt_thread_mdelay(100);
    
    /* Unsubscribe from signals to prevent lingering subscriptions */
    QActive_unsubscribe(&l_producerAO.super, THROUGHPUT_START_SIG);
    QActive_unsubscribe(&l_producerAO.super, THROUGHPUT_STOP_SIG);
    
    QActive_unsubscribe(&l_consumerAO.super, THROUGHPUT_START_SIG);
    QActive_unsubscribe(&l_consumerAO.super, THROUGHPUT_RECV_SIG);
    QActive_unsubscribe(&l_consumerAO.super, THROUGHPUT_STOP_SIG);
    
    /* Mark test as stopped */
    throughput_test_running = RT_FALSE;
    
    /* Cleanup common infrastructure */
    PerfCommon_cleanupTest();
    
    /* Print final results */
    PerfCommon_printResults("Throughput", g_throughput_measurements);
    
    rt_kprintf("Throughput test stopped successfully\n");
}

/* RT-Thread MSH command exports */
#ifdef RT_USING_FINSH
#include <finsh.h>
MSH_CMD_EXPORT(ThroughputTest_start, start throughput performance test);
MSH_CMD_EXPORT(ThroughputTest_stop, stop throughput performance test);
#endif