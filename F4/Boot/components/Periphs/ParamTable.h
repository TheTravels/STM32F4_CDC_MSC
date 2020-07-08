﻿/******************** (C) COPYRIGHT 2015 merafour ********************
* File Name          : Param Table.h
* Author             : 冷月追风 && Merafour
* Version            : V1.0.0
* Last Modified Date : 07/03/2020
* Description        : STM32F4 片内 Flash 操作接口.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
*******************************************************************************/
#ifndef __PARAM_TABLE_H__
#define __PARAM_TABLE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Flash Base Addr
//#define STM32_PARAM_BASE 0x0800C000
extern uint16_t ParamTable_Size(void);
extern int ParamTable_Write(const void *const Param, const uint32_t _size);
extern int ParamTable_Read(void *const Param, const uint32_t _size);

extern uint16_t param_write_flash(const uint8_t buf[], const uint32_t seek, const uint16_t block);
extern uint16_t param_read_flash(uint8_t buf[], const uint32_t seek, const uint16_t block);

extern uint16_t param_write_key(uint32_t _key[]);
extern uint16_t param_read_key(uint32_t _key[]);

extern const uint32_t param_flash_size;    // App 大小
extern const uint32_t param_flash_start;      // App 地址

#ifdef __cplusplus
}
#endif
#endif // __PARAM_TABLE_H__
