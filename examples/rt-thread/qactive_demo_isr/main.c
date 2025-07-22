/*============================================================================
* QP/C Real-Time Embedded Framework (RTEF)
* Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
*
* SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
*
* This software is dual-licensed under the terms of the open source GNU
* General Public License version 3 (or any later version), or alternatively,
* under the terms of one of the closed source Quantum Leaps commercial
* licenses.
*
* Contact information:
* <www.state-machine.com>
* <info@state-machine.com>
============================================================================*/
/*!
 * @date Last updated on: 2025-07-22
 * @version Last updated for: @ref qpc_7_3_0
 *
 * @file
 * @brief ISR-safe event publishing usage example for RT-Thread
 */

/*
 * This file demonstrates how to use the ISR-safe event publishing
 * system with RT-Thread. Only compile this demo when ISR relay is enabled.
 */

#include "qactive_demo.h"
#include "qf_opt_layer.h"
#include <rtthread.h>
#include <stdlib.h> /* For atoi */

#ifndef rt_hw_interrupt_disable
#define rt_hw_interrupt_disable() (0)
#endif
#ifndef rt_hw_interrupt_enable
#define rt_hw_interrupt_enable(x) ((void)(x))
#endif

Q_DEFINE_THIS_FILE

#define MAX_PUB_SIG 32U
static QSubscrList subscrSto[MAX_PUB_SIG];

/* use QF_postFromISR post event */
static void isr_post_sensor_data(QActive *target, rt_uint16_t value)
{
    SensorDataEvt *evt = NULL;
    evt = Q_NEW(SensorDataEvt, SENSOR_DATA_SIG);
    if (evt != NULL)
    {
        evt->data = value;
        rt_kprintf("[ISR] QF_postFromISR AO_Sensor SENSOR_DATA_SIG, value=%u\n", value);
        (void)QF_postFromISR(target, &evt->super);
    }
}
static void isr_post_processor_start(QActive *target)
{
    QEvt *evt = NULL;
    evt = Q_NEW(QEvt, PROCESSOR_START_SIG);
    if (evt != NULL)
    {
        rt_kprintf("[ISR] QF_postFromISR AO_Processor PROCESSOR_START_SIG\n");
        (void)QF_postFromISR(target, evt);
    }
}
static void isr_post_worker_work(QActive *target, rt_uint16_t workid)
{
    WorkerWorkEvt *evt = NULL;
    evt = Q_NEW(WorkerWorkEvt, WORKER_WORK_SIG);
    if (evt != NULL)
    {
        evt->work_id = workid;
        rt_kprintf("[ISR] QF_postFromISR AO_Worker WORKER_WORK_SIG, workid=%u\n", workid);
        (void)QF_postFromISR(target, &evt->super);
    }
}
static void isr_post_monitor_check(QActive *target)
{
    QEvt *evt = NULL;
    evt = Q_NEW(QEvt, MONITOR_CHECK_SIG);
    if (evt != NULL)
    {
        rt_kprintf("[ISR] QF_postFromISR AO_Monitor MONITOR_CHECK_SIG\n");
        (void)QF_postFromISR(target, evt);
    }
}

/*
 * @brief Sensor Active Object structure
 */
typedef struct
{
    QActive super;
    QTimeEvt timeEvt;
    uint32_t sensor_count;
} SensorAO;

/*
 * @brief Processor Active Object structure
 */
typedef struct
{
    QActive super;
    uint32_t processed_count;
} ProcessorAO;

/*
 * @brief Worker Active Object structure
 */
typedef struct
{
    QActive super;
    QTimeEvt timeEvt;
    uint32_t work_count;
} WorkerAO;

/*
 * @brief Monitor Active Object structure
 */
typedef struct
{
    QActive super;
    QTimeEvt timeEvt;
    uint32_t check_count; /* monitor event counter */
} MonitorAO;

