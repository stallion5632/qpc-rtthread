/*============================================================================
* Product: QActive Demo for RT-Thread - Main Application
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

#ifdef QPC_USING_QACTIVE_DEMO
#ifdef RT_USING_FINSH

#include <finsh.h>
#include <stdio.h>


Q_DEFINE_THIS_FILE

/* QF publish-subscribe table */
#define MAX_PUB_SIG     32U
static QSubscrList subscrSto[MAX_PUB_SIG];

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
/* Worker Active Object */
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
static QState MonitorAO_monitoring(MonitorAO * const me, QEvt const * const e);

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

            /* Simulate processing time */
            uint32_t result = me->processed_count * 100;

            /* Send result and notify worker AO */
            ProcessorResultEvt *evt = Q_NEW(ProcessorResultEvt, PROCESSOR_RESULT_SIG);
            evt->result = result;

            /* Post work to worker AO */
            WorkerWorkEvt *work_evt = Q_NEW(WorkerWorkEvt, WORKER_WORK_SIG);
            work_evt->work_id = me->processed_count;
            QACTIVE_POST(AO_Worker, &work_evt->super, me);

             /* Post work again */
            WorkerWorkEvt *work_evt2 = Q_NEW(WorkerWorkEvt, WORKER_WORK_SIG);
            /* Distinguish by different IDs */
            work_evt2->work_id = me->processed_count + 1000;
            QACTIVE_POST(AO_Worker, &work_evt2->super, me);

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
/* Worker Active Object Implementation */
/*==========================================================================*/
static void WorkerAO_ctor(void) {
    WorkerAO *me = &l_workerAO;

    QActive_ctor(&me->super, Q_STATE_CAST(&WorkerAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, WORKER_TIMEOUT_SIG, 0U);
    me->work_count = 0;
}

static QState WorkerAO_initial(WorkerAO * const me, QEvt const * const e) {
    (void)e;

    /* Subscribe to worker signals */
    QActive_subscribe(&me->super, WORKER_WORK_SIG);

    return Q_TRAN(&WorkerAO_idle);
}

static QState WorkerAO_idle(WorkerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Worker: Idle, waiting for work\n");
            status = Q_HANDLED();
            break;
        }

        case WORKER_WORK_SIG: {
            WorkerWorkEvt const *evt = (WorkerWorkEvt const *)e;
            rt_kprintf("Worker: Received work ID %u\n", evt->work_id);
            status = Q_TRAN(&WorkerAO_working);
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
            rt_kprintf("Worker: Processing work (total: %u)\n", me->work_count);

            /* Simulate work processing with timeout */
            QTimeEvt_armX(&me->timeEvt, 50, 0); /* 500ms delay */

            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->timeEvt);
            status = Q_HANDLED();
            break;
        }

        case WORKER_TIMEOUT_SIG: {
            rt_kprintf("Worker: Work completed\n");
            status = Q_TRAN(&WorkerAO_idle);
            break;
        }

        case WORKER_WORK_SIG: {
            WorkerWorkEvt const *evt = (WorkerWorkEvt const *)e;
            rt_kprintf("Worker: Additional work ID %u queued\n", evt->work_id);
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
/* Monitor Active Object Implementation */
/*==========================================================================*/
static void MonitorAO_ctor(void) {
    MonitorAO *me = &l_monitorAO;

    QActive_ctor(&me->super, Q_STATE_CAST(&MonitorAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, MONITOR_TIMEOUT_SIG, 0U);
    me->check_count = 0;
}

static QState MonitorAO_initial(MonitorAO * const me, QEvt const * const e) {
    (void)e;

    /* Subscribe to monitor signals */
    QActive_subscribe(&me->super, MONITOR_CHECK_SIG);

    return Q_TRAN(&MonitorAO_monitoring);
}

static QState MonitorAO_monitoring(MonitorAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Monitor: Starting periodic monitoring\n");
            QTimeEvt_armX(&me->timeEvt, 300, 300); /* 3 second intervals */
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->timeEvt);
            status = Q_HANDLED();
            break;
        }

        case MONITOR_TIMEOUT_SIG: {
            /* Periodic monitoring */
            me->check_count++;
            rt_kprintf("Monitor: System check #%u - All systems operational\n",
                      me->check_count);

            /* Send check signal to self (could be used for health checks) */
            QACTIVE_POST(&me->super, Q_NEW(QEvt, MONITOR_CHECK_SIG), me);

            status = Q_HANDLED();
            break;
        }

        case MONITOR_CHECK_SIG: {
            rt_kprintf("Monitor: Health check completed\n");
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
/* Event pools must be initialized in ascending order of event size. */
/* Only one pool is allowed for events of the same size. */
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(QEvt) basicEventPool[50];              /* 4-byte event pool for QEvt only */
ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(SensorDataEvt) shared8Pool[60];        /* 8-byte event pool shared by SensorDataEvt, ProcessorResultEvt, WorkerWorkEvt */

/*==========================================================================*/
/* QActive Demo Initialization */
/*==========================================================================*/
void QActiveDemo_init(void) {
    static uint8_t isInitialized = 0;

    /* 防止重复初始化 */
    if (isInitialized != 0) {
        printf("QActive Demo: Already initialized, skipping...\n");
        return;
    }
    isInitialized = 1;

    /* Initialize QF framework */
    QF_init();
    printf("QActive Demo: Initializing QF framework...\n");
    /* Initialize publish-subscribe system */
    QF_psInit(subscrSto, Q_DIM(subscrSto));
    printf("QActive Demo: Initializing publish-subscribe system...\n");

    /* Print event struct sizes for debugging and pool design. */
    printf("sizeof(QEvt)=%d\n", (int)sizeof(QEvt));
    printf("sizeof(SensorDataEvt)=%d\n", (int)sizeof(SensorDataEvt));
    printf("sizeof(ProcessorResultEvt)=%d\n", (int)sizeof(ProcessorResultEvt));
    printf("sizeof(WorkerWorkEvt)=%d\n", (int)sizeof(WorkerWorkEvt));

    /* Initialize event pools. All 8-byte events use shared8Pool. */
    QF_poolInit(basicEventPool, sizeof(basicEventPool), sizeof(QEvt));
    printf("QActive Demo: Initializing basic event pool...\n");
    QF_poolInit(shared8Pool, sizeof(shared8Pool), sizeof(SensorDataEvt));
    printf("QActive Demo: Initializing shared 8-byte event pool for SensorDataEvt, ProcessorResultEvt, WorkerWorkEvt...\n");

    /* Construct all active objects. */
    SensorAO_ctor();
    ProcessorAO_ctor();
    WorkerAO_ctor();
    MonitorAO_ctor();
}

/*==========================================================================*/
/* QActive Demo Startup */
/*==========================================================================*/
int qactive_demo_start(void) {
    static uint8_t isStarted = 0;
    if (isStarted != 0) {
        printf("QActive Demo: Already started, skipping...\n");
        return 0;
    }
    isStarted = 1;

    /* Event queue storage */
    ALIGN(RT_ALIGN_SIZE) static QEvt const *sensorQueue[10];
    ALIGN(RT_ALIGN_SIZE) static QEvt const *processorQueue[10];
    ALIGN(RT_ALIGN_SIZE) static QEvt const *workerQueue[10];
    ALIGN(RT_ALIGN_SIZE) static QEvt const *monitorQueue[10];

    /* Stack storage */
    ALIGN(RT_ALIGN_SIZE) static uint8_t sensorStack[1024];
    ALIGN(RT_ALIGN_SIZE) static uint8_t processorStack[1024];
    ALIGN(RT_ALIGN_SIZE) static uint8_t workerStack[1024];
    ALIGN(RT_ALIGN_SIZE) static uint8_t monitorStack[1024];
    printf("QActive Demo: Initializing...\n");
    /* Initialize the demo */
    QActiveDemo_init();

    printf("QActive Demo: Starting with 4 QActive objects...\n");
    /* Start active objects */
    QACTIVE_START(AO_Sensor,
                  1U, /* Priority */
                  sensorQueue, Q_DIM(sensorQueue),
                  sensorStack, sizeof(sensorStack),
                  (void *)0);
    printf("QActive Demo: Sensor AO started\n");
    QACTIVE_START(AO_Processor,
                  2U, /* Priority */
                  processorQueue, Q_DIM(processorQueue),
                  processorStack, sizeof(processorStack),
                  (void *)0);
    printf("QActive Demo: Processor AO started\n");
    QACTIVE_START(AO_Worker,
                  3U, /* Priority */
                  workerQueue, Q_DIM(workerQueue),
                  workerStack, sizeof(workerStack),
                  (void *)0);
    printf("QActive Demo: Worker AO started\n");
    QACTIVE_START(AO_Monitor,
                  4U, /* Priority */
                  monitorQueue, Q_DIM(monitorQueue),
                  monitorStack, sizeof(monitorStack),
                  (void *)0);

    rt_kprintf("QActive Demo: Started - 4 QActive objects\n");

    return QF_run(); /* Run the QF application */
}

/* RT-Thread MSH command exports */
MSH_CMD_EXPORT(qactive_demo_start, start QActive demo with 4 AOs);

/* RT-Thread application auto-initialization */
static int qactive_demo_init(void) {
    rt_kprintf("=== QActive Demo Auto-Initialize ===\n");
    return qactive_demo_start();
}
INIT_APP_EXPORT(qactive_demo_init);

int main(void)
{
    QActiveDemo_init();
    rt_kprintf("[System] Starting QF application\n");

    int ret = qactive_demo_start();
    rt_kprintf("[System] System startup completed\n");
    return ret;
}

#endif /* RT_USING_FINSH */
#endif /* QPC_USING_QACTIVE_DEMO */
