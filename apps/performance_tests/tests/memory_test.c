/*============================================================================
* Product: Memory Performance Test for QPC-RT-Thread
* Integrated into performance_tests framework
* Based on original memory_test.c from backup_old_tests
============================================================================*/
#include "perf_test.h"
#include "qpc.h"

/*==========================================================================*/
/* Memory Test Data Structure */
/*==========================================================================*/
typedef struct {
    rt_uint32_t total_allocations;
    rt_uint32_t total_frees;
    rt_uint64_t total_allocated_bytes;
    rt_uint64_t total_freed_bytes;
    rt_uint32_t max_allocated_bytes;
    rt_uint32_t allocation_failures;
    rt_uint32_t current_allocated_bytes;
    rt_bool_t test_running;
    void *allocated_ptrs[200]; /* Track allocated pointers */
    rt_uint32_t allocated_sizes[200]; /* Track allocated sizes */
    rt_uint32_t ptr_count;
} memory_test_data_t;

static memory_test_data_t s_memory_data;

/*==========================================================================*/
/* Memory Test Functions */
/*==========================================================================*/
static rt_bool_t allocate_memory(rt_uint32_t size)
{
    if (s_memory_data.ptr_count >= 200) {
        /* Array full, need to free some memory first */
        return RT_FALSE;
    }

    void *ptr = rt_malloc(size);
    if (ptr) {
        s_memory_data.allocated_ptrs[s_memory_data.ptr_count] = ptr;
        s_memory_data.allocated_sizes[s_memory_data.ptr_count] = size;
        s_memory_data.ptr_count++;

        s_memory_data.total_allocations++;
        s_memory_data.total_allocated_bytes += size;
        s_memory_data.current_allocated_bytes += size;

        if (s_memory_data.current_allocated_bytes > s_memory_data.max_allocated_bytes) {
            s_memory_data.max_allocated_bytes = s_memory_data.current_allocated_bytes;
        }

        return RT_TRUE;
    } else {
        s_memory_data.allocation_failures++;
        return RT_FALSE;
    }
}

static rt_bool_t free_memory(rt_uint32_t index)
{
    if (index >= s_memory_data.ptr_count || !s_memory_data.allocated_ptrs[index]) {
        return RT_FALSE;
    }

    rt_uint32_t size = s_memory_data.allocated_sizes[index];
    rt_free(s_memory_data.allocated_ptrs[index]);

    s_memory_data.allocated_ptrs[index] = RT_NULL;
    s_memory_data.allocated_sizes[index] = 0;

    s_memory_data.total_frees++;
    s_memory_data.total_freed_bytes += size;
    s_memory_data.current_allocated_bytes -= size;

    return RT_TRUE;
}

static void cleanup_all_memory(void)
{
    for (rt_uint32_t i = 0; i < s_memory_data.ptr_count; i++) {
        if (s_memory_data.allocated_ptrs[i]) {
            free_memory(i);
        }
    }
    s_memory_data.ptr_count = 0;
}

/*==========================================================================*/
/* Test Implementation Functions */
/*==========================================================================*/
static int memory_test_init(perf_test_case_t *tc)
{
    /* Initialize test data */
    rt_memset(&s_memory_data, 0, sizeof(s_memory_data));
    s_memory_data.test_running = RT_TRUE;

    /* Initialize statistics */
    tc->stats.total_allocations = 0;
    tc->stats.total_frees = 0;
    tc->stats.total_allocated_bytes = 0;
    tc->stats.total_freed_bytes = 0;
    tc->stats.max_allocated_bytes = 0;
    tc->stats.allocation_failures = 0;

    tc->user_data = &s_memory_data;
    tc->iterations = 0;

    rt_kprintf("[Memory Test] Initialized\n");

    return 0;
}

static int memory_test_run(perf_test_case_t *tc)
{
    memory_test_data_t *data = (memory_test_data_t *)tc->user_data;
    rt_uint32_t target_allocations = 200;
    rt_uint32_t allocation_sizes[] = {64, 128, 256, 512, 1024, 2048};
    rt_uint32_t num_sizes = sizeof(allocation_sizes) / sizeof(allocation_sizes[0]);

    rt_kprintf("[Memory Test] Starting memory allocation test...\n");

    /* Phase 1: Allocate memory blocks */
    for (rt_uint32_t i = 0; i < target_allocations && data->test_running; i++) {
        rt_uint32_t size = allocation_sizes[i % num_sizes];

        if (!allocate_memory(size)) {
            rt_kprintf("[Memory Test] Allocation failed for size %u\n", size);
        }

        tc->iterations++;

        /* Small delay to prevent overwhelming the system */
        if (i % 10 == 0) {
            rt_thread_mdelay(1);
        }
    }

    rt_kprintf("[Memory Test] Allocated %u blocks, max memory: %u bytes\n",
               data->total_allocations, data->max_allocated_bytes);

    /* Phase 2: Free some memory blocks (every other block) */
    rt_uint32_t free_count = 0;
    for (rt_uint32_t i = 0; i < data->ptr_count && data->test_running; i += 2) {
        if (free_memory(i)) {
            free_count++;
        }
        tc->iterations++;
    }

    rt_kprintf("[Memory Test] Freed %u blocks\n", free_count);

    /* Phase 3: Allocate more memory to fill gaps */
    for (rt_uint32_t i = 0; i < 20 && data->test_running; i++) {
        rt_uint32_t size = allocation_sizes[i % num_sizes];
        allocate_memory(size);
        tc->iterations++;

        rt_thread_mdelay(1);
    }

    /* Update final statistics */
    tc->stats.total_allocations = data->total_allocations;
    tc->stats.total_frees = data->total_frees;
    tc->stats.total_allocated_bytes = data->total_allocated_bytes;
    tc->stats.total_freed_bytes = data->total_freed_bytes;
    tc->stats.max_allocated_bytes = data->max_allocated_bytes;
    tc->stats.allocation_failures = data->allocation_failures;

    rt_kprintf("[Memory Test] Completed - Allocs: %u, Frees: %u, Failures: %u\n",
               data->total_allocations, data->total_frees, data->allocation_failures);

    return 0;
}

static int memory_test_stop(perf_test_case_t *tc)
{
    memory_test_data_t *data = (memory_test_data_t *)tc->user_data;
    if (data) {
        data->test_running = RT_FALSE;

        /* Clean up all allocated memory */
        cleanup_all_memory();
    }

    rt_kprintf("[Memory Test] Stopped and cleaned up\n");
    return 0;
}

/* Register the test case */
PERF_TEST_REG(memory, memory_test_init, memory_test_run, memory_test_stop);
