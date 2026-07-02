#include "camera_pipeline.h"
#include "imx219.h"
#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
#include "imx219_isp_param_conf.h"
#include "isp_api.h"
#endif
#include "main.h"
#include <stdio.h>
#include <string.h>

#define CAMERA_PIPELINE_SENSOR_RESOLUTION IMX219_R1640_1232
#define CAMERA_PIPELINE_PIXEL_FORMAT IMX219_RAW10
#define CAMERA_PIPELINE_LANE_COUNT 2U
#define CAMERA_PIPELINE_BUFFER_ADDRESS 0x34082000U
#define CAMERA_PIPELINE_BUFFER_BYTES (1024U * 1024U)
#define CAMERA_PIPELINE_PITCH_BYTES CAMERA_PIPELINE_OUTPUT_PITCH_BYTES
#define CAMERA_PIPELINE_RAW10_LINE_BYTES ((CAMERA_PIPELINE_INPUT_WIDTH * 10U) / 8U)
#define CAMERA_PIPELINE_CSI_LINE_BYTE_PROBE 1U
#define CAMERA_PIPELINE_PIPE1_FORCE_RAW10_FORMAT 1U
#define CAMERA_PIPELINE_PIPE1_EFFECTIVE_SOURCE_WIDTH CAMERA_PIPELINE_BAYER2RGB_OUTPUT_WIDTH
#define CAMERA_PIPELINE_PIPE1_EFFECTIVE_SOURCE_HEIGHT CAMERA_PIPELINE_BAYER2RGB_OUTPUT_HEIGHT
#define CAMERA_PIPELINE_FORCE_ISP_DECIMATION2_REQUEST 0U
#define CAMERA_PIPELINE_FORCE_PIPE1_DECIMATION2_REQUEST 1U
#define CAMERA_PIPELINE_ENABLE_SENSOR_TEST_PATTERN 0U
#define CAMERA_PIPELINE_SENSOR_TEST_PATTERN_MODE 2U
#define CAMERA_PIPELINE_MANUAL_BRIGHTNESS_VERIFY 1U
#define CAMERA_PIPELINE_MANUAL_EXPOSURE_US 20000
#define CAMERA_PIPELINE_MANUAL_GAIN_MDB 18000
#define CAMERA_PIPELINE_LINE_TIME_NS_DEFAULT 33367U
#if ((CAMERA_PIPELINE_PIPE0_RAW16_DEBUG != 0U) || (CAMERA_PIPELINE_PIPE0_RAW10P_DEBUG != 0U))
#define CAMERA_PIPELINE_PIPE0_DEBUG 1U
#else
#define CAMERA_PIPELINE_PIPE0_DEBUG 0U
#endif
#if ((CAMERA_PIPELINE_FORCE_ISP_DECIMATION2_REQUEST != 0U) && (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U))
#define CAMERA_PIPELINE_FORCE_ISP_DECIMATION2 1U
#else
#define CAMERA_PIPELINE_FORCE_ISP_DECIMATION2 0U
#endif
#if ((CAMERA_PIPELINE_FORCE_PIPE1_DECIMATION2_REQUEST != 0U) && \
     (CAMERA_PIPELINE_PIPE1_GRAY_DECIM_ONLY_DEBUG != 0U))
#define CAMERA_PIPELINE_FORCE_PIPE1_DECIMATION2 1U
#else
#define CAMERA_PIPELINE_FORCE_PIPE1_DECIMATION2 0U
#endif
#if ((CAMERA_PIPELINE_PIPE0_DEBUG == 0U) && \
     (CAMERA_PIPELINE_PIPE1_RAWBAYER_ONLY_DEBUG == 0U) && \
     (CAMERA_PIPELINE_PIPE1_GRAY_CROP_ONLY_DEBUG == 0U))
#define CAMERA_PIPELINE_DOWNSIZE_SOURCE_WIDTH CAMERA_PIPELINE_PIPE1_EFFECTIVE_SOURCE_WIDTH
#define CAMERA_PIPELINE_DOWNSIZE_SOURCE_HEIGHT CAMERA_PIPELINE_PIPE1_EFFECTIVE_SOURCE_HEIGHT
#elif ((CAMERA_PIPELINE_ISP_GEOMETRY_PROBE != 0U) && \
       ((CAMERA_PIPELINE_FORCE_ISP_DECIMATION2 != 0U) || (CAMERA_PIPELINE_FORCE_PIPE1_DECIMATION2 != 0U)))
#define CAMERA_PIPELINE_DOWNSIZE_SOURCE_WIDTH (CAMERA_PIPELINE_INPUT_WIDTH / 2U)
#define CAMERA_PIPELINE_DOWNSIZE_SOURCE_HEIGHT (CAMERA_PIPELINE_INPUT_HEIGHT / 2U)
#elif (CAMERA_PIPELINE_ISP_GEOMETRY_PROBE != 0U)
#define CAMERA_PIPELINE_DOWNSIZE_SOURCE_WIDTH CAMERA_PIPELINE_INPUT_WIDTH
#define CAMERA_PIPELINE_DOWNSIZE_SOURCE_HEIGHT CAMERA_PIPELINE_INPUT_HEIGHT
#elif (CAMERA_PIPELINE_PIPE1_DOWNSIZE_820X320_TO_640X240_DEBUG != 0U)
#define CAMERA_PIPELINE_DOWNSIZE_SOURCE_WIDTH CAMERA_PIPELINE_BAYER2RGB_OUTPUT_WIDTH
#define CAMERA_PIPELINE_DOWNSIZE_SOURCE_HEIGHT 320U
#else
#define CAMERA_PIPELINE_DOWNSIZE_SOURCE_WIDTH (CAMERA_PIPELINE_INPUT_WIDTH / 2U)
#define CAMERA_PIPELINE_DOWNSIZE_SOURCE_HEIGHT (CAMERA_PIPELINE_INPUT_HEIGHT / 2U)
#endif
#define CAMERA_PIPELINE_RGB565_SNAPSHOT_VERIFY 1U
#define CAMERA_PIPELINE_CONTINUOUS_FREEZE_PROBE 1U
#define CAMERA_PIPELINE_FREEZE_MIN_WAIT_MS 120U
#define CAMERA_PIPELINE_FREEZE_TIMEOUT_MS 5000U
#define CAMERA_PIPELINE_TAIL_READY_LINES 16U
#if (CAMERA_PIPELINE_PIPE0_DEBUG != 0U)
#define CAMERA_PIPELINE_FREEZE_AFTER_FRAMES 0xFFFFFFFFU
#elif (CAMERA_PIPELINE_RAW_GRAY_DEBUG != 0U)
#define CAMERA_PIPELINE_FREEZE_AFTER_FRAMES 1U
#elif ((CAMERA_PIPELINE_CONTINUOUS_FREEZE_PROBE != 0U) && \
       (CAMERA_PIPELINE_ENABLE_SENSOR_TEST_PATTERN == 0U))
#define CAMERA_PIPELINE_FREEZE_AFTER_FRAMES 90U
#elif (CAMERA_PIPELINE_ISP_GEOMETRY_PROBE != 0U)
#define CAMERA_PIPELINE_ISP_WARMUP_FRAMES 60U
#define CAMERA_PIPELINE_FREEZE_AFTER_FRAMES 1U
#elif (CAMERA_PIPELINE_RGB565_SNAPSHOT_VERIFY != 0U)
#define CAMERA_PIPELINE_FREEZE_AFTER_FRAMES 1U
#else
#define CAMERA_PIPELINE_FREEZE_AFTER_FRAMES 3U
#endif
#define CAMERA_PIPELINE_LINE_LENGTH 20000U
#define CAMERA_PIPELINE_FRAME_LENGTH 3000U
#define CAMERA_PIPELINE_FIXED_ANALOG_GAIN 0x80U
#define CAMERA_PIPELINE_FIXED_EXPOSURE_LINES 0x012CU
#define CAMERA_PIPELINE_FIXED_DIGITAL_GAIN 0x0100U
#define CAMERA_PIPELINE_USE_LOW_LINK 1U
#define CAMERA_PIPELINE_LOW_LINK_OP_PLL_MULT 0x0039U
#define CAMERA_PIPELINE_UART_DUMP_AFTER_FREEZE 1U
#define CAMERA_PIPELINE_UART_DUMP_BYTES_PER_LINE 64U
#if ((CAMERA_PIPELINE_PIPE0_DEBUG != 0U) || \
     (CAMERA_PIPELINE_PIPE1_RAWBAYER_ONLY_DEBUG != 0U) || \
     (CAMERA_PIPELINE_PIPE1_DOWNSIZE_820X320_TO_640X240_DEBUG != 0U))
#define CAMERA_PIPELINE_UART_DUMP_REPEAT_COUNT 1U
#else
#define CAMERA_PIPELINE_UART_DUMP_REPEAT_COUNT 3U
#endif
#define CAMERA_PIPELINE_UART_DUMP_FIRST_DELAY_MS 1000U
#define CAMERA_PIPELINE_UART_DUMP_REPEAT_DELAY_MS 3000U
#define CAMERA_PIPELINE_PERIODIC_CAPTURE_ENABLE 1U
#define CAMERA_PIPELINE_PERIODIC_CAPTURE_INTERVAL_MS 120000U
#if (CAMERA_PIPELINE_USE_LOW_LINK != 0U)
#define CAMERA_PIPELINE_LINE_TIME_NS \
  ((CAMERA_PIPELINE_LINE_TIME_NS_DEFAULT * 0x0072U) / CAMERA_PIPELINE_LOW_LINK_OP_PLL_MULT)
#else
#define CAMERA_PIPELINE_LINE_TIME_NS CAMERA_PIPELINE_LINE_TIME_NS_DEFAULT
#endif
#define CAMERA_PIPELINE_PIPE1_BAYER DCMIPP_RAWBAYER_RGGB
#define CAMERA_PIPELINE_PIPE1_BAYER_STRENGTH DCMIPP_RAWBAYER_ALGO_STRENGTH_8
#define CAMERA_PIPELINE_PIPE1_ENABLE_WB_EXPOSURE 1U
#define CAMERA_PIPELINE_PIPE1_GAIN_1X 100000000U
#define CAMERA_PIPELINE_PIPE1_WB_R_GAIN 137500000U
#define CAMERA_PIPELINE_PIPE1_WB_G_GAIN CAMERA_PIPELINE_PIPE1_GAIN_1X
#define CAMERA_PIPELINE_PIPE1_WB_B_GAIN 125000000U
#define CAMERA_PIPELINE_MASK_DPHY_IRQ 1U
#define CAMERA_PIPELINE_CSI_DPHY_DATA_LANE_ERROR_IT                                      \
  (DCMIPP_CSI_IT_ECTRLDL1 | DCMIPP_CSI_IT_ESYNCESCDL1 | DCMIPP_CSI_IT_EESCDL1 |          \
   DCMIPP_CSI_IT_ESOTSYNCDL1 | DCMIPP_CSI_IT_ESOTDL1 | DCMIPP_CSI_IT_ECTRLDL0 |          \
   DCMIPP_CSI_IT_ESYNCESCDL0 | DCMIPP_CSI_IT_EESCDL0 | DCMIPP_CSI_IT_ESOTSYNCDL0 |       \
   DCMIPP_CSI_IT_ESOTDL0)
#if ((CAMERA_PIPELINE_CONTINUOUS_FREEZE_PROBE == 0U) && \
     (CAMERA_PIPELINE_ISP_GEOMETRY_PROBE != 0U) && \
     (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U))
#define CAMERA_PIPELINE_ISP_WARMUP_ENABLE 1U
#else
#define CAMERA_PIPELINE_ISP_WARMUP_ENABLE 0U
#endif
#define CAMERA_PIPELINE_IMX219_REG_MODE_SELECT 0x0100U
#define CAMERA_PIPELINE_IMX219_REG_CSI_LANE_MODE 0x0114U
#define CAMERA_PIPELINE_IMX219_REG_X_ADD_STA 0x0164U
#define CAMERA_PIPELINE_IMX219_REG_X_ADD_END 0x0166U
#define CAMERA_PIPELINE_IMX219_REG_Y_ADD_STA 0x0168U
#define CAMERA_PIPELINE_IMX219_REG_Y_ADD_END 0x016AU
#define CAMERA_PIPELINE_IMX219_REG_X_OUTPUT_SIZE 0x016CU
#define CAMERA_PIPELINE_IMX219_REG_Y_OUTPUT_SIZE 0x016EU
#define CAMERA_PIPELINE_IMX219_REG_X_ODD_INC 0x0170U
#define CAMERA_PIPELINE_IMX219_REG_Y_ODD_INC 0x0171U
#define CAMERA_PIPELINE_IMX219_REG_BINNING_MODE_H 0x0174U
#define CAMERA_PIPELINE_IMX219_REG_BINNING_MODE_V 0x0175U
#define CAMERA_PIPELINE_IMX219_REG_CSI_DATA_FORMAT_A 0x018CU
#define CAMERA_PIPELINE_IMX219_REG_CSI_DATA_FORMAT_B 0x018DU
#define CAMERA_PIPELINE_IMX219_REG_LINE_LENGTH_A 0x0162U
#define CAMERA_PIPELINE_IMX219_REG_FRAME_LENGTH_A 0x0160U
#define CAMERA_PIPELINE_IMX219_REG_OP_PIX_CLK_DIV 0x0309U
#define CAMERA_PIPELINE_IMX219_REG_OP_SYS_CLK_DIV 0x030BU
#define CAMERA_PIPELINE_IMX219_REG_OP_PLL_MPY 0x030CU
#define CAMERA_PIPELINE_IMX219_REG_TEST_PATTERN 0x0600U
#define CAMERA_PIPELINE_IMX219_REG_TP_WINDOW_WIDTH 0x0624U
#define CAMERA_PIPELINE_IMX219_REG_TP_WINDOW_HEIGHT 0x0626U
#define CAMERA_PIPELINE_FRAME_WORDS ((CAMERA_PIPELINE_FRAME_BYTES + 3U) / 4U)
#if (CAMERA_PIPELINE_PIPE0_DEBUG != 0U)
#define CAMERA_PIPELINE_DCC_DONE_BYTES (CAMERA_PIPELINE_OUTPUT_LINE_BYTES * CAMERA_PIPELINE_OUTPUT_HEIGHT)
#else
#define CAMERA_PIPELINE_DCC_DONE_BYTES CAMERA_PIPELINE_FRAME_BYTES
#endif
#define CAMERA_PIPELINE_DCC_DONE_WORDS ((CAMERA_PIPELINE_DCC_DONE_BYTES + 3U) / 4U)
#if (CAMERA_PIPELINE_PIPE0_DEBUG != 0U)
#define CAMERA_PIPELINE_PIPE0_CROP_HSTART 0U
#define CAMERA_PIPELINE_PIPE0_CROP_VSTART 0U
#else
#define CAMERA_PIPELINE_PIPE0_CROP_HSTART 0U
#define CAMERA_PIPELINE_PIPE0_CROP_VSTART 0U
#endif
#define CAMERA_PIPELINE_PIPE0_CROP_HSIZE CAMERA_PIPELINE_OUTPUT_WIDTH

