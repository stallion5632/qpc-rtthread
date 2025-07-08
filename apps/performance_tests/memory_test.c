/*============================================================================
* Product: Memory Performance Test
* Last updated for version 7.2.0
* Last updated on  2024-07-08
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2024 Quantum Leaps, LLC. All rights reserved.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <www.gnu.org/licenses/>.
*
* Contact information:
* <www.state-machine.com/licensing>
* <info@state-machine.com>
============================================================================*/
#include "perf_common.h"

Q_DEFINE_THIS_FILE

/*==========================================================================*/
/* Memory Test Active Object */
/*==========================================================================*/

typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    uint32_t alloc_count;
    uint32_t free_count;
    uint32_t total_allocated;
    uint32_t total_freed;
    uint32_t max_allocated;
    uint32_t current_allocated;
    uint32_t allocation_failures;
    uint32_t test_cycle;
} MemoryAO;

static MemoryAO l_memoryAO;

/* Memory allocation sizes to test */
static const uint32_t test_sizes[] = {
    32, 64, 128, 256, 512, 1024, 2048, 4096
};
static const uint32_t num_test_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

/* Memory allocation tracking */
#define MAX_ALLOCS 100
static struct {
    void *ptr;
    uint32_t size;
    uint32_t timestamp;
} alloc_tracker[MAX_ALLOCS];
static uint32_t alloc_tracker_index = 0;

/* State function declarations */
static QState MemoryAO_initial(MemoryAO * const me, QEvt const * const e);
static QState MemoryAO_idle(MemoryAO * const me, QEvt const * const e);
static QState MemoryAO_testing(MemoryAO * const me, QEvt const * const e);

/* Memory test thread */
static void memory_test_thread_func(void *parameter);
static rt_thread_t memory_test_thread = RT_NULL;

/*==========================================================================*/
/* Active Object Constructor */
/*==========================================================================*/

static void MemoryAO_ctor(void) {
    MemoryAO *me = &l_memoryAO;
    
    QActive_ctor(&me->super, Q_STATE_CAST(&MemoryAO_initial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, MEMORY_TIMEOUT_SIG, 0U);
    
    me->alloc_count = 0;
    me->free_count = 0;
    me->total_allocated = 0;
    me->total_freed = 0;
    me->max_allocated = 0;
    me->current_allocated = 0;
    me->allocation_failures = 0;
    me->test_cycle = 0;
}

/*==========================================================================*/
/* Memory Management Helper Functions */
/*==========================================================================*/

static void add_to_tracker(void *ptr, uint32_t size) {
    if (alloc_tracker_index < MAX_ALLOCS) {
        alloc_tracker[alloc_tracker_index].ptr = ptr;
        alloc_tracker[alloc_tracker_index].size = size;
        alloc_tracker[alloc_tracker_index].timestamp = PerfCommon_getDWTCycles();
        alloc_tracker_index++;
    }
}

static uint32_t remove_from_tracker(void *ptr) {
    for (uint32_t i = 0; i < alloc_tracker_index; i++) {
        if (alloc_tracker[i].ptr == ptr) {
            uint32_t size = alloc_tracker[i].size;
            
            /* Move last entry to this position */
            if (i < alloc_tracker_index - 1) {
                alloc_tracker[i] = alloc_tracker[alloc_tracker_index - 1];
            }
            alloc_tracker_index--;
            
            return size;
        }
    }
    return 0;
}

static void free_all_tracked(void) {
    for (uint32_t i = 0; i < alloc_tracker_index; i++) {
        if (alloc_tracker[i].ptr != RT_NULL) {
            PerfCommon_free(alloc_tracker[i].ptr);
            alloc_tracker[i].ptr = RT_NULL;
        }
    }
    alloc_tracker_index = 0;
}

/*==========================================================================*/
/* Memory Test Thread Function */
/*==========================================================================*/

static void memory_test_thread_func(void *parameter) {
    (void)parameter;
    
    uint32_t cycle = 0;
    
    while (!g_stopLoadThreads) {
        /* Allocate memory of various sizes */
        for (uint32_t i = 0; i < num_test_sizes; i++) {
            uint32_t size = test_sizes[i];
            
            MemoryEvt *alloc_evt = Q_NEW(MemoryEvt, MEMORY_ALLOC_SIG);
            alloc_evt->timestamp = PerfCommon_getDWTCycles();
            alloc_evt->alloc_size = size;
            alloc_evt->ptr = RT_NULL;
            
            QACTIVE_POST(&l_memoryAO.super, &alloc_evt->super, &l_memoryAO);
            
            /* Small delay between allocations */
            rt_thread_mdelay(10);
            
            if (g_stopLoadThreads) break;
        }
        
        /* Occasionally free some memory */
        if (cycle % 3 == 0) {
            MemoryEvt *free_evt = Q_NEW(MemoryEvt, MEMORY_FREE_SIG);
            free_evt->timestamp = PerfCommon_getDWTCycles();
            free_evt->alloc_size = 0;
            free_evt->ptr = RT_NULL;
            
            QACTIVE_POST(&l_memoryAO.super, &free_evt->super, &l_memoryAO);
        }
        
        cycle++;
        rt_thread_mdelay(100);
    }
    
    rt_kprintf("Memory test thread exiting\n");
}

/*==========================================================================*/
/* State Functions */
/*==========================================================================*/

static QState MemoryAO_initial(MemoryAO * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            /* Subscribe to memory test signals */
            QActive_subscribe(&me->super, MEMORY_START_SIG);
            QActive_subscribe(&me->super, MEMORY_STOP_SIG);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&MemoryAO_idle);
        }
        default: {
            return Q_SUPER(&QHsm_top);
        }
    }
}