static QState SensorAO_initial(SensorAO *const me, QEvt const *const e);
static QState SensorAO_active(SensorAO *const me, QEvt const *const e);
static QState ProcessorAO_initial(ProcessorAO *const me, QEvt const *const e);
static QState ProcessorAO_idle(ProcessorAO *const me, QEvt const *const e);
static QState ProcessorAO_processing(ProcessorAO *const me, QEvt const *const e);
static QState WorkerAO_initial(WorkerAO *const me, QEvt const *const e);
static QState WorkerAO_idle(WorkerAO *const me, QEvt const *const e);
static QState WorkerAO_working(WorkerAO *const me, QEvt const *const e);
static QState MonitorAO_initial(MonitorAO *const me, QEvt const *const e);
static QState MonitorAO_monitoring(MonitorAO *const me, QEvt const *const e);

static SensorAO l_sensorAO;
QActive *const AO_Sensor = &l_sensorAO.super;
static ProcessorAO l_processorAO;
QActive *const AO_Processor = &l_processorAO.super;
static WorkerAO l_workerAO;
QActive *const AO_Worker = &l_workerAO.super;
static MonitorAO l_monitorAO;
QActive *const AO_Monitor = &l_monitorAO.super;

/*==================== AO state machine ====================*/
static QState SensorAO_initial(SensorAO *const me, QEvt const *const e)
{
    QState ret = Q_RET_SUPER;
    (void)e;
    QActive_subscribe(&me->super, SENSOR_READ_SIG);
    ret = Q_TRAN(&SensorAO_active);
    return ret;
}
static QState SensorAO_active(SensorAO *const me, QEvt const *const e)
{
    QState ret = Q_RET_SUPER;
    uint32_t sensor_data = 0U;
    /* SensorAO state machine: handles periodic sensor events and relays via ISR */
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        QTimeEvt_armX(&me->timeEvt, 20U, 20U);
        ret = Q_HANDLED();
        break;
    case Q_EXIT_SIG:
        QTimeEvt_disarm(&me->timeEvt);
        ret = Q_HANDLED();
        break;
    case TIMEOUT_SIG:
        me->sensor_count++;
        sensor_data = (me->sensor_count * 10U) + (rt_tick_get() & 0xFFU);
        rt_kprintf("[AO] SensorAO TIMEOUT_SIG, trigger ISR relay for SENSOR_DATA_SIG=%u\n", sensor_data);
        isr_post_sensor_data(AO_Sensor, (rt_uint16_t)sensor_data);
        ret = Q_HANDLED();
        break;
    case SENSOR_READ_SIG:
        QACTIVE_POST(&me->super, Q_NEW(QEvt, TIMEOUT_SIG), me);
        ret = Q_HANDLED();
        break;
    default:
        ret = Q_SUPER(&QHsm_top);
        break;
    }
    return ret;
}
static void SensorAO_ctor(void)
{
    QActive_ctor(&l_sensorAO.super, Q_STATE_CAST(&SensorAO_initial));
    QTimeEvt_ctorX(&l_sensorAO.timeEvt, &l_sensorAO.super, TIMEOUT_SIG, 0U);
    l_sensorAO.sensor_count = 0U;
}