#if (CAMERA_PIPELINE_PIPE0_DEBUG != 0U)
#define CAMERA_PIPELINE_CAPTURE_PIPE DCMIPP_PIPE0
#define CAMERA_PIPELINE_CAPTURE_MODE DCMIPP_MODE_CONTINUOUS
#if (CAMERA_PIPELINE_PIPE0_RAW10P_DEBUG != 0U)
#define CAMERA_PIPELINE_UART_BEGIN_TAG "RAW10P_UART_DUMP_BEGIN"
#define CAMERA_PIPELINE_UART_DATA_TAG "RAW10P_UART_DUMP_DATA"
#define CAMERA_PIPELINE_UART_END_TAG "RAW10P_UART_DUMP_END"
#else
#define CAMERA_PIPELINE_UART_BEGIN_TAG "RAW16_UART_DUMP_BEGIN"
#define CAMERA_PIPELINE_UART_DATA_TAG "RAW16_UART_DUMP_DATA"
#define CAMERA_PIPELINE_UART_END_TAG "RAW16_UART_DUMP_END"
#endif
#elif (CAMERA_PIPELINE_RAW_GRAY_DEBUG != 0U)
#define CAMERA_PIPELINE_CAPTURE_PIPE DCMIPP_PIPE1
#if (CAMERA_PIPELINE_CONTINUOUS_FREEZE_PROBE != 0U)
#define CAMERA_PIPELINE_CAPTURE_MODE DCMIPP_MODE_CONTINUOUS
#else
#define CAMERA_PIPELINE_CAPTURE_MODE DCMIPP_MODE_SNAPSHOT
#endif
#define CAMERA_PIPELINE_UART_BEGIN_TAG "GRAY8_UART_DUMP_BEGIN"
#define CAMERA_PIPELINE_UART_DATA_TAG "GRAY8_UART_DUMP_DATA"
#define CAMERA_PIPELINE_UART_END_TAG "GRAY8_UART_DUMP_END"
#else
#define CAMERA_PIPELINE_CAPTURE_PIPE DCMIPP_PIPE1
#if (CAMERA_PIPELINE_CONTINUOUS_FREEZE_PROBE != 0U)
#define CAMERA_PIPELINE_CAPTURE_MODE DCMIPP_MODE_CONTINUOUS
#elif (CAMERA_PIPELINE_ISP_GEOMETRY_PROBE != 0U)
#define CAMERA_PIPELINE_CAPTURE_MODE DCMIPP_MODE_SNAPSHOT
#elif ((CAMERA_PIPELINE_RGB565_SNAPSHOT_VERIFY != 0U) || (CAMERA_PIPELINE_PIPE1_RAWBAYER_ONLY_DEBUG != 0U))
#define CAMERA_PIPELINE_CAPTURE_MODE DCMIPP_MODE_SNAPSHOT
#else
#define CAMERA_PIPELINE_CAPTURE_MODE DCMIPP_MODE_CONTINUOUS
#endif
#define CAMERA_PIPELINE_UART_BEGIN_TAG "RGB565_UART_DUMP_BEGIN"
#define CAMERA_PIPELINE_UART_DATA_TAG "RGB565_UART_DUMP_DATA"
#define CAMERA_PIPELINE_UART_END_TAG "RGB565_UART_DUMP_END"
#endif

#define DCMIPP_DOWNSIZE_DIV_FACTOR(SRC, DST) \
  ((((uint32_t)((1024U * (DST)) / (SRC))) > 1023U) ? 1023U : ((uint32_t)((1024U * (DST)) / (SRC))))
#define DCMIPP_DOWNSIZE_RATIO(SRC, DST) \
  ((((uint32_t)(((SRC) * 8192U) / (DST))) < 8192U) ? 8192U : ((uint32_t)(((SRC) * 8192U) / (DST))))

DCMIPP_HandleTypeDef hdcmipp;

static IMX219_Object_t hcamera_pipeline;
#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
static ISP_HandleTypeDef hcamera_isp;
#endif
static CameraPipeline_Status_t pipeline_status = { CAMERA_PIPELINE_STATE_RESET, 0U, 0U, 0U };
#if ((CAMERA_PIPELINE_MANUAL_BRIGHTNESS_VERIFY != 0U) && \
     (CAMERA_PIPELINE_ENABLE_SENSOR_TEST_PATTERN == 0U))
static int32_t isp_gain = CAMERA_PIPELINE_MANUAL_GAIN_MDB;
static int32_t isp_exposure = CAMERA_PIPELINE_MANUAL_EXPOSURE_US;
#else
static int32_t isp_gain = 6000;
static int32_t isp_exposure = 10000;
#endif
static uint32_t pipeline_last_log_tick;
static uint32_t pipeline_sof_count;
static uint32_t pipeline_eof_count;
static uint32_t pipeline_frozen;
static uint32_t pipeline_freeze_pending;
static uint32_t pipeline_freeze_tick;
static uint32_t pipeline_uart_dumped;
static uint32_t pipeline_uart_next_dump_tick;
static uint32_t pipeline_next_capture_tick;
static uint32_t pipeline_capture_cycle;
static uint32_t pipeline_sensor_streaming;
#if (CAMERA_PIPELINE_ISP_WARMUP_ENABLE != 0U)
static uint32_t pipeline_probe_warmup_done;
#endif
static volatile uint32_t pipeline_p0_limit_count;
static volatile uint32_t pipeline_p0_limit_tick;
static volatile uint32_t pipeline_p0_limit_dcc;
#if (CAMERA_PIPELINE_CSI_LINE_BYTE_PROBE != 0U)
static volatile uint32_t pipeline_csi_lb_count[4];
#endif

static HAL_StatusTypeDef CameraPipeline_BringUpSensor(I2C_HandleTypeDef *hi2c);
static HAL_StatusTypeDef CameraPipeline_DCMIPP_Init(void);
static HAL_StatusTypeDef CameraPipeline_StartCaptureCycle(void);
#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
static ISP_StatusTypeDef GetSensorInfoHelper(uint32_t Instance, ISP_SensorInfoTypeDef *SensorInfo);
static ISP_StatusTypeDef SetSensorGainHelper(uint32_t Instance, int32_t Gain);
static ISP_StatusTypeDef GetSensorGainHelper(uint32_t Instance, int32_t *Gain);
static ISP_StatusTypeDef SetSensorExposureHelper(uint32_t Instance, int32_t Exposure);
static ISP_StatusTypeDef GetSensorExposureHelper(uint32_t Instance, int32_t *Exposure);
static ISP_StatusTypeDef SetSensorTestPatternHelper(uint32_t Instance, int32_t Mode);
static uint8_t CameraPipeline_GainMdBToAnalogReg(int32_t GainMdB);
static uint16_t CameraPipeline_ExposureUsToLines(int32_t ExposureUs);
static void CameraPipeline_PrintIspGeometry(const char *tag);
#endif
static void CameraPipeline_PrintDcmippPipe1Regs(const char *tag);
static void CameraPipeline_PrintDcmippPipe0Regs(const char *tag);
static void CameraPipeline_PrintCsiRegs(const char *tag);
static void CameraPipeline_PrintImx219Regs(const char *tag);
static void CameraPipeline_PrintFrameBufferStats(void);
static void CameraPipeline_DumpFrameBufferOverUart(void);
static uint32_t CameraPipeline_BufferTailReady(void);
static void CameraPipeline_ToExposureShiftMultiplier(uint32_t gain, uint8_t *shift, uint8_t *multiplier);

