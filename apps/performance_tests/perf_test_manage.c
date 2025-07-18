/*============================================================================
* Product: Performance Test Manager Implementation
* Last updated for version 7.2.0
* Last updated on  2025-01-17
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
*==========================================================================*/
// 文件顶部只定义一次 Q_DEFINE_THIS_FILE
#include "qpc.h"
#include "perf_common.h"
#include "perf_test_manage.h"
#include <stdint.h>
#include <rtthread.h>
// Q_DEFINE_THIS_FILE 宏已移除，避免未使用变量警告

/*==========================================================================*/
/* Local Definitions */
/*==========================================================================*/
static uint8_t s_framework_initialized = 0U;
static rt_timer_t test_timer = RT_NULL;
static uint8_t current_test_index = 0;
static rt_bool_t all_tests_running = RT_FALSE;

/* 测试项目列表 */
typedef struct {
    const char *name;
    void (*start_func)(void);
    void (*stop_func)(void);
    uint32_t duration_ms;  /* 每个测试运行时间(毫秒) */
} test_item_t;

/* 外部测试函数声明（必须放在 test_items 数组定义之前） */
void LatencyTest_start(void);
void LatencyTest_stop(void);
void ThroughputTest_start(void);
void ThroughputTest_stop(void);
void JitterTest_start(void);
void JitterTest_stop(void);
void IdleCpuTest_start(void);
void IdleCpuTest_stop(void);
void MemoryTest_start(void);
void MemoryTest_stop(void);

static const test_item_t test_items[] = {
    {"Latency Test", LatencyTest_start, LatencyTest_stop, 5000},
    {"Throughput Test", ThroughputTest_start, ThroughputTest_stop, 5000},
    {"Jitter Test", JitterTest_start, JitterTest_stop, 5000},
    {"Idle CPU Test", IdleCpuTest_start, IdleCpuTest_stop, 5000},
    {"Memory Test", MemoryTest_start, MemoryTest_stop, 5000},
};

#define NUM_TESTS (sizeof(test_items) / sizeof(test_items[0]))

