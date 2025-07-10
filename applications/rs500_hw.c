#include "rs500_defs.h"
#include <rtthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* 模拟硬件状态 */
static struct
{
    uint8_t shutter_opened; /* 快门状态: 1=打开, 0=关闭 */
    uint8_t isp_running;    /* ISP状态: 1=运行, 0=停止 */
} dev = {1, 0};             /* 默认快门打开,ISP停止 */

/*
 * 硬件初始化
 */
rt_err_t rs500_hw_init(void)
{
    rt_kprintf("[HW] RS500 simulation initialized\n");
    return RT_EOK;
}

/*
 * 快门控制
 */
rt_err_t rs500_shutter_control(ShutterCmd cmd)
{
    switch (cmd)
    {
    case SHUTTER_CMD_OPEN:
    case SHUTTER_CMD_URGENT_OPEN:
        dev.shutter_opened = 1;
        rt_thread_mdelay(10); /* 简单延时 */
        rt_kprintf("[HW] Shutter opened\n");
        break;

    case SHUTTER_CMD_CLOSE:
    case SHUTTER_CMD_URGENT_CLOSE:
        dev.shutter_opened = 0;
        rt_thread_mdelay(10); /* 简单延时 */
        rt_kprintf("[HW] Shutter closed\n");
        break;

    default:
        return -RT_ERROR;
    }

    return RT_EOK;
}

/*
 * ISP控制
 */
rt_err_t rs500_isp_control(IspCmd cmd)
{
    switch (cmd)
    {
    case ISP_CMD_STOP_TECLESS_B:
        dev.isp_running = 0;
        rt_thread_mdelay(10);
        rt_kprintf("[HW] ISP stopped\n");
        break;

    case ISP_CMD_START_TECLESS_B:
        dev.isp_running = 1;
        rt_thread_mdelay(10);
        rt_kprintf("[HW] ISP started\n");
        break;

    case ISP_CMD_UPDATE_B_PREPARE:
        rt_thread_mdelay(10);
        rt_kprintf("[HW] ISP preparing update\n");
        break;

    case ISP_CMD_UPDATE_B:
        rt_thread_mdelay(10);
        rt_kprintf("[HW] ISP updating\n");
        break;

    case ISP_CMD_UPDATE_B_END:
        rt_thread_mdelay(10);
        rt_kprintf("[HW] ISP update completed\n");
        break;

    default:
        return -RT_ERROR;
    }

    return RT_EOK;
}

#ifdef RT_USING_MSH
/* 测试命令 */
static void rs500(int argc, char **argv)
{
    if (argc < 3)
    {
        rt_kprintf("Usage: rs500 s <1=open|2=close>\n");
        rt_kprintf("       rs500 i <1=start|2=stop>\n");
        return;
    }

    if (argv[1][0] == 's')
    {
        rs500_shutter_control((ShutterCmd)atoi(argv[2]));
    }
    else if (argv[1][0] == 'i')
    {
        rs500_isp_control((IspCmd)atoi(argv[2]));
    }
}
MSH_CMD_EXPORT(rs500, rs500 hardware test);
#endif