HAL_StatusTypeDef CameraPipeline_InitAndStart(I2C_HandleTypeDef *hi2c)
{
#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
  ISP_AppliHelpersTypeDef appli_helpers = {0};
  ISP_StatusTypeDef isp_status;
#if (CAMERA_PIPELINE_FORCE_ISP_DECIMATION2 != 0U)
  DCMIPP_DecimationConfTypeDef isp_decimation_conf = {0};
#endif
#endif

  memset(&pipeline_status, 0, sizeof(pipeline_status));
  pipeline_status.state = CAMERA_PIPELINE_STATE_SCAFFOLD_READY;
  pipeline_sensor_streaming = 0U;
#if (CAMERA_PIPELINE_ISP_WARMUP_ENABLE != 0U)
  pipeline_probe_warmup_done = 0U;
#endif

  printf("\r\nCSI IMX219 %s pipeline start\r\n",
         (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U) ? "ISP" : "DCMIPP");
  printf("%s pipeline: input=%lux%lu RAW10 downsize_src=%lux%lu output=%lux%lu %s buffer=0x%08lX pitch=%lu\r\n",
         (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U) ? "ISP" : "DCMIPP",
         (uint32_t)CAMERA_PIPELINE_INPUT_WIDTH,
         (uint32_t)CAMERA_PIPELINE_INPUT_HEIGHT,
         (uint32_t)CAMERA_PIPELINE_DOWNSIZE_SOURCE_WIDTH,
         (uint32_t)CAMERA_PIPELINE_DOWNSIZE_SOURCE_HEIGHT,
         (uint32_t)CAMERA_PIPELINE_OUTPUT_WIDTH,
         (uint32_t)CAMERA_PIPELINE_OUTPUT_HEIGHT,
         CAMERA_PIPELINE_OUTPUT_FORMAT_NAME,
         (uint32_t)CAMERA_PIPELINE_BUFFER_ADDRESS,
         (uint32_t)CAMERA_PIPELINE_PITCH_BYTES);
  printf("%s geometry probe: sensor_info=%lux%lu raw_effective=%lux%lu test_pattern_mode=%lu isp_runtime=%lu\r\n",
         (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U) ? "ISP" : "DCMIPP",
         (uint32_t)CAMERA_PIPELINE_INPUT_WIDTH,
         (uint32_t)CAMERA_PIPELINE_INPUT_HEIGHT,
         (uint32_t)CAMERA_PIPELINE_DOWNSIZE_SOURCE_WIDTH,
         (uint32_t)CAMERA_PIPELINE_DOWNSIZE_SOURCE_HEIGHT,
         (uint32_t)((CAMERA_PIPELINE_ENABLE_SENSOR_TEST_PATTERN != 0U) ? CAMERA_PIPELINE_SENSOR_TEST_PATTERN_MODE : 0U),
         (uint32_t)CAMERA_PIPELINE_USE_ISP_RUNTIME);
  printf("ISP geometry probe mode=%lu\r\n", (uint32_t)CAMERA_PIPELINE_ISP_GEOMETRY_PROBE);
  printf("ISP decimation override: force_1_out_2=%lu\r\n",
         (uint32_t)CAMERA_PIPELINE_FORCE_ISP_DECIMATION2);
  printf("CSI link profile: phy=%s low_link=%lu op_pll_mult=%s\r\n",
#if (CAMERA_PIPELINE_USE_LOW_LINK != 0U)
         "BT_450",
#else
         "BT_1600",
#endif
         (uint32_t)CAMERA_PIPELINE_USE_LOW_LINK,
#if (CAMERA_PIPELINE_USE_LOW_LINK != 0U)
         "0x0039"
#else
         "sensor_default"
#endif
         );
#if (CAMERA_PIPELINE_PIPE0_DEBUG != 0U)
  printf("PIPE0 %s crop: hsize=%lu vsize=%lu line_bytes=%lu pitch=%lu dump_bytes=%lu dcc_done_bytes=%lu limit_words=%lu\r\n",
         CAMERA_PIPELINE_OUTPUT_FORMAT_NAME,
         (uint32_t)CAMERA_PIPELINE_PIPE0_CROP_HSIZE,
         (uint32_t)CAMERA_PIPELINE_OUTPUT_HEIGHT,
         (uint32_t)CAMERA_PIPELINE_OUTPUT_LINE_BYTES,
         (uint32_t)CAMERA_PIPELINE_PITCH_BYTES,
         (uint32_t)CAMERA_PIPELINE_FRAME_BYTES,
         (uint32_t)CAMERA_PIPELINE_DCC_DONE_BYTES,
         (uint32_t)CAMERA_PIPELINE_DCC_DONE_WORDS);
  printf("PIPE0 %s crop window: hstart=%lu vstart=%lu hsize=%lu vsize=%lu\r\n",
         CAMERA_PIPELINE_OUTPUT_FORMAT_NAME,
         (uint32_t)CAMERA_PIPELINE_PIPE0_CROP_HSTART,
         (uint32_t)CAMERA_PIPELINE_PIPE0_CROP_VSTART,
         (uint32_t)CAMERA_PIPELINE_PIPE0_CROP_HSIZE,
         (uint32_t)CAMERA_PIPELINE_OUTPUT_HEIGHT);
#endif

  if (CameraPipeline_DCMIPP_Init() != HAL_OK)
  {
    pipeline_status.state = CAMERA_PIPELINE_STATE_ERROR;
    return HAL_ERROR;
  }
  CameraPipeline_PrintDcmippPipe1Regs("after_dcmipp_init");

  if (CameraPipeline_BringUpSensor(hi2c) != HAL_OK)
  {
    pipeline_status.state = CAMERA_PIPELINE_STATE_ERROR;
    return HAL_ERROR;
  }
  CameraPipeline_PrintImx219Regs("after_sensor_init");

#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
  appli_helpers.GetSensorInfo = GetSensorInfoHelper;
  appli_helpers.SetSensorGain = SetSensorGainHelper;
  appli_helpers.GetSensorGain = GetSensorGainHelper;
  appli_helpers.SetSensorExposure = SetSensorExposureHelper;
  appli_helpers.GetSensorExposure = GetSensorExposureHelper;
  appli_helpers.SetSensorTestPattern = SetSensorTestPatternHelper;

  isp_status = ISP_Init(&hcamera_isp, &hdcmipp, 0U, &appli_helpers, ISP_IQParamCacheInit[0]);
  if (isp_status != ISP_OK)
  {
    pipeline_status.last_error = 20U + (uint32_t)isp_status;
    printf("ISP_Init failed: %lu\r\n", (uint32_t)isp_status);
    pipeline_status.state = CAMERA_PIPELINE_STATE_ERROR;
    return HAL_ERROR;
  }
  CameraPipeline_PrintIspGeometry("after_isp_init");
  CameraPipeline_PrintDcmippPipe1Regs("after_isp_init");
#if (CAMERA_PIPELINE_FORCE_ISP_DECIMATION2 != 0U)
  isp_decimation_conf.HRatio = DCMIPP_HDEC_1_OUT_2;
  isp_decimation_conf.VRatio = DCMIPP_VDEC_1_OUT_2;
  if (HAL_DCMIPP_PIPE_SetISPDecimationConfig(&hdcmipp, DCMIPP_PIPE1, &isp_decimation_conf) != HAL_OK)
  {
    pipeline_status.last_error = 24U;
    printf("Forced PIPE1 ISP decimation config failed\r\n");
    pipeline_status.state = CAMERA_PIPELINE_STATE_ERROR;
    return HAL_ERROR;
  }
  if (HAL_DCMIPP_PIPE_EnableISPDecimation(&hdcmipp, DCMIPP_PIPE1) != HAL_OK)
  {
    pipeline_status.last_error = 25U;
    printf("Forced PIPE1 ISP decimation enable failed\r\n");
    pipeline_status.state = CAMERA_PIPELINE_STATE_ERROR;
    return HAL_ERROR;
  }
  printf("Forced PIPE1 ISP decimation: raw 1640x1232 -> 820x616 before RawBayer2RGB/downsize\r\n");
  CameraPipeline_PrintDcmippPipe1Regs("after_forced_isp_decim2");
#endif
#endif

  if (CameraPipeline_StartCaptureCycle() != HAL_OK)
  {
    pipeline_status.state = CAMERA_PIPELINE_STATE_ERROR;
    return HAL_ERROR;
  }

  printf("CSI IMX219 %s pipeline RUNNING\r\n", CAMERA_PIPELINE_OUTPUT_FORMAT_NAME);
  return HAL_OK;
}

void CameraPipeline_Task(void)
{
#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
  ISP_StatusTypeDef isp_status;
#endif
  uint32_t now = HAL_GetTick();
  uint32_t hw_frames = 0U;

  if (pipeline_status.state == CAMERA_PIPELINE_STATE_FROZEN)
  {
#if (CAMERA_PIPELINE_UART_DUMP_AFTER_FREEZE != 0U)
    if ((pipeline_uart_dumped < CAMERA_PIPELINE_UART_DUMP_REPEAT_COUNT) &&
        ((int32_t)(now - pipeline_uart_next_dump_tick) >= 0))
    {
      pipeline_uart_dumped++;
      printf("%s UART dump %lu/%lu\r\n",
             CAMERA_PIPELINE_OUTPUT_FORMAT_NAME,
             (uint32_t)pipeline_uart_dumped,
             (uint32_t)CAMERA_PIPELINE_UART_DUMP_REPEAT_COUNT);
      CameraPipeline_DumpFrameBufferOverUart();
      pipeline_uart_next_dump_tick = HAL_GetTick() + CAMERA_PIPELINE_UART_DUMP_REPEAT_DELAY_MS;
      if (pipeline_uart_dumped >= CAMERA_PIPELINE_UART_DUMP_REPEAT_COUNT)
      {
#if (CAMERA_PIPELINE_PERIODIC_CAPTURE_ENABLE != 0U)
        pipeline_next_capture_tick = HAL_GetTick() + CAMERA_PIPELINE_PERIODIC_CAPTURE_INTERVAL_MS;
        printf("%s next capture in %lu ms\r\n",
               CAMERA_PIPELINE_OUTPUT_FORMAT_NAME,
               (uint32_t)CAMERA_PIPELINE_PERIODIC_CAPTURE_INTERVAL_MS);
#endif
      }
    }
#endif
#if (CAMERA_PIPELINE_PERIODIC_CAPTURE_ENABLE != 0U)
    if ((pipeline_uart_dumped >= CAMERA_PIPELINE_UART_DUMP_REPEAT_COUNT) &&
        (pipeline_next_capture_tick != 0U) &&
        ((int32_t)(now - pipeline_next_capture_tick) >= 0))
    {
#if (CAMERA_PIPELINE_ISP_WARMUP_ENABLE != 0U)
      pipeline_probe_warmup_done = 0U;
#endif
      if (CameraPipeline_StartCaptureCycle() != HAL_OK)
      {
        pipeline_status.state = CAMERA_PIPELINE_STATE_ERROR;
        CameraPipeline_PrintErrorContext();
      }
    }
#endif
    return;
  }

  if (pipeline_status.state != CAMERA_PIPELINE_STATE_RUNNING)
  {
    return;
  }

#if (CAMERA_PIPELINE_ISP_WARMUP_ENABLE != 0U)
  if ((pipeline_probe_warmup_done == 0U) &&
      (pipeline_status.frame_count >= CAMERA_PIPELINE_ISP_WARMUP_FRAMES))
  {
    printf("ISP geometry warmup complete: frames=%lu sof=%lu eof=%lu, switching PIPE1 continuous -> snapshot\r\n",
           (uint32_t)pipeline_status.frame_count,
           (uint32_t)pipeline_sof_count,
           (uint32_t)pipeline_eof_count);
    CameraPipeline_PrintCsiRegs("before_warmup_stop");
    CameraPipeline_PrintDcmippPipe1Regs("before_warmup_stop");
#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
    CameraPipeline_PrintIspGeometry("before_warmup_stop");
#endif
    if (HAL_DCMIPP_CSI_PIPE_Stop(&hdcmipp,
                                 CAMERA_PIPELINE_CAPTURE_PIPE,
                                 DCMIPP_VIRTUAL_CHANNEL0) != HAL_OK)
    {
      pipeline_status.last_error = 61U;
      pipeline_status.state = CAMERA_PIPELINE_STATE_ERROR;
      printf("HAL_DCMIPP_CSI_PIPE_Stop failed after ISP warmup\r\n");
      CameraPipeline_PrintErrorContext();
      return;
    }

    pipeline_probe_warmup_done = 1U;
    if (CameraPipeline_StartCaptureCycle() != HAL_OK)
    {
      pipeline_status.state = CAMERA_PIPELINE_STATE_ERROR;
      CameraPipeline_PrintErrorContext();
    }
    return;
  }
#endif

  if ((CAMERA_PIPELINE_CAPTURE_PIPE == DCMIPP_PIPE0) &&
      (pipeline_p0_limit_count == 0U) &&
      (DCMIPP->P0DCCNTR >= CAMERA_PIPELINE_DCC_DONE_BYTES))
  {
    pipeline_p0_limit_count = 1U;
    pipeline_p0_limit_tick = now;
    pipeline_p0_limit_dcc = DCMIPP->P0DCCNTR;
    CLEAR_BIT(DCMIPP->P0FCTCR, DCMIPP_P0FCTCR_CPTREQ);
    CLEAR_BIT(DCMIPP->P0FSCR, DCMIPP_P0FSCR_PIPEN);
    printf("%s PIPE0 DCC fallback complete: dcc=%lu dcc_done_bytes=%lu dump_bytes=%lu\r\n",
           CAMERA_PIPELINE_OUTPUT_FORMAT_NAME,
           (uint32_t)pipeline_p0_limit_dcc,
           (uint32_t)CAMERA_PIPELINE_DCC_DONE_BYTES,
           (uint32_t)CAMERA_PIPELINE_FRAME_BYTES);
    CameraPipeline_PrintDcmippPipe0Regs("p0_dcc_fallback");
  }

  if ((pipeline_frozen == 0U) &&
      (pipeline_freeze_pending == 0U) &&
#if (CAMERA_PIPELINE_ISP_WARMUP_ENABLE != 0U)
      (pipeline_probe_warmup_done != 0U) &&
#endif
      ((pipeline_status.frame_count >= CAMERA_PIPELINE_FREEZE_AFTER_FRAMES) ||
       ((CAMERA_PIPELINE_CAPTURE_PIPE == DCMIPP_PIPE0) && (pipeline_p0_limit_count != 0U))))
  {
    pipeline_freeze_pending = 1U;
    pipeline_freeze_tick = now;
    printf("%s freeze pending: cb=%lu sof=%lu eof=%lu min_wait_ms=%lu timeout_ms=%lu\r\n",
           CAMERA_PIPELINE_OUTPUT_FORMAT_NAME,
           (uint32_t)pipeline_status.frame_count,
           (uint32_t)pipeline_sof_count,
           (uint32_t)pipeline_eof_count,
           (uint32_t)CAMERA_PIPELINE_FREEZE_MIN_WAIT_MS,
           (uint32_t)CAMERA_PIPELINE_FREEZE_TIMEOUT_MS);
  }

  if ((pipeline_frozen == 0U) &&
      (pipeline_freeze_pending != 0U))
  {
    uint32_t elapsed = now - pipeline_freeze_tick;
    uint32_t tail_ready = CameraPipeline_BufferTailReady();

    if (elapsed < CAMERA_PIPELINE_FREEZE_MIN_WAIT_MS)
    {
      return;
    }

    if (!((CAMERA_PIPELINE_CAPTURE_PIPE == DCMIPP_PIPE0) && (pipeline_p0_limit_count != 0U)) &&
        (tail_ready == 0U) &&
        (elapsed < CAMERA_PIPELINE_FREEZE_TIMEOUT_MS))
    {
      return;
    }

    pipeline_frozen = 1U;
    CameraPipeline_PrintCsiRegs("before_freeze_stop");
    CameraPipeline_PrintImx219Regs("before_freeze_stop");
    (void)IMX219_Stop(&hcamera_pipeline);
    pipeline_sensor_streaming = 0U;
    (void)HAL_DCMIPP_CSI_PIPE_Stop(&hdcmipp, CAMERA_PIPELINE_CAPTURE_PIPE, DCMIPP_VIRTUAL_CHANNEL0);
    pipeline_status.state = CAMERA_PIPELINE_STATE_FROZEN;
    printf("%s pipeline frozen for dump: cb=%lu sof=%lu eof=%lu elapsed=%lu tail_ready=%lu p0limit=%lu p0limit_tick=%lu p0limit_dcc=%lu p0dcc=%lu p0scszr=0x%08lX p0cscszr=0x%08lX p1sr=0x%08lX p1dscr=0x%08lX p1dsrtior=0x%08lX p1dsszr=0x%08lX"
#if (CAMERA_PIPELINE_CSI_LINE_BYTE_PROBE != 0U)
           " lb1=%lu lb124=%lu lb616=%lu lb1232=%lu"
#endif
           "\r\n",
           CAMERA_PIPELINE_OUTPUT_FORMAT_NAME,
           (uint32_t)pipeline_status.frame_count,
           (uint32_t)pipeline_sof_count,
           (uint32_t)pipeline_eof_count,
           (uint32_t)elapsed,
           (uint32_t)tail_ready,
           (uint32_t)pipeline_p0_limit_count,
           (uint32_t)pipeline_p0_limit_tick,
           (uint32_t)pipeline_p0_limit_dcc,
           (uint32_t)DCMIPP->P0DCCNTR,
           (uint32_t)DCMIPP->P0SCSZR,
           (uint32_t)DCMIPP->P0CSCSZR,
           (uint32_t)DCMIPP->P1SR,
           (uint32_t)DCMIPP->P1DSCR,
           (uint32_t)DCMIPP->P1DSRTIOR,
           (uint32_t)DCMIPP->P1DSSZR
#if (CAMERA_PIPELINE_CSI_LINE_BYTE_PROBE != 0U)
           ,
           (uint32_t)pipeline_csi_lb_count[0],
           (uint32_t)pipeline_csi_lb_count[1],
           (uint32_t)pipeline_csi_lb_count[2],
           (uint32_t)pipeline_csi_lb_count[3]
#endif
           );
    CameraPipeline_PrintDcmippPipe0Regs("frozen");
    CameraPipeline_PrintDcmippPipe1Regs("frozen");
#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
    CameraPipeline_PrintIspGeometry("frozen");
#endif
#if (CAMERA_PIPELINE_UART_DUMP_AFTER_FREEZE != 0U)
    pipeline_uart_next_dump_tick = HAL_GetTick() + CAMERA_PIPELINE_UART_DUMP_FIRST_DELAY_MS;
    printf("%s UART dump starts in %lu ms, repeats=%lu\r\n",
           CAMERA_PIPELINE_OUTPUT_FORMAT_NAME,
           (uint32_t)CAMERA_PIPELINE_UART_DUMP_FIRST_DELAY_MS,
           (uint32_t)CAMERA_PIPELINE_UART_DUMP_REPEAT_COUNT);
#endif
    return;
  }

#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
  isp_status = ISP_BackgroundProcess(&hcamera_isp);
  if (isp_status != ISP_OK)
  {
    pipeline_status.last_error = 100U + (uint32_t)isp_status;
    printf("ISP_BackgroundProcess failed: %lu\r\n", (uint32_t)isp_status);
  }
#endif

  if ((now - pipeline_last_log_tick) >= 1000U)
  {
    pipeline_last_log_tick = now;
    (void)HAL_DCMIPP_PIPE_ReadFrameCounter(&hdcmipp, CAMERA_PIPELINE_CAPTURE_PIPE, &hw_frames);
    printf("camera heartbeat fmt=%s cb=%lu hw=%lu sof=%lu eof=%lu gain=%ld exposure=%ld err=%lu state=%lu p0sr=0x%08lX p0fctcr=0x%08lX p0dcc=%lu p1sr=0x%08lX p1fctcr=0x%08lX csi_sr1=0x%08lX csi_err1=0x%08lX csi_err2=0x%08lX"
#if (CAMERA_PIPELINE_CSI_LINE_BYTE_PROBE != 0U)
           " lb1=%lu lb124=%lu lb616=%lu lb1232=%lu"
#endif
           "\r\n",
           CAMERA_PIPELINE_OUTPUT_FORMAT_NAME,
           pipeline_status.frame_count,
           hw_frames,
           pipeline_sof_count,
           pipeline_eof_count,
           (long)isp_gain,
           (long)isp_exposure,
           pipeline_status.last_error,
           (uint32_t)HAL_DCMIPP_GetState(&hdcmipp),
           (uint32_t)DCMIPP->P0SR,
           (uint32_t)DCMIPP->P0FCTCR,
           (uint32_t)DCMIPP->P0DCCNTR,
           (uint32_t)DCMIPP->P1SR,
           (uint32_t)DCMIPP->P1FCTCR,
           (uint32_t)CSI->SR1,
           (uint32_t)CSI->ERR1,
           (uint32_t)CSI->ERR2
#if (CAMERA_PIPELINE_CSI_LINE_BYTE_PROBE != 0U)
           ,
           (uint32_t)pipeline_csi_lb_count[0],
           (uint32_t)pipeline_csi_lb_count[1],
           (uint32_t)pipeline_csi_lb_count[2],
           (uint32_t)pipeline_csi_lb_count[3]
#endif
           );
    CameraPipeline_PrintCsiRegs("heartbeat");
  }
}

