/*============================================================================
* Product: Idle CPU Performance Test for QPC-RT-Thread
* Integrated into performance_tests framework
* Based on original idle_cpu_test.c from backup_old_tests
============================================================================*/
#include "perf_test.h"
#include "qpc.h"
#include "cycle_counter.h"

/*==========================================================================*/
/* Idle CPU Test Data Structure */
/*==========================================================================*/
typedef struct
{
    rt_uint32_t measurement_count;
    rt_uint32_t target_measurements;
    rt_uint32_t test_duration_cycles;
    rt_uint32_t start_cycles;
    rt_uint32_t total_idle_count;
    rt_uint32_t current_idle_count;
    rt_bool_t test_running;
    rt_thread_t monitor_thread;
} idle_cpu_test_data_t;

static idle_cpu_test_data_t s_idle_data;

/*==========================================================================*/
/* DWT Cycle Counter Functions */
/*==========================================================================*/
#define DWT_CTRL (*(volatile rt_uint32_t *)0xE0001000)
#define DWT_CYCCNT (*(volatile rt_uint32_t *)0xE0001004)
#define CoreDebug_DEMCR (*(volatile rt_uint32_t *)0xE000EDFC)

/*==========================================================================*/
/* Idle Task - Simulates idle CPU activity */
/*==========================================================================*/
static void idle_task_func(void *parameter)
{
    idle_cpu_test_data_t *data = (idle_cpu_test_data_t *)parameter;
    rt_uint32_t local_idle_count = 0;

    rt_kprintf("[Idle CPU Task] Started\n");

    while (data->test_running)
    {
        /* Simulate idle work - simple counting */
        for (int i = 0; i < 1000; i++)
        {
            local_idle_count++;
        }

        /* Update global idle count periodically */
        data->current_idle_count = local_idle_count;

        /* Small yield to allow other tasks to run */
        rt_thread_yield();
    }

    data->total_idle_count = local_idle_count;
    rt_kprintf("[Idle CPU Task] Finished - Total idle count: %u\n", local_idle_count);
}

/*==========================================================================*/
/* Monitor Thread - Collects idle measurements */
/*==========================================================================*/
static void monitor_thread_func(void *parameter)
{
    idle_cpu_test_data_t *data = (idle_cpu_test_data_t *)parameter;
    rt_uint32_t last_idle_count = 0;
    rt_uint32_t measurement_interval_ms = 100; /* 100ms per measurement */

    rt_kprintf("[Idle CPU Monitor] Started\n");

    while (data->test_running && data->measurement_count < data->target_measurements)
    {
        rt_thread_mdelay(measurement_interval_ms);

        /* Record measurement */
        rt_uint32_t current_idle = data->current_idle_count;
        rt_uint32_t idle_delta = current_idle - last_idle_count;

        data->total_idle_count += idle_delta; // Add current sample idle count
        data->measurement_count++;
        last_idle_count = current_idle;

        /* Log every 10 measurements */
        if (data->measurement_count % 10 == 0)
        {
            rt_kprintf("[Idle CPU Monitor] Progress: %u/%u measurements, current idle delta: %u\n",
                       data->measurement_count, data->target_measurements, idle_delta);
        }

        /* Check if test duration reached */
        rt_uint32_t current_cycles = dwt_get_cycles();
        if ((current_cycles - data->start_cycles) >= data->test_duration_cycles)
        {
            break;
        }
    }

    rt_kprintf("[Idle CPU Monitor] Finished - %u measurements\n", data->measurement_count);
}

/*==========================================================================*/
/* Test Implementation Functions */
/*==========================================================================*/
static int idle_cpu_test_init(perf_test_case_t *tc)
{
    /* Initialize DWT cycle counter */
    dwt_init();

    /* Initialize test data */
    s_idle_data.measurement_count = 0;
    s_idle_data.target_measurements = 100;      /* Match expected report */
    s_idle_data.test_duration_cycles = 1000000; /* 1M cycles for test duration */
    s_idle_data.start_cycles = 0;
    s_idle_data.total_idle_count = 0;
    s_idle_data.current_idle_count = 0;
    s_idle_data.test_running = RT_TRUE;
    s_idle_data.monitor_thread = RT_NULL;

    /* Initialize statistics */
    tc->stats.measurements = 0;
    tc->stats.total_cycles = s_idle_data.test_duration_cycles;
    tc->stats.total_idle_count = 0;
    tc->stats.avg_idle_per_measurement = 0;

    tc->user_data = &s_idle_data;
    tc->iterations = 0;

    rt_kprintf("[Idle CPU Test] Initialized - Target: %u measurements, Duration: %u cycles\n",
               s_idle_data.target_measurements, s_idle_data.test_duration_cycles);

    return 0;
}

static int idle_cpu_test_run(perf_test_case_t *tc)
{
    idle_cpu_test_data_t *data = (idle_cpu_test_data_t *)tc->user_data;
    rt_thread_t idle_thread;

    rt_kprintf("[Idle CPU Test] Starting idle CPU test...\n");

    /* Record start time */
    data->start_cycles = dwt_get_cycles();

    /* Create idle task thread */
    idle_thread = rt_thread_create("idle_task",
                                   idle_task_func,
                                   data,
                                   2048,
                                   RT_THREAD_PRIORITY_MAX - 1, /* Low priority */
                                   10);

    /* Create monitor thread */
    data->monitor_thread = rt_thread_create("idle_monitor",
                                            monitor_thread_func,
                                            data,
                                            2048,
                                            RT_THREAD_PRIORITY_MAX / 2,
                                            10);

    if (!idle_thread || !data->monitor_thread)
    {
        rt_kprintf("[Idle CPU Test] Failed to create threads\n");
        if (idle_thread)
            rt_thread_delete(idle_thread);
        if (data->monitor_thread)
            rt_thread_delete(data->monitor_thread);
        return -1;
    }

    /* Start threads */
    rt_thread_startup(idle_thread);
    rt_thread_startup(data->monitor_thread);

    /* Wait for completion */
    while (data->test_running)
    {
        /* Check if test should stop */
        rt_uint32_t current_cycles = dwt_get_cycles();
        if ((current_cycles - data->start_cycles) >= data->test_duration_cycles ||
            data->measurement_count >= data->target_measurements)
        {
            data->test_running = RT_FALSE;
            break;
        }

        rt_thread_mdelay(100);
        tc->iterations++;
    }

    /* Wait for threads to naturally exit, no manual delete needed to avoid assertion failure */
    rt_thread_mdelay(200);

    /* Update final statistics */
    tc->stats.measurements = data->measurement_count;
    tc->stats.total_idle_count = data->total_idle_count;
    if (data->measurement_count > 0)
    {
        tc->stats.avg_idle_per_measurement = data->total_idle_count / data->measurement_count;
    }

    rt_kprintf("[Idle CPU Test] Completed - Measurements: %u, Total idle: %u, Avg per measurement: %u\n",
               data->measurement_count, data->total_idle_count, tc->stats.avg_idle_per_measurement);

    return 0;
}

static int idle_cpu_test_stop(perf_test_case_t *tc)
{
    idle_cpu_test_data_t *data = (idle_cpu_test_data_t *)tc->user_data;
    if (data)
    {
        data->test_running = RT_FALSE;
    }

    rt_kprintf("[Idle CPU Test] Stopped\n");
    return 0;
}

/* Register the test case */
PERF_TEST_REG(idle_cpu, idle_cpu_test_init, idle_cpu_test_run, idle_cpu_test_stop);