static QState ProcessorAO_initial(ProcessorAO *const me, QEvt const *const e)
{
    QState ret = Q_RET_SUPER;
    (void)e;
    QActive_subscribe(&me->super, SENSOR_DATA_SIG);
    QActive_subscribe(&me->super, PROCESSOR_START_SIG);
    ret = Q_TRAN(&ProcessorAO_idle);
    return ret;
}
static QState ProcessorAO_idle(ProcessorAO *const me, QEvt const *const e)
{
    QState ret = Q_RET_SUPER;
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        ret = Q_HANDLED();
        break;
    case SENSOR_DATA_SIG:
        me->processed_count++;
        rt_kprintf("[AO] ProcessorAO got SENSOR_DATA_SIG, trigger ISR relay for PROCESSOR_START_SIG\n");
        isr_post_processor_start(AO_Processor);
        ret = Q_TRAN(&ProcessorAO_processing);
        break;
    case PROCESSOR_START_SIG:
        ret = Q_TRAN(&ProcessorAO_processing);
        break;
    default:
        ret = Q_SUPER(&QHsm_top);
        break;
    }
    return ret;
}
static QState ProcessorAO_processing(ProcessorAO *const me, QEvt const *const e)
{
    QState ret = Q_RET_SUPER;
    uint32_t result = 0U;
    ProcessorResultEvt *evt = NULL;
    /* ProcessorAO state machine: processes sensor data and relays work event via ISR */
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        result = me->processed_count * 100U;
        evt = Q_NEW(ProcessorResultEvt, PROCESSOR_RESULT_SIG);
        if (evt != NULL)
        {
            evt->result = result;
        }
        rt_kprintf("[AO] ProcessorAO processing done, trigger ISR relay for WORKER_WORK_SIG=%u\n", me->processed_count);
        isr_post_worker_work(AO_Worker, (rt_uint16_t)me->processed_count);
        ret = Q_TRAN(&ProcessorAO_idle);
        break;
    default:
        ret = Q_SUPER(&QHsm_top);
        break;
    }
    return ret;
}
static void ProcessorAO_ctor(void)
{
    /* ProcessorAO constructor: initialize AO */
    QActive_ctor(&l_processorAO.super, Q_STATE_CAST(&ProcessorAO_initial));
    l_processorAO.processed_count = 0U;
}

static QState WorkerAO_initial(WorkerAO *const me, QEvt const *const e)
{
    QState ret = Q_RET_SUPER;
    (void)e;
    QActive_subscribe(&me->super, WORKER_WORK_SIG);
    ret = Q_TRAN(&WorkerAO_idle);
    return ret;
}
static QState WorkerAO_idle(WorkerAO *const me, QEvt const *const e)
{
    QState ret = Q_RET_SUPER;
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        ret = Q_HANDLED();
        break;
    case WORKER_WORK_SIG:
        me->work_count++;
        rt_kprintf("[AO] WorkerAO got WORKER_WORK_SIG, start work #%u\n", me->work_count);
        QTimeEvt_armX(&me->timeEvt, 10U, 0U);
        ret = Q_TRAN(&WorkerAO_working);
        break;
    default:
        ret = Q_SUPER(&QHsm_top);
        break;
    }
    return ret;
}
static QState WorkerAO_working(WorkerAO *const me, QEvt const *const e)
{
    QState ret = Q_RET_SUPER;
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        ret = Q_HANDLED();
        break;
    case Q_EXIT_SIG:
        QTimeEvt_disarm(&me->timeEvt);
        ret = Q_HANDLED();
        break;
    case WORKER_TIMEOUT_SIG:
        ret = Q_TRAN(&WorkerAO_idle);
        break;
    default:
        ret = Q_SUPER(&QHsm_top);
        break;
    }
    return ret;
}

static void WorkerAO_ctor(void)
{
    QActive_ctor(&l_workerAO.super, Q_STATE_CAST(&WorkerAO_initial));
    QTimeEvt_ctorX(&l_workerAO.timeEvt, &l_workerAO.super, WORKER_TIMEOUT_SIG, 0U);
    l_workerAO.work_count = 0U;
}

static QState MonitorAO_initial(MonitorAO *const me, QEvt const *const e)
{
    QState ret = Q_RET_SUPER;
    (void)e;
    QActive_subscribe(&me->super, MONITOR_CHECK_SIG);
    ret = Q_TRAN(&MonitorAO_monitoring);
    return ret;
}
static QState MonitorAO_monitoring(MonitorAO *const me, QEvt const *const e)
{
    QState ret = Q_RET_SUPER;
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        QTimeEvt_armX(&me->timeEvt, 30U, 30U);
        ret = Q_HANDLED();
        break;
    case Q_EXIT_SIG:
        QTimeEvt_disarm(&me->timeEvt);
        ret = Q_HANDLED();
        break;
    case MONITOR_TIMEOUT_SIG:
        me->check_count++;
        rt_kprintf("[AO] MonitorAO MONITOR_TIMEOUT_SIG, trigger ISR relay for MONITOR_CHECK_SIG\n");
        isr_post_monitor_check(AO_Monitor);
        ret = Q_HANDLED();
        break;
    case MONITOR_CHECK_SIG:
        ret = Q_HANDLED();
        break;
    default:
        ret = Q_SUPER(&QHsm_top);
        break;
    }
    return ret;
}

