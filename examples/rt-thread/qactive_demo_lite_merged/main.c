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

#ifdef QPC_USING_QACTIVE_DEMO_LITE_MERGED
#ifdef RT_USING_FINSH

#include <finsh.h>
#include <stdio.h>

#include <finsh.h>
#include <stdio.h>
Q_DEFINE_THIS_FILE

/* QF publish-subscribe table */
#define MAX_PUB_SIG 32U
static QSubscrList subscrSto[MAX_PUB_SIG];

/* Signal and event types are defined in qactive_demo.h to avoid duplication. */

/* -------------------------------------------------------------------------- */
/* Combined Active Object 1: SPWAO */
/* -------------------------------------------------------------------------- */
typedef struct
{
    QActive super;
    QTimeEvt sensorTimeEvt;
    uint32_t sensor_count;
    uint32_t processed_count;
    QTimeEvt workerTimeEvt; /* Worker time event */
    uint32_t work_count;    /* Worker count */
} SPWAO;

static SPWAO l_SPWAO;
QActive *const AO_SPW = &l_SPWAO.super;

/* SPWAO state functions */
static QState SPWAO_initial(SPWAO *const me, QEvt const *const e);
static QState SPWAO_idle(SPWAO *const me, QEvt const *const e);
static QState SPWAO_reading(SPWAO *const me, QEvt const *const e);
static QState SPWAO_processing(SPWAO *const me, QEvt const *const e);
static QState SPWAO_working(SPWAO *const me, QEvt const *const e);

/*==========================================================================*/
/* Monitor Active Object (remains separate) */
/*==========================================================================*/
typedef struct
{
    QActive super;
    QTimeEvt timeEvt;
    uint32_t check_count;
} MonitorAO;

static MonitorAO l_monitorAO;
QActive *const AO_Monitor = &l_monitorAO.super;

/* Monitor state functions */
static QState MonitorAO_initial(MonitorAO *const me, QEvt const *const e);
static QState MonitorAO_monitoring(MonitorAO *const me, QEvt const *const e);

/*==========================================================================*/
/* SPWAO Implementation */
/*==========================================================================*/
static void SPWAO_ctor(void)
{
    SPWAO *me = &l_SPWAO;

    QActive_ctor(&me->super, Q_STATE_CAST(&SPWAO_initial));
    QTimeEvt_ctorX(&me->sensorTimeEvt, &me->super, TIMEOUT_SIG, 0U);
    /* Initialize worker's timer */
    QTimeEvt_ctorX(&me->workerTimeEvt, &me->super, WORKER_TIMEOUT_SIG, 0U);
    me->sensor_count = 0;
    me->processed_count = 0;
    me->work_count = 0; /* Initialize worker's count */
}

static QState SPWAO_initial(SPWAO *const me, QEvt const *const e)
{
    (void)e;

    /* Subscribe to signals relevant for Sensor, Processor, and Worker parts */
    QActive_subscribe(&me->super, SENSOR_READ_SIG);
    QActive_subscribe(&me->super, SENSOR_DATA_SIG);
    QActive_subscribe(&me->super, PROCESSOR_START_SIG);
    QActive_subscribe(&me->super, WORKER_WORK_SIG);

    /*
    * 本合并AO（SPWAO）将Sensor、Processor和Worker的功能全部集成在同一个AO线程中，
    * 通过状态机分阶段处理各自的逻辑。
    *
    * 设计思路：
    * - 以Idle为初始状态，等待采集或定时事件；
    * - 状态迁移依次为：Idle -> Reading -> Processing -> Working -> Idle，形成闭环；
    * - 每个阶段的事件和处理都在本AO线程内完成，不再拆分为多个AO；
    * - 事件驱动状态迁移，保证流程清晰且易于维护；
    * - 如果某阶段收到其他相关事件（如手动触发），也能正确响应并切换状态；
    * - 这样可以用一个AO线程实现完整的数据采集、处理和工作流程，简化系统结构。
    */
    return Q_TRAN(&SPWAO_idle);
}

/* Sensor part of the combined AO */

static QState SPWAO_idle(SPWAO *const me, QEvt const *const e) {
    QState status;
    switch (e->sig) {
    case Q_ENTRY_SIG:
        rt_kprintf("SPW_AO: Idle - waiting for SENSOR_READ_SIG or TIMEOUT_SIG\n");
        /* periodic wakeup */
        QTimeEvt_armX(&me->sensorTimeEvt, 10, 10);
        status = Q_HANDLED();
        break;
    case Q_EXIT_SIG:
        QTimeEvt_disarm(&me->sensorTimeEvt);
        status = Q_HANDLED();
        break;
    case SENSOR_READ_SIG:
        rt_kprintf("SPW_AO: Idle - manual read triggered\n");
        status = Q_TRAN(&SPWAO_reading);
        break;
    case TIMEOUT_SIG:
        rt_kprintf("SPW_AO: Idle - periodic read triggered\n");
        status = Q_TRAN(&SPWAO_reading);
        break;
    default:
        status = Q_SUPER(&QHsm_top);
        break;
    }
    return status;
}

