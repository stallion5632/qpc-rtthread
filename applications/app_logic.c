#include "app_logic.h"
#include "isp_ao.h"
#include "shutter_ao.h"

/* 事件处理间隔 */
#define EVENT_INTERVAL_MS 100U

/*
 * 序列状态枚举
 */
typedef enum
{
    SEQ_STATE_IDLE = 0, /* 空闲状态 */
    SEQ_STATE_RUNNING,  /* 运行状态 */
    SEQ_STATE_ERROR,    /* 错误状态 */
    SEQ_STATE_ROLLBACK, /* 回退状态 */
    SEQ_STATE_ABORTED   /* 中止状态 */
} SequenceState;

/*
 * 序列节点定义
 */
typedef struct
{
    enum AppEvtSig sig;   /* 事件信号 */
    QActive *target;      /* 目标AO */
    bool isRollbackPoint; /* 是否为回退点 */
} SequenceNode;

/*
 * 自动快门序列表
 */
static const SequenceNode AUTO_SHUTTER_SEQ[] = {
    {EVT_ISP_STOP_TECLESS_B, &ispAO.super, false},
    {EVT_ISP_UPDATE_B_PREPARE, &ispAO.super, true}, /* 回退点1 */
    {EVT_SHUTTER_CLOSE, &shutterAO.super, true},    /* 回退点2 */
    {EVT_ISP_UPDATE_B, &ispAO.super, false},
    {EVT_SHUTTER_OPEN, &shutterAO.super, true}, /* 回退点3 */
    {EVT_ISP_UPDATE_B_END, &ispAO.super, false},
    {EVT_ISP_START_TECLESS_B, &ispAO.super, false},
    {EVT_SEQUENCE_COMPLETE, &ispAO.super, false}};

/*
 * 序列控制块
 */
static struct
{
    SequenceState state; /* 序列状态 */
    size_t currentIdx;   /* 当前执行索引 */
    size_t rollbackIdx;  /* 回退目标索引 */
} seqCtrl = {SEQ_STATE_IDLE, 0U, 0U};

/*
 * 错误处理函数
 */
static void handle_sequence_error(void)
{
    size_t i;
    AppEvt evt;

    rt_kprintf("[Sequence] Error detected, initiating rollback\n");
    seqCtrl.state = SEQ_STATE_ROLLBACK;

    /* 查找最近的回退点 */
    for (i = seqCtrl.currentIdx; i > 0U; i--)
    {
        if (AUTO_SHUTTER_SEQ[i].isRollbackPoint)
        {
            seqCtrl.rollbackIdx = i;
            break;
        }
    }

    /* 发送回退事件 */
    evt.super.sig = EVT_SEQUENCE_ROLLBACK;
    evt.param = NULL;

    /* 按顺序通知各个AO执行回退 */
    QACTIVE_POST(&shutterAO.super, &evt.super, NULL);
    rt_thread_mdelay(EVENT_INTERVAL_MS);

    QACTIVE_POST(&ispAO.super, &evt.super, NULL);
    rt_thread_mdelay(EVENT_INTERVAL_MS);

    rt_kprintf("[Sequence] Rollback completed to step %u\n",
               (uint32_t)seqCtrl.rollbackIdx);
}

/*
 * 中止序列处理
 */
void abort_current_sequence(void)
{
    AppEvt evt;

    if (seqCtrl.state == SEQ_STATE_RUNNING)
    {
        rt_kprintf("[Sequence] Aborting current sequence\n");

        /* 发送回退事件确保安全状态 */
        evt.super.sig = EVT_SEQUENCE_ROLLBACK;
        evt.param = NULL;

        QACTIVE_POST(&shutterAO.super, &evt.super, NULL);
        rt_thread_mdelay(EVENT_INTERVAL_MS);

        QACTIVE_POST(&ispAO.super, &evt.super, NULL);
        rt_thread_mdelay(EVENT_INTERVAL_MS);

        seqCtrl.state = SEQ_STATE_ABORTED;
        rt_kprintf("[Sequence] Sequence aborted\n");
    }
}

/*
 * 自动快门序列处理函数
 */
void auto_shutter_sequence(void)
{
    AppEvt evt;
    size_t maxSteps;

    /* 检查是否可以启动序列 */
    if (seqCtrl.state != SEQ_STATE_IDLE)
    {
        rt_kprintf("[Sequence] Cannot start: sequence already running\n");
        return;
    }

    /* 初始化序列状态 */
    seqCtrl.state = SEQ_STATE_RUNNING;
    seqCtrl.currentIdx = 0U;
    maxSteps = sizeof(AUTO_SHUTTER_SEQ) / sizeof(AUTO_SHUTTER_SEQ[0]);

    rt_kprintf("[Sequence] Starting auto shutter sequence\n");

    /* 执行序列 */
    while ((seqCtrl.currentIdx < maxSteps) &&
           (seqCtrl.state == SEQ_STATE_RUNNING))
    {

        evt.super.sig = AUTO_SHUTTER_SEQ[seqCtrl.currentIdx].sig;
        evt.param = NULL;

        rt_kprintf("[Sequence] Step %u/%u: executing event %u\n",
                   (uint32_t)seqCtrl.currentIdx + 1U, (uint32_t)maxSteps,
                   (uint32_t)evt.super.sig);

        /* 发送事件到目标AO */
        QACTIVE_POST(AUTO_SHUTTER_SEQ[seqCtrl.currentIdx].target, &evt.super, NULL);

        /* 等待事件处理完成 */
        rt_thread_mdelay(EVENT_INTERVAL_MS);

        /* 检查执行状态 */
        if ((shutterAO.state == SHUTTER_STATE_ERROR) ||
            (ispAO.state == ISP_STATE_ERROR))
        {
            handle_sequence_error();
            break;
        }

        seqCtrl.currentIdx++;
    }

    /* 输出序列执行结果 */
    switch (seqCtrl.state)
    {
    case SEQ_STATE_RUNNING:
        rt_kprintf("[Sequence] Completed successfully\n");
        seqCtrl.state = SEQ_STATE_IDLE;
        break;

    case SEQ_STATE_ERROR:
        rt_kprintf("[Sequence] Failed with errors\n");
        break;

    case SEQ_STATE_ROLLBACK:
        rt_kprintf("[Sequence] Rolled back due to errors\n");
        break;

    case SEQ_STATE_ABORTED:
        rt_kprintf("[Sequence] Aborted by request\n");
        break;

    default:
        break;
    }
}
