#include "camera_debug.h"
#include "main.h"
#include "imx219.h"
#include <stdio.h>

#ifndef LED1_GPIO_Port
#define LED1_GPIO_Port LED_GPIO_Port
#endif

#ifndef LED1_Pin
#define LED1_Pin LED_Pin
#endif

#define CAMERA_DEBUG_I2C_ONLY 0U
#define CAMERA_DEBUG_SENSOR_STREAM_ONLY 0U
#define CAMERA_DEBUG_CSI_MONITOR_ONLY 0U
#define CAMERA_DEBUG_PIPE_MONITOR_ONLY 0U
#define IMX219_DEBUG_START_STREAM 1U
#define IMX219_DEBUG_TEST_PATTERN 0U
#define IMX219_DEBUG_SLOW_TIMING 0U
#define IMX219_DEBUG_LINE_LENGTH 20000U
#define IMX219_DEBUG_FRAME_LENGTH 3000U
#define IMX219_DEBUG_LOW_LINK 0U
#define IMX219_DEBUG_OP_PLL_MULT 0x0039U
#define IMX219_DEBUG_LANE_COUNT 2U
#define IMX219_DEBUG_EXPOSURE_GAIN 1U
#define IMX219_DEBUG_ANALOG_GAIN 0x40U
#define IMX219_DEBUG_EXPOSURE_LINES 0x0400U
#define IMX219_DEBUG_DIGITAL_GAIN 0x0100U
#define IMX219_DEBUG_REAL_FRAME_SETTLE_MS 1000U
#define DCMIPP_VERIFY_FRAME_DONE_TIMEOUT_MS 5000U
#define DCMIPP_VERIFY_PIPE_IDLE_TIMEOUT_MS 100U
#define DCMIPP_VERIFY_FREEZE_SETTLE_MS 50U
#define IMX219_DEBUG_RESOLUTION IMX219_R640_480
#define DCMIPP_VERIFY_WIDTH 640U
#define DCMIPP_VERIFY_HEIGHT 480U
#define DCMIPP_VERIFY_OUTPUT_WIDTH DCMIPP_VERIFY_WIDTH
#define DCMIPP_VERIFY_OUTPUT_HEIGHT DCMIPP_VERIFY_HEIGHT
#define DCMIPP_VERIFY_RAW10_FRAME_BYTES (((DCMIPP_VERIFY_WIDTH * DCMIPP_VERIFY_HEIGHT * 10U) + 7U) / 8U)
#define DCMIPP_VERIFY_RAW10_FRAME_WORDS ((DCMIPP_VERIFY_RAW10_FRAME_BYTES + 3U) / 4U)
#define DCMIPP_VERIFY_RAW10_LINE_BYTES (((DCMIPP_VERIFY_WIDTH * 10U) + 7U) / 8U)
#define DCMIPP_VERIFY_RGB565_LINE_BYTES (DCMIPP_VERIFY_WIDTH * 2U)
#define DCMIPP_VERIFY_LINE_PITCH DCMIPP_VERIFY_RGB565_LINE_BYTES
#define DCMIPP_VERIFY_PIPE0_DUMP_BYTES (DCMIPP_VERIFY_WIDTH * DCMIPP_VERIFY_HEIGHT * 2U)
#define DCMIPP_VERIFY_PIPE0_DUMP_WORDS ((DCMIPP_VERIFY_PIPE0_DUMP_BYTES + 3U) / 4U)
#define DCMIPP_VERIFY_RGB565_FRAME_BYTES (DCMIPP_VERIFY_WIDTH * DCMIPP_VERIFY_HEIGHT * 2U)
#define DCMIPP_VERIFY_RGB565_FRAME_WORDS ((DCMIPP_VERIFY_RGB565_FRAME_BYTES + 3U) / 4U)
#define DCMIPP_VERIFY_BUFFER_BYTES (1024U * 1024U)
#define DCMIPP_VERIFY_BUFFER_WORDS ((DCMIPP_VERIFY_BUFFER_BYTES + 3U) / 4U)
#define DCMIPP_VERIFY_FRAME_BYTES DCMIPP_VERIFY_RGB565_FRAME_BYTES
#define DCMIPP_VERIFY_FRAME_WORDS DCMIPP_VERIFY_RGB565_FRAME_WORDS
#define DCMIPP_VERIFY_ENABLE_DOWNSIZE 0U
#define DCMIPP_DOWNSIZE_DIV_FACTOR(SRC, DST) \
  ((((uint32_t)((1024U * (DST)) / (SRC))) > 1023U) ? 1023U : ((uint32_t)((1024U * (DST)) / (SRC))))
#define DCMIPP_DOWNSIZE_RATIO(SRC, DST) \
  ((((uint32_t)(((SRC) * 8192U) / (DST))) < 8192U) ? 8192U : ((uint32_t)(((SRC) * 8192U) / (DST))))
#define DCMIPP_VERIFY_PHY_BITRATE DCMIPP_CSI_PHY_BT_900
#define DCMIPP_VERIFY_PHY_SCAN 0U
#define DCMIPP_VERIFY_PHY_SCAN_TIMEOUT_MS 600U
#define DCMIPP_VERIFY_PIXEL_PACKER DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1
#define DCMIPP_VERIFY_LANE_MAPPING DCMIPP_CSI_PHYSICAL_DATA_LANES
#define DCMIPP_VERIFY_NUMBER_OF_LANES DCMIPP_CSI_TWO_DATA_LANES
#define DCMIPP_VERIFY_CAPTURE_PIPE DCMIPP_PIPE1
#define DCMIPP_VERIFY_CAPTURE_MODE DCMIPP_MODE_SNAPSHOT
#define DCMIPP_VERIFY_DATA_TYPE_MODE DCMIPP_DTMODE_DTIDA
#define DCMIPP_VERIFY_DUMP_HEADER 0U
#define DCMIPP_VERIFY_PIPE0_LIMIT_WORDS 0U
#define DCMIPP_VERIFY_PIPE0_DECIMATE 0U
#define DCMIPP_VERIFY_PIPE0_BYTE_START DCMIPP_OEBS_ODD
#define DCMIPP_VERIFY_PIPE0_BYTE_SELECT DCMIPP_BSM_DATA_OUT_2
#define DCMIPP_VERIFY_PIPE0_LINE_START DCMIPP_OELS_ODD
#define DCMIPP_VERIFY_PIPE0_LINE_SELECT DCMIPP_LSM_ALTERNATE_2
#define DCMIPP_VERIFY_PIPE0_CROP 0U
#define DCMIPP_VERIFY_PIPE0_CROP_HSTART 0U
#define DCMIPP_VERIFY_PIPE0_CROP_VSTART 0U
#define DCMIPP_VERIFY_PIPE0_CROP_HSIZE DCMIPP_VERIFY_WIDTH
#define DCMIPP_VERIFY_PIPE0_CROP_VSIZE DCMIPP_VERIFY_HEIGHT
#define DCMIPP_VERIFY_CONFIG_IPPLUG 0U
#define DCMIPP_VERIFY_IPPLUG_PAGE_SIZE DCMIPP_MEMORY_PAGE_SIZE_8KBYTES
#define DCMIPP_VERIFY_BUFFER_FILL 0xA5U
#define DCMIPP_VERIFY_MASK_DPHY_IRQ 1U
#define DCMIPP_VERIFY_CACHE_MAINT 1U
#define DCMIPP_VERIFY_WAIT_DPHY_STOP 1U
#define DCMIPP_VERIFY_DPHY_STOP_TIMEOUT_MS 100U
#define DCMIPP_VERIFY_DUMP_RAW16_LAYOUT 0U
#define DCMIPP_VERIFY_DUMP_RGB565_LAYOUT 1U
#define DCMIPP_VERIFY_RAW16_DUMP_PIXELS 32U
#define DCMIPP_VERIFY_RAW16_DUMP_LINES 2U
#define DCMIPP_VERIFY_RGB565_DUMP_PIXELS 16U
#define DCMIPP_VERIFY_RGB565_DUMP_LINES 2U
#define DCMIPP_VERIFY_PIPE1_BLACK_LEVEL 16U
#define DCMIPP_VERIFY_PIPE1_EXPOSURE_SHIFT 6U
#define DCMIPP_VERIFY_PIPE1_WB_R_MULT 80U
#define DCMIPP_VERIFY_PIPE1_WB_G_MULT 64U
#define DCMIPP_VERIFY_PIPE1_WB_B_MULT 80U
#define DCMIPP_VERIFY_PIPE1_BAYER DCMIPP_RAWBAYER_RGGB
#define DCMIPP_VERIFY_PIPE1_BAYER_STRENGTH DCMIPP_RAWBAYER_ALGO_STRENGTH_8
#define DCMIPP_CSI_DPHY_DATA_LANE_ERROR_IT                                         \
  (DCMIPP_CSI_IT_ECTRLDL1 | DCMIPP_CSI_IT_ESYNCESCDL1 | DCMIPP_CSI_IT_EESCDL1 |    \
   DCMIPP_CSI_IT_ESOTSYNCDL1 | DCMIPP_CSI_IT_ESOTDL1 | DCMIPP_CSI_IT_ECTRLDL0 |    \
   DCMIPP_CSI_IT_ESYNCESCDL0 | DCMIPP_CSI_IT_EESCDL0 | DCMIPP_CSI_IT_ESOTSYNCDL0 | \
   DCMIPP_CSI_IT_ESOTDL0)


DCMIPP_HandleTypeDef hdcmipp;

static IMX219_Object_t hcamera;
static volatile uint32_t imx219_id;
static volatile uint32_t imx219_status;
static volatile uint32_t dcmipp_status;
static volatile uint32_t dcmipp_frame_count;
static volatile uint32_t dcmipp_frame_done_tick;
static volatile uint32_t dcmipp_capture_frozen;
static volatile uint32_t dcmipp_freeze_tick;
static volatile uint32_t imx219_streaming;
static volatile uint32_t dcmipp_vsync_count;
static volatile uint32_t dcmipp_sof_count;
static volatile uint32_t dcmipp_eof_count;
static volatile uint32_t dcmipp_pipe_error_count;
static volatile uint32_t dcmipp_line_error_count;
static volatile uint32_t dcmipp_common_error_count;
static volatile uint32_t dcmipp_short_packet_count;
static volatile uint32_t dcmipp_last_error;
static volatile uint32_t dcmipp_csi_sr0;
static volatile uint32_t dcmipp_csi_sr1;
static volatile uint32_t dcmipp_csi_cr;
static volatile uint32_t dcmipp_csi_ier0;
static volatile uint32_t dcmipp_csi_ier1;
static volatile uint32_t dcmipp_csi_pfcr;
static volatile uint32_t dcmipp_csi_pcr;
static volatile uint32_t dcmipp_csi_lmcfgr;
static volatile uint32_t dcmipp_csi_vc0cfgr1;
static volatile uint32_t dcmipp_csi_vc0cfgr2;
static volatile uint32_t dcmipp_csi_vc0cfgr3;
static volatile uint32_t dcmipp_csi_vc0cfgr4;
static volatile uint32_t dcmipp_csi_lb0cfgr;
static volatile uint32_t dcmipp_csi_prgitr;
static volatile uint32_t dcmipp_csi_err1;
static volatile uint32_t dcmipp_csi_err2;
static volatile uint32_t dcmipp_csi_spdfr;
static volatile uint32_t dcmipp_csi_prcr;
static volatile uint32_t dcmipp_csi_pmcr;
static volatile uint32_t dcmipp_csi_lb0_count;
static volatile uint32_t dcmipp_csi_lb_last_counter;
static volatile uint32_t dcmipp_dphy_sample_count;
static volatile uint32_t dcmipp_cmcr;
static volatile uint32_t dcmipp_cmsr1;
static volatile uint32_t dcmipp_p0_dccntr;
static volatile uint32_t dcmipp_p0_dclmtr;
static volatile uint32_t dcmipp_p0_fscr;
static volatile uint32_t dcmipp_p0_fctcr;
static volatile uint32_t dcmipp_p0_ppcr;
static volatile uint32_t dcmipp_p0_ppm0ar1;
static volatile uint32_t dcmipp_p0_stm0ar;
static volatile uint32_t dcmipp_p0_cfscr;
static volatile uint32_t dcmipp_p0_cfctcr;
static volatile uint32_t dcmipp_p0_cppcr;
static volatile uint32_t dcmipp_p0_cppm0ar1;
static volatile uint32_t dcmipp_p0_sr;
static volatile uint32_t dcmipp_p0_ier;
static volatile uint32_t dcmipp_p0_scstr;
static volatile uint32_t dcmipp_p0_scszr;
static volatile uint32_t dcmipp_p1_fscr;
static volatile uint32_t dcmipp_p1_fctcr;
static volatile uint32_t dcmipp_p1_dmcr;
static volatile uint32_t dcmipp_p1_dscr;
static volatile uint32_t dcmipp_p1_dsrtior;
static volatile uint32_t dcmipp_p1_dsszr;
static volatile uint32_t dcmipp_p1_ppcr;
static volatile uint32_t dcmipp_p1_ppm0ar1;
static volatile uint32_t dcmipp_p1_ppm0pr;
static volatile uint32_t dcmipp_p1_stm0ar;
static volatile uint32_t dcmipp_p1_cfscr;
static volatile uint32_t dcmipp_p1_cfctcr;
static volatile uint32_t dcmipp_p1_cppcr;
static volatile uint32_t dcmipp_p1_cppm0ar1;
static volatile uint32_t dcmipp_p1_cppm0pr;
static volatile uint32_t dcmipp_p1_sr;
static volatile uint32_t dcmipp_p1_ier;
static volatile uint32_t dcmipp_cmsr2;
static volatile uint32_t dcmipp_cmier;
static volatile uint32_t dcmipp_ipgr1;
static volatile uint32_t dcmipp_ipc1r1;
static volatile uint32_t dcmipp_ipc1r2;
static volatile uint32_t dcmipp_ipc1r3;
static volatile uint32_t dcmipp_ipc2r1;
static volatile uint32_t dcmipp_ipc2r2;
static volatile uint32_t dcmipp_ipc2r3;
static volatile uint32_t dcmipp_ipc3r1;
static volatile uint32_t dcmipp_ipc3r2;
static volatile uint32_t dcmipp_ipc3r3;
static volatile uint32_t dcmipp_ipc4r1;
static volatile uint32_t dcmipp_ipc4r2;
static volatile uint32_t dcmipp_ipc4r3;
static volatile uint32_t dcmipp_ipc5r1;
static volatile uint32_t dcmipp_ipc5r2;
static volatile uint32_t dcmipp_ipc5r3;
static volatile uint32_t dcmipp_buf_checksum;
static volatile uint32_t dcmipp_buf_nonzero;
static volatile uint32_t dcmipp_buf_changed;
static volatile uint32_t dcmipp_buf_w0;
static volatile uint32_t dcmipp_buf_w1;
static volatile uint32_t dcmipp_buf_w2;
static volatile uint32_t dcmipp_buf_w3;
static volatile uint32_t dcmipp_buf_w4;
static volatile uint32_t dcmipp_buf_w5;
static volatile uint32_t dcmipp_buf_w6;
static volatile uint32_t dcmipp_buf_w7;
static volatile uint32_t dcmipp_raw16_layout_dumped;
static volatile uint32_t dcmipp_raw16_valid;
static volatile uint32_t dcmipp_raw16_min;
static volatile uint32_t dcmipp_raw16_max;
static volatile uint32_t dcmipp_raw16_low6_nonzero;
static volatile uint32_t dcmipp_raw16_high6_nonzero;
static volatile uint32_t dcmipp_raw16_sat10_count;
static volatile uint32_t dcmipp_raw16_align;
static volatile uint32_t dcmipp_raw16_s0;
static volatile uint32_t dcmipp_raw16_s1;
static volatile uint32_t dcmipp_raw16_s2;
static volatile uint32_t dcmipp_raw16_s3;
static volatile uint32_t dcmipp_raw16_s4;
static volatile uint32_t dcmipp_raw16_s5;
static volatile uint32_t dcmipp_raw16_s6;
static volatile uint32_t dcmipp_raw16_s7;
static volatile uint32_t dcmipp_rgb565_layout_dumped;
static volatile uint32_t dcmipp_rgb565_valid;
static volatile uint32_t dcmipp_rgb565_min_r5;
static volatile uint32_t dcmipp_rgb565_max_r5;
static volatile uint32_t dcmipp_rgb565_min_g6;
static volatile uint32_t dcmipp_rgb565_max_g6;
static volatile uint32_t dcmipp_rgb565_min_b5;
static volatile uint32_t dcmipp_rgb565_max_b5;
static volatile uint32_t dcmipp_rgb565_s0;
static volatile uint32_t dcmipp_rgb565_s1;
static volatile uint32_t dcmipp_rgb565_s2;
static volatile uint32_t dcmipp_rgb565_s3;
static uint8_t dcmipp_frame_buffer[DCMIPP_VERIFY_BUFFER_BYTES]
    __attribute__((section(".noncacheable"), aligned(8192)));


