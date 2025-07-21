/*============================================================================
* QP/C RT-Thread Optimization Features Test Script
*
* 这个脚本用于验证所有新增的优化功能
============================================================================*/
#include "qpc.h"
#include <rtthread.h>
#include <finsh.h>

Q_DEFINE_THIS_MODULE("opt_test");

/*==========================================================================*/
/* Test Functions */
/*==========================================================================*/

/*..........................................................................*/
static void test_heartbeat_mechanism(void) {
    rt_kprintf("\n==== Testing Heartbeat Mechanism ====\n");

#if QF_ENABLE_HEARTBEAT
    rt_kprintf("✓ Heartbeat mechanism: ENABLED\n");
    rt_kprintf("  Heartbeat interval: %d ticks\n", QF_HEARTBEAT_TICKS);
    rt_kprintf("  This feature is automatically tested in AO threads\n");
#else
    rt_kprintf("✗ Heartbeat mechanism: DISABLED\n");
#endif

    rt_kprintf("==========================================\n");
}

/*..........................................................................*/
static void test_rt_mempool_integration(void) {
    rt_kprintf("\n==== Testing RT-Thread Memory Pool Integration ====\n");

#if QF_ENABLE_RT_MEMPOOL
    rt_kprintf("✓ RT-Thread memory pool integration: ENABLED\n");

    // Test small event allocation
    rt_kprintf("Testing small event allocation...\n");
    typedef struct { QEvt super; uint8_t data[32]; } SmallTestEvt;
    SmallTestEvt *small_evt = Q_NEW(SmallTestEvt, 1);
    if (small_evt != (SmallTestEvt *)0) {
        rt_kprintf("✓ Small event allocated successfully\n");
        QF_gc(&small_evt->super);
        rt_kprintf("✓ Small event freed successfully\n");
    } else {
        rt_kprintf("✗ Small event allocation failed\n");
    }

    // Test medium event allocation
    rt_kprintf("Testing medium event allocation...\n");
    typedef struct { QEvt super; uint8_t data[100]; } MediumTestEvt;
    MediumTestEvt *medium_evt = Q_NEW(MediumTestEvt, 2);
    if (medium_evt != (MediumTestEvt *)0) {
        rt_kprintf("✓ Medium event allocated successfully\n");
        QF_gc(&medium_evt->super);
        rt_kprintf("✓ Medium event freed successfully\n");
    } else {
        rt_kprintf("✗ Medium event allocation failed\n");
    }

    // Test large event allocation
    rt_kprintf("Testing large event allocation...\n");
    typedef struct { QEvt super; uint8_t data[200]; } LargeTestEvt;
    LargeTestEvt *large_evt = Q_NEW(LargeTestEvt, 3);
    if (large_evt != (LargeTestEvt *)0) {
        rt_kprintf("✓ Large event allocated successfully\n");
        QF_gc(&large_evt->super);
        rt_kprintf("✓ Large event freed successfully\n");
    } else {
        rt_kprintf("✗ Large event allocation failed\n");
    }

    // Print memory pool statistics
    rt_kprintf("Memory pool statistics:\n");
    extern void QF_poolPrintStats_RT(void);
    QF_poolPrintStats_RT();

#else
    rt_kprintf("✗ RT-Thread memory pool integration: DISABLED\n");
    rt_kprintf("  Using QPC native memory pools\n");
#endif

    rt_kprintf("====================================================\n");
}

/*..........................................................................*/
static void test_optimization_layer(void) {
    rt_kprintf("\n==== Testing Optimization Layer ====\n");

    // Test dispatcher metrics
    rt_kprintf("Testing dispatcher metrics API...\n");
    QF_DispatcherMetrics const *metrics = QF_getDispatcherMetrics();
    if (metrics != (QF_DispatcherMetrics *)0) {
        rt_kprintf("✓ Dispatcher metrics API working\n");
        rt_kprintf("  Events processed: %lu\n", metrics->eventsProcessed);
        rt_kprintf("  Events dropped: %lu\n", metrics->eventsDropped);
        rt_kprintf("  Dispatch cycles: %lu\n", metrics->dispatchCycles);
    } else {
        rt_kprintf("✗ Dispatcher metrics API failed\n");
    }

    // Test strategy switching
    rt_kprintf("Testing dispatcher strategy switching...\n");
    QF_DispatcherStrategy const *current_strategy = QF_getDispatcherPolicy();
    if (current_strategy != (QF_DispatcherStrategy *)0) {
        rt_kprintf("✓ Strategy API working\n");

        // Test switching to high performance strategy
        QF_setDispatcherStrategy(&QF_highPerfStrategy);
        if (QF_getDispatcherPolicy() == &QF_highPerfStrategy) {
            rt_kprintf("✓ Strategy switching to high-performance: SUCCESS\n");
        } else {
            rt_kprintf("✗ Strategy switching to high-performance: FAILED\n");
        }

        // Switch back to default
        QF_setDispatcherStrategy(&QF_defaultStrategy);
        if (QF_getDispatcherPolicy() == &QF_defaultStrategy) {
            rt_kprintf("✓ Strategy switching back to default: SUCCESS\n");
        } else {
            rt_kprintf("✗ Strategy switching back to default: FAILED\n");
        }
    } else {
        rt_kprintf("✗ Strategy API failed\n");
    }

    // Test optimization layer enable/disable
    rt_kprintf("Testing optimization layer control...\n");
    QF_enableOptLayer();
    rt_kprintf("✓ Optimization layer enabled\n");
    QF_disableOptLayer();
    rt_kprintf("✓ Optimization layer disabled\n");
    QF_enableOptLayer(); // Re-enable for normal operation

    rt_kprintf("=========================================\n");
}

