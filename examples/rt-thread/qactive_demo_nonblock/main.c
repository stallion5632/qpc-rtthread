/*============================================================================
* Product: QActive Demo for RT-Thread - Enhanced Version (superset of lite)
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
#include "qactive_demo.h"
#include "rt_integration.h"
#include "config_proxy.h"

#ifdef QPC_USING_QACTIVE_DEMO_NONBLOCK
#ifdef RT_USING_FINSH

#include <finsh.h>
#include <stdio.h>
#include <string.h>

Q_DEFINE_THIS_FILE

/* QF publish-subscribe table */
#define MAX_PUB_SIG     32U
static QSubscrList subscrSto[MAX_PUB_SIG];

/*==========================================================================*/
/* RT-Thread Configuration & Storage (Enhanced from original) */
/*==========================================================================*/
#define SENSOR_QUEUE_SIZE      16U    /* Sensor AO event queue size */
#define PROCESSOR_QUEUE_SIZE   16U    /* Processor AO event queue size */
#define WORKER_QUEUE_SIZE      16U    /* Worker AO event queue size */
#define MONITOR_QUEUE_SIZE     16U    /* Monitor AO event queue size */
#define SENSOR_STACK_SIZE    1024U    /* Sensor AO thread stack size */
#define PROCESSOR_STACK_SIZE 1024U    /* Processor AO thread stack size */
#define WORKER_STACK_SIZE    1024U    /* Worker AO thread stack size */
#define MONITOR_STACK_SIZE   1024U    /* Monitor AO thread stack size */

/* Service priority definitions (RT-Thread integration) */
#define SENSOR_PRIO            1U     /* Sensor AO priority (higher priority for responsiveness) */
#define PROCESSOR_PRIO         2U     /* Processor AO priority */
#define WORKER_PRIO            3U     /* Worker AO priority */
#define MONITOR_PRIO           4U     /* Monitor AO priority */

/* Version information */
static const char *VERSION = "2.0.0-enhanced";

/*==========================================================================*/
/* Sensor Active Object (Event-driven, no RT-Thread IPC) */
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
/* Processor Active Object (Event-driven, no RT-Thread IPC) */
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
/* Worker Active Object (Event-driven, no RT-Thread IPC) */
/*==========================================================================*/
typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    uint32_t work_count;
} WorkerAO;

static WorkerAO l_workerAO;
QActive * const AO_Worker = &l_workerAO.super;

/* Worker state functions */
static QState WorkerAO_initial(WorkerAO * const me, QEvt const * const e);
static QState WorkerAO_idle(WorkerAO * const me, QEvt const * const e);
static QState WorkerAO_working(WorkerAO * const me, QEvt const * const e);

/*==========================================================================*/
/* Monitor Active Object (Event-driven, no RT-Thread IPC) */
/*==========================================================================*/
typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    uint32_t check_count;
} MonitorAO;

static MonitorAO l_monitorAO;
QActive * const AO_Monitor = &l_monitorAO.super;

/* Monitor state functions */
static QState MonitorAO_initial(MonitorAO * const me, QEvt const * const e);
static QState MonitorAO_monitoring(MonitorAO * const me, QEvt const * const e);

