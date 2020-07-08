/******************** (C) COPYRIGHT 2015 merafour ********************
* File Name          : Param Table.c
* Author             : 冷月追风 && Merafour
* Version            : V1.0.0
* Last Modified Date : 07/03/2020
* Description        : STM32F4 片内 Flash 操作接口.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
*******************************************************************************/
#include <string.h>
#include "ParamTable.h"
#include "Flash.h"
#include "version.h"
#define __ParamTable_Size    (256)
// 用于对齐
static char __attribute__ ((aligned (4))) param_buf[__ParamTable_Size] = \
"[Param]\r\n"\
"SN=0A0CK90N4123\r\n"\
"Host=120.78.134.213\r\n"\
"port=20008\r\n"\
"FTPH=120.78.134.213\r\n"\
"FTPP=21\r\n"\
"Time=2020.08.08\r\n"\
"[End]\r\n";

uint16_t ParamTable_Size(void)
{
	return __ParamTable_Size;
}
int ParamTable_Write(const void *const Param, const uint32_t _size)
{
	uint32_t size = _size;
	if(size>__ParamTable_Size) size=__ParamTable_Size;
	memcpy(param_buf, Param, size);
	return size;
}
int ParamTable_Read(void *const Param, const uint32_t _size)
{
	uint32_t size = _size;
	if(size>__ParamTable_Size) size=__ParamTable_Size;
	memcpy(Param, param_buf, size);
	return size;
}

//#define FLASH_SECTOR_ADDR_MAP     ((uint32_t)0x08040000) 	// 128 Kbytes
//#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) 	// 128 Kbytes
uint16_t param_write_flash(const uint8_t buf[], const uint32_t seek, const uint16_t block)
{
	return 0;
}
uint16_t param_read_flash(uint8_t buf[], const uint32_t seek, const uint16_t block)
{
	const char* const flash_addr_start = (const char*)(0x08040000+seek);
	const char* const flash_addr_end = (const char*)0x08080000;
	if((flash_addr_start+block)<flash_addr_end)
	{
		memcpy(buf, flash_addr_start, block);
		return block;
	}
	else if(flash_addr_start<flash_addr_end)
	{
		const uint16_t len = flash_addr_end - flash_addr_start;
		memcpy(buf, flash_addr_start, len);
		return len;
	}
	return 0;
}

static const int param_key_len = 8;
uint16_t param_write_key(uint32_t _key[])
{
	int i;
	// 用于对齐
	uint32_t Key[16];
	const struct Emb_Device_Version* const _Version = (const struct Emb_Device_Version*)0x08000000;
	for(i=0; i<param_key_len; i++)
	{
		if(0xFFFFFFFF!=_Version->mtext[i]) break;
	}
	if(i<param_key_len) return 0;
	memcpy(Key, _key, sizeof(_Version->mtext));
	// 强制写入,不执行擦除
	Flash_Write_Force((const uint32_t)&_Version->mtext, Key, sizeof(_Version->mtext));
	return sizeof(_Version->mtext);
}
uint16_t param_read_key(uint32_t _key[])
{
	int i;
	const struct Emb_Device_Version* const _Version = (const struct Emb_Device_Version*)0x08000000;
	for(i=0; i<param_key_len; i++) _key[i] = _Version->mtext[i];
	return sizeof(_Version->mtext);
}



