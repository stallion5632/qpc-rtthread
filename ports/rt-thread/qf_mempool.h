#ifndef QF_MEMPOOL_H
#define QF_MEMPOOL_H

#include "qf_port.h"

#if QF_ENABLE_RT_MEMPOOL
void QF_poolInit_RT(void);
QEvt *QF_newX_RT(uint_fast16_t evtSize, uint_fast16_t margin, enum_t sig);
void QF_gc_RT(QEvt const * const e);
void QF_poolGetStat_RT(uint8_t poolId,
                       uint16_t *pFree,
                       uint16_t *pUsed,
                       uint16_t *pPeak);
#endif

#endif /* QF_MEMPOOL_H */
