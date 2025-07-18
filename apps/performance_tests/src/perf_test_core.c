#include "perf_test.h"
#include <string.h>

/* Simple static test case registry - no linker script dependency */
#define MAX_TEST_CASES 16
perf_test_case_t *s_test_registry[MAX_TEST_CASES];
rt_int32_t s_test_count = 0;

/* Register a test case */
void perf_test_register(perf_test_case_t *test_case)
{
    if (s_test_count < MAX_TEST_CASES)
    {
        s_test_registry[s_test_count++] = test_case;
    }
}

static void test_thread_entry(void *param)
{
    perf_test_case_t *tc = (perf_test_case_t *)param;
    tc->start_tick = rt_tick_get();
    tc->iterations = 0;
    if (tc->init)
        tc->init(tc);
    tc->state = STATE_RUNNING;
    tc->result_code = tc->run(tc);
    tc->end_tick = rt_tick_get();
    tc->state = STATE_FINISHED;
}

static perf_test_case_t *find_case(const char *name)
{
    for (rt_int32_t i = 0; i < s_test_count; i++)
    {
        if (strcmp(s_test_registry[i]->name, name) == 0)
        {
            return s_test_registry[i];
        }
    }
    return RT_NULL;
}

static void reset_case(perf_test_case_t *tc)
{
    tc->state = STATE_IDLE;
    tc->start_tick = tc->end_tick = 0;
    tc->iterations = 0;
    tc->result_code = 0;
    tc->user_data = RT_NULL;
    tc->thread = RT_NULL;

    /* Reset statistics */
    rt_memset(&tc->stats, 0, sizeof(tc->stats));
    tc->stats.min_value = 0xFFFFFFFF;
}

void perf_test_list(void)
{
    rt_kprintf("Available tests:\n");
    for (rt_int32_t i = 0; i < s_test_count; i++)
    {
        perf_test_case_t *tc = s_test_registry[i];
        const char *s = (tc->state == STATE_IDLE) ? "IDLE" : (tc->state == STATE_RUNNING) ? "RUN"
                                                         : (tc->state == STATE_FINISHED)  ? "DONE"
                                                                                          : "?";
        rt_kprintf("  %s [%s]\n", tc->name, s);
    }
}

rt_int32_t perf_test_start(const char *name)
{
    perf_test_case_t *tc = find_case(name);
    if (!tc)
        return -1;
    if (tc->state == STATE_RUNNING)
        return -2;
    reset_case(tc);
    tc->thread = rt_thread_create(tc->name,
                                  test_thread_entry, tc,
                                  2048,
                                  RT_THREAD_PRIORITY_MAX / 2,
                                  10);
    if (!tc->thread)
        return -3;
    rt_thread_startup(tc->thread);
    return 0;
}

rt_int32_t perf_test_stop(const char *name)
{
    perf_test_case_t *tc = find_case(name);
    if (!tc)
        return -1;
    if (tc->state != STATE_RUNNING)
        return -2;
    if (tc->stop)
        tc->stop(tc);
    if (tc->thread)
    {
        rt_thread_delete(tc->thread);
        tc->thread = RT_NULL;
    }
    tc->state = STATE_FINISHED;
    return 0;
}

rt_int32_t perf_test_restart(const char *name)
{
    rt_int32_t r = perf_test_stop(name);
    return (r == 0) ? perf_test_start(name) : r;
}

