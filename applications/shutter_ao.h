#ifndef SHUTTER_AO_H
#define SHUTTER_AO_H

#include "qpc.h"
#include "app_events.h"
#include "rs500_defs.h"
#include <rtthread.h>

/*
 * 配置参数定义
 */
#define SHUTTER_CLOSE_TIMEOUT (RT_TICK_PER_SECOND * 2U) /* 2秒超时 */
#define SHUTTER_RETRY_COUNT 3U                          /* 最大重试次数 */

/*
 * 快门活动对象
 */
typedef struct
{
    QActive super;       /* 继承QActive */
    QTimeEvt closeTimer; /* 关闭超时定时器 */
    ShutterState state;  /* 快门状态 */
    uint8_t retryCount;  /* 重试计数 */
} ShutterAO;

/* 对外接口 */
extern ShutterAO shutterAO;
void ShutterAO_ctor(ShutterAO *const me);

#endif /* SHUTTER_AO_H */
