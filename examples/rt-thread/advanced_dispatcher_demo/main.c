/*============================================================================
* Product: Advanced Dispatcher Demo for RT-Thread - Main Application
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

#ifdef QPC_USING_ADVANCED_DISPATCHER_DEMO
#ifdef RT_USING_FINSH

#include <finsh.h>
#include <stdio.h>

Q_DEFINE_THIS_FILE

/* QF publish-subscribe table */
#define MAX_PUB_SIG     32U
static QSubscrList subscrSto[MAX_PUB_SIG];

/*==========================================================================*/
/* Producer Active Object */
/*==========================================================================*/
typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    uint32_t sequence;
    uint32_t load_level;
} ProducerAO;

static ProducerAO l_producerAO;
QActive * const AO_Producer = &l_producerAO.super;

/* Producer state functions */
static QState ProducerAO_initial(ProducerAO * const me, QEvt const * const e);
static QState ProducerAO_producing(ProducerAO * const me, QEvt const * const e);

/*==========================================================================*/
/* Consumer Active Object */
/*==========================================================================*/
typedef struct {
    QActive super;
    uint32_t highPrioCount;
    uint32_t normalPrioCount;
    uint32_t lowPrioCount;
    uint32_t mergeableCount;
    uint32_t criticalCount;
    uint32_t totalProcessed;
} ConsumerAO;

static ConsumerAO l_consumerAO;
QActive * const AO_Consumer = &l_consumerAO.super;

/* Consumer state functions */
static QState ConsumerAO_initial(ConsumerAO * const me, QEvt const * const e);
static QState ConsumerAO_consuming(ConsumerAO * const me, QEvt const * const e);

/*==========================================================================*/
/* Monitor Active Object */
/*==========================================================================*/
typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    uint32_t report_count;
} MonitorAO;

static MonitorAO l_monitorAO;
QActive * const AO_Monitor = &l_monitorAO.super;

/* Monitor state functions */
static QState MonitorAO_initial(MonitorAO * const me, QEvt const * const e);
static QState MonitorAO_monitoring(MonitorAO * const me, QEvt const * const e);

/*==========================================================================*/
/* Controller Active Object */
/*==========================================================================*/
typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    uint32_t current_strategy;
    bool auto_switch;
} ControllerAO;

static ControllerAO l_controllerAO;
QActive * const AO_Controller = &l_controllerAO.super;

/* Controller state functions */
static QState ControllerAO_initial(ControllerAO * const me, QEvt const * const e);
static QState ControllerAO_controlling(ControllerAO * const me, QEvt const * const e);

/*==========================================================================*/
/* Producer Active Object Implementation */
/*==========================================================================*/
static void ProducerAO_ctor(void) {
    ProducerAO *me = &l_producerAO;
    QActive_ctor(&me->super, Q_STATE_CAST(&ProducerAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, TIMEOUT_SIG, 0U);
    me->sequence = 0;
    me->load_level = 1;
}

static QState ProducerAO_initial(ProducerAO * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&ProducerAO_producing);
}

