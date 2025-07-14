#include "qpc.h"
#include "app_main.h"
#include <rtthread.h>

/* Minimal implementation to avoid linker errors */
void QActive_stop(QActive *me)
{
    /* Cleanup logic can be added if needed; currently empty implementation */
    (void)me;
}

/* Main function: start performance test application */
int main(void)
{
    /* Initialize and start performance test */
    PerformanceApp_init();
    PerformanceApp_start();

    /* Wait for a period to simulate test execution */
    rt_kprintf("[QPC] Auto running performance test cases...\n");
    rt_thread_mdelay(5000); /* Run for 5 seconds */

    /* Print statistics results */
    PerformanceStats stats;
    PerformanceApp_getStats(&stats);
    rt_kprintf("=== Auto Test Statistics ===\n");
    rt_kprintf("Test running: %s\n", stats.test_running ? "Yes" : "No");
    rt_kprintf("Test duration: %u ms\n", stats.test_duration_ms);
    rt_kprintf("Counter updates: %u\n", stats.counter_updates);
    rt_kprintf("Timer ticks: %u\n", stats.timer_ticks);
    rt_kprintf("Timer reports: %u\n", stats.timer_reports);
    rt_kprintf("Log messages: %u\n", stats.log_messages);

    /* Stop the test */
    PerformanceApp_stop();
    rt_kprintf("[QPC] Performance test stopped\n");

    /* Reset statistics */
    PerformanceApp_resetStats();
    rt_kprintf("[QPC] Statistics reset\n");

    /* Enter idle loop or return */
    while (1)
    {
        rt_thread_mdelay(1000);
    }
    return 0;
}