static void Camera_LogStatus(const char *Step);
static void Camera_DumpRegisters(const char *Tag);
#if (DCMIPP_VERIFY_CONFIG_IPPLUG != 0U)
static void DCMIPP_ConfigIPPlug(void);
#endif
#if (CAMERA_DEBUG_I2C_ONLY == 0U)
static void DCMIPP_SetCsiConfig(uint32_t PhyBitrate, uint32_t LaneMapping);
static void DCMIPP_SetCsiPhyBitrate(uint32_t PhyBitrate);
#endif
static void DCMIPP_StartCapture(void);
#if (DCMIPP_VERIFY_PHY_SCAN != 0U)
static uint32_t DCMIPP_StopCaptureAndStream(void);
static void DCMIPP_ResetRuntimeCounters(void);
static uint32_t DCMIPP_TrySetCsiConfig(uint32_t PhyBitrate, uint32_t LaneMapping);
static uint32_t DCMIPP_TryStartCapture(void);
static void DCMIPP_RestoreDefaultAfterScan(void);
static uint32_t DCMIPP_RunPhyBitrateScan(void);
#endif
static uint32_t DCMIPP_WaitForFrameComplete(uint32_t TimeoutMs);
static uint32_t DCMIPP_WaitForPipeIdle(uint32_t TimeoutMs);
static void DCMIPP_FreezeCaptureForDump(void);
static void DCMIPP_LogPipeRegisters(const char *Tag);
static void DCMIPP_SampleCsiFlags(void);
static void DCMIPP_ClearCsiFlags(void);
static uint32_t DCMIPP_WaitForDphyStopState(uint32_t TimeoutMs);
static void DCMIPP_UpdateBufferStats(void);
#if (DCMIPP_VERIFY_DUMP_RAW16_LAYOUT != 0U)
static void DCMIPP_DumpRaw16Layout(void);
#endif
static void DCMIPP_DumpRGB565Layout(void);
static void DCMIPP_FillBuffer(uint8_t Value);
static void DCMIPP_CleanBuffer(void);
static void DCMIPP_InvalidateBuffer(void);



#if (CAMERA_DEBUG_I2C_ONLY == 0U)
static void MX_DCMIPP_Init(void);
#endif

void CameraDebug_InitAndStart(I2C_HandleTypeDef *hi2c)
{
  uint32_t sensor_id;

#if (CAMERA_DEBUG_I2C_ONLY == 0U)
  printf("CSI IMX219 boot: before MX_DCMIPP_Init\r\n");
  MX_DCMIPP_Init();
  printf("CSI IMX219 boot: after MX_DCMIPP_Init\r\n");
  printf("RCC clocks: hclk=%lu dcmipp=%lu csi=%lu pll1cfgr1=0x%08lX pll1cfgr2=0x%08lX pll1cfgr3=0x%08lX ic17cfgr=0x%08lX ic18cfgr=0x%08lX\r\n",
         HAL_RCC_GetHCLKFreq(),
         HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_DCMIPP),
         HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_CSI),
         RCC->PLL1CFGR1,
         RCC->PLL1CFGR2,
         RCC->PLL1CFGR3,
         RCC->IC17CFGR,
         RCC->IC18CFGR);
#endif
  /* USER CODE BEGIN 2 */
#if (CAMERA_DEBUG_I2C_ONLY != 0U)
  printf("\r\nIMX219 sensor stream-only bring-up start\r\n");
#else
  printf("\r\nCSI IMX219 bring-up start\r\n");
#endif
  printf("USART3: 115200 8N1, I2C addr: 0x%02lX, expected ID: 0x%04lX\r\n",
         (uint32_t)IMX219_I2C_ADDR_7BIT, (uint32_t)IMX219_CHIP_ID);
#if (CAMERA_DEBUG_I2C_ONLY == 0U)
  printf("DCMIPP: %lux%lu RAW10 on VC0, output=%lux%lu, lanes=%lu, lane_map=%lu, PHY BT index=%lu, PIPE%lu mode=0x%08lX buffer=%lu bytes/%lu words, raw10_in=%lu bytes/%lu words, pipe0_dump=%lu bytes/%lu words, pixel_packer=0x%08lX pitch=%lu, downsize=%lu, p0_limit_words=%lu, p0_decimate=%lu byte_sel=0x%08lX line_sel=0x%08lX, p0_crop=%lu crop_h=%lu crop_v=%lu, test_pattern=%lu, slow_timing=%lu line=%lu frame=%lu, low_link=%lu low_link_op_pll=0x%04lX, dtmode=%lu, dump_header=%lu, ipplug_cfg=%lu, cache_maint=%lu\r\n",
         (uint32_t)DCMIPP_VERIFY_WIDTH, (uint32_t)DCMIPP_VERIFY_HEIGHT,
         (uint32_t)DCMIPP_VERIFY_OUTPUT_WIDTH, (uint32_t)DCMIPP_VERIFY_OUTPUT_HEIGHT,
         (uint32_t)(DCMIPP_VERIFY_NUMBER_OF_LANES >> CSI_LMCFGR_LANENB_Pos),
         (uint32_t)DCMIPP_VERIFY_LANE_MAPPING, (uint32_t)DCMIPP_VERIFY_PHY_BITRATE,
         (uint32_t)DCMIPP_VERIFY_CAPTURE_PIPE,
         (uint32_t)DCMIPP_VERIFY_CAPTURE_MODE,
         (uint32_t)DCMIPP_VERIFY_BUFFER_BYTES,
         (uint32_t)DCMIPP_VERIFY_BUFFER_WORDS,
         (uint32_t)DCMIPP_VERIFY_RAW10_FRAME_BYTES,
         (uint32_t)DCMIPP_VERIFY_RAW10_FRAME_WORDS,
         (uint32_t)DCMIPP_VERIFY_PIPE0_DUMP_BYTES,
         (uint32_t)DCMIPP_VERIFY_PIPE0_DUMP_WORDS,
         (uint32_t)DCMIPP_VERIFY_PIXEL_PACKER,
         (uint32_t)DCMIPP_VERIFY_LINE_PITCH,
         (uint32_t)DCMIPP_VERIFY_ENABLE_DOWNSIZE,
         (uint32_t)DCMIPP_VERIFY_PIPE0_LIMIT_WORDS,
         (uint32_t)DCMIPP_VERIFY_PIPE0_DECIMATE,
         (uint32_t)DCMIPP_VERIFY_PIPE0_BYTE_SELECT,
         (uint32_t)DCMIPP_VERIFY_PIPE0_LINE_SELECT,
         (uint32_t)DCMIPP_VERIFY_PIPE0_CROP,
         (uint32_t)DCMIPP_VERIFY_PIPE0_CROP_HSIZE,
         (uint32_t)DCMIPP_VERIFY_PIPE0_CROP_VSIZE,
         (uint32_t)IMX219_DEBUG_TEST_PATTERN,
         (uint32_t)IMX219_DEBUG_SLOW_TIMING,
         (uint32_t)IMX219_DEBUG_LINE_LENGTH,
         (uint32_t)IMX219_DEBUG_FRAME_LENGTH,
         (uint32_t)IMX219_DEBUG_LOW_LINK,
         (uint32_t)IMX219_DEBUG_OP_PLL_MULT,
         (uint32_t)DCMIPP_VERIFY_DATA_TYPE_MODE,
         (uint32_t)DCMIPP_VERIFY_DUMP_HEADER,
         (uint32_t)DCMIPP_VERIFY_CONFIG_IPPLUG,
         (uint32_t)DCMIPP_VERIFY_CACHE_MAINT);
#endif

  imx219_status = 1U;
  Camera_LogStatus("enable module power");
  HAL_GPIO_WritePin(EN_MODULE_GPIO_Port, EN_MODULE_Pin, GPIO_PIN_SET);
  HAL_Delay(200);

  imx219_status = 2U;
  Camera_LogStatus("register I2C bus");
  if (IMX219_RegisterBusIO(&hcamera, hi2c, IMX219_I2C_ADDR_7BIT) != IMX219_OK)
  {
    Error_Handler();
  }

  imx219_status = 3U;
  Camera_LogStatus("read sensor ID");
  if ((IMX219_ReadID(&hcamera, &sensor_id) != IMX219_OK) || (sensor_id != IMX219_CHIP_ID))
  {
    imx219_id = sensor_id;
    Error_Handler();
  }
  imx219_id = sensor_id;
  printf("IMX219 ID OK: 0x%04lX\r\n", imx219_id);

  imx219_status = 4U;
  Camera_LogStatus("enter CSI LP-11");
  if (IMX219_EnterLp11(&hcamera) != IMX219_OK)
  {
    Error_Handler();
  }

  imx219_status = 5U;
  Camera_LogStatus("write RAW10 init table");
  if (IMX219_Init(&hcamera, IMX219_DEBUG_RESOLUTION, IMX219_RAW10) != IMX219_OK)
  {
    Error_Handler();
  }
  printf("IMX219 init OK: %lux%lu RAW10, stream kept in standby\r\n", hcamera.Width, hcamera.Height);

#if (IMX219_DEBUG_LANE_COUNT != 2U)
  if (IMX219_SetCsiLaneMode(&hcamera, (uint8_t)IMX219_DEBUG_LANE_COUNT) != IMX219_OK)
  {
    Error_Handler();
  }
  printf("IMX219 CSI lane mode ON: lanes=%lu\r\n", (uint32_t)IMX219_DEBUG_LANE_COUNT);
#endif

#if (IMX219_DEBUG_SLOW_TIMING != 0U)
  if (IMX219_SetFrameTiming(&hcamera,
                            (uint16_t)IMX219_DEBUG_LINE_LENGTH,
                            (uint16_t)IMX219_DEBUG_FRAME_LENGTH) != IMX219_OK)
  {
    Error_Handler();
  }
  printf("IMX219 slow timing ON: line_length=%lu frame_length=%lu\r\n",
         (uint32_t)IMX219_DEBUG_LINE_LENGTH,
         (uint32_t)IMX219_DEBUG_FRAME_LENGTH);
#endif

#if (IMX219_DEBUG_LOW_LINK != 0U)
  if (IMX219_SetDebugOpPllMultiplier(&hcamera, (uint16_t)IMX219_DEBUG_OP_PLL_MULT) != IMX219_OK)
  {
    Error_Handler();
  }
  printf("IMX219 low link ON: op_pll_mult=0x%04lX\r\n", (uint32_t)IMX219_DEBUG_OP_PLL_MULT);
#endif

  if (IMX219_SetTestPattern(&hcamera, IMX219_DEBUG_TEST_PATTERN) != IMX219_OK)
  {
    Error_Handler();
  }
  printf("IMX219 test pattern %s\r\n", (IMX219_DEBUG_TEST_PATTERN != 0U) ? "ON" : "OFF");

#if (IMX219_DEBUG_EXPOSURE_GAIN != 0U)
  if (IMX219_SetExposureGain(&hcamera,
                             (uint16_t)IMX219_DEBUG_EXPOSURE_LINES,
                             (uint8_t)IMX219_DEBUG_ANALOG_GAIN,
                             (uint16_t)IMX219_DEBUG_DIGITAL_GAIN) != IMX219_OK)
  {
    Error_Handler();
  }
  printf("IMX219 exposure/gain ON: analog=0x%02lX exposure=0x%04lX digital=0x%04lX\r\n",
         (uint32_t)IMX219_DEBUG_ANALOG_GAIN,
         (uint32_t)IMX219_DEBUG_EXPOSURE_LINES,
         (uint32_t)IMX219_DEBUG_DIGITAL_GAIN);
#endif
  Camera_DumpRegisters("after_init");
#if (CAMERA_DEBUG_I2C_ONLY != 0U)
  dcmipp_status = 0U;
#if (CAMERA_DEBUG_SENSOR_STREAM_ONLY != 0U)
  imx219_status = 6U;
  Camera_LogStatus("start stream without DCMIPP");
  if (IMX219_Start(&hcamera) != IMX219_OK)
  {
    Error_Handler();
  }
  imx219_streaming = 1U;
  HAL_Delay(IMX219_DEBUG_REAL_FRAME_SETTLE_MS);
  Camera_DumpRegisters("stream_only_on");
  imx219_status = 7U;
  printf("IMX219 stream-only bring-up PASS: sensor streaming, DCMIPP not initialized\r\n");
#else
  imx219_status = 7U;
  imx219_streaming = 0U;
  printf("IMX219 I2C-only bring-up PASS: sensor configured and left in standby\r\n");
#endif
  dcmipp_status = 0U;
  return;
