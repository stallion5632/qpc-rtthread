/*============================================================================
* Product: Board Support Package Implementation for Performance Tests
* Last updated for version 7.2.0
* Last updated on  2024-07-13
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
#include <rtthread.h>

#ifndef rt_hw_interrupt_disable
#define rt_hw_interrupt_disable() (0)
#endif
#ifndef rt_hw_interrupt_enable
#define rt_hw_interrupt_enable(level) ((void)(level))
#endif

/*==========================================================================*/
/* Local Variables and State */
/*==========================================================================*/

/* BSP initialization state flag */
static rt_bool_t s_bsp_initialized = RT_FALSE;

/* DWT initialization state flag */
static rt_bool_t s_dwt_initialized = RT_FALSE;

/* System startup timestamp for uptime calculation */
static uint32_t s_system_start_timestamp = 0U;

/* Performance statistics counters */
static uint32_t s_context_switch_count = 0U;
static uint32_t s_interrupt_count = 0U;
static uint32_t s_max_interrupt_latency = 0U;

/* Timing validation reference values */
static uint32_t s_cycles_per_microsecond = 0U;
static uint32_t s_cycles_per_millisecond = 0U;

/*==========================================================================*/
/* BSP Initialization and Configuration Functions */
/*==========================================================================*/

void BSP_init(void)
{
    if (s_bsp_initialized) {
        rt_kprintf("BSP: Already initialized, skipping...\n");
        return;
    }

    rt_kprintf("BSP: Initializing Board Support Package...\n");

    /* Configure system clock parameters */
    BSP_configureSystemClock();

    /* Initialize high-resolution timing */
    BSP_initDWT();

    /* Record system startup timestamp */
    s_system_start_timestamp = BSP_getTimestamp();

    /* Calculate timing constants */
    s_cycles_per_microsecond = BSP_SYSTEM_CLOCK_HZ / 1000000U;
    s_cycles_per_millisecond = BSP_SYSTEM_CLOCK_HZ / 1000U;

    /* Initialize performance counters */
    s_context_switch_count = 0U;
    s_interrupt_count = 0U;
    s_max_interrupt_latency = 0U;

    /* Mark BSP as initialized */
    s_bsp_initialized = RT_TRUE;

    rt_kprintf("BSP: Initialization complete\n");
    rt_kprintf("BSP: System clock: %u Hz\n", BSP_SYSTEM_CLOCK_HZ);
    rt_kprintf("BSP: Cycles per microsecond: %u\n", s_cycles_per_microsecond);
    rt_kprintf("BSP: Cycles per millisecond: %u\n", s_cycles_per_millisecond);
}

void BSP_initDWT(void)
{
    if (s_dwt_initialized) {
        rt_kprintf("BSP: DWT already initialized\n");
        return;
    }

    rt_kprintf("BSP: Initializing DWT cycle counter...\n");

    /* Enable trace and debug features */
    BSP_DWT_DEMCR_REG |= BSP_DWT_DEMCR_TRCENA_MASK;

    /* Reset cycle counter to zero */
    BSP_DWT_CYCCNT_REG = 0U;

    /* Enable cycle counter */
    BSP_DWT_CTRL_REG |= BSP_DWT_CTRL_CYCCNTENA_MASK;

    /* Verify DWT is working */
    uint32_t test_start = BSP_DWT_CYCCNT_REG;
    /* Small delay to generate some cycles */
    for (volatile uint32_t i = 0U; i < 1000U; i++) {
        /* Busy wait loop */
    }
    uint32_t test_end = BSP_DWT_CYCCNT_REG;

    if (test_end > test_start) {
        s_dwt_initialized = RT_TRUE;
        rt_kprintf("BSP: DWT cycle counter initialized successfully\n");
        rt_kprintf("BSP: DWT test cycles: %u\n", test_end - test_start);
    } else {
        rt_kprintf("BSP: DWT cycle counter initialization failed\n");
        s_dwt_initialized = RT_FALSE;
    }
}

void BSP_deinit(void)
{
    if (!s_bsp_initialized) {
        return;
    }

    rt_kprintf("BSP: Deinitializing Board Support Package...\n");

    /* Disable DWT cycle counter */
    BSP_DWT_CTRL_REG &= ~BSP_DWT_CTRL_CYCCNTENA_MASK;

    /* Reset state flags */
    s_bsp_initialized = RT_FALSE;
    s_dwt_initialized = RT_FALSE;

    rt_kprintf("BSP: Deinitialization complete\n");
}

