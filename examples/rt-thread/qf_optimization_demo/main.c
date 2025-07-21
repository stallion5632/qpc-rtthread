/*============================================================================
* QP/C RT-Thread Optimization Features Demo
*
* This demo demonstrates:
* 1. Heartbeat and watchdog integration
* 2. RT-Thread memory pool integration
* 3. Optimization layer features
* 4. Diagnostic and monitoring capabilities
============================================================================*/
#include "qpc.h"
#include <rtthread.h>
#include <finsh.h>

Q_DEFINE_THIS_MODULE("opt_demo")

/* Application signals */
enum AppSignals {
    HEARTBEAT_SIG = Q_USER_SIG,    /* Heartbeat signal */
    DATA_SIG,                      /* Data processing signal */
    TIMEOUT_SIG,                   /* Timeout signal */
    STRESS_TEST_SIG,               /* Memory stress test signal */
    MONITOR_SIG,                   /* System monitoring signal */
    SHUTDOWN_SIG,                  /* Shutdown signal */
    MAX_PUB_SIG,                   /* Last published signal */

    MAX_SIG                        /* Last signal */
};

/*==========================================================================*/
/* Data Producer Active Object - Tests RT-Thread memory pool */
/*==========================================================================*/
typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    uint32_t data_count;
    uint32_t alloc_failures;
} DataProducerAO;

/* Data event structure - tests different sizes for memory pool selection */
typedef struct {
    QEvt super;
    uint32_t sequence;
    uint32_t timestamp;
    uint8_t data[100];  /* Medium-size event to test medium pool */
} DataEvt;

typedef struct {
    QEvt super;
    uint32_t sequence;
    uint16_t value;     /* Small event to test small pool */
} SmallDataEvt;

typedef struct {
    QEvt super;
    uint32_t sequence;
    uint32_t timestamp;
    uint8_t large_data[200];  /* Large event to test large pool */
} LargeDataEvt;

static DataProducerAO l_dataProducerAO;

/* State function declarations */
static QState DataProducerAO_initial(DataProducerAO * const me, void const * const par);
static QState DataProducerAO_active(DataProducerAO * const me, QEvt const * const e);
static QState DataProducerAO_stress_test(DataProducerAO * const me, QEvt const * const e);

/*..........................................................................*/
static QState DataProducerAO_initial(DataProducerAO * const me, void const * const par) {
    (void)par;

    me->data_count = 0U;
    me->alloc_failures = 0U;

    QTimeEvt_ctorX(&me->timeEvt, &me->super, DATA_SIG, 0U);

    rt_kprintf("[DataProducer] Initialized\n");
    return Q_TRAN(&DataProducerAO_active);
}

/*..........................................................................*/
static QState DataProducerAO_active(DataProducerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("[DataProducer] Entering active state\n");
            QTimeEvt_armX(&me->timeEvt, 200U, 200U); /* 2 seconds interval */
            status = Q_HANDLED();
            break;
        }

        case DATA_SIG: {
            ++me->data_count;

            /* Test different memory pool sizes */
            if ((me->data_count % 3) == 0) {
                /* Test small pool */
                SmallDataEvt *evt = Q_NEW(SmallDataEvt, DATA_SIG);
                if (evt != (SmallDataEvt *)0) {
                    evt->sequence = me->data_count;
                    evt->value = (uint16_t)(me->data_count & 0xFFFF);
                    QACTIVE_PUBLISH(&evt->super, &me->super);
                    rt_kprintf("[DataProducer] Small event #%lu published\n", me->data_count);
                } else {
                    ++me->alloc_failures;
                    rt_kprintf("[DataProducer] Small event allocation failed!\n");
                }
            }
            else if ((me->data_count % 3) == 1) {
                /* Test medium pool */
                DataEvt *evt = Q_NEW(DataEvt, DATA_SIG);
                if (evt != (DataEvt *)0) {
                    evt->sequence = me->data_count;
                    evt->timestamp = rt_tick_get();
                    /* Fill data array */
                    for (int i = 0; i < 100; ++i) {
                        evt->data[i] = (uint8_t)(i + me->data_count);
                    }
                    QACTIVE_PUBLISH(&evt->super, &me->super);
                    rt_kprintf("[DataProducer] Medium event #%lu published\n", me->data_count);
                } else {
                    ++me->alloc_failures;
                    rt_kprintf("[DataProducer] Medium event allocation failed!\n");
                }
            }
            else {
                /* Test large pool */
                LargeDataEvt *evt = Q_NEW(LargeDataEvt, DATA_SIG);
                if (evt != (LargeDataEvt *)0) {
                    evt->sequence = me->data_count;
                    evt->timestamp = rt_tick_get();
                    /* Fill large data array */
                    for (int i = 0; i < 200; ++i) {
                        evt->large_data[i] = (uint8_t)(i ^ me->data_count);
                    }
                    QACTIVE_PUBLISH(&evt->super, &me->super);
                    rt_kprintf("[DataProducer] Large event #%lu published\n", me->data_count);
                } else {
                    ++me->alloc_failures;
                    rt_kprintf("[DataProducer] Large event allocation failed!\n");
                }
            }

            status = Q_HANDLED();
            break;
        }

        case STRESS_TEST_SIG: {
            rt_kprintf("[DataProducer] Starting memory stress test\n");
            status = Q_TRAN(&DataProducerAO_stress_test);
            break;
        }

        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->timeEvt);
            rt_kprintf("[DataProducer] Exiting active state\n");
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

