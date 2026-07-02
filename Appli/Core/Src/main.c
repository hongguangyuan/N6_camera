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
#include "camera_pipeline.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CAMERA_DEBUG_CONFIG_RISAF 1U
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
  /* USER CODE BEGIN 2 */
  __enable_irq();
  printf("\r\nFSBL->Appli camera application start\r\n");
  printf("SystemIsolation: before config\r\n");
  SystemIsolation_Config();
  printf("SystemIsolation: risaf=%lu dapcid=0x%08lX\r\n",
         (uint32_t)CAMERA_DEBUG_CONFIG_RISAF,
         (uint32_t)HAL_RIF_RIMC_GetDebugAccessPortCID());
  if (CameraPipeline_InitAndStart(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    CameraPipeline_Task();
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
  HAL_RIF_RIMC_SetDebugAccessPortCID(RIF_CID_1);

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

  /*
   * Keep the application in the secure world, but make the main RAM/flash
   * regions readable and writable by every RIF compartment. This is useful
   * during camera bring-up because the DCMIPP frame buffer is in CPUAXI RAM0
   * at 0x34082000 and external debug tools may use a different CID.
   */
  risaf_base_config.Filtering = RISAF_FILTER_ENABLE;
  risaf_base_config.ReadWhitelist = RIF_CID_0 | RIF_CID_1 | RIF_CID_2 | RIF_CID_3 |
                                    RIF_CID_4 | RIF_CID_5 | RIF_CID_6 | RIF_CID_7;
  risaf_base_config.WriteWhitelist = RIF_CID_0 | RIF_CID_1 | RIF_CID_2 | RIF_CID_3 |
                                     RIF_CID_4 | RIF_CID_5 | RIF_CID_6 | RIF_CID_7;
  risaf_base_config.Secure = RIF_ATTRIBUTE_SEC;
  risaf_base_config.PrivWhitelist = RIF_CID_0 | RIF_CID_1 | RIF_CID_2 | RIF_CID_3 |
                                    RIF_CID_4 | RIF_CID_5 | RIF_CID_6 | RIF_CID_7;
  risaf_base_config.StartAddress = 0x0000;

  /* CPUAXI RAM0 configured range from the .ioc is 0x34000000..0x3409BFFF.
     This includes the camera frame buffer at 0x34082000. */
  risaf_base_config.EndAddress = 0x0009BFFF;
  HAL_RIF_RISAF_ConfigBaseRegion(RISAF2, RISAF_REGION_1, &risaf_base_config);

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
 * @brief  This function is executed in case
 * of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while (1)
  {
    static const uint8_t error_banner[] = "\r\nERROR_HANDLER_ALIVE\r\n";
    (void)HAL_UART_Transmit(&huart3,
                            (uint8_t *)error_banner,
                            (uint16_t)(sizeof(error_banner) - 1U),
                            100U);
    CameraPipeline_PrintErrorContext();
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