void CameraPipeline_GetStatus(CameraPipeline_Status_t *status)
{
  if (status != NULL)
  {
    *status = pipeline_status;
  }
}

uint8_t *CameraPipeline_GetFrameBuffer(void)
{
  return (uint8_t *)CAMERA_PIPELINE_BUFFER_ADDRESS;
}

uint32_t CameraPipeline_GetFrameBufferSize(void)
{
  return CAMERA_PIPELINE_FRAME_BYTES;
}

void CameraPipeline_PrintPortingNotes(void)
{
#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
  printf("CameraPipeline: ST ISP middleware active, IMX219 IQ is UNTUNED\r\n");
#else
  printf("CameraPipeline: DCMIPP direct path active, ST ISP middleware disabled\r\n");
#endif
}

void CameraPipeline_PrintErrorContext(void)
{
  printf("CameraPipeline error: state=%lu sensor=0x%04lX err=%lu frames=%lu p1sr=0x%08lX\r\n",
         (uint32_t)pipeline_status.state,
         (uint32_t)pipeline_status.sensor_id,
         (uint32_t)pipeline_status.last_error,
         (uint32_t)pipeline_status.frame_count,
         (uint32_t)DCMIPP->P1SR);
}

static HAL_StatusTypeDef CameraPipeline_StartCaptureCycle(void)
{
#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
  ISP_StatusTypeDef isp_status;
#endif
  uint32_t capture_mode = CAMERA_PIPELINE_CAPTURE_MODE;

#if (CAMERA_PIPELINE_ISP_WARMUP_ENABLE != 0U)
  capture_mode = (pipeline_probe_warmup_done == 0U) ? DCMIPP_MODE_CONTINUOUS : DCMIPP_MODE_SNAPSHOT;
#endif

  pipeline_capture_cycle++;
  pipeline_status.frame_count = 0U;
  pipeline_sof_count = 0U;
  pipeline_eof_count = 0U;
  pipeline_frozen = 0U;
  pipeline_freeze_pending = 0U;
  pipeline_freeze_tick = 0U;
  pipeline_uart_dumped = 0U;
  pipeline_uart_next_dump_tick = 0U;
  pipeline_next_capture_tick = 0U;
  pipeline_p0_limit_count = 0U;
  pipeline_p0_limit_tick = 0U;
  pipeline_p0_limit_dcc = 0U;

  memset((void *)CAMERA_PIPELINE_BUFFER_ADDRESS, 0xA5, CAMERA_PIPELINE_FRAME_BYTES);
  SCB_CleanDCache_by_Addr((void *)CAMERA_PIPELINE_BUFFER_ADDRESS,
                          (int32_t)CAMERA_PIPELINE_FRAME_BYTES);

  printf("%s capture cycle %lu request: mode=%s warmup_done=%lu sensor_streaming=%lu\r\n",
         CAMERA_PIPELINE_OUTPUT_FORMAT_NAME,
         (uint32_t)pipeline_capture_cycle,
         (capture_mode == DCMIPP_MODE_CONTINUOUS) ? "continuous" : "snapshot",
#if (CAMERA_PIPELINE_ISP_WARMUP_ENABLE != 0U)
         (uint32_t)pipeline_probe_warmup_done,
#else
         1UL,
#endif
         (uint32_t)pipeline_sensor_streaming);

  if (HAL_DCMIPP_CSI_PIPE_Start(&hdcmipp,
                                CAMERA_PIPELINE_CAPTURE_PIPE,
                                DCMIPP_VIRTUAL_CHANNEL0,
                                CAMERA_PIPELINE_BUFFER_ADDRESS,
                                capture_mode) != HAL_OK)
  {
    pipeline_status.last_error = 40U;
    printf("HAL_DCMIPP_CSI_PIPE_Start failed on cycle %lu\r\n",
           (uint32_t)pipeline_capture_cycle);
    return HAL_ERROR;
  }

#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
  if (pipeline_capture_cycle == 1U)
  {
    isp_status = ISP_Start(&hcamera_isp);
    if (isp_status != ISP_OK)
    {
      pipeline_status.last_error = 50U + (uint32_t)isp_status;
      (void)HAL_DCMIPP_CSI_PIPE_Stop(&hdcmipp,
                                     CAMERA_PIPELINE_CAPTURE_PIPE,
                                     DCMIPP_VIRTUAL_CHANNEL0);
      printf("ISP_Start failed on cycle %lu: %lu\r\n",
             (uint32_t)pipeline_capture_cycle,
             (uint32_t)isp_status);
      return HAL_ERROR;
    }
    CameraPipeline_PrintIspGeometry("after_isp_start");
    CameraPipeline_PrintDcmippPipe1Regs("after_isp_start");
#if ((CAMERA_PIPELINE_MANUAL_BRIGHTNESS_VERIFY != 0U) && \
     (CAMERA_PIPELINE_ENABLE_SENSOR_TEST_PATTERN == 0U))
    isp_gain = CAMERA_PIPELINE_MANUAL_GAIN_MDB;
    isp_exposure = CAMERA_PIPELINE_MANUAL_EXPOSURE_US;
    if (IMX219_SetExposureGain(&hcamera_pipeline,
                               CameraPipeline_ExposureUsToLines(isp_exposure),
                               CameraPipeline_GainMdBToAnalogReg(isp_gain),
                               CAMERA_PIPELINE_FIXED_DIGITAL_GAIN) != IMX219_OK)
    {
      pipeline_status.last_error = 56U;
      printf("IMX219 manual brightness set failed after ISP_Start\r\n");
      return HAL_ERROR;
    }
    printf("IMX219 manual brightness: exposure=%ldus lines=%lu line_time_ns=%lu gain=%ldmdB analog=0x%02lX\r\n",
           (long)isp_exposure,
           (uint32_t)CameraPipeline_ExposureUsToLines(isp_exposure),
           (uint32_t)CAMERA_PIPELINE_LINE_TIME_NS,
           (long)isp_gain,
           (uint32_t)CameraPipeline_GainMdBToAnalogReg(isp_gain));
#endif
  }
#endif

  if (pipeline_sensor_streaming == 0U)
  {
    if (IMX219_Start(&hcamera_pipeline) != IMX219_OK)
    {
      pipeline_status.last_error = 41U;
      (void)HAL_DCMIPP_CSI_PIPE_Stop(&hdcmipp,
                                     CAMERA_PIPELINE_CAPTURE_PIPE,
                                     DCMIPP_VIRTUAL_CHANNEL0);
      printf("IMX219_Start failed on cycle %lu\r\n",
             (uint32_t)pipeline_capture_cycle);
      return HAL_ERROR;
    }
    pipeline_sensor_streaming = 1U;
  }
  else
  {
    printf("IMX219 already streaming for cycle %lu\r\n",
           (uint32_t)pipeline_capture_cycle);
  }

  pipeline_status.state = CAMERA_PIPELINE_STATE_RUNNING;
  pipeline_last_log_tick = HAL_GetTick();
  printf("%s capture cycle %lu started\r\n",
         CAMERA_PIPELINE_OUTPUT_FORMAT_NAME,
         (uint32_t)pipeline_capture_cycle);
  return HAL_OK;
}

static HAL_StatusTypeDef CameraPipeline_DCMIPP_Init(void)
{
  DCMIPP_CSI_ConfTypeDef csi_conf = {0};
  DCMIPP_CSI_PIPE_ConfTypeDef csi_pipe_conf = {0};
  DCMIPP_PipeConfTypeDef pipe_conf = {0};
#if (CAMERA_PIPELINE_CSI_LINE_BYTE_PROBE != 0U)
  DCMIPP_CSI_LineByteCounterConfTypeDef line_byte_conf = {0};
#endif
#if (CAMERA_PIPELINE_PIPE0_DEBUG != 0U)
  DCMIPP_CropConfTypeDef crop_conf = {0};
#else
#if (CAMERA_PIPELINE_PIPE1_GRAY_CROP_ONLY_DEBUG != 0U)
  DCMIPP_CropConfTypeDef crop_conf = {0};
#endif
#if (CAMERA_PIPELINE_FORCE_PIPE1_DECIMATION2 != 0U)
  DCMIPP_DecimationConfTypeDef decimation_conf = {0};
#endif
#if ((CAMERA_PIPELINE_USE_ISP_RUNTIME == 0U) && (CAMERA_PIPELINE_RAW_GRAY_DEBUG == 0U))
  DCMIPP_RawBayer2RGBConfTypeDef raw_bayer_conf = {0};
#if (CAMERA_PIPELINE_PIPE1_ENABLE_WB_EXPOSURE != 0U)
  DCMIPP_ExposureConfTypeDef exposure_conf = {0};
#endif
#endif
#if ((CAMERA_PIPELINE_PIPE1_RAWBAYER_ONLY_DEBUG == 0U) && \
     (CAMERA_PIPELINE_PIPE1_GRAY_DECIM_ONLY_DEBUG == 0U) && \
     (CAMERA_PIPELINE_PIPE1_GRAY_CROP_ONLY_DEBUG == 0U))
  DCMIPP_DownsizeTypeDef downsize_conf = {0};
#endif
#endif

  hdcmipp.Instance = DCMIPP;
  if (HAL_DCMIPP_Init(&hdcmipp) != HAL_OK)
  {
    pipeline_status.last_error = 10U;
    return HAL_ERROR;
  }

  csi_conf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
  csi_conf.NumberOfLanes = DCMIPP_CSI_TWO_DATA_LANES;
