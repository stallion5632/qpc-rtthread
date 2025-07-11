/*============================================================================
* QP/C Real-Time Embedded Framework (RTEF)
* Advanced Dispatcher Demo for RT-Thread
*
* @file
* @brief Demonstration of advanced dispatcher features
*/
#include "qf_port.h"
#include "qf_opt_layer.h"
#include "qassert.h"
#include <rtthread.h>

/* Demo event signals */
enum DemoSignals {
    DEMO_SIG = Q_USER_SIG,
    HIGH_PRIO_SIG,
    NORMAL_PRIO_SIG,
    LOW_PRIO_SIG,
    MERGEABLE_SIG,
    CRITICAL_SIG,
    MAX_SIG
};

/* Demo Active Object */
typedef struct {
    QActive super;
    uint32_t eventCount;
    uint32_t highPrioCount;
    uint32_t normalPrioCount;
    uint32_t lowPrioCount;
    uint32_t mergeableCount;
    uint32_t criticalCount;
} DemoAO;

/* Demo AO instance */
static DemoAO l_demoAO;

/* Demo event pools */
static QEvt const *l_demoQueueSto[20];
static QEvt const *l_basicEvents[50];
static QEvt const *l_extendedEvents[30];

/* Demo AO state machine */
static QState DemoAO_initial(DemoAO * const me, void const * const par);
static QState DemoAO_active(DemoAO * const me, QEvt const * const e);

/*..........................................................................*/
static QState DemoAO_initial(DemoAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);
    
    me->eventCount = 0;
    me->highPrioCount = 0;
    me->normalPrioCount = 0;
    me->lowPrioCount = 0;
    me->mergeableCount = 0;
    me->criticalCount = 0;
    
    return Q_TRAN(&DemoAO_active);
}

