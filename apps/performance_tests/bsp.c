/*============================================================================
* Product: Board Support Package Implementation
* Last updated for version 7.2.0
* Last updated on  2024-12-19
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
#include "bsp.h"
#include "rt_integration.h"
#include <stdio.h>

Q_DEFINE_THIS_MODULE("bsp")
#define rt_hw_interrupt_enable(x)
#define rt_hw_interrupt_disable()

/*==========================================================================*/
/* Local definitions */
/*==========================================================================*/

/* DWT (Data Watchpoint and Trace) registers for performance monitoring */
#define DWT_CTRL        (*(volatile uint32_t*)0xE0001000UL)
#define DWT_CYCCNT      (*(volatile uint32_t*)0xE0001004UL)
#define DWT_DEMCR       (*(volatile uint32_t*)0xE000EDFCUL)

#define DWT_CTRL_CYCCNTENA_Pos      (0U)
#define DWT_CTRL_CYCCNTENA_Msk      (1UL << DWT_CTRL_CYCCNTENA_Pos)
#define DWT_DEMCR_TRCENA_Pos        (24U)
#define DWT_DEMCR_TRCENA_Msk        (1UL << DWT_DEMCR_TRCENA_Pos)

/* LED state tracking */
static rt_bool_t l_led_state = RT_FALSE;

/* Performance monitoring variables */
static rt_bool_t l_perf_initialized = RT_FALSE;

/* Critical section nesting counter */
static volatile uint32_t l_critical_nesting = 0U;

/*==========================================================================*/
/* System Initialization */
/*==========================================================================*/
void BSP_init(void) {
    rt_kprintf("[QPC] module: %s\n", Q_this_module_);
    /* Initialize performance monitoring */
    BSP_perfInit();

    /* Initialize LED (simulated) */
    BSP_ledInit();

    /* Note: RT-Thread system initialization is handled by the framework */
    rt_kprintf("BSP: Board Support Package initialized\n");
}

/*==========================================================================*/
/* Time and Timing Functions */
/*==========================================================================*/
uint32_t BSP_getTimestampMs(void) {
    /* Get current RT-Thread tick and convert to milliseconds */
    rt_tick_t tick = rt_tick_get();
    return (uint32_t)(tick * 1000U / RT_TICK_PER_SECOND);
}

uint32_t BSP_getTimestampUs(void) {
    /* Get current RT-Thread tick and convert to microseconds */
    rt_tick_t tick = rt_tick_get();
    return (uint32_t)(tick * 1000000U / RT_TICK_PER_SECOND);
}

void BSP_delayMs(uint32_t delay_ms) {
    rt_thread_mdelay(delay_ms);
}

/*==========================================================================*/
/* LED Control Functions */
/*==========================================================================*/
void BSP_ledInit(void) {
    l_led_state = RT_FALSE;
    /* Note: Actual LED initialization would be platform-specific */
}

void BSP_ledOn(void) {
    l_led_state = RT_TRUE;
    /* Note: Actual LED control would be platform-specific */
}

void BSP_ledOff(void) {
    l_led_state = RT_FALSE;
    /* Note: Actual LED control would be platform-specific */
}

void BSP_ledToggle(void) {
    l_led_state = !l_led_state;
    /* Note: Actual LED control would be platform-specific */
}

rt_bool_t BSP_ledGetState(void) {
    return l_led_state;
}

/*==========================================================================*/
/* System Information */
/*==========================================================================*/
uint32_t BSP_getCpuFreq(void) {
    /* Return system clock frequency - platform specific */
    /* For simulation, return a typical ARM Cortex-M frequency */
    return 72000000U; /* 72 MHz */
}

uint32_t BSP_getMemTotal(void) {
    /* Get total memory from RT-Thread */
    rt_size_t total, used, max_used;
    rt_memory_info(&total, &used, &max_used);
    return (uint32_t)total;
}

uint32_t BSP_getMemFree(void) {
    /* Get free memory from RT-Thread */
    rt_size_t total, used, max_used;
    rt_memory_info(&total, &used, &max_used);
    return (uint32_t)(total - used);
}

/*==========================================================================*/
/* Performance Monitoring */
/*==========================================================================*/
void BSP_perfInit(void) {
    rt_kprintf("[QPC] module: %s\n", Q_this_module_);
    if (!l_perf_initialized) {
        /* Enable trace in DEMCR */
        DWT_DEMCR |= DWT_DEMCR_TRCENA_Msk;

        /* Reset and enable cycle counter */
        DWT_CYCCNT = 0U;
        DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk;

        l_perf_initialized = RT_TRUE;
    }
}

uint32_t BSP_perfGetCycles(void) {
    if (l_perf_initialized) {
        return DWT_CYCCNT;
    } else {
        return 0U;
    }
}

