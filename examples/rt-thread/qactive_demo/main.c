/*============================================================================
* Product: QActive Demo for RT-Thread with Industrial IoT Gateway Simulation
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
#include <rtthread.h>
#include <rtdef.h>

#ifdef QPC_USING_QACTIVE_DEMO

/* QP/C module definition */
Q_DEFINE_THIS_FILE
#define MAX_PUB_SIG 32U  /* maximum number of published signals */
static QSubscrList subscrSto[MAX_PUB_SIG];

/*
 * Queue and stack configuration
 */
#define SENSOR_QUEUE_SIZE      16U    /* Sensor AO event queue size */
#define PROCESSOR_QUEUE_SIZE   16U    /* Processor AO event queue size */
#define WORKER_QUEUE_SIZE      16U    /* Worker AO event queue size */
#define MONITOR_QUEUE_SIZE     16U    /* Monitor AO event queue size */
#define SENSOR_STACK_SIZE    1024U    /* Sensor AO thread stack size */
#define PROCESSOR_STACK_SIZE 1024U    /* Processor AO thread stack size */
#define WORKER_STACK_SIZE    1024U    /* Worker AO thread stack size */
#define MONITOR_STACK_SIZE   1024U    /* Monitor AO thread stack size */

/*
 * Service priority definitions
 */
#define SENSOR_PRIO            3U     /* Sensor AO priority (lower number = higher priority) */
#define PROCESSOR_PRIO         4U     /* Processor AO priority */
#define WORKER_PRIO            5U     /* Worker AO priority */
#define MONITOR_PRIO           6U     /* Monitor AO priority */

/*
 * Storage area definitions
 */

static QEvt const * sensorQueueSto[SENSOR_QUEUE_SIZE];      /* Sensor queue storage */
static QEvt const * processorQueueSto[PROCESSOR_QUEUE_SIZE]; /* Processor queue storage */
static QEvt const * workerQueueSto[WORKER_QUEUE_SIZE];      /* Worker queue storage */
static QEvt const * monitorQueueSto[MONITOR_QUEUE_SIZE];    /* Monitor queue storage */
ALIGN(RT_ALIGN_SIZE) static rt_uint8_t sensorStackSto[SENSOR_STACK_SIZE];       /* Sensor stack storage */
ALIGN(RT_ALIGN_SIZE) static rt_uint8_t processorStackSto[PROCESSOR_STACK_SIZE]; /* Processor stack storage */
ALIGN(RT_ALIGN_SIZE) static rt_uint8_t workerStackSto[WORKER_STACK_SIZE];       /* Worker stack storage */
ALIGN(RT_ALIGN_SIZE) static rt_uint8_t monitorStackSto[MONITOR_STACK_SIZE];     /* Monitor stack storage */

/* Event pools must be initialized in ascending order of event size. */
/* Only one pool is allowed for events of the same size. */
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(QEvt) basicEventPool[20];              /* 4-byte event pool for QEvt only */
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(SensorDataEvt) shared16Pool[30];        /* 16-byte event pool shared by SensorDataEvt, ProcessorResultEvt, WorkerWorkEvt */

/*
 * Version information
 */
static const char *VERSION = "1.0.0";

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

static QState SensorAO_initial(SensorAO * const me, QEvt const * const e) {
    (void)e;
    me->sensor_count = 0;
    QTimeEvt_ctorX(&me->timeEvt, &me->super, SENSOR_TIMEOUT_SIG, 0U);
    return Q_TRAN(&SensorAO_active);
}