/*==========================================================================*/
/* Sensor Active Object Implementation (Pure event-driven) */
/*==========================================================================*/
static void SensorAO_ctor(void) {
    SensorAO *me = &l_sensorAO;

    rt_kprintf("[SensorAO_ctor] Constructing Sensor Active Object\n");
    QActive_ctor(&me->super, Q_STATE_CAST(&SensorAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, TIMEOUT_SIG, 0U);
    me->sensor_count = 0;
    rt_kprintf("[SensorAO_ctor] Sensor AO constructed successfully\n");
}

static QState SensorAO_initial(SensorAO * const me, QEvt const * const e) {
    (void)e;

    rt_thread_t t = rt_thread_self();
    rt_kprintf("[SensorAO_initial] Initializing Sensor AO - Thread: %s, QActive_Prio: %u, RT_Prio: %d\n",
               t ? t->name : "ISR", SENSOR_PRIO, t ? t->current_priority : -1);

    /* Subscribe to sensor signals */
    QActive_subscribe(&me->super, SENSOR_READ_SIG);
    rt_kprintf("[SensorAO_initial] Subscribed to SENSOR_READ_SIG\n");

    return Q_TRAN(&SensorAO_active);
}

static QState SensorAO_active(SensorAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_thread_t t = rt_thread_self();
            rt_kprintf("[SensorAO_active] ENTRY - Thread: %s, Prio: %d, Addr: 0x%08x\n",
                       t ? t->name : "ISR",
                       t ? t->current_priority : -1,
                       (unsigned int)t);
            rt_kprintf("[SensorAO_active] Starting periodic sensor readings\n");
            QTimeEvt_armX(&me->timeEvt, 200, 200); /* 2 second intervals */
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("[SensorAO_active] EXIT - Disarming timer\n");
            QTimeEvt_disarm(&me->timeEvt);
            status = Q_HANDLED();
            break;
        }

        case TIMEOUT_SIG: {
            /* Simulate sensor reading */
            me->sensor_count++;
            uint32_t sensor_data = me->sensor_count * 10 + (rt_tick_get() & 0xFF);

            rt_kprintf("[SensorAO_active] TIMEOUT - Reading #%u, data = %u (tick=%u)\n",
                      me->sensor_count, sensor_data, (uint32_t)rt_tick_get());

            /* Send data to processor (pure event-driven) */
            SensorDataEvt *evt = Q_NEW(SensorDataEvt, SENSOR_DATA_SIG);
            evt->data = sensor_data;
            rt_kprintf("[SensorAO_active] Posting sensor data to Processor AO\n");
            QACTIVE_POST(AO_Processor, &evt->super, me);

            /* Update statistics (event-driven, no RT-Thread IPC) */
            g_system_stats.sensor_readings++;
            rt_kprintf("[SensorAO_active] Updated sensor readings count: %u\n", g_system_stats.sensor_readings);

            status = Q_HANDLED();
            break;
        }

        case SENSOR_READ_SIG: {
            rt_kprintf("[SensorAO_active] SENSOR_READ_SIG - Manual read triggered\n");
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
/* Processor Active Object Implementation (Pure event-driven) */
/*==========================================================================*/
static void ProcessorAO_ctor(void) {
    ProcessorAO *me = &l_processorAO;

    rt_kprintf("[ProcessorAO_ctor] Constructing Processor Active Object\n");
    QActive_ctor(&me->super, Q_STATE_CAST(&ProcessorAO_initial));
    me->processed_count = 0;
    rt_kprintf("[ProcessorAO_ctor] Processor AO constructed successfully\n");
}

static QState ProcessorAO_initial(ProcessorAO * const me, QEvt const * const e) {
    (void)e;

    rt_thread_t t = rt_thread_self();
    rt_kprintf("[ProcessorAO_initial] Initializing Processor AO - Thread: %s, QActive_Prio: %u, RT_Prio: %d\n",
               t ? t->name : "ISR", PROCESSOR_PRIO, t ? t->current_priority : -1);

    /* Subscribe to processor signals */
    QActive_subscribe(&me->super, SENSOR_DATA_SIG);
    QActive_subscribe(&me->super, PROCESSOR_START_SIG);
    QActive_subscribe(&me->super, CONFIG_CFM_SIG);  /* Subscribe to config confirmations */
    rt_kprintf("[ProcessorAO_initial] Subscribed to SENSOR_DATA_SIG, PROCESSOR_START_SIG, CONFIG_CFM_SIG\n");

    return Q_TRAN(&ProcessorAO_idle);
}

static QState ProcessorAO_idle(ProcessorAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_thread_t t = rt_thread_self();
            rt_kprintf("[ProcessorAO_idle] ENTRY - Thread: %s, Prio: %d, Addr: 0x%08x\n",
                       t ? t->name : "ISR",
                       t ? t->current_priority : -1,
                       (unsigned int)t);
            rt_kprintf("[ProcessorAO_idle] Processor idle, waiting for data\n");
            status = Q_HANDLED();
            break;
        }

        case SENSOR_DATA_SIG: {
            SensorDataEvt const *evt = (SensorDataEvt const *)e;
            rt_kprintf("[ProcessorAO_idle] SENSOR_DATA_SIG - Received sensor data = %u\n", evt->data);

            /* Store data for processing state */
            me->processed_count++;

            status = Q_TRAN(&ProcessorAO_processing);
            break;
        }

        case PROCESSOR_START_SIG: {
            rt_kprintf("[ProcessorAO_idle] PROCESSOR_START_SIG - Manual start triggered\n");
            status = Q_TRAN(&ProcessorAO_processing);
            break;
        }

        case CONFIG_CFM_SIG: {
            ConfigCfmEvt const *evt = (ConfigCfmEvt const *)e;
            rt_kprintf("[ProcessorAO_idle] CONFIG_CFM_SIG - Config loaded: key=%u\n", evt->key);
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
            rt_kprintf("[ProcessorAO_processing] ENTRY - Processing data (count: %u)\n", me->processed_count);

            /* Simulate processing time and generate results */
            uint32_t result = me->processed_count * 100;
            rt_kprintf("[ProcessorAO_processing] Generated result: %u\n", result);

            /* Send result event (pure event-driven) */
            ProcessorResultEvt *res_evt = Q_NEW(ProcessorResultEvt, PROCESSOR_RESULT_SIG);
            res_evt->result = result;
            rt_kprintf("[ProcessorAO_processing] Created ProcessorResultEvt with result: %u\n", result);

            /* Post work to worker AO (pure event-driven) */
            rt_kprintf("[ProcessorAO_processing] About to create WorkerWorkEvt (size=%u bytes)\n", (unsigned int)sizeof(WorkerWorkEvt));
            WorkerWorkEvt *work_evt = Q_NEW(WorkerWorkEvt, WORKER_WORK_SIG);
            if (work_evt != (WorkerWorkEvt *)0) {
                work_evt->work_id = me->processed_count;
                work_evt->data_size = sizeof(SensorDataEvt);
                work_evt->priority = 1;
                rt_kprintf("[ProcessorAO_processing] Posting work to Worker AO (id=%u, size=%u, prio=%u)\n",
                          work_evt->work_id, work_evt->data_size, work_evt->priority);
                QACTIVE_POST(AO_Worker, &work_evt->super, me);
            } else {
                rt_kprintf("[ProcessorAO_processing] ERROR: Failed to allocate WorkerWorkEvt!\n");
            }

            /* Post work again */
            rt_kprintf("[ProcessorAO_processing] About to create second WorkerWorkEvt\n");
            WorkerWorkEvt *work_evt2 = Q_NEW(WorkerWorkEvt, WORKER_WORK_SIG);
            if (work_evt2 != (WorkerWorkEvt *)0) {
                /* Distinguish by different IDs */
                work_evt2->work_id = me->processed_count + 1000;
                work_evt2->data_size = sizeof(SensorDataEvt);
                work_evt2->priority = 2;
                rt_kprintf("[ProcessorAO_processing] Posting second work to Worker AO (id=%u, size=%u, prio=%u)\n",
                          work_evt2->work_id, work_evt2->data_size, work_evt2->priority);
                QACTIVE_POST(AO_Worker, &work_evt2->super, me);
            } else {
                rt_kprintf("[ProcessorAO_processing] ERROR: Failed to allocate second WorkerWorkEvt!\n");
            }

            /* Request config via proxy (event-driven, non-blocking) */
            post_config_request(0x1234, RT_NULL, &me->super);

            /* Update statistics (event-driven, no RT-Thread IPC) */
            g_system_stats.processed_data++;
            rt_kprintf("[ProcessorAO_processing] Updated processed data count: %u\n", g_system_stats.processed_data);

            status = Q_TRAN(&ProcessorAO_idle);
            break;
        }

        case SENSOR_DATA_SIG: {
            SensorDataEvt const *evt = (SensorDataEvt const *)e;
            rt_kprintf("[ProcessorAO_processing] SENSOR_DATA_SIG - Processing additional sensor data = %u\n", evt->data);
            me->processed_count++;
            status = Q_HANDLED();
            break;
        }

        case CONFIG_CFM_SIG: {
            ConfigCfmEvt const *evt = (ConfigCfmEvt const *)e;
            rt_kprintf("[ProcessorAO_processing] CONFIG_CFM_SIG - Config received during processing: key=%u\n", evt->key);
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
/* Worker Active Object Implementation (Pure event-driven) */
/*==========================================================================*/
static void WorkerAO_ctor(void) {
    WorkerAO *me = &l_workerAO;

    rt_kprintf("[WorkerAO_ctor] Constructing Worker Active Object\n");
    QActive_ctor(&me->super, Q_STATE_CAST(&WorkerAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, WORKER_TIMEOUT_SIG, 0U);
    me->work_count = 0;
    rt_kprintf("[WorkerAO_ctor] Worker AO constructed successfully\n");
}

static QState WorkerAO_initial(WorkerAO * const me, QEvt const * const e) {
    (void)e;

    rt_thread_t t = rt_thread_self();
    rt_kprintf("[WorkerAO_initial] Initializing Worker AO - Thread: %s, QActive_Prio: %u, RT_Prio: %d\n",
               t ? t->name : "ISR", WORKER_PRIO, t ? t->current_priority : -1);

    /* Subscribe to worker signals */
    QActive_subscribe(&me->super, WORKER_WORK_SIG);
    QActive_subscribe(&me->super, STORE_CFM_SIG);  /* Subscribe to storage confirmations */
    rt_kprintf("[WorkerAO_initial] Subscribed to WORKER_WORK_SIG, STORE_CFM_SIG\n");

    return Q_TRAN(&WorkerAO_idle);
}

static QState WorkerAO_idle(WorkerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_thread_t t = rt_thread_self();
            rt_kprintf("[WorkerAO_idle] ENTRY - Thread: %s, Prio: %d, Addr: 0x%08x\n",
                       t ? t->name : "ISR",
                       t ? t->current_priority : -1,
                       (unsigned int)t);
            rt_kprintf("[WorkerAO_idle] Worker idle, waiting for work\n");
            status = Q_HANDLED();
            break;
        }

        case WORKER_WORK_SIG: {
            WorkerWorkEvt const *evt = (WorkerWorkEvt const *)e;
            rt_kprintf("[WorkerAO_idle] WORKER_WORK_SIG - Received work ID %u (size=%u, prio=%u)\n",
                      evt->work_id, evt->data_size, evt->priority);
            status = Q_TRAN(&WorkerAO_working);
            break;
        }

        case STORE_CFM_SIG: {
            StoreCfmEvt const *evt = (StoreCfmEvt const *)e;
            rt_kprintf("[WorkerAO_idle] STORE_CFM_SIG - Storage completed with result: %d\n", evt->result);
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

static QState WorkerAO_working(WorkerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->work_count++;
            rt_kprintf("[WorkerAO_working] ENTRY - Processing work (total: %u)\n", me->work_count);

            /* Simulate work processing with timeout */
            QTimeEvt_armX(&me->timeEvt, 50, 0); /* 500ms delay */
            rt_kprintf("[WorkerAO_working] Armed timeout for 500ms work simulation\n");

            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("[WorkerAO_working] EXIT - Disarming work timer\n");
            QTimeEvt_disarm(&me->timeEvt);
            status = Q_HANDLED();
            break;
        }

        case WORKER_TIMEOUT_SIG: {
            rt_kprintf("[WorkerAO_working] WORKER_TIMEOUT_SIG - Work completed\n");

            /* Request storage via proxy (event-driven, non-blocking) */
            uint8_t work_data[32];
            snprintf((char*)work_data, sizeof(work_data), "Work result %lu", (unsigned long)me->work_count);
            post_storage_request(work_data, strlen((char*)work_data), &me->super);

            status = Q_TRAN(&WorkerAO_idle);
            break;
        }

        case WORKER_WORK_SIG: {
            WorkerWorkEvt const *evt = (WorkerWorkEvt const *)e;
            rt_kprintf("[WorkerAO_working] WORKER_WORK_SIG - Additional work ID %u queued\n", evt->work_id);
            status = Q_HANDLED();
            break;
        }

        case STORE_CFM_SIG: {
            StoreCfmEvt const *evt = (StoreCfmEvt const *)e;
            rt_kprintf("[WorkerAO_working] STORE_CFM_SIG - Storage completed during work with result: %d\n", evt->result);
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
/* Monitor Active Object Implementation (Pure event-driven) */
/*==========================================================================*/
static void MonitorAO_ctor(void) {
    MonitorAO *me = &l_monitorAO;

    rt_kprintf("[MonitorAO_ctor] Constructing Monitor Active Object\n");
    QActive_ctor(&me->super, Q_STATE_CAST(&MonitorAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, MONITOR_TIMEOUT_SIG, 0U);
    me->check_count = 0;
    rt_kprintf("[MonitorAO_ctor] Monitor AO constructed successfully\n");
}

static QState MonitorAO_initial(MonitorAO * const me, QEvt const * const e) {
    (void)e;

    rt_thread_t t = rt_thread_self();
    rt_kprintf("[MonitorAO_initial] Initializing Monitor AO - Thread: %s, QActive_Prio: %u, RT_Prio: %d\n",
               t ? t->name : "ISR", MONITOR_PRIO, t ? t->current_priority : -1);

    /* Subscribe to monitor signals */
    QActive_subscribe(&me->super, MONITOR_CHECK_SIG);
    rt_kprintf("[MonitorAO_initial] Subscribed to MONITOR_CHECK_SIG\n");

    return Q_TRAN(&MonitorAO_monitoring);
}

static QState MonitorAO_monitoring(MonitorAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_thread_t t = rt_thread_self();
            rt_kprintf("[MonitorAO_monitoring] ENTRY - Thread: %s, Prio: %d, Addr: 0x%08x\n",
                       t ? t->name : "ISR",
                       t ? t->current_priority : -1,
                       (unsigned int)t);
            rt_kprintf("[MonitorAO_monitoring] Starting periodic monitoring\n");
            QTimeEvt_armX(&me->timeEvt, 300, 300); /* 3 second intervals */
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("[MonitorAO_monitoring] EXIT - Disarming monitor timer\n");
            QTimeEvt_disarm(&me->timeEvt);
            status = Q_HANDLED();
            break;
        }

        case MONITOR_TIMEOUT_SIG: {
            /* Periodic monitoring */
            me->check_count++;
            rt_kprintf("[MonitorAO_monitoring] MONITOR_TIMEOUT_SIG - System check #%u - All systems operational\n",
                      me->check_count);

            /* Monitor event pools only (QF_getQueueMin not available in QP/C 7.x) */
            rt_kprintf("[QF_Monitor] PoolMin(4B)=%u, PoolMin(8B)=%u, PoolMin(16B)=%u, PoolMin(64B)=%u, PoolMin(256B)=%u\n",
                      QF_getPoolMin(1U), QF_getPoolMin(2U), QF_getPoolMin(3U), QF_getPoolMin(4U), QF_getPoolMin(5U));

            /* Send check signal to self (health check mechanism) */
            rt_kprintf("[MonitorAO_monitoring] Posting self-check signal\n");
            QACTIVE_POST(&me->super, Q_NEW(QEvt, MONITOR_CHECK_SIG), me);

            /* Update statistics (event-driven, no RT-Thread IPC) */
            g_system_stats.health_checks++;
            rt_kprintf("[MonitorAO_monitoring] Updated health checks count: %u\n", g_system_stats.health_checks);

            status = Q_HANDLED();
            break;
        }

        case MONITOR_CHECK_SIG: {
            rt_kprintf("[MonitorAO_monitoring] MONITOR_CHECK_SIG - Health check completed\n");
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

/* Initialize event pools - must be in ascending order of event size */
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(QEvt) basicEventPool[50];              /* 4-byte event pool for QEvt only */
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(SensorDataEvt) shared8Pool[60];        /* 8-byte event pool for SensorDataEvt, ProcessorResultEvt */
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(WorkerWorkEvt) worker16Pool[40];       /* 16-byte event pool for WorkerWorkEvt */
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(ConfigReqEvt) config64Pool[30];        /* 64-byte event pool: increased for concurrent config requests */
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(StoreReqEvt) store256Pool[20];         /* 256-byte event pool: increased for concurrent storage requests */

/*==========================================================================*/
/* QActive Demo Initialization (Enhanced from lite version) */
/*==========================================================================*/
void QActiveDemo_init(void) {
    static uint8_t isInitialized = 0;

    /* Prevent duplicate initialization */
    if (isInitialized != 0) {
        rt_kprintf("[QActiveDemo_init] Already initialized, skipping...\n");
        return;
    }
    isInitialized = 1;

    rt_kprintf("[QActiveDemo_init] *** QActive Demo Enhanced v%s ***\n", VERSION);
    rt_kprintf("[QActiveDemo_init] Build: %s %s\n", __DATE__, __TIME__);

    /* Initialize QF framework */
    QF_init();
    rt_kprintf("[QActiveDemo_init] QF framework initialized\n");

    /* Initialize publish-subscribe system */
    QF_psInit(subscrSto, Q_DIM(subscrSto));
    rt_kprintf("[QActiveDemo_init] Publish-subscribe system initialized\n");

    /* Print event struct sizes for debugging and pool design */
    rt_kprintf("[QActiveDemo_init] sizeof(QEvt)=%d\n", (int)sizeof(QEvt));
    rt_kprintf("[QActiveDemo_init] sizeof(SensorDataEvt)=%d\n", (int)sizeof(SensorDataEvt));
    rt_kprintf("[QActiveDemo_init] sizeof(ProcessorResultEvt)=%d\n", (int)sizeof(ProcessorResultEvt));
    rt_kprintf("[QActiveDemo_init] sizeof(WorkerWorkEvt)=%d\n", (int)sizeof(WorkerWorkEvt));
    rt_kprintf("[QActiveDemo_init] sizeof(ConfigReqEvt)=%d\n", (int)sizeof(ConfigReqEvt));
    rt_kprintf("[QActiveDemo_init] sizeof(StoreReqEvt)=%d\n", (int)sizeof(StoreReqEvt));

    /* Initialize event pools. Events organized by size. */
    QF_poolInit(basicEventPool, sizeof(basicEventPool), sizeof(QEvt));
    rt_kprintf("[QActiveDemo_init] Basic event pool initialized\n");
    QF_poolInit(shared8Pool, sizeof(shared8Pool), sizeof(SensorDataEvt));
    rt_kprintf("[QActiveDemo_init] Shared 8-byte event pool initialized for SensorDataEvt, ProcessorResultEvt\n");
    QF_poolInit(worker16Pool, sizeof(worker16Pool), sizeof(WorkerWorkEvt));
    rt_kprintf("[QActiveDemo_init] Worker 16-byte event pool initialized for WorkerWorkEvt\n");
    QF_poolInit(config64Pool, sizeof(config64Pool), sizeof(ConfigReqEvt));
    rt_kprintf("[QActiveDemo_init] Config 64-byte event pool initialized for Config events\n");
    QF_poolInit(store256Pool, sizeof(store256Pool), sizeof(StoreReqEvt));
    rt_kprintf("[QActiveDemo_init] Storage 256-byte event pool initialized for Storage events\n");

    /* Initialize RT-Thread integration */
    if (rt_integration_init() == 0) {
        rt_kprintf("[QActiveDemo_init] RT-Thread integration initialized successfully\n");
    } else {
        rt_kprintf("[QActiveDemo_init] WARNING: RT-Thread integration initialization failed\n");
    }

    /* Construct all active objects */
    SensorAO_ctor();
    ProcessorAO_ctor();
    WorkerAO_ctor();
    MonitorAO_ctor();
    rt_kprintf("[QActiveDemo_init] All Active Objects constructed\n");
}

/*==========================================================================*/
/* QActive Demo Startup (Enhanced from lite version) */
/*==========================================================================*/
int qactive_demo_start(void) {
    static uint8_t isStarted = 0;
    if (isStarted != 0) {
        rt_kprintf("[qactive_demo_start] Already started, skipping...\n");
        return 0;
    }
    isStarted = 1;

    /* Event queue storage */
    ALIGN(RT_ALIGN_SIZE) static QEvt const *sensorQueue[SENSOR_QUEUE_SIZE];
    ALIGN(RT_ALIGN_SIZE) static QEvt const *processorQueue[PROCESSOR_QUEUE_SIZE];
    ALIGN(RT_ALIGN_SIZE) static QEvt const *workerQueue[WORKER_QUEUE_SIZE];
    ALIGN(RT_ALIGN_SIZE) static QEvt const *monitorQueue[MONITOR_QUEUE_SIZE];

    /* Stack storage */
    ALIGN(RT_ALIGN_SIZE) static uint8_t sensorStack[SENSOR_STACK_SIZE];
    ALIGN(RT_ALIGN_SIZE) static uint8_t processorStack[PROCESSOR_STACK_SIZE];
    ALIGN(RT_ALIGN_SIZE) static uint8_t workerStack[WORKER_STACK_SIZE];
    ALIGN(RT_ALIGN_SIZE) static uint8_t monitorStack[MONITOR_STACK_SIZE];

    rt_kprintf("[qactive_demo_start] Starting QActive Demo with enhanced RT-Thread integration...\n");

    /* Initialize the demo (if not already done) */
    QActiveDemo_init();

    rt_kprintf("[qactive_demo_start] Starting 4 QActive objects with RT-Thread scheduling...\n");

    /* Start active objects with RT-Thread integration */
    QACTIVE_START(AO_Sensor,
                  SENSOR_PRIO, /* Priority */
                  sensorQueue, Q_DIM(sensorQueue),
                  sensorStack, sizeof(sensorStack),
                  (void *)0);
    /* Set thread name for RT-Thread integration */
    QActive_setAttr(AO_Sensor, THREAD_NAME_ATTR, "sensor_ao");
    rt_kprintf("[qactive_demo_start] Sensor AO started (prio=%u, thread=%s)\n",
               SENSOR_PRIO, AO_Sensor->thread.name ? AO_Sensor->thread.name : "NULL");

    QACTIVE_START(AO_Processor,
                  PROCESSOR_PRIO, /* Priority */
                  processorQueue, Q_DIM(processorQueue),
                  processorStack, sizeof(processorStack),
                  (void *)0);
    QActive_setAttr(AO_Processor, THREAD_NAME_ATTR, "processor_ao");
    rt_kprintf("[qactive_demo_start] Processor AO started (prio=%u, thread=%s)\n",
               PROCESSOR_PRIO, AO_Processor->thread.name ? AO_Processor->thread.name : "NULL");

    QACTIVE_START(AO_Worker,
                  WORKER_PRIO, /* Priority */
                  workerQueue, Q_DIM(workerQueue),
                  workerStack, sizeof(workerStack),
                  (void *)0);
    QActive_setAttr(AO_Worker, THREAD_NAME_ATTR, "worker_ao");
    rt_kprintf("[qactive_demo_start] Worker AO started (prio=%u, thread=%s)\n",
               WORKER_PRIO, AO_Worker->thread.name ? AO_Worker->thread.name : "NULL");

    QACTIVE_START(AO_Monitor,
                  MONITOR_PRIO, /* Priority */
                  monitorQueue, Q_DIM(monitorQueue),
                  monitorStack, sizeof(monitorStack),
                  (void *)0);
    QActive_setAttr(AO_Monitor, THREAD_NAME_ATTR, "monitor_ao");
    rt_kprintf("[qactive_demo_start] Monitor AO started (prio=%u, thread=%s)\n",
               MONITOR_PRIO, AO_Monitor->thread.name ? AO_Monitor->thread.name : "NULL");

    /* Start RT-Thread integration components */
    if (rt_integration_start() == 0) {
        rt_kprintf("[qactive_demo_start] RT-Thread integration components started successfully\n");
    } else {
        rt_kprintf("[qactive_demo_start] WARNING: RT-Thread integration startup failed\n");
    }

    rt_kprintf("[qactive_demo_start] *** QActive Demo Enhanced Started Successfully ***\n");
    rt_kprintf("[qactive_demo_start] All components running under RT-Thread scheduling\n");

    return QF_run(); /* Run the QF application */
}

/* RT-Thread MSH command exports */
MSH_CMD_EXPORT(qactive_demo_start, start enhanced QActive demo with 4 AOs plus RT-Thread integration);

/* RT-Thread application auto-initialization */
static int qactive_demo_init(void) {
    rt_kprintf("=== QActive Demo Enhanced Auto-Initialize ===\n");
    return qactive_demo_start();
}
INIT_APP_EXPORT(qactive_demo_init);

#ifdef RT_USING_MSH
/*
 * Enhanced MSH command processing functions (RT-Thread integration)
 */
static void cmd_qactive_control(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: qactive_control <start|stop|stats|config>\n");
        return;
    }

    if (rt_strcmp(argv[1], "start") == 0) {
        qactive_start_cmd(argc, argv);
    } else if (rt_strcmp(argv[1], "stop") == 0) {
        qactive_stop_cmd(argc, argv);
    } else if (rt_strcmp(argv[1], "stats") == 0) {
        qactive_stats_cmd(argc, argv);
    } else if (rt_strcmp(argv[1], "config") == 0) {
        qactive_config_cmd(argc, argv);
    } else {
        rt_kprintf("Unknown command: %s\n", argv[1]);
    }
}
MSH_CMD_EXPORT(cmd_qactive_control, Enhanced QActive control: start/stop/stats/config);
#endif

/* Main function entry point (Enhanced from lite version) */
int main(void)
{
    rt_kprintf("[main] *** QActive Demo Enhanced v%s ***\n", VERSION);
    rt_kprintf("[main] Main function called - demo should be auto-started by RT-Thread\n");
    rt_kprintf("[main] Type 'qactive_control start' for manual control if needed\n");

    /* Main function should not be needed since auto-init handles startup */
    return 0;
}

#endif /* RT_USING_FINSH */
#endif /* QPC_USING_QACTIVE_DEMO_NONBLOCK */
