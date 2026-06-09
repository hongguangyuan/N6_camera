#ifndef CAMERA_DEBUG_H
#define CAMERA_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32n6xx_hal.h"
#include <stdint.h>

#define CAMERA_DEBUG_WIDTH 640U
#define CAMERA_DEBUG_HEIGHT 480U
#define CAMERA_DEBUG_RAW16_FRAME_BYTES (CAMERA_DEBUG_WIDTH * CAMERA_DEBUG_HEIGHT * 2U)
#define CAMERA_DEBUG_RGB565_FRAME_BYTES (CAMERA_DEBUG_WIDTH * CAMERA_DEBUG_HEIGHT * 2U)

typedef struct
{
  uint32_t sensor_id;
  uint32_t sensor_status;
  uint32_t dcmipp_status;
  uint32_t streaming;
  uint32_t frozen;
  uint32_t frame_count;
  uint32_t frame_done_tick;
  uint32_t p0_dump_bytes;
  uint32_t expected_bytes;
  uint32_t raw16_valid;
  uint32_t raw16_align;
  uint32_t raw16_min;
  uint32_t raw16_max;
  uint32_t raw16_sat10_count;
  uint32_t last_error;
} CameraDebug_Status_t;

extern DCMIPP_HandleTypeDef hdcmipp;

/*
 * Bring-up entry point for the verified IMX219 + CSI + DCMIPP snapshot path.
 * The caller must initialize GPIO, UART, I2C, clocks/RIF, and sensor power GPIO
 * ownership before calling this function.
 */
void CameraDebug_InitAndStart(I2C_HandleTypeDef *hi2c);

/*
 * Periodic debug logger. The captured frame is frozen after the first complete
 * snapshot so external GDB/OpenOCD scripts can dump a stable frame buffer.
 */
void CameraDebug_Task(void);
void CameraDebug_GetStatus(CameraDebug_Status_t *status);

/* Returns the captured 640x480 frame buffer. Current snapshot mode stores RGB565LE. */
uint8_t *CameraDebug_GetFrameBuffer(void);
uint32_t CameraDebug_GetFrameBufferSize(void);
void CameraDebug_PrintErrorContext(void);

#ifdef __cplusplus
}
#endif

#endif /* CAMERA_DEBUG_H */
