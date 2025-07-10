#ifndef ISP_AO_H
#define ISP_AO_H

#include "app_events.h"
#include "qpc.h"
#include "rs500_defs.h"
#include <rtthread.h>


/*
 * ISP活动对象
 */
typedef struct {
  QActive super;  /* 继承QActive */
  IspState state; /* ISP状态 */
} IspAO;

extern IspAO ispAO;
void IspAO_ctor(IspAO *const me);

#endif /* ISP_AO_H */
