/*============================================================================
* Product: Performance Tests Application - System Logging
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
#ifndef SYS_LOG_H_
#define SYS_LOG_H_

#include <rtthread.h>

#define SYS_LOG_I(fmt, ...) rt_kprintf("[INFO ] " fmt "\r\n", ##__VA_ARGS__)
#define SYS_LOG_D(fmt, ...) rt_kprintf("[DEBUG] " fmt "\r\n", ##__VA_ARGS__)
#define SYS_LOG_E(fmt, ...) rt_kprintf("[ERROR] " fmt "\r\n", ##__VA_ARGS__)

#endif /* SYS_LOG_H_ */