static QState MemoryAO_idle(MemoryAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Memory Test: Idle state\n");
            status = Q_HANDLED();
            break;
        }
        
        case Q_EXIT_SIG: {
            status = Q_HANDLED();
            break;
        }
        
        case Q_INIT_SIG: {
            status = Q_HANDLED();
            break;
        }
        
        case Q_EMPTY_SIG: {
            status = Q_HANDLED();
            break;
        }
        
        case MEMORY_START_SIG: {
            rt_kprintf("Memory Test: Starting memory performance test\n");
            
            /* Reset measurement counters */
            me->alloc_count = 0;
            me->free_count = 0;
            me->total_allocated = 0;
            me->total_freed = 0;
            me->max_allocated = 0;
            me->current_allocated = 0;
            me->allocation_failures = 0;
            me->test_cycle = 0;
            g_memory_measurements = 0;
            g_stopLoadThreads = RT_FALSE;
            
            /* Clear allocation tracker */
            alloc_tracker_index = 0;
            
            /* Reset and start DWT cycle counter */
            PerfCommon_resetDWT();
            
            /* Arm timeout timer (10 seconds) */
            QTimeEvt_armX(&me->timeEvt, 10 * 100, 0); /* 10 seconds */
            
            /* Create memory test thread with smaller stack for embedded */
            memory_test_thread = rt_thread_create("mem_test",
                                                 memory_test_thread_func,
                                                 RT_NULL,
                                                 1024,  /* Reduced from 2048 */
                                                 LOAD_THREAD_PRIO,
                                                 20);
            if (memory_test_thread != RT_NULL) {
                rt_thread_startup(memory_test_thread);
            }
            
            status = Q_TRAN(&MemoryAO_testing);
            break;
        }
        
        case MEMORY_STOP_SIG: {
            rt_kprintf("Memory Test: Stopping\n");
            status = Q_HANDLED();
            break;
        }
        
        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }
    
    return status;
}