void BSP_configureSystemClock(void)
{
    /* System clock configuration is typically handled by RT-Thread startup */
    rt_kprintf("BSP: System clock configuration delegated to RT-Thread\n");
    rt_kprintf("BSP: Current tick frequency: %u Hz\n", RT_TICK_PER_SECOND);
}

rt_bool_t BSP_isInitialized(void)
{
    return s_bsp_initialized;
}

/*==========================================================================*/
/* High-Resolution Timing Functions */
/*==========================================================================*/

uint32_t BSP_getTimestamp(void)
{
    if (s_dwt_initialized) {
        return BSP_DWT_CYCCNT_REG;
    } else {
        /* Fallback to RT-Thread tick counter if DWT not available */
        return rt_tick_get() * BSP_CYCLES_PER_TICK;
    }
}

uint32_t BSP_getTimestampMicroseconds(void)
{
    uint32_t cycles = BSP_getTimestamp();
    return BSP_cyclesToMicroseconds(cycles);
}

uint32_t BSP_getTimestampMilliseconds(void)
{
    uint32_t cycles = BSP_getTimestamp();
    return BSP_cyclesToMilliseconds(cycles);
}

void BSP_resetTimestamp(void)
{
    if (s_dwt_initialized) {
        BSP_DWT_CYCCNT_REG = 0U;
        rt_kprintf("BSP: DWT cycle counter reset to zero\n");
    } else {
        rt_kprintf("BSP: DWT not initialized, cannot reset timestamp\n");
    }
}

uint32_t BSP_cyclesToMicroseconds(uint32_t cycles)
{
    if (s_cycles_per_microsecond > 0U) {
        return cycles / s_cycles_per_microsecond;
    } else {
        return cycles / (BSP_SYSTEM_CLOCK_HZ / 1000000U);
    }
}

uint32_t BSP_cyclesToMilliseconds(uint32_t cycles)
{
    if (s_cycles_per_millisecond > 0U) {
        return cycles / s_cycles_per_millisecond;
    } else {
        return cycles / (BSP_SYSTEM_CLOCK_HZ / 1000U);
    }
}

uint32_t BSP_microsecondsToCycles(uint32_t microseconds)
{
    if (s_cycles_per_microsecond > 0U) {
        return microseconds * s_cycles_per_microsecond;
    } else {
        return microseconds * (BSP_SYSTEM_CLOCK_HZ / 1000000U);
    }
}

uint32_t BSP_getCyclesPerMs(void)
{
    return s_cycles_per_millisecond;
}

uint32_t BSP_getTicksPerMs(void)
{
    return RT_TICK_PER_SECOND / 1000U;
}

/*==========================================================================*/
/* Performance Measurement Functions */
/*==========================================================================*/

void BSP_startPerfMeasurement(BSP_PerfMeasurement *measurement)
{
    if (measurement != (BSP_PerfMeasurement *)0) {
        measurement->start_timestamp = BSP_getTimestamp();
        measurement->end_timestamp = 0U;
        measurement->elapsed_cycles = 0U;
        measurement->elapsed_microseconds = 0U;
        measurement->is_active = RT_TRUE;
    }
}

void BSP_stopPerfMeasurement(BSP_PerfMeasurement *measurement)
{
    if (measurement != (BSP_PerfMeasurement *)0 && measurement->is_active) {
        measurement->end_timestamp = BSP_getTimestamp();
        measurement->elapsed_cycles = BSP_calculateTimeDifference(measurement->start_timestamp,
                                                                  measurement->end_timestamp);
        measurement->elapsed_microseconds = BSP_cyclesToMicroseconds(measurement->elapsed_cycles);
        measurement->is_active = RT_FALSE;
    }
}

uint32_t BSP_getMeasurementCycles(BSP_PerfMeasurement const *measurement)
{
    if (measurement != (BSP_PerfMeasurement *)0) {
        return measurement->elapsed_cycles;
    }
    return 0U;
}

uint32_t BSP_getMeasurementMicroseconds(BSP_PerfMeasurement const *measurement)
{
    if (measurement != (BSP_PerfMeasurement *)0) {
        return measurement->elapsed_microseconds;
    }
    return 0U;
}

