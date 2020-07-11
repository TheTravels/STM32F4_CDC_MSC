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
#include "tea.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define  TEA_TEST    1
#undef   TEA_TEST

#ifdef   TEA_TEST
#define pr_debug(fmt, ...) printf(fmt, ##__VA_ARGS__); fflush(stdout);
static void print_hex(const char _data[], const uint32_t _dsize)
{
    uint32_t _size = 0;
    pr_debug("HEX:");
    for (_size = 0; _size < _dsize; _size++) pr_debug("%02X ", _data[_size]&0xFF);
    pr_debug("\n");
}
#else
#define pr_debug(fmt, ...) ;
#endif

//加密函数,iteration迭代次数
static void __tea_encrypt(uint32_t* v, const uint32_t* const key, const uint32_t _iteration)
{
    uint32_t v0 = v[0], v1 = v[1], sum = 0, i;           /* set up */
    const uint32_t delta = 0x9e3779b9;                     /* a key schedule constant */
    uint32_t k0 = key[0], k1 = key[1], k2 = key[2], k3 = key[3];   /* cache key */
    for (i = 0; i < _iteration; i++)  // for (i = 0; i < 32; i++)
    {                       /* basic cycle start */
        sum += delta;
        v0 += ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        v1 += ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
    }                                              /* end cycle */
    v[0] = v0; v[1] = v1;
}
uint32_t tea_encrypt(void* const _data, const uint32_t _dsize, const uint32_t* const key, const uint32_t _iteration)
{
    uint32_t dsize = 0;// _dsize;
    uint32_t* const data = (uint32_t*)_data;
    uint32_t index;
    uint32_t iteration;
    // 64bit/8byte对齐,未对齐部分补齐
    dsize = 0;
    if ((_dsize & 0x07) > 0) dsize = 8;
    dsize = dsize + (_dsize & 0xFFFFFFF8);// dsize = (dsize & 0xFFFFFFF8) + 8;
    //dsize = dsize >> 2;
    //printf("[%s--] size[%d %d]\n", __func__, _dsize, dsize);
#if 0
    // 迭代次数必须是32的倍数
    iteration = 0;
    if ((_iteration & 0x1F) > 0) iteration = 32;
    iteration = iteration + (_iteration & 0xFFFFFFE0);
#else
    iteration = _iteration;
#endif
    pr_debug("[%s--%d] iteration[%d] size[%d]\n", __func__, __LINE__, iteration, dsize);
    for (index = 0; index < (dsize >> 2); index+=2)
    {
        __tea_encrypt(&data[index], key, iteration);
    }
#ifdef TEA_TEST
    print_hex((char*)data, dsize);
#endif
    return dsize;
}
//解密函数
static void __tea_decrypt(uint32_t* v, const uint32_t* const key, const uint32_t _iteration)
{
    uint32_t v0 = v[0], v1 = v[1], sum = 0xC6EF3720, i;  /* set up 0xC6EF3720:32轮迭代*/
    const uint32_t delta = 0x9e3779b9;                     /* a key schedule constant */
    uint32_t k0 = key[0], k1 = key[1], k2 = key[2], k3 = key[3];   /* cache key */
    sum = 0;
    for (i = 0; i < _iteration; i++)  // for (i = 0; i < 32; i++)
    {                         /* basic cycle start */
        sum += delta;
    }
    for (i = 0; i < _iteration; i++)  // for (i = 0; i < 32; i++)
    {                         /* basic cycle start */
        v1 -= ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
        v0 -= ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        sum -= delta;
    }                                              /* end cycle */
    v[0] = v0; v[1] = v1;
}
void tea_decrypt(void* const _data, const uint32_t _dsize, const uint32_t* const key, const uint32_t _iteration)
{
    uint32_t dsize = _dsize;
    uint32_t* const data = (uint32_t*)_data;
    uint32_t index;
    uint32_t iteration;
    // 64bit/8byte对齐,未对齐部分舍弃
    dsize = dsize & 0xFFFFFFF8;
    //dsize = dsize >> 2;
    pr_debug("[%s--] size[%d %d]\n", __func__, _dsize, dsize);
#ifdef TEA_TEST
    print_hex((char*)data, dsize);
#endif
#if 0
    // 迭代次数必须是32的倍数
    iteration = 0;
    if ((_iteration & 0x1F) > 0) iteration = 32;
    iteration = iteration + (_iteration & 0xFFFFFFE0);
#else
    iteration = _iteration;
#endif
    pr_debug("[%s--%d] iteration[%d] size[%d]\n", __func__, __LINE__, iteration, dsize);
    for (index = 0; index < (dsize >> 2); index += 2)
    {
        __tea_decrypt(&data[index], key, iteration);
    }
}

