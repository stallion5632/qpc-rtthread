/*============================================================================
* Product: Board Support Package Header
* Last updated for version 7.2.0
* Last updated on  2024-12-19
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2024 Quantum Leaps, LLC. All rights reserved.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <www.gnu.org/licenses/>.
*
* Contact information:
* <www.state-machine.com/licensing>
* <info@state-machine.com>
============================================================================*/
#ifndef BSP_H_
#define BSP_H_

#include "qpc.h"
#include <rtthread.h>

/*==========================================================================*/
/* BSP Configuration */
/*==========================================================================*/
#define BSP_TICKS_PER_SEC       (1000U)   /* RT-Thread tick frequency */

/*==========================================================================*/
/* Hardware Abstraction */
/*==========================================================================*/

/* System initialization */
void BSP_init(void);

/* Time and timing functions */
uint32_t BSP_getTimestampMs(void);
uint32_t BSP_getTimestampUs(void);
void BSP_delayMs(uint32_t delay_ms);

/* LED control functions */
void BSP_ledInit(void);
void BSP_ledOn(void);
void BSP_ledOff(void);
void BSP_ledToggle(void);
rt_bool_t BSP_ledGetState(void);

/* System information */
uint32_t BSP_getCpuFreq(void);
uint32_t BSP_getMemTotal(void);
uint32_t BSP_getMemFree(void);

/* Performance monitoring */
void BSP_perfInit(void);
uint32_t BSP_perfGetCycles(void);
void BSP_perfReset(void);

/* System watchdog (if available) */
void BSP_watchdogInit(void);
void BSP_watchdogFeed(void);

/* Critical section support */
void BSP_criticalSectionEnter(void);
void BSP_criticalSectionExit(void);

/*==========================================================================*/
/* Error handling */
/*==========================================================================*/
void BSP_errorHandler(const char *file, int line, const char *func);

/* Assertion macro */
#define BSP_ASSERT(condition_) \
    do { \
        if (!(condition_)) { \
            BSP_errorHandler(__FILE__, __LINE__, __func__); \
        } \
    } while (0)

/*==========================================================================*/
/* RT-Thread specific BSP functions */
/*==========================================================================*/

/* Thread stack monitoring */
rt_size_t BSP_getThreadStackUsed(rt_thread_t thread);
rt_size_t BSP_getThreadStackFree(rt_thread_t thread);

/* System load monitoring */
uint8_t BSP_getCpuUsage(void);
uint32_t BSP_getIdleCount(void);

#endif /* BSP_H_ */