void BSP_resetPerfMeasurement(BSP_PerfMeasurement *measurement)
{
    if (measurement != (BSP_PerfMeasurement *)0) {
        measurement->start_timestamp = 0U;
        measurement->end_timestamp = 0U;
        measurement->elapsed_cycles = 0U;
        measurement->elapsed_microseconds = 0U;
        measurement->is_active = RT_FALSE;
    }
}

uint32_t BSP_calculateTimeDifference(uint32_t start_time, uint32_t end_time)
{
    /* Handle timer wraparound case for 32-bit counter */
    if (end_time >= start_time) {
        return (end_time - start_time);
    } else {
        /* Counter has wrapped around */
        return (BSP_MAX_TIMER_VALUE - start_time + end_time + 1U);
    }
}

/*==========================================================================*/
/* System Statistics and Monitoring Functions */
/*==========================================================================*/

void BSP_getSystemStats(BSP_SystemStats *stats)
{
    if (stats != (BSP_SystemStats *)0) {
        stats->cpu_utilization_percent = BSP_getCpuUtilization();
        stats->free_heap_bytes = BSP_getFreeHeapBytes();
        stats->total_heap_bytes = BSP_getTotalHeapBytes();
        stats->context_switches = s_context_switch_count;
        stats->interrupts_processed = s_interrupt_count;
        stats->max_interrupt_latency_cycles = s_max_interrupt_latency;
        stats->system_uptime_ms = BSP_getSystemUptimeMs();
    }
}

uint8_t BSP_getCpuUtilization(void)
{
    /* CPU utilization calculation would require more sophisticated monitoring */
    /* For now, return a placeholder value based on system load indicators */
    rt_thread_t current_thread = rt_thread_self();
    if (current_thread != RT_NULL) {
        /* Simple heuristic based on thread activity */
        return 25U; /* Placeholder: 25% utilization */
    }
    return 0U;
}

uint32_t BSP_getFreeHeapBytes(void)
{
    /* Use RT-Thread memory management functions */
    rt_size_t total, used, max_used;
    rt_memory_info(&total, &used, &max_used);
    return (total - used);
}

uint32_t BSP_getTotalHeapBytes(void)
{
    /* Use RT-Thread memory management functions */
    rt_size_t total, used, max_used;
    rt_memory_info(&total, &used, &max_used);
    return total;
}

uint32_t BSP_getSystemUptimeMs(void)
{
    if (s_bsp_initialized) {
        uint32_t current_timestamp = BSP_getTimestamp();
        uint32_t uptime_cycles = BSP_calculateTimeDifference(s_system_start_timestamp, current_timestamp);
        return BSP_cyclesToMilliseconds(uptime_cycles);
    }
    return 0U;
}

uint32_t BSP_getStackUsage(void)
{
    /* Get current thread stack usage */
    rt_thread_t current_thread = rt_thread_self();
    if (current_thread != RT_NULL) {
        /* Calculate stack usage (placeholder implementation) */
        return 512U; /* Placeholder value */
    }
    return 0U;
}

/*==========================================================================*/
/* Hardware Control Functions */
/*==========================================================================*/

rt_base_t BSP_enterCritical(void)
{
    return rt_hw_interrupt_disable();
}

void BSP_exitCritical(rt_base_t level)
{
    rt_hw_interrupt_enable(level);
}

void BSP_delayMicroseconds(uint32_t microseconds)
{
    if (s_dwt_initialized && microseconds > 0U) {
        uint32_t start_cycles = BSP_getTimestamp();
        uint32_t delay_cycles = BSP_microsecondsToCycles(microseconds);
        /* Busy wait until delay_cycles reached */
        while (BSP_calculateTimeDifference(start_cycles, BSP_getTimestamp()) < delay_cycles) {
            /* Busy wait loop */
        }
    } else {
        /* Fallback to RT-Thread delay */
        rt_thread_mdelay((microseconds + 999U) / 1000U); /* Round up to milliseconds */
    }
}

void BSP_delayMilliseconds(uint32_t milliseconds)
{
    if (milliseconds > 0U) {
        rt_thread_mdelay(milliseconds);
    }
}

void BSP_busyWaitCycles(uint32_t cycles)
{
    if (s_dwt_initialized && cycles > 0U) {
        uint32_t start_cycles = BSP_getTimestamp();

        /* Busy wait until specified cycles elapsed */
        while (BSP_calculateTimeDifference(start_cycles, BSP_getTimestamp()) < cycles) {
            /* Busy wait loop */
        }
    }
}