#if (CAMERA_PIPELINE_USE_LOW_LINK != 0U)
  csi_conf.PHYBitrate = DCMIPP_CSI_PHY_BT_450;
#else
  csi_conf.PHYBitrate = DCMIPP_CSI_PHY_BT_1600;
#endif
  if (HAL_DCMIPP_CSI_SetConfig(&hdcmipp, &csi_conf) != HAL_OK)
  {
    pipeline_status.last_error = 11U;
    return HAL_ERROR;
  }

#if (CAMERA_PIPELINE_MASK_DPHY_IRQ != 0U)
  __HAL_DCMIPP_CSI_DPHY_DISABLE_IT(CSI, CAMERA_PIPELINE_CSI_DPHY_DATA_LANE_ERROR_IT);
#endif

  if (HAL_DCMIPP_CSI_SetVCConfig(&hdcmipp, DCMIPP_VIRTUAL_CHANNEL0, DCMIPP_CSI_DT_BPP10) != HAL_OK)
  {
    pipeline_status.last_error = 12U;
    return HAL_ERROR;
  }

#if (CAMERA_PIPELINE_CSI_LINE_BYTE_PROBE != 0U)
  line_byte_conf.VirtualChannel = DCMIPP_VIRTUAL_CHANNEL0;
  line_byte_conf.ByteCounter = CAMERA_PIPELINE_RAW10_LINE_BYTES;

  line_byte_conf.LineCounter = 1U;
  if (HAL_DCMIPP_CSI_SetLineByteCounterConfig(&hdcmipp, DCMIPP_CSI_COUNTER0, &line_byte_conf) != HAL_OK)
  {
    pipeline_status.last_error = 120U;
    return HAL_ERROR;
  }
  if (HAL_DCMIPP_CSI_EnableLineByteCounter(&hdcmipp, DCMIPP_CSI_COUNTER0) != HAL_OK)
  {
    pipeline_status.last_error = 121U;
    return HAL_ERROR;
  }

  line_byte_conf.LineCounter = 124U;
  if (HAL_DCMIPP_CSI_SetLineByteCounterConfig(&hdcmipp, DCMIPP_CSI_COUNTER1, &line_byte_conf) != HAL_OK)
  {
    pipeline_status.last_error = 122U;
    return HAL_ERROR;
  }
  if (HAL_DCMIPP_CSI_EnableLineByteCounter(&hdcmipp, DCMIPP_CSI_COUNTER1) != HAL_OK)
  {
    pipeline_status.last_error = 123U;
    return HAL_ERROR;
  }

  line_byte_conf.LineCounter = 616U;
  if (HAL_DCMIPP_CSI_SetLineByteCounterConfig(&hdcmipp, DCMIPP_CSI_COUNTER2, &line_byte_conf) != HAL_OK)
  {
    pipeline_status.last_error = 124U;
    return HAL_ERROR;
  }
  if (HAL_DCMIPP_CSI_EnableLineByteCounter(&hdcmipp, DCMIPP_CSI_COUNTER2) != HAL_OK)
  {
    pipeline_status.last_error = 125U;
    return HAL_ERROR;
  }

  line_byte_conf.LineCounter = CAMERA_PIPELINE_INPUT_HEIGHT;
  if (HAL_DCMIPP_CSI_SetLineByteCounterConfig(&hdcmipp, DCMIPP_CSI_COUNTER3, &line_byte_conf) != HAL_OK)
  {
    pipeline_status.last_error = 126U;
    return HAL_ERROR;
  }
  if (HAL_DCMIPP_CSI_EnableLineByteCounter(&hdcmipp, DCMIPP_CSI_COUNTER3) != HAL_OK)
  {
    pipeline_status.last_error = 127U;
    return HAL_ERROR;
  }
  printf("CSI line/byte probe armed: byte=%lu lines=1,124,616,%lu\r\n",
         (uint32_t)CAMERA_PIPELINE_RAW10_LINE_BYTES,
         (uint32_t)CAMERA_PIPELINE_INPUT_HEIGHT);
#endif

  csi_pipe_conf.DataTypeMode = DCMIPP_DTMODE_DTIDA;
  csi_pipe_conf.DataTypeIDA = DCMIPP_DT_RAW10;
  csi_pipe_conf.DataTypeIDB = DCMIPP_DT_RAW10;
  if (HAL_DCMIPP_CSI_PIPE_SetConfig(&hdcmipp, CAMERA_PIPELINE_CAPTURE_PIPE, &csi_pipe_conf) != HAL_OK)
  {
    pipeline_status.last_error = 13U;
    return HAL_ERROR;
  }
#if ((CAMERA_PIPELINE_PIPE1_FORCE_RAW10_FORMAT != 0U) && (CAMERA_PIPELINE_CAPTURE_PIPE == DCMIPP_PIPE1))
  if (HAL_DCMIPP_PIPE_CSI_ForceDataTypeFormat(&hdcmipp, DCMIPP_PIPE1, DCMIPP_DT_RAW10) != HAL_OK)
  {
    pipeline_status.last_error = 131U;
    return HAL_ERROR;
  }
  printf("DCMIPP PIPE1 force data type format: RAW10\r\n");
#endif

  pipe_conf.FrameRate = DCMIPP_FRAME_RATE_ALL;
#if (CAMERA_PIPELINE_PIPE0_DEBUG != 0U)
  pipe_conf.PixelPackerFormat = DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1;
#else
#if (CAMERA_PIPELINE_RAW_GRAY_DEBUG != 0U)
  pipe_conf.PixelPackerFormat = DCMIPP_PIXEL_PACKER_FORMAT_MONO_Y8_G8_1;
#else
  pipe_conf.PixelPackerFormat = DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1;
#endif
#endif
  pipe_conf.PixelPipePitch = CAMERA_PIPELINE_PITCH_BYTES;
  if (HAL_DCMIPP_PIPE_SetConfig(&hdcmipp, CAMERA_PIPELINE_CAPTURE_PIPE, &pipe_conf) != HAL_OK)
  {
    pipeline_status.last_error = 14U;
    return HAL_ERROR;
  }

  if (HAL_DCMIPP_PIPE_SetFrameCounterConfig(&hdcmipp, CAMERA_PIPELINE_CAPTURE_PIPE) != HAL_OK)
  {
    pipeline_status.last_error = 141U;
    return HAL_ERROR;
  }

  if (HAL_DCMIPP_PIPE_ResetFrameCounter(&hdcmipp, CAMERA_PIPELINE_CAPTURE_PIPE) != HAL_OK)
  {
    pipeline_status.last_error = 142U;
    return HAL_ERROR;
  }

#if (CAMERA_PIPELINE_PIPE0_DEBUG != 0U)
  WRITE_REG(DCMIPP->P0PPCR, 0U);
  if (HAL_DCMIPP_PIPE_SetBytesDecimationConfig(&hdcmipp,
                                               CAMERA_PIPELINE_CAPTURE_PIPE,
                                               DCMIPP_OEBS_ODD,
                                               DCMIPP_BSM_ALL) != HAL_OK)
  {
    pipeline_status.last_error = 153U;
    return HAL_ERROR;
  }
  if (HAL_DCMIPP_PIPE_SetLinesDecimationConfig(&hdcmipp,
                                               CAMERA_PIPELINE_CAPTURE_PIPE,
                                               DCMIPP_OELS_ODD,
                                               DCMIPP_LSM_ALL) != HAL_OK)
  {
    pipeline_status.last_error = 154U;
    return HAL_ERROR;
  }
  printf("PIPE0 %s byte/line select forced: ppcr=0x%08lX bytes=ALL lines=ALL\r\n",
         CAMERA_PIPELINE_OUTPUT_FORMAT_NAME,
         (uint32_t)DCMIPP->P0PPCR);
  crop_conf.HStart = CAMERA_PIPELINE_PIPE0_CROP_HSTART;
  crop_conf.VStart = CAMERA_PIPELINE_PIPE0_CROP_VSTART;
  crop_conf.HSize = CAMERA_PIPELINE_PIPE0_CROP_HSIZE;
  crop_conf.VSize = CAMERA_PIPELINE_OUTPUT_HEIGHT;
  crop_conf.PipeArea = DCMIPP_POSITIVE_AREA;
  if (HAL_DCMIPP_PIPE_SetCropConfig(&hdcmipp, CAMERA_PIPELINE_CAPTURE_PIPE, &crop_conf) != HAL_OK)
  {
    pipeline_status.last_error = 143U;
    return HAL_ERROR;
  }
  if (HAL_DCMIPP_PIPE_EnableCrop(&hdcmipp, CAMERA_PIPELINE_CAPTURE_PIPE) != HAL_OK)
  {
    pipeline_status.last_error = 144U;
    return HAL_ERROR;
  }
  if (HAL_DCMIPP_PIPE_CSI_DisableHeader(&hdcmipp, CAMERA_PIPELINE_CAPTURE_PIPE) != HAL_OK)
  {
    pipeline_status.last_error = 145U;
    return HAL_ERROR;
  }
  CameraPipeline_PrintImx219Regs("after_sensor_start");
  CameraPipeline_PrintCsiRegs("after_sensor_start");
  if (HAL_DCMIPP_PIPE_EnableLimitEvent(&hdcmipp,
                                       CAMERA_PIPELINE_CAPTURE_PIPE,
                                       CAMERA_PIPELINE_DCC_DONE_WORDS) != HAL_OK)
  {
    pipeline_status.last_error = 148U;
    return HAL_ERROR;
  }
  CameraPipeline_PrintDcmippPipe0Regs("after_dcmipp_init");
#else
  (void)HAL_DCMIPP_PIPE_DisableDecimation(&hdcmipp, DCMIPP_PIPE1);
#if (CAMERA_PIPELINE_FORCE_PIPE1_DECIMATION2 != 0U)
  decimation_conf.HRatio = DCMIPP_HDEC_1_OUT_2;
  decimation_conf.VRatio = DCMIPP_VDEC_1_OUT_2;
  if (HAL_DCMIPP_PIPE_SetDecimationConfig(&hdcmipp, DCMIPP_PIPE1, &decimation_conf) != HAL_OK)
  {
    pipeline_status.last_error = 149U;
    return HAL_ERROR;
  }
  if (HAL_DCMIPP_PIPE_EnableDecimation(&hdcmipp, DCMIPP_PIPE1) != HAL_OK)
  {
    pipeline_status.last_error = 150U;
    return HAL_ERROR;
  }
  printf("DCMIPP PIPE1 normal decimation forced: raw 1640x1232 -> 820x616 before downsize\r\n");
#endif

#if (CAMERA_PIPELINE_USE_ISP_RUNTIME == 0U)
  (void)HAL_DCMIPP_PIPE_DisableISPDecimation(&hdcmipp, DCMIPP_PIPE1);
#if (CAMERA_PIPELINE_RAW_GRAY_DEBUG != 0U)
  printf("DCMIPP PIPE1 raw gray debug: RawBayer2RGB disabled, RAW10->GRAY8 pixel packer path\r\n");
#else
  raw_bayer_conf.RawBayerType = CAMERA_PIPELINE_PIPE1_BAYER;
  raw_bayer_conf.PeakStrength = CAMERA_PIPELINE_PIPE1_BAYER_STRENGTH;
  raw_bayer_conf.EdgeStrength = CAMERA_PIPELINE_PIPE1_BAYER_STRENGTH;
  raw_bayer_conf.VLineStrength = CAMERA_PIPELINE_PIPE1_BAYER_STRENGTH;
  raw_bayer_conf.HLineStrength = CAMERA_PIPELINE_PIPE1_BAYER_STRENGTH;
#if (CAMERA_PIPELINE_PIPE1_ENABLE_WB_EXPOSURE != 0U)
  CameraPipeline_ToExposureShiftMultiplier(CAMERA_PIPELINE_PIPE1_WB_R_GAIN,
                                           &exposure_conf.ShiftRed,
                                           &exposure_conf.MultiplierRed);
  CameraPipeline_ToExposureShiftMultiplier(CAMERA_PIPELINE_PIPE1_WB_G_GAIN,
                                           &exposure_conf.ShiftGreen,
                                           &exposure_conf.MultiplierGreen);
  CameraPipeline_ToExposureShiftMultiplier(CAMERA_PIPELINE_PIPE1_WB_B_GAIN,
                                           &exposure_conf.ShiftBlue,
                                           &exposure_conf.MultiplierBlue);
  if (HAL_DCMIPP_PIPE_SetISPExposureConfig(&hdcmipp, DCMIPP_PIPE1, &exposure_conf) != HAL_OK)
  {
    pipeline_status.last_error = 155U;
    return HAL_ERROR;
  }
  if (HAL_DCMIPP_PIPE_EnableISPExposure(&hdcmipp, DCMIPP_PIPE1) != HAL_OK)
  {
    pipeline_status.last_error = 156U;
    return HAL_ERROR;
  }
  printf("DCMIPP PIPE1 WB exposure: gain_r/g/b=%lu/%lu/%lu shift_r/g/b=%lu/%lu/%lu mult_r/g/b=%lu/%lu/%lu\r\n",
         (uint32_t)CAMERA_PIPELINE_PIPE1_WB_R_GAIN,
         (uint32_t)CAMERA_PIPELINE_PIPE1_WB_G_GAIN,
         (uint32_t)CAMERA_PIPELINE_PIPE1_WB_B_GAIN,
         (uint32_t)exposure_conf.ShiftRed,
         (uint32_t)exposure_conf.ShiftGreen,
         (uint32_t)exposure_conf.ShiftBlue,
         (uint32_t)exposure_conf.MultiplierRed,
         (uint32_t)exposure_conf.MultiplierGreen,
         (uint32_t)exposure_conf.MultiplierBlue);
