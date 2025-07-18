/*============================================================================
* Product: Performance Test Integration Validator
* This file helps validate that all tests are properly integrated
============================================================================*/
#include "perf_test.h"
#include <string.h>

/* Test validation functions */
static void validate_test_registration(void)
{
    rt_kprintf("=== Performance Test Integration Validation ===\n");

    /* Expected tests */
    const char *expected_tests[] = {
        "latency",
        "throughput",
        "jitter",
        "idle_cpu",
        "memory",
        "cpu_load",
        "counter_ao",
        "timer_ao",
        "mem_stress",
        "multithread"
    };

    rt_uint32_t expected_count = sizeof(expected_tests) / sizeof(expected_tests[0]);
    rt_uint32_t found_count = 0;

    rt_kprintf("Expected tests: %u\n", expected_count);
    rt_kprintf("Registered tests: %u\n", s_test_count);

    /* Check each expected test */
    for (rt_uint32_t i = 0; i < expected_count; i++) {
        rt_bool_t found = RT_FALSE;

        for (rt_int32_t j = 0; j < s_test_count; j++) {
            if (strcmp(s_test_registry[j]->name, expected_tests[i]) == 0) {
                found = RT_TRUE;
                found_count++;
                break;
            }
        }

        rt_kprintf("Test '%s': %s\n",
                   expected_tests[i],
                   found ? "FOUND" : "MISSING");
    }

    rt_kprintf("\nValidation Summary:\n");
    rt_kprintf("Found: %u/%u tests\n", found_count, expected_count);
    rt_kprintf("Status: %s\n",
               (found_count == expected_count) ? "PASS" : "FAIL");
}

/* MSH command for validation */
static int cmd_perf_validate(int argc, char **argv)
{
    validate_test_registration();
    return 0;
}
MSH_CMD_EXPORT(cmd_perf_validate, Validate performance test integration);

/* Quick test runner for validation */
static int cmd_perf_quick_test(int argc, char **argv)
{
    rt_kprintf("=== Quick Performance Test Run ===\n");

    const char *quick_tests[] = {"latency", "throughput", "jitter", "idle_cpu", "memory"};
    rt_uint32_t test_count = sizeof(quick_tests) / sizeof(quick_tests[0]);

    for (rt_uint32_t i = 0; i < test_count; i++) {
        rt_kprintf("\n--- Testing %s ---\n", quick_tests[i]);

        /* Start test */
        rt_int32_t result = perf_test_start(quick_tests[i]);
        if (result != 0) {
            rt_kprintf("Failed to start %s (error: %d)\n", quick_tests[i], result);
            continue;
        }

        /* Wait for completion */
        rt_thread_mdelay(2000); /* 2 second timeout */

        /* Stop test */
        perf_test_stop(quick_tests[i]);

        rt_kprintf("Test %s completed\n", quick_tests[i]);
    }

    rt_kprintf("\n=== Quick Test Summary ===\n");
    perf_test_report();

    return 0;
}
MSH_CMD_EXPORT(cmd_perf_quick_test, Run quick performance test validation);
