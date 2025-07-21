/*============================================================================
* RT-Thread Memory Pool Integration Example for QP/C
* Last updated: 2025-07-21
*
* This example demonstrates how to use RT-Thread memory pools as the backend
* for QP/C event pools with zero intrusion to the QP/C core.
============================================================================*/

#include "qpc.h"
#include <rtthread.h>
#include "qf_rtmpool_config.h"
#if QF_ENABLE_RT_MEMPOOL
#include "qf_rtmpool.h"
#endif
#include <finsh.h>
#include <stdio.h>

#ifdef QPC_RTMPoolDemo
Q_DEFINE_THIS_MODULE("rtmpool_demo")

/* Application-specific events */
enum AppSignals {
    TICK_SIG = Q_USER_SIG,
    MAX_SIG
};

/* Event with payload */
typedef struct {
    QEvt super;
    uint32_t data;
    uint32_t _padding; // 保证DataEvt比QEvt大，防止事件池断言
} DataEvt;

/* Active Object class */
typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    uint32_t counter;
} DemoAO;

static DemoAO l_demoAO;
QActive * const AO_Demo = &l_demoAO.super;

/* State machine declarations */
static QState DemoAO_initial(DemoAO * const me, QEvt const * const e);
static QState DemoAO_running(DemoAO * const me, QEvt const * const e);

/* Event pools - same declarations as usual QP/C applications */
ALIGN(RT_ALIGN_SIZE)
static QF_MPOOL_EL(QEvt) l_smlPoolSto[10]; /* small pool for basic events */

ALIGN(RT_ALIGN_SIZE)
static QF_MPOOL_EL(DataEvt) l_medPoolSto[5]; /* medium pool for DataEvt events */

/* AO queue and stack storage */
static QEvt const *l_demoQSto[10];
static uint8_t l_demoStackSto[512];

/* AO constructor */
static void DemoAO_ctor(void) {
    DemoAO *me = &l_demoAO;
    QActive_ctor(&me->super, Q_STATE_CAST(&DemoAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, TICK_SIG, 0U);
    me->counter = 0U;
}

/* Initial state */
static QState DemoAO_initial(DemoAO * const me, QEvt const * const e) {
    (void)e; /* unused parameter */

    /* Start periodic time event */
    QTimeEvt_armX(&me->timeEvt, RT_TICK_PER_SECOND, RT_TICK_PER_SECOND);

    return Q_TRAN(&DemoAO_running);
}

/* Running state */
static QState DemoAO_running(DemoAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("DemoAO: Entered running state\n");
            status = Q_HANDLED();
            break;
        }

        case TICK_SIG: {
            ++me->counter;

            /* Every 3 ticks, publish a DataEvt with dynamic allocation */
            if ((me->counter % 3) == 0) {
                DataEvt *evt = Q_NEW(DataEvt, MAX_SIG); /* AUTO-allocated event */
                if (evt != NULL) {
                    evt->data = me->counter;
                    rt_kprintf("DemoAO: Created event with data=%u\n", evt->data);

                    /* Post to self (will be garbage collected automatically) */
                    QACTIVE_POST(&me->super, &evt->super, me);
                }
                else {
                    rt_kprintf("DemoAO: Failed to allocate DataEvt\n");
                }
            }

            status = Q_HANDLED();
            break;
        }

        case MAX_SIG: {
            DataEvt const *dataEvt = (DataEvt const *)e;
            rt_kprintf("DemoAO: Received DataEvt with data=%u\n", dataEvt->data);

#if QF_ENABLE_RT_MEMPOOL && QF_RTMPOOL_DEBUG
            /* QF_pool_ is not a public symbol; direct access is not portable. */
            /* QF_RTMemPool_printStats(&QF_pool_[0]);*/ /* Small pool stats */
            /* QF_RTMemPool_printStats(&QF_pool_[1]);*/ /* Medium pool stats */
#endif

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

/* Application initialization */
void rtmpool_demo_init(void) {
    rt_kprintf("RT-Thread Memory Pool Demo for QP/C\n");
    rt_kprintf("QP/C version: %s\n", QP_VERSION_STR);

#if QF_ENABLE_RT_MEMPOOL
    rt_kprintf("RT-Thread memory pool backend is ENABLED\n");
#else
    rt_kprintf("Using native QP/C memory pool implementation\n");
#endif

    /* 打印事件结构体大小，便于排查断言201 */
    rt_kprintf("sizeof(QEvt)=%d, sizeof(DataEvt)=%d\n", (int)sizeof(QEvt), (int)sizeof(DataEvt));

    /* Initialize the framework */
    QF_init();

    /* Initialize event pools (严格递增) */
    QF_poolInit(l_smlPoolSto, sizeof(l_smlPoolSto), sizeof(QEvt));
    QF_poolInit(l_medPoolSto, sizeof(l_medPoolSto), sizeof(DataEvt));

    /* Initialize demo active object */
    DemoAO_ctor();
}

/* Start the application */
int rtmpool_demo_start(void) {
    rtmpool_demo_init();

    /* Start the active object */
    QACTIVE_START(AO_Demo,          /* active object to start */
                 1U,                /* priority */
                 l_demoQSto,        /* event queue storage */
                 Q_DIM(l_demoQSto), /* event queue size */
                 l_demoStackSto,    /* stack storage */
                 sizeof(l_demoStackSto), /* stack size */
                 NULL);             /* initialization parameter */

    /* Start the framework */
    return QF_run();
}

/* Export to RT-Thread shell */
MSH_CMD_EXPORT(rtmpool_demo_start, Start RT-Thread mempool integration demo);


int main(void)
{
    int ret = rtmpool_demo_start();
    rt_kprintf("RT-Thread Memory Pool Demo finished with return code: %d\n", ret);
    /* Note: In a real application, you might want to stop the QF application here
     * or perform cleanup. For this demo, we just return the status.
     */
    return ret;
}

#endif /* QPC_RTMPoolDemo */