#endif
  if (HAL_DCMIPP_PIPE_SetISPRawBayer2RGBConfig(&hdcmipp, DCMIPP_PIPE1, &raw_bayer_conf) != HAL_OK)
  {
    pipeline_status.last_error = 146U;
    return HAL_ERROR;
  }

  if (HAL_DCMIPP_PIPE_EnableISPRawBayer2RGB(&hdcmipp, DCMIPP_PIPE1) != HAL_OK)
  {
    pipeline_status.last_error = 147U;
    return HAL_ERROR;
  }
#endif
#else
  printf("DCMIPP PIPE1 ISP middleware owns decimation/rawbayer/stat config\r\n");
#endif

#if (CAMERA_PIPELINE_PIPE1_GRAY_CROP_ONLY_DEBUG != 0U)
  crop_conf.HStart = 0U;
  crop_conf.VStart = 0U;
  crop_conf.HSize = CAMERA_PIPELINE_OUTPUT_WIDTH;
  crop_conf.VSize = CAMERA_PIPELINE_OUTPUT_HEIGHT;
  crop_conf.PipeArea = DCMIPP_POSITIVE_AREA;
  if (HAL_DCMIPP_PIPE_SetCropConfig(&hdcmipp, DCMIPP_PIPE1, &crop_conf) != HAL_OK)
  {
    pipeline_status.last_error = 151U;
    return HAL_ERROR;
  }
  if (HAL_DCMIPP_PIPE_EnableCrop(&hdcmipp, DCMIPP_PIPE1) != HAL_OK)
  {
    pipeline_status.last_error = 152U;
    return HAL_ERROR;
  }
  printf("DCMIPP PIPE1 gray crop only: crop=%lux%lu downsize=off decimation=off pitch=%lu frame_bytes=%lu\r\n",
         (uint32_t)CAMERA_PIPELINE_OUTPUT_WIDTH,
         (uint32_t)CAMERA_PIPELINE_OUTPUT_HEIGHT,
         (uint32_t)CAMERA_PIPELINE_PITCH_BYTES,
         (uint32_t)CAMERA_PIPELINE_FRAME_BYTES);
#endif

#if (CAMERA_PIPELINE_PIPE1_RAWBAYER_ONLY_DEBUG != 0U)
  printf("DCMIPP PIPE1 rawbayer2rgb only: expected=%lux%lu pitch=%lu frame_bytes=%lu\r\n",
         (uint32_t)CAMERA_PIPELINE_OUTPUT_WIDTH,
         (uint32_t)CAMERA_PIPELINE_OUTPUT_HEIGHT,
         (uint32_t)CAMERA_PIPELINE_PITCH_BYTES,
         (uint32_t)CAMERA_PIPELINE_FRAME_BYTES);
#elif (CAMERA_PIPELINE_PIPE1_GRAY_DECIM_ONLY_DEBUG != 0U)
  printf("DCMIPP PIPE1 gray decimation only: expected=%lux%lu pitch=%lu frame_bytes=%lu downsize=off\r\n",
         (uint32_t)CAMERA_PIPELINE_OUTPUT_WIDTH,
         (uint32_t)CAMERA_PIPELINE_OUTPUT_HEIGHT,
         (uint32_t)CAMERA_PIPELINE_PITCH_BYTES,
         (uint32_t)CAMERA_PIPELINE_FRAME_BYTES);
#elif (CAMERA_PIPELINE_PIPE1_GRAY_CROP_ONLY_DEBUG != 0U)
  printf("DCMIPP PIPE1 gray crop only output: expected=%lux%lu\r\n",
         (uint32_t)CAMERA_PIPELINE_OUTPUT_WIDTH,
         (uint32_t)CAMERA_PIPELINE_OUTPUT_HEIGHT);
#else
  downsize_conf.HSize = CAMERA_PIPELINE_OUTPUT_WIDTH;
  downsize_conf.VSize = CAMERA_PIPELINE_OUTPUT_HEIGHT;
  downsize_conf.HRatio = DCMIPP_DOWNSIZE_RATIO(CAMERA_PIPELINE_DOWNSIZE_SOURCE_WIDTH, CAMERA_PIPELINE_OUTPUT_WIDTH);
  downsize_conf.VRatio = DCMIPP_DOWNSIZE_RATIO(CAMERA_PIPELINE_DOWNSIZE_SOURCE_HEIGHT, CAMERA_PIPELINE_OUTPUT_HEIGHT);
  downsize_conf.HDivFactor = DCMIPP_DOWNSIZE_DIV_FACTOR(CAMERA_PIPELINE_DOWNSIZE_SOURCE_WIDTH, CAMERA_PIPELINE_OUTPUT_WIDTH);
  downsize_conf.VDivFactor = DCMIPP_DOWNSIZE_DIV_FACTOR(CAMERA_PIPELINE_DOWNSIZE_SOURCE_HEIGHT, CAMERA_PIPELINE_OUTPUT_HEIGHT);
  printf("DCMIPP PIPE1 downsize: src=%lux%lu dst=%lux%lu hratio=%lu vratio=%lu hdiv=%lu vdiv=%lu\r\n",
         (uint32_t)CAMERA_PIPELINE_DOWNSIZE_SOURCE_WIDTH,
         (uint32_t)CAMERA_PIPELINE_DOWNSIZE_SOURCE_HEIGHT,
         (uint32_t)downsize_conf.HSize,
         (uint32_t)downsize_conf.VSize,
         (uint32_t)downsize_conf.HRatio,
         (uint32_t)downsize_conf.VRatio,
         (uint32_t)downsize_conf.HDivFactor,
         (uint32_t)downsize_conf.VDivFactor);
  if (HAL_DCMIPP_PIPE_SetDownsizeConfig(&hdcmipp, DCMIPP_PIPE1, &downsize_conf) != HAL_OK)
  {
    pipeline_status.last_error = 15U;
    return HAL_ERROR;
  }

  if (HAL_DCMIPP_PIPE_EnableDownsize(&hdcmipp, DCMIPP_PIPE1) != HAL_OK)
  {
    pipeline_status.last_error = 16U;
    return HAL_ERROR;
  }
#endif
#endif

  return HAL_OK;
}

static HAL_StatusTypeDef CameraPipeline_BringUpSensor(I2C_HandleTypeDef *hi2c)
{
  uint32_t id = 0U;

  if (hi2c == NULL)
  {
    pipeline_status.last_error = 1U;
    return HAL_ERROR;
  }

  printf("IMX219 ISP: enable module power\r\n");
  HAL_GPIO_WritePin(EN_MODULE_GPIO_Port, EN_MODULE_Pin, GPIO_PIN_SET);
  HAL_Delay(200U);

  if (IMX219_RegisterBusIO(&hcamera_pipeline, hi2c, IMX219_I2C_ADDR_7BIT) != IMX219_OK)
  {
    pipeline_status.last_error = 2U;
    return HAL_ERROR;
  }

  if (IMX219_ReadID(&hcamera_pipeline, &id) != IMX219_OK)
  {
    pipeline_status.last_error = 3U;
    printf("IMX219 ISP: read ID failed\r\n");
    return HAL_ERROR;
  }

  pipeline_status.sensor_id = id;
  if (id != IMX219_CHIP_ID)
  {
    pipeline_status.last_error = 4U;
    printf("IMX219 ISP: unexpected ID 0x%04lX\r\n", (uint32_t)id);
    return HAL_ERROR;
  }

  if (IMX219_EnterLp11(&hcamera_pipeline) != IMX219_OK)
  {
    pipeline_status.last_error = 5U;
    return HAL_ERROR;
  }

  if (IMX219_Init(&hcamera_pipeline,
                  CAMERA_PIPELINE_SENSOR_RESOLUTION,
                  CAMERA_PIPELINE_PIXEL_FORMAT) != IMX219_OK)
  {
    pipeline_status.last_error = 6U;
    return HAL_ERROR;
  }

  if (IMX219_SetCsiLaneMode(&hcamera_pipeline, CAMERA_PIPELINE_LANE_COUNT) != IMX219_OK)
  {
    pipeline_status.last_error = 7U;
    return HAL_ERROR;
  }

#if (CAMERA_PIPELINE_USE_LOW_LINK != 0U)
  if (IMX219_SetDebugOpPllMultiplier(&hcamera_pipeline, CAMERA_PIPELINE_LOW_LINK_OP_PLL_MULT) != IMX219_OK)
  {
    pipeline_status.last_error = 71U;
    return HAL_ERROR;
  }
  printf("IMX219 ISP low link ON: op_pll_mult=0x%04lX\r\n", (uint32_t)CAMERA_PIPELINE_LOW_LINK_OP_PLL_MULT);
#else
  printf("IMX219 ISP low link OFF: using sensor default op_pll_mult\r\n");
#endif

  if (IMX219_SetFrameTiming(&hcamera_pipeline,
                            CAMERA_PIPELINE_LINE_LENGTH,
                            CAMERA_PIPELINE_FRAME_LENGTH) != IMX219_OK)
  {
    pipeline_status.last_error = 8U;
    return HAL_ERROR;
  }

  if (IMX219_SetExposureGain(&hcamera_pipeline,
                             CAMERA_PIPELINE_FIXED_EXPOSURE_LINES,
                             CAMERA_PIPELINE_FIXED_ANALOG_GAIN,
                             CAMERA_PIPELINE_FIXED_DIGITAL_GAIN) != IMX219_OK)
  {
    pipeline_status.last_error = 9U;
    return HAL_ERROR;
  }

#if (CAMERA_PIPELINE_ENABLE_SENSOR_TEST_PATTERN != 0U)
  if (IMX219_SetTestPattern(&hcamera_pipeline, CAMERA_PIPELINE_SENSOR_TEST_PATTERN_MODE) != IMX219_OK)
  {
    pipeline_status.last_error = 91U;
    return HAL_ERROR;
  }
  printf("IMX219 ISP test pattern ON: mode=%lu\r\n", (uint32_t)CAMERA_PIPELINE_SENSOR_TEST_PATTERN_MODE);
  {
    uint16_t tp_mode = 0U;
    uint16_t tp_width = 0U;
    uint16_t tp_height = 0U;

    if ((IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_TEST_PATTERN, &tp_mode) == IMX219_OK) &&
        (IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_TP_WINDOW_WIDTH, &tp_width) == IMX219_OK) &&
        (IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_TP_WINDOW_HEIGHT, &tp_height) == IMX219_OK))
    {
      printf("IMX219 ISP test pattern regs: mode=0x%04lX window=%lux%lu\r\n",
             (uint32_t)tp_mode,
             (uint32_t)tp_width,
             (uint32_t)tp_height);
    }
    else
    {
      printf("IMX219 ISP test pattern regs readback failed\r\n");
    }
  }
#else
  if (IMX219_SetTestPattern(&hcamera_pipeline, 0U) != IMX219_OK)
  {
    pipeline_status.last_error = 92U;
    return HAL_ERROR;
  }
  printf("IMX219 ISP test pattern OFF\r\n");
#endif

  printf("IMX219 ISP sensor ready: id=0x%04lX %lux%lu RAW10\r\n",
         (uint32_t)id,
         (uint32_t)CAMERA_PIPELINE_INPUT_WIDTH,
         (uint32_t)CAMERA_PIPELINE_INPUT_HEIGHT);
  return HAL_OK;
}

#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
static ISP_StatusTypeDef GetSensorInfoHelper(uint32_t Instance, ISP_SensorInfoTypeDef *SensorInfo)
{
  (void)Instance;

  if (SensorInfo == NULL)
  {
    return ISP_ERR_EINVAL;
  }

  memset(SensorInfo, 0, sizeof(*SensorInfo));
  (void)snprintf(SensorInfo->name, sizeof(SensorInfo->name), "IMX219");
  SensorInfo->bayer_pattern = (CAMERA_PIPELINE_RAW_GRAY_DEBUG != 0U) ? ISP_DEMOS_TYPE_MONO : CAMERA_PIPELINE_SENSOR_BAYER_PATTERN;
  SensorInfo->color_depth = 10U;
  SensorInfo->width = CAMERA_PIPELINE_INPUT_WIDTH;
  SensorInfo->height = CAMERA_PIPELINE_INPUT_HEIGHT;
  SensorInfo->gain_min = 0U;
  SensorInfo->gain_max = 24000U;
  SensorInfo->exposure_min = 100U;
  SensorInfo->exposure_max = 30000U;
  printf("ISP GetSensorInfo: instance=%lu name=%s bayer=%lu depth=%lu size=%lux%lu gain=[%lu,%lu] exposure=[%lu,%lu]\r\n",
         (uint32_t)Instance,
         SensorInfo->name,
         (uint32_t)SensorInfo->bayer_pattern,
         (uint32_t)SensorInfo->color_depth,
         (uint32_t)SensorInfo->width,
         (uint32_t)SensorInfo->height,
         (uint32_t)SensorInfo->gain_min,
         (uint32_t)SensorInfo->gain_max,
         (uint32_t)SensorInfo->exposure_min,
         (uint32_t)SensorInfo->exposure_max);
  return ISP_OK;
}

