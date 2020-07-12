/******************** (C) COPYRIGHT 2015 merafour ********************
* File Name          : ec20_ftp_upload.c
* Author             : 冷月追风 && Merafour
* Version            : V1.0.0
* Last Modified Date : 07/10/2020
* Description        : 移远 EC20 模块驱动 - FTP 升级.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
*******************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include "ec20.h"
#include "at.h"
#include "main.h"
#include "Periphs/uart.h"
#include "Periphs/ParamTable.h"
#include "Periphs/crc.h"
#include "stm32f4xx_hal.h"
#include "Ini/Ini.h"

static const char fw_name[] = "Ini"; // "FW";
static const char fw_key_bin[] = "BIN";

static char ini_data[1024*20];
static int ini_size=0;
static void ini_save_seek(const int total, const uint32_t _seek, const char* const data, const uint16_t block)
{
	uint32_t count;
	for(count=0; count<block; count++)
	{
		if((_seek+count)>sizeof(ini_data)) break;
		ini_data[_seek+count] = data[count];
	}
	ini_size = total;
}

void decode_path(char path[], char dir[32], char file[32])
{
    // 解析文件名和路径
	char* str=NULL;
    str = strrchr(path,'\\');
    if(NULL==str)
    {
    	str = strrchr(path,'/');
        if(NULL==str) // 无目录
        {
        	dir[0] = '/';
        	dir[1] = '\0';
        	strcpy(file, path);
        }
        else // 包含目录
        {
        	*str++ = '\0';
        	strcpy(dir, path);
        	strcpy(file, str);
        }
    }
    else // 包含目录
    {
    	*str++ = '\0';
    	strcpy(dir, path);
    	strcpy(file, str);
    }
}
// A800,47EF9CDA,MCU418.bin
// CRC,total,path
int download_firmware(struct ec20_ofps* const _ofps, const char cfg[])
{
    char path[64];
//    char dir[64];
//    char file[64];
    uint32_t crc_ftp;
    uint32_t crc_flash;
    uint32_t total;
    uint8_t len;
    memset(path, 0, sizeof(path));
//    memset(dir, 0, sizeof(dir));
//    memset(file, 0, sizeof(file));
    crc_ftp=0;
    total=0;
	at_get_split_hex(cfg, &total, ',', 1);
	at_get_split_hex(cfg, &crc_ftp, ',', 2);
    len = at_get_str_split(cfg, path, ',', 3, sizeof(path));
    if(len>0)
    {
    	// 计算本地 CRC
    	crc_flash=0;
    	if(total>param_flash_size) total=param_flash_size;
    	crc_flash = fw_crc(crc_flash, (const unsigned char*)(param_flash_start), total);
    	//decode_path(path, dir, file);
    	app_debug("\r\n[%s-%d] path:<%s> \r\n", __func__, __LINE__, path);
    	app_debug("\r\n[%s-%d] crc_ftp[0x%08X] crc_flash[0x%08X] total[0x%04X] \r\n", __func__, __LINE__, crc_ftp, crc_flash, total);
    	if(crc_flash != crc_ftp)
    	{
        	if(EC20_RESP_OK==FTP_DownLoad_RAM(_ofps, "/", path, ini_save_seek))
        	{
        		// 校验,并将新固件写入 flash
        		return 0;
        	}
    	}
    }
    return -1;
}
// FTP升级
int EC20_FTP_Upload(void)
{
    int resp=-1;
    int count;
    struct Ini_Parse Ini = {
        "fw.Ini",
        .text = ini_data,
         ._bsize = sizeof(ini_data),
         .pos = 0,
         ._dsize = 0,
    };
    char bin[64];
    char _sn[64];
    for(count=0; count<1; count++)
    {
    	app_debug("\r\n[%s-%d] EC20_Init ...\r\n", __func__, __LINE__);
    	EC20_Init();
    	app_debug("\r\n[%s-%d] EC20_FTP_Login ...\r\n", __func__, __LINE__);
        resp=EC20_FTP_Login(&_ec20_ofps, _ec20_ofps.NetInfo.NADR, 21, 1, "obd4g", "obd.4g");
        if(EC20_RESP_OK!=resp)
        {
            at_print("AT+QFTPCLOSE\r\n");
            //at_get_resps("\r\nOK", NULL, NULL, NULL, NULL, 100,1500);
            resp = at_get_resps("\r\nOK", NULL, NULL, 1500, 100, &_ec20_ofps._at);
            _ec20_ofps.ModuleState = EC20_MODULE_RESET;
            break;
        }
        app_debug("\r\n[%s-%d] DownLoad Ini ...\r\n", __func__, __LINE__);
        memset(ini_data, 0, sizeof(ini_data));
        ini_size=0;
        resp=FTP_DownLoad_RAM(&_ec20_ofps, "EPS418", "0A0CK90N41.Ini", ini_save_seek);
        if(EC20_RESP_OK!=resp)
        {
            at_print("AT+QFTPCLOSE\r\n");
            resp = at_get_resps("\r\nOK", NULL, NULL, 1500, 100, &_ec20_ofps._at);
            _ec20_ofps.ModuleState = EC20_MODULE_RESET;
            break;
        }
        Ini._dsize = strlen(Ini.text);
        Ini_get_field(&Ini, fw_name, fw_key_bin, "*", bin);
        Ini_get_field(&Ini, fw_name, "23", "*", _sn);
        if('*'!=_sn[0])  // 单独配置
        {
        	// 指定固件
        	if('-'!=_sn[0]) download_firmware(&_ec20_ofps, _sn);
        	else if(NULL==strstr(_sn, ".Ini")) // 指定目录<_sn>下的配置文件
        	{
        	    app_debug("\r\n[%s-%d] path:<%s> \r\n", __func__, __LINE__, _sn);
                memset(ini_data, 0, sizeof(ini_data));
                ini_size=0;
                resp=FTP_DownLoad_RAM(&_ec20_ofps, &_sn[1], "fw.Ini", ini_save_seek);
                Ini._dsize = strlen(Ini.text);
                Ini_get_field(&Ini, fw_name, fw_key_bin, "*", bin);
                Ini_get_field(&Ini, fw_name, "0A0CK90N4123", "*", _sn);
                // 单独配置
                if(('*'!=_sn[0]) && ('-'!=_sn[0])) download_firmware(&_ec20_ofps, _sn);
                else if('*'!=bin[0]) download_firmware(&_ec20_ofps, bin);
        	}
        	else // 指定配置文件
        	{
//        	    char dir[64];
//        	    char file[64];
//        	    // 解析文件名和路径
//        	    memset(dir, 0, sizeof(dir));
//        	    decode_path(_sn, dir, file);
        	    app_debug("\r\n[%s-%d] path:<%s> \r\n", __func__, __LINE__, _sn);
                memset(ini_data, 0, sizeof(ini_data));
                ini_size=0;
                resp=FTP_DownLoad_RAM(&_ec20_ofps, "/", &_sn[1], ini_save_seek);
                Ini._dsize = strlen(Ini.text);
                Ini_get_field(&Ini, fw_name, fw_key_bin, "*", bin);
                Ini_get_field(&Ini, fw_name, "0A0CK90N4123", "*", _sn);
                // 单独配置
                if(('*'!=_sn[0]) && ('-'!=_sn[0])) download_firmware(&_ec20_ofps, _sn);
                else if('*'!=bin[0]) download_firmware(&_ec20_ofps, bin);
        	}
        }
        // 解析配置重新下载
        else if('*'!=bin[0]) download_firmware(&_ec20_ofps, bin);
        at_print("AT+QFTPCLOSE\r\n");
        //at_get_resps("\r\nOK", NULL, NULL, NULL, NULL, 100,1500);
        resp = at_get_resps("\r\nOK", NULL, NULL, 1500, 100, &_ec20_ofps._at);
    }
    return 0;
}


/************************ (C) COPYRIGHT Merafour *****END OF FILE****/

