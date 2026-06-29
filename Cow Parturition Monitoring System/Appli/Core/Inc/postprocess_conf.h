/*---------------------------------------------------------------------------------------------
#  * Copyright (c) 2023 STMicroelectronics.
#  * All rights reserved.
#  *
#  * This software is licensed under terms that can be found in the LICENSE file in
#  * the root directory of this software component.
#  * If no LICENSE file comes with this software, it is provided AS-IS.
#  *--------------------------------------------------------------------------------------------*/

/* ---------------    Generated code    ----------------- */
#ifndef __POSTPROCESS_CONF_H__
#define __POSTPROCESS_CONF_H__

#include "arm_math.h"

/* I/O configuration */
#define AI_OBJDETECT_YOLOV2_PP_NB_CLASSES        (3)
#define AI_OBJDETECT_YOLOV2_PP_NB_ANCHORS        (5)
#define AI_OBJDETECT_YOLOV2_PP_GRID_WIDTH        (7)
#define AI_OBJDETECT_YOLOV2_PP_GRID_HEIGHT       (7)
#define AI_OBJDETECT_YOLOV2_PP_NB_INPUT_BOXES    (AI_OBJDETECT_YOLOV2_PP_GRID_WIDTH * AI_OBJDETECT_YOLOV2_PP_GRID_HEIGHT)

/* Anchor boxes */
static const float32_t AI_OBJDETECT_YOLOV2_PP_ANCHORS[2*AI_OBJDETECT_YOLOV2_PP_NB_ANCHORS] = {
    0.1384000000f,     0.2333000000f,
    0.2076000000f,     0.2500000000f,
    0.3562000000f,     0.4516000000f,
    0.3317000000f,     0.9120000000f,
    0.8190000000f,     1.6509000000f,
  };

#endif      /* __POSTPROCESS_CONF_H__  */

