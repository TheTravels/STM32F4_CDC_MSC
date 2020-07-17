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
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Periphs/uart.h"
#include "version.h"
#include "driver/ec20.h"
#include "Periphs/ParamTable.h"
#include "tea/tea.h"
#include "Periphs/crc.h"
#include "Periphs/Flash.h"

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
uint8_t vbus_high_count=0;
uint8_t vbus_low_count=0;
uint8_t vbus_connect=0;
uint32_t led_tick = 0;
//extern void bl_entry(void);
extern uint32_t g_pfnVectors;

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
		LL_GPIO_TogglePin(GPIOD, LED_GREEN_Pin);
		//LL_GPIO_TogglePin(GPIOD, LED_RED_Pin);
	}
}

// 对芯片签名, 签名长度 32B
extern uint8_t read_uid(uint8_t uid[]);
//static uint8_t send_buf[256];
uint32_t sign_flag = 0;
static inline uint32_t sign_chip(uint32_t sign[8])
{
	// 对齐
	uint32_t uid[8];
	uint16_t count, i;
	uint32_t crc16 = 0;
	uint32_t crc;
	//uint32_t addr = (uint32_t)&bl_entry;
	uint32_t addr = (uint32_t)0x08003658;
	const uint32_t* flash = (const uint32_t*)0x08000400;
	const uint32_t key[4]={0x89480304, 0x60670708, 0x090A0B0C, 0x68270F10};
	const uint16_t emb_iteration = 64;

	addr = addr-(addr&0x03);  // 对齐
	flash = (const uint32_t*)(addr+((flash[0]&0x7F)<<2));
	//memset(uid, 0x5A, sizeof(uid));   // 填默认值
	memcpy(uid, flash, sizeof(uid));  // 将代码作为随机数使用
	// 读取 ID
	read_uid((uint8_t*)uid);  // 96 bit
	// 使用 TEA 加密, 迭代处理,将对称加密变成不可解密加密算法
	for(count=0; count< 16; count++)
	{
		memcpy(&sign[0], key, sizeof(key));
		memcpy(&sign[4], key, sizeof(key));
		tea_encrypt(sign, 4*8, uid, emb_iteration);
		// 叠加,将 sign 和 key 带入 uid,从而让数据不可解密
		for(i=0; i<8; i++) uid[i] = uid[i] + ((uid[i]>>i)&0xFFFF) + ((sign[i]<<i)&0xFFFF0000);
		tea_encrypt(uid, sizeof(uid), key, emb_iteration);
		sign_flag++;
	}
	// 计算 CRC
	crc16 = 0;
#ifdef FAST_CRC16
	crc16 = fast_crc16(crc16, (const unsigned char*)(0x08000000+0x0400), 1024*32); // 32K 代码校验
#else
	crc16 = fw_crc(crc16, (const unsigned char*)(0x08000000+0x0400), 1024*32); // 32K 代码校验
#endif
	crc = crc16;
	memcpy(&sign[0], uid, sizeof(uid));
	MX_USART3_UART_Init();
	/*app_debug("[%s--%d] crc:0x%08X sign_flag:%d \r\n", __func__, __LINE__, crc, sign_flag);
	app_debug("[%s--%d] sign: \r\n", __func__, __LINE__);
	for(i=0; i<8; i++) app_debug("0x%08X \r\n", sign[i]);
	app_debug("\r\n");*/
	return crc;
}
// 验签代码
void verify_chip(void)
{
	uint32_t sign[8];
	uint32_t crc;
	int led;
	const struct Emb_Device_Version*const _Emb_version = (const struct Emb_Device_Version*)0x08000200;
	// 签名
	crc = sign_chip(sign);
	//app_debug("[%s--%d] uid: \r\n", __func__, __LINE__);
	app_debug("[%s--%d] sign_flag:%d \r\n", __func__, __LINE__, sign_flag);
	app_debug("[%s--%d] crc:0x%08X 0x%08X \r\n", __func__, __LINE__, _Emb_version->crc, crc);
	for(int i=0; i<8; i++) app_debug("[%s--%d] signData[%d]:0x%08X 0x%08X \r\n", __func__, __LINE__, i, _Emb_version->signData[i], sign[i]);
	if((16==sign_flag) && (_Emb_version->crc==crc) && (0==memcmp(sign, _Emb_version->signData, sizeof(_Emb_version->signData))))
	{
		app_debug("[%s--%d] verify_chip pass! \r\n", __func__, __LINE__);
		//bl_entry();
		return ;
	}
	//app_debug("[%s--%d] uid: \r\n", __func__, __LINE__);
	// 加密验证错误, 错误提示:3s快闪,3s慢闪
	led_tick = 0;
	MX_GPIO_Init();
	while(1)  // nop
	{
		// 使用";"会出现 warning: this 'for' clause does not guard... [-Wmisleading-indentation]
		//asm("mov r0,r0");
		// 刷入固件前有 3s 快闪提示
		for(led=0; led<60; led++) // 3s
		{
			HAL_Delay(50);
			LL_GPIO_TogglePin(GPIOD, LED_GREEN_Pin);
		}
		for(led=0; led<6; led++)  // 3s
		{
			HAL_Delay(500);
			LL_GPIO_TogglePin(GPIOD, LED_GREEN_Pin);
		}
	}
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

	//int conut=0;
	//SCB->VTOR = g_pfnVectors;//FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
	SCB->VTOR = 0x08010000UL ;
	//SCB->VTOR = 0x20000000UL; /* Vector Table Relocation in Internal FLASH */
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

  HAL_Delay(500);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
	USART1_Init(115200);
	USART2_Init(115200);
	USART3_Init(115200);
	led_tick = HAL_GetTick() + 200;
	app_debug("[%s--%d] Ver[%d | 0x%08X]:%s\r\n", __func__, __LINE__, sizeof(Emb_Version), &Emb_Version, Emb_Version.version);
#if 0
	app_debug("[%s--%d] HAL_OK       = 0x00U, \r\n", __func__, __LINE__);
	app_debug("[%s--%d] HAL_ERROR    = 0x01U, \r\n", __func__, __LINE__);
	app_debug("[%s--%d] HAL_BUSY     = 0x02U, \r\n", __func__, __LINE__);
	app_debug("[%s--%d] HAL_TIMEOUT  = 0x03U, \r\n", __func__, __LINE__);
	app_debug("[%s--%d] FLASH_Erase[0x%08X-0x%08X]:%d\r\n", __func__, __LINE__, 0x08000000, 0x08000FFF, FLASH_Erase(0x08000000, 0x08000FFF));
#endif
	//verify_chip();
	//EC20_Test();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  //app_debug("[%s--%d] conut:%d\r\n", __func__, __LINE__, conut++);
	  HAL_Delay(500);
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
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);

  if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_5)
  {
  Error_Handler();  
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {
    
  }
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_8, 168, LL_RCC_PLLP_DIV_2);
  LL_RCC_PLL_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_8, 168, LL_RCC_PLLQ_DIV_7);
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {
    
  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  
  }
  LL_SetSystemCoreClock(168000000);

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
