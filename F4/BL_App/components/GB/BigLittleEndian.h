/************************ (C) COPYLEFT 2018 Merafour *************************

* File Name          : BigLittleEndian.h
* Author             : Merafour
* Last Modified Date : 01/14/2020
* Description        : 数据大小端处理.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
******************************************************************************/
#ifndef _BIG_LITTLE_ENDIAN_H_
#define _BIG_LITTLE_ENDIAN_H_

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

/* 大端存储 */
// 大端编码 encode,返回数据长度
extern uint8_t bigend16_encode(uint8_t buf[], const uint16_t data);
extern uint8_t bigend32_encode(uint8_t buf[], const uint32_t data);
extern uint16_t array32_encode(const uint32_t array[], uint8_t buf[], const uint8_t len);
// 大端合并 merge,返回数据
extern uint16_t __bigend16_merge(const uint8_t byte0, const uint8_t byte1);
extern uint32_t __bigend32_merge(const uint8_t byte0, const uint8_t byte1, const uint8_t byte2, const uint8_t byte3);
// 大端合并 merge,返回字节数
extern uint8_t bigend16_merge(uint16_t* const value, const uint8_t byte0, const uint8_t byte1);
extern uint8_t bigend32_merge(uint32_t* const value, const uint8_t byte0, const uint8_t byte1, const uint8_t byte2, const uint8_t byte3);
extern uint16_t array32_merge(uint32_t array[], const uint8_t _data[], const uint8_t len);

#ifdef __cplusplus
}
#endif


#endif // _BIG_LITTLE_ENDIAN_H_