#endif
#if (DCMIPP_VERIFY_WAIT_DPHY_STOP != 0U)
  uint32_t dphy_stop_ok = DCMIPP_WaitForDphyStopState(DCMIPP_VERIFY_DPHY_STOP_TIMEOUT_MS);
  DCMIPP_SampleCsiFlags();
  printf("DCMIPP D-PHY stop-state before_arm: ok=%lu timeout_ms=%lu sr1=0x%08lX stop_dl0=%lu stop_dl1=%lu stop_clk=%lu sync_dl0=%lu sync_dl1=%lu act_dl0=%lu act_dl1=%lu act_clk=%lu\r\n",
         dphy_stop_ok,
         (uint32_t)DCMIPP_VERIFY_DPHY_STOP_TIMEOUT_MS,
         dcmipp_csi_sr1,
         (uint32_t)(((dcmipp_csi_sr1 & CSI_SR1_STOPDL0F) != 0U) ? 1U : 0U),
         (uint32_t)(((dcmipp_csi_sr1 & CSI_SR1_STOPDL1F) != 0U) ? 1U : 0U),
         (uint32_t)(((dcmipp_csi_sr1 & CSI_SR1_STOPCLF) != 0U) ? 1U : 0U),
         (uint32_t)(((dcmipp_csi_sr1 & CSI_SR1_SYNCDL0F) != 0U) ? 1U : 0U),
         (uint32_t)(((dcmipp_csi_sr1 & CSI_SR1_SYNCDL1F) != 0U) ? 1U : 0U),
         (uint32_t)(((dcmipp_csi_sr1 & CSI_SR1_ACTDL0F) != 0U) ? 1U : 0U),
         (uint32_t)(((dcmipp_csi_sr1 & CSI_SR1_ACTDL1F) != 0U) ? 1U : 0U),
         (uint32_t)(((dcmipp_csi_sr1 & CSI_SR1_ACTCLF) != 0U) ? 1U : 0U));
#endif
  DCMIPP_ClearCsiFlags();

#if (CAMERA_DEBUG_CSI_MONITOR_ONLY != 0U)
  dcmipp_status = 2U;
  printf("[DCMIPP:%lu] CSI monitor-only: PIPE%lu will not be armed\r\n",
         dcmipp_status, (uint32_t)DCMIPP_VERIFY_CAPTURE_PIPE);

  imx219_status = 6U;
  Camera_LogStatus("start stream for CSI monitor");
  if (IMX219_Start(&hcamera) != IMX219_OK)
  {
    Error_Handler();
  }
  imx219_streaming = 1U;
  HAL_Delay(IMX219_DEBUG_REAL_FRAME_SETTLE_MS);
  Camera_DumpRegisters("csi_monitor_stream_on");
  DCMIPP_SampleCsiFlags();
  imx219_status = 7U;
  printf("CSI monitor-only active: sr0=0x%08lX sr1=0x%08lX err1=0x%08lX err2=0x%08lX sof=%lu eof=%lu spkt=%lu lb0=%lu\r\n",
         dcmipp_csi_sr0, dcmipp_csi_sr1, dcmipp_csi_err1, dcmipp_csi_err2,
         dcmipp_sof_count, dcmipp_eof_count,
         dcmipp_short_packet_count, dcmipp_csi_lb0_count);
  return;
#endif

  DCMIPP_LogPipeRegisters("before_arm");

  dcmipp_status = 2U;
  printf("[DCMIPP:%lu] arm PIPE%lu before sensor stream\r\n",
         dcmipp_status, (uint32_t)DCMIPP_VERIFY_CAPTURE_PIPE);
  DCMIPP_FillBuffer((uint8_t)DCMIPP_VERIFY_BUFFER_FILL);
  DCMIPP_UpdateBufferStats();
  printf("DCMIPP buffer prefill: fill=0x%02lX buf_changed=%lu buf_w0=0x%08lX\r\n",
         (uint32_t)DCMIPP_VERIFY_BUFFER_FILL, dcmipp_buf_changed, dcmipp_buf_w0);
  DCMIPP_StartCapture();
  DCMIPP_LogPipeRegisters("after_arm");
  dcmipp_status = 3U;
  dcmipp_capture_frozen = 0U;
  printf("DCMIPP capture armed: buffer=0x%08lX size=%lu bytes\r\n",
         (uint32_t)(uintptr_t)dcmipp_frame_buffer,
         (uint32_t)sizeof(dcmipp_frame_buffer));

#if (IMX219_DEBUG_START_STREAM != 0U)
  imx219_status = 6U;
  Camera_LogStatus("start stream");
  if (IMX219_Start(&hcamera) != IMX219_OK)
  {
    Error_Handler();
  }
  imx219_streaming = 1U;
  printf("IMX219 stream ON\r\n");
  Camera_DumpRegisters("stream_on");
#endif

#if (CAMERA_DEBUG_PIPE_MONITOR_ONLY != 0U)
  HAL_Delay(IMX219_DEBUG_REAL_FRAME_SETTLE_MS);
  DCMIPP_SampleCsiFlags();
  DCMIPP_UpdateBufferStats();
  printf("DCMIPP PIPE monitor-only active: frames=%lu p0dcc=%lu/%lu buf_changed=%lu raw16=%lu sof=%lu eof=%lu err=0x%08lX sr0=0x%08lX sr1=0x%08lX csi_err1=0x%08lX csi_err2=0x%08lX p0sr=0x%08lX\r\n",
         dcmipp_frame_count,
         dcmipp_p0_dccntr,
         (uint32_t)DCMIPP_VERIFY_FRAME_BYTES,
         dcmipp_buf_changed,
         dcmipp_raw16_valid,
         dcmipp_sof_count,
         dcmipp_eof_count,
         dcmipp_last_error,
         dcmipp_csi_sr0,
         dcmipp_csi_sr1,
         dcmipp_csi_err1,
         dcmipp_csi_err2,
         dcmipp_p0_sr);
  return;
#endif

  printf("DCMIPP wait frame complete before dump freeze: timeout=%lu ms\r\n",
         (uint32_t)DCMIPP_VERIFY_FRAME_DONE_TIMEOUT_MS);
  if (DCMIPP_WaitForFrameComplete(DCMIPP_VERIFY_FRAME_DONE_TIMEOUT_MS) == 0U)
  {
    dcmipp_status = 4U;
    printf("DCMIPP frame complete wait TIMEOUT: frames=%lu p0dcc=%lu/%lu tick=%lu\r\n",
           dcmipp_frame_count,
           dcmipp_p0_dccntr,
           (uint32_t)DCMIPP_VERIFY_FRAME_BYTES,
           HAL_GetTick());
    DCMIPP_LogPipeRegisters("frame_timeout_keep_streaming");
#if (DCMIPP_VERIFY_PHY_SCAN != 0U)
    (void)DCMIPP_RunPhyBitrateScan();
#endif
    return;
  }
  DCMIPP_LogPipeRegisters("frame_done_before_freeze");
  DCMIPP_FreezeCaptureForDump();
  DCMIPP_LogPipeRegisters("frozen_for_dump");
  imx219_status = 7U;
  printf("CSI IMX219 bring-up PASS\r\n");


}

void CameraDebug_Task(void)
{
  static uint32_t heartbeat = 0U;

#if (CAMERA_DEBUG_I2C_ONLY != 0U)
  uint32_t sensor_id = 0U;
  uint16_t mode = 0xFFFFU;
  uint16_t lane = 0xFFFFU;
  uint16_t fmt = 0xFFFFU;
  uint16_t line = 0xFFFFU;
  uint16_t frame = 0xFFFFU;
  uint32_t id_ok = 0U;
  uint32_t regs_ok = 0U;

  if ((IMX219_ReadID(&hcamera, &sensor_id) == IMX219_OK) && (sensor_id == IMX219_CHIP_ID))
  {
    id_ok = 1U;
  }

  if ((IMX219_ReadRegister16(&hcamera, 0x0100U, &mode) == IMX219_OK) &&
      (IMX219_ReadRegister16(&hcamera, 0x0114U, &lane) == IMX219_OK) &&
      (IMX219_ReadRegister16(&hcamera, 0x018CU, &fmt) == IMX219_OK) &&
      (IMX219_ReadRegister16(&hcamera, 0x0162U, &line) == IMX219_OK) &&
      (IMX219_ReadRegister16(&hcamera, 0x0160U, &frame) == IMX219_OK))
  {
    regs_ok = 1U;
  }

  printf("sensor heartbeat=%lu imx219=%lu id_ok=%lu id=0x%04lX regs_ok=%lu mode=0x%04lX lane=0x%04lX fmt=0x%04lX line=0x%04lX frame=0x%04lX stream=%s dcmipp=off\r\n",
         heartbeat++, imx219_status, id_ok, sensor_id, regs_ok,
         (uint32_t)mode, (uint32_t)lane, (uint32_t)fmt,
         (uint32_t)line, (uint32_t)frame,
         (imx219_streaming != 0U) ? "on" : "off");
  HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
  HAL_Delay(1000);
  return;
#endif

#if (CAMERA_DEBUG_CSI_MONITOR_ONLY != 0U)
  uint32_t sensor_id = 0U;
  uint16_t mode = 0xFFFFU;
  uint32_t id_ok = 0U;

  if ((IMX219_ReadID(&hcamera, &sensor_id) == IMX219_OK) && (sensor_id == IMX219_CHIP_ID))
  {
    id_ok = 1U;
  }
  (void)IMX219_ReadRegister16(&hcamera, 0x0100U, &mode);
  DCMIPP_SampleCsiFlags();

  printf("csi heartbeat=%lu imx219=%lu id_ok=%lu id=0x%04lX mode=0x%04lX stream=%s dcmipp=%lu sof=%lu eof=%lu spkt=%lu lb0=%lu lb_last=%lu dphy_samples=%lu err=0x%08lX sr0=0x%08lX sr1=0x%08lX csi_err1=0x%08lX csi_err2=0x%08lX spdfr=0x%08lX prcr=0x%08lX pmcr=0x%08lX lmcfgr=0x%08lX vc0cfgr1=0x%08lX vc0cfgr2=0x%08lX lb0cfgr=0x%08lX\r\n",
         heartbeat++, imx219_status, id_ok, sensor_id, (uint32_t)mode,
         (imx219_streaming != 0U) ? "on" : "off",
         dcmipp_status,
         dcmipp_sof_count, dcmipp_eof_count,
         dcmipp_short_packet_count, dcmipp_csi_lb0_count,
         dcmipp_csi_lb_last_counter, dcmipp_dphy_sample_count,
         dcmipp_last_error, dcmipp_csi_sr0, dcmipp_csi_sr1,
         dcmipp_csi_err1, dcmipp_csi_err2,
         dcmipp_csi_spdfr, dcmipp_csi_prcr, dcmipp_csi_pmcr,
         dcmipp_csi_lmcfgr, dcmipp_csi_vc0cfgr1,
         dcmipp_csi_vc0cfgr2, dcmipp_csi_lb0cfgr);
  HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
  HAL_Delay(1000);
  return;
#endif

uint32_t hw_frame_count = 0U;
uint32_t dump_count = 0U;
(void)HAL_DCMIPP_PIPE_ReadFrameCounter(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE, &hw_frame_count);
if (DCMIPP_VERIFY_CAPTURE_PIPE == DCMIPP_PIPE0)
{
  (void)HAL_DCMIPP_PIPE_GetDataCounter(&hdcmipp, DCMIPP_PIPE0, &dump_count);
}
DCMIPP_SampleCsiFlags();
DCMIPP_UpdateBufferStats();
#if (DCMIPP_VERIFY_DUMP_RAW16_LAYOUT != 0U)
if ((dcmipp_raw16_layout_dumped == 0U) && (dcmipp_p0_dccntr >= DCMIPP_VERIFY_FRAME_BYTES))
{
  DCMIPP_DumpRaw16Layout();
  dcmipp_raw16_layout_dumped = 1U;
}
#endif
#if (DCMIPP_VERIFY_DUMP_RGB565_LAYOUT != 0U)
if ((dcmipp_rgb565_layout_dumped == 0U) && (dcmipp_frame_count > 0U))
{
  DCMIPP_DumpRGB565Layout();
  dcmipp_rgb565_layout_dumped = 1U;
}
#endif
printf("heartbeat=%lu imx219=%lu stream=%s frozen=%lu done_tick=%lu freeze_tick=%lu dcmipp=%lu pipe=%lu frames=%lu hw=%lu dump=%lu/%lu buf_changed=%lu rgb565=%lu r5=0x%02lX..0x%02lX g6=0x%02lX..0x%02lX b5=0x%02lX..0x%02lX s=%04lX,%04lX,%04lX,%04lX vsync=%lu sof=%lu eof=%lu err=0x%08lX p1_ovr=%lu post_frame_ovr=%lu csi_err1=0x%08lX csi_err2=0x%08lX p0sr=0x%08lX p1sr=0x%08lX sr0=0x%08lX sr1=0x%08lX\r\n",
       heartbeat++, imx219_status,
       (imx219_streaming != 0U) ? "on" : "off",
       dcmipp_capture_frozen, dcmipp_frame_done_tick, dcmipp_freeze_tick,
       dcmipp_status, (uint32_t)DCMIPP_VERIFY_CAPTURE_PIPE,
       dcmipp_frame_count, hw_frame_count,
       (DCMIPP_VERIFY_CAPTURE_PIPE == DCMIPP_PIPE0) ? dcmipp_p0_dccntr : dump_count,
       (uint32_t)DCMIPP_VERIFY_FRAME_BYTES,
       dcmipp_buf_changed, dcmipp_rgb565_valid,
       dcmipp_rgb565_min_r5, dcmipp_rgb565_max_r5,
       dcmipp_rgb565_min_g6, dcmipp_rgb565_max_g6,
       dcmipp_rgb565_min_b5, dcmipp_rgb565_max_b5,
       dcmipp_rgb565_s0, dcmipp_rgb565_s1, dcmipp_rgb565_s2, dcmipp_rgb565_s3,
       dcmipp_vsync_count, dcmipp_sof_count, dcmipp_eof_count,
       dcmipp_last_error,
       (uint32_t)(((dcmipp_p1_sr & DCMIPP_P1SR_OVRF) != 0U) ? 1U : 0U),
       (uint32_t)(((dcmipp_frame_count > 0U) &&
                   ((dcmipp_last_error & HAL_DCMIPP_ERROR_PIPE1_OVR) != 0U)) ? 1U : 0U),
       dcmipp_csi_err1, dcmipp_csi_err2,
       dcmipp_p0_sr, dcmipp_p1_sr, dcmipp_csi_sr0, dcmipp_csi_sr1);
#if 0
printf("heartbeat=%lu imx219_status=%lu imx219_id=0x%04lX stream=%s dcmipp_status=%lu frames=%lu hw_frames=%lu p0dump_bytes=%lu p0dcc_bytes=%lu expect_bytes=%lu buf_sum=0x%08lX buf_nz=%lu buf_changed=%lu buf_w0=0x%08lX buf_w1=0x%08lX buf_w2=0x%08lX buf_w3=0x%08lX buf_w4=0x%08lX buf_w5=0x%08lX buf_w6=0x%08lX buf_w7=0x%08lX raw16_valid=%lu raw16_align=%lu raw16_min=0x%04lX raw16_max=0x%04lX raw16_low6_nz=%lu raw16_high6_nz=%lu raw16_s0=0x%04lX raw16_s1=0x%04lX raw16_s2=0x%04lX raw16_s3=0x%04lX raw16_s4=0x%04lX raw16_s5=0x%04lX raw16_s6=0x%04lX raw16_s7=0x%04lX vsync=%lu sof=%lu eof=%lu perr=%lu lerr=%lu cerr=%lu spkt=%lu lb0=%lu lb_last=%lu dphy_samples=%lu err=0x%08lX sr0=0x%08lX sr1=0x%08lX cr=0x%08lX ier0=0x%08lX ier1=0x%08lX pfcr=0x%08lX pcr=0x%08lX lmcfgr=0x%08lX vc0cfgr1=0x%08lX vc0cfgr2=0x%08lX vc0cfgr3=0x%08lX vc0cfgr4=0x%08lX lb0cfgr=0x%08lX prgitr=0x%08lX csi_err1=0x%08lX csi_err2=0x%08lX spdfr=0x%08lX csi_prcr=0x%08lX csi_pmcr=0x%08lX cmcr=0x%08lX cmsr1=0x%08lX p0sr=0x%08lX p0ier=0x%08lX p0dclmtr=0x%08lX p0fscr=0x%08lX p0fctcr=0x%08lX p0ppcr=0x%08lX p0m0ar1=0x%08lX p0stm0ar=0x%08lX p0cfscr=0x%08lX p0cfctcr=0x%08lX p0cppcr=0x%08lX p0cm0ar1=0x%08lX p0scstr=0x%08lX p0scszr=0x%08lX p1sr=0x%08lX p1ier=0x%08lX p1fscr=0x%08lX p1fctcr=0x%08lX p1dmcr=0x%08lX p1dscr=0x%08lX p1dsrtior=0x%08lX p1dsszr=0x%08lX p1ppcr=0x%08lX p1m0ar1=0x%08lX p1m0pr=0x%08lX p1stm0ar=0x%08lX p1cfscr=0x%08lX p1cfctcr=0x%08lX p1cppcr=0x%08lX p1cm0ar1=0x%08lX p1cm0pr=0x%08lX cmsr2=0x%08lX cmier=0x%08lX ipgr1=0x%08lX ipc1r1=0x%08lX ipc1r2=0x%08lX ipc1r3=0x%08lX ipc2r1=0x%08lX ipc2r2=0x%08lX ipc2r3=0x%08lX ipc3r1=0x%08lX ipc3r2=0x%08lX ipc3r3=0x%08lX ipc4r1=0x%08lX ipc4r2=0x%08lX ipc4r3=0x%08lX ipc5r1=0x%08lX ipc5r2=0x%08lX ipc5r3=0x%08lX\r\n",
       heartbeat++, imx219_status, imx219_id,
       (IMX219_DEBUG_START_STREAM != 0U) ? "on" : "standby",
       dcmipp_status, dcmipp_frame_count, hw_frame_count, dump_count, dcmipp_p0_dccntr,
       (uint32_t)DCMIPP_VERIFY_FRAME_BYTES,
       dcmipp_buf_checksum, dcmipp_buf_nonzero, dcmipp_buf_changed,
       dcmipp_buf_w0, dcmipp_buf_w1, dcmipp_buf_w2, dcmipp_buf_w3,
       dcmipp_buf_w4, dcmipp_buf_w5, dcmipp_buf_w6, dcmipp_buf_w7,
       dcmipp_raw16_valid, dcmipp_raw16_align, dcmipp_raw16_min,
       dcmipp_raw16_max, dcmipp_raw16_low6_nonzero,
       dcmipp_raw16_high6_nonzero,
       dcmipp_raw16_s0, dcmipp_raw16_s1, dcmipp_raw16_s2, dcmipp_raw16_s3,
       dcmipp_raw16_s4, dcmipp_raw16_s5, dcmipp_raw16_s6, dcmipp_raw16_s7,
       dcmipp_vsync_count, dcmipp_sof_count, dcmipp_eof_count,
       dcmipp_pipe_error_count, dcmipp_line_error_count,
       dcmipp_common_error_count, dcmipp_short_packet_count,
       dcmipp_csi_lb0_count, dcmipp_csi_lb_last_counter,
       dcmipp_dphy_sample_count, dcmipp_last_error,
       dcmipp_csi_sr0, dcmipp_csi_sr1, dcmipp_csi_cr,
       dcmipp_csi_ier0, dcmipp_csi_ier1, dcmipp_csi_pfcr,
       dcmipp_csi_pcr, dcmipp_csi_lmcfgr,
       dcmipp_csi_vc0cfgr1, dcmipp_csi_vc0cfgr2,
       dcmipp_csi_vc0cfgr3, dcmipp_csi_vc0cfgr4,
       dcmipp_csi_lb0cfgr, dcmipp_csi_prgitr,
       dcmipp_csi_err1, dcmipp_csi_err2, dcmipp_csi_spdfr,
       dcmipp_csi_prcr, dcmipp_csi_pmcr,
       dcmipp_cmcr, dcmipp_cmsr1, dcmipp_p0_sr, dcmipp_p0_ier,
       dcmipp_p0_dclmtr, dcmipp_p0_fscr, dcmipp_p0_fctcr, dcmipp_p0_ppcr,
       dcmipp_p0_ppm0ar1, dcmipp_p0_stm0ar, dcmipp_p0_cfscr,
       dcmipp_p0_cfctcr, dcmipp_p0_cppcr, dcmipp_p0_cppm0ar1,
       dcmipp_p0_scstr, dcmipp_p0_scszr,
       dcmipp_p1_sr, dcmipp_p1_ier,
       dcmipp_p1_fscr, dcmipp_p1_fctcr, dcmipp_p1_dmcr,
       dcmipp_p1_dscr, dcmipp_p1_dsrtior, dcmipp_p1_dsszr,
       dcmipp_p1_ppcr,
       dcmipp_p1_ppm0ar1, dcmipp_p1_ppm0pr, dcmipp_p1_stm0ar,
       dcmipp_p1_cfscr, dcmipp_p1_cfctcr, dcmipp_p1_cppcr,
       dcmipp_p1_cppm0ar1, dcmipp_p1_cppm0pr,
       dcmipp_cmsr2, dcmipp_cmier,
       dcmipp_ipgr1, dcmipp_ipc1r1, dcmipp_ipc1r2, dcmipp_ipc1r3,
       dcmipp_ipc2r1, dcmipp_ipc2r2, dcmipp_ipc2r3,
       dcmipp_ipc3r1, dcmipp_ipc3r2, dcmipp_ipc3r3,
       dcmipp_ipc4r1, dcmipp_ipc4r2, dcmipp_ipc4r3,
       dcmipp_ipc5r1, dcmipp_ipc5r2, dcmipp_ipc5r3);
#endif
HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
HAL_Delay(1000);
}

