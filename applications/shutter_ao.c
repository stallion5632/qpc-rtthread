#include "shutter_ao.h"
#include "rs500_defs.h"

/* 内部函数声明 */
static QState ShutterAO_initial(ShutterAO *const me, QEvt const *const e);
static QState ShutterAO_active(ShutterAO *const me, QEvt const *const e);
static rt_err_t ShutterAO_handleCmd(ShutterCmd cmd);

/* 全局实例 */
ShutterAO shutterAO;

/*
 * 快门服务构造函数
 */
void ShutterAO_ctor(ShutterAO *const me)
{
    QActive_ctor(&me->super, Q_STATE_CAST(&ShutterAO_initial));
    QTimeEvt_ctorX(&me->closeTimer, &me->super, EVT_SHUTTER_CLOSE_TIMEOUT, 0U);
    me->state = SHUTTER_STATE_OPENED;
    me->retryCount = 0U;
}

/*
 * 初始状态处理函数
 */
static QState ShutterAO_initial(ShutterAO *const me, QEvt const *const e)
{
    (void)e;

    QActive_subscribe(&me->super, EVT_SHUTTER_CLOSE);
    QActive_subscribe(&me->super, EVT_SHUTTER_OPEN);
    QActive_subscribe(&me->super, EVT_SHUTTER_URGENT_CLOSE);
    QActive_subscribe(&me->super, EVT_SHUTTER_URGENT_OPEN);
    QActive_subscribe(&me->super, EVT_SEQUENCE_ROLLBACK);

    return Q_TRAN(&ShutterAO_active);
}

/*
 * 活动状态处理函数
 */
static QState ShutterAO_active(ShutterAO *const me, QEvt const *const e)
{
    QState status;
    rt_err_t ret;

    switch (e->sig)
    {
    case EVT_SHUTTER_CLOSE:
    {
        rt_kprintf("[ShutterAO] Executing normal close\n");
        QTimeEvt_armX(&me->closeTimer, SHUTTER_CLOSE_TIMEOUT, 0U);
        ret = ShutterAO_handleCmd(SHUTTER_CMD_CLOSE);
        if (RT_EOK == ret)
        {
            me->state = SHUTTER_STATE_CLOSED;
            QTimeEvt_disarm(&me->closeTimer);
            me->retryCount = 0U;
        }
        status = Q_HANDLED();
        break;
    }

    case EVT_SHUTTER_OPEN:
    {
        rt_kprintf("[ShutterAO] Executing normal open\n");
        ret = ShutterAO_handleCmd(SHUTTER_CMD_OPEN);
        if (RT_EOK == ret)
        {
            me->state = SHUTTER_STATE_OPENED;
            me->retryCount = 0U;
        }
        status = Q_HANDLED();
        break;
    }

    case EVT_SHUTTER_URGENT_CLOSE:
    {
        rt_kprintf("[ShutterAO] Executing urgent close\n");
        ret = ShutterAO_handleCmd(SHUTTER_CMD_URGENT_CLOSE);
        if (RT_EOK == ret)
        {
            me->state = SHUTTER_STATE_CLOSED;
        }
        status = Q_HANDLED();
        break;
    }

    case EVT_SHUTTER_URGENT_OPEN:
    {
        rt_kprintf("[ShutterAO] Executing urgent open\n");
        ret = ShutterAO_handleCmd(SHUTTER_CMD_URGENT_OPEN);
        if (RT_EOK == ret)
        {
            me->state = SHUTTER_STATE_OPENED;
        }
        status = Q_HANDLED();
        break;
    }

    case EVT_SHUTTER_CLOSE_TIMEOUT:
    {
        rt_kprintf("[ShutterAO] Close operation timeout!\n");
        if (me->retryCount < SHUTTER_RETRY_COUNT)
        {
            rt_kprintf("[ShutterAO] Retrying (%u/%u)\n",
                       me->retryCount + 1U, SHUTTER_RETRY_COUNT);
            me->retryCount++;
            AppEvt evt = {{EVT_SHUTTER_CLOSE}, NULL};
            QACTIVE_POST(&me->super, &evt.super, NULL);
        }
        else
        {
            rt_kprintf("[ShutterAO] Max retry count reached\n");
            me->state = SHUTTER_STATE_ERROR;
            AppEvt evt = {{EVT_SHUTTER_ERROR}, NULL};
            QACTIVE_POST(&me->super, &evt.super, NULL);
        }
        status = Q_HANDLED();
        break;
    }

    case EVT_SEQUENCE_ROLLBACK:
    {
        rt_kprintf("[ShutterAO] Rolling back to safe state\n");
        /* 回退时总是打开快门确保安全 */
        ret = ShutterAO_handleCmd(SHUTTER_CMD_URGENT_OPEN);
        if (RT_EOK == ret)
        {
            me->state = SHUTTER_STATE_OPENED;
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
 * 快门命令处理函数
 */
static rt_err_t ShutterAO_handleCmd(ShutterCmd cmd)
{
    rt_err_t ret;

    if ((cmd > SHUTTER_CMD_NONE) && (cmd < SHUTTER_CMD_MAX))
    {
        ret = rs500_shutter_control(cmd);
    }
    else
    {
        ret = -RT_EINVAL;
    }

    return ret;
}