static QState SPWAO_reading(SPWAO *const me, QEvt const *const e) {
    QState status;
    switch (e->sig) {
    case Q_ENTRY_SIG:
        me->sensor_count++;
        uint32_t sensor_data = me->sensor_count * 10 + (rt_tick_get() & 0xFF);
        rt_kprintf("SPW_AO: Reading - Reading %u, data = %u\n", me->sensor_count, sensor_data);
        /* post SENSOR_DATA_SIG event to self */
        SensorDataEvt *evt = Q_NEW(SensorDataEvt, SENSOR_DATA_SIG);
        evt->data = sensor_data;
        QACTIVE_POST(&me->super, &evt->super, me);
        status = Q_HANDLED();
        break;
    case SENSOR_DATA_SIG:
        status = Q_TRAN(&SPWAO_processing);
        break;
    default:
        status = Q_SUPER(&QHsm_top);
        break;
    }
    return status;
}

static QState SPWAO_processing(SPWAO *const me, QEvt const *const e) {
    QState status;
    switch (e->sig) {
    case Q_ENTRY_SIG:
        me->processed_count++;
        rt_kprintf("SPW_AO: Processing - processing data (count: %u)\n", me->processed_count);
        /* post PROCESSOR_RESULT_SIG event to self */
        ProcessorResultEvt *evt = Q_NEW(ProcessorResultEvt, PROCESSOR_RESULT_SIG);
        evt->result = me->processed_count * 100;
        QACTIVE_POST(&me->super, &evt->super, me);
        status = Q_HANDLED();
        break;
    case PROCESSOR_RESULT_SIG:
        status = Q_TRAN(&SPWAO_working);
        break;
    default:
        status = Q_SUPER(&QHsm_top);
        break;
    }
    return status;
}

static QState SPWAO_working(SPWAO *const me, QEvt const *const e) {
    QState status;
    switch (e->sig) {
    case Q_ENTRY_SIG:
        me->work_count++;
        rt_kprintf("SPW_AO: Working - processing work (total: %u)\n", me->work_count);
        /* simulate work processing, post WORKER_TIMEOUT_SIG to self */
        QTimeEvt_armX(&me->workerTimeEvt, 5, 0); /* 50ms delay */
        status = Q_HANDLED();
        break;
    case Q_EXIT_SIG:
        QTimeEvt_disarm(&me->workerTimeEvt);
        status = Q_HANDLED();
        break;
    case WORKER_TIMEOUT_SIG:
        rt_kprintf("SPW_AO: Working - work completed\n");
        /* notify MonitorAO */
        QACTIVE_POST(AO_Monitor, Q_NEW(QEvt, WORKER_TIMEOUT_SIG), me);
        status = Q_TRAN(&SPWAO_idle);
        break;
    default:
        status = Q_SUPER(&QHsm_top);
        break;
    }
    return status;
}

