/*============================================================================
* Product: QXK Demo for RT-Thread
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
#ifndef QXK_DEMO_H_
#define QXK_DEMO_H_

#include "qpc.h"
#include <rtthread.h>

/*==========================================================================*/
/* QXK Demo Signals */
/*==========================================================================*/
enum QXKDemoSignals {
    SENSOR_READ_SIG = Q_USER_SIG,
    SENSOR_DATA_SIG,
    PROCESSOR_START_SIG,
    PROCESSOR_RESULT_SIG,
    WORKER_WORK_SIG,
    MONITOR_CHECK_SIG,
    TIMEOUT_SIG,
    
    MAX_DEMO_SIG
};

/*==========================================================================*/
/* QXK Demo Events */
/*==========================================================================*/
typedef struct {
    QEvt super;
    uint32_t data;
} SensorDataEvt;

typedef struct {
    QEvt super;
    uint32_t result;
} ProcessorResultEvt;

typedef struct {
    QEvt super;
    uint32_t work_id;
} WorkerWorkEvt;

/*==========================================================================*/
/* Active Object Handles */
/*==========================================================================*/
extern QActive * const AO_Sensor;
extern QActive * const AO_Processor;
extern QActive * const AO_Worker;
extern QActive * const AO_Monitor;

/*==========================================================================*/
/* QXK Demo Functions */
/*==========================================================================*/
void QXKDemo_init(void);
int qxk_demo_start(void);

#endif /* QXK_DEMO_H_ */