/*..........................................................................*/
static QState DataProducerAO_stress_test(DataProducerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("[DataProducer] Stress test - rapid allocation/deallocation\n");
            QTimeEvt_armX(&me->timeEvt, 5U, 5U); /* 50ms rapid fire */
            status = Q_HANDLED();
            break;
        }

        case DATA_SIG: {
            /* Rapid allocation to stress test memory pools */
            for (int i = 0; i < 5; ++i) {
                DataEvt *evt = Q_NEW(DataEvt, DATA_SIG);
                if (evt != (DataEvt *)0) {
                    evt->sequence = me->data_count++;
                    evt->timestamp = rt_tick_get();
                    QACTIVE_PUBLISH(&evt->super, &me->super);
                } else {
                    ++me->alloc_failures;
                }
            }

            /* After 50 rapid allocations, return to normal mode */
            if (me->data_count > 50) {
                rt_kprintf("[DataProducer] Stress test completed, failures: %lu\n",
                          me->alloc_failures);
                status = Q_TRAN(&DataProducerAO_active);
            } else {
                status = Q_HANDLED();
            }
            break;
        }

        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->timeEvt);
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
/* Data Consumer Active Object - Tests heartbeat and processing */
/*==========================================================================*/
typedef struct {
    QActive super;
    QTimeEvt heartbeatEvt;
    uint32_t processed_count;
    uint32_t heartbeat_count;
    uint32_t last_data_time;
} DataConsumerAO;

static DataConsumerAO l_dataConsumerAO;

static QState DataConsumerAO_initial(DataConsumerAO * const me, void const * const par);
static QState DataConsumerAO_idle(DataConsumerAO * const me, QEvt const * const e);
static QState DataConsumerAO_processing(DataConsumerAO * const me, QEvt const * const e);

/*..........................................................................*/
static QState DataConsumerAO_initial(DataConsumerAO * const me, void const * const par) {
    (void)par;

    me->processed_count = 0U;
    me->heartbeat_count = 0U;
    me->last_data_time = rt_tick_get();

    QTimeEvt_ctorX(&me->heartbeatEvt, &me->super, HEARTBEAT_SIG, 0U);

    /* Subscribe to data events */
    QActive_subscribe(&me->super, DATA_SIG);

    rt_kprintf("[DataConsumer] Initialized and subscribed to DATA_SIG\n");
    return Q_TRAN(&DataConsumerAO_idle);
}

/*..........................................................................*/
static QState DataConsumerAO_idle(DataConsumerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("[DataConsumer] Entering idle state\n");
            QTimeEvt_armX(&me->heartbeatEvt, 500U, 500U); /* 5 second heartbeat */
            status = Q_HANDLED();
            break;
        }

        case DATA_SIG: {
            me->last_data_time = rt_tick_get();
            rt_kprintf("[DataConsumer] Received data, processing...\n");
            status = Q_TRAN(&DataConsumerAO_processing);
            break;
        }

        case HEARTBEAT_SIG: {
            ++me->heartbeat_count;
            uint32_t current_time = rt_tick_get();
            uint32_t idle_time = current_time - me->last_data_time;

            rt_kprintf("[DataConsumer] Heartbeat #%lu, idle for %lu ticks, processed: %lu\n",
                      me->heartbeat_count, idle_time, me->processed_count);

            /* Test optimization layer API */
            if ((me->heartbeat_count % 10) == 0) {
                rt_kprintf("[DataConsumer] Requesting dispatcher metrics...\n");
                /* This would trigger metrics update in a real system */
            }

            status = Q_HANDLED();
            break;
        }

        case TIMEOUT_SIG: {
            rt_kprintf("[DataConsumer] Timeout in idle state\n");
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->heartbeatEvt);
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

