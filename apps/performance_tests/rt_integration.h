/*============================================================================
* Product: Performance Tests Application - RT-Thread Integration
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
#ifndef RT_INTEGRATION_H_
#define RT_INTEGRATION_H_

#include <rtthread.h>

/*==========================================================================*/
/* Thread-safe logging functions */
/*==========================================================================*/

/* Function to lock logging mutex - implemented in BSP */
void log_lock(void);

/* Function to unlock logging mutex - implemented in BSP */
void log_unlock(void);

#endif /* RT_INTEGRATION_H_ */