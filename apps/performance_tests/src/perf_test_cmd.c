#include "perf_test.h"
#include <string.h>

/*
 * Command handler for performance test shell commands.
 * Supports: list, start, stop, restart, report.
 */
static int cmd_perf(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: perf <list|start|stop|restart|report> [name]\n");
        return -1;
    }
    const char *op = argv[1];
    if (strcmp(op, "list") == 0)
    {
        perf_test_list();
    }
    else if ((strcmp(op, "start") == 0 || strcmp(op, "stop") == 0 ||
              strcmp(op, "restart") == 0) &&
             argc == 3)
    {
        rt_int32_t res = (strcmp(op, "start") == 0)
                             ? perf_test_start(argv[2])
                         : (strcmp(op, "stop") == 0)
                             ? perf_test_stop(argv[2])
                             : perf_test_restart(argv[2]);
        rt_kprintf("perf %s %s -> %s (code=%d)\n",
                   op, argv[2],
                   res == 0 ? "OK" : "FAIL", res);
    }
    else if (strcmp(op, "report") == 0)
    {
        perf_test_report();
    }
    else
    {
        rt_kprintf("Unknown command\n");
    }
    return 0;
}

/* Register the shell command with RT-Thread MSH */
MSH_CMD_EXPORT(cmd_perf, perf<list | start | stop | restart | report>[test_name]);
