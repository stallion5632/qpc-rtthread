/*============================================================================
* Product: Performance Tests Framework - BSP Implementation
* Last updated for version 7.3.0
* Last updated on  2025-07-18
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2025 Quantum Leaps, LLC. All rights reserved.
============================================================================*/

#include "bsp.h"
#include <rtthread.h>

/*==========================================================================*/
/* BSP Implementation for Performance Tests */
/*==========================================================================*/

void BSP_init(void)
{
    /* Minimal BSP initialization for performance testing */
    rt_kprintf("[BSP] Performance test BSP initialized\n");
}

uint32_t BSP_getTimestampMs(void)
{
    return rt_tick_get() * 1000 / RT_TICK_PER_SECOND;
}

void BSP_ledOn(void)
{
    /* LED operations are optional for performance testing */
}

void BSP_ledOff(void)
{
    /* LED operations are optional for performance testing */
}

void BSP_ledToggle(void)
{
    /* LED operations are optional for performance testing */
}
