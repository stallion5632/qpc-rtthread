/*============================================================================
* QP/C RT-Thread Integration Example
* Demonstrates the optimized features including:
* 1. Heartbeat and watchdog integration
* 2. Basic event handling
============================================================================*/
#include "qf_port.h"
#include "qpc.h"
#include <rtthread.h>

Q_DEFINE_THIS_MODULE("example")

/* Application signals */
enum AppSignals {
    HEARTBEAT_SIG = Q_USER_SIG, /* Custom heartbeat signal */
    TIMEOUT_SIG,                /* Timeout signal */
    DATA_READY_SIG,            /* Data ready signal */
    MAX_PUB_SIG,               /* the last published signal */

    MAX_SIG                    /* the last signal */
};

/* Example Active Object structure */
typedef struct {
    QActive super;             /* inherit QActive */

    /* Private members */
    uint32_t heartbeat_count;
    uint32_t timeout_count;
    uint32_t data_count;
} ExampleAO;

/* Global objects */
static ExampleAO l_exampleAO;

/* Forward declaration for state handler */
static QState ExampleAO_active(ExampleAO * const me, QEvt const * const e);

/* Forward declarations */
static QState ExampleAO_initial(ExampleAO * const me, void const * const par);
static QState ExampleAO_active(ExampleAO * const me, QEvt const * const e);

/* Example event with data */
typedef struct {
    QEvt super;               /* inherit QEvt */
    uint32_t data;           /* event data */
    uint16_t sequence;       /* sequence number */
} DataEvt;

/* 全局常量心跳事件，常驻内存，不回收 */
const QEvt heartbeat_evt = QEVT_INITIALIZER(HEARTBEAT_SIG);
/*..........................................................................*/
static QState ExampleAO_initial(ExampleAO * const me, void const * const par) {
    (void)par; /* unused parameter */

    rt_kprintf("[Example] ExampleAO initial state\n");

    /* Initialize private members */
    me->heartbeat_count = 0U;
    me->timeout_count = 0U;
    me->data_count = 0U;

    return Q_TRAN(&ExampleAO_active);
}

/*..........................................................................*/
static QState ExampleAO_active(ExampleAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("[Example] Entering active state\n");


            /* 使用全局常量事件，且不回收 */
            QACTIVE_POST(&me->super, &heartbeat_evt, &me->super);

            status = Q_HANDLED();
            break;
        }

        case HEARTBEAT_SIG: {
            ++me->heartbeat_count;
            rt_kprintf("[Example] Heartbeat #%lu\n", me->heartbeat_count);

            /* Demonstrate RT-Thread memory pool usage */
            if ((me->heartbeat_count % 10) == 0) {
                DataEvt *data_evt = Q_NEW(DataEvt, DATA_READY_SIG);
                if (data_evt != (DataEvt *)0) {
                    data_evt->data = me->heartbeat_count;
                    data_evt->sequence = (uint16_t)(me->heartbeat_count / 10);
                    /* POST事件到自己 */
                    QACTIVE_POST(&me->super, &data_evt->super, &me->super);
                    /* QACTIVE_POST后立即置NULL，防止悬挂指针 */
                    data_evt = (DataEvt *)0;
                }
            }

            /* 继续调度心跳，使用全局常量事件 */
            QACTIVE_POST(&me->super, &heartbeat_evt, &me->super);

            status = Q_HANDLED();
            break;
        }

        case DATA_READY_SIG: {
            /* 正确类型转换，防止未定义行为 */
            DataEvt const *data_evt = (DataEvt const *)e;
            ++me->data_count;

            rt_kprintf("[Example] Data ready: data=%lu, seq=%u, total_events=%lu\n",
                       data_evt->data, data_evt->sequence, me->data_count);

            status = Q_HANDLED();
            break;
        }

        case TIMEOUT_SIG: {
            ++me->timeout_count;
            rt_kprintf("[Example] Timeout event #%lu\n", me->timeout_count);
            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG: {
            rt_kprintf("[Example] Exiting active state\n");
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
void ExampleAO_ctor(void) {
    ExampleAO *me = &l_exampleAO;
    QActive_ctor(&me->super, Q_STATE_CAST(&ExampleAO_initial));
}

/*..........................................................................*/
/* Test function to demonstrate the features */
int qf_example_test(void) {
    rt_kprintf("\n==== QP/C RT-Thread Integration Example ====\n");

#if QF_ENABLE_RT_MEMPOOL
    rt_kprintf("RT-Thread memory pool integration: ENABLED\n");
#else
    rt_kprintf("RT-Thread memory pool integration: DISABLED\n");
#endif

#if QF_ENABLE_HEARTBEAT
    rt_kprintf("Heartbeat and watchdog integration: ENABLED\n");
#else
    rt_kprintf("Heartbeat and watchdog integration: DISABLED\n");
#endif

    /* Initialize the framework */
    QF_init();

    /* Initialize the AO */
    ExampleAO_ctor();

    /* Create event queue storage */
    static QEvt const *exampleQueueSto[10];

    /* Create stack storage */
    static uint64_t exampleStack[64];  /* stack storage */

    /* Set AO attributes (name) */
    // QActive_setAttr(&l_exampleAO.super, THREAD_NAME_ATTR, "Example");

    /* Start the AO */
    QACTIVE_START(&l_exampleAO.super,
                  22U,                              /* QP priority */
                  exampleQueueSto,                 /* event queue storage */
                  Q_DIM(exampleQueueSto),         /* queue length */
                  exampleStack,                    /* stack storage */
                  sizeof(exampleStack),            /* stack size */
                  (void *)0);                      /* initialization param */

    rt_kprintf("ExampleAO started successfully\n");
    rt_kprintf("============================================\n");

    return 0; /* success */
}

/* Export to finsh shell */
MSH_CMD_EXPORT_ALIAS(qf_example_test, qf_test, Run QP/C integration example);
