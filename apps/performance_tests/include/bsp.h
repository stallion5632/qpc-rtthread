/*============================================================================
* Product: Performance Tests Framework - BSP Header
* Last updated for version 7.3.0
* Last updated on  2025-07-18
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2025 Quantum Leaps, LLC. All rights reserved.
============================================================================*/
#ifndef BSP_H_
#define BSP_H_

#include "qpc.h"
#include <rtthread.h>

/*==========================================================================*/
/* BSP API for Performance Tests */
/*==========================================================================*/

/* BSP initialization */
void BSP_init(void);

/* Timestamp functions */
uint32_t BSP_getTimestampMs(void);

/* LED control functions (optional for performance testing) */
void BSP_ledOn(void);
void BSP_ledOff(void);
void BSP_ledToggle(void);

#endif /* BSP_H_ */
