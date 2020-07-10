/******************** (C) COPYRIGHT 2015 merafour ********************
* File Name          : at.c
* Author             : 冷月追风 && Merafour
* Version            : V1.0.0
* Last Modified Date : 07/10/2020
* Description        : AT指令处理.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "at.h"
#include "Periphs/uart.h"
#include "stm32f4xx_hal.h"

// 接收缓存
//static uint8_t _ccm __attribute__ ((aligned (4))) at_data[1024*4];

// AT 设备发送命令
int at_print(char *fmt, ...)
{
    int len;
    char buf[256];
    va_list args;

    memset(buf, 0, sizeof(buf));
    va_start (args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end (args);

    //清空串口缓冲
    //rt_device_control(uart,RT_DEVICE_CTRL_CLEAR_RX,0);
    //rt_device_write(uart, 0, buf, len);
    uart1_send((uint8_t*)buf, len);
    app_debug("[%s-%d] AT --> [%s] \r\n", __func__, __LINE__, buf);
    return len;
}
// 获取 AT 指令响应, 3个 resp 满足大多数需求
int at_get_resps(const char resp[], const char resp_ok[], const char resp_err[], const uint32_t _timeout, const uint16_t dt, struct at_data* const _at)
{
	uint32_t timeout;
	int rlen;
	int len;
	//int i;
	uint16_t index;
    const char* const resps[3] = {resp, resp_ok, resp_err};
    // 读取模块响应
    memset(_at, 0, sizeof(struct at_data));
    rlen = 0;
    for(timeout=0; timeout<_timeout; timeout+=dt)
    {
    	// 读取数据
    	HAL_Delay(dt);
    	// int uart1_read(uint8_t buf[], const uint32_t _size)
    	rlen += uart1_read((uint8_t*)&_at->_rbuf[rlen], sizeof(_at->_rbuf)-rlen);
    	// 使用编译器知道大小的缓存,避免由于存在安全风险而优化掉数据拷贝操作
//    	memset(at_data, 0, sizeof(at_data));
//    	len = uart1_read(at_data, sizeof(at_data));
//    	for(i=0; i<len; i++)
//    	{
//    		if(rlen+i<_rsize) _rbuf[rlen+i] = at_data[i];
//    	}
//    	rlen += len;
    	//app_debug("[%s-%d] AT<-- <%d--%d | %s> \r\n", __func__, __LINE__, sizeof(_at->_rbuf), rlen, _at->_rbuf);
    	//app_debug("[%s-%d] _w:%d _r:%d buf:%s data:%s \r\n", __func__, __LINE__, UART1_RX_cache.index_w, UART1_RX_cache.index_r, UART1_RX_cache.buf, _rbuf);
    	//uart3_send((const uint8_t)_rbuf, rlen);
    	for(index=0; index<3; index++)
    	{
    		if(NULL==resps[index]) continue;
            if(NULL != strstr((const char *)_at->_rbuf, resps[index]))
            {
            	int count;
            	// 400ms 用于接收为接收完的数据
            	for(count=0; count<20; count++)
            	{
            		HAL_Delay(20);
            		len = uart1_read((uint8_t*)&_at->_rbuf[rlen], sizeof(_at->_rbuf)-rlen);
//                	memset(at_data, 0, sizeof(at_data));
//                	len = uart1_read(at_data, sizeof(at_data));
            		if(len<=0) break;
//                	for(i=0; i<len; i++)
//                	{
//                		if(rlen+i<_rsize) _rbuf[rlen+i] = at_data[i];
//                	}
                	rlen += len;
            	}
            	app_debug("[%s-%d] AT<-- [%s] \r\n", __func__, __LINE__, _at->_rbuf);
            	_at->_rsize = rlen;
                return index;
            }
    	}
    }
    app_debug("[%s-%d] AT<-- [%s] \r\n", __func__, __LINE__, _at->_rbuf);
    _at->_rsize = rlen;
    return -2;
}
// 获取数据
int at_get_datas(const char resp[], const char resp_err[], const uint32_t timeout, char _rbuf[], const uint16_t _rsize)
{
}
/*******************************************************************************
 * 功能:搜索字符串 src 中 separator 分隔的第 nFieldNum 段数据
 * src:字符串
 * dest:缓存
 * separator:分隔符
 * nFieldNum:第几个段,从1开始
 * nMaxFieldLen:buf长度
 *******************************************************************************/
uint8_t at_get_str_split(const char src[], char dest[], const char separator, const uint8_t nFieldNum, const uint8_t nMaxFieldLen)
{
     uint8_t index=0;
     uint8_t pos=0;
     uint16_t i;
     dest[0] = '\0';
     for(i=0; '\0'!=src[i]; i++)
     {
        //if(separator==src[i]|| '\r'==src[i])
        if(separator==src[i])
        {
            index++;
            if(nFieldNum==index){
                dest[pos]=0;
                return pos;
            }
            pos=0;
            continue;
        }
        if(pos<nMaxFieldLen-1){
            dest[pos++]=src[i];
        }
     }
     dest[pos]=0;
     return pos;
}
/*******************************************************************************
 * 功能:搜索字符串 src 中 resp 后面的数据,以换行结尾
 * resp:字符串
 * dest:缓存
 * _dsize:dest大小
 *******************************************************************************/
int at_get_resp_split(const char src[], const char resp[], char dest[], const uint8_t _dsize)
{
	uint8_t len;
    const char* data=strstr(src, resp);
    if(NULL==data) return 0;
    data += strlen(resp);
    len = at_get_str_split(data, dest, '\n', 1, _dsize);
    return len;
}
/*******************************************************************************
 * 功能:搜索字符串 src 中 resp 后面的数据,以换行结尾
 * resp:字符串
 * val:缓存
 * separator:分隔符
 * nFieldNum:第几个段,从1开始
 *******************************************************************************/
uint8_t at_get_resp_split_int(const char src[], const char resp[], int* const val, const char separator, const uint8_t nFieldNum)
{
    const char *Resp = NULL;
    char dest[128];
    uint8_t len;

    memset(dest, 0, sizeof(dest));
    Resp = strstr(src, resp);
    if(NULL==Resp) return 0;
    Resp += strlen(resp);
    len = at_get_str_split(Resp, dest, separator, nFieldNum, sizeof(dest));
    if(len>0)
    {
        *val = atoi(dest);
        return len;
    }
    return 0;
}



/************************ (C) COPYRIGHT Merafour *****END OF FILE****/

