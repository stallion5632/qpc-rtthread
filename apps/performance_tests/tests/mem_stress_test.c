#include "perf_test.h"
#include <rtthread.h>

/* Memory stress test data */
typedef struct
{
    void **allocated_blocks;   /* Array of pointers to allocated memory blocks */
    uint32_t block_count;      /* Current number of allocated blocks */
    uint32_t max_blocks;       /* Maximum number of blocks to allocate */
    uint32_t alloc_count;      /* Total allocation operations */
    uint32_t free_count;       /* Total free operations */
} mem_stress_data_t;

static mem_stress_data_t s_mem_data;

static int mem_stress_init(perf_test_case_t *tc)
{
    s_mem_data.max_blocks = 100;
    s_mem_data.allocated_blocks = rt_malloc(s_mem_data.max_blocks * sizeof(void *));
    s_mem_data.block_count = 0;
    s_mem_data.alloc_count = 0;
    s_mem_data.free_count = 0;

    return (s_mem_data.allocated_blocks != RT_NULL) ? 0 : -1;
}

static int mem_stress_run(perf_test_case_t *tc)
{
    uint32_t operations = 1000;

    for (uint32_t i = 0; i < operations; i++)
    {
        /* Randomly allocate or free memory */
        if ((i % 3 == 0) && (s_mem_data.block_count < s_mem_data.max_blocks))
        {
            /* Allocate memory */
            void *ptr = rt_malloc(64 + (i % 128));
            if (ptr)
            {
                s_mem_data.allocated_blocks[s_mem_data.block_count++] = ptr;
                s_mem_data.alloc_count++;
            }
        }
        else if (s_mem_data.block_count > 0)
        {
            /* Free memory */
            uint32_t idx = i % s_mem_data.block_count;
            rt_free(s_mem_data.allocated_blocks[idx]);
            s_mem_data.allocated_blocks[idx] = s_mem_data.allocated_blocks[--s_mem_data.block_count];
            s_mem_data.free_count++;
        }

        tc->iterations++;

        /* Small delay to prevent overwhelming the system */
        if (i % 50 == 0)
        {
            rt_thread_yield();
        }
    }

    return 0;
}

static int mem_stress_stop(perf_test_case_t *tc)
{
    /* Free all remaining allocated blocks */
    for (uint32_t i = 0; i < s_mem_data.block_count; i++)
    {
        rt_free(s_mem_data.allocated_blocks[i]);
    }

    /* Free the pointer array */
    rt_free(s_mem_data.allocated_blocks);

    rt_kprintf("[Memory Test] Allocations: %u, Frees: %u\n",
               s_mem_data.alloc_count, s_mem_data.free_count);

    return 0;
}

PERF_TEST_REG(mem_stress, mem_stress_init, mem_stress_run, mem_stress_stop);
