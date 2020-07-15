/************************ (C) COPYLEFT 2018 Merafour *************************
* File Name          : IniParser.c
* Author             : Merafour
* Last Modified Date : 04/05/2020
* Description        : 配置文件解析.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
******************************************************************************/
#include "Ini.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include "Files.h"
#include "Periphs/uart.h"
enum keylen{ KEYVALLEN = 256};
#define CFGPATH "./cfg.data"

/*   删除左边的空格   */
static char * l_trim(char * szOutput, const char *szInput)
{
//    assert(szInput != NULL);
//    assert(szOutput != NULL);
//    assert(szOutput != szInput);
    for(; *szInput != '\0' && isspace(*szInput); ++szInput){
        ;
    }
    return strcpy(szOutput, szInput);
}

/*   删除右边的空格   */
static char *r_trim(char *szOutput, const char *szInput)
{
    char *p = NULL;
//    assert(szInput != NULL);
//    assert(szOutput != NULL);
//    assert(szOutput != szInput);
    strcpy(szOutput, szInput);
    for(p = szOutput + strlen(szOutput) - 1; p >= szOutput && isspace(*p); --p){
        ;
    }
    *(++p) = '\0';
    return szOutput;
}

/*   删除两边的空格   */
static char * a_trim(char * szOutput, const char * szInput)
{
    char *p = NULL;
//    assert(szInput != NULL);
//    assert(szOutput != NULL);
    l_trim(szOutput, szInput);
    for(p = szOutput + strlen(szOutput) - 1;p >= szOutput && isspace(*p); --p){
        ;
    }
    *(++p) = '\0';
    return szOutput;
}
/*   获取一行   */
static char *serach_gets(struct Ini_Parse* const Ini, char * const str, const int n)
{
    int _end;
    int count=0;
    char _byte=0;
    _end = Ini->pos + n;
    memset(str, 0, (size_t)n);
    for(count=0; count<n-1; count++)
    {
        if(Ini->pos>=Ini->_dsize) return  NULL;
        if(Ini->pos>=_end) return  NULL;
        _byte = Ini->text[Ini->pos++];
        str[count] = _byte;
        if('\n'==_byte) break;
        if('\0'==_byte) break;
    }
    //str[count] = '\0';
    str[n-1] = '\0';
    /*if(0==count)
    {
        printf("[%s-%d] stream->pos:%d stream->_stext:%d _end:%d _byte:0x%X\n", __func__, __LINE__, stream->pos, stream->_stext, _end, _byte);
        return  NULL;
    }*/
    return  str;
}
// 跳到下一行
static int next_gets(struct Ini_Parse* const Ini, const int _seek)
{
    char _byte=0;
    int _pos;                 // 当前文件指针位置
    for(_pos=_seek; _pos<Ini->_bsize; _pos++)
    {
        _byte = Ini->text[_pos];
        if('\n'==_byte)
        {
            _pos++;   // skip  '\n'
            break;
        }
        if('\0'==_byte) break;
    }
    return  _pos;
}
// 跳到前一行
static int previ_gets(struct Ini_Parse* const Ini, const int _seek)
{
    char _byte=0;
    int _pos;                 // 当前文件指针位置
    for(_pos=_seek; _pos>=0; _pos--)
    {
        _byte = Ini->text[_pos];
        if('\n'==_byte)
        {
            _pos++;   // skip  '\n'
            break;
        }
        if('\0'==_byte)
        {
            _pos++;   // skip  '\0'
            break;
        }
    }
    return  _pos;
}
/*   在当前位置插入一行数据   */
// const int _bsize;        // buf 大小
static int _insert(struct Ini_Parse* const Ini, const int seek, const char item[])
{
    int _size;
    int i;
    // move item
    _size = (int)strlen(item);
    //printf("[%s-%d] Item[%d]:%s", __func__, __LINE__, _size, buf);
    // 所有数据向后移动 _size byte
    for(i=Ini->_bsize-1; i>=(seek+_size); i--)
    {
        Ini->text[i] = Ini->text[i-_size];
    }
    // insert curr pos
    memcpy(&Ini->text[seek], item, _size);
    Ini->_dsize = (int)strlen(Ini->text);
    return 0;
}
/*   格式化插入一行数据   */
static int insert(struct Ini_Parse* const Ini, const int seek, const char *__format, ...)
{
    char buf[256];
    //int _size;
    //int i;
    va_list ap;
    memset(buf, 0, sizeof(buf));
    va_start(ap, __format);
    //vprintf(__format, ap);
    //snprintf(prt, __n-_size, __format, ap);
    vsprintf(buf, __format, ap);
    va_end(ap);
#if 0
    // move item
    _size = strlen(buf);
    //printf("[%s-%d] Item[%d]:%s", __func__, __LINE__, _size, buf);
    // 所有数据向后移动 _size byte
    for(i=Size()-1; i>=(seek+_size); i--)
    {
        Info[i] = Info[i-_size];
    }
    // insert curr pos
    memcpy(&Info[seek], buf, _size);
    _isize = strlen(Info);
#endif
    _insert(Ini, seek, buf);
    return 0;
}
/*   获取一段信息   */
int Ini_get_field(struct Ini_Parse* const Ini, const char section[], const char _key[], const char dft[], char _value[])
{
    int dft_size = (int)strlen(dft);
    //printf("[%s-%d] \n", __func__, __LINE__);
    int ret = Ini_GetProfileString(Ini, section, _key, _value);
    if(0!=ret)
    {
        memcpy(_value, dft, dft_size);
        _value[dft_size] = '\0';
    }
    return 0;
}
/*   获取一一个整数   */
int Ini_get_int(struct Ini_Parse* const Ini, const char section[], const char _key[], const int dft)
{
    char _value[512];
    char _dft[32];
    int value=dft;
    memset(_value, 0, sizeof(_value));
    memset(_dft, 0, sizeof(_dft));
    snprintf(_dft, sizeof(_dft)-1, "%8d", dft);
    int ret = Ini_get_field(Ini, section, _key, _dft, _value);
    if(0==ret)
    {
        // char to int
        value = atoi(_value);
    }
    return value;
}
/*   获取一一个double数据   */
#if 0
double Ini_get_double(struct Ini_Parse* const Ini, const char section[], const char _key[], const double dft)
{
    char _value[512];
    char _dft[32];
    double value=dft;
    memset(_value, 0, sizeof(_value));
    memset(_dft, 0, sizeof(_dft));
    snprintf(_dft, sizeof(_dft)-1, "%8f", dft);
    int ret = Ini_get_field(Ini, section, _key, _dft, _value);
    if(0==ret)
    {
        // char to int
        value = atof(_value);
    }
    return value;
}
#endif
int Ini_sample(struct Ini_Parse* const Ini)
{
    Ini_print(Ini, "[Ini]\r\n");
    Ini_print(Ini, "[End]\r\n");
    return 0;
}
/*   初始化样例   */
int Ini_list_empty(struct Ini_Parse* const Ini)
{
    Ini_print(Ini, "# Ini 配置文件 \r\n");
    Ini_print(Ini, "# Version:	1.7.12 \r\n");
    Ini_print(Ini, "# '#' 后面的内容为注释 \r\n");
    Ini_print(Ini, "# [EG1] 表示 EG1 段的开始,到下一个段之前结束 \r\n");
    Ini_print(Ini, "# Key=Val 表示键值对,且键值对必须在段里面否则不解析 \r\n");
    Ini_print(Ini, "# 以下为示例\r\n");
    Ini_print(Ini, "# \r\n");
    Ini_print(Ini, "\r\n# 日志选项 \r\n");
    Ini_print(Ini, "# no:关闭,yes:开启 \r\n");
    Ini_print(Ini, "[EG1_LOG] \r\n");
    Ini_print(Ini, "TurnOn=no \r\n");
    Ini_print(Ini, "\r\n# 服务器配置 \r\n");
    Ini_print(Ini, "[EG2_Host] \r\n");
    Ini_print(Ini, "Server=127.0.0.1 \r\n");
    Ini_print(Ini, "Port=8888 \r\n");
    Ini_print(Ini, "\r\n\r\n");
    Ini_print(Ini, "[End] \r\n");
    Ini_print(Ini, "# 文件结束 \r\n");
    return 0;
}
/*   获取一个 Key对应的值   */
int Ini_GetProfileString(struct Ini_Parse* const Ini, const char AppName[], const char KeyName[], char KeyVal[])
{
    char appname[32],keyname[32];
    char *buf,*c;
    char buf_i[KEYVALLEN], buf_o[KEYVALLEN];
    int found=0; /* 1 AppName 2 KeyName */
    memset( appname, 0, sizeof(appname) );
    sprintf( appname,"[%s]", AppName );

    Ini->pos = 0;
    //printf("[%s-%d] pos:%d size:%d \n", __func__, __LINE__, pos, size);
    while( (Ini->pos<Ini->_dsize) && serach_gets(Ini, buf_i, KEYVALLEN)!=NULL )
    {
        // 读取一行
        //printf("[%s-%d] pos:%d size:%d buf_i:%s \n", __func__, __LINE__, pos, size, buf_i);
        l_trim(buf_o, buf_i);
        if( strlen(buf_o) <= 0 )
            continue;
        buf = NULL;
        buf = buf_o;

        //printf("[%s-%d] appname:%s fp.pos:%d found:%d buf:%s\n", __func__, __LINE__, appname, fp.pos, found, buf);
        if( found == 0 )
        {
            if( buf[0] != '[' )
            {
                continue;
            }
            else if ( strncmp(buf,appname,strlen(appname))==0 )
            {
                found = 1;
                continue;
            }
        }
        else if( found == 1 )
        {
            if('#'==buf[0]) // if( buf[0] == '#' )
            {
                continue;
            }
            // add
            //else if(0!=is_empty_line(buf, KEYVALLEN)) continue;
            else if ( buf[0] == '[' )
            {
                break;
            }
            else
            {
                if( (c = (char*)strchr(buf, '=')) == NULL )
                    continue;
                memset( keyname, 0, sizeof(keyname) );

                sscanf( buf, "%[^=|^ |^\t]", keyname );
                if( strcmp(keyname, KeyName) == 0 )
                {
                    sscanf( ++c, "%[^\n]", KeyVal );
                    //char *KeyVal_o = (char *)malloc(strlen(KeyVal) + 1);
                    int _size = (int)strlen(KeyVal) + 1;
                    char *KeyVal_o = (char *)malloc(_size);
                    if(KeyVal_o != NULL)
                    {
                        //memset(KeyVal_o, 0, sizeof(KeyVal_o));
                        memset(KeyVal_o, 0, _size);
                        a_trim(KeyVal_o, KeyVal);
                        if(KeyVal_o && strlen(KeyVal_o) > 0)
                            strcpy(KeyVal, KeyVal_o);
                        free(KeyVal_o);
                        KeyVal_o = NULL;
                    }
                    found = 2;
                    break;
                }
                else
                {
                    continue;
                }
            }
        }
    }
    //printf("[%s-%d] \n", __func__, __LINE__);
    if( found == 2 ) return(0);
    else return(-1);
}
/*   获取一个段的地址   */
int Ini_GetSection(struct Ini_Parse* const Ini, const char AppName[], int* const _pos)
{
    char appname[64];
    char *buf;//,*c;
    char buf_i[KEYVALLEN], buf_o[KEYVALLEN];
    int found=0; /* 1 AppName 2 KeyName */
    memset( appname, 0, sizeof(appname) );
    snprintf( appname, sizeof(appname)-1,"[%s]", AppName );

    Ini->pos = 0;
    //printf("[%s-%d] pos:%d size:%d \n", __func__, __LINE__, pos, size);
    while( (Ini->pos<Ini->_dsize) && serach_gets(Ini, buf_i, KEYVALLEN)!=NULL )
    {
        // 读取一行
        //printf("[%s-%d] pos:%d size:%d buf_i:%s \n", __func__, __LINE__, pos, size, buf_i);
        l_trim(buf_o, buf_i);
        if( strlen(buf_o) <= 0 )
            continue;
        buf = NULL;
        buf = buf_o;

        //printf("[%s-%d] appname:%s fp.pos:%d found:%d buf:%s\n", __func__, __LINE__, appname, fp.pos, found, buf);
        if( found == 0 )
        {
            if( buf[0] != '[' )
            {
                continue;
            }
            else if ( strncmp(buf,appname,strlen(appname))==0 )
            {
                found = 1;
                //printf("\r\n\r\n[%s-%d] text[%d]:\n%s\n", __func__, __LINE__, pos, &text[pos]);
                //continue;
                break;
            }
        }
        else if( found == 1 )
        {
            if('#'==buf[0]) // if( buf[0] == '#' )
            {
                continue;
            }
            //printf("\r\n[%s-%d] text[%d]:\n%s\n", __func__, __LINE__, pos, &text[pos]);
            break;
        }
    }
    //printf("[%s-%d] \n", __func__, __LINE__);
    *_pos = Ini->pos;
    if( found == 1 ) return 0;
    else return -1;
    //return pos;
}
/*   获取一个键值对的地址   */
int Ini_GetSection_Key(struct Ini_Parse* const Ini, const char AppName[], const char KeyName[], int* const _pos)
{
    char appname[32],keyname[32];
    char *buf,*c;
    char buf_i[KEYVALLEN], buf_o[KEYVALLEN];
    int found=0; /* 1 AppName 2 KeyName */
    memset( appname, 0, sizeof(appname) );
    sprintf( appname,"[%s]", AppName );

    Ini->pos = 0;
    //printf("[%s-%d] pos:%d size:%d \n", __func__, __LINE__, pos, size);
    while( (Ini->pos<Ini->_dsize) && serach_gets(Ini, buf_i, KEYVALLEN)!=NULL )
    {
        // 读取一行
        //printf("[%s-%d] pos:%d size:%d buf_i:%s \n", __func__, __LINE__, pos, size, buf_i);
        l_trim(buf_o, buf_i);
        if( strlen(buf_o) <= 0 )
            continue;
        buf = NULL;
        buf = buf_o;

        //printf("[%s-%d] appname:%s fp.pos:%d found:%d buf:%s\n", __func__, __LINE__, appname, fp.pos, found, buf);
        if( found == 0 )
        {
            if( buf[0] != '[' )
            {
                continue;
            }
            else if ( strncmp(buf,appname,strlen(appname))==0 )
            {
                found = 1;
                continue;
            }
        }
        else if( found == 1 )
        {
            if('#'==buf[0]) // if( buf[0] == '#' )
            {
                continue;
            }
            // add
            //else if(0!=is_empty_line(buf, KEYVALLEN)) continue;
            else if ( buf[0] == '[' )
            {
                break;
            }
            else
            {
                if( (c = (char*)strchr(buf, '=')) == NULL )
                    continue;
                memset( keyname, 0, sizeof(keyname) );

                sscanf( buf, "%[^=|^ |^\t]", keyname );
                if( strcmp(keyname, KeyName) == 0 )
                {
                    found = 2;
                    break;
                }
                else
                {
                    continue;
                }
            }
        }
    }
    *_pos = Ini->pos;
    //return pos;
    if( found == 2 ) return 0;
    else return -1;
}
/*   刷新数据到文件   */
int Ini_fflush(const struct Ini_Parse* const Ini)
{
    //printf("\nIni:%s\r\n", Info); fflush(stdout);
    //printf("[%s-%d] Ini: %s \r\n", __func__, __LINE__, Ini->path);
    file_save(Ini->path, Ini->text, Ini->_dsize);
    return 0;
}
/*   从文件加载数据   */
int Ini_load(struct Ini_Parse* const Ini)
{
    long _size = file_read(Ini->path, Ini->text, Ini->_bsize);
    app_debug("[%s-%d] path[%s] _isize[%d] : \n%s\n", __func__, __LINE__, Ini->path, Ini->_dsize, Ini->text);
    //app_debug("[%s-%d] path[%s] _isize[%d] \r\n", __func__, __LINE__, Ini->path, Ini->_dsize);
    if(_size<=0)
    {
        Ini->_dsize=0;
        return -1;
    }
    Ini->_dsize=(int)_size;
    return 0;
}
/*   更新一个键值对，没有则创建   */
int Ini_update(struct Ini_Parse* const Ini, const char AppName[], const char KeyName[], const char *__format, ...)
{
    int _seek;
    char buf[256];
    va_list ap;
    memset(buf, 0, sizeof(buf));
    va_start(ap, __format);
    //vprintf(__format, ap);
    //snprintf(prt, __n-_size, __format, ap);
    //vsprintf(buf, __format, ap);
    vsnprintf(buf, sizeof(buf)-1, __format, ap);
    va_end(ap);
    //printf("[%s-%d] AppName[%s] KeyName[%s] _seek[%d] _isize[%d] \n", __func__, __LINE__, AppName, KeyName, _seek, Ini->_dsize);
    if(0!=Ini_GetSection(Ini, AppName, &_seek))  // 添加新的段
    {
        if(0!=Ini_GetSection(Ini, "End", &_seek)) return -2;
        _seek = previ_gets(Ini, _seek-3);  // skip curr line
        insert(Ini, _seek, "\r\n[%s]\r\n", AppName);  // 在文件末尾插入
        _seek = next_gets(Ini, _seek+3);
        _insert(Ini, _seek, buf);
    }
    else
    {
        if(0==Ini_GetSection_Key(Ini, AppName, KeyName, &_seek)) // found
        {
            _seek = previ_gets(Ini, _seek-3);  // skip curr line
            //printf("[%s-%d] AppName[%s] KeyName[%s] _seek[%d]\n", __func__, __LINE__, AppName, KeyName, _seek);
            Ini_del_item(Ini, _seek);
            _insert(Ini, _seek, buf);
        }
        else // new
        {
            if(0!=Ini_GetSection(Ini, AppName, &_seek)) return -2;
            _insert(Ini, _seek, buf);
        }
    }
    //printf("[%s-%d]\r\n", __func__, __LINE__);
    return 0;
}
/*   删除一个键值对   */
int Ini_del(struct Ini_Parse* const Ini, const char AppName[], const char KeyName[])
{
    int _seek;
    // 添加新的段
    if(0!=Ini_GetSection(Ini, AppName, &_seek)) return 0;
    if(0==Ini_GetSection_Key(Ini, AppName, KeyName, &_seek)) // found
    {
        _seek = previ_gets(Ini, _seek-3);  // skip curr line
        //printf("[%s-%d] AppName[%s] KeyName[%s] _seek[%d]\n", __func__, __LINE__, AppName, KeyName, _seek);
        Ini_del_item(Ini, _seek);
    }
    //printf("[%s-%d]\r\n", __func__, __LINE__);
    return 0;
}
/*   删除一行数据   */
int Ini_del_item(struct Ini_Parse* const Ini, const int seek)
{
    int i;
    int _pos = next_gets(Ini, seek);
    int _size = _pos - seek;
    // 所有数据向前移动 _size byte
    for(i=seek; i<(Ini->_bsize-_size); i++)
    {
        Ini->text[i] = Ini->text[i+_size];
    }
    for(i=Ini->_bsize-_size; i<Ini->_bsize; i++)
    {
        Ini->text[i] = '\0';
    }
    //printf("[%s-%d] del_item[%d]:\r\n%s", __func__, __LINE__, _size, &Info[seek]);
    return 0;
}
/*   输出一行信息到 Ini   */
int Ini_print(struct Ini_Parse* const Ini, const char *__format, ...)
{
    char* prt=NULL;
    const size_t __n = Ini->_bsize;
    size_t _size=0;
    va_list ap;
    _size = strlen(Ini->text);
    if(_size>=__n) return -1;
    prt = &Ini->text[_size];
    va_start(ap, __format);
    //vprintf(__format, ap);
    //snprintf(prt, __n-_size, __format, ap);
    vsprintf(prt, __format, ap);
    va_end(ap);
    Ini->_dsize = (int)strlen(Ini->text);
    return 0;
}

int Ini_init(struct Ini_Parse* const _ini, char *const text, const int _bsize)
{
    struct Ini_Parse Ini = {
                .path = "Param.Ini",
                .text = text,
                ._bsize = _bsize,
                .pos = 0,
                ._dsize = 0,
    };
    memcpy(_ini, &Ini, sizeof(struct Ini_Parse));
    return 0;
}

