#include "qpc.h"
#include "shutter_ao.h"
#include "isp_ao.h"
#include "app_logic.h"
#include "rs500_defs.h"
#include <rtthread.h>

// QP/C 断言模块名定义
#define Q_this_module_ "main"
#define MAX_PUB_SIG 32U  // 事件信号最大数量，可根据实际需求调整
static QSubscrList subscrSto[MAX_PUB_SIG];
/*
 * 队列及栈配置
 */
#define SHUTTER_QUEUE_SIZE      16U    /* 快门服务事件队列大小 */
#define ISP_QUEUE_SIZE          16U    /* ISP服务事件队列大小 */
#define SHUTTER_STACK_SIZE    1024U    /* 快门服务线程栈大小 */
#define ISP_STACK_SIZE        1024U    /* ISP服务线程栈大小 */

/*
 * 服务优先级定义
 */
#define SHUTTER_PRIO            3U     /* 快门服务优先级(越小越高) */
#define ISP_PRIO                4U     /* ISP服务优先级 */

/*
 * 存储区定义
 */
static QEvt const * shutterQueueSto[SHUTTER_QUEUE_SIZE];  /* 快门队列存储 */
static QEvt const * ispQueueSto[ISP_QUEUE_SIZE];          /* ISP队列存储 */
ALIGN(RT_ALIGN_SIZE) static rt_uint8_t shutterStackSto[SHUTTER_STACK_SIZE];  /* 快门栈存储 */
ALIGN(RT_ALIGN_SIZE) static rt_uint8_t ispStackSto[ISP_STACK_SIZE];          /* ISP栈存储 */

/*
 * 版本信息
 */
static const char *VERSION = "1.0.0";

/*
 * BSP初始化
 * @return RT_EOK 成功, 其他值表示错误码
 */
static rt_err_t bsp_init(void)
{
    rt_err_t ret;

    rt_kprintf("[System] RS500 QP/C Framework v%s\n", VERSION);
    rt_kprintf("[System] Build: %s %s\n", __DATE__, __TIME__);

    /* 硬件初始化 */
    ret = rs500_hw_init();
    if (RT_EOK != ret)
    {
        rt_kprintf("[System] Hardware initialization failed: %d\n", ret);
        return ret;
    }
    rt_kprintf("[System] Hardware initialized\n");

    return RT_EOK;
}

/*
 * 活动对象初始化
 * @return RT_EOK 成功, 其他值表示错误码
 */
static rt_err_t ao_init(void)
{
    /* QF框架初始化 */
    QF_init();
    QF_psInit(subscrSto, Q_DIM(subscrSto)); // 添加发布-订阅表初始化
    rt_kprintf("[System] QF framework initialized\n");

    /* 构造活动对象 */
    ShutterAO_ctor(&shutterAO);
    IspAO_ctor(&ispAO);

    /* 启动快门服务 */
    QACTIVE_START(&shutterAO.super,
                  SHUTTER_PRIO,            /* 优先级 */
                  shutterQueueSto,         /* 事件队列存储 */
                  SHUTTER_QUEUE_SIZE,      /* 队列大小 */
                  shutterStackSto,         /* 线程栈存储 */
                  sizeof(shutterStackSto), /* 栈大小 */
                  (void *)0);              /* 额外参数 */
    rt_kprintf("[System] Shutter service started, prio=%u\n", SHUTTER_PRIO);

    /* 启动ISP服务 */
    QACTIVE_START(&ispAO.super,
                  ISP_PRIO,            /* 优先级 */
                  ispQueueSto,         /* 事件队列存储 */
                  ISP_QUEUE_SIZE,      /* 队列大小 */
                  ispStackSto,         /* 线程栈存储 */
                  sizeof(ispStackSto), /* 栈大小 */
                  (void *)0);          /* 额外参数 */
    rt_kprintf("[System] ISP service started, prio=%u\n", ISP_PRIO);

    return RT_EOK;
}

#ifdef RT_USING_MSH
/*
 * MSH命令处理函数
 */
static void cmd_sequence(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: sequence <start|abort>\n");
        return;
    }

    if (rt_strcmp(argv[1], "start") == 0)
    {
        auto_shutter_sequence();
    }
    else if (rt_strcmp(argv[1], "abort") == 0)
    {
        abort_current_sequence();
    }
    else
    {
        rt_kprintf("Unknown command: %s\n", argv[1]);
    }
}
MSH_CMD_EXPORT(cmd_sequence, sequence control : start / abort);
#endif

/* 主函数入口 */
int main(void)
{
    rt_err_t ret;

    /* BSP初始化 */
    ret = bsp_init();
    if (RT_EOK != ret)
    {
        return (int)ret;
    }

    /* 活动对象初始化 */
    ret = ao_init();
    if (RT_EOK != ret)
    {
        return (int)ret;
    }

    rt_kprintf("[System] System startup completed\n");
    rt_kprintf("[System] Type 'sequence start' to begin auto sequence\n");

    /* 启动QF调度器 */
    return (int)QF_run();
}

#ifdef RT_USING_FINSH
/* 导出到FINSH的命令 */
static int sequence(int argc, char **argv)
{
    cmd_sequence(argc, argv);
    return 0;
}
FINSH_FUNCTION_EXPORT(sequence, sequence control);
MSH_CMD_EXPORT(sequence, sequence control);
#endif
