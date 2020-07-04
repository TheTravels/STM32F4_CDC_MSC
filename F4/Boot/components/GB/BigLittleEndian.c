/************************ (C) COPYLEFT 2018 Merafour *************************

* File Name          : BigLittleEndian.c
* Author             : Merafour
* Last Modified Date : 01/14/2020
* Description        : 数据大小端处理.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
******************************************************************************/
#include "BigLittleEndian.h"

/* 取对应字节 */
#define BYTE0(data)    (data&0xFF)
#define BYTE1(data)    ((data>>8)&0xFF)
#define BYTE2(data)    ((data>>16)&0xFF)
#define BYTE3(data)    ((data>>24)&0xFF)

// 大端编码 encode,返回数据长度
uint8_t bigend16_encode(uint8_t buf[], const uint16_t data)
{
    buf[0] = BYTE1(data);
    buf[1] = BYTE0(data);
    return 2;
}
uint8_t bigend32_encode(uint8_t buf[], const uint32_t data)
{
    buf[0] = BYTE3(data);
    buf[1] = BYTE2(data);
    buf[2] = BYTE1(data);
    buf[3] = BYTE0(data);
    return 4;
}
uint16_t array32_encode(uint8_t buf[], const uint32_t data[], const uint8_t len)
{
    uint16_t index=0, i;
    index=0;
    for(i=0; i<len; i++) index += bigend32_encode(&buf[index], data[i]);
    return index; // len
}

// 大端合并 merge,返回数据
uint16_t __bigend16_merge(const uint8_t byte0, const uint8_t byte1)
{
    uint16_t data=0;
    data = byte0;
    data = (data<<8) | byte1;
    return data;
}
uint32_t __bigend32_merge(const uint8_t byte0, const uint8_t byte1, const uint8_t byte2, const uint8_t byte3)
{
    uint32_t data=0;
    data = byte0;
    data = data<<8 | byte1;
    data = data<<8 | byte2;
    data = data<<8 | byte3;
    return data;
}
// 大端合并 merge,返回字节数
uint8_t bigend16_merge(uint16_t* const value, const uint8_t byte0, const uint8_t byte1)
{
    uint16_t data=0;
    data = byte0;
    data = (data<<8) | byte1;
    *value = data;
    return 2;
}
uint8_t bigend32_merge(uint32_t* const value, const uint8_t byte0, const uint8_t byte1, const uint8_t byte2, const uint8_t byte3)
{
    uint32_t data=0;
    data = byte0;
    data = data<<8 | byte1;
    data = data<<8 | byte2;
    data = data<<8 | byte3;
    *value = data;
    return 4;
}
uint16_t array32_merge(uint32_t array[], const uint8_t data[], const uint8_t len)
{
    uint16_t index=0, i;
    index=0;
    for(i=0; i<len; i++) index += bigend32_merge(&array[i], data[index], data[index+1], data[index+2], data[index+3]);
    return index; // len
}