static QState MemoryAO_testing(MemoryAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            rt_kprintf("Memory Test: Testing state\n");
            status = Q_HANDLED();
            break;
        }
        
        case Q_EXIT_SIG: {
            QTimeEvt_disarm(&me->timeEvt);
            g_stopLoadThreads = RT_TRUE;
            
            /* Free all tracked allocations */
            free_all_tracked();
            
            status = Q_HANDLED();
            break;
        }
        
        case Q_INIT_SIG: {
            status = Q_HANDLED();
            break;
        }
        
        case Q_EMPTY_SIG: {
            status = Q_HANDLED();
            break;
        }
        
        case MEMORY_ALLOC_SIG: {
            MemoryEvt const *evt = (MemoryEvt const *)e;
            
            /* Attempt to allocate memory */
            void *ptr = PerfCommon_malloc(evt->alloc_size);
            
            if (ptr != RT_NULL) {
                /* Successful allocation */
                me->alloc_count++;
                me->total_allocated += evt->alloc_size;
                me->current_allocated += evt->alloc_size;
                
                if (me->current_allocated > me->max_allocated) {
                    me->max_allocated = me->current_allocated;
                }
                
                /* Add to tracker */
                add_to_tracker(ptr, evt->alloc_size);
                
                /* Write to memory to ensure it's accessible */
                uint8_t *byte_ptr = (uint8_t *)ptr;
                for (uint32_t i = 0; i < evt->alloc_size; i++) {
                    byte_ptr[i] = (uint8_t)(i & 0xFF);
                }
                
                if (me->alloc_count % 10 == 0) {
                    rt_kprintf("Memory alloc %u: size=%u, current=%u\n",
                              me->alloc_count, evt->alloc_size, me->current_allocated);
                }
            } else {
                /* Allocation failed */
                me->allocation_failures++;
                rt_kprintf("Memory allocation failed: size=%u\n", evt->alloc_size);
            }
            
            g_memory_measurements++;
            status = Q_HANDLED();
            break;
        }
        
        case MEMORY_FREE_SIG: {
            /* Free a random allocation */
            if (alloc_tracker_index > 0) {
                uint32_t index = me->test_cycle % alloc_tracker_index;
                void *ptr = alloc_tracker[index].ptr;
                
                if (ptr != RT_NULL) {
                    uint32_t size = remove_from_tracker(ptr);
                    PerfCommon_free(ptr);
                    
                    me->free_count++;
                    me->total_freed += size;
                    me->current_allocated -= size;
                    
                    rt_kprintf("Memory free %u: size=%u, current=%u\n",
                              me->free_count, size, me->current_allocated);
                }
            }
            
            me->test_cycle++;
            status = Q_HANDLED();
            break;
        }
        
        case MEMORY_MEASURE_SIG: {
            MemoryEvt const *evt = (MemoryEvt const *)e;
            
            /* Process memory measurement */
            rt_kprintf("Memory measurement: alloc_size=%u, ptr=%p\n",
                      evt->alloc_size, evt->ptr);
            
            status = Q_HANDLED();
            break;
        }
        
        case MEMORY_TIMEOUT_SIG: {
            rt_kprintf("Memory Test: Timeout reached\n");
            
            /* Set stop flag for memory test thread */
            g_stopLoadThreads = RT_TRUE;
            
            /* Wait for thread to stop and then delete it */
            PerfCommon_waitForThreads();
            if (memory_test_thread != RT_NULL) {
                rt_thread_delete(memory_test_thread);
                memory_test_thread = RT_NULL;
            }
            
            /* Free all tracked allocations */
            free_all_tracked();
            
            /* Calculate and print results */
            rt_kprintf("=== Memory Test Results ===\n");
            rt_kprintf("Total allocations: %u\n", me->alloc_count);
            rt_kprintf("Total frees: %u\n", me->free_count);
            rt_kprintf("Total allocated: %u bytes\n", me->total_allocated);
            rt_kprintf("Total freed: %u bytes\n", me->total_freed);
            rt_kprintf("Max allocated: %u bytes\n", me->max_allocated);
            rt_kprintf("Current allocated: %u bytes\n", me->current_allocated);
            rt_kprintf("Allocation failures: %u\n", me->allocation_failures);
            rt_kprintf("Memory measurements: %u\n", g_memory_measurements);
            
            status = Q_TRAN(&MemoryAO_idle);
            break;
        }
        
        case MEMORY_STOP_SIG: {
            rt_kprintf("Memory Test: Stopping test\n");
            QTimeEvt_disarm(&me->timeEvt);
            
            g_stopLoadThreads = RT_TRUE;
            
            /* Wait for thread to stop and then delete it */
            PerfCommon_waitForThreads();
            if (memory_test_thread != RT_NULL) {
                rt_thread_delete(memory_test_thread);
                memory_test_thread = RT_NULL;
            }
            
            /* Free all tracked allocations */
            free_all_tracked();
            
            status = Q_TRAN(&MemoryAO_idle);
            break;
        }
        
        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }
    
    return status;
}

