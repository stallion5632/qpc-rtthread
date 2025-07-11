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
#ifndef QACTIVE_DEMO_H_
#define QACTIVE_DEMO_H_

#include "qpc.h"
#include <rtthread.h>

/*==========================================================================*/
/* QActive Demo Signals - Standardized from lite version */
/*==========================================================================*/
enum QActiveDemoSignals {
    SENSOR_READ_SIG = Q_USER_SIG,
    SENSOR_DATA_SIG,
    PROCESSOR_START_SIG,
    PROCESSOR_RESULT_SIG,
    WORKER_WORK_SIG,
    MONITOR_CHECK_SIG,
    TIMEOUT_SIG,
    WORKER_TIMEOUT_SIG,
    MONITOR_TIMEOUT_SIG,
    /* Additional RT-Thread integration signals */
    SENSOR_TIMEOUT_SIG,
    PROCESSOR_CONFIG_SIG,
    MAX_DEMO_SIG
};

/*==========================================================================*/
/* QActive Demo Events - Unified from both versions */
/*==========================================================================*/
/* Primary data event (from lite version) */
typedef struct {
    QEvt super;
    uint32_t data;
} SensorDataEvt;

/* Result event (from lite version) */
typedef struct {
    QEvt super;
    uint32_t result;
} ProcessorResultEvt;

/* Work event (from lite version with RT-Thread extensions) */
typedef struct {
    QEvt super;
    uint32_t work_id;
    uint32_t data_size;     /* RT-Thread extension */
    uint32_t priority;      /* RT-Thread extension */
} WorkerWorkEvt;

/* Configuration event (RT-Thread specific) */
typedef struct {
    QEvt super;
    uint32_t sensor_rate;
    uint32_t storage_interval;
    uint32_t system_flags;
} ProcessorConfigEvt;

/*==========================================================================*/
/* Active Object Handles */
/*==========================================================================*/
extern QActive * const AO_Sensor;
extern QActive * const AO_Processor;
extern QActive * const AO_Worker;
extern QActive * const AO_Monitor;

/*==========================================================================*/
/* QActive Demo Functions - Standardized naming */
/*==========================================================================*/
void QActiveDemo_init(void);
int qactive_demo_start(void);

#endif /* QACTIVE_DEMO_H_ */
