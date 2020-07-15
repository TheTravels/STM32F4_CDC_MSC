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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include "Ini.h"
#include "Files.h"

int _Ini_Test_read(struct Ini_Parse* const Ini)
{
    char vinList[64];
    Ini_load(Ini);
    memset(vinList, 0, sizeof(vinList));
    Ini_GetProfileString(Ini, "LocalCFG", "vinList", vinList);
    printf("[%s-%d] vinList<%s> \n", __func__, __LINE__, vinList);
    memset(vinList, 0, sizeof(vinList));
    Ini_get_field(Ini, "LocalCFG", "vinList", "hello", vinList);
    printf("[%s-%d] vinList<%s> \n", __func__, __LINE__, vinList);
    memset(vinList, 0, sizeof(vinList));
    Ini_get_field(Ini, "LocalCFG", "Ip", "hello", vinList);
    printf("[%s-%d] vinList<%s> \n", __func__, __LINE__, vinList);
    printf("[%s-%d] Port<%d> \n", __func__, __LINE__, Ini_get_int(Ini, "LocalCFG", "Port", 110));
    return 0;
}
int _Ini_Test_write(struct Ini_Parse* const Ini)
{
    Ini_Path(Ini, "./cfg.data");
    Ini_update(Ini, "AppName", "KeyName", "%s=\"%s\" \r\n", "KeyName", "0x12345678");
    Ini_fflush(Ini);
    return 0;
}

void Ini_Test(void)
{
    char buf[1024*10];
    struct Ini_Parse Ini = {
                .path = "ServerConfig.cfg",
                .text = buf,
                ._bsize = sizeof(buf),
                .pos = 0,
                ._dsize = 0,
    };
    _Ini_Test_read(&Ini);
    _Ini_Test_write(&Ini);
    fflush(stdout);
}
