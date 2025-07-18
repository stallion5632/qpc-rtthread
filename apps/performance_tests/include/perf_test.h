#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>

/* Performance Test Statistics Structure */
typedef struct {
    /* Common statistics */
    rt_uint32_t measurements;
    rt_uint32_t total_cycles;
    rt_uint32_t min_value;
    rt_uint32_t max_value;
    rt_uint32_t avg_value;

    /* Latency specific */
    rt_uint32_t total_latency;

    /* Throughput specific */
    rt_uint32_t packets_sent;
    rt_uint32_t packets_received;
    rt_uint32_t test_duration;

    /* Jitter specific */
    rt_uint32_t expected_interval;

    /* Memory specific */
    rt_uint32_t total_allocations;
    rt_uint32_t total_frees;
    rt_uint64_t total_allocated_bytes;
    rt_uint64_t total_freed_bytes;
    rt_uint32_t max_allocated_bytes;
    rt_uint32_t allocation_failures;

    /* Idle CPU specific */
    rt_uint32_t total_idle_count;
    rt_uint32_t avg_idle_per_measurement;
} perf_test_stats_t;

/* Global registry for test cases (for main.c auto test traversal) */
extern struct perf_test_case* s_test_registry[];
extern rt_int32_t s_test_count;
#ifdef __cplusplus
}
#endif
#ifndef PERF_TEST_H_
#define PERF_TEST_H_

#define STATE_IDLE      0
#define STATE_INITED    1
#define STATE_RUNNING   2
#define STATE_FINISHED  3

/* Forward declaration for test case structure */

typedef struct perf_test_case perf_test_case_t;

/* Function pointer type for test case functions (init, run, stop) */
typedef int (*perf_test_func_t)(perf_test_case_t *tc);

/*
 * Structure representing a performance test case.
 * All fields must be initialized before registration.
 * Note: user_data is for test-specific context, must be managed by test logic.
 */
typedef struct perf_test_case {
    const char      *name;              /*< Test case name (must be unique) */
    perf_test_func_t  init;             /*< Optional initialization function */
    perf_test_func_t  run;              /*< Required run function */
    perf_test_func_t  stop;             /*< Optional stop/cleanup function */
    rt_thread_t      thread;            /*< RT-Thread thread handle for test execution */
    rt_uint8_t       state;             /*< 0=IDLE,1=INITED,2=RUNNING,3=FINISHED */
    void            *user_data;         /*< User data pointer for test context */
    rt_tick_t        start_tick;        /*< Test start tick */
    rt_tick_t        end_tick;          /*< Test end tick */
    rt_uint32_t      iterations;        /*< Operation count for performance statistics */
    int              result_code;       /*< Return value from run() */
    perf_test_stats_t stats;            /*< Detailed test statistics */
} perf_test_case_t;

/*
 * Register a test case at runtime.
 * This function is called by the auto-registration macro.
 * test_case pointer must not be NULL.
 */
void perf_test_register(perf_test_case_t* test_case);

/*
 * Macro for static registration of a performance test case.
 * Usage: PERF_TEST_REG(name, init, run, stop)
 * Each test case must use this macro for auto-registration.
 */
#define PERF_TEST_REG(c_name, c_init, c_run, c_stop) \
    static perf_test_case_t c_name##_case = { \
        .name      = #c_name, /* You can change this to a more meaningful string if needed */ \
        .init      = c_init, \
        .run       = c_run, \
        .stop      = c_stop, \
        .thread    = RT_NULL, \
        .state     = 0U, \
        .user_data = RT_NULL, \
    }; \
    static int c_name##_register(void) { \
        /* Optionally set a more meaningful name here, e.g. c_name##_case.name = "counter active object"; */ \
        perf_test_register(&c_name##_case); \
        return 0; \
    } \
    INIT_APP_EXPORT(c_name##_register);

/*
 * List all registered test cases.
 * Prints available test cases and their states.
 */
void perf_test_list(void);

/*
 * Start a test case by name.
 * Returns 0 on success, negative on error.
 */
rt_int32_t  perf_test_start(const char *name);

/*
 * Stop a running test case by name.
 * Returns 0 on success, negative on error.
 */
rt_int32_t  perf_test_stop(const char *name);

/*
 * Restart a test case by name.
 * Returns 0 on success, negative on error.
 */
rt_int32_t  perf_test_restart(const char *name);

/*
 * Print a summary report of all test cases.
 */
void perf_test_report(void);

#endif /* PERF_TEST_H_ */
