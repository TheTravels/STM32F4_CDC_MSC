/**
  ******************************************************************************
  * File Name          : USART.c
  * Description        : This file provides code for the configuration
  *                      of the USART instances.
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

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include "usart.h"
#include "uart.h"
#include "version.h"

/* USER CODE BEGIN 0 */
extern uint8_t vbus_connect;

cache_queue UART1_RX_cache;
cache_queue UART2_RX_cache;
cache_queue UART3_RX_cache;
cache_queue cdc_RX_cache;

//__STATIC_INLINE
static int usart_send(USART_TypeDef *USARTx, const uint8_t data[], const uint32_t _size)
{
	uint8_t Value;
	uint32_t len;
	for(len=0; len<_size; len++)
	{
#if (USE_TIMEOUT == 1)
		Timeout = USART_SEND_TIMEOUT_TXE_MS;
#endif /* USE_TIMEOUT */

		/* Wait for TXE flag to be raised */
		while (!LL_USART_IsActiveFlag_TXE(USARTx))
		{
#if (USE_TIMEOUT == 1)
			/* Check Systick counter flag to decrement the time-out value */
			if (LL_SYSTICK_IsActiveCounterFlag())
			{
				if(Timeout-- == 0)
				{
					/* Time-out occurred. Set LED to blinking mode */
					LED_Blinking(LED_BLINK_SLOW);
				}
			}
#endif /* USE_TIMEOUT */
		}

		/* If last char to be sent, clear TC flag */
		/*if (ubSend == (sizeof(aStringToSend) - 1))
		{
		  LL_USART_ClearFlag_TC(USARTx);
		}*/

		/* Write character in Transmit Data register.
		   TXE flag is cleared by writing data in DR register */
		Value = data[len];
		LL_USART_TransmitData8(USARTx, Value);
	}
	LL_USART_ClearFlag_TC(USARTx);
	return _size;
}

void uartx_queue_init(void)
{
    init_queue(&UART1_RX_cache);
    init_queue(&UART2_RX_cache);
    init_queue(&UART3_RX_cache);
    init_queue(&cdc_RX_cache);
}

int uart1_send(const uint8_t data[], const uint32_t _size)
{
	return usart_send(USART1, data, _size);
}
int uart1_read(uint8_t buf[], const uint32_t _size)
{
	//return cache_queue_reads(&Debug_RX_cache, buf, _size);
	uint16_t index;
	//__asm("CPSID  I");
	macro_queue_read(index, buf, _size, UART1_RX_cache);
	//__asm("CPSIE  I");
	return index;
}
int uart1_size(void)
{
	return cache_queue_size(&UART1_RX_cache);
}
int uart1_isempty(void)
{
	return cache_queue_isempty(&UART1_RX_cache);
}
int uart2_send(const uint8_t data[], const uint32_t _size)
{
	return usart_send(USART2, data, _size);
}
int uart2_read(uint8_t buf[], const uint32_t _size)
{
	//return cache_queue_reads(&Debug_RX_cache, buf, _size);
	uint16_t index;
	//__asm("CPSID  I");
	macro_queue_read(index, buf, _size, UART2_RX_cache);
	//__asm("CPSIE  I");
	return index;
}
int uart3_send(const uint8_t data[], const uint32_t _size)
{
	return usart_send(USART3, data, _size);
}
int uart3_read(uint8_t buf[], const uint32_t _size)
{
	//return cache_queue_reads(&Debug_RX_cache, buf, _size);
	uint16_t index;
	//__asm("CPSID  I");
	macro_queue_read(index, buf, _size, UART3_RX_cache);
	//__asm("CPSIE  I");
	return index;
}
#include "usb_device.h"
extern uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
int cdc_send(const uint8_t data[], const uint32_t _size)
{
	//return usart_send(USART3, data, _size);
	if(USBD_OK==CDC_Transmit_FS(data, _size)) return _size;
	return 0;
}
int cdc_read(uint8_t buf[], const uint32_t _size)
{
	//return cache_queue_reads(&Debug_RX_cache, buf, _size);
	uint16_t index;
	//__asm("CPSID  I");
	macro_queue_read(index, buf, _size, cdc_RX_cache);
	//__asm("CPSIE  I");
	return index;
}
//static _ccm char debug_text[1024*2];
static char debug_text[1024*2];
int app_debug(const char *__format, ...)
{
	//char debug_text[512];
	va_list ap;
    // 判断日志级别
    //if(LOG_LEVEL_DEBUG<_log_level) return -1;
	memset(debug_text, 0, sizeof(debug_text));
	va_start(ap, __format);
	//vprintf(__format, ap);
	//snprintf(text, sizeof (text), __format, ap);
	//vsprintf(debug_text, __format, ap);
	vsnprintf(debug_text, sizeof(debug_text)-1, __format, ap);
	va_end(ap);
	if(EMB_DEBUG_UART1==Emb_Version.cfg.debug) uart1_send((uint8_t*)debug_text, strlen(debug_text));
	else if(EMB_DEBUG_UART2==Emb_Version.cfg.debug) uart2_send((uint8_t*)debug_text, strlen(debug_text));
	else if(EMB_DEBUG_UART3==Emb_Version.cfg.debug) uart3_send((uint8_t*)debug_text, strlen(debug_text));
	else if((1==vbus_connect) && (EMB_DEBUG_CDC==Emb_Version.cfg.debug)) cdc_send((uint8_t*)debug_text, strlen(debug_text));
	//_serial_debug->write(_serial_debug, 0, "\r\n", 2);
	return 0;
}