void CameraDebug_GetStatus(CameraDebug_Status_t *status)
{
  if (status == NULL)
  {
    return;
  }

  status->sensor_id = imx219_id;
  status->sensor_status = imx219_status;
  status->dcmipp_status = dcmipp_status;
  status->streaming = imx219_streaming;
  status->frozen = dcmipp_capture_frozen;
  status->frame_count = dcmipp_frame_count;
  status->frame_done_tick = dcmipp_frame_done_tick;
  status->p0_dump_bytes = dcmipp_p0_dccntr;
  status->expected_bytes = DCMIPP_VERIFY_FRAME_BYTES;
  status->raw16_valid = dcmipp_raw16_valid;
  status->raw16_align = dcmipp_raw16_align;
  status->raw16_min = dcmipp_raw16_min;
  status->raw16_max = dcmipp_raw16_max;
  status->raw16_sat10_count = dcmipp_raw16_sat10_count;
  status->last_error = dcmipp_last_error;
}

uint8_t *CameraDebug_GetFrameBuffer(void)
{
  return dcmipp_frame_buffer;
}

uint32_t CameraDebug_GetFrameBufferSize(void)
{
  return (uint32_t)sizeof(dcmipp_frame_buffer);
}

void CameraDebug_PrintErrorContext(void)
{
  printf("ERROR: IMX219 status=%lu id=0x%04lX DCMIPP status=%lu err=0x%08lX\r\n",
         imx219_status, imx219_id, dcmipp_status, dcmipp_last_error);
}

/**
 * @brief DCMIPP Initialization Function
 * @param None
 * @retval None
 */
