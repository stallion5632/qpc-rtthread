#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include "app_events.h"
#include "qpc.h"
#include <rtthread.h>


/*
 * 序列控制函数
 */
void auto_shutter_sequence(void);
void abort_current_sequence(void);

#endif /* APP_LOGIC_H */
