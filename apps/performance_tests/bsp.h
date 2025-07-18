/*============================================================================
* Product: Board Support Package Header for Performance Tests
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
#ifndef BSP_H_
#define BSP_H_

#include <rtthread.h>
#include <stdint.h>

/*==========================================================================*/
/* BSP Configuration Constants */
/*==========================================================================*/

/* System clock and timing configuration */
#define BSP_SYSTEM_CLOCK_HZ     168000000U  /* System clock frequency in Hz */
#define BSP_TICKS_PER_SECOND    RT_TICK_PER_SECOND  /* RT-Thread tick frequency */
#define BSP_CYCLES_PER_TICK     (BSP_SYSTEM_CLOCK_HZ / BSP_TICKS_PER_SECOND)  /* CPU cycles per tick */

/* Performance measurement configuration */
#define BSP_TIMESTAMP_ENABLED   1U          /* Enable high-resolution timestamps */
#define BSP_DWT_ENABLED         1U          /* Enable DWT cycle counter */
#define BSP_PROFILING_ENABLED   1U          /* Enable performance profiling */

/* Hardware abstraction constants */
#define BSP_MAX_TIMER_VALUE     0xFFFFFFFFU /* Maximum timer counter value */
#define BSP_TIMER_RESOLUTION_NS 6U          /* Timer resolution in nanoseconds */

/*==========================================================================*/
/* DWT (Data Watchpoint and Trace) Registers for High-Resolution Timing */
/*==========================================================================*/

/* DWT register addresses for cycle counting */
#define BSP_DWT_CTRL_REG        (*(volatile uint32_t*)0xE0001000)  /* DWT Control Register */
#define BSP_DWT_CYCCNT_REG      (*(volatile uint32_t*)0xE0001004)  /* DWT Cycle Count Register */
#define BSP_DWT_DEMCR_REG       (*(volatile uint32_t*)0xE000EDFC)  /* Debug Exception Monitor Control */

/* DWT control register bit definitions */
#define BSP_DWT_CTRL_CYCCNTENA_POS      0U                              /* Cycle counter enable bit position */
#define BSP_DWT_CTRL_CYCCNTENA_MASK     (1UL << BSP_DWT_CTRL_CYCCNTENA_POS)  /* Cycle counter enable mask */
#define BSP_DWT_DEMCR_TRCENA_POS        24U                             /* Trace enable bit position */
#define BSP_DWT_DEMCR_TRCENA_MASK       (1UL << BSP_DWT_DEMCR_TRCENA_POS)    /* Trace enable mask */

/*==========================================================================*/
/* BSP Hardware Abstraction Types */
/*==========================================================================*/

/* Performance measurement context structure */
typedef struct {
    uint32_t start_timestamp;               /* Measurement start timestamp */
    uint32_t end_timestamp;                 /* Measurement end timestamp */
    uint32_t elapsed_cycles;                /* Elapsed CPU cycles */
    uint32_t elapsed_microseconds;          /* Elapsed time in microseconds */
    rt_bool_t is_active;                    /* Measurement active flag */
} BSP_PerfMeasurement;

/* System performance statistics structure */
typedef struct {
    uint32_t cpu_utilization_percent;       /* CPU utilization percentage */
    uint32_t free_heap_bytes;               /* Free heap memory in bytes */
    uint32_t total_heap_bytes;              /* Total heap memory in bytes */
    uint32_t context_switches;              /* Number of context switches */
    uint32_t interrupts_processed;          /* Number of interrupts processed */
    uint32_t max_interrupt_latency_cycles;  /* Maximum interrupt latency */
    uint32_t system_uptime_ms;              /* System uptime in milliseconds */
} BSP_SystemStats;

/*==========================================================================*/
/* BSP Initialization and Configuration Functions */
/*==========================================================================*/

/* Initialize Board Support Package and hardware peripherals */
void BSP_init(void);

/* Initialize DWT cycle counter for high-resolution timing */
void BSP_initDWT(void);

/* Deinitialize BSP and cleanup resources */
void BSP_deinit(void);

/* Configure system clock and timing parameters */
void BSP_configureSystemClock(void);

/* Verify hardware initialization status */
rt_bool_t BSP_isInitialized(void);

/*==========================================================================*/
/* High-Resolution Timing Functions */
/*==========================================================================*/

/* Get current high-resolution timestamp in CPU cycles */
uint32_t BSP_getTimestamp(void);

/* Get current timestamp in microseconds */
uint32_t BSP_getTimestampMicroseconds(void);

/* Get current timestamp in milliseconds */
uint32_t BSP_getTimestampMilliseconds(void);

/* Reset DWT cycle counter to zero */
void BSP_resetTimestamp(void);

