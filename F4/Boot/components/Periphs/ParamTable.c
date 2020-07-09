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
#include "Periphs/uart.h"
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

//#define FLASH_SECTOR_ADDR_MAP     ((uint32_t)0x08040000) 	// 128 Kbytes
//#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) 	// 128 Kbytes
const uint32_t param_flash_start = 0x08010000;      // App 地址
const uint32_t param_flash_size = (64+128)*1024;    // App 大小
const uint32_t param_table_start = 0x0800C000;      // 参数表 地址
const uint32_t param_table_size = 16*1024;    // 参数表 大小

static const uint32_t quality_flag = 0xFFFF0000;




/*
 * ParamTable sector
 * 由于 F4 的扇区为：16 + 16 + 16 + 16 + 64 + 128 + 128 + 128 ...
 * 故取第四个扇区 16K 作为参数表的存储地址,为避免频繁擦除带来数据丢失的风险,
 * 这里采用将 16K 分成 256B 的块进行存储,即参数表不能超过 256B 数据,这能够满足设备出厂的关键数据刷写.
 * 参数总的写入次数为 16KB/256B 即 64 次, 64 次之后将会执行在写入之前擦除扇区.
 * 参数表的分区管理如下:
 * Table0    |-----------------| start
 * uint32_t 0| quality_flag    | 0000H
 *          1|                 | 0004H
 *          2|                 | 0008H
 *          3|                 | 000CH
 *        ...| Ini0            | ...H
 *         64|                 | 00FCH
 * Table1    |-----------------| start
 * uint32_t 0| quality_flag    | 0100H
 *          1|                 | 0004H
 *          2|                 | 0008H
 *          3|                 | 000CH
 *        ...| Ini1            | ...H
 *         64|                 | 01FCH
 * Table2    |-----------------| start
 * uint32_t 0| quality_flag    | 0200H
 *          1|                 | 0004H
 *          2|                 | 0008H
 *          3|                 | 000CH
 *        ...| Ini2            | ...H
 *         64|                 | 02FCH
 * Table3 ...|-Ini3 ...--------| start
 * Table63   |-----------------| start
 * uint32_t 0| quality_flag    | 3F00H
 *          1|                 | 3F04H
 *          2|                 | 3F08H
 *          3|                 | 3F0CH
 *        ...| Ini63           | ...H
 *         64|                 | 3FFCH
 *           |-------End-------| 4000H
 * 搜索分区表的原理是从 Table0 到 Table63 进行遍历,第一个全为 0xFF 的分区为可用分区,其前一个分区为最新的数据分区.
 * 因此读取参数表时需要 addr -= __ParamTable_Size;才是最新的数据地址.
 * 所有的参数表操作均基于以上分区表管理机制.
 */

// 搜索参数表中的空白区域
static uint16_t ParamTable_search(void)
{
	uint16_t addr; // 表内地址
	uint16_t _search;
	//uint32_t param[__ParamTable_Size/4];
	const uint32_t* param = NULL;
	for(addr=0; addr<param_table_size; addr+=__ParamTable_Size)
	{
		int i;
		//memset(param, 0, sizeof(param));
		//memcpy(param, (char*)(param_table_start+addr), __ParamTable_Size);
		param = (const uint32_t*)(param_table_start+addr);
		_search = 1;
		for(i=0; i<=(__ParamTable_Size/4); i++)
		{
			if(0xFFFFFFFF!=param[i])
			{
				//app_debug("[%s--%d] 0xFFFFFFFF param:0x%08X addr:0x%04X \r\n", __func__, __LINE__, param[i], addr);
				_search = 0;
				break;
			}
		}
		if(1==_search) return addr;
	}
	//app_debug("[%s--%d] addr:0x%04X \r\n", __func__, __LINE__, addr);
	return param_table_size; // 找不到空白区域
}

int ParamTable_quality(void)
{
	const uint32_t * quality = (const uint32_t *)param_buf;
	uint16_t addr; // 表内地址
	//uint32_t * const quality = (uint32_t *)param_buf;
	// 查找空白区域
	addr = ParamTable_search();
	if(addr>=__ParamTable_Size) // 空闲空间的前一个空间才有数据
	{
		addr -= __ParamTable_Size;
	}
	quality = (const uint32_t *)(param_table_start+addr);
	// 质检标志
	if(quality_flag==(*quality)) return 1;
	return 0;
}

uint16_t ParamTable_Size(void)
{
	return __ParamTable_Size-4;
}
int ParamTable_Write(const void *const Param, const uint32_t _size)
{
	uint32_t size = _size;
	uint16_t addr; // 表内地址
	// 查找空白区域
	addr = ParamTable_search();
	//app_debug("[%s--%d] addr:0x%04X \r\n", __func__, __LINE__, addr);
	if(addr>=param_table_size) // 无空闲空间,擦除扇区
	{
		FLASH_Erase(param_table_start, param_table_start+param_table_size-1);
		addr = 0;
	}
	if(size>(__ParamTable_Size-4)) size=__ParamTable_Size-4;
	//app_debug("[%s--%d] addr:0x%04X size:%d \r\n", __func__, __LINE__, addr, size);
	memset(param_buf, 0xFF, sizeof(param_buf));
	memcpy(param_buf+4, Param, size);
	// 写入 flash
	app_debug("[%s--%d] Flash_Write_Force:0x%08X param_buf:0x%08X \r\n", __func__, __LINE__, param_table_start+addr, param_buf);
	Flash_Write_Force(param_table_start+addr, (const uint32_t *)param_buf, __ParamTable_Size/4);
	return size;
}
// quality:质检标志
int ParamTable_Read(void *const Param, const int _quality, const uint32_t _size)
{
	uint32_t size = _size;
	uint16_t addr; // 表内地址
	//uint32_t * const quality = (uint32_t *)param_buf;
	// 查找空白区域
	addr = ParamTable_search();
	if(addr>=__ParamTable_Size) // 空闲空间的前一个空间才有数据
	{
		addr -= __ParamTable_Size;
	}
	// 检查是否是质检操作
	if(1==_quality)  // 写入质检标志
	{
		//*quality = quality_flag;
		Flash_Write_Force(param_table_start+addr, &quality_flag, 1);  // 32位写入
	}
	if(size>(__ParamTable_Size-4)) size=__ParamTable_Size-4;
	//memcpy(Param, param_buf+4, size);
	memcpy(Param, (const char*)(param_table_start+addr+4), size);
	return size;
}

uint32_t param_write_erase(void)
{
	uint32_t addr;
	const uint32_t* const flash = (const uint32_t*)param_flash_start;
	for(addr=0; addr<(param_flash_size>>2); addr++)
	{
		if(0xFFFFFFFF!=flash[addr])
		{
			// 擦除
			FLASH_Erase(param_flash_start, param_flash_start+param_flash_size-1);
			break;
		}
	}
	return param_flash_size;
}

uint16_t param_write_flash(const uint8_t buf[], const uint32_t seek, const uint16_t block)
{
	if(0==Flash_Write_Force(param_flash_start+seek, (const uint32_t *)buf, block/4)) return block;
	return 0;
}
uint16_t param_read_flash(uint8_t buf[], const uint32_t seek, const uint16_t block)
{
	const char* const flash_addr_start = (const char*)(param_flash_start+seek);
	const char* const flash_addr_end = (const char*)(param_flash_start+param_flash_size);
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