#if (CAMERA_DEBUG_I2C_ONLY == 0U)
static void MX_DCMIPP_Init(void)
{
  DCMIPP_CSI_PIPE_ConfTypeDef csi_pipe_conf = {0};
  DCMIPP_CSI_LineByteCounterConfTypeDef line_byte_conf = {0};
  DCMIPP_PipeConfTypeDef pipe_conf = {0};
  DCMIPP_RawBayer2RGBConfTypeDef raw_bayer_conf = {0};
  DCMIPP_BlackLevelConfTypeDef black_level_conf = {0};
  DCMIPP_ExposureConfTypeDef exposure_conf = {0};
#if (DCMIPP_VERIFY_ENABLE_DOWNSIZE != 0U)
  DCMIPP_DownsizeTypeDef downsize_conf = {0};
#endif
#if ((DCMIPP_VERIFY_CAPTURE_PIPE == DCMIPP_PIPE0) && (DCMIPP_VERIFY_PIPE0_CROP != 0U))
  DCMIPP_CropConfTypeDef crop_conf = {0};
#endif

  dcmipp_status = 1U;

  hdcmipp.Instance = DCMIPP;
  if (HAL_DCMIPP_Init(&hdcmipp) != HAL_OK)
  {
    Error_Handler();
  }

#if (DCMIPP_VERIFY_CONFIG_IPPLUG != 0U)
  DCMIPP_ConfigIPPlug();
#endif

  DCMIPP_SetCsiPhyBitrate(DCMIPP_VERIFY_PHY_BITRATE);

#if (DCMIPP_VERIFY_MASK_DPHY_IRQ != 0U)
  __HAL_DCMIPP_CSI_DPHY_DISABLE_IT(CSI, DCMIPP_CSI_DPHY_DATA_LANE_ERROR_IT);
#endif

  if (HAL_DCMIPP_CSI_SetVCConfig(&hdcmipp, DCMIPP_VIRTUAL_CHANNEL0, DCMIPP_CSI_DT_BPP10) != HAL_OK)
  {
    Error_Handler();
  }

  line_byte_conf.VirtualChannel = DCMIPP_VIRTUAL_CHANNEL0;
  line_byte_conf.LineCounter = 1U;
  line_byte_conf.ByteCounter = DCMIPP_VERIFY_RAW10_LINE_BYTES;
  if (HAL_DCMIPP_CSI_SetLineByteCounterConfig(&hdcmipp, DCMIPP_CSI_COUNTER0, &line_byte_conf) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DCMIPP_CSI_EnableLineByteCounter(&hdcmipp, DCMIPP_CSI_COUNTER0) != HAL_OK)
  {
    Error_Handler();
  }

  csi_pipe_conf.DataTypeMode = DCMIPP_VERIFY_DATA_TYPE_MODE;
  csi_pipe_conf.DataTypeIDA = DCMIPP_DT_RAW10;
  csi_pipe_conf.DataTypeIDB = DCMIPP_DT_RAW10;
  if (HAL_DCMIPP_CSI_PIPE_SetConfig(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE, &csi_pipe_conf) != HAL_OK)
  {
    Error_Handler();
  }

  pipe_conf.FrameRate = DCMIPP_FRAME_RATE_ALL;
  pipe_conf.PixelPipePitch = DCMIPP_VERIFY_LINE_PITCH;
  pipe_conf.PixelPackerFormat = DCMIPP_VERIFY_PIXEL_PACKER;
  if (HAL_DCMIPP_PIPE_SetConfig(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE, &pipe_conf) != HAL_OK)
  {
    Error_Handler();
  }

#if ((DCMIPP_VERIFY_CAPTURE_PIPE == DCMIPP_PIPE0) && (DCMIPP_VERIFY_PIPE0_DECIMATE != 0U))
  if (HAL_DCMIPP_PIPE_SetBytesDecimationConfig(&hdcmipp,
                                               DCMIPP_VERIFY_CAPTURE_PIPE,
                                               DCMIPP_VERIFY_PIPE0_BYTE_START,
                                               DCMIPP_VERIFY_PIPE0_BYTE_SELECT) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DCMIPP_PIPE_SetLinesDecimationConfig(&hdcmipp,
                                               DCMIPP_VERIFY_CAPTURE_PIPE,
                                               DCMIPP_VERIFY_PIPE0_LINE_START,
                                               DCMIPP_VERIFY_PIPE0_LINE_SELECT) != HAL_OK)
  {
    Error_Handler();
  }
#endif

#if ((DCMIPP_VERIFY_CAPTURE_PIPE == DCMIPP_PIPE0) && (DCMIPP_VERIFY_PIPE0_CROP != 0U))
  crop_conf.HStart = DCMIPP_VERIFY_PIPE0_CROP_HSTART;
  crop_conf.VStart = DCMIPP_VERIFY_PIPE0_CROP_VSTART;
  crop_conf.HSize = DCMIPP_VERIFY_PIPE0_CROP_HSIZE;
  crop_conf.VSize = DCMIPP_VERIFY_PIPE0_CROP_VSIZE;
  crop_conf.PipeArea = DCMIPP_POSITIVE_AREA;
  if (HAL_DCMIPP_PIPE_SetCropConfig(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE, &crop_conf) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DCMIPP_PIPE_EnableCrop(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE) != HAL_OK)
  {
    Error_Handler();
  }
#endif

  if (DCMIPP_VERIFY_CAPTURE_PIPE == DCMIPP_PIPE1)
  {
    black_level_conf.RedCompBlackLevel = DCMIPP_VERIFY_PIPE1_BLACK_LEVEL;
    black_level_conf.GreenCompBlackLevel = DCMIPP_VERIFY_PIPE1_BLACK_LEVEL;
    black_level_conf.BlueCompBlackLevel = DCMIPP_VERIFY_PIPE1_BLACK_LEVEL;
    if (HAL_DCMIPP_PIPE_SetISPBlackLevelCalibrationConfig(&hdcmipp,
                                                          DCMIPP_VERIFY_CAPTURE_PIPE,
                                                          &black_level_conf) != HAL_OK)
    {
      Error_Handler();
    }
    if (HAL_DCMIPP_PIPE_EnableISPBlackLevelCalibration(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE) != HAL_OK)
    {
      Error_Handler();
    }

    exposure_conf.ShiftRed = DCMIPP_VERIFY_PIPE1_EXPOSURE_SHIFT;
    exposure_conf.MultiplierRed = DCMIPP_VERIFY_PIPE1_WB_R_MULT;
    exposure_conf.ShiftGreen = DCMIPP_VERIFY_PIPE1_EXPOSURE_SHIFT;
    exposure_conf.MultiplierGreen = DCMIPP_VERIFY_PIPE1_WB_G_MULT;
    exposure_conf.ShiftBlue = DCMIPP_VERIFY_PIPE1_EXPOSURE_SHIFT;
    exposure_conf.MultiplierBlue = DCMIPP_VERIFY_PIPE1_WB_B_MULT;
    if (HAL_DCMIPP_PIPE_SetISPExposureConfig(&hdcmipp,
                                             DCMIPP_VERIFY_CAPTURE_PIPE,
                                             &exposure_conf) != HAL_OK)
    {
      Error_Handler();
    }
    if (HAL_DCMIPP_PIPE_EnableISPExposure(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE) != HAL_OK)
    {
      Error_Handler();
    }

    raw_bayer_conf.RawBayerType = DCMIPP_VERIFY_PIPE1_BAYER;
    raw_bayer_conf.PeakStrength = DCMIPP_VERIFY_PIPE1_BAYER_STRENGTH;
    raw_bayer_conf.EdgeStrength = DCMIPP_VERIFY_PIPE1_BAYER_STRENGTH;
    raw_bayer_conf.VLineStrength = DCMIPP_VERIFY_PIPE1_BAYER_STRENGTH;
    raw_bayer_conf.HLineStrength = DCMIPP_VERIFY_PIPE1_BAYER_STRENGTH;
    if (HAL_DCMIPP_PIPE_SetISPRawBayer2RGBConfig(&hdcmipp,
                                                 DCMIPP_VERIFY_CAPTURE_PIPE,
                                                 &raw_bayer_conf) != HAL_OK)
    {
      Error_Handler();
    }
    if (HAL_DCMIPP_PIPE_EnableISPRawBayer2RGB(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE) != HAL_OK)
    {
      Error_Handler();
    }
    if (HAL_DCMIPP_PIPE_EnableGammaConversion(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE) != HAL_OK)
    {
      Error_Handler();
    }

#if (DCMIPP_VERIFY_ENABLE_DOWNSIZE != 0U)
    downsize_conf.HSize = DCMIPP_VERIFY_OUTPUT_WIDTH;
    downsize_conf.VSize = DCMIPP_VERIFY_OUTPUT_HEIGHT;
    downsize_conf.HRatio = DCMIPP_DOWNSIZE_RATIO(DCMIPP_VERIFY_WIDTH, DCMIPP_VERIFY_OUTPUT_WIDTH);
    downsize_conf.VRatio = DCMIPP_DOWNSIZE_RATIO(DCMIPP_VERIFY_HEIGHT, DCMIPP_VERIFY_OUTPUT_HEIGHT);
    downsize_conf.HDivFactor = DCMIPP_DOWNSIZE_DIV_FACTOR(DCMIPP_VERIFY_WIDTH, DCMIPP_VERIFY_OUTPUT_WIDTH);
    downsize_conf.VDivFactor = DCMIPP_DOWNSIZE_DIV_FACTOR(DCMIPP_VERIFY_HEIGHT, DCMIPP_VERIFY_OUTPUT_HEIGHT);
    if (HAL_DCMIPP_PIPE_SetDownsizeConfig(&hdcmipp,
                                          DCMIPP_VERIFY_CAPTURE_PIPE,
                                          &downsize_conf) != HAL_OK)
    {
      Error_Handler();
    }
    if (HAL_DCMIPP_PIPE_EnableDownsize(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE) != HAL_OK)
    {
      Error_Handler();
    }
#endif
  }

#if ((DCMIPP_VERIFY_CAPTURE_PIPE == DCMIPP_PIPE0) && (DCMIPP_VERIFY_PIPE0_LIMIT_WORDS != 0U))
  if (HAL_DCMIPP_PIPE_EnableLimitEvent(&hdcmipp,
                                       DCMIPP_VERIFY_CAPTURE_PIPE,
                                       DCMIPP_VERIFY_PIPE0_LIMIT_WORDS) != HAL_OK)
  {
    Error_Handler();
  }
#endif

#if ((DCMIPP_VERIFY_DUMP_HEADER != 0U) && (DCMIPP_VERIFY_CAPTURE_PIPE == DCMIPP_PIPE0))
  if (HAL_DCMIPP_PIPE_CSI_EnableHeader(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE) != HAL_OK)
  {
    Error_Handler();
  }
#elif (DCMIPP_VERIFY_CAPTURE_PIPE == DCMIPP_PIPE0)
  if (HAL_DCMIPP_PIPE_CSI_DisableHeader(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE) != HAL_OK)
  {
    Error_Handler();
  }
#endif

  if (HAL_DCMIPP_PIPE_SetFrameCounterConfig(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_DCMIPP_PIPE_ResetFrameCounter(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE) != HAL_OK)
  {
    Error_Handler();
  }
}
#endif

static void Camera_LogStatus(const char *Step)
{
  printf("[IMX219:%lu] %s\r\n", imx219_status, Step);
}

static void Camera_DumpRegisters(const char *Tag)
{
  uint16_t mode = 0U;
  uint16_t lane = 0U;
  uint16_t fmt = 0U;
  uint16_t vt_pll = 0U;
  uint16_t op_pll = 0U;
  uint16_t line = 0U;
  uint16_t frame = 0U;
  uint16_t test = 0U;
  uint8_t analog_gain = 0U;
  uint16_t exposure = 0U;
  uint16_t digital_gain = 0U;

  if ((IMX219_ReadRegister16(&hcamera, 0x0100U, &mode) != IMX219_OK) ||
      (IMX219_ReadRegister16(&hcamera, 0x0114U, &lane) != IMX219_OK) ||
      (IMX219_ReadRegister16(&hcamera, 0x018CU, &fmt) != IMX219_OK) ||
      (IMX219_ReadRegister16(&hcamera, 0x0306U, &vt_pll) != IMX219_OK) ||
      (IMX219_ReadRegister16(&hcamera, 0x030CU, &op_pll) != IMX219_OK) ||
      (IMX219_ReadRegister16(&hcamera, 0x0162U, &line) != IMX219_OK) ||
      (IMX219_ReadRegister16(&hcamera, 0x0160U, &frame) != IMX219_OK) ||
      (IMX219_ReadRegister8(&hcamera, 0x0157U, &analog_gain) != IMX219_OK) ||
      (IMX219_ReadRegister16(&hcamera, 0x015AU, &exposure) != IMX219_OK) ||
      (IMX219_ReadRegister16(&hcamera, 0x0158U, &digital_gain) != IMX219_OK) ||
      (IMX219_ReadRegister16(&hcamera, 0x0600U, &test) != IMX219_OK))
  {
    printf("IMX219 regs %s: readback failed\r\n", Tag);
    return;
  }

  printf("IMX219 regs %s: mode=0x%04lX lane=0x%04lX fmt=0x%04lX vt_pll=0x%04lX op_pll=0x%04lX line=0x%04lX frame=0x%04lX again=0x%02lX exposure=0x%04lX dgain=0x%04lX test=0x%04lX\r\n",
         Tag, (uint32_t)mode, (uint32_t)lane, (uint32_t)fmt,
         (uint32_t)vt_pll, (uint32_t)op_pll, (uint32_t)line,
         (uint32_t)frame, (uint32_t)analog_gain, (uint32_t)exposure,
         (uint32_t)digital_gain, (uint32_t)test);
}

#if (DCMIPP_VERIFY_CONFIG_IPPLUG != 0U)
static void DCMIPP_ConfigIPPlug(void)
{
  DCMIPP_IPPlugConfTypeDef ipplug_conf = {0};

  ipplug_conf.Client = DCMIPP_CLIENT1;
  ipplug_conf.MemoryPageSize = DCMIPP_VERIFY_IPPLUG_PAGE_SIZE;
  ipplug_conf.Traffic = DCMIPP_TRAFFIC_BURST_SIZE_128BYTES;
  ipplug_conf.MaxOutstandingTransactions = DCMIPP_OUTSTANDING_TRANSACTION_16;
  ipplug_conf.DPREGStart = 0U;
  ipplug_conf.DPREGEnd = 0x3FFU;
  ipplug_conf.WLRURatio = 0xFU;

  if (HAL_DCMIPP_SetIPPlugConfig(&hdcmipp, &ipplug_conf) != HAL_OK)
  {
    dcmipp_last_error = HAL_DCMIPP_GetError(&hdcmipp);
    Error_Handler();
  }
}
#endif

#if (CAMERA_DEBUG_I2C_ONLY == 0U)
static void DCMIPP_SetCsiPhyBitrate(uint32_t PhyBitrate)
{
  DCMIPP_SetCsiConfig(PhyBitrate, DCMIPP_VERIFY_LANE_MAPPING);
}

static void DCMIPP_SetCsiConfig(uint32_t PhyBitrate, uint32_t LaneMapping)
{
  DCMIPP_CSI_ConfTypeDef csiconf = {0};

  csiconf.DataLaneMapping = LaneMapping;
  csiconf.NumberOfLanes = DCMIPP_VERIFY_NUMBER_OF_LANES;
  csiconf.PHYBitrate = PhyBitrate;
  if (HAL_DCMIPP_CSI_SetConfig(&hdcmipp, &csiconf) != HAL_OK)
  {
    dcmipp_last_error = HAL_DCMIPP_GetError(&hdcmipp);
    Error_Handler();
  }
}
#endif

static void DCMIPP_StartCapture(void)
{
  if (HAL_DCMIPP_CSI_PIPE_Start(&hdcmipp,
                                DCMIPP_VERIFY_CAPTURE_PIPE,
                                DCMIPP_VIRTUAL_CHANNEL0,
                                (uint32_t)(uintptr_t)dcmipp_frame_buffer,
                                DCMIPP_VERIFY_CAPTURE_MODE) != HAL_OK)
  {
    dcmipp_last_error = HAL_DCMIPP_GetError(&hdcmipp);
    Error_Handler();
  }
}

#if (DCMIPP_VERIFY_PHY_SCAN != 0U)
static uint32_t DCMIPP_StopCaptureAndStream(void)
{
  uint32_t ok = 1U;

  if (imx219_streaming != 0U)
  {
    if (IMX219_Stop(&hcamera) != IMX219_OK)
    {
      printf("IMX219 stop during scan failed\r\n");
      ok = 0U;
    }
    imx219_streaming = 0U;
    HAL_Delay(50U);
  }

  if (HAL_DCMIPP_CSI_PIPE_Stop(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE, DCMIPP_VIRTUAL_CHANNEL0) != HAL_OK)
  {
    dcmipp_last_error = HAL_DCMIPP_GetError(&hdcmipp);
    printf("DCMIPP scan stop pipe returned error err=0x%08lX\r\n", dcmipp_last_error);
    ok = 0U;
  }
  if (DCMIPP_WaitForPipeIdle(DCMIPP_VERIFY_PIPE_IDLE_TIMEOUT_MS) == 0U)
  {
    printf("DCMIPP scan pipe idle wait timeout\r\n");
    ok = 0U;
  }

  return ok;
}

static void DCMIPP_ResetRuntimeCounters(void)
{
  dcmipp_frame_count = 0U;
  dcmipp_vsync_count = 0U;
  dcmipp_sof_count = 0U;
  dcmipp_eof_count = 0U;
  dcmipp_pipe_error_count = 0U;
  dcmipp_line_error_count = 0U;
  dcmipp_common_error_count = 0U;
  dcmipp_short_packet_count = 0U;
  dcmipp_csi_lb0_count = 0U;
  dcmipp_csi_lb_last_counter = 0U;
  dcmipp_dphy_sample_count = 0U;
  dcmipp_frame_done_tick = 0U;
  dcmipp_last_error = 0U;

  if (HAL_DCMIPP_PIPE_ResetFrameCounter(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE) != HAL_OK)
  {
    dcmipp_last_error = HAL_DCMIPP_GetError(&hdcmipp);
    printf("DCMIPP reset frame counter failed err=0x%08lX\r\n", dcmipp_last_error);
  }
}

static uint32_t DCMIPP_TrySetCsiConfig(uint32_t PhyBitrate, uint32_t LaneMapping)
{
  DCMIPP_CSI_ConfTypeDef csiconf = {0};

  csiconf.DataLaneMapping = LaneMapping;
  csiconf.NumberOfLanes = DCMIPP_VERIFY_NUMBER_OF_LANES;
  csiconf.PHYBitrate = PhyBitrate;
  if (HAL_DCMIPP_CSI_SetConfig(&hdcmipp, &csiconf) != HAL_OK)
  {
    dcmipp_last_error = HAL_DCMIPP_GetError(&hdcmipp);
    printf("DCMIPP scan set CSI config failed err=0x%08lX\r\n", dcmipp_last_error);
    return 0U;
  }

  return 1U;
}

static uint32_t DCMIPP_TryStartCapture(void)
{
  if (HAL_DCMIPP_CSI_PIPE_Start(&hdcmipp,
                                DCMIPP_VERIFY_CAPTURE_PIPE,
                                DCMIPP_VIRTUAL_CHANNEL0,
                                (uint32_t)(uintptr_t)dcmipp_frame_buffer,
                                DCMIPP_VERIFY_CAPTURE_MODE) != HAL_OK)
  {
    dcmipp_last_error = HAL_DCMIPP_GetError(&hdcmipp);
    printf("DCMIPP scan start pipe failed err=0x%08lX\r\n", dcmipp_last_error);
    return 0U;
  }

  return 1U;
}

static void DCMIPP_RestoreDefaultAfterScan(void)
{
  printf("DCMIPP PHY/lane scan restore default config\r\n");

  /*
   * The scan can arrive here because PIPE_Stop already failed. Do not call
   * PIPE_Stop again from the recovery path; on this target that can stall the
   * debug loop before heartbeat resumes.
   */
  (void)DCMIPP_TrySetCsiConfig(DCMIPP_VERIFY_PHY_BITRATE, DCMIPP_VERIFY_LANE_MAPPING);
  DCMIPP_ClearCsiFlags();
  DCMIPP_ResetRuntimeCounters();
  DCMIPP_SampleCsiFlags();
  DCMIPP_UpdateBufferStats();
  printf("DCMIPP PHY/lane scan restore done: stream=%s frames=%lu p0dcc=%lu/%lu err=0x%08lX sr0=0x%08lX sr1=0x%08lX csi_err1=0x%08lX\r\n",
         (imx219_streaming != 0U) ? "on" : "off",
         dcmipp_frame_count,
         dcmipp_p0_dccntr,
         (uint32_t)DCMIPP_VERIFY_FRAME_BYTES,
         dcmipp_last_error,
         dcmipp_csi_sr0,
         dcmipp_csi_sr1,
         dcmipp_csi_err1);
}

static uint32_t DCMIPP_RunPhyBitrateScan(void)
{
  static const uint32_t lane_candidates[] =
  {
    DCMIPP_CSI_PHYSICAL_DATA_LANES,
    DCMIPP_CSI_INVERTED_DATA_LANES,
  };
  static const uint32_t phy_candidates[] =
  {
    DCMIPP_CSI_PHY_BT_80,
    DCMIPP_CSI_PHY_BT_90,
    DCMIPP_CSI_PHY_BT_100,
    DCMIPP_CSI_PHY_BT_110,
    DCMIPP_CSI_PHY_BT_120,
    DCMIPP_CSI_PHY_BT_130,
    DCMIPP_CSI_PHY_BT_140,
    DCMIPP_CSI_PHY_BT_150,
    DCMIPP_CSI_PHY_BT_160,
    DCMIPP_CSI_PHY_BT_170,
    DCMIPP_CSI_PHY_BT_180,
    DCMIPP_CSI_PHY_BT_190,
    DCMIPP_CSI_PHY_BT_205,
    DCMIPP_CSI_PHY_BT_220,
    DCMIPP_CSI_PHY_BT_235,
    DCMIPP_CSI_PHY_BT_250,
    DCMIPP_CSI_PHY_BT_275,
    DCMIPP_CSI_PHY_BT_300,
    DCMIPP_CSI_PHY_BT_325,
    DCMIPP_CSI_PHY_BT_350,
    DCMIPP_CSI_PHY_BT_400,
    DCMIPP_CSI_PHY_BT_450,
    DCMIPP_CSI_PHY_BT_500,
    DCMIPP_CSI_PHY_BT_550,
    DCMIPP_CSI_PHY_BT_600,
    DCMIPP_CSI_PHY_BT_650,
    DCMIPP_CSI_PHY_BT_700,
    DCMIPP_CSI_PHY_BT_750,
    DCMIPP_CSI_PHY_BT_800,
    DCMIPP_CSI_PHY_BT_850,
    DCMIPP_CSI_PHY_BT_900,
    DCMIPP_CSI_PHY_BT_950,
    DCMIPP_CSI_PHY_BT_1000,
  };

  printf("DCMIPP PHY/lane scan start: lane_candidates=%lu phy_candidates=%lu timeout_ms=%lu\r\n",
         (uint32_t)(sizeof(lane_candidates) / sizeof(lane_candidates[0])),
         (uint32_t)(sizeof(phy_candidates) / sizeof(phy_candidates[0])),
         (uint32_t)DCMIPP_VERIFY_PHY_SCAN_TIMEOUT_MS);

  for (uint32_t lane_i = 0U; lane_i < (uint32_t)(sizeof(lane_candidates) / sizeof(lane_candidates[0])); lane_i++)
  {
    uint32_t lane = lane_candidates[lane_i];

    for (uint32_t i = 0U; i < (uint32_t)(sizeof(phy_candidates) / sizeof(phy_candidates[0])); i++)
    {
      uint32_t phy = phy_candidates[i];
      uint32_t ok;

      printf("DCMIPP PHY/lane scan try: lane_map=%lu phy_bt=%lu\r\n", lane, phy);
      if (DCMIPP_StopCaptureAndStream() == 0U)
      {
        printf("DCMIPP PHY/lane scan abort: pipe did not stop cleanly\r\n");
        DCMIPP_RestoreDefaultAfterScan();
        return 0U;
      }
      if (DCMIPP_TrySetCsiConfig(phy, lane) == 0U)
      {
        DCMIPP_RestoreDefaultAfterScan();
        return 0U;
      }
      DCMIPP_ClearCsiFlags();
      DCMIPP_ResetRuntimeCounters();
      DCMIPP_FillBuffer((uint8_t)DCMIPP_VERIFY_BUFFER_FILL);
      if (DCMIPP_TryStartCapture() == 0U)
      {
        DCMIPP_RestoreDefaultAfterScan();
        return 0U;
      }

      if (IMX219_Start(&hcamera) != IMX219_OK)
      {
        printf("IMX219 start during PHY scan failed\r\n");
        DCMIPP_RestoreDefaultAfterScan();
        return 0U;
      }
      imx219_streaming = 1U;

      ok = DCMIPP_WaitForFrameComplete(DCMIPP_VERIFY_PHY_SCAN_TIMEOUT_MS);
      DCMIPP_SampleCsiFlags();
      DCMIPP_UpdateBufferStats();
      printf("DCMIPP PHY/lane scan result: lane_map=%lu phy_bt=%lu ok=%lu frames=%lu p0dcc=%lu/%lu buf_changed=%lu raw16=%lu sof=%lu eof=%lu err=0x%08lX csi_err1=0x%08lX csi_err2=0x%08lX sr0=0x%08lX sr1=0x%08lX p0sr=0x%08lX\r\n",
             lane, phy, ok, dcmipp_frame_count,
             dcmipp_p0_dccntr, (uint32_t)DCMIPP_VERIFY_FRAME_BYTES,
             dcmipp_buf_changed, dcmipp_raw16_valid,
             dcmipp_sof_count, dcmipp_eof_count,
             dcmipp_last_error, dcmipp_csi_err1, dcmipp_csi_err2,
             dcmipp_csi_sr0, dcmipp_csi_sr1, dcmipp_p0_sr);

      if (ok != 0U)
      {
        dcmipp_status = 5U;
        printf("DCMIPP PHY/lane scan PASS: lane_map=%lu phy_bt=%lu\r\n", lane, phy);
        return 1U;
      }
    }
  }

  dcmipp_status = 4U;
  printf("DCMIPP PHY/lane scan FAIL: no candidate produced a complete frame\r\n");
  DCMIPP_RestoreDefaultAfterScan();
  return 0U;
}
#endif

static uint32_t DCMIPP_WaitForFrameComplete(uint32_t TimeoutMs)
{
  uint32_t start = HAL_GetTick();

  do
  {
    DCMIPP_SampleCsiFlags();
    if ((dcmipp_frame_count > 0U) &&
        ((DCMIPP_VERIFY_CAPTURE_PIPE != DCMIPP_PIPE0) || (dcmipp_p0_dccntr >= DCMIPP_VERIFY_FRAME_BYTES)))
    {
      if (dcmipp_frame_done_tick == 0U)
      {
        dcmipp_frame_done_tick = HAL_GetTick();
      }
      return 1U;
    }
  } while ((HAL_GetTick() - start) < TimeoutMs);

  return 0U;
}

static uint32_t DCMIPP_WaitForPipeIdle(uint32_t TimeoutMs)
{
  uint32_t start = HAL_GetTick();

  do
  {
    DCMIPP_SampleCsiFlags();
    if (DCMIPP_VERIFY_CAPTURE_PIPE == DCMIPP_PIPE0)
    {
      if (((dcmipp_p0_fscr & DCMIPP_P0FSCR_PIPEN) == 0U) &&
          ((dcmipp_p0_fctcr & DCMIPP_P0FCTCR_CPTREQ) == 0U) &&
          ((dcmipp_p0_sr & DCMIPP_P0SR_CPTACT) == 0U))
      {
        return 1U;
      }
    }
    else
    {
      if (((dcmipp_p1_fscr & DCMIPP_P1FSCR_PIPEN) == 0U) &&
          ((dcmipp_p1_fctcr & DCMIPP_P1FCTCR_CPTREQ) == 0U) &&
          ((dcmipp_p1_sr & DCMIPP_P1SR_CPTACT) == 0U))
      {
        return 1U;
      }
    }
  } while ((HAL_GetTick() - start) < TimeoutMs);

  DCMIPP_SampleCsiFlags();
  return 0U;
}

static void DCMIPP_FreezeCaptureForDump(void)
{
  uint16_t mode = 0xFFFFU;
  uint32_t pipe_idle;

  dcmipp_freeze_tick = HAL_GetTick();
  printf("DCMIPP freeze start: frame_done_tick=%lu freeze_tick=%lu frames=%lu pipe=%lu p0dcc=%lu/%lu\r\n",
         dcmipp_frame_done_tick,
         dcmipp_freeze_tick,
         dcmipp_frame_count,
         (uint32_t)DCMIPP_VERIFY_CAPTURE_PIPE,
         dcmipp_p0_dccntr,
         (uint32_t)DCMIPP_VERIFY_FRAME_BYTES);

  if (HAL_DCMIPP_CSI_PIPE_Stop(&hdcmipp, DCMIPP_VERIFY_CAPTURE_PIPE, DCMIPP_VIRTUAL_CHANNEL0) != HAL_OK)
  {
    dcmipp_last_error = HAL_DCMIPP_GetError(&hdcmipp);
    printf("DCMIPP freeze: HAL_DCMIPP_CSI_PIPE_Stop returned error err=0x%08lX\r\n", dcmipp_last_error);
  }

  pipe_idle = DCMIPP_WaitForPipeIdle(DCMIPP_VERIFY_PIPE_IDLE_TIMEOUT_MS);
  if (pipe_idle == 0U)
  {
    printf("DCMIPP freeze: PIPE idle wait timeout pipe=%lu p0_enabled=%lu p0_cptreq=%lu p0_cptact=%lu p0fctcr=0x%08lX p0sr=0x%08lX p1_enabled=%lu p1_cptreq=%lu p1_cptact=%lu p1fctcr=0x%08lX p1sr=0x%08lX\r\n",
           (uint32_t)DCMIPP_VERIFY_CAPTURE_PIPE,
           (uint32_t)(((dcmipp_p0_fscr & DCMIPP_P0FSCR_PIPEN) != 0U) ? 1U : 0U),
           (uint32_t)(((dcmipp_p0_fctcr & DCMIPP_P0FCTCR_CPTREQ) != 0U) ? 1U : 0U),
           (uint32_t)(((dcmipp_p0_sr & DCMIPP_P0SR_CPTACT) != 0U) ? 1U : 0U),
           dcmipp_p0_fctcr,
           dcmipp_p0_sr,
           (uint32_t)(((dcmipp_p1_fscr & DCMIPP_P1FSCR_PIPEN) != 0U) ? 1U : 0U),
           (uint32_t)(((dcmipp_p1_fctcr & DCMIPP_P1FCTCR_CPTREQ) != 0U) ? 1U : 0U),
           (uint32_t)(((dcmipp_p1_sr & DCMIPP_P1SR_CPTACT) != 0U) ? 1U : 0U),
           dcmipp_p1_fctcr,
           dcmipp_p1_sr);
  }

#if (IMX219_DEBUG_START_STREAM != 0U)
  if (IMX219_Stop(&hcamera) != IMX219_OK)
  {
    printf("IMX219 stream OFF failed during freeze\r\n");
  }
  else
  {
    imx219_streaming = 0U;
  }
#endif

#if (DCMIPP_VERIFY_CACHE_MAINT != 0U)
  HAL_Delay(DCMIPP_VERIFY_FREEZE_SETTLE_MS);
  /*
   * DMA has already written the frame. Cleaning here can write stale CPU cache
   * lines back over the captured image; only invalidate before reading/dumping.
   */
  DCMIPP_InvalidateBuffer();
  __DSB();
#endif

  DCMIPP_SampleCsiFlags();
  (void)IMX219_ReadRegister16(&hcamera, 0x0100U, &mode);
  dcmipp_capture_frozen = 1U;
  printf("DCMIPP freeze done: pipe=%lu p0_enabled=%lu p0_cptreq=%lu p0_cptact=%lu p0fctcr=0x%08lX p0sr=0x%08lX p1_enabled=%lu p1_cptreq=%lu p1_cptact=%lu p1fctcr=0x%08lX p1sr=0x%08lX sensor_mode=0x%04lX stream=%s p0dcc=%lu/%lu\r\n",
         (uint32_t)DCMIPP_VERIFY_CAPTURE_PIPE,
         (uint32_t)(((dcmipp_p0_fscr & DCMIPP_P0FSCR_PIPEN) != 0U) ? 1U : 0U),
         (uint32_t)(((dcmipp_p0_fctcr & DCMIPP_P0FCTCR_CPTREQ) != 0U) ? 1U : 0U),
         (uint32_t)(((dcmipp_p0_sr & DCMIPP_P0SR_CPTACT) != 0U) ? 1U : 0U),
         dcmipp_p0_fctcr,
         dcmipp_p0_sr,
         (uint32_t)(((dcmipp_p1_fscr & DCMIPP_P1FSCR_PIPEN) != 0U) ? 1U : 0U),
         (uint32_t)(((dcmipp_p1_fctcr & DCMIPP_P1FCTCR_CPTREQ) != 0U) ? 1U : 0U),
         (uint32_t)(((dcmipp_p1_sr & DCMIPP_P1SR_CPTACT) != 0U) ? 1U : 0U),
         dcmipp_p1_fctcr,
         dcmipp_p1_sr,
         (uint32_t)mode,
         (imx219_streaming != 0U) ? "on" : "off",
         dcmipp_p0_dccntr,
         (uint32_t)DCMIPP_VERIFY_FRAME_BYTES);
}

static void DCMIPP_LogPipeRegisters(const char *Tag)
{
  DCMIPP_SampleCsiFlags();
  printf("DCMIPP PIPE%lu %s: configured_input=%lux%lu output=%lux%lu raw10_line_bytes=%lu pitch=%lu raw10_frame=%lu frame_bytes=%lu buffer=%lu p0fscr=0x%08lX p0fctcr=0x%08lX p0ppcr=0x%08lX p0dclmtr=0x%08lX p0m0ar1=0x%08lX p0stm0ar=0x%08lX p0dcc_bytes=%lu p0sr=0x%08lX p1fscr=0x%08lX p1fctcr=0x%08lX p1dmcr=0x%08lX p1ppcr=0x%08lX p1m0ar1=0x%08lX p1m0pr=0x%08lX p1stm0ar=0x%08lX p1sr=0x%08lX p1_ovr=%lu sr0=0x%08lX sr1=0x%08lX csi_err1=0x%08lX csi_err2=0x%08lX spdfr=0x%08lX csi_prcr=0x%08lX csi_pmcr=0x%08lX dt=0x%02lX vc=0 downsize=%lu crop=%lu decimate=%lu pixel_packer=0x%08lX header=%lu\r\n",
         (uint32_t)DCMIPP_VERIFY_CAPTURE_PIPE,
         Tag,
         (uint32_t)DCMIPP_VERIFY_WIDTH,
         (uint32_t)DCMIPP_VERIFY_HEIGHT,
         (uint32_t)DCMIPP_VERIFY_OUTPUT_WIDTH,
         (uint32_t)DCMIPP_VERIFY_OUTPUT_HEIGHT,
         (uint32_t)DCMIPP_VERIFY_RAW10_LINE_BYTES,
         (uint32_t)DCMIPP_VERIFY_LINE_PITCH,
         (uint32_t)DCMIPP_VERIFY_RAW10_FRAME_BYTES,
         (uint32_t)DCMIPP_VERIFY_FRAME_BYTES,
         (uint32_t)sizeof(dcmipp_frame_buffer),
         dcmipp_p0_fscr,
         dcmipp_p0_fctcr,
         dcmipp_p0_ppcr,
         dcmipp_p0_dclmtr,
         dcmipp_p0_ppm0ar1,
         dcmipp_p0_stm0ar,
         dcmipp_p0_dccntr,
         dcmipp_p0_sr,
         dcmipp_p1_fscr,
         dcmipp_p1_fctcr,
         dcmipp_p1_dmcr,
         dcmipp_p1_ppcr,
         dcmipp_p1_ppm0ar1,
         dcmipp_p1_ppm0pr,
         dcmipp_p1_stm0ar,
         dcmipp_p1_sr,
         (uint32_t)(((dcmipp_p1_sr & DCMIPP_P1SR_OVRF) != 0U) ? 1U : 0U),
         dcmipp_csi_sr0,
         dcmipp_csi_sr1,
         dcmipp_csi_err1,
         dcmipp_csi_err2,
         dcmipp_csi_spdfr,
         dcmipp_csi_prcr,
         dcmipp_csi_pmcr,
         (uint32_t)DCMIPP_DT_RAW10,
         (uint32_t)DCMIPP_VERIFY_ENABLE_DOWNSIZE,
         (uint32_t)DCMIPP_VERIFY_PIPE0_CROP,
         (uint32_t)DCMIPP_VERIFY_PIPE0_DECIMATE,
         (uint32_t)DCMIPP_VERIFY_PIXEL_PACKER,
         (uint32_t)DCMIPP_VERIFY_DUMP_HEADER);
}

void HAL_DCMIPP_PIPE_FrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp_cb, uint32_t Pipe)
{
  UNUSED(hdcmipp_cb);

  if (Pipe == DCMIPP_VERIFY_CAPTURE_PIPE)
  {
    dcmipp_frame_count++;
  }
}