/*..........................................................................*/
static QState DataConsumerAO_processing(DataConsumerAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            ++me->processed_count;
            rt_kprintf("[DataConsumer] Processing data item #%lu\n", me->processed_count);

            /* Simulate processing time */
            rt_thread_mdelay(10);

            /* Post completion event */
            static QEvt const completion_evt = QEVT_INITIALIZER(TIMEOUT_SIG);
            QACTIVE_POST(&me->super, &completion_evt, &me->super);

            status = Q_HANDLED();
            break;
        }

        case TIMEOUT_SIG: {
            rt_kprintf("[DataConsumer] Processing completed\n");
            status = Q_TRAN(&DataConsumerAO_idle);
            break;
        }

        case DATA_SIG: {
            rt_kprintf("[DataConsumer] New data arrived while processing\n");
            /* Stay in processing state but update time */
            me->last_data_time = rt_tick_get();
            status = Q_HANDLED();
            break;
        }

        default: {
            status = Q_SUPER(&DataConsumerAO_idle);
            break;
        }
    }

    return status;
}

/*==========================================================================*/
/* System Monitor Active Object - Tests diagnostic features */
/*==========================================================================*/
typedef struct {
    QActive super;
    QTimeEvt monitorEvt;
    uint32_t monitor_cycles;
} SystemMonitorAO;

static SystemMonitorAO l_systemMonitorAO;

static QState SystemMonitorAO_initial(SystemMonitorAO * const me, void const * const par);
static QState SystemMonitorAO_monitoring(SystemMonitorAO * const me, QEvt const * const e);

/*..........................................................................*/
static QState SystemMonitorAO_initial(SystemMonitorAO * const me, void const * const par) {
    (void)par;

    me->monitor_cycles = 0U;
    QTimeEvt_ctorX(&me->monitorEvt, &me->super, MONITOR_SIG, 0U);

    rt_kprintf("[SystemMonitor] Initialized\n");
    return Q_TRAN(&SystemMonitorAO_monitoring);
}

/*..........................................................................*/
static QState SystemMonitorAO_monitoring(SystemMonitorAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("[SystemMonitor] Starting system monitoring\n");
            QTimeEvt_armX(&me->monitorEvt, 1000U, 1000U); /* 10 second intervals */
            status = Q_HANDLED();
            break;
        }

        case MONITOR_SIG: {
            ++me->monitor_cycles;
            rt_kprintf("\n[SystemMonitor] === Cycle #%lu ===\n", me->monitor_cycles);

            /* Test diagnostic APIs */
#if QF_ENABLE_RT_MEMPOOL
            rt_kprintf("[SystemMonitor] Memory pool status:\n");
            extern void QF_poolPrintStats_RT(void);
            QF_poolPrintStats_RT();
#endif

            /* Test optimization layer APIs if available */
#ifdef QF_ENABLE_OPT_LAYER
            rt_kprintf("[SystemMonitor] Optimization layer metrics:\n");
            QF_DispatcherMetrics const *metrics = QF_getDispatcherMetrics();
            rt_kprintf("  Events processed: %lu\n", metrics->eventsProcessed);
            rt_kprintf("  Events dropped: %lu\n", metrics->eventsDropped);
#endif

            rt_kprintf("[SystemMonitor] === End Cycle ===\n\n");
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->monitorEvt);
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
/* Demo Control Functions */
/*==========================================================================*/

/* QF publish-subscribe table */
static QSubscrList l_subscrSto[MAX_PUB_SIG];

/* Event pools for different sizes */
static QF_MPOOL_EL(SmallDataEvt) l_smlPoolSto[20];
static QF_MPOOL_EL(DataEvt) l_medPoolSto[10];
static QF_MPOOL_EL(LargeDataEvt) l_lrgPoolSto[5];

