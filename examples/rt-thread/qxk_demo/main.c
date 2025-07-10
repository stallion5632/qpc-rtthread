/*============================================================================
* Product: QXK Demo for RT-Thread - Main Application
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
#include "qxk_demo.h"
#include "rt_integration.h"

#ifdef QPC_USING_QXK_DEMO
#ifdef RT_USING_FINSH

#include <finsh.h>

Q_DEFINE_THIS_FILE

/*==========================================================================*/
/* Sensor Active Object */
/*==========================================================================*/
typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    uint32_t sensor_count;
} SensorAO;

static SensorAO l_sensorAO;
QActive * const AO_Sensor = &l_sensorAO.super;

/* Sensor state functions */
static QState SensorAO_initial(SensorAO * const me, QEvt const * const e);
static QState SensorAO_active(SensorAO * const me, QEvt const * const e);

/*==========================================================================*/
/* Processor Active Object */
/*==========================================================================*/
typedef struct {
    QActive super;
    uint32_t processed_count;
} ProcessorAO;

static ProcessorAO l_processorAO;
QActive * const AO_Processor = &l_processorAO.super;

/* Processor state functions */
static QState ProcessorAO_initial(ProcessorAO * const me, QEvt const * const e);
static QState ProcessorAO_idle(ProcessorAO * const me, QEvt const * const e);
static QState ProcessorAO_processing(ProcessorAO * const me, QEvt const * const e);

/*==========================================================================*/
/* Worker QXThread */
/*==========================================================================*/
typedef struct {
    QXThread super;
    uint32_t work_count;
} WorkerThread;

static WorkerThread l_workerThread;
static void WorkerThread_run(QXThread * const me);

/*==========================================================================*/
/* Monitor QXThread */
/*==========================================================================*/
typedef struct {
    QXThread super;
    uint32_t check_count;
} MonitorThread;

static MonitorThread l_monitorThread;
static void MonitorThread_run(QXThread * const me);

