#ifndef RTPOOL_DEMO_H
#define RTPOOL_DEMO_H

#include "qpc.h"
#include <rtthread.h>

/* Signals */
enum AppSignals {
    TICK_SIG = Q_USER_SIG,
    MAX_SIG
};

/* Base event */
typedef QEvt BaseEvt;

/* Data event with payload */
typedef struct {
    QEvt super;
    uint32_t data;
    uint32_t _padding; /* ensure size > QEvt */
} DataEvt;

/* Demo Active Object */
typedef struct {
    QActive super;
    QTimeEvt timeEvt;
    uint32_t counter;
} DemoAO;

extern QActive * const AO_Demo;

/* Initialization and start functions */
void rtmpool_demo_init(void);
int rtmpool_demo_start(void);

#endif /* RTPOOL_DEMO_H */