void HAL_DCMIPP_PIPE_VsyncEventCallback(DCMIPP_HandleTypeDef *hdcmipp_cb, uint32_t Pipe)
{
  UNUSED(hdcmipp_cb);

  if (Pipe == DCMIPP_VERIFY_CAPTURE_PIPE)
  {
    dcmipp_vsync_count++;
  }
}

void HAL_DCMIPP_PIPE_ErrorCallback(DCMIPP_HandleTypeDef *hdcmipp_cb, uint32_t Pipe)
{
  if (Pipe == DCMIPP_VERIFY_CAPTURE_PIPE)
  {
    dcmipp_pipe_error_count++;
    dcmipp_last_error = HAL_DCMIPP_GetError(hdcmipp_cb);
  }
}

void HAL_DCMIPP_ErrorCallback(DCMIPP_HandleTypeDef *hdcmipp_cb)
{
  dcmipp_common_error_count++;
  dcmipp_last_error = HAL_DCMIPP_GetError(hdcmipp_cb);
}

void HAL_DCMIPP_CSI_StartOfFrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp_cb, uint32_t VirtualChannel)
{
  UNUSED(hdcmipp_cb);

  if (VirtualChannel == DCMIPP_VIRTUAL_CHANNEL0)
  {
    dcmipp_sof_count++;
  }
}

