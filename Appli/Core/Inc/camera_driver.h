#ifndef CAMERA_DRIVER_H
#define CAMERA_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "camera_pipeline.h"

typedef CameraPipeline_ImageControl_t CameraDriver_ImageControl_t;
typedef CameraPipeline_Status_t CameraDriver_Status_t;

#define CAMERA_DRIVER_WB_GAIN_1X CAMERA_PIPELINE_WB_GAIN_1X

HAL_StatusTypeDef CameraDriver_InitAndStart(I2C_HandleTypeDef *hi2c);
void CameraDriver_Task(void);
void CameraDriver_GetDefaultImageControl(CameraDriver_ImageControl_t *control);
void CameraDriver_GetImageControl(CameraDriver_ImageControl_t *control);
HAL_StatusTypeDef CameraDriver_ApplyImageControl(const CameraDriver_ImageControl_t *control);
HAL_StatusTypeDef CameraDriver_SetImageControl(uint16_t exposure_lines,
                                               uint8_t analog_gain,
                                               uint16_t digital_gain,
                                               uint32_t wb_red_gain,
                                               uint32_t wb_green_gain,
                                               uint32_t wb_blue_gain,
                                               uint8_t gamma_enable);
void CameraDriver_GetStatus(CameraDriver_Status_t *status);
uint8_t *CameraDriver_GetFrameBuffer(void);
uint32_t CameraDriver_GetFrameBufferSize(void);
void CameraDriver_PrintErrorContext(void);

#ifdef __cplusplus
}
#endif

#endif /* CAMERA_DRIVER_H */