static ISP_StatusTypeDef SetSensorGainHelper(uint32_t Instance, int32_t Gain)
{
  (void)Instance;

#if ((CAMERA_PIPELINE_MANUAL_BRIGHTNESS_VERIFY != 0U) && \
     (CAMERA_PIPELINE_ENABLE_SENSOR_TEST_PATTERN == 0U))
  (void)Gain;
  isp_gain = CAMERA_PIPELINE_MANUAL_GAIN_MDB;
  if (IMX219_SetExposureGain(&hcamera_pipeline,
                             CameraPipeline_ExposureUsToLines(isp_exposure),
                             CameraPipeline_GainMdBToAnalogReg(isp_gain),
                             CAMERA_PIPELINE_FIXED_DIGITAL_GAIN) != IMX219_OK)
  {
    return ISP_ERR_SENSORGAIN;
  }
  return ISP_OK;
#else
  isp_gain = Gain;
  if (IMX219_SetExposureGain(&hcamera_pipeline,
                             CameraPipeline_ExposureUsToLines(isp_exposure),
                             CameraPipeline_GainMdBToAnalogReg(Gain),
                             CAMERA_PIPELINE_FIXED_DIGITAL_GAIN) != IMX219_OK)
  {
    return ISP_ERR_SENSORGAIN;
  }

  return ISP_OK;
#endif
}

static ISP_StatusTypeDef GetSensorGainHelper(uint32_t Instance, int32_t *Gain)
{
  (void)Instance;
  if (Gain == NULL)
  {
    return ISP_ERR_EINVAL;
  }

  *Gain = isp_gain;
  return ISP_OK;
}

static ISP_StatusTypeDef SetSensorExposureHelper(uint32_t Instance, int32_t Exposure)
{
  (void)Instance;

#if ((CAMERA_PIPELINE_MANUAL_BRIGHTNESS_VERIFY != 0U) && \
     (CAMERA_PIPELINE_ENABLE_SENSOR_TEST_PATTERN == 0U))
  (void)Exposure;
  isp_exposure = CAMERA_PIPELINE_MANUAL_EXPOSURE_US;
  if (IMX219_SetExposureGain(&hcamera_pipeline,
                             CameraPipeline_ExposureUsToLines(isp_exposure),
                             CameraPipeline_GainMdBToAnalogReg(isp_gain),
                             CAMERA_PIPELINE_FIXED_DIGITAL_GAIN) != IMX219_OK)
  {
    return ISP_ERR_SENSOREXPOSURE;
  }
  return ISP_OK;
#else
  isp_exposure = Exposure;
  if (IMX219_SetExposureGain(&hcamera_pipeline,
                             CameraPipeline_ExposureUsToLines(Exposure),
                             CameraPipeline_GainMdBToAnalogReg(isp_gain),
                             CAMERA_PIPELINE_FIXED_DIGITAL_GAIN) != IMX219_OK)
  {
    return ISP_ERR_SENSOREXPOSURE;
  }

  return ISP_OK;
#endif
}

static ISP_StatusTypeDef GetSensorExposureHelper(uint32_t Instance, int32_t *Exposure)
{
  (void)Instance;
  if (Exposure == NULL)
  {
    return ISP_ERR_EINVAL;
  }

  *Exposure = isp_exposure;
  return ISP_OK;
}

static ISP_StatusTypeDef SetSensorTestPatternHelper(uint32_t Instance, int32_t Mode)
{
  (void)Instance;
  return (IMX219_SetTestPattern(&hcamera_pipeline, (uint8_t)Mode) == IMX219_OK) ? ISP_OK : ISP_ERR_SENSORTESTPATTERN;
}

static uint8_t CameraPipeline_GainMdBToAnalogReg(int32_t GainMdB)
{
  if (GainMdB <= 0)
  {
    return 0U;
  }
  if (GainMdB < 6000)
  {
    return 64U;
  }
  if (GainMdB < 12000)
  {
    return 128U;
  }
  if (GainMdB < 18000)
  {
    return 192U;
  }
  return 224U;
}

static uint16_t CameraPipeline_ExposureUsToLines(int32_t ExposureUs)
{
  uint32_t lines;

  if (ExposureUs <= 0)
  {
    return CAMERA_PIPELINE_FIXED_EXPOSURE_LINES;
  }

  lines = ((uint32_t)ExposureUs * 1000U) / CAMERA_PIPELINE_LINE_TIME_NS;
  if (lines < 1U)
  {
    lines = 1U;
  }
  if (lines > (CAMERA_PIPELINE_FRAME_LENGTH - 4U))
  {
    lines = CAMERA_PIPELINE_FRAME_LENGTH - 4U;
  }

  return (uint16_t)lines;
}

static void CameraPipeline_PrintIspGeometry(const char *tag)
{
  ISP_DecimationTypeDef decimation = {0};
  ISP_StatAreaTypeDef stat_area = {0};
  ISP_StatusTypeDef dec_status;
  ISP_StatusTypeDef stat_status;

  dec_status = ISP_GetDecimationFactor(&hcamera_isp, &decimation);
  stat_status = ISP_GetStatArea(&hcamera_isp, &stat_area);
  printf("ISP geometry %s: sensor=%s bayer=%lu depth=%lu size=%lux%lu gain=[%lu,%lu] exposure=[%lu,%lu] dec_status=%lu dec_factor=%lu stat_status=%lu stat=(%lu,%lu %lux%lu)\r\n",
         tag,
         hcamera_isp.sensorInfo.name,
         (uint32_t)hcamera_isp.sensorInfo.bayer_pattern,
         (uint32_t)hcamera_isp.sensorInfo.color_depth,
         (uint32_t)hcamera_isp.sensorInfo.width,
         (uint32_t)hcamera_isp.sensorInfo.height,
         (uint32_t)hcamera_isp.sensorInfo.gain_min,
         (uint32_t)hcamera_isp.sensorInfo.gain_max,
         (uint32_t)hcamera_isp.sensorInfo.exposure_min,
         (uint32_t)hcamera_isp.sensorInfo.exposure_max,
         (uint32_t)dec_status,
         (uint32_t)decimation.factor,
         (uint32_t)stat_status,
         (uint32_t)stat_area.X0,
         (uint32_t)stat_area.Y0,
         (uint32_t)stat_area.XSize,
         (uint32_t)stat_area.YSize);
}
#endif

static void CameraPipeline_PrintDcmippPipe0Regs(const char *tag)
{
  printf("DCMIPP P0 regs %s: fscr=0x%08lX fctcr=0x%08lX scstr=0x%08lX scszr=0x%08lX cfscr=0x%08lX cfctcr=0x%08lX cscstr=0x%08lX cscszr=0x%08lX ppcr=0x%08lX ppm0ar1=0x%08lX ppm0ar2=0x%08lX stm0ar=0x%08lX dclmtr=0x%08lX dccntr=%lu sr=0x%08lX cmsr1=0x%08lX cmsr2=0x%08lX\r\n",
         tag,
         (uint32_t)DCMIPP->P0FSCR,
         (uint32_t)DCMIPP->P0FCTCR,
         (uint32_t)DCMIPP->P0SCSTR,
         (uint32_t)DCMIPP->P0SCSZR,
         (uint32_t)DCMIPP->P0CFSCR,
         (uint32_t)DCMIPP->P0CFCTCR,
         (uint32_t)DCMIPP->P0CSCSTR,
         (uint32_t)DCMIPP->P0CSCSZR,
         (uint32_t)DCMIPP->P0PPCR,
         (uint32_t)DCMIPP->P0PPM0AR1,
         (uint32_t)DCMIPP->P0PPM0AR2,
         (uint32_t)DCMIPP->P0STM0AR,
         (uint32_t)DCMIPP->P0DCLMTR,
         (uint32_t)DCMIPP->P0DCCNTR,
         (uint32_t)DCMIPP->P0SR,
         (uint32_t)DCMIPP->CMSR1,
         (uint32_t)DCMIPP->CMSR2);
}

static void CameraPipeline_PrintDcmippPipe1Regs(const char *tag)
{
  printf("DCMIPP P1 regs %s: fscr=0x%08lX srcr=0x%08lX decr=0x%08lX ststr=0x%08lX stszr=0x%08lX dmcr=0x%08lX fctcr=0x%08lX crstr=0x%08lX crszr=0x%08lX dccr=0x%08lX dscr=0x%08lX dsrtior=0x%08lX dsszr=0x%08lX ppcr=0x%08lX ppm0ar1=0x%08lX ppm0ar2=0x%08lX ppm0pr=0x%08lX stm0ar=0x%08lX sr=0x%08lX\r\n",
         tag,
         (uint32_t)DCMIPP->P1FSCR,
         (uint32_t)DCMIPP->P1SRCR,
         (uint32_t)DCMIPP->P1DECR,
         (uint32_t)DCMIPP->P1STSTR,
         (uint32_t)DCMIPP->P1STSZR,
         (uint32_t)DCMIPP->P1DMCR,
         (uint32_t)DCMIPP->P1FCTCR,
         (uint32_t)DCMIPP->P1CRSTR,
         (uint32_t)DCMIPP->P1CRSZR,
         (uint32_t)DCMIPP->P1DCCR,
         (uint32_t)DCMIPP->P1DSCR,
         (uint32_t)DCMIPP->P1DSRTIOR,
         (uint32_t)DCMIPP->P1DSSZR,
         (uint32_t)DCMIPP->P1PPCR,
         (uint32_t)DCMIPP->P1PPM0AR1,
         (uint32_t)DCMIPP->P1PPM0AR2,
         (uint32_t)DCMIPP->P1PPM0PR,
         (uint32_t)DCMIPP->P1STM0AR,
         (uint32_t)DCMIPP->P1SR);
  printf("DCMIPP P1 current %s: cfscr=0x%08lX cststr=0x%08lX cstszr=0x%08lX cdmcr=0x%08lX cfctcr=0x%08lX ccrstr=0x%08lX ccrszr=0x%08lX cdccr=0x%08lX cdscr=0x%08lX cdsrtior=0x%08lX cdsszr=0x%08lX cppcr=0x%08lX cppm0ar1=0x%08lX cppm0ar2=0x%08lX cppm0pr=0x%08lX\r\n",
         tag,
         (uint32_t)DCMIPP->P1CFSCR,
         (uint32_t)DCMIPP->P1CSTSTR,
         (uint32_t)DCMIPP->P1CSTSZR,
         (uint32_t)DCMIPP->P1DMCR,
         (uint32_t)DCMIPP->P1CFCTCR,
         (uint32_t)DCMIPP->P1CCRSTR,
         (uint32_t)DCMIPP->P1CCRSZR,
         (uint32_t)DCMIPP->P1CDCCR,
         (uint32_t)DCMIPP->P1CDSCR,
         (uint32_t)DCMIPP->P1CDSRTIOR,
         (uint32_t)DCMIPP->P1CDSSZR,
         (uint32_t)DCMIPP->P1CPPCR,
         (uint32_t)DCMIPP->P1CPPM0AR1,
         (uint32_t)DCMIPP->P1CPPM0AR2,
         (uint32_t)DCMIPP->P1CPPM0PR);
}

static void CameraPipeline_PrintCsiRegs(const char *tag)
{
  printf("CSI regs %s: cr=0x%08lX pcr=0x%08lX vc0cfgr1=0x%08lX vc0cfgr2=0x%08lX vc0cfgr3=0x%08lX vc0cfgr4=0x%08lX lb0=0x%08lX lb1=0x%08lX lb2=0x%08lX lb3=0x%08lX lmcfgr=0x%08lX sr0=0x%08lX sr1=0x%08lX spdfr=0x%08lX err1=0x%08lX err2=0x%08lX prcr=0x%08lX pmcr=0x%08lX pfcr=0x%08lX ptsr=0x%08lX cmsr1=0x%08lX cmsr2=0x%08lX\r\n",
         tag,
         (uint32_t)CSI->CR,
         (uint32_t)CSI->PCR,
         (uint32_t)CSI->VC0CFGR1,
         (uint32_t)CSI->VC0CFGR2,
         (uint32_t)CSI->VC0CFGR3,
         (uint32_t)CSI->VC0CFGR4,
         (uint32_t)CSI->LB0CFGR,
         (uint32_t)CSI->LB1CFGR,
         (uint32_t)CSI->LB2CFGR,
         (uint32_t)CSI->LB3CFGR,
         (uint32_t)CSI->LMCFGR,
         (uint32_t)CSI->SR0,
         (uint32_t)CSI->SR1,
         (uint32_t)CSI->SPDFR,
         (uint32_t)CSI->ERR1,
         (uint32_t)CSI->ERR2,
         (uint32_t)CSI->PRCR,
         (uint32_t)CSI->PMCR,
         (uint32_t)CSI->PFCR,
         (uint32_t)CSI->PTSR,
         (uint32_t)DCMIPP->CMSR1,
         (uint32_t)DCMIPP->CMSR2);
}

