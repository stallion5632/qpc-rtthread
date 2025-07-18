#include "perf_test.h"
#include "qpc.h"
#include "app_main.h"
#include <rtthread.h>

/* CounterAO: Active Object for counter performance test */
typedef struct CounterAO {
    QActive super;           /*< QP/C base class: must be first member */
    uint32_t count;          /*< Counter value */
    rt_timer_t timer;        /*< RT-Thread timer handle */
} CounterAO;

/* State handler for CounterAO */
static QState CounterAO_active(CounterAO * const me, QEvt const * const e);

/* Performance test data for counter AO */
typedef struct {
    QActive *counter_ao;         /*< Pointer to AO instance */
    uint32_t updates_sent;       /*< Number of update events sent */
    uint32_t responses_received; /*< Number of responses received */
    rt_tick_t start_time;        /*< Test start tick */
} counter_test_data_t;

static counter_test_data_t s_counter_data; /*< Global test data instance */

static CounterAO s_counter_ao; /*< Static AO instance */
static QEvt const *counterAOQueueSto[32]; /*< Event queue storage, size 32 for high throughput */
static rt_uint8_t counterAOStack[1024];   /*< Static thread stack for AO, required by RT-Thread */

/* State handler for CounterAO  */
static QState CounterAO_active(CounterAO * const me, QEvt const * const e)
{
    QState status = Q_SUPER(&QHsm_top);

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            /* Reset test counters on entry */
            s_counter_data.updates_sent = 0U;
            s_counter_data.responses_received = 0U;
            status = Q_HANDLED();
            break;
        }
        case COUNTER_UPDATE_SIG: {
            /* Handle counter update event */
            me->count++;
            s_counter_data.responses_received++;
            status = Q_HANDLED();
            break;
        }
        default: {
            /* Pass unhandled signals to superstate */
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status;
}

/* Initial state handler for CounterAO  */
static QState CounterAO_initial(CounterAO * const me, QEvt const * const e)
{
    (void)e; /* Unused parameter */
    me->count = 0U; /* Initialize counter */
    return Q_TRAN(&CounterAO_active); /* Transition to active state */
}

/* Timer callback for periodic counter updates  */
static void counter_timer_callback(void *param)
{
    perf_test_case_t *tc = (perf_test_case_t *)param;
    /* Only post event if test is running */
    if (tc->state == 2) { /* STATE_RUNNING */
        static QEvt const update_evt = { COUNTER_UPDATE_SIG, 0U, 0U };
        QACTIVE_POST((QActive*)&s_counter_ao, &update_evt, tc);
        s_counter_data.updates_sent++;
        tc->iterations++;
    }
}

/* AO initialization function for performance test  */
static int counter_ao_init(perf_test_case_t *tc)
{

    /* Construct AO and set initial state */
    QActive_ctor((QActive*)&s_counter_ao, Q_STATE_CAST(&CounterAO_initial));
    s_counter_ao.count = 0U;

    /* Set AO thread name before start */
    QActive_setAttr((QActive*)&s_counter_ao, THREAD_NAME_ATTR, "cnt_ao");

    /* Start AO with event queue and stack */
    QActive_start_((QActive*)&s_counter_ao,
                  1, /* prioSpec: AO priority, tune as needed */
                  counterAOQueueSto, 32, /* Event queue storage and length */
                  counterAOStack, sizeof(counterAOStack), /* Stack pointer and size */
                  NULL); /* par: no init parameter */

    /* Create periodic timer for event generation */
    s_counter_ao.timer = rt_timer_create("counter_tmr",
                                        counter_timer_callback,
                                        tc,
                                        RT_TICK_PER_SECOND / 10,  /* 100ms period */
                                        RT_TIMER_FLAG_PERIODIC);

    s_counter_data.counter_ao = (QActive*)&s_counter_ao;
    s_counter_data.start_time = rt_tick_get();

    return (s_counter_ao.timer != RT_NULL) ? 0 : -1;
}

/* AO run function for performance test  */
static int counter_ao_run(perf_test_case_t *tc)
{
    /* Start periodic timer for counter updates */
    if (s_counter_ao.timer != RT_NULL) {
        rt_timer_start(s_counter_ao.timer);
    }

    /* Run test for fixed duration */
    rt_thread_mdelay(5000U); /* 5 seconds */

    return 0;
}

/* AO stop/cleanup function for performance test  */
static int counter_ao_stop(perf_test_case_t *tc)
{
    /* Stop and delete timer if valid */
    if (s_counter_ao.timer != RT_NULL) {
        rt_timer_stop(s_counter_ao.timer);
        rt_timer_delete(s_counter_ao.timer);
        s_counter_ao.timer = RT_NULL;
    }

    /* Update final iteration count for reporting */
    tc->iterations = s_counter_data.updates_sent;

    /* Print test summary */
    rt_kprintf("[Counter AO] Updates sent: %u, Responses: %u, Count: %u\n",
               s_counter_data.updates_sent,
               s_counter_data.responses_received,
               s_counter_ao.count);

    return 0;
}

/* Register the counter AO performance test case */
PERF_TEST_REG(counter_ao, counter_ao_init, counter_ao_run, counter_ao_stop);