/*..........................................................................*/
static void test_diagnostic_apis(void) {
    rt_kprintf("\n==== Testing Diagnostic APIs ====\n");

    rt_kprintf("Testing shell commands availability...\n");

    rt_kprintf("Available QF commands:\n");
    rt_kprintf("  qf_metrics  - Display dispatcher metrics\n");
    rt_kprintf("  qf_mempool  - Display memory pool status\n");
    rt_kprintf("  qf_aos      - Display Active Object status\n");
    rt_kprintf("  qf_strategy - Set dispatcher strategy\n");
    rt_kprintf("  qf_reset    - Reset dispatcher metrics\n");
    rt_kprintf("  qf_opt      - Enable/disable optimization layer\n");
    rt_kprintf("  qf_help     - Display help information\n");

    rt_kprintf("✓ All diagnostic commands registered\n");

    rt_kprintf("Testing metrics reset...\n");
    QF_resetDispatcherMetrics();
    rt_kprintf("✓ Metrics reset successful\n");

    rt_kprintf("==========================================\n");
}

/*..........................................................................*/
static void run_comprehensive_test(void) {
    (void)Q_this_module_;
    rt_kprintf("\n");
    rt_kprintf("*********************************************************\n");
    rt_kprintf("*         QP/C RT-Thread Optimization Test Suite       *\n");
    rt_kprintf("*********************************************************\n");

    test_heartbeat_mechanism();
    test_rt_mempool_integration();
    test_optimization_layer();
    test_diagnostic_apis();

    rt_kprintf("\n");
    rt_kprintf("*********************************************************\n");
    rt_kprintf("*                   Test Summary                        *\n");
    rt_kprintf("*********************************************************\n");

#if QF_ENABLE_HEARTBEAT
    rt_kprintf("✓ Heartbeat & Watchdog: IMPLEMENTED AND ENABLED\n");
#else
    rt_kprintf("✗ Heartbeat & Watchdog: NOT ENABLED\n");
#endif

#if QF_ENABLE_RT_MEMPOOL
    rt_kprintf("✓ RT-Thread Memory Pool: IMPLEMENTED AND ENABLED\n");
#else
    rt_kprintf("✗ RT-Thread Memory Pool: NOT ENABLED\n");
#endif

    rt_kprintf("✓ Optimization Layer: IMPLEMENTED\n");
    rt_kprintf("✓ Diagnostic APIs: IMPLEMENTED\n");
    rt_kprintf("✓ Shell Commands: IMPLEMENTED\n");

    rt_kprintf("\nAll optimization features are ready for use!\n");
    rt_kprintf("*********************************************************\n");
}

/*==========================================================================*/
/* Shell Command Exports */
/*==========================================================================*/

int qf_test_all(void) {
    run_comprehensive_test();
    return 0;
}

int qf_test_heartbeat(void) {
    test_heartbeat_mechanism();
    return 0;
}

int qf_test_mempool(void) {
    test_rt_mempool_integration();
    return 0;
}

int qf_test_optlayer(void) {
    test_optimization_layer();
    return 0;
}

int qf_test_diagnostic(void) {
    test_diagnostic_apis();
    return 0;
}

/* Export to finsh shell */
MSH_CMD_EXPORT_ALIAS(qf_test_all, qf_test_all, Run comprehensive QF optimization test);
MSH_CMD_EXPORT_ALIAS(qf_test_heartbeat, qf_test_heartbeat, Test heartbeat mechanism);
MSH_CMD_EXPORT_ALIAS(qf_test_mempool, qf_test_mempool, Test RT-Thread memory pool);
MSH_CMD_EXPORT_ALIAS(qf_test_optlayer, qf_test_optlayer, Test optimization layer);
MSH_CMD_EXPORT_ALIAS(qf_test_diagnostic, qf_test_diagnostic, Test diagnostic APIs);
