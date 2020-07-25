/******************** (C) COPYRIGHT 2015 merafour ********************
* File Name          : crc.h
* Author             : 冷月追风 && Merafour
* Version            : V1.0.0
* Last Modified Date : 07/03/2020
* Description        : CRC 计算.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
*******************************************************************************/
#ifndef __CRC_H__
#define __CRC_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FAST_CRC16     1
#undef  FAST_CRC16

#define SLOW_CRC16     1
#undef  SLOW_CRC16

#define SLOW_CRC32     1
//#undef  SLOW_CRC32

// 计算 CRC
#ifdef FAST_CRC16
extern unsigned short fast_crc16(unsigned short sum, const unsigned char *p, unsigned int len);
#endif

#ifdef SLOW_CRC16
extern unsigned short slow_crc16(unsigned short sum, const unsigned char *p, unsigned int len);
#endif

#ifdef SLOW_CRC32
extern uint32_t px4_bl_crc32(const uint8_t *src, unsigned len, unsigned state);
#endif

extern uint32_t fw_crc(const uint32_t sum, const unsigned char *p, const unsigned int len);

#ifdef __cplusplus
}
#endif
#endif // __CRC_H__
