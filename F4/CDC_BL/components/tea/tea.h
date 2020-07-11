/************************ (C) COPYLEFT 2018 Merafour *************************
* File Name          : tea.h
* Author             : Merafour
* Last Modified Date : 03/05/2020
* Description        : TEA加密算法.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
******************************************************************************/
#ifndef __TEA_H__
#define __TEA_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
 extern uint32_t tea_encrypt(void* const _data, const uint32_t _dsize, const uint32_t* const key, const uint32_t _iteration);
 extern void tea_decrypt(void* const _data, const uint32_t _dsize, const uint32_t* const key, const uint32_t _iteration);
 extern void tea_swap_key(uint32_t _key[4], const uint32_t def_key[4], const uint32_t enc_key[4]);
 extern void _swap_key(uint32_t _rand_key[4], const uint32_t _sub_key[4]);
 extern void tea_rand(uint32_t _rand[], const uint32_t _size);  // 获取随机数
 extern void tea_test(void);
 extern void tea_swap_test(void);

 extern const uint32_t tea_swap_iteration;

#ifdef __cplusplus
 }
#endif

#endif /* __TEA_H__ */

