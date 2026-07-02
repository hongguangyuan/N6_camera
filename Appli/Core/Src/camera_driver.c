#include "camera_driver.h"

HAL_StatusTypeDef CameraDriver_InitAndStart(I2C_HandleTypeDef *hi2c)
{
  return CameraPipeline_InitAndStart(hi2c);
}

void CameraDriver_Task(void)
{
  CameraPipeline_Task();
}

void CameraDriver_GetDefaultImageControl(CameraDriver_ImageControl_t *control)
{
  CameraPipeline_GetDefaultImageControl(control);
}

void CameraDriver_GetImageControl(CameraDriver_ImageControl_t *control)
{
  CameraPipeline_GetImageControl(control);
}

HAL_StatusTypeDef CameraDriver_ApplyImageControl(const CameraDriver_ImageControl_t *control)
{
  return CameraPipeline_ApplyImageControl(control);
}

HAL_StatusTypeDef CameraDriver_SetImageControl(uint16_t exposure_lines,
                                               uint8_t analog_gain,
                                               uint16_t digital_gain,
                                               uint32_t wb_red_gain,
                                               uint32_t wb_green_gain,
                                               uint32_t wb_blue_gain,
                                               uint8_t gamma_enable)
{
  CameraDriver_ImageControl_t control;

  control.exposure_lines = exposure_lines;
  control.analog_gain = analog_gain;
  control.digital_gain = digital_gain;
  control.wb_red_gain = wb_red_gain;
  control.wb_green_gain = wb_green_gain;
  control.wb_blue_gain = wb_blue_gain;
  control.gamma_enable = gamma_enable;

  return CameraPipeline_ApplyImageControl(&control);
}

void CameraDriver_GetStatus(CameraDriver_Status_t *status)
{
  CameraPipeline_GetStatus(status);
}

uint8_t *CameraDriver_GetFrameBuffer(void)
{
  return CameraPipeline_GetFrameBuffer();
}

uint32_t CameraDriver_GetFrameBufferSize(void)
{
  return CameraPipeline_GetFrameBufferSize();
}

void CameraDriver_PrintErrorContext(void)
{
  CameraPipeline_PrintErrorContext();
}