/*
 * 加密密钥交换
 * _key:随机密钥
 * def_key:固定密钥
 * enc_key:加密密钥
 */
const uint32_t tea_swap_iteration = 128;
void tea_swap_key(uint32_t _key[4], const uint32_t def_key[4], const uint32_t enc_key[4])
{
    uint32_t tea_key[4]={0x01020304, 0x05060708, 0x090A0B0C, 0x0D0E0F10};
    //const uint32_t _iteration = 64;
    int i;
    pr_debug("[%s--%d] _key \t[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, _key[0], _key[1], _key[2], _key[3]);
    // 更新密钥
    for(i=0; i<4; i++) tea_key[i] = _key[i] + def_key[i];
    pr_debug("[%s--%d] tea_key \t[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, tea_key[0], tea_key[1], tea_key[2], tea_key[3]);
    // 对密钥进行加密
    tea_encrypt(tea_key, sizeof(tea_key), enc_key, tea_swap_iteration);
    memcpy(_key, tea_key, sizeof(tea_key));   // 交换密钥
#if 0
    printf("[%s--%d] _key2 \t[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, _key[0], _key[1], _key[2], _key[3]);
    // 解密测试
    tea_decrypt(tea_key, sizeof(tea_key), enc_key, tea_swap_iteration);
    printf("[%s--%d] tea_key2 \t[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, tea_key[0], tea_key[1], tea_key[2], tea_key[3]);
    for(i=0; i<4; i++) tea_key[i] = tea_key[i] - def_key[i];
    printf("[%s--%d] tea_key3 \t[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, tea_key[0], tea_key[1], tea_key[2], tea_key[3]);
#endif
}
/*
 * 加密密钥合并
 * _rand_key:随机密钥
 * _sub_key:子密钥，通常位对方发过来的密钥
 */
void _swap_key(uint32_t _rand_key[4], const uint32_t _sub_key[4])
{
    uint32_t tea_key[4]={0x01020304, 0x05060708, 0x090A0B0C, 0x0D0E0F10};
    int i;
    pr_debug("[%s--%d] _rand_key \t[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, _rand_key[0], _rand_key[1], _rand_key[2], _rand_key[3]);
    // 更新密钥
    for(i=0; i<4; i++) tea_key[i] = _rand_key[i] + _sub_key[i];
    pr_debug("[%s--%d] tea_key \t[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, tea_key[0], tea_key[1], tea_key[2], tea_key[3]);
    memcpy(_rand_key, tea_key, sizeof(tea_key));   // 交换密钥
}
static uint32_t st_rand(void)   // ST随机数硬件
{
      //uint32_t data;
      //while(RNG_GetFlagStatus(RNG_FLAG_DRDY) == RESET);  //等待随机数准备完毕
      //data = RNG_GetRandomNumber();   //读数
      //return data;
      return 0x12345678;
}
void tea_rand(uint32_t _rand[], const uint32_t _size)  // 获取随机数
{
    uint32_t i;
    int key;
    //RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG,ENABLE);
    //RNG_Cmd(ENABLE);
    for(i=0; i<_size; i++)
    {
        key = st_rand() + st_rand();//+time(NULL);
        key = (key<<16) + st_rand();//+time(NULL);
        key = (key<<4) + st_rand();
        _rand[i] = key;
    }
    //RNG_DeInit(); // 关闭随机数硬件
}
#if 0
#include <stdio.h>
static void __tea_test(void* const _data, const uint32_t _dsize, const uint32_t _key[], const uint32_t _iteration)
{
    uint32_t _decrypt[128];
    uint32_t _dlen;

    memset(_decrypt, 0, sizeof(_decrypt));
    printf("\r\n_iteration[%d] _data[%d]:[%s]\r\n", _iteration, _dsize, (char*)_data);
    _dlen = tea_encrypt(_data, _dsize, _key, _iteration);
    memcpy(_decrypt, _data, _dsize);
    tea_decrypt(_decrypt, _dlen, _key, _iteration);
    printf("tea_encrypt[%d]:_dsize[%d] _dlen[%d]\r\n", _iteration, _dsize, _dlen);
    printf("tea_decrypt[%d]:[%s]\r\n", _iteration, (char*)_decrypt);
    fflush(stdout);
}
void tea_test(void)   // 加密解密测试
{
    const uint32_t tea_key[4]={0x01020304, 0x05060708, 0x090A0B0C, 0x0D0E0F10};
    uint32_t buffer[8];
    uint32_t _iteration;

    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, "0123456789ABCDEF0123456789ABCDEF", sizeof (buffer)-1);
    _iteration = 64;
#if 0
    for(_iteration=1; _iteration<256; _iteration++)
    {
        char* const _data = (char*)buffer;
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, "0123456789ABCDEF0123456789ABCDEF", sizeof (buffer)-1);
        _data[11] = '0'+((_iteration/100)%10);
        _data[12] = '0'+((_iteration/10)%10);
        _data[13] = '0'+(_iteration%10);
        __tea_test(buffer, sizeof(buffer), tea_key, _iteration);
    }
#else
    __tea_test(buffer, sizeof(buffer), tea_key, _iteration);
#endif
}

void tea_swap_test(void)   // 加密密钥交换测试
{
    const uint32_t def_key[4]={0x64F7A758, 0x2F113A3D, 0x5DD74DC1, 0x6782269E};
    const uint32_t enc_key[4]={0x2C480B95, 0x93B04D96, 0x62939B2F, 0xC7E6A328};
    uint32_t rand_key[4];  // 随机密钥
    uint32_t test_key[4];  // 测试密钥
    uint32_t buf_key[4];   //
    const uint32_t _iteration = 64;
    //int i;
    /*int key;
    for(i=0; i<4; i++)
    {
        key = rand() + rand();//+time(NULL);
        key = (key<<16) + rand();//+time(NULL);
        key = (key<<4) + rand();
        rand_key[i] = key;
    }
    for(i=0; i<4; i++)
    {
        key = rand() + rand();//+time(NULL);
        key = (key<<16) + rand();//+time(NULL);
        key = (key<<4) + rand();
        test_key[i] = key;
    }*/
    tea_rand(rand_key, 4);
    tea_rand(test_key, 4);

    // buf_key
    printf("\r\n[%s--%d] rand_key \t[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, rand_key[0], rand_key[1], rand_key[2], rand_key[3]);
    printf("[%s--%d] test_key \t[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, test_key[0], test_key[1], test_key[2], test_key[3]);
    printf("[%s--%d] def_key \t[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, def_key[0], def_key[1], def_key[2], def_key[3]);
    printf("[%s--%d] enc_key \t[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, enc_key[0], def_key[1], enc_key[2], enc_key[3]);
    //for(i=0; i<4; i++) buf_key[i] = rand_key[i] + test_key[i] + def_key[i] ;
    memset(buf_key, 0, sizeof(buf_key));
    _swap_key(buf_key, rand_key);
    _swap_key(buf_key, test_key);
    _swap_key(buf_key, def_key);
    printf("[%s--%d] buf_key \t[0x%08X 0x%08X 0x%08X 0x%08X]\r\n\r\n", __func__, __LINE__, buf_key[0], buf_key[1], buf_key[2], buf_key[3]);
    // rand_key
    memset(buf_key, 0, sizeof(buf_key));
    memcpy(buf_key, rand_key, sizeof(rand_key));
    tea_swap_key(buf_key, def_key, enc_key);
    tea_decrypt(buf_key, sizeof(buf_key), enc_key, _iteration);
    _swap_key(buf_key, test_key);//for(i=0; i<4; i++) buf_key[i] = buf_key[i] + test_key[i];
    printf("[%s--%d] rand_key-\t[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, buf_key[0], buf_key[1], buf_key[2], buf_key[3]);
    // test_key
    memset(buf_key, 0, sizeof(buf_key));
    memcpy(buf_key, test_key, sizeof(test_key));
    tea_swap_key(buf_key, def_key, enc_key);
    tea_decrypt(buf_key, sizeof(buf_key), enc_key, _iteration);
    _swap_key(buf_key, rand_key);//for(i=0; i<4; i++) buf_key[i] = buf_key[i] + rand_key[i];
    printf("[%s--%d] test_key-\t[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, buf_key[0], buf_key[1], buf_key[2], buf_key[3]);

    fflush(stdout);
}
#endif
