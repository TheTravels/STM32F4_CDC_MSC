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
#include "Periphs/crc.h"
#include "Periphs/ParamTable.h"
#include "version.h"
#include "GB/ZKHY/ZKHY_Dev_upload.h"
#include "core_cm4.h"
#include "Ini/Ini.h"
#include "Ini/Files.h"
#include "tea/tea.h"
#include "driver/ec20.h"

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
//static const char fw_name[] = "FW";
//static const char fw_key_name[] = "Name";
//static const char fw_key_total[] = "total";
//static const char fw_key_crc[] = "CRC";
//static const char fw_key_sha1[] = "SHA1";
//static const char fw_key_time[] = "Time";
extern uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
//unsigned char cdc_rx_buffer[1024];
//int cdc_rx_flag = 0;
extern int rx_buf_get(void);
extern void fs_test(void);
extern void fs_test_sdio(void);

void bl_entry(void);

//uint8_t SD_GetCardInfo(HAL_SD_CardInfoTypeDef *cardinfo)
//{
//    uint8_t sta;
//    sta=HAL_SD_GetCardInfo(&hsd,cardinfo);
//    return sta;
//}

//static uint8_t _ccm __attribute__ ((aligned (4))) bl_data[1024*4];
//extern void HAL_SD_MspDeInit(SD_HandleTypeDef* sdHandle);
extern void boot_app(void);

extern int check_bl(uint8_t _buf[], const uint16_t _bsize, int (*const read_func)(uint8_t buf[], const uint32_t _size));
extern void msc_upload(void);

// 对芯片签名, 签名长度 32B
extern uint8_t read_uid(uint8_t uid[]);
//static uint8_t send_buf[256];
extern uint32_t sign_flag;
extern void first_sign_chip(void);
static const char Param_name[] = "Param";
static const char Param_key_sn[] = "SN";
//static const char Param_key_host[] = "Host";
//static const char Param_key_port[] = "Port";
static const char Param_key_ftph[] = "FTPH";
static const char Param_key_ftpp[] = "FTPP";
static const char Param_key_user[] = "user";
static const char Param_key_passwd[] = "pass";
//static const char Param_key_time[] = "Time";
extern void verify_chip(void);

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	/* USER CODE BEGIN 1 */
	//static uint8_t send_buf[256];
	//int len=0;
	//int bl_len;
	//uint32_t crc;
	uint32_t addr = (uint32_t)&first_sign_chip;
	addr = addr-(addr&0x03);  // 对齐
	//SCB->VTOR = 0x08010000UL ;
	//HAL_SD_CardInfoTypeDef cardinfo;
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
	//USART3_Init(115200);
	//jump_to_app();
	//boot_app();
	// 芯片加密校验
#if 1 // 调试时关闭这部分功能
	if(0x00000000!=(*(const uint32_t*)addr)) first_sign_chip();
	verify_chip();
#endif
	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USB_DEVICE_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();
	MX_USART3_UART_Init();
	MX_FATFS_Init();
	/* USER CODE BEGIN 2 */
	led_tick = 100;
	USART1_Init(115200);
	USART2_Init(115200);
	USART3_Init(115200);
	//EC20_Test();
	//EC20_FTP_Test();
	// 未质检设备不连 FTP升级,即生产中的设备不升级
	if(1==ParamTable_quality())
	{
		int port;
		char param_buf[512];
		char sn[32];
		char ftp[32];
		char user[32];
		char passwd[32];
	    struct Ini_Parse Ini = {
	        "fw.Ini",
	        .text = param_buf,
	         ._bsize = sizeof(param_buf),
	         .pos = 0,
	         ._dsize = 0,
	    };
	    ParamTable_Read(param_buf, 0, sizeof(param_buf));
	    Ini._dsize = strlen(Ini.text);
	    // 解析参数
	    memset(sn, 0, sizeof(sn));
	    Ini_get_field(&Ini, Param_name, Param_key_sn, "-", sn);
	    Ini_get_field(&Ini, Param_name, Param_key_ftph, "39.108.51.99", ftp);
	    Ini_get_field(&Ini, Param_name, Param_key_user, "obd4g", user);
	    Ini_get_field(&Ini, Param_name, Param_key_passwd, "obd.4g", passwd);
	    port = Ini_get_int(&Ini, Param_name, Param_key_ftpp, 21);
		//EC20_FTP_Upload(Emb_Version.hardware, "0A0CK90N4123", "39.108.51.99", 21, "obd4g", "obd.4g");
	    // 序列号必须有效
	    if('-'!=sn[0]) EC20_FTP_Upload(Emb_Version.hardware, sn, "39.108.51.99", port, user, passwd);
	}
	led_tick = 100;
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
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