void HAL_DCMIPP_CSI_EndOfFrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp_cb, uint32_t VirtualChannel)
{
  UNUSED(hdcmipp_cb);

  if (VirtualChannel == DCMIPP_VIRTUAL_CHANNEL0)
  {
    dcmipp_eof_count++;
  }
}

void HAL_DCMIPP_CSI_LineErrorCallback(DCMIPP_HandleTypeDef *hdcmipp_cb, uint32_t DataLane)
{
  UNUSED(DataLane);

  dcmipp_line_error_count++;
  dcmipp_last_error = HAL_DCMIPP_GetError(hdcmipp_cb);
}

void HAL_DCMIPP_CSI_ShortPacketDetectionEventCallback(DCMIPP_HandleTypeDef *hdcmipp_cb)
{
  UNUSED(hdcmipp_cb);
  dcmipp_short_packet_count++;
}

void HAL_DCMIPP_CSI_LineByteEventCallback(DCMIPP_HandleTypeDef *hdcmipp_cb, uint32_t Counter)
{
  UNUSED(hdcmipp_cb);

  if (Counter == DCMIPP_CSI_COUNTER0)
  {
    dcmipp_csi_lb0_count++;
  }
  dcmipp_csi_lb_last_counter = Counter;
}

static void DCMIPP_SampleCsiFlags(void)
{
  dcmipp_csi_sr0 = CSI->SR0;
  dcmipp_csi_sr1 = CSI->SR1;
  dcmipp_csi_cr = CSI->CR;
  dcmipp_csi_ier0 = CSI->IER0;
  dcmipp_csi_ier1 = CSI->IER1;
  dcmipp_csi_pfcr = CSI->PFCR;
  dcmipp_csi_pcr = CSI->PCR;
  dcmipp_csi_lmcfgr = CSI->LMCFGR;
  dcmipp_csi_vc0cfgr1 = CSI->VC0CFGR1;
  dcmipp_csi_vc0cfgr2 = CSI->VC0CFGR2;
  dcmipp_csi_vc0cfgr3 = CSI->VC0CFGR3;
  dcmipp_csi_vc0cfgr4 = CSI->VC0CFGR4;
  dcmipp_csi_lb0cfgr = CSI->LB0CFGR;
  dcmipp_csi_prgitr = CSI->PRGITR;
  dcmipp_csi_err1 = CSI->ERR1;
  dcmipp_csi_err2 = CSI->ERR2;
  dcmipp_csi_spdfr = CSI->SPDFR;
  dcmipp_csi_prcr = CSI->PRCR;
  dcmipp_csi_pmcr = CSI->PMCR;
  dcmipp_cmcr = DCMIPP->CMCR;
  dcmipp_cmsr1 = DCMIPP->CMSR1;
  dcmipp_p0_dccntr = DCMIPP->P0DCCNTR;
  dcmipp_p0_dclmtr = DCMIPP->P0DCLMTR;
  dcmipp_p0_fscr = DCMIPP->P0FSCR;
  dcmipp_p0_fctcr = DCMIPP->P0FCTCR;
  dcmipp_p0_ppcr = DCMIPP->P0PPCR;
  dcmipp_p0_ppm0ar1 = DCMIPP->P0PPM0AR1;
  dcmipp_p0_stm0ar = DCMIPP->P0STM0AR;
  dcmipp_p0_cfscr = DCMIPP->P0CFSCR;
  dcmipp_p0_cfctcr = DCMIPP->P0CFCTCR;
  dcmipp_p0_cppcr = DCMIPP->P0CPPCR;
  dcmipp_p0_cppm0ar1 = DCMIPP->P0CPPM0AR1;
  dcmipp_p0_sr = DCMIPP->P0SR;
  dcmipp_p0_ier = DCMIPP->P0IER;
  dcmipp_p0_scstr = DCMIPP->P0SCSTR;
  dcmipp_p0_scszr = DCMIPP->P0SCSZR;
  dcmipp_p1_fscr = DCMIPP->P1FSCR;
  dcmipp_p1_fctcr = DCMIPP->P1FCTCR;
  dcmipp_p1_dmcr = DCMIPP->P1DMCR;
  dcmipp_p1_dscr = DCMIPP->P1DSCR;
  dcmipp_p1_dsrtior = DCMIPP->P1DSRTIOR;
  dcmipp_p1_dsszr = DCMIPP->P1DSSZR;
  dcmipp_p1_ppcr = DCMIPP->P1PPCR;
  dcmipp_p1_ppm0ar1 = DCMIPP->P1PPM0AR1;
  dcmipp_p1_ppm0pr = DCMIPP->P1PPM0PR;
  dcmipp_p1_stm0ar = DCMIPP->P1STM0AR;
  dcmipp_p1_cfscr = DCMIPP->P1CFSCR;
  dcmipp_p1_cfctcr = DCMIPP->P1CFCTCR;
  dcmipp_p1_cppcr = DCMIPP->P1CPPCR;
  dcmipp_p1_cppm0ar1 = DCMIPP->P1CPPM0AR1;
  dcmipp_p1_cppm0pr = DCMIPP->P1CPPM0PR;
  dcmipp_p1_sr = DCMIPP->P1SR;
  dcmipp_p1_ier = DCMIPP->P1IER;
  dcmipp_cmsr2 = DCMIPP->CMSR2;
  dcmipp_cmier = DCMIPP->CMIER;
  dcmipp_ipgr1 = DCMIPP->IPGR1;
  dcmipp_ipc1r1 = DCMIPP->IPC1R1;
  dcmipp_ipc1r2 = DCMIPP->IPC1R2;
  dcmipp_ipc1r3 = DCMIPP->IPC1R3;
  dcmipp_ipc2r1 = DCMIPP->IPC2R1;
  dcmipp_ipc2r2 = DCMIPP->IPC2R2;
  dcmipp_ipc2r3 = DCMIPP->IPC2R3;
  dcmipp_ipc3r1 = DCMIPP->IPC3R1;
  dcmipp_ipc3r2 = DCMIPP->IPC3R2;
  dcmipp_ipc3r3 = DCMIPP->IPC3R3;
  dcmipp_ipc4r1 = DCMIPP->IPC4R1;
  dcmipp_ipc4r2 = DCMIPP->IPC4R2;
  dcmipp_ipc4r3 = DCMIPP->IPC4R3;
  dcmipp_ipc5r1 = DCMIPP->IPC5R1;
  dcmipp_ipc5r2 = DCMIPP->IPC5R2;
  dcmipp_ipc5r3 = DCMIPP->IPC5R3;

  if ((dcmipp_csi_sr1 & DCMIPP_CSI_DPHY_DATA_LANE_ERROR_IT) != 0U)
  {
    dcmipp_dphy_sample_count++;
    __HAL_DCMIPP_CSI_CLEAR_DPHY_FLAG(CSI, dcmipp_csi_sr1 & DCMIPP_CSI_DPHY_DATA_LANE_ERROR_IT);
  }
}

static void DCMIPP_ClearCsiFlags(void)
{
  CSI->FCR0 = CSI_FCR0_CLB0F | CSI_FCR0_CLB1F | CSI_FCR0_CLB2F | CSI_FCR0_CLB3F |
              CSI_FCR0_CTIM0F | CSI_FCR0_CTIM1F | CSI_FCR0_CTIM2F | CSI_FCR0_CTIM3F |
              CSI_FCR0_CSOF0F | CSI_FCR0_CSOF1F | CSI_FCR0_CSOF2F | CSI_FCR0_CSOF3F |
              CSI_FCR0_CEOF0F | CSI_FCR0_CEOF1F | CSI_FCR0_CEOF2F | CSI_FCR0_CEOF3F |
              CSI_FCR0_CSPKTF | CSI_FCR0_CCCFIFOFF | CSI_FCR0_CCRCERRF |
              CSI_FCR0_CECCERRF | CSI_FCR0_CCECCERRF | CSI_FCR0_CIDERRF |
              CSI_FCR0_CSPKTERRF | CSI_FCR0_CWDERRF | CSI_FCR0_CSYNCERRF;

  CSI->FCR1 = CSI_FCR1_CESOTDL0F | CSI_FCR1_CESOTSYNCDL0F | CSI_FCR1_CEESCDL0F |
              CSI_FCR1_CESYNCESCDL0F | CSI_FCR1_CECTRLDL0F |
              CSI_FCR1_CESOTDL1F | CSI_FCR1_CESOTSYNCDL1F | CSI_FCR1_CEESCDL1F |
              CSI_FCR1_CESYNCESCDL1F | CSI_FCR1_CECTRLDL1F;
}

static uint32_t DCMIPP_WaitForDphyStopState(uint32_t TimeoutMs)
{
  const uint32_t required_stop = CSI_SR1_STOPDL0F | CSI_SR1_STOPDL1F | CSI_SR1_STOPCLF;
  uint32_t start = HAL_GetTick();

  do
  {
    uint32_t sr1 = CSI->SR1;
    if ((sr1 & required_stop) == required_stop)
    {
      return 1U;
    }
  } while ((HAL_GetTick() - start) < TimeoutMs);

  return 0U;
}