/* 定时器回调函数：自动切换到下一个测试 */
static void test_timer_callback(void *parameter);
/*==========================================================================*/
/* Public API Functions - 全局可见供 main.c 调用 */
/*==========================================================================*/
void PerfTestManager_init(void) {
    if (s_framework_initialized != 0U) {
        rt_kprintf("Performance Test Manager: Already initialized\n");
        return;
    }

    rt_kprintf("Performance Test Manager: Initializing...\n");

    /* 基础框架初始化 */
    QF_init();

    /* 初始化发布订阅 */
    static QSubscrList subscrSto[32];
    QF_psInit(subscrSto, Q_DIM(subscrSto));

    /* 创建定时器用于自动测试切换 */
    if (test_timer == RT_NULL) {
        test_timer = rt_timer_create("test_timer",
                                   test_timer_callback,
                                   RT_NULL,
                                   1000,  /* 1秒超时，将在启动时重新配置 */
                                   RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
        if (test_timer == RT_NULL) {
            rt_kprintf("Failed to create test timer\n");
            return;
        }
    }

    current_test_index = 0;
    all_tests_running = RT_FALSE;

    s_framework_initialized = 1U;
    rt_kprintf("Performance Test Manager: Initialized successfully\n");
}

/* 定时器回调函数实现 */
static void test_timer_callback(void *parameter) {
    (void)parameter;

    if (!all_tests_running) {
        return;
    }

    /* 停止当前测试 */
    if (current_test_index > 0 && current_test_index <= NUM_TESTS) {
        rt_kprintf("\n=== Stopping %s ===\n", test_items[current_test_index - 1].name);
        test_items[current_test_index - 1].stop_func();
        rt_thread_mdelay(1000); /* 给测试停止一些时间 */
    }

    /* 检查是否所有测试都已完成 */
    if (current_test_index >= NUM_TESTS) {
        rt_kprintf("\n=== All Performance Tests Completed ===\n");
        rt_kprintf("Performance test suite finished successfully!\n");
        rt_kprintf("Use 'perf_test_results' command to view detailed results.\n");
        all_tests_running = RT_FALSE;
        return;
    }

    /* 启动下一个测试 */
    rt_kprintf("\n=== Starting %s (%d/%d) ===\n",
               test_items[current_test_index].name,
               current_test_index + 1,
               (int)NUM_TESTS);

    test_items[current_test_index].start_func();

    /* 设置定时器为当前测试的持续时间 */
    uint32_t duration = test_items[current_test_index].duration_ms;
    rt_timer_control(test_timer, RT_TIMER_CTRL_SET_TIME, &duration);
    rt_timer_start(test_timer);

    current_test_index++;
}

int32_t PerfTestManager_startAll(void) {
    if (s_framework_initialized == 0U) {
        rt_kprintf("Performance Test Manager: Not initialized, calling init first\n");
        PerfTestManager_init();
    }

    if (all_tests_running) {
        rt_kprintf("Performance tests are already running. Please wait for completion or stop them first.\n");
        return -1;
    }

    rt_kprintf("Performance Test Manager: Starting automated test suite\n");
    rt_kprintf("=== Performance Test Suite Started ===\n");
    rt_kprintf("Total tests to run: %d\n", (int)NUM_TESTS);
    rt_kprintf("Each test duration: 5 seconds\n");
    rt_kprintf("Total estimated time: %d seconds\n", (int)(NUM_TESTS * 5));
    rt_kprintf("Tests will run automatically in sequence...\n\n");

    /* 重置测试状态 */
    current_test_index = 0;
    all_tests_running = RT_TRUE;

    /* 启动第一个测试 */
    test_timer_callback(RT_NULL);

    return 0;
}

void PerfTestManager_stopAll(void) {
    rt_kprintf("Performance Test Manager: Stopping all tests\n");

    if (!all_tests_running) {
        rt_kprintf("No tests are currently running\n");
        return;
    }

    /* 停止当前正在运行的测试 */
    if (current_test_index > 0 && current_test_index <= NUM_TESTS) {
        rt_kprintf("Stopping current test: %s\n", test_items[current_test_index - 1].name);
        test_items[current_test_index - 1].stop_func();
    }

    /* 停止定时器 */
    if (test_timer != RT_NULL) {
        rt_timer_stop(test_timer);
    }

    all_tests_running = RT_FALSE;
    current_test_index = 0;

    rt_kprintf("All performance tests stopped\n");
}

/*==========================================================================*/
/* MSH Commands - Shell命令导出 */
/*==========================================================================*/
int perf_test_info_cmd(int argc, char **argv) {
    (void)argc;
    (void)argv;

    rt_kprintf("Performance Test Suite Information:\n");
    rt_kprintf("Available tests (%d total):\n", (int)NUM_TESTS);
    for (uint8_t i = 0; i < NUM_TESTS; i++) {
        rt_kprintf("  %d. %s - Duration: %d ms\n",
                   i + 1,
                   test_items[i].name,
                   test_items[i].duration_ms);
    }
    rt_kprintf("Commands:\n");
    rt_kprintf("  perf_test_start_all - Start all tests automatically\n");
    rt_kprintf("  perf_test_stop_all - Stop all tests\n");
    rt_kprintf("  perf_test_info - Show this information\n");
    rt_kprintf("Test Status: %s\n", all_tests_running ? "Running" : "Stopped");
    if (all_tests_running) {
        rt_kprintf("Current test: %d/%d\n", current_test_index, (int)NUM_TESTS);
    }
    return 0;
}

int perf_test_start_all_cmd(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return PerfTestManager_startAll();
}

int perf_test_stop_all_cmd(int argc, char **argv) {
    (void)argc;
    (void)argv;
    PerfTestManager_stopAll();
    return 0;
}

/* Export MSH commands */
#ifdef RT_USING_FINSH
#include <finsh.h>
MSH_CMD_EXPORT_ALIAS(perf_test_info_cmd, perf_test_info,
                     show performance test information);
MSH_CMD_EXPORT_ALIAS(perf_test_start_all_cmd, perf_test_start_all,
                     start all performance tests);
MSH_CMD_EXPORT_ALIAS(perf_test_stop_all_cmd, perf_test_stop_all,
                     stop all performance tests);
#endif
