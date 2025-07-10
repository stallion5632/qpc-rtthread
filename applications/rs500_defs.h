#ifndef RS500_DEFS_H
#define RS500_DEFS_H
#include <rtthread.h>
/*
 * 硬件命令定义
 */
typedef enum ShutterCmd {
  SHUTTER_CMD_NONE = 0,
  SHUTTER_CMD_OPEN,
  SHUTTER_CMD_CLOSE,
  SHUTTER_CMD_URGENT_OPEN,
  SHUTTER_CMD_URGENT_CLOSE,
  SHUTTER_CMD_MAX
} ShutterCmd;

typedef enum IspCmd {
  ISP_CMD_NONE = 0,
  ISP_CMD_STOP_TECLESS_B,
  ISP_CMD_START_TECLESS_B,
  ISP_CMD_UPDATE_B_PREPARE,
  ISP_CMD_UPDATE_B,
  ISP_CMD_UPDATE_B_END,
  ISP_CMD_MAX
} IspCmd;

/*
 * 状态定义
 */
typedef enum ShutterState {
  SHUTTER_STATE_CLOSED = 0,
  SHUTTER_STATE_OPENED = 1,
  SHUTTER_STATE_ERROR = 2
} ShutterState;

typedef enum IspState {
  ISP_STATE_IDLE = 0,
  ISP_STATE_BUSY = 1,
  ISP_STATE_ERROR = 2
} IspState;

/*
 * 硬件接口声明
 */
rt_err_t rs500_hw_init(void);
rt_err_t rs500_shutter_control(ShutterCmd cmd);
rt_err_t rs500_isp_control(IspCmd cmd);

#endif /* RS500_DEFS_H */