/*..........................................................................*/
static void demo_init(void) {
    rt_kprintf("\n==== QP/C RT-Thread Optimization Demo ====\n");

    /* Initialize QF */
    QF_init();
    QF_psInit(l_subscrSto, Q_DIM(l_subscrSto));

#if !QF_ENABLE_RT_MEMPOOL
    /* Initialize event pools (only if not using RT-Thread pools) */
    QF_poolInit(l_smlPoolSto, sizeof(l_smlPoolSto), sizeof(l_smlPoolSto[0]));
    QF_poolInit(l_medPoolSto, sizeof(l_medPoolSto), sizeof(l_medPoolSto[0]));
    QF_poolInit(l_lrgPoolSto, sizeof(l_lrgPoolSto), sizeof(l_lrgPoolSto[0]));
#endif

    rt_kprintf("Framework initialized\n");
}

/*..........................................................................*/
static void demo_start(void) {
    /* Event queue storage and stack storage */
    static QEvt const *dataProducerQueueSto[10];
    static uint64_t dataProducerStack[64];

    static QEvt const *dataConsumerQueueSto[10];
    static uint64_t dataConsumerStack[64];

    static QEvt const *systemMonitorQueueSto[5];
    static uint64_t systemMonitorStack[64];

    /* Initialize Active Objects */
    QActive_ctor(&l_dataProducerAO.super, Q_STATE_CAST(&DataProducerAO_initial));
    QActive_ctor(&l_dataConsumerAO.super, Q_STATE_CAST(&DataConsumerAO_initial));
    QActive_ctor(&l_systemMonitorAO.super, Q_STATE_CAST(&SystemMonitorAO_initial));

    /* Set AO names */
    QActive_setAttr(&l_dataProducerAO.super, THREAD_NAME_ATTR, "DataProd");
    QActive_setAttr(&l_dataConsumerAO.super, THREAD_NAME_ATTR, "DataCons");
    QActive_setAttr(&l_systemMonitorAO.super, THREAD_NAME_ATTR, "SysMon");

    /* Start the Active Objects */
    QACTIVE_START(&l_dataProducerAO.super,
                  1U,                            /* QP priority */
                  dataProducerQueueSto,         /* event queue storage */
                  Q_DIM(dataProducerQueueSto),  /* queue length */
                  dataProducerStack,            /* stack storage */
                  sizeof(dataProducerStack),    /* stack size */
                  (void *)0);                   /* initialization param */

    QACTIVE_START(&l_dataConsumerAO.super,
                  2U,                           /* QP priority */
                  dataConsumerQueueSto,        /* event queue storage */
                  Q_DIM(dataConsumerQueueSto), /* queue length */
                  dataConsumerStack,           /* stack storage */
                  sizeof(dataConsumerStack),   /* stack size */
                  (void *)0);                  /* initialization param */

    QACTIVE_START(&l_systemMonitorAO.super,
                  3U,                            /* QP priority */
                  systemMonitorQueueSto,        /* event queue storage */
                  Q_DIM(systemMonitorQueueSto), /* queue length */
                  systemMonitorStack,           /* stack storage */
                  sizeof(systemMonitorStack),   /* stack size */
                  (void *)0);                   /* initialization param */

    rt_kprintf("All Active Objects started\n");
    rt_kprintf("===========================================\n\n");
}

/*..........................................................................*/
static void demo_stress_test(void) {
    rt_kprintf("\n[DEMO] Starting stress test...\n");
    static QEvt const stress_evt = QEVT_INITIALIZER(STRESS_TEST_SIG);
    QACTIVE_POST(&l_dataProducerAO.super, &stress_evt, (void *)0);
}

/*..........................................................................*/
static void demo_stop(void) {
    rt_kprintf("\n[DEMO] Stopping demo...\n");
    static QEvt const shutdown_evt = QEVT_INITIALIZER(SHUTDOWN_SIG);
    QACTIVE_PUBLISH(&shutdown_evt, (void *)0);
    rt_kprintf("Shutdown signal published\n");
}

/*==========================================================================*/
/* Shell Commands */
/*==========================================================================*/

int qf_opt_demo_start(void) {
    demo_init();
    demo_start();
    return 0;
}

int qf_opt_demo_stress(void) {
    demo_stress_test();
    return 0;
}

int qf_opt_demo_stop(void) {
    demo_stop();
    return 0;
}

/* Export shell commands */
MSH_CMD_EXPORT_ALIAS(qf_opt_demo_start, qf_demo_start, Start QP/C optimization demo);
MSH_CMD_EXPORT_ALIAS(qf_opt_demo_stress, qf_demo_stress, Run memory stress test);
MSH_CMD_EXPORT_ALIAS(qf_opt_demo_stop, qf_demo_stop, Stop demo);