/*==========================================================================*/
/* Sensor Active Object Implementation */
/*==========================================================================*/
static void SensorAO_ctor(void) {
    SensorAO *me = &l_sensorAO;
    
    QActive_ctor(&me->super, Q_STATE_CAST(&SensorAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, TIMEOUT_SIG, 0U);
    me->sensor_count = 0;
}

static QState SensorAO_initial(SensorAO * const me, QEvt const * const e) {
    (void)e;
    
    /* Subscribe to sensor signals */
    QActive_subscribe(&me->super, SENSOR_READ_SIG);
    
    return Q_TRAN(&SensorAO_active);
}

static QState SensorAO_active(SensorAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Sensor: Starting periodic sensor readings\n");
            QTimeEvt_armX(&me->timeEvt, 200, 200); /* 2 second intervals */
            status = Q_HANDLED();
            break;
        }
        
        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->timeEvt);
            status = Q_HANDLED();
            break;
        }
        
        case TIMEOUT_SIG: {
            /* Simulate sensor reading */
            me->sensor_count++;
            uint32_t sensor_data = me->sensor_count * 10 + (rt_tick_get() & 0xFF);
            
            rt_kprintf("Sensor: Reading %u, data = %u\n", me->sensor_count, sensor_data);
            
            /* Update statistics */
            rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
            g_system_stats.sensor_readings++;
            rt_mutex_release(g_config_mutex);
            
            /* Send data to processor */
            SensorDataEvt *evt = Q_NEW(SensorDataEvt, SENSOR_DATA_SIG);
            evt->data = sensor_data;
            QACTIVE_POST(AO_Processor, &evt->super, me);
            
            status = Q_HANDLED();
            break;
        }
        
        case SENSOR_READ_SIG: {
            rt_kprintf("Sensor: Manual read triggered\n");
            QACTIVE_POST(&me->super, Q_NEW(QEvt, TIMEOUT_SIG), me);
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

/*==========================================================================*/
/* Processor Active Object Implementation */
/*==========================================================================*/
static void ProcessorAO_ctor(void) {
    ProcessorAO *me = &l_processorAO;
    
    QActive_ctor(&me->super, Q_STATE_CAST(&ProcessorAO_initial));
    me->processed_count = 0;
}

static QState ProcessorAO_initial(ProcessorAO * const me, QEvt const * const e) {
    (void)e;
    
    /* Subscribe to processor signals */
    QActive_subscribe(&me->super, SENSOR_DATA_SIG);
    QActive_subscribe(&me->super, PROCESSOR_START_SIG);
    QActive_subscribe(&me->super, NETWORK_CONFIG_SIG);
    
    return Q_TRAN(&ProcessorAO_idle);
}

static QState ProcessorAO_idle(ProcessorAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Processor: Idle, waiting for data\n");
            status = Q_HANDLED();
            break;
        }
        
        case SENSOR_DATA_SIG: {
            SensorDataEvt const *evt = (SensorDataEvt const *)e;
            rt_kprintf("Processor: Received sensor data = %u\n", evt->data);
            status = Q_TRAN(&ProcessorAO_processing);
            break;
        }
        
        case PROCESSOR_START_SIG: {
            rt_kprintf("Processor: Manual start triggered\n");
            status = Q_TRAN(&ProcessorAO_processing);
            break;
        }
        
        case NETWORK_CONFIG_SIG: {
            NetworkConfigEvt const *config_evt = (NetworkConfigEvt const *)e;
            rt_kprintf("Processor: Received network configuration - sensor_rate=%u\n", 
                      config_evt->sensor_rate);
            
            /* Update sensor timing if needed */
            if (config_evt->sensor_rate != g_shared_config.sensor_rate) {
                rt_kprintf("Processor: Updating sensor rate to %u\n", config_evt->sensor_rate);
                /* In a real implementation, this would update the sensor's timer */
            }
            
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

static QState ProcessorAO_processing(ProcessorAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->processed_count++;
            rt_kprintf("Processor: Processing data (count: %u)\n", me->processed_count);
            
            /* Update statistics */
            rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
            g_system_stats.processed_data++;
            rt_mutex_release(g_config_mutex);
            
            /* Simulate processing time */
            uint32_t result = me->processed_count * 100;
            
            /* Send result and notify worker thread */
            ProcessorResultEvt *evt = Q_NEW(ProcessorResultEvt, PROCESSOR_RESULT_SIG);
            evt->result = result;
            
            /* Post work to worker thread */
            WorkerWorkEvt *work_evt = Q_NEW(WorkerWorkEvt, WORKER_WORK_SIG);
            work_evt->work_id = me->processed_count;
            QACTIVE_POST((QActive *)&l_workerThread, &work_evt->super, me);
            
            status = Q_TRAN(&ProcessorAO_idle);
            break;
        }
        
        case SENSOR_DATA_SIG: {
            SensorDataEvt const *evt = (SensorDataEvt const *)e;
            rt_kprintf("Processor: Processing additional sensor data = %u\n", evt->data);
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

/*==========================================================================*/
/* Worker QXThread Implementation */
/*==========================================================================*/
static void WorkerThread_ctor(void) {
    WorkerThread *me = &l_workerThread;
    
    QXThread_ctor(&me->super, &WorkerThread_run, 0);
    me->work_count = 0;
}

static void WorkerThread_run(QXThread * const me) {
    WorkerThread *wt = (WorkerThread *)me;
    
    rt_kprintf("Worker: Thread started\n");
    
    for (;;) {
        QEvt const *e = QXThread_queueGet(QXTHREAD_NO_TIMEOUT);
        
        if (e != (QEvt const *)0) {
            switch (e->sig) {
                case WORKER_WORK_SIG: {
                    WorkerWorkEvt const *evt = (WorkerWorkEvt const *)e;
                    wt->work_count++;
                    rt_kprintf("Worker: Processing work ID %u (total: %u)\n", 
                              evt->work_id, wt->work_count);
                    
                    /* Simulate work processing */
                    QXThread_delay(50); /* 500ms delay */
                    
                    rt_kprintf("Worker: Work ID %u completed\n", evt->work_id);
                    
                    /* Send processed data to network thread */
                    NetworkDataEvt *net_data = (NetworkDataEvt *)rt_malloc(sizeof(NetworkDataEvt));
                    if (net_data != RT_NULL) {
                        net_data->data = evt->work_id * 1000; /* Processed data */
                        net_data->timestamp = rt_tick_get();
                        net_data->source_id = wt->work_count;
                        
                        /* Send to network queue */
                        if (rt_mq_send(g_network_queue, &net_data, sizeof(NetworkDataEvt*)) != RT_EOK) {
                            rt_kprintf("Worker: Failed to send data to network queue\n");
                            rt_free(net_data);
                        } else {
                            rt_kprintf("Worker: Data sent to network queue\n");
                        }
                    }
                    
                    /* Release storage semaphore to trigger save */
                    rt_sem_release(g_storage_sem);
                    
                    break;
                }
                
                default: {
                    rt_kprintf("Worker: Unknown signal %u\n", e->sig);
                    break;
                }
            }
            
            Q_GC(e); /* Garbage collect the event */
        }
    }
}

/*==========================================================================*/
/* Monitor QXThread Implementation */
/*==========================================================================*/
static void MonitorThread_ctor(void) {
    MonitorThread *me = &l_monitorThread;
    
    QXThread_ctor(&me->super, &MonitorThread_run, 0);
    me->check_count = 0;
}

static void MonitorThread_run(QXThread * const me) {
    MonitorThread *mt = (MonitorThread *)me;
    
    rt_kprintf("Monitor: Thread started\n");
    
    for (;;) {
        /* Periodic monitoring */
        QXThread_delay(300); /* 3 second intervals */
        
        mt->check_count++;
        rt_kprintf("Monitor: System check #%u - All systems operational\n", 
                  mt->check_count);
        
        /* Update health statistics */
        rt_mutex_take(g_config_mutex, RT_WAITING_FOREVER);
        g_system_stats.health_checks++;
        rt_mutex_release(g_config_mutex);
        
        /* Send health check event to system */
        rt_event_send(g_system_event, RT_EVENT_HEALTH_CHECK);
        
        /* Send check signal to monitor (could be used for health checks) */
        QACTIVE_POST((QActive *)&l_monitorThread, Q_NEW(QEvt, MONITOR_CHECK_SIG), me);
    }
}

/*==========================================================================*/
/* QXK Demo Initialization */
/*==========================================================================*/
void QXKDemo_init(void) {
    /* Initialize event pools */
    static QF_MPOOL_EL(SensorDataEvt) sensorDataPool[10];
    static QF_MPOOL_EL(ProcessorResultEvt) processorResultPool[10];
    static QF_MPOOL_EL(WorkerWorkEvt) workerWorkPool[10];
    static QF_MPOOL_EL(NetworkConfigEvt) networkConfigPool[5];
    static QF_MPOOL_EL(SystemHealthEvt) systemHealthPool[5];
    
    QF_poolInit(sensorDataPool, sizeof(sensorDataPool), sizeof(SensorDataEvt));
    QF_poolInit(processorResultPool, sizeof(processorResultPool), sizeof(ProcessorResultEvt));
    QF_poolInit(workerWorkPool, sizeof(workerWorkPool), sizeof(WorkerWorkEvt));
    QF_poolInit(networkConfigPool, sizeof(networkConfigPool), sizeof(NetworkConfigEvt));
    QF_poolInit(systemHealthPool, sizeof(systemHealthPool), sizeof(SystemHealthEvt));
    
    /* Construct all objects */
    SensorAO_ctor();
    ProcessorAO_ctor();
    WorkerThread_ctor();
    MonitorThread_ctor();
    
    /* Initialize RT-Thread integration */
    rt_integration_init();
}

/*==========================================================================*/
/* QXK Demo Startup */
/*==========================================================================*/
int qxk_demo_start(void) {
    /* Event queue storage */
    static QEvt const *sensorQueue[10];
    static QEvt const *processorQueue[10];
    static QEvt const *workerQueue[10];
    static QEvt const *monitorQueue[10];
    
    /* Stack storage */
    static uint8_t sensorStack[1024];
    static uint8_t processorStack[1024];
    static uint8_t workerStack[1024];
    static uint8_t monitorStack[1024];
    
    /* Initialize QF */
    QF_init();
    
    /* Initialize the demo */
    QXKDemo_init();
    
    /* Start active objects */
    QACTIVE_START(AO_Sensor,
                  1U, /* Priority */
                  sensorQueue, Q_DIM(sensorQueue),
                  sensorStack, sizeof(sensorStack),
                  (void *)0);
    
    QACTIVE_START(AO_Processor,
                  2U, /* Priority */
                  processorQueue, Q_DIM(processorQueue),
                  processorStack, sizeof(processorStack),
                  (void *)0);
    
    /* Start QXThread extension threads */
    QXTHREAD_START(&l_workerThread.super,
                   3U, /* Priority */
                   workerQueue, Q_DIM(workerQueue),
                   workerStack, sizeof(workerStack),
                   (void *)0);
    
    QXTHREAD_START(&l_monitorThread.super,
                   4U, /* Priority */
                   monitorQueue, Q_DIM(monitorQueue),
                   monitorStack, sizeof(monitorStack),
                   (void *)0);
    
    rt_kprintf("QXK Demo: Started - 2 QActive objects + 2 QXThread extensions\n");
    
    /* Start RT-Thread integration components */
    rt_integration_start();
    
    /* Signal that QXK is ready */
    rt_event_send(g_system_event, RT_EVENT_QXK_READY);
    
    rt_kprintf("QXK Demo: RT-Thread integration started - 3 RT-Thread threads\n");
    
    return QF_run(); /* Run the QF application */
}

/* RT-Thread MSH command exports */
MSH_CMD_EXPORT(qxk_demo_start, start QXK demo with 2 AOs and 2 XThreads);

/* Manual trigger function for explicit start */
static int qxk_demo_manual_start(void) {
    rt_kprintf("=== QXK Demo Manual Start ===\n");
    return qxk_demo_start();
}

/* RT-Thread application auto-initialization */
static int qxk_demo_init(void) {
    rt_kprintf("=== QXK Demo Auto-Initialize ===\n");
    return qxk_demo_start();
}
INIT_APP_EXPORT(qxk_demo_init);

#endif /* RT_USING_FINSH */
#endif /* QPC_USING_QXK_DEMO */