void BSP_perfReset(void) {
    if (l_perf_initialized) {
        DWT_CYCCNT = 0U;
    }
}

/*==========================================================================*/
/* System Watchdog */
/*==========================================================================*/
void BSP_watchdogInit(void) {
    /* Platform-specific watchdog initialization */
    /* For RT-Thread, this might not be needed or could use RT-Thread watchdog */
}

void BSP_watchdogFeed(void) {
    /* Platform-specific watchdog feeding */
    /* For RT-Thread, this might not be needed or could use RT-Thread watchdog */
}

/*==========================================================================*/
/* Critical Section Support */
/*==========================================================================*/
void BSP_criticalSectionEnter(void) {
    rt_hw_interrupt_disable();
    ++l_critical_nesting;
}

void BSP_criticalSectionExit(void) {
    if (l_critical_nesting > 0U) {
        --l_critical_nesting;
        if (l_critical_nesting == 0U) {
            rt_hw_interrupt_enable(0);
        }
    }
}

/*==========================================================================*/
/* Error Handling */
/*==========================================================================*/
void BSP_errorHandler(const char *file, int line, const char *func) {
    /* Thread-safe error reporting */
    rt_kprintf("ASSERTION FAILED: %s:%d in %s\n", file, line, func);

    /* In a real system, you might want to:
     * 1. Log error to persistent storage
     * 2. Trigger system reset
     * 3. Enter safe mode
     * 4. Send error notification
     */

    /* For now, just loop indefinitely */
    for (;;) {
        BSP_ledToggle();
        BSP_delayMs(250U);
    }
}

/*==========================================================================*/
/* RT-Thread Specific BSP Functions */
/*==========================================================================*/
rt_size_t BSP_getThreadStackUsed(rt_thread_t thread) {
    if (thread == RT_NULL) {
        return 0U;
    }

    /* Calculate stack usage - this is platform/implementation specific */
    rt_uint8_t *stack_start = (rt_uint8_t *)thread->stack_addr;
    rt_uint8_t *stack_end = stack_start + thread->stack_size;
    rt_uint8_t *stack_ptr = (rt_uint8_t *)thread->sp;

    if (stack_ptr >= stack_start && stack_ptr <= stack_end) {
        return (rt_size_t)(stack_end - stack_ptr);
    } else {
        return 0U; /* Invalid stack pointer */
    }
}

rt_size_t BSP_getThreadStackFree(rt_thread_t thread) {
    if (thread == RT_NULL) {
        return 0U;
    }

    rt_size_t used = BSP_getThreadStackUsed(thread);
    if (used <= thread->stack_size) {
        return thread->stack_size - used;
    } else {
        return 0U; /* Stack overflow condition */
    }
}

uint8_t BSP_getCpuUsage(void) {
    /* Get CPU usage from RT-Thread if available */
    /* This requires RT-Thread's CPU usage module to be enabled */
#ifdef RT_USING_CPU_USAGE
    return (uint8_t)rt_cpu_usage_get();
#else
    /* Return a simulated value if CPU usage monitoring is not available */
    static uint8_t simulated_usage = 50U;
    simulated_usage = (simulated_usage + 1U) % 100U; /* Simple simulation */
    return simulated_usage;
#endif
}

uint32_t BSP_getIdleCount(void) {
    /* Get idle thread run count if available */
    /* This is implementation-specific and may not be available on all systems */
    static uint32_t simulated_idle = 0U;
    return ++simulated_idle; /* Simple simulation */
}

/*==========================================================================*/
/* Thread-safe Logging Support */
/*==========================================================================*/

/* Declare external mutex from app_main.c */
extern rt_mutex_t g_log_mutex;

void BSP_logLock(void) {
    if (g_log_mutex != RT_NULL) {
        rt_mutex_take(g_log_mutex, RT_WAITING_FOREVER);
    }
}

void BSP_logUnlock(void) {
    if (g_log_mutex != RT_NULL) {
        rt_mutex_release(g_log_mutex);
    }
}

/* ...existing code... */

/*==========================================================================*/
/* RT-Thread Integration Functions */
/*==========================================================================*/

/* Implement the log lock/unlock functions for rt_integration.h */
void log_lock(void) {
    BSP_logLock();
}

void log_unlock(void) {
    BSP_logUnlock();
}

#ifdef Q_SPY

/* QS-RT-Thread lock/unlock functions for thread-safe trace */
static void QS_onLock(void) {
    log_lock();
}

static void QS_onUnlock(void) {
    log_unlock();
}

bool QF_onStartup(void) {
    QS_init(NULL); /* Initialize QS */
    QS_onLock_fun = &QS_onLock;
    QS_onUnlock_fun = &QS_onUnlock;
    return true; /* Return true if startup successful */
}
#endif /* Q_SPY */
