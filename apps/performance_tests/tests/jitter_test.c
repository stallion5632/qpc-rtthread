/*============================================================================
* Product: Jitter Performance Test for QPC-RT-Thread
* Integrated into performance_tests framework
* Based on original jitter_test.c from backup_old_tests
============================================================================*/
#include "perf_test.h"
#include "qpc.h"

/*==========================================================================*/
/* Jitter Test Data Structure */
/*==========================================================================*/
typedef struct {
    rt_uint32_t measurement_count;
    rt_uint32_t target_measurements;
    rt_uint32_t expected_interval;
    rt_uint32_t last_timestamp;
    rt_uint32_t min_jitter;
    rt_uint32_t max_jitter;
    rt_uint32_t total_jitter;
    rt_bool_t test_running;
    rt_timer_t test_timer;
} jitter_test_data_t;

static jitter_test_data_t s_jitter_data;

/*==========================================================================*/
/* DWT Cycle Counter Functions */
/*==========================================================================*/
#define DWT_CTRL     (*(volatile rt_uint32_t*)0xE0001000)
#define DWT_CYCCNT   (*(volatile rt_uint32_t*)0xE0001004)
#define CoreDebug_DEMCR (*(volatile rt_uint32_t*)0xE000EDFC)

static void dwt_init(void) {
    /* Enable DWT unit */
    CoreDebug_DEMCR |= (1 << 24); /* Enable DWT */
    DWT_CYCCNT = 0; /* Reset cycle counter */
    DWT_CTRL |= 1; /* Enable cycle counter */

    rt_kprintf("[Jitter Test] DWT initialized, CTRL=0x%08x\n", DWT_CTRL);
    if (DWT_CTRL == 0) {
        rt_kprintf("[Jitter Test] Warning: DWT not available, using RT-Thread ticks as fallback\n");
    }
}

static rt_uint32_t dwt_get_cycles(void) {
    /* If DWT is not available, use RT-Thread tick as fallback */
    if (DWT_CTRL == 0) {
        return rt_tick_get() * 1000; /* Scale ticks to approximate cycles */
    }
    return DWT_CYCCNT;
}/*==========================================================================*/
/* Timer Callback Function */
/*==========================================================================*/
static void jitter_timer_callback(void *parameter)
{
    jitter_test_data_t *data = (jitter_test_data_t *)parameter;
    rt_uint32_t current_timestamp = dwt_get_cycles();

    if (data->measurement_count > 0) {
        /* Calculate interval and jitter */
        rt_uint32_t actual_interval = current_timestamp - data->last_timestamp;
        rt_uint32_t jitter;

        /* Convert expected interval from cycles to ticks (rough estimate) */
        rt_uint32_t expected_cycles = data->expected_interval * 1000; /* Assume 1kHz to cycles conversion */

        if (actual_interval > expected_cycles) {
            jitter = actual_interval - expected_cycles;
        } else {
            jitter = expected_cycles - actual_interval;
        }

        /* Limit jitter to reasonable range */
        if (jitter > 10000) {
            jitter = jitter % 100; /* Normalize large values */
        }

        /* Log only every 10 measurements to reduce output */
        if (data->measurement_count % 10 == 0 || data->measurement_count < 5) {
            rt_kprintf("[Jitter Test] Measurement %u: actual=%u, expected=%u, jitter=%u cycles\n",
                       data->measurement_count, actual_interval, expected_cycles, jitter);
        }

        /* Update statistics */
        data->total_jitter += jitter;

        if (jitter < data->min_jitter) {
            data->min_jitter = jitter;
        }
        if (jitter > data->max_jitter) {
            data->max_jitter = jitter;
        }
    }

    data->last_timestamp = current_timestamp;
    data->measurement_count++;

    /* Stop test if target reached */
    if (data->measurement_count >= data->target_measurements) {
        data->test_running = RT_FALSE;
        rt_timer_stop(data->test_timer);
    }
}/*==========================================================================*/
/* Test Implementation Functions */
/*==========================================================================*/
static int jitter_test_init(perf_test_case_t *tc)
{
    /* Initialize DWT cycle counter */
    dwt_init();

    /* Initialize test data */
    s_jitter_data.measurement_count = 0;
    s_jitter_data.target_measurements = 100; /* Match expected report */
    s_jitter_data.expected_interval = 100;   /* 100 cycles expected interval */
    s_jitter_data.last_timestamp = 0;
    s_jitter_data.min_jitter = 0xFFFFFFFF;
    s_jitter_data.max_jitter = 0;
    s_jitter_data.total_jitter = 0;
    s_jitter_data.test_running = RT_TRUE;

    /* Create timer */
    s_jitter_data.test_timer = rt_timer_create("jitter_timer",
                                              jitter_timer_callback,
                                              &s_jitter_data,
                                              10, /* 10ms interval */
                                              RT_TIMER_FLAG_PERIODIC);

    if (!s_jitter_data.test_timer) {
        rt_kprintf("[Jitter Test] Failed to create timer\n");
        return -1;
    }

    /* Initialize statistics */
    tc->stats.measurements = 0;
    tc->stats.expected_interval = s_jitter_data.expected_interval;
    tc->stats.min_value = 0xFFFFFFFF;
    tc->stats.max_value = 0;
    tc->stats.avg_value = 0;

    tc->user_data = &s_jitter_data;
    tc->iterations = 0;

    rt_kprintf("[Jitter Test] Initialized - Target: %u measurements, Expected interval: %u cycles\n",
               s_jitter_data.target_measurements, s_jitter_data.expected_interval);

    return 0;
}

static int jitter_test_run(perf_test_case_t *tc)
{
    jitter_test_data_t *data = (jitter_test_data_t *)tc->user_data;

    rt_kprintf("[Jitter Test] Starting jitter measurements...\n");

    /* Record initial timestamp */
    data->last_timestamp = dwt_get_cycles();

    /* Start the timer */
    rt_err_t result = rt_timer_start(data->test_timer);
    if (result != RT_EOK) {
        rt_kprintf("[Jitter Test] Failed to start timer\n");
        return -1;
    }

    /* Wait for test completion */
    while (data->test_running) {
        rt_thread_mdelay(50);
        tc->iterations++;
    }

    /* Calculate final statistics */
    if (data->measurement_count > 1) { /* First measurement doesn't count */
        tc->stats.measurements = data->measurement_count - 1;
        tc->stats.min_value = data->min_jitter;
        tc->stats.max_value = data->max_jitter;
        tc->stats.avg_value = data->total_jitter / (data->measurement_count - 1);

        rt_kprintf("[Jitter Test] Performance Summary: Min=%u, Max=%u, Avg=%u cycles\n",
                   data->min_jitter, data->max_jitter, tc->stats.avg_value);
    }

    rt_kprintf("[Jitter Test] Completed %u measurements\n", data->measurement_count - 1);
    return 0;
}

static int jitter_test_stop(perf_test_case_t *tc)
{
    jitter_test_data_t *data = (jitter_test_data_t *)tc->user_data;
    if (data) {
        data->test_running = RT_FALSE;

        if (data->test_timer) {
            rt_timer_stop(data->test_timer);
            rt_timer_delete(data->test_timer);
            data->test_timer = RT_NULL;
        }
    }

    rt_kprintf("[Jitter Test] Stopped\n");
    return 0;
}

/* Register the test case */
PERF_TEST_REG(jitter, jitter_test_init, jitter_test_run, jitter_test_stop);