/*==========================================================================*/
/* Memory Test Static Variables - Persistent for RT-Thread integration */
/*==========================================================================*/

static QEvt const *memory_queueSto[15];  /* Reduced queue size */
static uint8_t memory_stack[1024];       /* Reduced stack size for embedded */
static rt_bool_t memory_test_running = RT_FALSE;

/*==========================================================================*/
/* Memory Test Public Functions */
/*==========================================================================*/

void MemoryTest_start(void) {
    /* Prevent multiple simultaneous test instances */
    if (memory_test_running) {
        rt_kprintf("Memory test already running\n");
        return;
    }
    
    /* Initialize common performance test infrastructure */
    PerfCommon_initTest();
    
    /* Initialize only the memory event pool */
    PerfCommon_initMemoryPool();
    
    /* Initialize QF if not already done - safe to call multiple times in RT-Thread */
    QF_init();
    
    /* Construct the memory AO */
    MemoryAO_ctor();
    
    /* Start the memory AO */
    QACTIVE_START(&l_memoryAO.super,
                  MEMORY_AO_PRIO,
                  memory_queueSto, Q_DIM(memory_queueSto),
                  memory_stack, sizeof(memory_stack),
                  (void *)0);
    
    /* Initialize QF framework (returns immediately in RT-Thread) */
    QF_run();
    
    /* Mark test as running */
    memory_test_running = RT_TRUE;
    
    /* Send start signal */
    QACTIVE_POST(&l_memoryAO.super, Q_NEW(QEvt, MEMORY_START_SIG), &l_memoryAO);
    
    rt_kprintf("Memory test started successfully\n");
}

void MemoryTest_stop(void) {
    if (!memory_test_running) {
        rt_kprintf("Memory test not running\n");
        return;
    }
    
    /* Send stop signal */
    QACTIVE_POST(&l_memoryAO.super, Q_NEW(QEvt, MEMORY_STOP_SIG), &l_memoryAO);
    
    /* Give time for stop signal to be processed */
    rt_thread_mdelay(200);
    
    /* Unsubscribe from signals to prevent lingering subscriptions */
    QActive_unsubscribe(&l_memoryAO.super, MEMORY_START_SIG);
    QActive_unsubscribe(&l_memoryAO.super, MEMORY_STOP_SIG);
    
    /* Mark test as stopped */
    memory_test_running = RT_FALSE;
    
    /* Cleanup common infrastructure */
    PerfCommon_cleanupTest();
    
    /* Print final results */
    PerfCommon_printResults("Memory", g_memory_measurements);
    
    rt_kprintf("Memory test stopped successfully\n");
}

/* RT-Thread MSH command exports */
#ifdef RT_USING_FINSH
#include <finsh.h>
MSH_CMD_EXPORT(MemoryTest_start, start memory performance test);
MSH_CMD_EXPORT(MemoryTest_stop, stop memory performance test);
#endif