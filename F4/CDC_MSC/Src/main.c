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
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//#include "usbd_cdc.h"
#include "fatfs.h"
#include "ff.h"
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
extern uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
//unsigned char cdc_rx_buffer[1024];
//int cdc_rx_flag = 0;
extern int rx_buf_get(void);

void fs_test(void)
{
	FATFS fs;           /* Filesystem object */
    FIL fil;            /* File object */
    FRESULT res;  /* API result code */
    UINT bw;            /* Bytes written */
    uint8_t work[1024]; /* Work area (larger is better for processing time) */
    uint8_t mm[50];
    uint8_t wtext[] = "\r\n测试数据！！！"; //
    UINT i;

	//uart_printf("文件系统测试开始:\r\n");
	/* 格式化文件系统 */
	res = f_mkfs("0:", FM_ANY, 0, work, sizeof(work));//"0:"是卷标，来自于 #define SPI_FLASH		0
	if (res)
	{
		//uart_printf("文件系统格式化失败.\r\n");
		return ;
	}
	else
	{
		//uart_printf("文件系统格式化成功.\r\n");
	}
	/* 挂载文件系统 */
	res = f_mount(&fs, "0:", 0);
	if (res)
	{
		//uart_printf("文件系统挂载失败.\r\n");
	}
	else
	{
		//uart_printf("文件系统挂载成功.\r\n");
	}
	/* Create a file as new */
	res = f_open(&fil, "0:/123.txt", FA_CREATE_NEW|FA_WRITE|FA_READ);
	if (res)
	{
		//uart_printf("打开文件失败.\r\n");
	}
	else
	{
		//uart_printf("打开文件成功.\r\n");
	}
	/* Write a message */
	res = f_write(&fil, "Hello,World!", 12, &bw);
	res = f_write(&fil, wtext, sizeof(wtext), (void *)&bw);
	//uart_printf("res write:%d\r\n",res);
	if (bw == 12)
	{
		//uart_printf("写文件成功!\r\n");
	}
	else
	{
		//uart_printf("写文件失败!\r\n");
	}
	res = f_size(&fil);
	//uart_printf("文件大小:%d Bytes.\r\n",res);
	memset(mm,0x0,50);
	f_lseek(&fil,0);
	res = f_read(&fil,mm,12,&i);
	if (res == FR_OK)
	{
		//uart_printf("读文件成功!\r\n");
		//uart_printf("读到数据长度:%d Bytes.\r\n",i);
	}
	else
	{
		//uart_printf("读文件失败!\r\n");
	}
	//uart_printf("读到如下数据:\r\n");
	//buff_print((char *)mm,12);
	/* Close the file */
	f_close(&fil);
	/*卸载文件系统*/
	f_mount(0, "0:", 0);
	//uart_printf("文件系统测试完毕.\r\n");
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	static uint8_t send_buf[60];
    uint16_t len=0;
    int ch=-1;
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
  //MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  MX_FATFS_Init();
  fs_test();
  MX_USB_DEVICE_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//    if(cdc_rx_flag > 0)
//    {
//      USBD_LL_Transmit(&hUsbDeviceFS,
//                       CDC_IN_EP,
//                       cdc_rx_buffer,
//                       cdc_rx_flag);
//      cdc_rx_flag = 0;
//    }
      for(len=0; len<sizeof(send_buf); len++)
      {
          ch = rx_buf_get();
          if(ch<0) break;
          send_buf[len] = ch;
      }
      if(len>0)
      {
          CDC_Transmit_FS(send_buf, len);
      }
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 192;
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
