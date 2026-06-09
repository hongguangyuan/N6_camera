/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : FSBL -> Appli LED / USART baseline application.
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
#include "csi.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "camera_debug.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CAMERA_DEBUG_CONFIG_RISAF 0U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void SystemIsolation_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_I2C1_Init();
  SystemIsolation_Config();
  /* USER CODE BEGIN 2 */
  __enable_irq();
  printf("\r\nFSBL->Appli camera application start\r\n");
  printf("SystemIsolation: risaf=%lu\r\n", (uint32_t)CAMERA_DEBUG_CONFIG_RISAF);
  CameraDebug_InitAndStart(&hi2c1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    CameraDebug_Task();
  }
  /* USER CODE END 3 */
}

/**
 * @brief RIF Initialization Function
 * @param None
 * @retval None
 */
static void SystemIsolation_Config(void)
{

  /* USER CODE BEGIN RIF_Init 0 */

  /* USER CODE END RIF_Init 0 */

  /* set all required IPs as secure privileged */
  __HAL_RCC_RIFSC_CLK_ENABLE();

  /* RIF-Aware IPs Config */

  /* set up GPIO configuration */
  HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_0, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOC, GPIO_PIN_1, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOD, GPIO_PIN_2, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOD, GPIO_PIN_8, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOD, GPIO_PIN_9, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOH, GPIO_PIN_9, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOO, GPIO_PIN_1, GPIO_PIN_SEC | GPIO_PIN_NPRIV);

  /* USER CODE BEGIN RIF_Init 1 */
#if (CAMERA_DEBUG_CONFIG_RISAF != 0U)
  RISAF_BaseRegionConfig_t risaf_base_config = {0};

  __HAL_RCC_RISAF_CLK_ENABLE();

  risaf_base_config.Filtering = RISAF_FILTER_ENABLE;
  risaf_base_config.ReadWhitelist = 255;
  risaf_base_config.WriteWhitelist = 255;
  risaf_base_config.Secure = RIF_ATTRIBUTE_SEC;
  risaf_base_config.PrivWhitelist = RIF_CID_NONE;
  risaf_base_config.StartAddress = 0x0000;

  /* XSPI2 full range */
  risaf_base_config.EndAddress = 0x07FFFFFF;
  HAL_RIF_RISAF_ConfigBaseRegion(RISAF12, RISAF_REGION_1, &risaf_base_config);

  /* CPUAXI RAM1 full range */
  risaf_base_config.EndAddress = 0x000FFFFF;
  HAL_RIF_RISAF_ConfigBaseRegion(RISAF3, RISAF_REGION_1, &risaf_base_config);

  /* XSPI1 full range */
  risaf_base_config.EndAddress = 0x01FFFFFF;
  HAL_RIF_RISAF_ConfigBaseRegion(RISAF11, RISAF_REGION_1, &risaf_base_config);

  /* CPUAXI RAM0 full range, including the camera frame buffer */
  risaf_base_config.EndAddress = 0x001FFFFF;
  HAL_RIF_RISAF_ConfigBaseRegion(RISAF2, RISAF_REGION_1, &risaf_base_config);

  /* FLEXRAM full range */
  risaf_base_config.EndAddress = 0x00063FFF;
  HAL_RIF_RISAF_ConfigBaseRegion(RISAF7, RISAF_REGION_1, &risaf_base_config);

  /* GPDMA1 channels 0..15 secure and privileged. */
  __HAL_RCC_GPDMA1_CLK_ENABLE();
  GPDMA1->PRIVCFGR |= 0x0000FFFFU;
#if defined(CPU_IN_SECURE_STATE)
  GPDMA1->SECCFGR |= 0x0000FFFFU;
  GPDMA1_Channel0->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel1->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel2->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel3->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel4->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel5->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel6->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel7->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel8->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel9->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel10->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel11->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel12->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel13->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel14->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
  GPDMA1_Channel15->CTR1 |= DMA_CTR1_SSEC | DMA_CTR1_DSEC;
#endif /* CPU_IN_SECURE_STATE */
#endif /* CAMERA_DEBUG_CONFIG_RISAF */

  /* USER CODE END RIF_Init 1 */
  /* USER CODE BEGIN RIF_Init 2 */

  /* USER CODE END RIF_Init 2 */
}

/* USER CODE BEGIN 4 */
int __io_putchar(int ch)
{
  uint8_t c = (uint8_t)ch;

  if (HAL_UART_Transmit(&huart3, &c, 1U, 10U) != HAL_OK)
  {
    return EOF;
  }

  return ch;
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  CameraDebug_PrintErrorContext();
  __disable_irq();
  while (1)
  {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    for (volatile uint32_t i = 0U; i < 2000000U; i++)
    {
      __NOP();
    }
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