/*==========================================================================*/
/* Debug and Diagnostic Functions */
/*==========================================================================*/

void BSP_printStatus(void)
{
    rt_kprintf("=== BSP Status ===\n");
    rt_kprintf("BSP Initialized: %s\n", s_bsp_initialized ? "YES" : "NO");
    rt_kprintf("DWT Initialized: %s\n", s_dwt_initialized ? "YES" : "NO");
    rt_kprintf("System Clock: %u Hz\n", BSP_SYSTEM_CLOCK_HZ);
    rt_kprintf("Tick Frequency: %u Hz\n", RT_TICK_PER_SECOND);
    rt_kprintf("Cycles per µs: %u\n", s_cycles_per_microsecond);
    rt_kprintf("Cycles per ms: %u\n", s_cycles_per_millisecond);
    rt_kprintf("System Uptime: %u ms\n", BSP_getSystemUptimeMs());
    rt_kprintf("Current Timestamp: %u cycles\n", BSP_getTimestamp());
    rt_kprintf("==================\n");
}

void BSP_printSystemStats(void)
{
    BSP_SystemStats stats;
    BSP_getSystemStats(&stats);

    rt_kprintf("=== System Statistics ===\n");
    rt_kprintf("CPU Utilization: %u%%\n", stats.cpu_utilization_percent);
    rt_kprintf("Free Heap: %u bytes\n", stats.free_heap_bytes);
    rt_kprintf("Total Heap: %u bytes\n", stats.total_heap_bytes);
    rt_kprintf("Heap Usage: %.1f%%\n",
               (float)(stats.total_heap_bytes - stats.free_heap_bytes) * 100.0f / stats.total_heap_bytes);
    rt_kprintf("Context Switches: %u\n", stats.context_switches);
    rt_kprintf("Interrupts Processed: %u\n", stats.interrupts_processed);
    rt_kprintf("Max Interrupt Latency: %u cycles\n", stats.max_interrupt_latency_cycles);
    rt_kprintf("System Uptime: %u ms\n", stats.system_uptime_ms);
    rt_kprintf("=========================\n");
}

void BSP_printDWTStatus(void)
{
    rt_kprintf("=== DWT Status ===\n");
    rt_kprintf("DWT Initialized: %s\n", s_dwt_initialized ? "YES" : "NO");

    if (s_dwt_initialized) {
        rt_kprintf("DEMCR Register: 0x%08X\n", BSP_DWT_DEMCR_REG);
        rt_kprintf("CTRL Register: 0x%08X\n", BSP_DWT_CTRL_REG);
        rt_kprintf("Current Cycle Count: %u\n", BSP_DWT_CYCCNT_REG);
        rt_kprintf("Trace Enabled: %s\n",
                   (BSP_DWT_DEMCR_REG & BSP_DWT_DEMCR_TRCENA_MASK) ? "YES" : "NO");
        rt_kprintf("Cycle Counter Enabled: %s\n",
                   (BSP_DWT_CTRL_REG & BSP_DWT_CTRL_CYCCNTENA_MASK) ? "YES" : "NO");
    } else {
        rt_kprintf("DWT not initialized - high-resolution timing unavailable\n");
    }
    rt_kprintf("==================\n");
}

rt_bool_t BSP_validateTiming(void)
{
    if (!s_dwt_initialized) {
        rt_kprintf("BSP: Cannot validate timing - DWT not initialized\n");
        return RT_FALSE;
    }

    rt_kprintf("BSP: Running timing validation test...\n");

    /* Test 1: Measure known delay */
    BSP_PerfMeasurement measurement;
    BSP_startPerfMeasurement(&measurement);
    BSP_delayMicroseconds(1000U); /* 1ms delay */
    BSP_stopPerfMeasurement(&measurement);

    uint32_t measured_us = BSP_getMeasurementMicroseconds(&measurement);
    uint32_t expected_us = 1000U;
    uint32_t error_percent = (measured_us > expected_us) ?
                            ((measured_us - expected_us) * 100U / expected_us) :
                            ((expected_us - measured_us) * 100U / expected_us);

    rt_kprintf("BSP: Timing test results:\n");
    rt_kprintf("  Expected: %u µs\n", expected_us);
    rt_kprintf("  Measured: %u µs\n", measured_us);
    rt_kprintf("  Error: %u%%\n", error_percent);

    /* Consider timing valid if error is less than 10% */
    rt_bool_t timing_valid = (error_percent < 10U);
    rt_kprintf("  Timing Validation: %s\n", timing_valid ? "PASS" : "FAIL");

    return timing_valid;
}

