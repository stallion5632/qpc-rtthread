/*============================================================================
 * QP/C Real-Time Embedded Framework (RTEF)
 * QF Dispatcher Diagnostics Tool for RT-Thread Shell
 *
 * @file
 * @brief Diagnostic and monitoring utility for QF dispatcher
 */

#include "qf_port.h"
#include "qf_opt_layer.h"
#include <finsh.h>
#include "qpc.h"

/**
 * @brief Print QF dispatcher metrics to the shell
 */
static void QF_printMetrics(void)
{
    QF_DispatcherMetrics const *metrics = QF_getDispatcherMetrics();

    rt_kprintf("\n==== QF Dispatcher Metrics ====\n");
    rt_kprintf("| Metric                 | Value     |\n");
    rt_kprintf("|------------------------|----------|\n");
    rt_kprintf("| Dispatch Cycles        | %8lu  |\n", metrics->dispatchCycles);
    rt_kprintf("| Events Processed       | %8lu  |\n", metrics->eventsProcessed);
    rt_kprintf("| Events Merged          | %8lu  |\n", metrics->eventsMerged);
    rt_kprintf("| Events Dropped         | %8lu  |\n", metrics->eventsDropped);
    rt_kprintf("| Events Retried         | %8lu  |\n", metrics->eventsRetried);
    rt_kprintf("| Max Batch Size         | %8lu  |\n", metrics->maxBatchSize);
    rt_kprintf("| Avg Batch Size         | %8lu  |\n", metrics->avgBatchSize);
    rt_kprintf("| Max Queue Depth        | %8lu  |\n", metrics->maxQueueDepth);
    rt_kprintf("| Post Failures          | %8lu  |\n", metrics->postFailures);
    rt_kprintf("| Lost Events (Total)    | %8lu  |\n", QF_getLostEventCount());
    rt_kprintf("|------------------------|----------|\n");
    rt_kprintf("| Staging Overflows:     |          |\n");
    rt_kprintf("| - High Priority        | %8lu  |\n", metrics->stagingOverflows[QF_PRIO_HIGH]);
    rt_kprintf("| - Normal Priority      | %8lu  |\n", metrics->stagingOverflows[QF_PRIO_NORMAL]);
    rt_kprintf("| - Low Priority         | %8lu  |\n", metrics->stagingOverflows[QF_PRIO_LOW]);
    rt_kprintf("==================================\n");
}

/**
 * @brief Print status of all active objects (AO) to the shell
 */
static void QF_printAOStatus(void)
{
    rt_kprintf("\n==== Active Object Status ====\n");
    rt_kprintf("| AO# | Name           | Queue | Max   | State |\n");
    rt_kprintf("|-----|----------------|-------|-------|-------|\n");

    extern QActive *QActive_registry_[QF_MAX_ACTIVE + 1U];
    uint8_t ao_idx = 1;
    for (uint8_t prio = 1; prio <= QF_MAX_ACTIVE; ++prio) {
        QActive *ao = QActive_registry_[prio];
        if (ao != (QActive *)0) {
            struct rt_thread *thread = &ao->thread;
            if (thread == RT_NULL || thread->entry == RT_NULL) continue;
            const char *name = (thread->name[0] != '\0') ? thread->name : "N/A";
            uint32_t queueDepth = (ao->eQueue.size > 0) ? ao->eQueue.entry : 0;
            uint32_t queueSize  = (ao->eQueue.size > 0) ? ao->eQueue.size  : 0;
            const char *stateStr = "Other";
            switch (thread->stat) {
                case RT_THREAD_READY:   stateStr = "Ready"; break;
                case RT_THREAD_RUNNING: stateStr = "Run";   break;
                case RT_THREAD_SUSPEND: stateStr = "Susp";  break;
                default:                stateStr = "Other"; break;
            }
            rt_kprintf("| %2d  | %-14s | %5lu | %5lu | %s |\n",
                       ao_idx,
                       name,
                       queueDepth,
                       queueSize,
                       stateStr);
            ao_idx++;
        }
    }
    rt_kprintf("|-----|----------------|-------|-------|-------|\n");
    rt_kprintf("===============================\n");
}

/**
 * @brief Set the dispatcher strategy via shell command
 * @param argc Argument count
 * @param argv Argument vector
 */
static void QF_setStrategy(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: qf_strategy <default|highperf>\n");
        rt_kprintf("Current strategy: %s\n",
                   QF_getDispatcherPolicy() == &QF_defaultStrategy ? "Default" : QF_getDispatcherPolicy() == &QF_highPerfStrategy ? "High Performance"
                                                                                                                                  : "Unknown");
        return;
    }

    if (rt_strcmp(argv[1], "default") == 0)
    {
        QF_setDispatcherStrategy(&QF_defaultStrategy);
        rt_kprintf("Dispatcher strategy set to: Default\n");
    }
    else if (rt_strcmp(argv[1], "highperf") == 0)
    {
        QF_setDispatcherStrategy(&QF_highPerfStrategy);
        rt_kprintf("Dispatcher strategy set to: High Performance\n");
    }
    else
    {
        rt_kprintf("Unknown strategy: %s\n", argv[1]);
        rt_kprintf("Available strategies: default, highperf\n");
    }
}

/**
 * @brief Reset dispatcher metrics via shell command
 */
static void QF_resetMetrics(void)
{
    QF_resetDispatcherMetrics();
    rt_kprintf("Dispatcher metrics reset.\n");
}

/**
 * @brief Enable or disable optimization layer via shell command
 * @param argc Argument count
 * @param argv Argument vector
 */
static void QF_enableDisableOpt(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: qf_opt <enable|disable>\n");
        return;
    }

    if (rt_strcmp(argv[1], "enable") == 0)
    {
        QF_enableOptLayer();
        rt_kprintf("Optimization layer enabled.\n");
    }
    else if (rt_strcmp(argv[1], "disable") == 0)
    {
        QF_disableOptLayer();
        rt_kprintf("Optimization layer disabled.\n");
    }
    else
    {
        rt_kprintf("Usage: qf_opt <enable|disable>\n");
    }
}

/**
 * @brief Print help information for QF dispatcher shell commands
 */
static void QF_dispatcherHelp(void)
{
    rt_kprintf("\n==== QF Dispatcher Commands ====\n");
    rt_kprintf("qf_metrics      - Display dispatcher metrics\n");
    rt_kprintf("qf_aos          - Display Active Object status\n");
    rt_kprintf("qf_strategy     - Set dispatcher strategy\n");
    rt_kprintf("qf_reset        - Reset dispatcher metrics\n");
    rt_kprintf("qf_opt          - Enable/disable optimization layer\n");
    rt_kprintf("qf_help         - Display this help\n");
    rt_kprintf("=================================\n");
}

/* Export to finsh shell */
MSH_CMD_EXPORT_ALIAS(QF_printMetrics, qf_metrics, Display QF dispatcher metrics);
MSH_CMD_EXPORT_ALIAS(QF_printAOStatus, qf_aos, Display Active Object status);
MSH_CMD_EXPORT_ALIAS(QF_setStrategy, qf_strategy, Set dispatcher strategy);
MSH_CMD_EXPORT_ALIAS(QF_resetMetrics, qf_reset, Reset dispatcher metrics);
MSH_CMD_EXPORT_ALIAS(QF_enableDisableOpt, qf_opt, Enable / disable optimization layer);
MSH_CMD_EXPORT_ALIAS(QF_dispatcherHelp, qf_help, Display QF dispatcher help);