static QState ProducerAO_producing(ProducerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("[Producer] Starting event production\n");
            QTimeEvt_armX(&me->timeEvt, 50, 50); /* 500ms intervals */
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->timeEvt);
            status = Q_HANDLED();
            break;
        }

        case TIMEOUT_SIG: {
            me->sequence++;

            /* Generate events based on load level and sequence */
            uint32_t event_type = me->sequence % (5 * me->load_level);

            if (event_type == 0) {
                /* Critical event - highest priority, no drop */
                DataEvt *evt = Q_NEW(DataEvt, CRITICAL_SIG);
                if (evt != (DataEvt *)0) {
                    evt->data = me->sequence;
                    evt->sequence = me->sequence;
                    QF_PUBLISH((QEvt *)evt, me);
                }
            }
            else if (event_type == 1) {
                /* High priority event */
                DataEvt *evt = Q_NEW(DataEvt, HIGH_PRIO_SIG);
                if (evt != (DataEvt *)0) {
                    evt->data = me->sequence * 10;
                    evt->sequence = me->sequence;
                    QF_PUBLISH((QEvt *)evt, me);
                }
            }
            else if (event_type == 2) {
                /* Mergeable event */
                DataEvt *evt = Q_NEW(DataEvt, MERGEABLE_SIG);
                if (evt != (DataEvt *)0) {
                    evt->data = me->sequence * 100;
                    evt->sequence = me->sequence;
                    QF_PUBLISH((QEvt *)evt, me);
                }
            }
            else if (event_type == 3) {
                /* Normal priority event */
                DataEvt *evt = Q_NEW(DataEvt, NORMAL_PRIO_SIG);
                if (evt != (DataEvt *)0) {
                    evt->data = me->sequence * 1000;
                    evt->sequence = me->sequence;
                    QF_PUBLISH((QEvt *)evt, me);
                }
            }
            else {
                /* Low priority event */
                DataEvt *evt = Q_NEW(DataEvt, LOW_PRIO_SIG);
                if (evt != (DataEvt *)0) {
                    evt->data = me->sequence * 10000;
                    evt->sequence = me->sequence;
                    QF_PUBLISH((QEvt *)evt, me);
                }
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

/*==========================================================================*/
/* Consumer Active Object Implementation */
/*==========================================================================*/
static void ConsumerAO_ctor(void) {
    ConsumerAO *me = &l_consumerAO;
    QActive_ctor(&me->super, Q_STATE_CAST(&ConsumerAO_initial));
    me->highPrioCount = 0;
    me->normalPrioCount = 0;
    me->lowPrioCount = 0;
    me->mergeableCount = 0;
    me->criticalCount = 0;
    me->totalProcessed = 0;
}

static QState ConsumerAO_initial(ConsumerAO * const me, QEvt const * const e) {
    (void)e;

    /* Subscribe to all event types */
    QActive_subscribe(&me->super, HIGH_PRIO_SIG);
    QActive_subscribe(&me->super, NORMAL_PRIO_SIG);
    QActive_subscribe(&me->super, LOW_PRIO_SIG);
    QActive_subscribe(&me->super, MERGEABLE_SIG);
    QActive_subscribe(&me->super, CRITICAL_SIG);

    return Q_TRAN(&ConsumerAO_consuming);
}

static QState ConsumerAO_consuming(ConsumerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("[Consumer] Ready to consume events\n");
            status = Q_HANDLED();
            break;
        }

        case HIGH_PRIO_SIG: {
            DataEvt const *dataEvt = (DataEvt const *)e;
            me->highPrioCount++;
            me->totalProcessed++;
            rt_kprintf("[Consumer] HIGH_PRIO #%lu: data=%lu, seq=%lu\n",
                      me->highPrioCount, dataEvt->data, dataEvt->sequence);
            status = Q_HANDLED();
            break;
        }

        case NORMAL_PRIO_SIG: {
            DataEvt const *dataEvt = (DataEvt const *)e;
            me->normalPrioCount++;
            me->totalProcessed++;
            rt_kprintf("[Consumer] NORMAL_PRIO #%lu: data=%lu, seq=%lu\n",
                      me->normalPrioCount, dataEvt->data, dataEvt->sequence);
            status = Q_HANDLED();
            break;
        }

        case LOW_PRIO_SIG: {
            DataEvt const *dataEvt = (DataEvt const *)e;
            me->lowPrioCount++;
            me->totalProcessed++;
            rt_kprintf("[Consumer] LOW_PRIO #%lu: data=%lu, seq=%lu\n",
                      me->lowPrioCount, dataEvt->data, dataEvt->sequence);
            status = Q_HANDLED();
            break;
        }

        case MERGEABLE_SIG: {
            DataEvt const *dataEvt = (DataEvt const *)e;
            me->mergeableCount++;
            me->totalProcessed++;
            rt_kprintf("[Consumer] MERGEABLE #%lu: data=%lu, seq=%lu\n",
                      me->mergeableCount, dataEvt->data, dataEvt->sequence);
            status = Q_HANDLED();
            break;
        }

        case CRITICAL_SIG: {
            DataEvt const *dataEvt = (DataEvt const *)e;
            me->criticalCount++;
            me->totalProcessed++;
            rt_kprintf("[Consumer] CRITICAL #%lu: data=%lu, seq=%lu\n",
                      me->criticalCount, dataEvt->data, dataEvt->sequence);
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
    QTimeEvt_ctorX(&me->timeEvt, &me->super, METRICS_REPORT_SIG, 0U);
    me->report_count = 0;
}

static QState MonitorAO_initial(MonitorAO * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&MonitorAO_monitoring);
}

static QState MonitorAO_monitoring(MonitorAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("[Monitor] Starting system monitoring\n");
            QTimeEvt_armX(&me->timeEvt, 1000, 1000); /* 10 second intervals */
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->timeEvt);
            status = Q_HANDLED();
            break;
        }

        case METRICS_REPORT_SIG: {
            me->report_count++;

            rt_kprintf("\n=== Dispatcher Metrics Report #%lu ===\n", me->report_count);

            /* Display consumer statistics */
            rt_kprintf("[Consumer Stats] Total: %lu, High: %lu, Normal: %lu, Low: %lu, Mergeable: %lu, Critical: %lu\n",
                      l_consumerAO.totalProcessed, l_consumerAO.highPrioCount,
                      l_consumerAO.normalPrioCount, l_consumerAO.lowPrioCount,
                      l_consumerAO.mergeableCount, l_consumerAO.criticalCount);

            /* Display system metrics */
            rt_kprintf("[System] Producer sequence: %lu, Load level: %lu\n",
                      l_producerAO.sequence, l_producerAO.load_level);

            rt_kprintf("[Controller] Current strategy: %lu, Auto-switch: %s\n",
                      l_controllerAO.current_strategy,
                      l_controllerAO.auto_switch ? "ON" : "OFF");

            rt_kprintf("=====================================\n\n");

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
/* Controller Active Object Implementation */
/*==========================================================================*/
static void ControllerAO_ctor(void) {
    ControllerAO *me = &l_controllerAO;
    QActive_ctor(&me->super, Q_STATE_CAST(&ControllerAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, STRATEGY_SWITCH_SIG, 0U);
    me->current_strategy = 0;
    me->auto_switch = true;
}

static QState ControllerAO_initial(ControllerAO * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&ControllerAO_controlling);
}

static QState ControllerAO_controlling(ControllerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("[Controller] Starting automatic strategy control\n");
            if (me->auto_switch) {
                QTimeEvt_armX(&me->timeEvt, 1500, 1500); /* 15 second intervals */
            }
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->timeEvt);
            status = Q_HANDLED();
            break;
        }

        case STRATEGY_SWITCH_SIG: {
            if (me->auto_switch) {
                me->current_strategy = (me->current_strategy + 1) % 3;

                switch (me->current_strategy) {
                    case 0:
                        rt_kprintf("[Controller] Switching to DEFAULT strategy\n");
                        /* Switch to default strategy */
                        break;
                    case 1:
                        rt_kprintf("[Controller] Switching to HIGH_PERFORMANCE strategy\n");
                        /* Switch to high performance strategy */
                        break;
                    case 2:
                        rt_kprintf("[Controller] Switching to LOW_LATENCY strategy\n");
                        /* Switch to low latency strategy */
                        break;
                }
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

/*==========================================================================*/
/* Global Demo Functions */
/*==========================================================================*/
void AdvancedDemo_init(void) {
    static uint8_t isInitialized = 0;
    if (isInitialized != 0) {
        rt_kprintf("Advanced Demo: Already initialized, skipping...\n");
        return;
    }
    isInitialized = 1;

    rt_kprintf("=== Advanced Dispatcher Demo Initialize ===\n");

    /* Initialize QF */
    QF_init();

    /* Initialize publish-subscribe */
    QF_psInit(subscrSto, Q_DIM(subscrSto));

    /* Initialize a single dynamic event pool (max event size) */
    ALIGN(RT_ALIGN_SIZE) static QF_MPOOL_EL(BulkDataEvt) dynPoolSto[50];
    QF_poolInit(dynPoolSto, sizeof(dynPoolSto), sizeof(dynPoolSto[0]));

    /* Construct all Active Objects */
    ProducerAO_ctor();
    ConsumerAO_ctor();
    MonitorAO_ctor();
    ControllerAO_ctor();

    rt_kprintf("Advanced Demo: Initialization completed\n");
}

/*==========================================================================*/
/* Event queue storage and stacks for Active Objects */
/*==========================================================================*/
ALIGN(RT_ALIGN_SIZE) static QEvt const *producerQueue[20];
ALIGN(RT_ALIGN_SIZE) static QEvt const *consumerQueue[30];
ALIGN(RT_ALIGN_SIZE) static QEvt const *monitorQueue[15];
ALIGN(RT_ALIGN_SIZE) static QEvt const *controllerQueue[10];

// Increased stack sizes to prevent RT-Thread stack overflow
ALIGN(RT_ALIGN_SIZE) static uint64_t producerStack[1024/sizeof(uint64_t)];  // 1KB stack
ALIGN(RT_ALIGN_SIZE) static uint64_t consumerStack[1024/sizeof(uint64_t)];  // 1KB stack
ALIGN(RT_ALIGN_SIZE) static uint64_t monitorStack[1024/sizeof(uint64_t)];   // 1KB stack
ALIGN(RT_ALIGN_SIZE) static uint64_t controllerStack[1024/sizeof(uint64_t)];// 1KB stack

/*==========================================================================*/
/* Demo Start Function */
/*==========================================================================*/
int advanced_demo_start(void) {
    static uint8_t isStarted = 0;
    if (isStarted != 0) {
        rt_kprintf("Advanced Demo: Already started, skipping...\n");
        return 0;
    }
    isStarted = 1;

    rt_kprintf("\n==== Advanced Dispatcher Demo Starting ====\n");

    /* Start active objects with appropriate priorities */
    QACTIVE_START(AO_Producer,
                  1U, /* Highest priority */
                  producerQueue, Q_DIM(producerQueue),
                  producerStack, sizeof(producerStack),
                  (void *)0);
    rt_kprintf("Advanced Demo: Producer AO started (Priority 1)\n");

    QACTIVE_START(AO_Consumer,
                  2U, /* High priority */
                  consumerQueue, Q_DIM(consumerQueue),
                  consumerStack, sizeof(consumerStack),
                  (void *)0);
    rt_kprintf("Advanced Demo: Consumer AO started (Priority 2)\n");

    QACTIVE_START(AO_Monitor,
                  3U, /* Medium priority */
                  monitorQueue, Q_DIM(monitorQueue),
                  monitorStack, sizeof(monitorStack),
                  (void *)0);
    rt_kprintf("Advanced Demo: Monitor AO started (Priority 3)\n");

    QACTIVE_START(AO_Controller,
                  4U, /* Lowest priority */
                  controllerQueue, Q_DIM(controllerQueue),
                  controllerStack, sizeof(controllerStack),
                  (void *)0);
    rt_kprintf("Advanced Demo: Controller AO started (Priority 4)\n");

    rt_kprintf("Advanced Demo: All AOs started - Demonstrating advanced dispatcher features\n");
    rt_kprintf("============================================\n");

    return QF_run(); /* Run the QF application */
}

/*==========================================================================*/
/* Demo Control Functions */
/*==========================================================================*/
void advanced_demo_stop(void) {
    rt_kprintf("[Demo] Stopping all active objects...\n");
    /* Note: QF_stop() would be called here in a full implementation */
}

void demo_show_metrics(void) {
    rt_kprintf("\n=== Current Dispatcher Metrics ===\n");
    rt_kprintf("[Consumer] Total: %lu, High: %lu, Normal: %lu, Low: %lu, Mergeable: %lu, Critical: %lu\n",
              l_consumerAO.totalProcessed, l_consumerAO.highPrioCount,
              l_consumerAO.normalPrioCount, l_consumerAO.lowPrioCount,
              l_consumerAO.mergeableCount, l_consumerAO.criticalCount);
    rt_kprintf("[Producer] Sequence: %lu, Load: %lu\n",
              l_producerAO.sequence, l_producerAO.load_level);
    rt_kprintf("================================\n");
}

void demo_switch_strategy(uint32_t strategy_id) {
    StrategyEvt *evt = Q_NEW(StrategyEvt, STRATEGY_SWITCH_SIG);
    if (evt != (StrategyEvt *)0) {
        evt->strategy_id = strategy_id;
        QACTIVE_POST(AO_Controller, (QEvt *)evt, 0);
        rt_kprintf("[Demo] Switching to strategy %lu\n", strategy_id);
    }
}

void demo_generate_load(uint32_t load_level) {
    l_producerAO.load_level = load_level;
    rt_kprintf("[Demo] Load level set to %lu\n", load_level);
}

/*==========================================================================*/
/* RT-Thread MSH command exports */
/*==========================================================================*/
MSH_CMD_EXPORT_ALIAS(advanced_demo_start, adv_demo_start, Start advanced dispatcher demo);
MSH_CMD_EXPORT_ALIAS(advanced_demo_stop, adv_demo_stop, Stop advanced dispatcher demo);
MSH_CMD_EXPORT_ALIAS(demo_show_metrics, adv_metrics, Show dispatcher metrics);
MSH_CMD_EXPORT_ALIAS(demo_switch_strategy, adv_strategy, Switch dispatcher strategy);
MSH_CMD_EXPORT_ALIAS(demo_generate_load, adv_load, Set event generation load);

/*==========================================================================*/
/* RT-Thread application auto-initialization */
/*==========================================================================*/
static int advanced_demo_init(void) {
    rt_kprintf("=== Advanced Dispatcher Demo Auto-Initialize ===\n");
    AdvancedDemo_init();
    return advanced_demo_start();
}
INIT_APP_EXPORT(advanced_demo_init);

/*==========================================================================*/
/* Main function entry */
/*==========================================================================*/
int main(void) {
    AdvancedDemo_init();
    rt_kprintf("[System] Starting Advanced Dispatcher Demo\n");

    int ret = advanced_demo_start();
    rt_kprintf("[System] Advanced Demo startup completed\n");
    return ret;
}

#endif /* RT_USING_FINSH */
#endif /* QPC_USING_ADVANCED_DISPATCHER_DEMO */