rt_bool_t BSP_runSelfTest(void)
{
    rt_kprintf("=== BSP Self-Test ===\n");

    rt_bool_t all_tests_passed = RT_TRUE;

    /* Test 1: BSP Initialization */
    rt_kprintf("Test 1: BSP Initialization... ");
    if (s_bsp_initialized) {
        rt_kprintf("PASS\n");
    } else {
        rt_kprintf("FAIL\n");
        all_tests_passed = RT_FALSE;
    }

    /* Test 2: DWT Functionality */
    rt_kprintf("Test 2: DWT Functionality... ");
    if (s_dwt_initialized) {
        uint32_t start = BSP_getTimestamp();
        BSP_busyWaitCycles(1000U);
        uint32_t end = BSP_getTimestamp();
        if (end > start) {
            rt_kprintf("PASS\n");
        } else {
            rt_kprintf("FAIL (no cycle increment)\n");
            all_tests_passed = RT_FALSE;
        }
    } else {
        rt_kprintf("FAIL (not initialized)\n");
        all_tests_passed = RT_FALSE;
    }

    /* Test 3: Performance Measurement */
    rt_kprintf("Test 3: Performance Measurement... ");
    BSP_PerfMeasurement test_measurement;
    BSP_startPerfMeasurement(&test_measurement);
    BSP_busyWaitCycles(5000U);
    BSP_stopPerfMeasurement(&test_measurement);

    if (test_measurement.elapsed_cycles > 4000U && test_measurement.elapsed_cycles < 10000U) {
        rt_kprintf("PASS (%u cycles)\n", test_measurement.elapsed_cycles);
    } else {
        rt_kprintf("FAIL (%u cycles)\n", test_measurement.elapsed_cycles);
        all_tests_passed = RT_FALSE;
    }

    /* Test 4: Timing Conversion */
    rt_kprintf("Test 4: Timing Conversion... ");
    uint32_t test_cycles = s_cycles_per_millisecond;
    uint32_t converted_ms = BSP_cyclesToMilliseconds(test_cycles);
    if (converted_ms == 1U) {
        rt_kprintf("PASS\n");
    } else {
        rt_kprintf("FAIL (expected 1ms, got %ums)\n", converted_ms);
        all_tests_passed = RT_FALSE;
    }

    rt_kprintf("=== Self-Test %s ===\n", all_tests_passed ? "PASSED" : "FAILED");

    return all_tests_passed;
}

/*==========================================================================*/
/* MSH Shell Command Implementations */
/*==========================================================================*/

void bsp_status_cmd(void)
{
    BSP_printStatus();
}

void bsp_stats_cmd(void)
{
    BSP_printSystemStats();
}

void bsp_test_cmd(void)
{
    rt_kprintf("Running BSP self-test...\n");
    rt_bool_t result = BSP_runSelfTest();
    rt_kprintf("BSP self-test %s\n", result ? "PASSED" : "FAILED");
}

void bsp_dwt_cmd(void)
{
    BSP_printDWTStatus();
}

void bsp_timing_test_cmd(void)
{
    rt_kprintf("Running timing validation test...\n");
    rt_bool_t result = BSP_validateTiming();
    rt_kprintf("Timing validation %s\n", result ? "PASSED" : "FAILED");
}

void bsp_reset_cmd(void)
{
    rt_kprintf("Resetting BSP performance counters...\n");
    s_context_switch_count = 0U;
    s_interrupt_count = 0U;
    s_max_interrupt_latency = 0U;
    BSP_resetTimestamp();
    rt_kprintf("BSP performance counters reset\n");
}

/* Export MSH commands for interactive testing */
MSH_CMD_EXPORT(bsp_status_cmd, Show BSP initialization status);
MSH_CMD_EXPORT(bsp_stats_cmd, Show system performance statistics);
MSH_CMD_EXPORT(bsp_test_cmd, Run BSP self-test diagnostics);
MSH_CMD_EXPORT(bsp_dwt_cmd, Show DWT cycle counter status);
MSH_CMD_EXPORT(bsp_timing_test_cmd, Test timing accuracy validation);
MSH_CMD_EXPORT(bsp_reset_cmd, Reset BSP performance counters);