/* Convert CPU cycles to microseconds */
uint32_t BSP_cyclesToMicroseconds(uint32_t cycles);

/* Convert CPU cycles to milliseconds */
uint32_t BSP_cyclesToMilliseconds(uint32_t cycles);

/* Convert microseconds to CPU cycles */
uint32_t BSP_microsecondsToCycles(uint32_t microseconds);

/* Get number of CPU cycles per millisecond */
uint32_t BSP_getCyclesPerMs(void);

/* Get number of ticks per millisecond */
uint32_t BSP_getTicksPerMs(void);

/*==========================================================================*/
/* Performance Measurement Functions */
/*==========================================================================*/

/* Start a performance measurement session */
void BSP_startPerfMeasurement(BSP_PerfMeasurement *measurement);

/* Stop a performance measurement session and calculate results */
void BSP_stopPerfMeasurement(BSP_PerfMeasurement *measurement);

/* Get elapsed time in cycles for a measurement */
uint32_t BSP_getMeasurementCycles(BSP_PerfMeasurement const *measurement);

/* Get elapsed time in microseconds for a measurement */
uint32_t BSP_getMeasurementMicroseconds(BSP_PerfMeasurement const *measurement);

/* Reset performance measurement context */
void BSP_resetPerfMeasurement(BSP_PerfMeasurement *measurement);

/* Calculate timing difference handling wraparound */
uint32_t BSP_calculateTimeDifference(uint32_t start_time, uint32_t end_time);

/*==========================================================================*/
/* System Statistics and Monitoring Functions */
/*==========================================================================*/

/* Get current system performance statistics */
void BSP_getSystemStats(BSP_SystemStats *stats);

/* Get current CPU utilization percentage */
uint8_t BSP_getCpuUtilization(void);

/* Get available heap memory in bytes */
uint32_t BSP_getFreeHeapBytes(void);

/* Get total heap memory in bytes */
uint32_t BSP_getTotalHeapBytes(void);

/* Get system uptime in milliseconds */
uint32_t BSP_getSystemUptimeMs(void);

/* Get maximum stack usage for current thread */
uint32_t BSP_getStackUsage(void);

/*==========================================================================*/
/* Hardware Control Functions */
/*==========================================================================*/

/* Enter critical section and disable interrupts */
rt_base_t BSP_enterCritical(void);

/* Exit critical section and restore interrupt state */
void BSP_exitCritical(rt_base_t level);

/* Delay execution for specified microseconds using precise timing */
void BSP_delayMicroseconds(uint32_t microseconds);

/* Delay execution for specified milliseconds */
void BSP_delayMilliseconds(uint32_t milliseconds);

/* Busy wait for specified number of CPU cycles */
void BSP_busyWaitCycles(uint32_t cycles);

/*==========================================================================*/
/* Debug and Diagnostic Functions */
/*==========================================================================*/

/* Print BSP initialization status and configuration */
void BSP_printStatus(void);

/* Print current system performance statistics */
void BSP_printSystemStats(void);

/* Print DWT configuration and status */
void BSP_printDWTStatus(void);

/* Validate BSP timing accuracy with known delays */
rt_bool_t BSP_validateTiming(void);

/* Run BSP self-test diagnostics */
rt_bool_t BSP_runSelfTest(void);

/*==========================================================================*/
/* Utility Macros for Performance Measurement */
/*==========================================================================*/

/* Macro to measure execution time of a code block */
#define BSP_MEASURE_EXECUTION_TIME(measurement, code_block) \
    do { \
        BSP_startPerfMeasurement(&(measurement)); \
        code_block; \
        BSP_stopPerfMeasurement(&(measurement)); \
    } while (0)

/* Macro to get current timestamp quickly */
#define BSP_GET_TIMESTAMP()         BSP_getTimestamp()

/* Macro to calculate time difference */
#define BSP_TIME_DIFF(start, end)   BSP_calculateTimeDifference(start, end)

/* Macro to convert cycles to microseconds */
#define BSP_CYCLES_TO_US(cycles)    BSP_cyclesToMicroseconds(cycles)

/* Macro to convert cycles to milliseconds */
#define BSP_CYCLES_TO_MS(cycles)    BSP_cyclesToMilliseconds(cycles)

/*==========================================================================*/
/* MSH Shell Command Functions for BSP */
/*==========================================================================*/

/* MSH command to show BSP status */
void bsp_status_cmd(void);

/* MSH command to show system statistics */
void bsp_stats_cmd(void);

/* MSH command to run BSP self-test */
void bsp_test_cmd(void);

/* MSH command to show DWT status */
void bsp_dwt_cmd(void);

/* MSH command to measure timing accuracy */
void bsp_timing_test_cmd(void);

/* MSH command to reset performance counters */
void bsp_reset_cmd(void);

#endif /* BSP_H_ */