static void DCMIPP_UpdateBufferStats(void)
{
  const volatile uint8_t *buf = dcmipp_frame_buffer;
  uint32_t checksum = 0U;
  uint32_t nonzero = 0U;
  uint32_t changed = 0U;
  uint32_t sample_len = 4096U;
  uint32_t frame_ready;
#if (DCMIPP_VERIFY_CAPTURE_PIPE != DCMIPP_PIPE1)
  uint32_t raw16_sample_count = DCMIPP_VERIFY_WIDTH * DCMIPP_VERIFY_HEIGHT;
  uint32_t raw16_min = 0xFFFFU;
  uint32_t raw16_max = 0U;
  uint32_t raw16_low6_nonzero = 0U;
  uint32_t raw16_high6_nonzero = 0U;
  uint32_t raw16_sat10_count = 0U;
#endif
#if (DCMIPP_VERIFY_CAPTURE_PIPE == DCMIPP_PIPE1)
  uint32_t rgb565_min_r5 = 0x1FU;
  uint32_t rgb565_max_r5 = 0U;
  uint32_t rgb565_min_g6 = 0x3FU;
  uint32_t rgb565_max_g6 = 0U;
  uint32_t rgb565_min_b5 = 0x1FU;
  uint32_t rgb565_max_b5 = 0U;
#endif

  if (sample_len > (uint32_t)sizeof(dcmipp_frame_buffer))
  {
    sample_len = (uint32_t)sizeof(dcmipp_frame_buffer);
  }
#if (DCMIPP_VERIFY_CAPTURE_PIPE != DCMIPP_PIPE1)
  if ((raw16_sample_count * 2U) > (uint32_t)sizeof(dcmipp_frame_buffer))
  {
    raw16_sample_count = ((uint32_t)sizeof(dcmipp_frame_buffer)) / 2U;
  }
#endif

  DCMIPP_InvalidateBuffer();

  for (uint32_t i = 0U; i < sample_len; i++)
  {
    uint32_t v = (uint32_t)buf[i];
    checksum = (checksum << 5) ^ (checksum >> 2) ^ v ^ i;
    if (v != 0U)
    {
      nonzero++;
    }
    if (v != DCMIPP_VERIFY_BUFFER_FILL)
    {
      changed++;
    }
  }

  dcmipp_buf_checksum = checksum;
  dcmipp_buf_nonzero = nonzero;
  dcmipp_buf_changed = changed;
  dcmipp_buf_w0 = ((uint32_t)buf[0]) | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
  dcmipp_buf_w1 = ((uint32_t)buf[4]) | ((uint32_t)buf[5] << 8) | ((uint32_t)buf[6] << 16) | ((uint32_t)buf[7] << 24);
  dcmipp_buf_w2 = ((uint32_t)buf[8]) | ((uint32_t)buf[9] << 8) | ((uint32_t)buf[10] << 16) | ((uint32_t)buf[11] << 24);
  dcmipp_buf_w3 = ((uint32_t)buf[12]) | ((uint32_t)buf[13] << 8) | ((uint32_t)buf[14] << 16) | ((uint32_t)buf[15] << 24);
  dcmipp_buf_w4 = ((uint32_t)buf[16]) | ((uint32_t)buf[17] << 8) | ((uint32_t)buf[18] << 16) | ((uint32_t)buf[19] << 24);
  dcmipp_buf_w5 = ((uint32_t)buf[20]) | ((uint32_t)buf[21] << 8) | ((uint32_t)buf[22] << 16) | ((uint32_t)buf[23] << 24);
  dcmipp_buf_w6 = ((uint32_t)buf[24]) | ((uint32_t)buf[25] << 8) | ((uint32_t)buf[26] << 16) | ((uint32_t)buf[27] << 24);
  dcmipp_buf_w7 = ((uint32_t)buf[28]) | ((uint32_t)buf[29] << 8) | ((uint32_t)buf[30] << 16) | ((uint32_t)buf[31] << 24);

  frame_ready = (DCMIPP_VERIFY_CAPTURE_PIPE == DCMIPP_PIPE0) ?
                ((dcmipp_p0_dccntr >= DCMIPP_VERIFY_FRAME_BYTES) ? 1U : 0U) :
                (((dcmipp_frame_count > 0U) || (dcmipp_capture_frozen != 0U)) ? 1U : 0U);
  if (frame_ready == 0U)
  {
    dcmipp_raw16_valid = 0U;
    dcmipp_raw16_align = 0U;
    dcmipp_rgb565_valid = 0U;
    return;
  }

#if (DCMIPP_VERIFY_CAPTURE_PIPE == DCMIPP_PIPE1)
  for (uint32_t i = 0U; i < (DCMIPP_VERIFY_WIDTH * DCMIPP_VERIFY_HEIGHT); i++)
  {
    uint32_t byte_index = i * 2U;
    uint32_t rgb = ((uint32_t)buf[byte_index]) | ((uint32_t)buf[byte_index + 1U] << 8);
    uint32_t r5 = (rgb >> 11) & 0x1FU;
    uint32_t g6 = (rgb >> 5) & 0x3FU;
    uint32_t b5 = rgb & 0x1FU;
    if (r5 < rgb565_min_r5)
    {
      rgb565_min_r5 = r5;
    }
    if (r5 > rgb565_max_r5)
    {
      rgb565_max_r5 = r5;
    }
    if (g6 < rgb565_min_g6)
    {
      rgb565_min_g6 = g6;
    }
    if (g6 > rgb565_max_g6)
    {
      rgb565_max_g6 = g6;
    }
    if (b5 < rgb565_min_b5)
    {
      rgb565_min_b5 = b5;
    }
    if (b5 > rgb565_max_b5)
    {
      rgb565_max_b5 = b5;
    }
  }

  dcmipp_rgb565_valid = 1U;
  dcmipp_rgb565_min_r5 = rgb565_min_r5;
  dcmipp_rgb565_max_r5 = rgb565_max_r5;
  dcmipp_rgb565_min_g6 = rgb565_min_g6;
  dcmipp_rgb565_max_g6 = rgb565_max_g6;
  dcmipp_rgb565_min_b5 = rgb565_min_b5;
  dcmipp_rgb565_max_b5 = rgb565_max_b5;
  dcmipp_rgb565_s0 = ((uint32_t)buf[0]) | ((uint32_t)buf[1] << 8);
  dcmipp_rgb565_s1 = ((uint32_t)buf[2]) | ((uint32_t)buf[3] << 8);
  dcmipp_rgb565_s2 = ((uint32_t)buf[4]) | ((uint32_t)buf[5] << 8);
  dcmipp_rgb565_s3 = ((uint32_t)buf[6]) | ((uint32_t)buf[7] << 8);
  dcmipp_raw16_valid = 0U;
  dcmipp_raw16_align = 0U;
  return;
#else
  for (uint32_t i = 0U; i < raw16_sample_count; i++)
  {
    uint32_t byte_index = i * 2U;
    uint32_t raw = ((uint32_t)buf[byte_index]) | ((uint32_t)buf[byte_index + 1U] << 8);
    if (raw < raw16_min)
    {
      raw16_min = raw;
    }
    if (raw > raw16_max)
    {
      raw16_max = raw;
    }
    if ((raw & 0x003FU) != 0U)
    {
      raw16_low6_nonzero++;
    }
    if ((raw & 0xFC00U) != 0U)
    {
      raw16_high6_nonzero++;
    }
    if ((raw & 0x03FFU) == 0x03FFU)
    {
      raw16_sat10_count++;
    }
  }

  dcmipp_raw16_valid = 1U;
  dcmipp_raw16_min = raw16_min;
  dcmipp_raw16_max = raw16_max;
  dcmipp_raw16_low6_nonzero = raw16_low6_nonzero;
  dcmipp_raw16_high6_nonzero = raw16_high6_nonzero;
  dcmipp_raw16_sat10_count = raw16_sat10_count;
  dcmipp_raw16_align = (raw16_high6_nonzero == 0U) ? 1U : ((raw16_low6_nonzero == 0U) ? 2U : 3U);
  dcmipp_raw16_s0 = ((uint32_t)buf[0]) | ((uint32_t)buf[1] << 8);
  dcmipp_raw16_s1 = ((uint32_t)buf[2]) | ((uint32_t)buf[3] << 8);
  dcmipp_raw16_s2 = ((uint32_t)buf[4]) | ((uint32_t)buf[5] << 8);
  dcmipp_raw16_s3 = ((uint32_t)buf[6]) | ((uint32_t)buf[7] << 8);
  dcmipp_raw16_s4 = ((uint32_t)buf[8]) | ((uint32_t)buf[9] << 8);
  dcmipp_raw16_s5 = ((uint32_t)buf[10]) | ((uint32_t)buf[11] << 8);
  dcmipp_raw16_s6 = ((uint32_t)buf[12]) | ((uint32_t)buf[13] << 8);
  dcmipp_raw16_s7 = ((uint32_t)buf[14]) | ((uint32_t)buf[15] << 8);
#endif
}

#if (DCMIPP_VERIFY_DUMP_RAW16_LAYOUT != 0U)
static void DCMIPP_DumpRaw16Layout(void)
{
  const volatile uint8_t *buf = dcmipp_frame_buffer;
  uint32_t min_raw = 0xFFFFU;
  uint32_t max_raw = 0U;
  uint32_t nonzero_low6 = 0U;
  uint32_t nonzero_high6 = 0U;
  uint32_t sat10_count = 0U;
  uint32_t sample_count;
  uint32_t first_high6_index = 0xFFFFFFFFU;
  uint32_t line_count = DCMIPP_VERIFY_RAW16_DUMP_LINES;
  uint32_t pixel_count = DCMIPP_VERIFY_RAW16_DUMP_PIXELS;

  if (line_count > DCMIPP_VERIFY_HEIGHT)
  {
    line_count = DCMIPP_VERIFY_HEIGHT;
  }
  if (pixel_count > DCMIPP_VERIFY_WIDTH)
  {
    pixel_count = DCMIPP_VERIFY_WIDTH;
  }
  sample_count = DCMIPP_VERIFY_WIDTH * DCMIPP_VERIFY_HEIGHT;

  DCMIPP_InvalidateBuffer();

  for (uint32_t i = 0U; i < sample_count; i++)
  {
    uint32_t byte_index = i * 2U;
    uint32_t raw = ((uint32_t)buf[byte_index]) | ((uint32_t)buf[byte_index + 1U] << 8);
    if (raw < min_raw)
    {
      min_raw = raw;
    }
    if (raw > max_raw)
    {
      max_raw = raw;
    }
    if ((raw & 0x003FU) != 0U)
    {
      nonzero_low6++;
    }
    if ((raw & 0xFC00U) != 0U)
    {
      nonzero_high6++;
      if (first_high6_index == 0xFFFFFFFFU)
      {
        first_high6_index = i;
      }
    }
    if ((raw & 0x03FFU) == 0x03FFU)
    {
      sat10_count++;
    }
  }

  printf("RAW16 layout: frame_bytes=%lu stride_bytes=%lu pixels=%lux%lu scanned_lines=%lu printed_lines=%lu min_raw=0x%04lX max_raw=0x%04lX sat10=%lu/%lu nonzero_low6=%lu/%lu nonzero_high6=%lu/%lu first_high6_y=%lu first_high6_x=%lu hint=%s\r\n",
         (uint32_t)DCMIPP_VERIFY_FRAME_BYTES,
         (uint32_t)(DCMIPP_VERIFY_WIDTH * 2U),
         (uint32_t)DCMIPP_VERIFY_WIDTH,
         (uint32_t)DCMIPP_VERIFY_HEIGHT,
         (uint32_t)DCMIPP_VERIFY_HEIGHT,
         line_count,
         min_raw,
         max_raw,
         sat10_count,
         sample_count,
         nonzero_low6,
         sample_count,
         nonzero_high6,
         sample_count,
         (first_high6_index == 0xFFFFFFFFU) ? 0xFFFFFFFFU : (first_high6_index / DCMIPP_VERIFY_WIDTH),
         (first_high6_index == 0xFFFFFFFFU) ? 0xFFFFFFFFU : (first_high6_index % DCMIPP_VERIFY_WIDTH),
         (nonzero_high6 == 0U) ? "right_aligned_10bit_likely" : ((nonzero_low6 == 0U) ? "left_aligned_10bit_likely" : "mixed_or_full_16bit"));

  for (uint32_t y = 0U; y < line_count; y++)
  {
    printf("RAW16 line%lu:", y);
    for (uint32_t x = 0U; x < pixel_count; x++)
    {
      uint32_t byte_index = ((y * DCMIPP_VERIFY_WIDTH) + x) * 2U;
      uint32_t raw = ((uint32_t)buf[byte_index]) | ((uint32_t)buf[byte_index + 1U] << 8);
      printf(" %04lX", raw);
    }
    printf("\r\n");
  }

  for (uint32_t y = 0U; y < line_count; y++)
  {
    printf("RAW10R line%lu:", y);
    for (uint32_t x = 0U; x < pixel_count; x++)
    {
      uint32_t byte_index = ((y * DCMIPP_VERIFY_WIDTH) + x) * 2U;
      uint32_t raw = ((uint32_t)buf[byte_index]) | ((uint32_t)buf[byte_index + 1U] << 8);
      printf(" %03lX", raw & 0x03FFU);
    }
    printf("\r\n");
  }

  for (uint32_t y = 0U; y < line_count; y++)
  {
    printf("RAW10L line%lu:", y);
    for (uint32_t x = 0U; x < pixel_count; x++)
    {
      uint32_t byte_index = ((y * DCMIPP_VERIFY_WIDTH) + x) * 2U;
      uint32_t raw = ((uint32_t)buf[byte_index]) | ((uint32_t)buf[byte_index + 1U] << 8);
      printf(" %03lX", (raw >> 6) & 0x03FFU);
    }
    printf("\r\n");
  }
}
#endif

static void DCMIPP_DumpRGB565Layout(void)
{
  const volatile uint8_t *buf = dcmipp_frame_buffer;
  uint32_t min_r5 = 0x1FU;
  uint32_t max_r5 = 0U;
  uint32_t min_g6 = 0x3FU;
  uint32_t max_g6 = 0U;
  uint32_t min_b5 = 0x1FU;
  uint32_t max_b5 = 0U;
  uint32_t zero_pixels = 0U;
  uint32_t line_count = DCMIPP_VERIFY_RGB565_DUMP_LINES;
  uint32_t pixel_count = DCMIPP_VERIFY_RGB565_DUMP_PIXELS;
  uint32_t sample_count = DCMIPP_VERIFY_WIDTH * DCMIPP_VERIFY_HEIGHT;

  if (line_count > DCMIPP_VERIFY_HEIGHT)
  {
    line_count = DCMIPP_VERIFY_HEIGHT;
  }
  if (pixel_count > DCMIPP_VERIFY_WIDTH)
  {
    pixel_count = DCMIPP_VERIFY_WIDTH;
  }

  DCMIPP_InvalidateBuffer();

  for (uint32_t i = 0U; i < sample_count; i++)
  {
    uint32_t byte_index = i * 2U;
    uint32_t rgb = ((uint32_t)buf[byte_index]) | ((uint32_t)buf[byte_index + 1U] << 8);
    uint32_t r5 = (rgb >> 11) & 0x1FU;
    uint32_t g6 = (rgb >> 5) & 0x3FU;
    uint32_t b5 = rgb & 0x1FU;
    if (rgb == 0U)
    {
      zero_pixels++;
    }
    if (r5 < min_r5)
    {
      min_r5 = r5;
    }
    if (r5 > max_r5)
    {
      max_r5 = r5;
    }
    if (g6 < min_g6)
    {
      min_g6 = g6;
    }
    if (g6 > max_g6)
    {
      max_g6 = g6;
    }
    if (b5 < min_b5)
    {
      min_b5 = b5;
    }
    if (b5 > max_b5)
    {
      max_b5 = b5;
    }
  }

  printf("RGB565 layout: frame_bytes=%lu stride_bytes=%lu pixels=%lux%lu printed_lines=%lu black_level=%lu wb_mult_r/g/b=%lu/%lu/%lu wb_shift=%lu bayer=%lu bayer_strength=%lu gamma=1 r5=0x%02lX..0x%02lX g6=0x%02lX..0x%02lX b5=0x%02lX..0x%02lX zero=%lu/%lu\r\n",
         (uint32_t)DCMIPP_VERIFY_FRAME_BYTES,
         (uint32_t)DCMIPP_VERIFY_RGB565_LINE_BYTES,
         (uint32_t)DCMIPP_VERIFY_WIDTH,
         (uint32_t)DCMIPP_VERIFY_HEIGHT,
         line_count,
         (uint32_t)DCMIPP_VERIFY_PIPE1_BLACK_LEVEL,
         (uint32_t)DCMIPP_VERIFY_PIPE1_WB_R_MULT,
         (uint32_t)DCMIPP_VERIFY_PIPE1_WB_G_MULT,
         (uint32_t)DCMIPP_VERIFY_PIPE1_WB_B_MULT,
         (uint32_t)DCMIPP_VERIFY_PIPE1_EXPOSURE_SHIFT,
         (uint32_t)DCMIPP_VERIFY_PIPE1_BAYER,
         (uint32_t)DCMIPP_VERIFY_PIPE1_BAYER_STRENGTH,
         min_r5,
         max_r5,
         min_g6,
         max_g6,
         min_b5,
         max_b5,
         zero_pixels,
         sample_count);

  for (uint32_t y = 0U; y < line_count; y++)
  {
    printf("RGB565 line%lu:", y);
    for (uint32_t x = 0U; x < pixel_count; x++)
    {
      uint32_t byte_index = ((y * DCMIPP_VERIFY_WIDTH) + x) * 2U;
      uint32_t rgb = ((uint32_t)buf[byte_index]) | ((uint32_t)buf[byte_index + 1U] << 8);
      printf(" %04lX", rgb);
    }
    printf("\r\n");
  }
}

static void DCMIPP_FillBuffer(uint8_t Value)
{
  volatile uint8_t *buf = dcmipp_frame_buffer;

  for (uint32_t i = 0U; i < (uint32_t)sizeof(dcmipp_frame_buffer); i++)
  {
    buf[i] = Value;
  }

  DCMIPP_CleanBuffer();
}

static void DCMIPP_CleanBuffer(void)
{
#if (DCMIPP_VERIFY_CACHE_MAINT != 0U)
  SCB_CleanDCache_by_Addr((uint32_t *)dcmipp_frame_buffer, (int32_t)sizeof(dcmipp_frame_buffer));
#endif
}

static void DCMIPP_InvalidateBuffer(void)
{
#if (DCMIPP_VERIFY_CACHE_MAINT != 0U)
  SCB_InvalidateDCache_by_Addr((void *)dcmipp_frame_buffer, (int32_t)sizeof(dcmipp_frame_buffer));
#endif
}

/* USER CODE END camera_debug */
