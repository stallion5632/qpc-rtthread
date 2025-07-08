/**
* @file
* @brief Internal QF interface.
*/
#ifndef QF_PKG_H
#define QF_PKG_H

/* Include all components for the port... */
#include "qf_port.h"

/* Internal QF interface... */
extern QF_EPOOL_TYPE_ QF_pool_[QF_MAX_EPOOL];
extern uint_fast8_t QF_maxPool_;
extern QSubscrList *QF_subscrList_;
extern QActive *QF_active_[QF_MAX_ACTIVE + 1];

/* Cooperative scheduler specific declarations */
#if (QF_COOP_SCHED_ENABLE != 0)
extern uint8_t QF_coopActive;
extern uint8_t QF_coopScheduling;

/* Check if cooperative scheduling is active */
#define QF_COOP_SCHED_ACTIVE() (QF_coopScheduling != 0)

/* Start cooperative scheduling */
void QF_coopSchedStart(void);

/* Stop cooperative scheduling */
void QF_coopSchedStop(void);
#endif

/* Framework functions for RT-Thread port */
void QActive_eventLoop_(QActive * const me);

#endif /* QF_PKG_H */