void perf_test_report(void)
{
    rt_kprintf("=== Performance Test Report ===\n\n");

    for (rt_int32_t i = 0; i < s_test_count; i++)
    {
        perf_test_case_t *tc = s_test_registry[i];
        rt_tick_t dt = tc->end_tick - tc->start_tick;
        uint32_t ms = (uint32_t)(dt * 1000 / RT_TICK_PER_SECOND);

        /* Check test type by name and display appropriate statistics */
        if (strcmp(tc->name, "latency") == 0 && tc->stats.measurements > 0) {
            rt_kprintf("=== Latency Test Results ===\n");
            rt_kprintf("Measurements: %u\n", tc->stats.measurements);
            rt_kprintf("Min latency: %u cycles\n", tc->stats.min_value);
            rt_kprintf("Max latency: %u cycles\n", tc->stats.max_value);
            rt_kprintf("Avg latency: %u cycles\n", tc->stats.avg_value);
            rt_kprintf("Total latency: %u cycles\n\n", tc->stats.total_latency);
        }
        else if (strcmp(tc->name, "throughput") == 0 && tc->stats.packets_sent > 0) {
            rt_kprintf("=== Throughput Test Results ===\n");
            rt_kprintf("Packets sent: %u\n", tc->stats.packets_sent);
            rt_kprintf("Packets received: %u\n", tc->stats.packets_received);
            rt_kprintf("Test duration: %u cycles\n", tc->stats.test_duration);
            rt_kprintf("Throughput: %u packets/cycle\n\n",
                       tc->stats.test_duration > 0 ? tc->stats.packets_received / tc->stats.test_duration : 0);
        }
        else if (strcmp(tc->name, "jitter") == 0 && tc->stats.measurements > 0) {
            rt_kprintf("=== Jitter Test Results ===\n");
            rt_kprintf("Measurements: %u\n", tc->stats.measurements);
            rt_kprintf("Expected interval: %u cycles\n", tc->stats.expected_interval);
            rt_kprintf("Min jitter: %u cycles\n", tc->stats.min_value);
            rt_kprintf("Max jitter: %u cycles\n", tc->stats.max_value);
            rt_kprintf("Avg jitter: %u cycles\n\n", tc->stats.avg_value);
        }
        else if (strcmp(tc->name, "idle_cpu") == 0 && tc->stats.measurements > 0) {
            rt_kprintf("=== Idle CPU Test Results ===\n");
            rt_kprintf("Test duration: %u cycles\n", tc->stats.total_cycles);
            rt_kprintf("Measurements: %u\n", tc->stats.measurements);
            rt_kprintf("Total idle count: %u\n", tc->stats.total_idle_count);
            rt_kprintf("Average idle per measurement: %u\n\n", tc->stats.avg_idle_per_measurement);
        }
        else if (strcmp(tc->name, "memory") == 0 && tc->stats.total_allocations > 0) {
            rt_kprintf("=== Memory Test Results ===\n");
            rt_kprintf("Total allocations: %u\n", tc->stats.total_allocations);
            rt_kprintf("Total frees: %u\n", tc->stats.total_frees);
            rt_kprintf("Total allocated: %u bytes\n", (rt_uint32_t)tc->stats.total_allocated_bytes);
            rt_kprintf("Total freed: %u bytes\n", (rt_uint32_t)tc->stats.total_freed_bytes);
            rt_kprintf("Max allocated: %u bytes\n", tc->stats.max_allocated_bytes);
            rt_kprintf("Allocation failures: %u\n\n", tc->stats.allocation_failures);
        }
        else {
            /* Generic report for other tests */
            rt_kprintf("=== %s Test Results ===\n", tc->name);
            rt_kprintf("Duration: %u ms\n", ms);
            rt_kprintf("Iterations: %u\n", tc->iterations);
            rt_kprintf("Result code: %d\n\n", tc->result_code);
        }
    }

    rt_kprintf("=== Summary ===\n");
    rt_kprintf("Name       Dur(ms)  Iter  Ret\n");
    for (rt_int32_t i = 0; i < s_test_count; i++)
    {
        perf_test_case_t *tc = s_test_registry[i];
        rt_tick_t dt = tc->end_tick - tc->start_tick;
        uint32_t ms = (uint32_t)(dt * 1000 / RT_TICK_PER_SECOND);
        rt_kprintf("%-10s %-8u %-5u %d\n",
                   tc->name, ms, tc->iterations, tc->result_code);
    }
}
