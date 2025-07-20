/*============================================================================
* Product: Latency Performance Test for QPC-RT-Thread
* Integrated into performance_tests framework
* Based on original latency_test.c from backup_old_tests
============================================================================*/
#include "perf_test.h"
#include "qpc.h"
#include "cycle_counter.h"

/*==========================================================================*/
/* Latency Test Data Structure */
/*==========================================================================*/
typedef struct
{
    rt_uint32_t measurement_count;
    rt_uint32_t target_measurements;
    rt_uint32_t start_time;
    rt_uint32_t end_time;
    rt_uint32_t min_latency;
    rt_uint32_t max_latency;
    rt_uint32_t total_latency;
    rt_bool_t test_running;
} latency_test_data_t;

static latency_test_data_t s_latency_data;

/*==========================================================================*/
/* Test Implementation Functions */
/*==========================================================================*/
static int latency_test_init(perf_test_case_t *tc)
{
    /* Initialize DWT cycle counter */
    dwt_init();

    /* Initialize test data */
    s_latency_data.measurement_count = 0;
    s_latency_data.target_measurements = 1000; /* Default target */
    s_latency_data.start_time = 0;
    s_latency_data.end_time = 0;
    s_latency_data.min_latency = 0xFFFFFFFF;
    s_latency_data.max_latency = 0;
    s_latency_data.total_latency = 0;
    s_latency_data.test_running = RT_TRUE;

    /* Initialize statistics */
    tc->stats.measurements = 0;
    tc->stats.min_value = 0xFFFFFFFF;
    tc->stats.max_value = 0;
    tc->stats.total_latency = 0;
    tc->stats.avg_value = 0;

    tc->user_data = &s_latency_data;
    tc->iterations = 0;

    rt_kprintf("[Latency Test] Initialized - Target: %u measurements\n",
               s_latency_data.target_measurements);

    return 0;
}

static int latency_test_run(perf_test_case_t *tc)
{
    latency_test_data_t *data = (latency_test_data_t *)tc->user_data;

    rt_kprintf("[Latency Test] Starting latency measurements...\n");

    while (data->test_running && data->measurement_count < data->target_measurements)
    {
        /* Measure latency of a simple operation */
        rt_uint32_t start_cycles = dwt_get_cycles();

        /* Simulate some work - memory access and simple calculation */
        volatile rt_uint32_t temp = *((volatile rt_uint32_t *)0x20000000);
        temp = temp + 1;
        *((volatile rt_uint32_t *)0x20000004) = temp;

        rt_uint32_t end_cycles = dwt_get_cycles();
        rt_uint32_t latency = end_cycles - start_cycles;

        /* Add minimum latency check to avoid zero values */
        if (latency < 1)
        {
            latency = 1; /* Minimum 1 cycle for any operation */
        }

        /* Log only every 100 measurements to reduce output */
        if ((data->measurement_count + 1) % 100 == 0 || data->measurement_count < 5)
        {
            rt_kprintf("[Latency Test] Measurement %u: %u cycles (start=%u, end=%u)\n",
                       data->measurement_count + 1, latency, start_cycles, end_cycles);
        }

        /* Update statistics */
        data->measurement_count++;
        data->total_latency += latency;

        if (latency < data->min_latency)
        {
            data->min_latency = latency;
        }
        if (latency > data->max_latency)
        {
            data->max_latency = latency;
        }

        tc->iterations++;

        /* Small delay to avoid overwhelming the system */
        rt_thread_mdelay(1);
    } /* Calculate final statistics */
    if (data->measurement_count > 0)
    {
        tc->stats.measurements = data->measurement_count;
        tc->stats.min_value = data->min_latency;
        tc->stats.max_value = data->max_latency;
        tc->stats.total_latency = data->total_latency;
        tc->stats.avg_value = data->total_latency / data->measurement_count;

        rt_kprintf("[Latency Test] Performance Summary: Min=%u, Max=%u, Avg=%u cycles\n",
                   data->min_latency, data->max_latency, tc->stats.avg_value);
    }

    rt_kprintf("[Latency Test] Completed %u measurements\n", data->measurement_count);
    return 0;
}

static int latency_test_stop(perf_test_case_t *tc)
{
    latency_test_data_t *data = (latency_test_data_t *)tc->user_data;
    if (data)
    {
        data->test_running = RT_FALSE;
    }

    rt_kprintf("[Latency Test] Stopped\n");
    return 0;
}

/* Register the test case */
PERF_TEST_REG(latency, latency_test_init, latency_test_run, latency_test_stop);
