
#include "qpc.h"
#include "qactive_demo.h"
#include "qf.h"
#include <stdint.h>

#ifndef SYSTEM_SIG
#define SYSTEM_SIG   (Q_USER_SIG + 10U) /* System signal definition */
#endif

/* Example: call this from an ISR or RT-Thread thread */
void publish_system_event(void) {
    QEvt sysEvt = { SYSTEM_SIG, 0U, 0U };  /* MISRA C 2012 compliant initialization */
    QF_PUBLISH(&sysEvt, (void const *)0);  /* Publisher is NULL for system events */
}
