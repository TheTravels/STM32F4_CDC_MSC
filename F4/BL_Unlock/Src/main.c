/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Periphs/uart.h"
#include "version.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static inline void delay_us(uint16_t us)
{
	while(us--)
	{
		asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");
		asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");
		asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");
		asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");
		asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");
		asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");
		asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");
		asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");
		asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");asm("mov r0,r0");
	}
}
void delay_ms(uint16_t ms)
{
	while(ms--)
	{
		//for(int i=0; i<48; i++) asm("mov r0,r0");
		delay_us(1000);
	}
}

/****************************************************************
* Function:    Flash_EnableReadProtection
* Description: Enable the read protection of user flash area.
* Input:        NONE
* Output:        NONE
* Return:  NONE
*****************************************************************/

void Flash_EnableReadProtection(void)
{

	FLASH_OBProgramInitTypeDef OBInit;

	__HAL_FLASH_PREFETCH_BUFFER_DISABLE();

	HAL_FLASHEx_OBGetConfig(&OBInit);
	if(OBInit.RDPLevel == OB_RDP_LEVEL_0)
	{
		OBInit.OptionType = OPTIONBYTE_RDP;
		OBInit.RDPLevel = OB_RDP_LEVEL_1;
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		HAL_FLASHEx_OBProgram(&OBInit);
		HAL_FLASH_OB_Launch();
		HAL_FLASH_OB_Lock();
		HAL_FLASH_Lock();
	}
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();

}

void Flash_EnableWriteProtection(void)
{

	FLASH_OBProgramInitTypeDef OBInit;

	__HAL_FLASH_PREFETCH_BUFFER_DISABLE();

	HAL_FLASHEx_OBGetConfig(&OBInit);
	app_debug("[%s--%d] WRPSector[0x%08X]\r\n", __func__, __LINE__, OBInit.WRPSector);
	// 锁住 boot 扇区, 0是保护
	if(0x00 != (OBInit.WRPSector&0x07))
	{
		OBInit.OptionType = OPTIONBYTE_WRP;
		OBInit.WRPState = OB_WRPSTATE_ENABLE;
		OBInit.WRPSector = 0x07;
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		HAL_FLASHEx_OBProgram(&OBInit);
		HAL_FLASH_OB_Launch();
		HAL_FLASH_OB_Lock();
		HAL_FLASH_Lock();
	}
	app_debug("[%s--%d] WRPSector[0x%08X]\r\n", __func__, __LINE__, OBInit.WRPSector);
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();

}

/****************************************************************
* Function:    Flash_DisableReadProtection
* Description: Disable the read protection of user flash area.
* Input:        NONE
* Output:        NONE
* Return:  NONE
*****************************************************************/
void Flash_DisableReadProtection(void)
{

  FLASH_OBProgramInitTypeDef OBInit;

  __HAL_FLASH_PREFETCH_BUFFER_DISABLE();

  HAL_FLASHEx_OBGetConfig(&OBInit);
  if(OBInit.RDPLevel != OB_RDP_LEVEL_0)
  {
    OBInit.OptionType = OPTIONBYTE_RDP;
    OBInit.RDPLevel = OB_RDP_LEVEL_0;
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    HAL_FLASHEx_OBProgram(&OBInit);
    HAL_FLASH_OB_Launch();
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
  }
  __HAL_FLASH_PREFETCH_BUFFER_ENABLE();

}
void Flash_DisableWriteProtection(void)
{

	FLASH_OBProgramInitTypeDef OBInit;

	__HAL_FLASH_PREFETCH_BUFFER_DISABLE();

	HAL_FLASHEx_OBGetConfig(&OBInit);
	app_debug("[%s--%d] WRPSector[0x%08X]\r\n", __func__, __LINE__, OBInit.WRPSector);
	// 解锁
	if(0x7FF != (OBInit.WRPSector&0x7FF))
	{
		OBInit.OptionType = OPTIONBYTE_WRP;
		OBInit.WRPState = OB_WRPSTATE_DISABLE;
		OBInit.WRPSector = 0x7FF;
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		HAL_FLASHEx_OBProgram(&OBInit);
		HAL_FLASH_OB_Launch();
		HAL_FLASH_OB_Lock();
		HAL_FLASH_Lock();
	}
	app_debug("[%s--%d] WRPSector[0x%08X]\r\n", __func__, __LINE__, OBInit.WRPSector);
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();

}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

	//const struct Emb_Device_Version* const flash = (const struct Emb_Device_Version*)0x08000000UL;
	//int conut=0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
	app_debug("[%s--%d] Ver[%d | 0x%08X]:%s\r\n", __func__, __LINE__, sizeof(Emb_Version), &Emb_Version, Emb_Version.version);
	//app_debug("[%s--%d] Ver[0x%08X]:%s\r\n", __func__, __LINE__, flash, flash->version);
	//
	//Flash_DisableReadProtection();
	Flash_EnableWriteProtection();
	Flash_DisableWriteProtection();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  LL_GPIO_TogglePin(GPIOD, LED_GREEN_Pin);
	  //app_debug("[%s--%d] count:%d \r\n", __func__, __LINE__, conut++);
	  delay_ms(500);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