/* -------------------------------------------------------------------------- */
/* Monitor Active Object Implementation (original, kept separate) */
/* -------------------------------------------------------------------------- */
static void MonitorAO_ctor(void)
{
    MonitorAO *me = &l_monitorAO;

    QActive_ctor(&me->super, Q_STATE_CAST(&MonitorAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, MONITOR_TIMEOUT_SIG, 0U);
    me->check_count = 0;
}

static QState MonitorAO_initial(MonitorAO *const me, QEvt const *const e)
{
    (void)e;

    /* Subscribe to monitor signals */
    QActive_subscribe(&me->super, MONITOR_CHECK_SIG);

    return Q_TRAN(&MonitorAO_monitoring);
}

static QState MonitorAO_monitoring(MonitorAO *const me, QEvt const *const e)
{
    QState status;

    switch (e->sig)
    {
    case Q_ENTRY_SIG:
    {
        rt_kprintf("Monitor: Starting periodic monitoring\n");
        QTimeEvt_armX(&me->timeEvt, 10, 10); /* 100 ms intervals */
        status = Q_HANDLED();
        break;
    }

    case Q_EXIT_SIG:
    {
        QTimeEvt_disarm(&me->timeEvt);
        status = Q_HANDLED();
        break;
    }

    case MONITOR_TIMEOUT_SIG:
    {
        /* Periodic monitoring */
        me->check_count++;
        rt_kprintf("Monitor: System check #%u - All systems operational\n",
                   me->check_count);

        /* Send check signal to self (could be used for health checks) */
        QACTIVE_POST(&me->super, Q_NEW(QEvt, MONITOR_CHECK_SIG), me);

        status = Q_HANDLED();
        break;
    }

    case MONITOR_CHECK_SIG:
    {
        rt_kprintf("Monitor: Health check completed\n");
        status = Q_HANDLED();
        break;
    }

    default:
    {
        status = Q_SUPER(&QHsm_top);
        break;
    }
    }

    return status;
}

/* Initialize event pools - must be in ascending order of event size */
ALIGN(RT_ALIGN_SIZE)
static QF_MPOOL_EL(QEvt) basicEventPool[50];
ALIGN(RT_ALIGN_SIZE)
static QF_MPOOL_EL(SensorDataEvt) shared8Pool[60];

/* -------------------------------------------------------------------------- */
/* QActive Demo Initialization */
/* -------------------------------------------------------------------------- */
void QActiveDemo_init(void)
{
    static uint8_t isInitialized = 0;

    if (isInitialized != 0)
    {
        printf("QActive Demo: Already initialized, skipping...\n");
        return;
    }
    isInitialized = 1;

    QF_init();
    printf("QActive Demo: Initializing QF framework...\n");
    QF_psInit(subscrSto, Q_DIM(subscrSto));
    printf("QActive Demo: Initializing publish-subscribe system...\n");

    printf("sizeof(QEvt)=%d\n", (int)sizeof(QEvt));
    printf("sizeof(SensorDataEvt)=%d\n", (int)sizeof(SensorDataEvt));
    printf("sizeof(ProcessorResultEvt)=%d\n", (int)sizeof(ProcessorResultEvt));
    printf("sizeof(WorkerWorkEvt)=%d\n", (int)sizeof(WorkerWorkEvt));

    QF_poolInit(basicEventPool, sizeof(basicEventPool), sizeof(QEvt));
    printf("QActive Demo: Initializing basic event pool...\n");
    QF_poolInit(shared8Pool, sizeof(shared8Pool), sizeof(SensorDataEvt));
    printf("QActive Demo: Initializing shared 8-byte event pool for SensorDataEvt, ProcessorResultEvt, WorkerWorkEvt...\n");

    /* Construct the two active objects. */
    SPWAO_ctor();
    /*!!! MonitorAO is still separate !!!*/
    MonitorAO_ctor();
}

/* -------------------------------------------------------------------------- */
/* QActive Demo Startup */
/* -------------------------------------------------------------------------- */
int qactive_demo_start(void)
{
    static uint8_t isStarted = 0;
    if (isStarted != 0)
    {
        printf("QActive Demo: Already started, skipping...\n");
        return 0;
    }
    isStarted = 1;

    /* Event queue storage for the two new AOs */
    ALIGN(RT_ALIGN_SIZE)
    static QEvt const *spwQueue[30]; /* Larger queue for combined AO */
    ALIGN(RT_ALIGN_SIZE)
    static QEvt const *monitorQueue[10]; /* Monitor queue unchanged */

    /* Stack storage for the two new AOs */
    ALIGN(RT_ALIGN_SIZE)
    static uint8_t spwStack[3072]; /* Larger stack for combined AO */
    ALIGN(RT_ALIGN_SIZE)
    static uint8_t monitorStack[1024]; /* Monitor stack unchanged */

    printf("QActive Demo: Initializing...\n");
    QActiveDemo_init();

    printf("QActive Demo: Starting with 2 QActive objects (1 combined, 1 original Monitor)...\n");
    /* Start combined active object */
    QACTIVE_START(AO_SPW,
                  1U, /* Priority (highest for the main data flow) */
                  spwQueue, Q_DIM(spwQueue),
                  spwStack, sizeof(spwStack),
                  (void *)0);
    printf("QActive Demo: SPW AO started\n");

    /* Start Monitor AO */
    QACTIVE_START(AO_Monitor,
                  2U, /* Priority (lower than combined AO) */
                  monitorQueue, Q_DIM(monitorQueue),
                  monitorStack, sizeof(monitorStack),
                  (void *)0);
    printf("QActive Demo: Monitor AO started\n");

    rt_kprintf("QActive Demo: Started - 2 QActive objects\n");

    return QF_run(); /* Run the QF application */
}

/* RT-Thread MSH command exports */
MSH_CMD_EXPORT(qactive_demo_start, start QActive demo with 2 AOs);

/* RT-Thread application auto-initialization */
static int qactive_demo_init(void)
{
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
#endif /* QPC_USING_QACTIVE_DEMO_LITE_MERGED */