static void CameraPipeline_PrintImx219Regs(const char *tag)
{
  uint16_t x_start = 0U;
  uint16_t x_end = 0U;
  uint16_t y_start = 0U;
  uint16_t y_end = 0U;
  uint16_t x_out = 0U;
  uint16_t y_out = 0U;
  uint16_t line_length = 0U;
  uint16_t frame_length = 0U;
  uint16_t op_pll_mpy = 0U;
  uint16_t tp_mode = 0U;
  uint16_t tp_width = 0U;
  uint16_t tp_height = 0U;
  uint8_t mode = 0U;
  uint8_t lane = 0U;
  uint8_t x_odd = 0U;
  uint8_t y_odd = 0U;
  uint8_t bin_h = 0U;
  uint8_t bin_v = 0U;
  uint8_t data_fmt_a = 0U;
  uint8_t data_fmt_b = 0U;
  uint8_t op_pix_div = 0U;
  uint8_t op_sys_div = 0U;

  (void)IMX219_ReadRegister8(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_MODE_SELECT, &mode);
  (void)IMX219_ReadRegister8(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_CSI_LANE_MODE, &lane);
  (void)IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_X_ADD_STA, &x_start);
  (void)IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_X_ADD_END, &x_end);
  (void)IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_Y_ADD_STA, &y_start);
  (void)IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_Y_ADD_END, &y_end);
  (void)IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_X_OUTPUT_SIZE, &x_out);
  (void)IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_Y_OUTPUT_SIZE, &y_out);
  (void)IMX219_ReadRegister8(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_X_ODD_INC, &x_odd);
  (void)IMX219_ReadRegister8(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_Y_ODD_INC, &y_odd);
  (void)IMX219_ReadRegister8(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_BINNING_MODE_H, &bin_h);
  (void)IMX219_ReadRegister8(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_BINNING_MODE_V, &bin_v);
  (void)IMX219_ReadRegister8(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_CSI_DATA_FORMAT_A, &data_fmt_a);
  (void)IMX219_ReadRegister8(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_CSI_DATA_FORMAT_B, &data_fmt_b);
  (void)IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_LINE_LENGTH_A, &line_length);
  (void)IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_FRAME_LENGTH_A, &frame_length);
  (void)IMX219_ReadRegister8(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_OP_PIX_CLK_DIV, &op_pix_div);
  (void)IMX219_ReadRegister8(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_OP_SYS_CLK_DIV, &op_sys_div);
  (void)IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_OP_PLL_MPY, &op_pll_mpy);
  (void)IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_TEST_PATTERN, &tp_mode);
  (void)IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_TP_WINDOW_WIDTH, &tp_width);
  (void)IMX219_ReadRegister16(&hcamera_pipeline, CAMERA_PIPELINE_IMX219_REG_TP_WINDOW_HEIGHT, &tp_height);

  printf("IMX219 regs %s: mode=0x%02lX lane=0x%02lX crop=(%lu,%lu)-(%lu,%lu) out=%lux%lu odd=(0x%02lX,0x%02lX) bin=(0x%02lX,0x%02lX) fmt=(0x%02lX,0x%02lX)\r\n",
         tag,
         (uint32_t)mode,
         (uint32_t)lane,
         (uint32_t)x_start,
         (uint32_t)y_start,
         (uint32_t)x_end,
         (uint32_t)y_end,
         (uint32_t)x_out,
         (uint32_t)y_out,
         (uint32_t)x_odd,
         (uint32_t)y_odd,
         (uint32_t)bin_h,
         (uint32_t)bin_v,
         (uint32_t)data_fmt_a,
         (uint32_t)data_fmt_b);
  printf("IMX219 timing %s: line_length=%lu frame_length=%lu op_pix_div=0x%02lX op_sys_div=0x%02lX op_pll_mpy=0x%04lX test_pattern=0x%04lX tp_window=%lux%lu\r\n",
         tag,
         (uint32_t)line_length,
         (uint32_t)frame_length,
         (uint32_t)op_pix_div,
         (uint32_t)op_sys_div,
         (uint32_t)op_pll_mpy,
         (uint32_t)tp_mode,
         (uint32_t)tp_width,
         (uint32_t)tp_height);
}

static void CameraPipeline_DumpFrameBufferOverUart(void)
{
  static const char hex[] = "0123456789ABCDEF";
  volatile const uint8_t *buffer = (volatile const uint8_t *)CAMERA_PIPELINE_BUFFER_ADDRESS;
  uint32_t offset = 0U;
  uint32_t checksum = 0U;

  SCB_InvalidateDCache_by_Addr((void *)CAMERA_PIPELINE_BUFFER_ADDRESS,
                               (int32_t)CAMERA_PIPELINE_FRAME_BYTES);

  CameraPipeline_PrintFrameBufferStats();

  printf("%s width=%lu height=%lu stride=%lu bytes=%lu addr=0x%08lX\r\n",
         CAMERA_PIPELINE_UART_BEGIN_TAG,
         (uint32_t)CAMERA_PIPELINE_OUTPUT_WIDTH,
         (uint32_t)CAMERA_PIPELINE_OUTPUT_HEIGHT,
         (uint32_t)CAMERA_PIPELINE_PITCH_BYTES,
         (uint32_t)CAMERA_PIPELINE_FRAME_BYTES,
         (uint32_t)CAMERA_PIPELINE_BUFFER_ADDRESS);

  while (offset < CAMERA_PIPELINE_FRAME_BYTES)
  {
    char line[(CAMERA_PIPELINE_UART_DUMP_BYTES_PER_LINE * 2U) + 1U];
    uint32_t chunk = CAMERA_PIPELINE_FRAME_BYTES - offset;

    if (chunk > CAMERA_PIPELINE_UART_DUMP_BYTES_PER_LINE)
    {
      chunk = CAMERA_PIPELINE_UART_DUMP_BYTES_PER_LINE;
    }

    for (uint32_t i = 0U; i < chunk; i++)
    {
      uint8_t v = buffer[offset + i];
      checksum += v;
      line[i * 2U] = hex[(v >> 4) & 0x0FU];
      line[(i * 2U) + 1U] = hex[v & 0x0FU];
    }
    line[chunk * 2U] = '\0';

    printf("%s %08lX %s\r\n", CAMERA_PIPELINE_UART_DATA_TAG, (uint32_t)offset, line);
    offset += chunk;
  }

  printf("%s checksum=0x%08lX\r\n", CAMERA_PIPELINE_UART_END_TAG, (uint32_t)checksum);
}

static void CameraPipeline_PrintFrameBufferStats(void)
{
  volatile const uint8_t *buffer = (volatile const uint8_t *)CAMERA_PIPELINE_BUFFER_ADDRESS;
  uint32_t a5_count = 0U;
  uint32_t non_a5_count = 0U;
  uint32_t first_non_a5_row = CAMERA_PIPELINE_OUTPUT_HEIGHT;
  uint32_t last_non_a5_row = 0U;
  uint32_t first_non_a5_offset = CAMERA_PIPELINE_FRAME_BYTES;
  uint32_t last_non_a5_offset = 0U;

  for (uint32_t row = 0U; row < CAMERA_PIPELINE_OUTPUT_HEIGHT; row++)
  {
    uint32_t row_has_data = 0U;
    uint32_t row_offset = row * CAMERA_PIPELINE_PITCH_BYTES;

    for (uint32_t col = 0U; col < CAMERA_PIPELINE_OUTPUT_LINE_BYTES; col++)
    {
      uint32_t offset = row_offset + col;

      if (buffer[offset] == 0xA5U)
      {
        a5_count++;
      }
      else
      {
        non_a5_count++;
        row_has_data = 1U;
        if (first_non_a5_offset == CAMERA_PIPELINE_FRAME_BYTES)
        {
          first_non_a5_offset = offset;
        }
        last_non_a5_offset = offset;
      }
    }

    if (row_has_data != 0U)
    {
      if (first_non_a5_row == CAMERA_PIPELINE_OUTPUT_HEIGHT)
      {
        first_non_a5_row = row;
      }
      last_non_a5_row = row;
    }
  }

  printf("%s buffer stats: non_a5=%lu a5=%lu first_row=%ld last_row=%ld first_off=0x%08lX last_off=0x%08lX\r\n",
         CAMERA_PIPELINE_OUTPUT_FORMAT_NAME,
         (uint32_t)non_a5_count,
         (uint32_t)a5_count,
         (long)((first_non_a5_row == CAMERA_PIPELINE_OUTPUT_HEIGHT) ? -1L : (long)first_non_a5_row),
         (long)((non_a5_count == 0U) ? -1L : (long)last_non_a5_row),
         (uint32_t)first_non_a5_offset,
         (uint32_t)last_non_a5_offset);
}

static uint32_t CameraPipeline_BufferTailReady(void)
{
  volatile const uint8_t *buffer = (volatile const uint8_t *)CAMERA_PIPELINE_BUFFER_ADDRESS;
  uint32_t tail_bytes = CAMERA_PIPELINE_TAIL_READY_LINES * CAMERA_PIPELINE_PITCH_BYTES;
  uint32_t start;
  uint32_t samples = 0U;
  uint32_t prefill = 0U;

  if (tail_bytes > CAMERA_PIPELINE_FRAME_BYTES)
  {
    tail_bytes = CAMERA_PIPELINE_FRAME_BYTES;
  }

  start = CAMERA_PIPELINE_FRAME_BYTES - tail_bytes;
  start &= ~3UL;

  SCB_InvalidateDCache_by_Addr((void *)CAMERA_PIPELINE_BUFFER_ADDRESS,
                               (int32_t)CAMERA_PIPELINE_FRAME_BYTES);

  for (uint32_t i = start; (i + 3U) < CAMERA_PIPELINE_FRAME_BYTES; i += 4U)
  {
    uint32_t word = ((uint32_t)buffer[i]) |
                    ((uint32_t)buffer[i + 1U] << 8) |
                    ((uint32_t)buffer[i + 2U] << 16) |
                    ((uint32_t)buffer[i + 3U] << 24);
    samples++;
    if (word == 0xA5A5A5A5UL)
    {
      prefill++;
    }
  }

  return ((samples != 0U) && (prefill < (samples / 2U))) ? 1U : 0U;
}

static void CameraPipeline_ToExposureShiftMultiplier(uint32_t gain, uint8_t *shift, uint8_t *multiplier)
{
  uint64_t val = gain;

  val = (val * 128ULL) / CAMERA_PIPELINE_PIPE1_GAIN_1X;

  *shift = 0U;
  while (val >= 256ULL)
  {
    val /= 2ULL;
    (*shift)++;
  }

  *multiplier = (uint8_t)val;
}

void HAL_DCMIPP_PIPE_FrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp_cb, uint32_t Pipe)
{
  (void)hdcmipp_cb;
  if (Pipe == CAMERA_PIPELINE_CAPTURE_PIPE)
  {
    pipeline_status.frame_count++;
  }
}

void HAL_DCMIPP_PIPE_LimitEventCallback(DCMIPP_HandleTypeDef *hdcmipp_cb, uint32_t Pipe)
{
  (void)hdcmipp_cb;

  if ((Pipe == DCMIPP_PIPE0) && (CAMERA_PIPELINE_CAPTURE_PIPE == DCMIPP_PIPE0))
  {
    pipeline_p0_limit_count++;
    pipeline_p0_limit_tick = HAL_GetTick();
    pipeline_p0_limit_dcc = DCMIPP->P0DCCNTR;
    CLEAR_BIT(DCMIPP->P0FCTCR, DCMIPP_P0FCTCR_CPTREQ);
    CLEAR_BIT(DCMIPP->P0FSCR, DCMIPP_P0FSCR_PIPEN);
  }
}

void HAL_DCMIPP_PIPE_VsyncEventCallback(DCMIPP_HandleTypeDef *hdcmipp_cb, uint32_t Pipe)
{
  (void)hdcmipp_cb;

  if (pipeline_status.state != CAMERA_PIPELINE_STATE_RUNNING)
  {
    return;
  }

  switch (Pipe)
  {
    case DCMIPP_PIPE0:
#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
      ISP_IncDumpFrameId(&hcamera_isp);
#endif
      break;
    case DCMIPP_PIPE1:
#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
      ISP_IncMainFrameId(&hcamera_isp);
      ISP_GatherStatistics(&hcamera_isp);
#endif
      break;
    case DCMIPP_PIPE2:
#if (CAMERA_PIPELINE_USE_ISP_RUNTIME != 0U)
      ISP_IncAncillaryFrameId(&hcamera_isp);
#endif
      break;
    default:
      break;
  }
}

void HAL_DCMIPP_CSI_StartOfFrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp_cb, uint32_t VirtualChannel)
{
  (void)hdcmipp_cb;
  if (VirtualChannel == DCMIPP_VIRTUAL_CHANNEL0)
  {
    pipeline_sof_count++;
  }
}

void HAL_DCMIPP_CSI_EndOfFrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp_cb, uint32_t VirtualChannel)
{
  (void)hdcmipp_cb;
  if (VirtualChannel == DCMIPP_VIRTUAL_CHANNEL0)
  {
    pipeline_eof_count++;
  }
}

#if (CAMERA_PIPELINE_CSI_LINE_BYTE_PROBE != 0U)
void HAL_DCMIPP_CSI_LineByteEventCallback(DCMIPP_HandleTypeDef *hdcmipp_cb, uint32_t Counter)
{
  (void)hdcmipp_cb;

  switch (Counter)
  {
    case DCMIPP_CSI_COUNTER0:
      pipeline_csi_lb_count[0]++;
      break;
    case DCMIPP_CSI_COUNTER1:
      pipeline_csi_lb_count[1]++;
      break;
    case DCMIPP_CSI_COUNTER2:
      pipeline_csi_lb_count[2]++;
      break;
    case DCMIPP_CSI_COUNTER3:
      pipeline_csi_lb_count[3]++;
      break;
    default:
      break;
  }
}
#endif
