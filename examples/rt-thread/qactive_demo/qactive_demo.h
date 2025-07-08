/*============================================================================
* Product: QActive Demo for RT-Thread
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
/* QActive Demo Signals */
/*==========================================================================*/
enum QActiveDemoSignals {
    SENSOR_TIMEOUT_SIG = Q_USER_SIG,
    SENSOR_DATA_SIG,
    SENSOR_READ_SIG,
    PROCESSOR_CONFIG_SIG,
    PROCESSOR_START_SIG,
    WORKER_WORK_SIG,
    MONITOR_TIMEOUT_SIG,
    
    MAX_DEMO_SIG
};

/*==========================================================================*/
/* QActive Demo Events */
/*==========================================================================*/
typedef struct {
    QEvt super;
    uint32_t temperature;
    uint32_t pressure;
    uint32_t timestamp;
} SensorDataEvt;

typedef struct {
    QEvt super;
    uint32_t sensor_rate;
    uint32_t storage_interval;
} ProcessorConfigEvt;

typedef struct {
    QEvt super;
    uint32_t data_id;
    uint32_t data_size;
    uint32_t priority;
} WorkerWorkEvt;

/*==========================================================================*/
/* Active Object Handles */
/*==========================================================================*/
extern QActive * const AO_Sensor;
extern QActive * const AO_Processor;
extern QActive * const AO_Worker;
extern QActive * const AO_Monitor;

/*==========================================================================*/
/* QActive Demo Functions */
/*==========================================================================*/
void SensorAO_ctor(void);
void ProcessorAO_ctor(void);
void WorkerAO_ctor(void);
void MonitorAO_ctor(void);

#endif /* QACTIVE_DEMO_H_ */