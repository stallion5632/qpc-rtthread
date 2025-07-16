/*============================================================================
* Product: Advanced Dispatcher Demo for RT-Thread
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
/* Advanced Dispatcher Demo Signals */
/*==========================================================================*/
enum AdvancedDemoSignals {
    DEMO_SIG = Q_USER_SIG,
    HIGH_PRIO_SIG,
    NORMAL_PRIO_SIG,
    LOW_PRIO_SIG,
    MERGEABLE_SIG,
    CRITICAL_SIG,
    BULK_DATA_SIG,
    TIMEOUT_SIG,
    METRICS_REPORT_SIG,
    STRATEGY_SWITCH_SIG,
    MAX_SIG
};

/*==========================================================================*/
/* Advanced Demo Events */
/*==========================================================================*/
typedef struct {
    QEvt super;
    uint32_t data;
    uint32_t sequence;
} DataEvt;

typedef struct {
    QEvt super;
    uint32_t bulk_size;
    uint8_t *payload;
} BulkDataEvt;

typedef struct {
    QEvt super;
    uint32_t strategy_id;
} StrategyEvt;

/*==========================================================================*/
/* Active Object Handles */
/*==========================================================================*/
extern QActive * const AO_Producer;
extern QActive * const AO_Consumer;
extern QActive * const AO_Monitor;
extern QActive * const AO_Controller;

/*==========================================================================*/
/* Advanced Demo Functions */
/*==========================================================================*/
void AdvancedDemo_init(void);
int advanced_demo_start(void);
void advanced_demo_stop(void);

/* Metrics and control functions */
void demo_show_metrics(void);
void demo_switch_strategy(uint32_t strategy_id);
void demo_generate_load(uint32_t load_level);

#endif /* QACTIVE_DEMO_H_ */
