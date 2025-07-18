#include "perf_test.h"
#include <rtthread.h>

/* Multi-thread performance test data */
typedef struct
{
    rt_thread_t *threads;
    uint32_t thread_count;
    volatile uint32_t shared_counter;
    rt_mutex_t counter_mutex;
    volatile rt_bool_t should_stop;
} multithread_test_data_t;

static multithread_test_data_t s_mt_data;

/* Worker thread function */
static void worker_thread_entry(void *param)
{
    perf_test_case_t *tc = (perf_test_case_t *)param;
    uint32_t local_counter = 0;

    while (!s_mt_data.should_stop && tc->state == 2)
    { /* STATE_RUNNING */
        /* Perform some work */
        for (int i = 0; i < 100; i++)
        {
            local_counter++;
        }

        /* Update shared counter with mutex protection */
        rt_mutex_take(s_mt_data.counter_mutex, RT_WAITING_FOREVER);
        s_mt_data.shared_counter += local_counter;
        rt_mutex_release(s_mt_data.counter_mutex);

        tc->iterations++;
        local_counter = 0;

        /* Small yield to allow other threads to run */
        rt_thread_yield();
    }
}

static int multithread_init(perf_test_case_t *tc)
{
    s_mt_data.thread_count = 4; /* Create 4 worker threads */
    s_mt_data.shared_counter = 0;
    s_mt_data.should_stop = RT_FALSE;

    /* Create mutex for shared counter */
    s_mt_data.counter_mutex = rt_mutex_create("mt_mutex", RT_IPC_FLAG_PRIO);
    if (!s_mt_data.counter_mutex)
    {
        return -1;
    }

    /* Allocate memory for thread handles */
    s_mt_data.threads = rt_malloc(s_mt_data.thread_count * sizeof(rt_thread_t));
    if (!s_mt_data.threads)
    {
        rt_mutex_delete(s_mt_data.counter_mutex);
        return -1;
    }

    return 0;
}

static int multithread_run(perf_test_case_t *tc)
{
    char thread_name[16];

    /* Create and start worker threads */
    for (uint32_t i = 0; i < s_mt_data.thread_count; i++)
    {
        rt_snprintf(thread_name, sizeof(thread_name), "worker_%u", i);
        s_mt_data.threads[i] = rt_thread_create(thread_name,
                                                worker_thread_entry,
                                                tc,
                                                1024,
                                                RT_THREAD_PRIORITY_MAX / 2 + 1,
                                                20);
        if (s_mt_data.threads[i])
        {
            rt_thread_startup(s_mt_data.threads[i]);
        }
    }

    /* Let threads run for test duration */
    rt_thread_mdelay(2000); /* Run for 2 seconds */

    return 0;
}

static int multithread_stop(perf_test_case_t *tc)
{
    /* Signal threads to stop */
    s_mt_data.should_stop = RT_TRUE;

    /* Wait for threads to complete and delete them */
    for (uint32_t i = 0; i < s_mt_data.thread_count; i++)
    {
        if (s_mt_data.threads[i])
        {
            rt_thread_delete(s_mt_data.threads[i]);
        }
    }

    /* Clean up resources */
    rt_free(s_mt_data.threads);
    rt_mutex_delete(s_mt_data.counter_mutex);

    rt_kprintf("[Multithread Test] Shared counter: %u, Threads: %u\n",
               s_mt_data.shared_counter, s_mt_data.thread_count);

    return 0;
}

PERF_TEST_REG(multithread, multithread_init, multithread_run, multithread_stop);
