#ifndef IMX219_ISP_PARAM_CONF_H
#define IMX219_ISP_PARAM_CONF_H

#include "isp_api.h"

#ifndef CAMERA_PIPELINE_SENSOR_BAYER_PATTERN
#define CAMERA_PIPELINE_SENSOR_BAYER_PATTERN ISP_DEMOS_TYPE_RGGB
#endif

static const ISP_IQParamTypeDef ISP_IQParamCacheInit_IMX219 = {
  .sensorGainStatic = {
    .gain = 6000U,
  },
  .sensorExposureStatic = {
    .exposure = 10000U,
  },
  .AECAlgo = {
    .enable = 0U,
    .exposureCompensation = EXPOSURE_TARGET_0_0_EV,
    .antiFlickerFreq = ANTIFLICKER_NONE,
  },
  .statRemoval = {
    .enable = 0U,
    .nbHeadLines = 0U,
    .nbValidLines = 0U,
  },
  .badPixelStatic = {
    .enable = 0U,
    .strength = 0U,
  },
  .badPixelAlgo = {
    .enable = 0U,
    .threshold = 0U,
  },
  .blackLevelStatic = {
    .enable = 1U,
    .BLCR = 12U,
    .BLCG = 12U,
    .BLCB = 12U,
  },
  .demosaicing = {
    .enable = 1U,
    .type = CAMERA_PIPELINE_SENSOR_BAYER_PATTERN,
    .peak = 4U,
    .lineV = 4U,
    .lineH = 4U,
    .edge = 4U,
  },
  .ispGainStatic = {
    .enable = 1U,
    .ispGainR = 100000000U,
    .ispGainG = 100000000U,
    .ispGainB = 100000000U,
  },
  .colorConvStatic = {
    .enable = 0U,
    .coeff = {
      { 100000000, 0, 0 },
      { 0, 100000000, 0 },
      { 0, 0, 100000000 },
    },
  },
  .AWBAlgo = {
    .enable = 0U,
    .id = { "D65", "D50", "CWF", "TL84", "A" },
    .referenceColorTemp = { 6500U, 5000U, 4100U, 3500U, 2800U },
    .ispGainR = { 100000000U, 100000000U, 100000000U, 100000000U, 100000000U },
    .ispGainG = { 100000000U, 100000000U, 100000000U, 100000000U, 100000000U },
    .ispGainB = { 100000000U, 100000000U, 100000000U, 100000000U, 100000000U },
    .coeff = {
      { { 100000000, 0, 0 }, { 0, 100000000, 0 }, { 0, 0, 100000000 } },
      { { 100000000, 0, 0 }, { 0, 100000000, 0 }, { 0, 0, 100000000 } },
      { { 100000000, 0, 0 }, { 0, 100000000, 0 }, { 0, 0, 100000000 } },
      { { 100000000, 0, 0 }, { 0, 100000000, 0 }, { 0, 0, 100000000 } },
      { { 100000000, 0, 0 }, { 0, 100000000, 0 }, { 0, 0, 100000000 } },
    },
  },
  .contrast = {
    .enable = 0U,
    .coeff = {
      .LUM_0 = 100U,
      .LUM_32 = 100U,
      .LUM_64 = 100U,
      .LUM_96 = 100U,
      .LUM_128 = 100U,
      .LUM_160 = 100U,
      .LUM_192 = 100U,
      .LUM_224 = 100U,
      .LUM_256 = 100U,
    },
  },
  .statAreaStatic = {
    .X0 = 0U,
    .Y0 = 0U,
    .XSize = 1640U,
    .YSize = 1232U,
  },
  .gamma = {
    .enable = 0U,
  },
  .sensorDelay = {
    .delay = 0U,
  },
};

static const ISP_IQParamTypeDef *ISP_IQParamCacheInit[] = {
  &ISP_IQParamCacheInit_IMX219,
};

#endif /* IMX219_ISP_PARAM_CONF_H */
