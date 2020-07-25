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
#include "Periphs/Flash.h"
#include "stm32f4xx_hal.h"
#include "Ini/Ini.h"
#include "version.h"

static const char fw_name[] = "Ini"; // "FW";
static const char fw_key_bin[] = "BIN";
static const char fw_key_ver[] = "VER";

// 对齐
static char __attribute__ ((aligned (4))) ini_data[1024*20];
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

static void flash_save_seek(const int total, const uint32_t _seek, const char* const data, const uint16_t block)
{
	if((_seek+block)<param_download_size)
	{
		//int ret;
		memset(ini_data, 0xFF, sizeof(ini_data));
		memcpy(ini_data, data, block);
		/*ret = */Flash_Write_Force(param_download_start+_seek, (const uint32_t *)ini_data, block/4);
		//app_debug("[%s-%d] Flash_Write_Force:<%d> \r\n", __func__, __LINE__, ret);
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
static int __download_firmware(struct ec20_ofps* const _ofps, const char dir[], uint32_t total, const uint32_t crc_ftp, const char fw_file[])
{
    //char path[64];
//    char dir[64];
//    char file[64];
    //uint32_t crc_ftp;
    uint32_t crc_flash;
    //uint32_t total;
    //uint8_t len;
    //memset(path, 0, sizeof(path));
//    memset(dir, 0, sizeof(dir));
//    memset(file, 0, sizeof(file));
    //crc_ftp=0;
    //total=0;
	//at_get_split_hex(cfg, &total, ',', 1);
	//at_get_split_hex(cfg, &crc_ftp, ',', 2);
    //len = at_get_str_split(cfg, path, ',', 3, sizeof(path));
    // 检查参数: total 必须在 0到 param_flash_size之间,crc_ftp为 0 无效
    if((total>0) && (total<param_flash_size) && (0!=crc_ftp))
    {
    	// 计算本地 CRC
    	crc_flash=0;
    	if(total>param_flash_size) total=param_flash_size;
    	crc_flash = fw_crc(crc_flash, (const unsigned char*)(param_flash_start), total);
    	//decode_path(path, dir, file);
    	app_debug("\r\n[%s-%d] dir:<%s> fw_file:<%s> \r\n", __func__, __LINE__, dir, fw_file);
    	app_debug("\r\n[%s-%d] crc_ftp[0x%08X] crc_flash[0x%08X] total[0x%04X] \r\n", __func__, __LINE__, crc_ftp, crc_flash, total);
    	if(crc_flash != crc_ftp)
    	{
			app_debug("[%s-%d] download[0x%04X]:0x%08X \r\n", __func__, __LINE__, total, param_download_start);
			// 计算下载扇区
			crc_flash = fw_crc(0, (const unsigned char*)(param_download_start), total);
			app_debug("\r\n[%s-%d] crc_ftp[0x%08X] crc_flash[0x%08X] total[0x%04X] \r\n", __func__, __LINE__, crc_ftp, crc_flash, total);
			if(crc_flash != crc_ftp)
			{
				// 擦除下载扇区
	    		FLASH_Erase(param_download_start, param_download_start+param_download_size-1);
	    		//FTP_DownLoad_RAM(_ofps, "/", path, flash_save_seek);
	    		FTP_DownLoad_RAM(_ofps, dir, fw_file, flash_save_seek);
			}
			// 校验,并将新固件写入 flash
			app_debug("[%s-%d] download[0x%04X]:0x%08X \r\n", __func__, __LINE__, total, param_download_start);
			crc_flash = fw_crc(0, (const unsigned char*)(param_download_start), total);
			app_debug("\r\n[%s-%d] crc_ftp[0x%08X] crc_flash[0x%08X] total[0x%04X] \r\n", __func__, __LINE__, crc_ftp, crc_flash, total);
			// 拷贝代码
			if(crc_flash == crc_ftp)
			{
				uint32_t seek;
				uint32_t _size;
				uint32_t count;
				const uint32_t* const flash_download = (const uint32_t*)param_download_start;
				uint32_t* const flash = (uint32_t*)ini_data;
				// program
				//param_write_erase();
				// 擦除App扇区
	    		FLASH_Erase(param_flash_start, param_flash_start+param_flash_size-1);
				for(seek=0; seek<total; seek+=512)
				{
					_size = total - seek;
					if(_size>512) _size = 512;
					memset(ini_data, 0xFF, sizeof(ini_data));
					//memcpy(ini_data, (const unsigned char*)(param_download_start+seek), _size);
					//memcpy(ini_data, flash_download+seek, _size);
					for(count=0; count<(_size>>2); count++)
					{
						flash[count] = flash_download[(seek>>2)+count];
					}
					//app_debug("[%s-%d] program seek[0x%08X] _size[0x%08X] \r\n", __func__, __LINE__, total, seek, _size);
					param_write_flash((uint8_t*)ini_data, seek, _size);
				}
				crc_flash = fw_crc(0, (const unsigned char*)(param_flash_start), total);
				app_debug("\r\n[%s-%d] crc_ftp[0x%08X] crc_flash[0x%08X] total[0x%04X] \r\n", __func__, __LINE__, crc_ftp, crc_flash, total);
				// 下载完成,跳转
				if(crc_flash == crc_ftp)
				{
			        at_print("AT+QFTPCLOSE\r\n");
			        at_get_resps("\r\nOK", NULL, NULL, 1500, 100, &_ec20_ofps._at);

			        boot_app();
				}
			}
			return 0;
    	}
    }
    return -1;
}
/**
 * "$hardware/$ver/fw.Ini"
 * "$hardware/$ver/fw.bin"
 * #Only ASCII encoding is allowed
 * [Ini]
 * BIN=5000,DA1438F8,BL_App_RED.bin
 * 0A0CK90N4123=5000,DA1438F8,BL_App_RED.bin
 * [End]
 * */
static int download_firmware(struct ec20_ofps* const _ofps, const char hardware[], const char ver[])
{
    char dir[64];
    char bin[64];
    char fw_file[64];
    uint32_t crc_ftp;
    uint32_t total;
    int resp=-1;
    struct Ini_Parse Ini = {
        "A108/V2.0.0/fw.Ini",
        .text = ini_data,
         ._bsize = sizeof(ini_data),
         .pos = 0,
         ._dsize = 0,
    };
    memset(dir, 0, sizeof(dir));
    snprintf(dir, sizeof(dir), "%s/%s", hardware, ver);
    crc_ftp=0;
    total=0;
    memset(ini_data, 0, sizeof(ini_data));
    ini_size=0;
    // "$hardware/$ver/fw.Ini"
	resp=FTP_DownLoad_RAM(&_ec20_ofps, dir, "fw.Ini", ini_save_seek);
	if(EC20_RESP_OK==resp)
	{
		/**
		 * #Only ASCII encoding is allowed
		 * [Ini]
		 * BIN=5000,DA1438F8,BL_App_RED.bin
		 * [End]
		 * */
        Ini._dsize = strlen(Ini.text);
        memset(bin, 0, sizeof(bin));
        memset(fw_file, 0, sizeof(fw_file));
        Ini_get_field(&Ini, fw_name, fw_key_bin, "-", bin);
        if('-'!=bin[0])
        {
        	at_get_split_hex(bin, &total, ',', 1);
        	at_get_split_hex(bin, &crc_ftp, ',', 2);
        	// "*.bin"
        	if(at_get_str_split(bin, fw_file, ',', 3, sizeof(fw_file))>=5) return __download_firmware(&_ec20_ofps, dir, total, crc_ftp, fw_file);
        }
	}
    return -1;
}
// FTP升级
int EC20_FTP_Upload(const char hardware[], const char SN[], const char ftp[], const int port, const char user[], const char passwd[])
{
	const uint8_t sn_place = 2+Emb_Version.cfg.sn;
    int resp=-1;
    int count;
    struct Ini_Parse Ini = {
        "A108/fw.Ini",
        .text = ini_data,
         ._bsize = sizeof(ini_data),
         .pos = 0,
         ._dsize = 0,
    };
    char ver[64];
    //char _sn[64];
    char sn1[32];
    char sn2[32];
    // 把序列号拆成前 10位和后 2位
    memset(sn1, 0, sizeof(sn1));
    memset(sn2, 0, sizeof(sn2));
#if 0
    memcpy(sn1, &SN[0], 10);
    memcpy(sn2, &SN[10], 2);
    strcpy(&sn1[10], ".Ini");
//#else
    memcpy(sn1, &SN[0], 9);
    memcpy(sn2, &SN[9], 3);
    strcpy(&sn1[9], ".Ini");
#endif
    memcpy(sn1, &SN[0], 12 - sn_place);
    memcpy(sn2, &SN[12 - sn_place], sn_place);
    strcpy(&sn1[12 - sn_place], ".Ini");
    for(count=0; count<1; count++)
    {
    	app_debug("\r\n[%s-%d] EC20_Init ...\r\n", __func__, __LINE__);
    	EC20_Init();
    	app_debug("\r\n[%s-%d] EC20_FTP_Login ...\r\n", __func__, __LINE__);
        //resp=EC20_FTP_Login(&_ec20_ofps, _ec20_ofps.NetInfo.NADR, 21, 1, "obd4g", "obd.4g");
    	resp=EC20_FTP_Login(&_ec20_ofps, ftp, port, 1, user, passwd);
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
        //resp=FTP_DownLoad_RAM(&_ec20_ofps, "EPS418", "0A0CK90N41.Ini", ini_save_seek);
        resp=FTP_DownLoad_RAM(&_ec20_ofps, hardware, sn1, ini_save_seek);
        memset(ver, 0, sizeof(ver));
        // "$hardware/Ver.Ini"
        if(EC20_RESP_OK!=resp)   // "A108/Ver.Ini"
        {
sn_ini:
            memset(ini_data, 0, sizeof(ini_data));
            memset(sn1, 0, sizeof(sn1));
            strcpy(&sn1[0], Emb_Version.model);
            strcpy(&sn1[strlen(sn1)], ".Ini");
            ini_size=0;
        	//resp=FTP_DownLoad_RAM(&_ec20_ofps, hardware, "Ver.Ini", ini_save_seek);
            resp=FTP_DownLoad_RAM(&_ec20_ofps, hardware, sn1, ini_save_seek);
        	if(EC20_RESP_OK==resp)
        	{
        		/**
        		 * #Only ASCII encoding is allowed
        		 * [Ini]
        		 * VER=v3.0.0
        		 * [End]
        		 * */
                Ini._dsize = strlen(Ini.text);
                Ini_get_field(&Ini, fw_name, fw_key_ver, "-", ver);
                /**
                 * "A108/v3.0.0/fw.Ini"
                 * "A108/v3.0.0/fw.bin"
                 * */
                if('-'!=ver[0]) download_firmware(&_ec20_ofps, hardware, ver);
        	}
            at_print("AT+QFTPCLOSE\r\n");
            resp = at_get_resps("\r\nOK", NULL, NULL, 1500, 100, &_ec20_ofps._at);
            _ec20_ofps.ModuleState = EC20_MODULE_RESET;
            break;
        }
        // "$hardware/$sn1.Ini"
        else   // "A108/0A0CK90N41.Ini"
        {
        	/**
        	 * #Only ASCII encoding is allowed
        	 * [Ini]
        	 * VER=v3.0.0
        	 * 23=v2.1.0
        	 * [End]
        	 * */
            Ini._dsize = strlen(Ini.text);
            //Ini_get_field(&Ini, fw_name, "23", "*", _sn);
            Ini_get_field(&Ini, fw_name, sn2, "\n", ver);
            //app_debug("\r\n[%s-%d] ver[%s] \r\n", __func__, __LINE__, ver);
            // 无记录
            if('\n'==ver[0]) goto sn_ini;
            // 版本必须是字母开头
            else if( (('a'<=ver[0]) && ('z'>=ver[0])) || (('A'<=ver[0]) && ('Z'>=ver[0])) )  // 23=v2.1.0
            {
            	download_firmware(&_ec20_ofps, hardware, ver);
            	// 号段文件不识别键值对 VER
            	//memset(ver, 0, sizeof(ver));
            	//Ini_get_field(&Ini, fw_name, fw_key_ver, "-", ver);
            	//if('-'!=ver[0]) download_firmware(&_ec20_ofps, hardware, ver);
            }
            at_print("AT+QFTPCLOSE\r\n");
            //at_get_resps("\r\nOK", NULL, NULL, NULL, NULL, 100,1500);
            resp = at_get_resps("\r\nOK", NULL, NULL, 1500, 100, &_ec20_ofps._at);
        }
    }
    return 0;
}


/************************ (C) COPYRIGHT Merafour *****END OF FILE****/

