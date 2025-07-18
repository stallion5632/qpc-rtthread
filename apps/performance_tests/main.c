// 只包含必要头文件
#include <stdint.h>
#include "perf_test_manage.h"

int main(void) {
    PerfTestManager_init();
    PerfTestManager_startAll();
    return 0;
}
