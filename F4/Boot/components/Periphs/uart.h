/**
  ******************************************************************************
  * File Name          : USART.h
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef ___UART_H
#define ___UART_H
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "cache_queue.h"

/* USER CODE BEGIN Prototypes */

extern cache_queue UART1_RX_cache;
extern cache_queue UART2_RX_cache;
extern cache_queue UART3_RX_cache;
extern cache_queue cdc_RX_cache;

static __inline void add_queue_uart1(const uint8_t _byte)
{
	//__asm("CPSID  I");
	//cache_queue_write(&UART5_RX_cache, _byte);
	// 宏展开
	//macro_queue_write(_byte,Debug_RX_cache);
	// 内联函数
	cache_queue_write(&UART1_RX_cache, _byte);
	//__asm("CPSIE  I");
}
static __inline void add_queue_uart2(const uint8_t _byte)
{
	//__asm("CPSID  I");
	macro_queue_write(_byte,UART2_RX_cache);
	//__asm("CPSIE  I");
}
static __inline void add_queue_uart3(const uint8_t _byte)
{
	//__asm("CPSID  I");
	macro_queue_write(_byte,UART3_RX_cache);
	//__asm("CPSIE  I");
}
static __inline void add_queue_cdc(const uint8_t _byte)
{
	//__asm("CPSID  I");
	macro_queue_write(_byte,cdc_RX_cache);
	//__asm("CPSIE  I");
}

/*_____________________________________________________________ 调试接口 ________________________________________________________________*/
//extern int qDebug(const char *__format, ...);
extern int app_debug(const char *__format, ...);

extern void uartx_queue_init(void);
/*_____________________________________________________________ UART1 ________________________________________________________________*/
extern int uart1_send(const uint8_t data[], const uint32_t _size);
extern int uart1_read(uint8_t buf[], const uint32_t _size);
extern int uart1_size(void);
extern int uart1_isempty(void);
/*_____________________________________________________________ UART2 ________________________________________________________________*/
extern int uart2_send(const uint8_t data[], const uint32_t _size);
extern int uart2_read(uint8_t buf[], const uint32_t _size);
/*_____________________________________________________________ UART3 ________________________________________________________________*/
extern int uart3_send(const uint8_t data[], const uint32_t _size);
extern int uart3_read(uint8_t buf[], const uint32_t _size);
/*_____________________________________________________________ UART3 ________________________________________________________________*/
extern int cdc_send(const uint8_t data[], const uint32_t _size);
extern int cdc_read(uint8_t buf[], const uint32_t _size);

extern void USART1_Init(const uint32_t BaudRate);
extern void USART2_Init(const uint32_t BaudRate);
extern void USART3_Init(const uint32_t BaudRate);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ ___UART_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
