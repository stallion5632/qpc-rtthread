#ifndef APP_EVENTS_H
#define APP_EVENTS_H

#include "qpc.h"

/*
 * 事件信号枚举
 */
typedef enum AppEvtSig
{
    /* 系统事件起始值 */
    EVT_SYS_START = Q_USER_SIG,

    /* 快门事件 */
    EVT_SHUTTER_CLOSE,         /* 普通关闭快门 */
    EVT_SHUTTER_OPEN,          /* 普通打开快门 */
    EVT_SHUTTER_URGENT_CLOSE,  /* 紧急关闭快门 */
    EVT_SHUTTER_URGENT_OPEN,   /* 紧急打开快门 */
    EVT_SHUTTER_CLOSE_TIMEOUT, /* 关闭快门超时 */
    EVT_SHUTTER_ERROR,         /* 快门错误 */

    /* ISP事件 */
    EVT_ISP_STOP_TECLESS_B,   /* 停止无TEC B */
    EVT_ISP_START_TECLESS_B,  /* 启动无TEC B */
    EVT_ISP_UPDATE_B_PREPARE, /* 准备更新B */
    EVT_ISP_UPDATE_B,         /* 更新B */
    EVT_ISP_UPDATE_B_END,     /* 完成更新B */
    EVT_ISP_TIMEOUT,          /* ISP操作超时 */
    EVT_ISP_ERROR,            /* ISP错误 */

    /* 流程控制事件 */
    EVT_SEQUENCE_ABORT,    /* 中止序列 */
    EVT_SEQUENCE_ROLLBACK, /* 回退序列 */
    EVT_SEQUENCE_COMPLETE, /* 完成序列 */

    MAX_PUB_SIG /* 最大信号值(必须最后) */
} AppEvtSig;

/*
 * pipeline节点输入参数结构体
 */
typedef struct PipelineParam
{
    void *input;    /* 输入参数指针 */
    uint32_t size;  /* 参数大小 */
    uint32_t flags; /* 控制标志 */
} PipelineParam;

/*
 * 事件基类
 */
typedef struct
{
    QEvt super;           /* 继承QEvt */
    PipelineParam *param; /* pipeline参数 */
} AppEvt;

#endif /* APP_EVENTS_H */
