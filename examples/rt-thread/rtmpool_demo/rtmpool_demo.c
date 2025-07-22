/*****************************************************************************
* @file    rtmpool_demo.c
* @brief   Example: Integrating RT-Thread memory pool as QP/C event pool backend
* @date    Last updated: 2025-07-21
* @details This example demonstrates how to use RT-Thread memory pools as the
*          backend for QP/C event pools with zero intrusion to the QP/C core.
*****************************************************************************/

#include "qpc.h"
#include <rtthread.h>
#include "qf_rtmpool_config.h"
#if QF_ENABLE_RT_MEMPOOL
#include "qf_rtmpool.h"
#endif
#include <finsh.h>
#include <stdio.h>

#ifdef QPC_USING_RTMPOOL_DEMO
Q_DEFINE_THIS_MODULE("rtmpool_demo")

/* Application-specific signals */
typedef enum {
    TICK_SIG = Q_USER_SIG,
    MARGIN_SIG,
    MAX_SIG
} AppSignals;

/* Data event with payload */
typedef struct {
    QEvt super;
    uint32_t data;
    uint32_t padding; /* Padding to ensure DataEvt is larger than QEvt */
} DataEvt;

/* Active Object class */
typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    uint32_t counter;
} DemoAO;

static DemoAO l_demoAO;
QActive *const AO_Demo = &l_demoAO.super;

/* State machine declarations */
static QState DemoAO_initial(DemoAO *const me, QEvt const *const e);
static QState DemoAO_running(DemoAO *const me, QEvt const *const e);


/* Allocate event storage for both backends */
ALIGN(RT_ALIGN_SIZE)
static QF_MPOOL_EL(QEvt) l_smlPoolSto[100];
ALIGN(RT_ALIGN_SIZE)
static QF_MPOOL_EL(DataEvt) l_medPoolSto[200];
/* Adjust margin parameters as needed */
#define QEVTPOOL_MARGIN   0
#define DATAEVTPOOL_MARGIN 0

#if defined(QF_ENABLE_RT_MEMPOOL) && (QF_ENABLE_RT_MEMPOOL == 1)
/* RT-Thread memory pool instances and names (global static) */
static QF_RTMemPool s_evtPool;
static QF_RTMemPool s_dataEvtPool;
static const char poolName_evt[] = "QEvtPool";
static const char poolName_data[] = "DataEvtPool";
#endif

/* Active Object queue and stack storage */
static QEvt const *l_demoQSto[100];
static uint8_t l_demoStackSto[2048];

/* Active Object constructor */
static void DemoAO_ctor(void)
{
    DemoAO *me = &l_demoAO;
    QActive_ctor(&me->super, Q_STATE_CAST(&DemoAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, TICK_SIG, 0U);
    me->counter = 0U;
}

/* Initial state handler */
static QState DemoAO_initial(DemoAO * const me, QEvt const * const e) {
    (void)e; /* Parameter not used */
    /* Start periodic time event */
    QTimeEvt_armX(&me->timeEvt, RT_TICK_PER_SECOND, RT_TICK_PER_SECOND);
    return Q_TRAN(&DemoAO_running);
}

/* Running state handler */
static QState DemoAO_running(DemoAO *const me, QEvt const *const e)
{
    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            rt_kprintf("[DemoAO] Entered running state\n");
            return Q_HANDLED();
        }
        case TICK_SIG:
        {
            /* Stress test: allocate multiple events to observe pool recycling */
            uint32_t i;
            for (i = 0U; i < 10U; ++i) {
                QEvt *evt0 = QF_NEW(QEvt, 3, MARGIN_SIG); /* margin=0: allow allocation as long as pool is not empty */
                if (evt0 != RT_NULL) {
                    QACTIVE_POST(&me->super, evt0, me);
                }
            }
            for (i = 0U; i < 2U; ++i) {
                DataEvt *evt1 = QF_NEW(DataEvt, 1, MAX_SIG);
                if (evt1 != RT_NULL) {
                    evt1->data = me->counter * 100U + i;
                    QACTIVE_POST(&me->super, &evt1->super, me);
                }
            }
            me->counter++;
            /* Print pool stats every 5 ticks */
#if defined(QF_ENABLE_RT_MEMPOOL) && (QF_ENABLE_RT_MEMPOOL == 1) && defined(QF_RTMPOOL_EXT) && (QF_RTMPOOL_EXT == 1)
            if ((me->counter % 5U) == 0U)
            {
                QF_RTMemPoolMgr_printStats();
            }
#endif
            return Q_HANDLED();
        }
        case MARGIN_SIG:
        {
            /* This event is processed and automatically recycled by the framework. */
            return Q_HANDLED();
        }
        case MAX_SIG:
        {
            /* This event is processed and automatically recycled by the framework. */
            return Q_HANDLED();
        }
        default:
        {
            return Q_SUPER(&QHsm_top);
        }
    }
}

/* Demo initialization function */
void rtmpool_demo_init(void)
{
    /* Demo initialization: memory-pool backend selection */
    rt_kprintf("Memory Pool Demo for QP/C\n");
    /* Initialize QP/C framework */
    QF_init();

#if defined(QF_ENABLE_RT_MEMPOOL) && (QF_ENABLE_RT_MEMPOOL == 1)
    /* RT-Thread mempool initialization */
#if defined(QF_RTMPOOL_EXT) && (QF_RTMPOOL_EXT == 1)
    QF_RTMemPoolMgr_init();
#endif
    (void)QF_RTMemPool_init(&s_evtPool, poolName_evt, l_smlPoolSto,
                             Q_DIM(l_smlPoolSto), sizeof(QEvt), QEVTPOOL_MARGIN);
    QF_RTMemPoolMgr_registerPool(&s_evtPool);
    (void)QF_RTMemPool_init(&s_dataEvtPool, poolName_data, l_medPoolSto,
                             Q_DIM(l_medPoolSto), sizeof(DataEvt), DATAEVTPOOL_MARGIN);
    QF_RTMemPoolMgr_registerPool(&s_dataEvtPool);
#else
    /* native QP/C mempool initialization */
    QF_poolInit(l_smlPoolSto, sizeof(l_smlPoolSto), sizeof(QEvt));
    QF_poolInit(l_medPoolSto, sizeof(l_medPoolSto), sizeof(DataEvt));
#endif
    /* Initialize demo active object */
    DemoAO_ctor();
}

int rtmpool_demo_start(void)
{
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

#endif /* QPC_USING_RTMPOOL_DEMO */