/*..........................................................................*/
static QState DemoAO_active(DemoAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("[DemoAO] Entered active state\n");
            status = Q_HANDLED();
            break;
        }
        
        case HIGH_PRIO_SIG: {
            me->highPrioCount++;
            me->eventCount++;
            rt_kprintf("[DemoAO] High priority event #%lu\n", me->highPrioCount);
            status = Q_HANDLED();
            break;
        }
        
        case NORMAL_PRIO_SIG: {
            me->normalPrioCount++;
            me->eventCount++;
            rt_kprintf("[DemoAO] Normal priority event #%lu\n", me->normalPrioCount);
            status = Q_HANDLED();
            break;
        }
        
        case LOW_PRIO_SIG: {
            me->lowPrioCount++;
            me->eventCount++;
            rt_kprintf("[DemoAO] Low priority event #%lu\n", me->lowPrioCount);
            status = Q_HANDLED();
            break;
        }
        
        case MERGEABLE_SIG: {
            me->mergeableCount++;
            me->eventCount++;
            rt_kprintf("[DemoAO] Mergeable event #%lu\n", me->mergeableCount);
            status = Q_HANDLED();
            break;
        }
        
        case CRITICAL_SIG: {
            me->criticalCount++;
            me->eventCount++;
            rt_kprintf("[DemoAO] Critical event #%lu\n", me->criticalCount);
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
static void demo_event_generator(void *parameter) {
    Q_UNUSED_PAR(parameter);
    
    rt_kprintf("[Demo] Event generator thread started\n");
    
    /* Generate various types of events */
    for (int i = 0; i < 100; ++i) {
        rt_thread_mdelay(100);  /* 100ms delay */
        
        /* Generate high priority events */
        if (i % 10 == 0) {
            QEvtEx *evt = QF_newEvtEx(HIGH_PRIO_SIG, sizeof(QEvtEx), 200, QF_EVT_FLAG_CRITICAL);
            if (evt != (QEvtEx *)0) {
                QACTIVE_POST(&l_demoAO, (QEvt *)evt, 0);
            }
        }
        
        /* Generate mergeable events */
        if (i % 5 == 0) {
            QEvtEx *evt = QF_newEvtEx(MERGEABLE_SIG, sizeof(QEvtEx), 100, QF_EVT_FLAG_MERGEABLE);
            if (evt != (QEvtEx *)0) {
                QACTIVE_POST(&l_demoAO, (QEvt *)evt, 0);
            }
        }
        
        /* Generate normal priority events */
        if (i % 3 == 0) {
            QEvtEx *evt = QF_newEvtEx(NORMAL_PRIO_SIG, sizeof(QEvtEx), 100, 0);
            if (evt != (QEvtEx *)0) {
                QACTIVE_POST(&l_demoAO, (QEvt *)evt, 0);
            }
        }
        
        /* Generate low priority events */
        if (i % 7 == 0) {
            QEvtEx *evt = QF_newEvtEx(LOW_PRIO_SIG, sizeof(QEvtEx), 50, 0);
            if (evt != (QEvtEx *)0) {
                QACTIVE_POST(&l_demoAO, (QEvt *)evt, 0);
            }
        }
        
        /* Generate critical events */
        if (i % 15 == 0) {
            QEvtEx *evt = QF_newEvtEx(CRITICAL_SIG, sizeof(QEvtEx), 255, 
                                     QF_EVT_FLAG_CRITICAL | QF_EVT_FLAG_NO_DROP);
            if (evt != (QEvtEx *)0) {
                QACTIVE_POST(&l_demoAO, (QEvt *)evt, 0);
            }
        }
    }
    
    rt_kprintf("[Demo] Event generator completed\n");
}

/*..........................................................................*/
static void demo_metrics_reporter(void *parameter) {
    Q_UNUSED_PAR(parameter);
    
    rt_kprintf("[Demo] Metrics reporter thread started\n");
    
    for (int i = 0; i < 20; ++i) {
        rt_thread_mdelay(1000);  /* 1 second delay */
        
        QF_DispatcherMetrics const *metrics = QF_getDispatcherMetrics();
        
        rt_kprintf("[Metrics] Cycle: %d, Events: %lu, Merged: %lu, Dropped: %lu, Retried: %lu\n",
                  i, metrics->eventsProcessed, metrics->eventsMerged, 
                  metrics->eventsDropped, metrics->eventsRetried);
        
        rt_kprintf("[DemoAO] Total: %lu, High: %lu, Normal: %lu, Low: %lu, Mergeable: %lu, Critical: %lu\n",
                  l_demoAO.eventCount, l_demoAO.highPrioCount, l_demoAO.normalPrioCount,
                  l_demoAO.lowPrioCount, l_demoAO.mergeableCount, l_demoAO.criticalCount);
    }
    
    rt_kprintf("[Demo] Metrics reporter completed\n");
}

/*..........................................................................*/
static void demo_strategy_switcher(void *parameter) {
    Q_UNUSED_PAR(parameter);
    
    rt_kprintf("[Demo] Strategy switcher thread started\n");
    
    /* Start with default strategy */
    QF_setDispatcherStrategy(&QF_defaultStrategy);
    rt_kprintf("[Demo] Using default strategy\n");
    
    rt_thread_mdelay(5000);  /* 5 seconds */
    
    /* Switch to high performance strategy */
    QF_setDispatcherStrategy(&QF_highPerfStrategy);
    rt_kprintf("[Demo] Switched to high performance strategy\n");
    
    rt_thread_mdelay(5000);  /* 5 seconds */
    
    /* Switch back to default strategy */
    QF_setDispatcherStrategy(&QF_defaultStrategy);
    rt_kprintf("[Demo] Switched back to default strategy\n");
    
    rt_kprintf("[Demo] Strategy switcher completed\n");
}

/*..........................................................................*/
void advanced_dispatcher_demo(void) {
    rt_kprintf("\n==== Advanced Dispatcher Demo Starting ====\n");
    
    /* Initialize event pools */
    QF_poolInit(l_basicEvents, sizeof(l_basicEvents), sizeof(QEvt));
    QF_poolInit(l_extendedEvents, sizeof(l_extendedEvents), sizeof(QEvtEx));
    
    /* Initialize demo AO */
    QMActive_ctor(&l_demoAO.super, Q_STATE_CAST(&DemoAO_initial));
    
    /* Set demo AO name */
    QACTIVE_SET_ATTR(&l_demoAO, THREAD_NAME_ATTR, "DemoAO");
    
    /* Start demo AO */
    QACTIVE_START(&l_demoAO,
                  1U,                        /* priority */
                  l_demoQueueSto,           /* event queue storage */
                  Q_DIM(l_demoQueueSto),    /* queue length */
                  (void *)0,                /* stack storage (not used) */
                  0U,                       /* stack size */
                  (void *)0);               /* initialization parameter */
    
    /* Create and start demo threads */
    rt_thread_t gen_thread = rt_thread_create("demo_gen", demo_event_generator, 
                                             RT_NULL, 2048, 10, 10);
    if (gen_thread != RT_NULL) {
        rt_thread_startup(gen_thread);
    }
    
    rt_thread_t metrics_thread = rt_thread_create("demo_metrics", demo_metrics_reporter, 
                                                 RT_NULL, 2048, 15, 10);
    if (metrics_thread != RT_NULL) {
        rt_thread_startup(metrics_thread);
    }
    
    rt_thread_t strategy_thread = rt_thread_create("demo_strategy", demo_strategy_switcher, 
                                                  RT_NULL, 2048, 20, 10);
    if (strategy_thread != RT_NULL) {
        rt_thread_startup(strategy_thread);
    }
    
    rt_kprintf("Demo threads started. Use 'qf_metrics' and 'qf_aos' commands to monitor.\n");
    rt_kprintf("==============================================\n");
}

/* Export to finsh shell */
MSH_CMD_EXPORT_ALIAS(advanced_dispatcher_demo, demo_start, Start advanced dispatcher demo);