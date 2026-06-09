/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32n6xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* USER CODE BEGIN Includes */
#include "stm32n6xx_ll_bus.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{

  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  /* System interrupt init*/

  HAL_PWREx_EnableVddIO2();

  HAL_PWREx_EnableVddIO4();

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}

/* USER CODE BEGIN 1 */
#define CAMERA_DCMIPP_IC17_DIVIDER 10U
#define CAMERA_CSI_IC18_DIVIDER 20U

void HAL_DCMIPP_MspInit(DCMIPP_HandleTypeDef* hdcmipp)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  RIMC_MasterConfig_t RIMC_master = {0};

  if (hdcmipp->Instance == DCMIPP)
  {
    __HAL_RCC_DCMIPP_CLK_ENABLE();
    __HAL_RCC_DCMIPP_FORCE_RESET();
    __HAL_RCC_DCMIPP_RELEASE_RESET();

    __HAL_RCC_CSI_CLK_ENABLE();
    __HAL_RCC_CSI_FORCE_RESET();
    __HAL_RCC_CSI_RELEASE_RESET();

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_DCMIPP;
    PeriphClkInitStruct.DcmippClockSelection = RCC_DCMIPPCLKSOURCE_IC17;
    PeriphClkInitStruct.ICSelection[RCC_IC17].ClockSelection = RCC_ICCLKSOURCE_PLL1;
    PeriphClkInitStruct.ICSelection[RCC_IC17].ClockDivider = CAMERA_DCMIPP_IC17_DIVIDER;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CSI;
    PeriphClkInitStruct.ICSelection[RCC_IC18].ClockSelection = RCC_ICCLKSOURCE_PLL1;
    PeriphClkInitStruct.ICSelection[RCC_IC18].ClockDivider = CAMERA_CSI_IC18_DIVIDER;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_RAMCFG_CLK_ENABLE();

    __HAL_RCC_RIFSC_CLK_ENABLE();
    RIMC_master.MasterCID = RIF_CID_1;
    RIMC_master.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV;
    HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_DCMIPP, &RIMC_master);
    HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_DCMIPP,
                                          RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);

    HAL_NVIC_SetPriority(DCMIPP_IRQn, 0x07, 0);
    HAL_NVIC_EnableIRQ(DCMIPP_IRQn);

    HAL_NVIC_SetPriority(CSI_IRQn, 0x07, 0);
    HAL_NVIC_EnableIRQ(CSI_IRQn);
  }
}

void HAL_DCMIPP_MspDeInit(DCMIPP_HandleTypeDef* hdcmipp)
{
  if (hdcmipp->Instance == DCMIPP)
  {
    HAL_NVIC_DisableIRQ(DCMIPP_IRQn);
    HAL_NVIC_DisableIRQ(CSI_IRQn);

    __HAL_RCC_DCMIPP_FORCE_RESET();
    __HAL_RCC_DCMIPP_RELEASE_RESET();
    __HAL_RCC_DCMIPP_CLK_DISABLE();

    __HAL_RCC_CSI_FORCE_RESET();
    __HAL_RCC_CSI_RELEASE_RESET();
    __HAL_RCC_CSI_CLK_DISABLE();
  }
}

/* USER CODE END 1 */
