#include "perf_test.h"
#include <rtthread.h>

/* External initialization function for the performance framework */
extern void PerformanceFramework_init(void);

/* Global registry and count for test cases (from perf_test_core.c) */
extern perf_test_case_t* s_test_registry[];
extern rt_int32_t s_test_count;

/*
 * Main entry point for performance test application.
 * Initializes the framework and runs all registered test cases sequentially.
 * Waits for each test to finish before starting the next.
 * Prints a summary report at the end.
 */
int main(void)
{
    PerformanceFramework_init(); /* Initialize framework and BSP */

    /* Start and wait for all registered test cases */
    for (rt_int32_t i = 0; i < s_test_count; ++i) {
        perf_test_case_t *tc = s_test_registry[i];
        perf_test_start(tc->name);
        /* Wait for test thread to finish (STATE_FINISHED = 3) */
        while (tc->state != STATE_FINISHED) {
            rt_thread_mdelay(100);
        }
    }

    perf_test_report(); /* Print summary report */
    return 0;
}
