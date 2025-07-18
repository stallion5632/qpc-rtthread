#include "perf_test.h"

static int cpu_load_init(perf_test_case_t *tc)
{
    tc->iterations = 0;
    return 0;
}

static int cpu_load_run(perf_test_case_t *tc)
{
    for (int i = 0; i < 100000; ++i)
    {
        tc->iterations++;
    }
    return 0;
}

static int cpu_load_stop(perf_test_case_t *tc)
{
    return 0;
}

PERF_TEST_REG(cpu_load, cpu_load_init, cpu_load_run, cpu_load_stop);
