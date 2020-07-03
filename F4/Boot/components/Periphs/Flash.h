/******************** (C) COPYRIGHT 2015 merafour ********************
* File Name          : Files.h
* Author             : 冷月追风 && Merafour
* Version            : V1.0.0
* Last Modified Date : 07/03/2020
* Description        : STM32F4 片内 Flash 操作接口.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
*******************************************************************************/
#ifndef __FLASH_H__
#define __FLASH_H__	 

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Flash Base Addr
#define STM32_FLASH_BASE 0x08000000 	
extern int Flash_Write(uint32_t const WriteAddr,uint32_t const *pBuffer,uint32_t const NumToWrite);
extern int Flash_Read(uint32_t ReadAddr,uint32_t *pBuffer,uint32_t NumToRead);
extern int FLASH_Erase(const uint32_t start_addr, const uint32_t end_addr);
extern int Flash_Test(const uint32_t EraseAddr, const uint32_t WriteAddr);

#ifdef __cplusplus
}
#endif
#endif // __FLASH_H__