static void MonitorAO_ctor(void)
{
    QActive_ctor(&l_monitorAO.super, Q_STATE_CAST(&MonitorAO_initial));
    QTimeEvt_ctorX(&l_monitorAO.timeEvt, &l_monitorAO.super, MONITOR_TIMEOUT_SIG, 0U);
    l_monitorAO.check_count = 0U;
}

ALIGN(RT_ALIGN_SIZE)
static QF_MPOOL_EL(QEvt) basicEventPool[50];
ALIGN(RT_ALIGN_SIZE)
static QF_MPOOL_EL(SensorDataEvt) shared8Pool[60];

void QActiveDemo_init(void)
{
    static uint8_t isInitialized = 0U;
    if (isInitialized == 0U)
    {
        isInitialized = 1U;
        QF_init();
        QF_psInit(subscrSto, Q_DIM(subscrSto));
        QF_poolInit(basicEventPool, sizeof(basicEventPool), sizeof(QEvt));
        QF_poolInit(shared8Pool, sizeof(shared8Pool), sizeof(SensorDataEvt));
        SensorAO_ctor();
        ProcessorAO_ctor();
        WorkerAO_ctor();
        MonitorAO_ctor();
    }
}

int qactive_demo_start(void)
{
    static uint8_t isStarted = 0U;
    if (isStarted != 0U)
    {
        /* Already started */
        return 0;
    }
    isStarted = 1U;
    ALIGN(RT_ALIGN_SIZE)
    static QEvt const *sensorQueue[10];
    ALIGN(RT_ALIGN_SIZE)
    static QEvt const *processorQueue[10];
    ALIGN(RT_ALIGN_SIZE)
    static QEvt const *workerQueue[10];
    ALIGN(RT_ALIGN_SIZE)
    static QEvt const *monitorQueue[10];
    ALIGN(RT_ALIGN_SIZE)
    static uint8_t sensorStack[1024];
    ALIGN(RT_ALIGN_SIZE)
    static uint8_t processorStack[1024];
    ALIGN(RT_ALIGN_SIZE)
    static uint8_t workerStack[1024];
    ALIGN(RT_ALIGN_SIZE)
    static uint8_t monitorStack[1024];
    /* Start all active objects and run the QP/C framework */
    QActiveDemo_init();
    QACTIVE_START(AO_Sensor, 1U, sensorQueue, Q_DIM(sensorQueue), sensorStack, sizeof(sensorStack), (void *)0);
    QACTIVE_START(AO_Processor, 2U, processorQueue, Q_DIM(processorQueue), processorStack, sizeof(processorStack), (void *)0);
    QACTIVE_START(AO_Worker, 3U, workerQueue, Q_DIM(workerQueue), workerStack, sizeof(workerStack), (void *)0);
    QACTIVE_START(AO_Monitor, 4U, monitorQueue, Q_DIM(monitorQueue), monitorStack, sizeof(monitorStack), (void *)0);
    rt_kprintf("QActive ISR Demo: Started - 4 QActive objects\n");
    return QF_run();
}

MSH_CMD_EXPORT(qactive_demo_start, start QActive ISR demo with 4 AOs);

static int qactive_demo_isr_init(void)
{
    rt_kprintf("=== QActive ISR Demo Auto-Initialize ===\n");
    return qactive_demo_start();
}
INIT_APP_EXPORT(qactive_demo_isr_init);

int main(void)
{
    int ret = 0;
    /* Main entry: initialize and start QActive ISR demo */
    QActiveDemo_init();
    rt_kprintf("[System] Starting QF ISR demo application\n");
    ret = qactive_demo_start();
    rt_kprintf("[System] System startup completed\n");
    return ret;
}
