/******************** (C) COPYRIGHT 2015 merafour ********************
* File Name          : at.h
* Author             : 冷月追风 && Merafour
* Version            : V1.0.0
* Last Modified Date : 07/10/2020
* Description        : AT指令处理.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
*******************************************************************************/
#ifndef __AT_H__
#define __AT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 使用数据结构避免 gcc 将数据拷贝关键操作优化掉
struct at_ofps{
	// UART 操作接口函数
	const int (* const uart_send)(const uint8_t data[], const uint32_t _size);
	const int (* const uart_read)(uint8_t buf[], const uint32_t _size);
	const int (* const uart_size)(void);
	const int (* const uart_is_empty)(void);
	char __attribute__ ((aligned (4))) _rbuf[1024*4];   // 缓存
	uint16_t _rsize;         // 缓存中数据大小
};

extern void at_ofps_init(struct at_ofps* const _at);

/*******************************************************************************
 * fmt:格式化输出
 *******************************************************************************/
extern int at_print(char *fmt, ...);
//extern int at_print_resp(const char resp[], const char resp_ok[], const char resp_err[], const uint32_t timeout, char *const _rbuf, char *fmt, ...);
/*******************************************************************************
 * resp:响应匹配字符串
 * _timeout:c超时时间  ms
 * dt:超时检测时间间隔 ms
 * _at:接收缓存
 *******************************************************************************/
extern int at_get_resps(const char resp[], const char resp_ok[], const char resp_err[], const uint32_t _timeout, const uint16_t dt, struct at_ofps* const _at);
extern int at_get_resp4s(const char resp1[], const char resp2[], const char resp3[], const char resp4[], const uint32_t _timeout, const uint16_t dt, struct at_ofps* const _at);

extern const char* at_get_data(const char _data[], const char _resp[]);
/*******************************************************************************
 * 功能:搜索字符串 src 中 separator 分隔的第 nFieldNum 段数据
 * src:字符串
 * dest缓存
 * separator:分隔符
 * nFieldNum:第几个段,从1开始
 * nMaxFieldLen:buf长度
 *******************************************************************************/
extern uint8_t at_get_str_split(const char src[], char dest[], const char separator, const uint8_t nFieldNum, const uint8_t nMaxFieldLen);
/*******************************************************************************
 * 功能:搜索字符串 src 中 resp 后面的数据,以换行结尾
 * resp:字符串
 * dest:缓存
 * _dsize:dest大小
 *******************************************************************************/
extern int at_get_resp_split(const char src[], const char resp[], char dest[], const uint8_t _dsize);
/*******************************************************************************
 * 功能:搜索字符串 src 中 resp 后面的数据,以换行结尾
 * resp:字符串
 * val:缓存
 * separator:分隔符
 * nFieldNum:第几个段,从1开始
 *******************************************************************************/
extern uint8_t at_get_resp_split_int(const char src[], const char resp[], int* const val, const char separator, const uint8_t nFieldNum);
/*******************************************************************************
 * 功能:搜索字符串 src 中 resp 后面的参数,以换行结尾
 * resp:字符串
 * args:参数
 * val:缓存
 *******************************************************************************/
extern uint8_t at_get_resp_args_int(const char src[], const char resp[], const char args[], int* const val);


#ifdef __cplusplus
}
#endif

#endif /* __EC20_H__ */

/************************ (C) COPYRIGHT Merafour *****END OF FILE****/

