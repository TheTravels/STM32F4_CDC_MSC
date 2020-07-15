/******************** (C) COPYRIGHT 2015 merafour ********************
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
extern void boot_app(void);
extern int ParamTable_quality(void);
extern uint16_t ParamTable_Size(void);
extern int ParamTable_Write(const void *const Param, const uint32_t _size);
extern int ParamTable_Read(void *const Param, const int _quality, const uint32_t _size);
// block 为字节数
extern uint32_t param_write_erase(void);
extern uint16_t param_write_flash(const uint8_t buf[], const uint32_t seek, const uint16_t block);
extern uint16_t param_read_flash(uint8_t buf[], const uint32_t seek, const uint16_t block);

extern uint16_t param_write_key(uint32_t _key[]);
extern uint16_t param_read_key(uint32_t _key[]);

union param_app_data{
	uint32_t data[200/4];
	struct {
		uint32_t lock; // 锁机标志, 仅为服务器端锁机标志的映射
		char date[16]; // 注册时间, 仅为服务器端设备注册时间的映射
	};
};

extern const uint32_t param_flash_size;       // App 大小
extern const uint32_t param_flash_start;      // App 地址
extern const uint32_t param_download_start;   // App 下载区地址
extern const uint32_t param_download_size;    // App 下载区大小

#ifdef __cplusplus
}
#endif
#endif // __PARAM_TABLE_H__