static QState SensorAO_active(SensorAO * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_thread_t t = rt_thread_self();
            rt_kprintf("[SensorAO] Thread: %s, Prio: %d, Addr: 0x%08x\n",
                       t ? t->name : "ISR",
                       t ? t->current_priority : -1,
                       (unsigned int)t);
            QTimeEvt_armX(&me->timeEvt, 200U, 200U); /* 2 second intervals */
            status = Q_HANDLED();
            break;
        }

        case SENSOR_TIMEOUT_SIG: {
            me->sensor_count++;
            rt_kprintf("Sensor: Generated data sample #%u\n", me->sensor_count);

            /* Send data to processor */
            SensorDataEvt *sde = Q_NEW(SensorDataEvt, SENSOR_DATA_SIG);
            sde->temperature = 20 + (me->sensor_count % 10);
            sde->pressure = 1000 + (me->sensor_count % 100);
            sde->timestamp = rt_tick_get();
            QACTIVE_POST(AO_Processor, &sde->super, me);

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

void SensorAO_ctor(void) {
    QActive_ctor(&l_sensorAO.super, Q_STATE_CAST(&SensorAO_initial));
}

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

static QState ProcessorAO_initial(ProcessorAO * const me, QEvt const * const e) {
    (void)e;
    me->processed_count = 0;
    return Q_TRAN(&ProcessorAO_idle);
}

static QState ProcessorAO_idle(ProcessorAO * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case SENSOR_DATA_SIG: {
            rt_thread_t t = rt_thread_self();
            rt_kprintf("[ProcessorAO] Thread: %s, Prio: %d, Addr: 0x%08x\n",
                       t ? t->name : "ISR",
                       t ? t->current_priority : -1,
                       (unsigned int)t);
            SensorDataEvt const *sde = Q_EVT_CAST(SensorDataEvt);
            me->processed_count++;
            rt_kprintf("Processor: Validated data #%u (temp=%u, pressure=%u)\n",
                      me->processed_count, sde->temperature, sde->pressure);

            /* Post to worker for compression */
            WorkerWorkEvt *wwe = Q_NEW(WorkerWorkEvt, WORKER_WORK_SIG);
            wwe->data_id = me->processed_count;
            wwe->data_size = sizeof(SensorDataEvt);
            wwe->priority = 1;
            QACTIVE_POST(AO_Worker, &wwe->super, me);

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

void ProcessorAO_ctor(void) {
    QActive_ctor(&l_processorAO.super, Q_STATE_CAST(&ProcessorAO_initial));
}

/*==========================================================================*/
/* Worker Active Object */
/*==========================================================================*/
typedef struct {
    QActive super;
    uint32_t work_count;
} WorkerAO;

static WorkerAO l_workerAO;
QActive * const AO_Worker = &l_workerAO.super;

/* Worker state functions */
static QState WorkerAO_initial(WorkerAO * const me, QEvt const * const e);
static QState WorkerAO_idle(WorkerAO * const me, QEvt const * const e);

static QState WorkerAO_initial(WorkerAO * const me, QEvt const * const e) {
    (void)e;
    me->work_count = 0;
    return Q_TRAN(&WorkerAO_idle);
}

static QState WorkerAO_idle(WorkerAO * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case WORKER_WORK_SIG: {
            rt_thread_t t = rt_thread_self();
            rt_kprintf("[WorkerAO] Thread: %s, Prio: %d, Addr: 0x%08x\n",
                       t ? t->name : "ISR",
                       t ? t->current_priority : -1,
                       (unsigned int)t);
            WorkerWorkEvt const *wwe = Q_EVT_CAST(WorkerWorkEvt);
            me->work_count++;
            rt_kprintf("Worker: Compressed data #%u (size=%u bytes)\n",
                      me->work_count, wwe->data_size);

            /* Simulate work */
            rt_thread_mdelay(50);

            /* Send compressed data to RT-Thread for storage */
            rt_sem_release(g_storage_sem);

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

void WorkerAO_ctor(void) {
    QActive_ctor(&l_workerAO.super, Q_STATE_CAST(&WorkerAO_initial));
}

/*==========================================================================*/
/* Monitor Active Object */
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
static QState MonitorAO_active(MonitorAO * const me, QEvt const * const e);

static QState MonitorAO_initial(MonitorAO * const me, QEvt const * const e) {
    (void)e;
    me->check_count = 0;
    QTimeEvt_ctorX(&me->timeEvt, &me->super, MONITOR_TIMEOUT_SIG, 0U);
    return Q_TRAN(&MonitorAO_active);
}

static QState MonitorAO_active(MonitorAO * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            QTimeEvt_armX(&me->timeEvt, 500U, 500U); /* 5 second intervals */
            status = Q_HANDLED();
            break;
        }

        case MONITOR_TIMEOUT_SIG: {
            rt_thread_t t = rt_thread_self();
            rt_kprintf("[MonitorAO] Thread: %s, Prio: %d, Addr: 0x%08x\n",
                       t ? t->name : "ISR",
                       t ? t->current_priority : -1,
                       (unsigned int)t);
            me->check_count++;
            rt_kprintf("Monitor: Health check #%u - All systems OK. Context: %s\n",
                       me->check_count,
                       rt_thread_self() ? rt_thread_self()->name : "ISR");

            /* Update health statistics using atomic operation to avoid mutex in ISR-like context */
            // rt_hw_interrupt_disable();
            g_system_stats.health_checks++; /* use rt_atomic_inc instead? */
            // rt_hw_interrupt_enable();

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

void MonitorAO_ctor(void) {
    QActive_ctor(&l_monitorAO.super, Q_STATE_CAST(&MonitorAO_initial));
}

/*
 * BSP initialization
 * @return RT_EOK on success, other values indicate error codes
 */
static rt_err_t bsp_init(void)
{
    rt_kprintf("[System] QActive Demo v%s\n", VERSION);
    rt_kprintf("[System] Build: %s %s\n", __DATE__, __TIME__);
    rt_kprintf("[System] QActive Demo with RT-Thread integration\n");

    return RT_EOK;
}

/*
 * Active Objects initialization
 * @return RT_EOK on success, other values indicate error codes
 */
static rt_err_t ao_init(void)
{

    /* QF framework initialization */
    QF_init();
    QF_psInit(subscrSto, Q_DIM(subscrSto)); /* Initialize publish-subscribe table */
    rt_kprintf("[System] QF framework initialized\n");


    /* Print event struct sizes for debugging and pool design. */
    rt_kprintf("sizeof(QEvt)=%d\n", (int)sizeof(QEvt));
    rt_kprintf("sizeof(SensorDataEvt)=%d\n", (int)sizeof(SensorDataEvt));
    rt_kprintf("sizeof(WorkerWorkEvt)=%d\n", (int)sizeof(WorkerWorkEvt));

    /* Initialize event pools. All 16-byte events use shared16Pool. */
    QF_poolInit(basicEventPool, sizeof(basicEventPool), sizeof(QEvt));
    rt_kprintf("[System] Initializing basic event pool...\n");
    QF_poolInit(shared16Pool, sizeof(shared16Pool), sizeof(SensorDataEvt));
    rt_kprintf("[System] Initializing shared 8-byte event pool for SensorDataEvt, ProcessorResultEvt, WorkerWorkEvt...\n");

    /* Initialize RT-Thread integration */
    rt_integration_init();

    /* Construct active objects */
    SensorAO_ctor();
    QActive_setAttr(AO_Sensor, THREAD_NAME_ATTR, "sensor_ao");
    ProcessorAO_ctor();
    QActive_setAttr(AO_Processor, THREAD_NAME_ATTR, "processor_ao");
    WorkerAO_ctor();
    QActive_setAttr(AO_Worker, THREAD_NAME_ATTR, "worker_ao");
    MonitorAO_ctor();
    QActive_setAttr(AO_Monitor, THREAD_NAME_ATTR, "monitor_ao");
    rt_kprintf("[System] AO thread names set\n");

    /* Start Sensor AO */
    rt_kprintf("[System] Starting Sensor AO...\n");
    QACTIVE_START(AO_Sensor,
                  SENSOR_PRIO,            /* Priority */
                  sensorQueueSto,         /* Event queue storage */
                  SENSOR_QUEUE_SIZE,      /* Queue size */
                  sensorStackSto,         /* Thread stack storage */
                  sizeof(sensorStackSto), /* Stack size */
                  (void *)0);             /* Extra parameter */
    rt_kprintf("[System] Sensor AO started, prio=%u, thread name: %s\n",
               SENSOR_PRIO, AO_Sensor->thread.name ? AO_Sensor->thread.name : "NULL");

    /* Start Processor AO */
    rt_kprintf("[System] Starting Processor AO...\n");
    QACTIVE_START(AO_Processor,
                  PROCESSOR_PRIO,            /* Priority */
                  processorQueueSto,         /* Event queue storage */
                  PROCESSOR_QUEUE_SIZE,      /* Queue size */
                  processorStackSto,         /* Thread stack storage */
                  sizeof(processorStackSto), /* Stack size */
                  (void *)0);                /* Extra parameter */
    rt_kprintf("[System] Processor AO started, prio=%u, thread name: %s\n",
               PROCESSOR_PRIO, AO_Processor->thread.name ? AO_Processor->thread.name : "NULL");

    /* Start Worker AO */
    rt_kprintf("[System] Starting Worker AO...\n");
    QACTIVE_START(AO_Worker,
                  WORKER_PRIO,            /* Priority */
                  workerQueueSto,         /* Event queue storage */
                  WORKER_QUEUE_SIZE,      /* Queue size */
                  workerStackSto,         /* Thread stack storage */
                  sizeof(workerStackSto), /* Stack size */
                  (void *)0);             /* Extra parameter */
    rt_kprintf("[System] Worker AO started, prio=%u, thread name: %s\n",
               WORKER_PRIO, AO_Worker->thread.name ? AO_Worker->thread.name : "NULL");

    /* Start Monitor AO */
    rt_kprintf("[System] Starting Monitor AO...\n");
    QACTIVE_START(AO_Monitor,
                  MONITOR_PRIO,            /* Priority */
                  monitorQueueSto,         /* Event queue storage */
                  MONITOR_QUEUE_SIZE,      /* Queue size */
                  monitorStackSto,         /* Thread stack storage */
                  sizeof(monitorStackSto), /* Stack size */
                  (void *)0);              /* Extra parameter */
    rt_kprintf("[System] Monitor AO started, prio=%u, thread name: %s\n",
               MONITOR_PRIO, AO_Monitor->thread.name ? AO_Monitor->thread.name : "NULL");

    /* Start RT-Thread integration components */
    rt_integration_start();

    return RT_EOK;
}

#ifdef RT_USING_MSH
/*
 * MSH command processing functions
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
MSH_CMD_EXPORT(cmd_qactive_control, QActive control : start/stop/stats/config);
#endif

/* Main function entry */
int main(void)
{
    rt_err_t ret;

    /* BSP initialization */
    ret = bsp_init();
    if (RT_EOK != ret) {
        return (int)ret;
    }

    /* Active objects initialization */
    ret = ao_init();
    if (RT_EOK != ret) {
        return (int)ret;
    }

    rt_kprintf("[System] System startup completed\n");
    rt_kprintf("[System] Type 'qactive_control start' to begin demo\n");

    /* Start QF scheduler */
    int ret_val = (int)QF_run();
    rt_kprintf("[main] QF_run() returned: %d\n", ret_val);
    rt_kprintf("[main] yielding CPU after QF_run()...\n");
    rt_thread_yield();
    return ret_val;
}

#endif /* QPC_USING_QACTIVE_DEMO */
