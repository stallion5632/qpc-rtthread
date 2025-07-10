#include "isp_ao.h"
#include "rs500_defs.h"

/* 内部函数声明 */
static QState IspAO_initial(IspAO *const me, QEvt const *const e);
static QState IspAO_active(IspAO *const me, QEvt const *const e);
static rt_err_t IspAO_handleCmd(IspCmd cmd);

/* 全局实例 */
IspAO ispAO;

/*
 * ISP服务构造函数
 */
void IspAO_ctor(IspAO *const me)
{
    QActive_ctor(&me->super, Q_STATE_CAST(&IspAO_initial));
    me->state = ISP_STATE_IDLE;
}

/*
 * 初始状态处理函数
 */
static QState IspAO_initial(IspAO *const me, QEvt const *const e)
{
    (void)e;

    QActive_subscribe(&me->super, EVT_ISP_STOP_TECLESS_B);
    QActive_subscribe(&me->super, EVT_ISP_START_TECLESS_B);
    QActive_subscribe(&me->super, EVT_ISP_UPDATE_B_PREPARE);
    QActive_subscribe(&me->super, EVT_ISP_UPDATE_B);
    QActive_subscribe(&me->super, EVT_ISP_UPDATE_B_END);
    QActive_subscribe(&me->super, EVT_SEQUENCE_ROLLBACK);

    return Q_TRAN(&IspAO_active);
}

/*
 * 活动状态处理函数
 */
static QState IspAO_active(IspAO *const me, QEvt const *const e)
{
    QState status;
    rt_err_t ret;

    switch (e->sig)
    {
    case EVT_ISP_STOP_TECLESS_B:
    {
        rt_kprintf("[IspAO] Stopping tecless B mode\n");
        me->state = ISP_STATE_BUSY;
        ret = IspAO_handleCmd(ISP_CMD_STOP_TECLESS_B);
        if (RT_EOK == ret)
        {
            me->state = ISP_STATE_IDLE;
        }
        else
        {
            me->state = ISP_STATE_ERROR;
            AppEvt evt = {{EVT_ISP_ERROR}, NULL};
            QACTIVE_POST(&me->super, &evt.super, NULL);
        }
        status = Q_HANDLED();
        break;
    }

    case EVT_ISP_START_TECLESS_B:
    {
        rt_kprintf("[IspAO] Starting tecless B mode\n");
        me->state = ISP_STATE_BUSY;
        ret = IspAO_handleCmd(ISP_CMD_START_TECLESS_B);
        if (RT_EOK == ret)
        {
            me->state = ISP_STATE_IDLE;
        }
        else
        {
            me->state = ISP_STATE_ERROR;
            AppEvt evt = {{EVT_ISP_ERROR}, NULL};
            QACTIVE_POST(&me->super, &evt.super, NULL);
        }
        status = Q_HANDLED();
        break;
    }

    case EVT_ISP_UPDATE_B_PREPARE:
    {
        rt_kprintf("[IspAO] Preparing B update\n");
        me->state = ISP_STATE_BUSY;
        ret = IspAO_handleCmd(ISP_CMD_UPDATE_B_PREPARE);
        if (RT_EOK != ret)
        {
            me->state = ISP_STATE_ERROR;
            AppEvt evt = {{EVT_ISP_ERROR}, NULL};
            QACTIVE_POST(&me->super, &evt.super, NULL);
        }
        status = Q_HANDLED();
        break;
    }

    case EVT_ISP_UPDATE_B:
    {
        rt_kprintf("[IspAO] Updating B\n");
        ret = IspAO_handleCmd(ISP_CMD_UPDATE_B);
        if (RT_EOK != ret)
        {
            me->state = ISP_STATE_ERROR;
            AppEvt evt = {{EVT_ISP_ERROR}, NULL};
            QACTIVE_POST(&me->super, &evt.super, NULL);
        }
        status = Q_HANDLED();
        break;
    }

    case EVT_ISP_UPDATE_B_END:
    {
        rt_kprintf("[IspAO] Completing B update\n");
        ret = IspAO_handleCmd(ISP_CMD_UPDATE_B_END);
        if (RT_EOK == ret)
        {
            me->state = ISP_STATE_IDLE;
        }
        else
        {
            me->state = ISP_STATE_ERROR;
            AppEvt evt = {{EVT_ISP_ERROR}, NULL};
            QACTIVE_POST(&me->super, &evt.super, NULL);
        }
        status = Q_HANDLED();
        break;
    }

    case EVT_SEQUENCE_ROLLBACK:
    {
        rt_kprintf("[IspAO] Rolling back to safe state\n");
        /* 回退时确保停止所有操作 */
        ret = IspAO_handleCmd(ISP_CMD_STOP_TECLESS_B);
        if (RT_EOK == ret)
        {
            me->state = ISP_STATE_IDLE;
        }
        status = Q_HANDLED();
        break;
    }

    default:
    {
        status = Q_SUPER(&QHsm_top);
        break;
    }
    }

    return status;
}

/*
 * ISP命令处理函数
 */
static rt_err_t IspAO_handleCmd(IspCmd cmd)
{
    rt_err_t ret;

    if ((cmd > ISP_CMD_NONE) && (cmd < ISP_CMD_MAX))
    {
        ret = rs500_isp_control(cmd);
    }
    else
    {
        ret = -RT_EINVAL;
    }

    return ret;
}
