/************************ (C) COPYLEFT 2018 Merafour *************************
* File Name          : Ini.h
* Author             : Merafour
* Last Modified Date : 04/05/2020
* Description        : 配置文件解析.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
******************************************************************************/
#ifndef __INI__
#define __INI__

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>

#ifdef __cplusplus
 extern "C" {
#endif

// 解析配置文件
/* ------------------------------------------------------- 读配置信息 ------------------------------------------------------------- */                                      // 黑名单
struct Ini_Parse {
    char path[64];
    char *const text;  // 文件内容
    const int _bsize;        // buf 大小
    int pos;                 // 当前文件指针位置
    int _dsize;              // data 大小
};

extern int Ini_init(struct Ini_Parse* const Ini, char *const text, const int _bsize);

extern int Ini_sample(struct Ini_Parse* const Ini);
// 创建空模板
extern int Ini_list_empty(struct Ini_Parse* const Ini);
// 更新一项数据,不存在时则添加
extern int Ini_update(struct Ini_Parse* const Ini, const char AppName[], const char KeyName[], const char *__format, ...);
// 删除一项数据
extern int Ini_del(struct Ini_Parse* const Ini, const char AppName[], const char KeyName[]);
// 刷新数据到文件
extern int Ini_fflush(const struct Ini_Parse* const Ini);
// 从文件加载数据
extern int Ini_load(struct Ini_Parse* const Ini);
extern int Ini_GetProfileString(struct Ini_Parse* const Ini, const char AppName[], const char KeyName[], char KeyVal[]);
// 返回一个区间偏移量
extern int Ini_GetSection(struct Ini_Parse* const Ini, const char AppName[], int* const _pos);
extern int Ini_GetSection_Key(struct Ini_Parse* const Ini, const char AppName[], const char KeyName[], int* const _pos);
// 获取缓存大小
static inline int Ini_Size(const struct Ini_Parse* const Ini){ return Ini->_dsize;}
// 插入一项数据
extern int Ini_del_item(struct Ini_Parse* const Ini, const int seek);
extern int Ini_print(struct Ini_Parse* const Ini, const char *__format, ...);
static inline int Ini_Path(struct Ini_Parse* const Ini, const char _path[])
{
    memset(Ini->path, 0, sizeof(Ini->path));
    memcpy(Ini->path, _path, strlen(_path));
    return 0;
}
extern int Ini_get_field(struct Ini_Parse* const Ini, const char section[], const char _key[], const char dft[], char _value[]);
extern int Ini_get_int(struct Ini_Parse* const Ini, const char section[], const char _key[], const int dft);
//extern double Ini_get_double(struct Ini_Parse* const Ini, const char section[], const char _key[], const double dft);

#ifdef __cplusplus
}
#endif

#endif  // __INI__