/* USER CODE END 0 */

/* USART1 init function */

void USART1_Init(const uint32_t BaudRate)
{
  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_USART_DisableIT_RXNE(USART1);
  LL_USART_DisableIT_ERROR(USART1);
  NVIC_DisableIRQ(USART1_IRQn);
  LL_USART_DeInit(USART1);

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
  
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
  /**USART1 GPIO Configuration  
  PB6   ------> USART1_TX
  PB7   ------> USART1_RX 
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_6|LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USART1 interrupt Init */
  NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(USART1_IRQn);

  USART_InitStruct.BaudRate = BaudRate;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART1, &USART_InitStruct);
  LL_USART_ConfigAsyncMode(USART1);
  LL_USART_Enable(USART1);
  LL_USART_EnableIT_RXNE(USART1);
  LL_USART_EnableIT_ERROR(USART1);

}
/* USART2 init function */

void USART2_Init(const uint32_t BaudRate)
{
  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_USART_DisableIT_RXNE(USART2);
  LL_USART_DisableIT_ERROR(USART2);
  NVIC_DisableIRQ(USART2_IRQn);
  LL_USART_DeInit(USART2);

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
  
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  /**USART2 GPIO Configuration  
  PA2   ------> USART2_TX
  PA3   ------> USART2_RX 
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_2|LL_GPIO_PIN_3;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USART2 interrupt Init */
  NVIC_SetPriority(USART2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(USART2_IRQn);

  USART_InitStruct.BaudRate = BaudRate;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART2, &USART_InitStruct);
  LL_USART_ConfigAsyncMode(USART2);
  LL_USART_Enable(USART2);
  LL_USART_EnableIT_RXNE(USART2);
  LL_USART_EnableIT_ERROR(USART2);

}
/* USART3 init function */

void USART3_Init(const uint32_t BaudRate)
{
  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_USART_DisableIT_RXNE(USART3);
  LL_USART_DisableIT_ERROR(USART3);
  NVIC_DisableIRQ(USART3_IRQn);
  LL_USART_DeInit(USART3);

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
  
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
  /**USART3 GPIO Configuration  
  PB10   ------> USART3_TX
  PB11   ------> USART3_RX 
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_10|LL_GPIO_PIN_11;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USART3 interrupt Init */
  NVIC_SetPriority(USART3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(USART3_IRQn);

  USART_InitStruct.BaudRate = BaudRate;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART3, &USART_InitStruct);
  LL_USART_ConfigAsyncMode(USART3);
  LL_USART_Enable(USART3);
  LL_USART_EnableIT_RXNE(USART3);
  LL_USART_EnableIT_ERROR(USART3);

}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
