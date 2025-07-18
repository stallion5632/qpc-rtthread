#include "perf_test.h"
#include <rtthread.h>

/* Timer performance test data */
typedef struct
{
    rt_timer_t test_timer;
    uint32_t tick_count;
    uint32_t report_count;
} timer_test_data_t;

static timer_test_data_t s_timer_data;

/* Timer callback for performance testing */
static void timer_test_callback(void *param)
{
    perf_test_case_t *tc = (perf_test_case_t *)param;
    if (tc->state == 2)
    { /* STATE_RUNNING */
        s_timer_data.tick_count++;
        tc->iterations++;

        /* Generate report every 10 ticks */
        if ((s_timer_data.tick_count % 10) == 0)
        {
            s_timer_data.report_count++;
            rt_kprintf("[Timer Test] Tick: %u, Reports: %u\n",
                       s_timer_data.tick_count, s_timer_data.report_count);
        }
    }
}

static int timer_ao_init(perf_test_case_t *tc)
{

    /* Initialize timer test data */
    s_timer_data.tick_count = 0;
    s_timer_data.report_count = 0;

    /* Create test timer */
    s_timer_data.test_timer = rt_timer_create("timer_test",
                                              timer_test_callback,
                                              tc,
                                              RT_TICK_PER_SECOND / 20, /* 50ms */
                                              RT_TIMER_FLAG_PERIODIC);

    return (s_timer_data.test_timer != RT_NULL) ? 0 : -1;
}

static int timer_ao_run(perf_test_case_t *tc)
{
    /* Start the timer */
    if (s_timer_data.test_timer)
    {
        rt_timer_start(s_timer_data.test_timer);
    }

    /* Run for test duration */
    rt_thread_mdelay(3000); /* Run for 3 seconds */

    return 0;
}

static int timer_ao_stop(perf_test_case_t *tc)
{
    /* Stop and delete timer */
    if (s_timer_data.test_timer)
    {
        rt_timer_stop(s_timer_data.test_timer);
        rt_timer_delete(s_timer_data.test_timer);
        s_timer_data.test_timer = RT_NULL;
    }

    /* Update final iteration count */
    tc->iterations = s_timer_data.tick_count;

    rt_kprintf("[Timer AO] Total ticks: %u, Reports: %u\n",
               s_timer_data.tick_count, s_timer_data.report_count);

    return 0;
}

PERF_TEST_REG(timer_ao, timer_ao_init, timer_ao_run, timer_ao_stop);
