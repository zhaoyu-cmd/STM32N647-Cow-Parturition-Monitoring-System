#ifndef __APP_H
#define __APP_H

#include "stm32n6xx_hal.h"

void app_run(void);
void app_dcmipp_pipe_vsync_cb(void);
void app_dcmipp_pipe_frame_cb(void);

#endif
