#include <rtthread.h>
#include "qpc.h"

#ifdef QF_BLOCKING_PROXY_ENABLE
#include "qf_block_proxy.h"
#endif

static rt_timer_t qpc_tick_timer;

void QF_onClockTick(void *parameter) {
    QF_TICK_X(0U, (void *)0);  /* perform the QF clock tick processing */
}

void QF_onStartup(void) {
    qpc_tick_timer = rt_timer_create("qpc_tick", QF_onClockTick,
                                            RT_NULL, 10,
                                            RT_TIMER_FLAG_PERIODIC);
    rt_timer_start(qpc_tick_timer);

#ifdef QF_BLOCKING_PROXY_ENABLE
    /* Initialize blocking proxy */
    QF_proxyInit();
#endif
}

void QF_onCleanup(void) {
    rt_timer_stop(qpc_tick_timer);
}

void Q_onAssert(char const * const module, int loc) {
    rt_assert_handler(module, "", loc);
    while(1);
}
