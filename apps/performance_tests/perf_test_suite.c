/*============================================================================
* Product: Performance Test Suite Runner
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
/* External Function Declarations */
/*==========================================================================*/

/* Test function declarations */
extern void LatencyTest_start(void);
extern void LatencyTest_stop(void);
extern void ThroughputTest_start(void);
extern void ThroughputTest_stop(void);
extern void JitterTest_start(void);
extern void JitterTest_stop(void);
extern void IdleCpuTest_start(void);
extern void IdleCpuTest_stop(void);
extern void MemoryTest_start(void);
extern void MemoryTest_stop(void);

/*==========================================================================*/
/* Performance Test Suite Functions */
/*==========================================================================*/

void PerfTestSuite_runAll(void) {
    rt_kprintf("==================================================\n");
    rt_kprintf("Starting QPC-RT-Thread Performance Test Suite\n");
    rt_kprintf("==================================================\n\n");
    
    /* Run Latency Test */
    rt_kprintf("### Running Latency Performance Test ###\n");
    LatencyTest_start();
    rt_thread_mdelay(1000); /* Wait 1 second between tests */
    LatencyTest_stop();
    rt_kprintf("\n");
    
    /* Run Throughput Test */
    rt_kprintf("### Running Throughput Performance Test ###\n");
    ThroughputTest_start();
    rt_thread_mdelay(1000); /* Wait 1 second between tests */
    ThroughputTest_stop();
    rt_kprintf("\n");
    
    /* Run Jitter Test */
    rt_kprintf("### Running Jitter Performance Test ###\n");
    JitterTest_start();
    rt_thread_mdelay(1000); /* Wait 1 second between tests */
    JitterTest_stop();
    rt_kprintf("\n");
    
    /* Run Idle CPU Test */
    rt_kprintf("### Running Idle CPU Performance Test ###\n");
    IdleCpuTest_start();
    rt_thread_mdelay(1000); /* Wait 1 second between tests */
    IdleCpuTest_stop();
    rt_kprintf("\n");
    
    /* Run Memory Test */
    rt_kprintf("### Running Memory Performance Test ###\n");
    MemoryTest_start();
    rt_thread_mdelay(1000); /* Wait 1 second between tests */
    MemoryTest_stop();
    rt_kprintf("\n");
    
    rt_kprintf("==================================================\n");
    rt_kprintf("Performance Test Suite Completed\n");
    rt_kprintf("==================================================\n");
}

void PerfTestSuite_runLatency(void) {
    rt_kprintf("Starting Latency Performance Test...\n");
    LatencyTest_start();
}

void PerfTestSuite_runThroughput(void) {
    rt_kprintf("Starting Throughput Performance Test...\n");
    ThroughputTest_start();
}

void PerfTestSuite_runJitter(void) {
    rt_kprintf("Starting Jitter Performance Test...\n");
    JitterTest_start();
}

void PerfTestSuite_runIdleCpu(void) {
    rt_kprintf("Starting Idle CPU Performance Test...\n");
    IdleCpuTest_start();
}

void PerfTestSuite_runMemory(void) {
    rt_kprintf("Starting Memory Performance Test...\n");
    MemoryTest_start();
}

void PerfTestSuite_stopAll(void) {
    rt_kprintf("Stopping all performance tests...\n");
    
    /* Stop all tests */
    LatencyTest_stop();
    ThroughputTest_stop();
    JitterTest_stop();
    IdleCpuTest_stop();
    MemoryTest_stop();
    
    rt_kprintf("All performance tests stopped\n");
}

void PerfTestSuite_printInfo(void) {
    rt_kprintf("==================================================\n");
    rt_kprintf("QPC-RT-Thread Performance Test Suite\n");
    rt_kprintf("==================================================\n");
    rt_kprintf("Available tests:\n");
    rt_kprintf("  1. Latency Test     - Measures event latency\n");
    rt_kprintf("  2. Throughput Test  - Measures event throughput\n");
    rt_kprintf("  3. Jitter Test      - Measures timing jitter\n");
    rt_kprintf("  4. Idle CPU Test    - Measures CPU idle time\n");
    rt_kprintf("  5. Memory Test      - Measures memory allocation performance\n");
    rt_kprintf("\n");
    rt_kprintf("Commands:\n");
    rt_kprintf("  PerfTestSuite_runAll()       - Run all tests\n");
    rt_kprintf("  PerfTestSuite_runLatency()   - Run latency test\n");
    rt_kprintf("  PerfTestSuite_runThroughput() - Run throughput test\n");
    rt_kprintf("  PerfTestSuite_runJitter()    - Run jitter test\n");
    rt_kprintf("  PerfTestSuite_runIdleCpu()   - Run idle CPU test\n");
    rt_kprintf("  PerfTestSuite_runMemory()    - Run memory test\n");
    rt_kprintf("  PerfTestSuite_stopAll()      - Stop all tests\n");
    rt_kprintf("  PerfTestSuite_printInfo()    - Print this information\n");
    rt_kprintf("==================================================\n");
}

/* RT-Thread MSH command exports */
#ifdef RT_USING_FINSH
#include <finsh.h>
MSH_CMD_EXPORT(PerfTestSuite_runAll, run all performance tests);
MSH_CMD_EXPORT(PerfTestSuite_runLatency, run latency performance test);
MSH_CMD_EXPORT(PerfTestSuite_runThroughput, run throughput performance test);
MSH_CMD_EXPORT(PerfTestSuite_runJitter, run jitter performance test);
MSH_CMD_EXPORT(PerfTestSuite_runIdleCpu, run idle CPU performance test);
MSH_CMD_EXPORT(PerfTestSuite_runMemory, run memory performance test);
MSH_CMD_EXPORT(PerfTestSuite_stopAll, stop all performance tests);
MSH_CMD_EXPORT(PerfTestSuite_printInfo, print performance test suite info);
#endif