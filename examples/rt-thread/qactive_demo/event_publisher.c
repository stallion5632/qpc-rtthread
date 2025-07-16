#include "qpc.h"
#include "qactive_demo.h"
#include "qf.h"

#ifndef SYSTEM_SIG
#define SYSTEM_SIG   (Q_USER_SIG + 10) // XXX 如无定义则补充定义
#endif

/* Example: call this from an ISR or RT-Thread thread */
void publish_system_event(void) {
    QEvt sysEvt = { SYSTEM_SIG, 0U };
    QF_PUBLISH(&sysEvt, (void*)0); // TODO sender 设为 NULL
}
