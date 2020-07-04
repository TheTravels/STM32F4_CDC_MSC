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
#include "fatfs.h"
#include "sdio.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//#include "usbd_cdc.h"
#include "fatfs.h"
#include "ff.h"
#include "sha1/sha1.h"
#include "Periphs/uart.h"
#include "Periphs/Flash.h"
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
extern USBD_StatusTypeDef USBD_DeInit(USBD_HandleTypeDef *pdev);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
//unsigned char cdc_rx_buffer[1024];
//int cdc_rx_flag = 0;
extern int rx_buf_get(void);
extern void fs_test(void);
extern void fs_test_sdio(void);

static uint8_t vbus_high_count=0;
static uint8_t vbus_low_count=0;
static uint8_t vbus_connect=0;
static uint32_t led_tick = 0;

void vbus_poll(const uint32_t _tick)
{
	uint32_t gpio = LL_GPIO_ReadInputPort(VBUS_GPIO_Port);
	if(gpio&VBUS_Pin)
	{
		vbus_high_count++;
		if(vbus_high_count>200) vbus_high_count = 200;
		vbus_low_count=0;
	}
	else
	{
		vbus_high_count=0;
		vbus_low_count++;
		if(vbus_low_count>200) vbus_low_count = 200;
	}
	if((led_tick>0) && (led_tick<=_tick))
	{
		led_tick = _tick + 100;
		LL_GPIO_TogglePin(GPIOD, LED_Pin|PWR_EN_GPS_Pin);
	}
}

uint8_t SD_GetCardInfo(HAL_SD_CardInfoTypeDef *cardinfo)
{
    uint8_t sta;
    sta=HAL_SD_GetCardInfo(&hsd,cardinfo);
    return sta;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	static uint8_t send_buf[256];
    int len=0;
    HAL_SD_CardInfoTypeDef cardinfo;
//    int ret = 0;
//    uint32_t data[3]={0x123456AB, 0x12CD4568, 0x1256EF34};
    //int ch=-1;
    led_tick = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  uartx_queue_init();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  //MX_USB_DEVICE_Init();
  MX_SDIO_SD_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  //USART1_Init(115200);
  //USART2_Init(115200);
  //USART3_Init(115200);
  //MX_FATFS_Init();
  SD_initialize(0);
  fs_test();
  //fs_test_sdio();
  //MX_USB_DEVICE_Init();
  //SHA1(NULL, "Hello", 5); // -Os Optimize code, add code 4K
  LL_GPIO_ResetOutputPin(GPIOD, LED_Pin|PWR_EN_GPS_Pin);
  led_tick = HAL_GetTick() + 200;
  HAL_Delay(200);  // delay, check VBUS
  app_debug("[%s--%d] system start!\r\n", __func__, __LINE__);
  app_debug("[%s--%d] Ver[%d | 0x%08X]:%s\r\n", __func__, __LINE__, sizeof(Emb_Version), &Emb_Version, Emb_Version.version);
//  ret = FLASH_Erase(0x08020000, 0x08030000);
//  app_debug("[%s--%d] FLASH_Erase[%d]\r\n", __func__, __LINE__, ret);
//  ret = Flash_Write(0x08010000, data, 3);
//  app_debug("[%s--%d] FLASH_write<%d>[%08X %08X %08X]\r\n", __func__, __LINE__, ret, data[0], data[1], data[2]);
//  memset(data, 0, sizeof(data));
//  ret = Flash_Read(0x08010000, data, 3);
//  app_debug("[%s--%d] FLASH_read<%d>[%08X %08X %08X]\r\n", __func__, __LINE__, ret, data[0], data[1], data[2]);
  //Flash_Test(0x08010000, 0x08020000);
  //flash_disk_init();
  //sram_disk_init();
  //SD_GetCardInfo(&cardinfo);
  BSP_SD_GetCardInfo(&cardinfo);
  app_debug("[%s--%d] Specifies the card Type :%d \r\n", __func__, __LINE__, cardinfo.CardType);
  app_debug("[%s--%d] Specifies the card version :%d \r\n", __func__, __LINE__, cardinfo.CardVersion);
  app_debug("[%s--%d] Specifies the class of the card class :%d \r\n", __func__, __LINE__, cardinfo.Class);
  app_debug("[%s--%d] Specifies the Relative Card Address :%d \r\n", __func__, __LINE__, cardinfo.RelCardAdd);
  app_debug("[%s--%d] Specifies the Card Capacity in blocks :%d | %d | %d \r\n", __func__, __LINE__, cardinfo.BlockNbr, cardinfo.BlockNbr*cardinfo.BlockSize, cardinfo.BlockNbr*cardinfo.BlockSize/1024/1024);
  app_debug("[%s--%d] Specifies one block size in bytes :%d \r\n", __func__, __LINE__, cardinfo.BlockSize);
  app_debug("[%s--%d] Specifies the Card logical Capacity in blocks :%d \r\n", __func__, __LINE__, cardinfo.LogBlockNbr);
  app_debug("[%s--%d] Specifies logical block size in bytes  :%d \r\n", __func__, __LINE__, cardinfo.LogBlockSize);
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
	  if((0==vbus_connect) && (vbus_high_count>100)) // usb connect
	  {
		  vbus_connect = 1;
		  MX_USB_DEVICE_Init();
	  }
	  if((1==vbus_connect) && (vbus_low_count>100)) // usb disconnect
	  {
		  vbus_connect = 0;
		  USBD_DeInit(&hUsbDeviceFS);
	  }
	  memset(send_buf, 0, sizeof(send_buf));
#if 0
      for(len=0; len<60; len++)
      {
          ch = rx_buf_get();
          if(ch<0) break;
          send_buf[len] = ch;
      }
      if(len>0)
      {
          CDC_Transmit_FS(send_buf, len);
      }
#else
      len = cdc_read(send_buf, sizeof(send_buf));
      if(len>0)
      {
    	  cdc_send(send_buf, len);
      }
#endif
      memset(send_buf, 0, sizeof(send_buf));
      len = uart3_read(send_buf, sizeof(send_buf));
      if(len>0)
      {
    	  uart3_send(send_buf, len);
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
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);

  if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1)
  {
  Error_Handler();  
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {
    
  }
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_16, 192, LL_RCC_PLLP_DIV_2);
  LL_RCC_PLL_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_16, 192, LL_RCC_PLLQ_DIV_4);
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {
    
  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_2);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  
  }
  LL_SetSystemCoreClock(48000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();  
  };
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
