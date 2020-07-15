﻿/************************ (C) COPYLEFT 2018 Merafour *************************
* File Name          : Files.c
* Author             : Merafour
* Last Modified Date : 03/23/2020
* Description        : 文件操作.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
******************************************************************************/
#include <string.h>
#include <fcntl.h>
#include "fatfs.h"
#include "Files.h"
#include "Periphs/uart.h"
#include "Periphs/crc.h"

extern const char* GB_GMT8_Format(void);
#define  MCU_BUILD   0
#if MCU_BUILD
// 下载临时路径
const char download_cfg_temp[] ="/Upgrade.cfg";
const char download_fw_temp[] ="/Upgrade.bin";
const char download_cache_path[] ="/Download.dat";
// 下载路径
const char config_path[] ="/OBDII.cfg";
const char fw_path[] ="/download.bin";
#else
// 下载临时路径
const char download_cfg_temp[] ="./upload/Upgrade.cfg";
const char download_fw_temp[]="./upload/Upgrade.bin";
const char download_cache_path[] = "./upload/Download.dat";
// 下载路径
const char config_path[] ="./upload/OBDII.cfg";
//const char fw_path[] ="./upload/fw_ZA0CK20N0001_OBDII-4G.bin";
const char fw_path[] ="./upload/download.bin";
#endif
// 临时 VIN
const char Temporary_vin[] = "VINTT23456789ABCDEF";

long file_save(const char *path, const void * const _data, const uint32_t _len)
{
	UINT _size=0;
    FIL fil;            /* File object */
    FRESULT res;  /* API result code */
    // w+ : 可读可写, 可以不存在, 必会擦掉原有内容从头写
    res = f_open(&fil, path, FA_OPEN_ALWAYS|FA_WRITE);
    if(FR_OK != res)
    {
        //pr_debug("file %s not exist! \n", filename);  fflush(stdout);
        return -1;
    }
    res = f_write(&fil, _data, _len, &_size);
    if(_size<0)
    {
    	f_close(&fil);
        return -2;
    }
    f_close(&fil);
    return _size;
}
long file_save_seek(const char _path[], const uint32_t _seek, const void* const data, const uint32_t len)
//long file_save_seek(const char _path[], const uint32_t _seek, const void* const data, const uint16_t len)
{
	UINT _size=0;
    FIL fil;            /* File object */
    FRESULT res;  /* API result code */
    res = f_open(&fil, _path, FA_OPEN_ALWAYS|FA_WRITE|FA_OPEN_APPEND);
    if(FR_OK != res)
    {
        //pr_debug("file %s not exist! \n", filename);  fflush(stdout);
        return -1;
    }
    else
    {
        //lseek(fd, _seek, SEEK_SET);
        f_lseek (&fil, _seek);
        //_size = write(fd, data, (size_t)len);
        res = f_write(&fil, data, len, &_size);
        if(_size<0)
        {
        	f_close(&fil);
            return -2;
        }
        f_close(&fil);
    }
    return 0;
}
long file_size(const char *path)
{
	UINT _size=0;
    FIL fil;            /* File object */
    FRESULT res;  /* API result code */
    res = f_open(&fil, path, FA_OPEN_EXISTING|FA_READ);
    if(FR_OK != res)
    {
        //pr_debug("file %s not exist! \n", filename);  fflush(stdout);
        return -1;
    }
    else
    {
    	_size = f_size(&fil);
        f_close(&fil);
    }
    return _size;
}
long file_read_seek(const char *path, const uint32_t _seek, void * const _data, const uint32_t _len)
{
	UINT _size=0;
	UINT _rsize=0;
    FIL fil;            /* File object */
    FRESULT res;  /* API result code */
    res = f_open(&fil, path, FA_OPEN_EXISTING | FA_READ);
    if(FR_OK != res)
    {
        //pr_debug("file %s not exist! \n", filename);  fflush(stdout);
        return -1;
    }
    else
    {
    	_size = f_size(&fil);
        if(_size<(long)_seek)
        {
            //printf("_size[%s] :%ld _seek:%d\n", path, _size, _seek);  fflush(stdout);
            return 0;
        }
        _size = _size - _seek;
        if(_size>(long)_len)
        {
            //printf("_size[%s] :%ld _len:%d\n", path, _size, _len);  fflush(stdout);
            _size=_len;
        }
        f_lseek (&fil, _seek);
        //printf("_size[%s] _size :%ld \n", path, _size);  fflush(stdout);
        //fread(_data, _size, 1, fd);
        res = f_read(&fil, _data, _size, &_rsize);
        //printf("fread[%s] _size :%ld \n", path, _size);  fflush(stdout);
        f_close(&fil);
    }
    return _rsize;
}
long file_read(const char *path, void * const _data, const uint32_t _len)
{
    // 调用底层接口实现
    return file_read_seek(path, 0, _data, _len);
}

// 生成一个空文件
int file_create_empty(const char* const _path, long _total, const void* const _buf, const uint16_t _bsize)
{
    long _size=0;
    uint32_t _seek;
    //const char* const _mem=(char*)_buf;
    //const uint32_t _mem_size = _bsize;
    //printf("[%s-%d] file_create %s \r\n", __func__, __LINE__,  _path);
    // 删除文件
    file_rm(_path);
    // 创建文件
    file_save(_path, _buf, 0);
    //memset(_mem, 0, _mem_size);
    _size = _bsize;//_mem_size;
    for(_seek=0; (long)_seek<_total; _seek += _size)
    {
        _size = _total - _seek;
        if(_size>(long)_bsize) _size=_bsize;
        if(0==_seek) file_save(_path, _buf, (size_t)_size);
        else file_save_seek(_path, _seek, _buf, (size_t)_size);
    }
    return 0;
}

// 文件删除
int file_rm(const char _path[])
{
    FIL fil;            /* File object */
    FRESULT res;  /* API result code */
    res = f_open(&fil, _path, FA_OPEN_EXISTING|FA_READ);
    if(FR_OK != res)
    {
        //pr_debug("file %s not exist! \n", filename);  fflush(stdout);
        return -1;
    }
    else
    {
        f_close(&fil);
        //remove(_path);
        f_unlink(_path);
        return 0;
    }
    return -1;
}
// 文件重命名
int _file_rename(const char _old_path[], const char _new_path[])
{
    file_rm(_new_path);
    return f_rename(_old_path, _new_path);
}

uint32_t file_crc16(const char* filename, void* const buffer, const uint16_t bsize)
{
    char* const filebin = (char*)buffer; // 1024B
    long _size=0;
    long _seek=0;
    long rsize=0;
    uint32_t checksum = 0x12345678;
    //unsigned short sum = 0;
    uint32_t sum = 0;
    _size = file_size(filename);
    //printf("%s@%d _size:%ld\n", __func__, __LINE__, _size); fflush(stdout);
    rsize=0;
    checksum = 0;
    sum = 0;
    for(_seek=0; _seek<_size; _seek+=rsize)
    {
        rsize = _size - _seek;
        if(rsize>bsize) rsize=bsize;
        // read
        memset(filebin, 0xFF, bsize);
        //read(fd, filebin, (size_t)rsize);
        file_read_seek(filename, _seek, filebin, (size_t)rsize);
        sum = fw_crc(sum, (unsigned char *)filebin, rsize);
    }
    checksum = sum;
    return checksum;
}
uint32_t flash_crc16(const uint32_t flash_addr, const uint32_t flash_size, void* const buffer, const uint16_t bsize)
{
    char* const filebin = (char*)buffer; // 1024B
    const char* const flash = (const char*)flash_addr;
    long _seek=0;
    long rsize=0;
    uint32_t checksum = 0x12345678;
    //unsigned short sum = 0;
    uint32_t sum = 0;
    //printf("%s@%d _size:%ld\n", __func__, __LINE__, _size); fflush(stdout);
    rsize=0;
    checksum = 0;
    sum = 0;
    for(_seek=0; _seek<flash_size; _seek+=rsize)
    {
        rsize = flash_size - _seek;
        if(rsize>bsize) rsize=bsize;
        // read
        memset(filebin, 0xFF, bsize);
        //read(fd, filebin, (size_t)rsize);
        //file_read_seek(filename, _seek, filebin, (size_t)rsize);
        memcpy(filebin, &flash[_seek], rsize);
        sum = fw_crc(sum, (unsigned char *)filebin, rsize);
    }
    checksum = sum;
    return checksum;
}

void file_test(void)
{
    uint8_t _buf[1024];
    uint32_t checksum=0;
    uint32_t checksum2=0;
    const char _path[] = "./upload/load.bin";
    app_debug("[%s-%d] <%s> \r\n", __func__, __LINE__, GB_GMT8_Format());
    /*file_rm(download_fw_temp);
    file_rename(download_cache_path, download_fw_temp);
    memset(_buf, 'a', sizeof(_buf));
    file_save(download_cache_path, "HelloHello", 5);
    file_create_empty(download_cache_path, 1024*19, _buf, sizeof(_buf));
    file_create_empty(download_fw_temp, 1024*5, _buf, sizeof(_buf));*/
    checksum = file_crc16(fw_path, _buf, sizeof(_buf));
    //app_debug("[%s-%d] checksum:0x%08X\n", __func__, __LINE__, checksum);
    checksum2 = file_crc16(fw_path, _buf, sizeof(_buf));
    app_debug("[%s-%d] checksum:0x%08X checksum2:0x%08X\n", __func__, __LINE__, checksum, checksum2);
    const uint8_t data[] = {0xC9, 0x60, 0x0C, 0xD4, 0xBA, 0xF1, 0x01, 0x0A, 0xBA, 0xF1, 0x01, 0x0F, 0x07, 0xDB, 0x5F, 0x45, 0x02, 0xD3, 0x20, 0x20, 0x8B, 0xF8, 0x00, 0x00, 0x1B, 0xF1, 0x01, 0x0B, 0xF2, 0xE7, 0x20, 0x68, 0x24, 0x1D, 0x8D, 0xF8, 0x0A, 0x00, 0x5F, 0x45, 0x03, 0xD3, 0x9D, 0xF8, 0x0A, 0x00, 0x8B, 0xF8, 0x00, 0x00, 0x1B, 0xF1, 0x01, 0x0B, 0xBA, 0xF1, 0x01, 0x0A, 0xBA, 0xF1, 0x01, 0x0F, 0x07, 0xDB, 0x5F, 0x45, 0x02, 0xD3, 0x20, 0x20, 0x8B, 0xF8, 0x00, 0x00, 0x1B, 0xF1, 0x01, 0x0B, 0xF2, 0xE7, 0xAA, 0xE0, 0x20, 0x68, 0x24, 0x1D, 0x80, 0x46, 0xB8, 0xF1, 0x00, 0x0F, 0x01, 0xD1, 0x8A, 0x48, 0x80, 0x46, 0x40, 0x46, 0xFF, 0xF7, 0x35, 0xFE, 0x03, 0x90, 0x5F, 0xEA, 0xC9, 0x60, 0x0D, 0xD4, 0x50, 0x46, 0xB0, 0xF1, 0x01, 0x0A, 0x03, 0x99, 0x81, 0x42, 0x07, 0xDA, 0x5F, 0x45, 0x02, 0xD3, 0x20, 0x20, 0x8B, 0xF8, 0x00, 0x00, 0x1B, 0xF1, 0x01, 0x0B, 0xF1, 0xE7, 0x00, 0x20, 0x06, 0x00, 0x03, 0x98, 0x86, 0x42, 0x0B, 0xDA, 0x5F, 0x45, 0x03, 0xD3, 0x98, 0xF8, 0x00, 0x00, 0x8B, 0xF8, 0x00, 0x00, 0x1B, 0xF1, 0x01, 0x0B, 0x18, 0xF1, 0x01, 0x08, 0x76, 0x1C, 0xF0, 0xE7, 0x50, 0x46, 0xB0, 0xF1, 0x01, 0x0A, 0x03, 0x99, 0x81, 0x42, 0x07, 0xDA, 0x5F, 0x45, 0x02, 0xD3, 0x20, 0x20, 0x8B, 0xF8, 0x00, 0x00, 0x1B, 0xF1, 0x01, 0x0B, 0xF1, 0xE7, 0x6D, 0xE0, 0x1A, 0xF1, 0x01, 0x0F, 0x03, 0xD1, 0x08, 0x20, 0x82, 0x46, 0x59, 0xF0, 0x01, 0x09, 0x22, 0x68, 0x24, 0x1D, 0x5F, 0xFA, 0x89, 0xF9, 0xCD, 0xF8, 0x04, 0x90, 0xCD, 0xF8, 0x00, 0xA0, 0x10, 0x23, 0x39, 0x00, 0x58, 0x46, 0xFF, 0xF7, 0x4F, 0xFE, 0x83, 0x46, 0x57, 0xE0, 0x5F, 0x45, 0x02, 0xD3, 0x25, 0x20, 0x8B, 0xF8, 0x00, 0x00, 0x1B, 0xF1, 0x01, 0x0B, 0x4F, 0xE0, 0x08, 0x20, 0x8D, 0xF8, 0x09, 0x00, 0x20, 0xE0, 0x59, 0xF0, 0x40, 0x09, 0x10, 0x20, 0x8D, 0xF8, 0x09, 0x00, 0x1A, 0xE0, 0x59, 0xF0, 0x02, 0x09, 0x17, 0xE0, 0x5F, 0x45, 0x02, 0xD3, 0x25, 0x20, 0x8B, 0xF8, 0x00, 0x00, 0x1B, 0xF1, 0x01, 0x0B, 0x06, 0x98, 0x00, 0x78, 0x00, 0x28, 0x08, 0xD0, 0x5F, 0x45, 0x03, 0xD3, 0x06, 0x98, 0x00, 0x78, 0x8B, 0xF8, 0x00, 0x00, 0x1B, 0xF1, 0x01, 0x0B, 0x02, 0xE0, 0x06, 0x98, 0x40, 0x1E, 0x06, 0x90, 0x2A, 0xE0, 0x9D, 0xF8, 0x08, 0x00, 0x6C, 0x28, 0x06, 0xD1, 0x20, 0x68, 0x24, 0x1D, 0x05, 0x00, 0x5F, 0xEA, 0x89, 0x70, 0x12, 0xD5, 0x11, 0xE0, 0x9D, 0xF8, 0x08, 0x00, 0x68, 0x28, 0x08, 0xD1, 0x20, 0x68, 0x24, 0x1D, 0x80, 0xB2, 0x05, 0x00, 0x5F, 0xEA, 0x89, 0x70, 0x06, 0xD5, 0x2D, 0xB2, 0x04, 0xE0, 0x20, 0x68, 0x24, 0x1D, 0x05, 0x00, 0x5F, 0xEA, 0x89, 0x70, 0x5F, 0xFA, 0x89, 0xF9, 0xCD, 0xF8, 0x04, 0x90, 0xCD, 0xF8, 0x00, 0xA0, 0x9D, 0xF8, 0x09, 0x30, 0x2A, 0x00, 0x39, 0x00, 0x58, 0x46, 0xFF, 0xF7, 0xF6, 0xFD, 0x83, 0x46, 0x06, 0x98, 0x40, 0x1C, 0x06, 0x90, 0x9B, 0xE6, 0x5F, 0x45, 0x03, 0xD3, 0x00, 0x20, 0x8B, 0xF8, 0x00, 0x00, 0x01, 0xE0, 0x00, 0x20, 0x38, 0x70, 0x04, 0x98, 0xBB, 0xEB, 0x00, 0x00, 0x07, 0xB0, 0xBD, 0xE8, 0xF0, 0x8F, 0x08, 0xB4, 0x2D, 0xE9, 0xF8, 0x41, 0x04, 0x00, 0x0D, 0x00, 0x16, 0x00, 0x07, 0xA8, 0x80, 0x46, 0x43, 0x46, 0x32, 0x00, 0x29, 0x00, 0x20, 0x00, 0xFF, 0xF7, 0x6C, 0xFE, 0x07, 0x00, 0x38, 0x00, 0xBD, 0xE8, 0xF2, 0x01, 0x5D, 0xF8, 0x08, 0xFB, 0x70, 0xB5, 0x04, 0x00, 0x24, 0x48, 0x00, 0x68, 0x06, 0x00, 0x20, 0x00, 0x00, 0xF0, 0xC5, 0xFA, 0x05, 0x00, 0x00, 0x2D, 0x0E, 0xD0, 0x20, 0x48, 0x00, 0x68, 0x00, 0x28, 0x03, 0xD0, 0x1E, 0x48, 0x00, 0x68, 0x00, 0xF0, 0x2E, 0xFB, 0x1C, 0x48, 0x05, 0x60, 0x03, 0x21, 0x1A, 0x48, 0x00, 0x68, 0x00, 0xF0, 0xDE, 0xFA, 0x30, 0x00, 0x70, 0xBD, 0x0E, 0xB4, 0x78, 0xB5, 0x04, 0x00, 0x05, 0xA8, 0x05, 0x00, 0x2B, 0x00, 0x22, 0x00, 0x80, 0x21, 0x14, 0x48, 0xFF, 0xF7, 0x3F, 0xFE, 0x06, 0x00, 0x11, 0x48, 0x00, 0x68, 0x00, 0x28, 0x03, 0xD1, 0x10, 0x48, 0x00, 0xF0, 0x21, 0xF8, 0x06, 0xE0, 0x33, 0x00, 0x0E, 0x4A, 0x00, 0x21, 0x0C, 0x48, 0x00, 0x68, 0x00, 0xF0, 0x5A, 0xFB, 0x71, 0xBC, 0x5D, 0xF8, 0x10, 0xFB, 0x60, 0x33, 0x01, 0x20, 0xD0, 0x37, 0x04, 0x08, 0x38, 0x0E, 0x04, 0x08, 0xDC, 0x37, 0x04, 0x08, 0xE4, 0x14, 0x04, 0x08, 0x74, 0x08, 0x04, 0x08, 0xD8, 0x27, 0x04, 0x08, 0xC4, 0x27, 0x04, 0x08, 0x7C, 0x3E, 0x04, 0x08, 0x64, 0x33, 0x01, 0x20, 0x24, 0x25, 0x01, 0x20, 0x70, 0x47, 0x03, 0x68, 0x1A, 0xB9, 0x5A, 0x1E, 0x02, 0x60, 0x08, 0x46, 0x70, 0x47, 0x19, 0x78, 0x11, 0xB9, 0x4F, 0xF0, 0xFF, 0x30, 0x70, 0x47, 0x59, 0x1C, 0x01, 0x60, 0x18, 0x78, 0x70, 0x47, 0x70, 0xB5, 0x04, 0x00, 0x00, 0x20, 0x06, 0x00, 0x01, 0xF0, 0xD7, 0xFA, 0x00, 0x20, 0x05, 0x00, 0x04, 0x2D, 0x15, 0xDA, 0xDF, 0xF8, 0x1C, 0x04, 0x50, 0xF8, 0x25, 0x00, 0x00, 0x28, 0x0D, 0xD0, 0x21, 0x68, 0xDF, 0xF8, 0x10, 0x04, 0x50, 0xF8, 0x25, 0x00, 0x00, 0x68, 0x01, 0xF0, 0x8C, 0xFC, 0x00, 0x28, 0x03, 0xD1, 0x5F, 0xF0, 0xFF, 0x30, 0x06, 0x00, 0x17, 0xE0, 0x6D, 0x1C, 0xE7, 0xE7, 0x00, 0x20, 0x05, 0x00, 0x04, 0x2D, 0x07, 0xDA, 0xDF, 0xF8, 0xE8, 0x03, 0x50, 0xF8, 0x25, 0x00, 0x00, 0x28, 0x01, 0xD0, 0x6D, 0x1C, 0xF5, 0xE7, 0x04, 0x2D, 0x03, 0xD1, 0x5F, 0xF0, 0xFF, 0x30, 0x06, 0x00, 0x03, 0xE0, 0xDF, 0xF8, 0xCC, 0x03, 0x40, 0xF8, 0x25, 0x40, 0x01, 0xF0, 0xC0, 0xFA, 0x30, 0x00, 0x70, 0xBD, 0x2D, 0xE9, 0xF0, 0x41, 0x04, 0x00, 0x00, 0x20, 0x05, 0x00, 0x00, 0x20, 0x80, 0x46, 0x01, 0xF0, 0x9A, 0xFA, 0x00, 0x20, 0x06, 0x00, 0x02, 0x2E, 0x34, 0xD2, 0xDF, 0xF8, 0xA8, 0x03, 0x31, 0x01, 0x08, 0x44, 0x40, 0x68, 0x00, 0x28, 0x2B, 0xD0, 0xDF, 0xF8, 0x98, 0x03, 0x31, 0x01, 0x08, 0x44, 0x40, 0x68, 0x01, 0xF0, 0x7D, 0xFC, 0x07, 0x00, 0x47, 0x45, 0x21, 0xD3, 0xDF, 0xF8, 0x84, 0x03, 0x31, 0x01, 0x08, 0x44, 0x80, 0x68, 0x00, 0x28, 0x1A, 0xD0, 0x3A, 0x00, 0x21, 0x00, 0xDF, 0xF8, 0x74, 0x03, 0x33, 0x01, 0x18, 0x44, 0x40, 0x68, 0x01, 0xF0, 0x85, 0xFC, 0x00, 0x28, 0x0F, 0xD1, 0x02, 0x2F, 0x07, 0xD3, 0x20, 0x00, 0x01, 0xF0, 0x63, 0xFC, 0x87, 0x42, 0x02, 0xD2, 0xE0, 0x5D, 0x2F, 0x28, 0x05, 0xD1, 0xDF, 0xF8, 0x4C, 0x03, 0x31, 0x01, 0x08, 0x44, 0x05, 0x00, 0xB8, 0x46, 0x76, 0x1C, 0xC8, 0xE7, 0x01, 0xF0, 0x7A, 0xFA, 0x28, 0x00, 0xBD, 0xE8, 0xF0, 0x81, 0x2D, 0xE9, 0xF0, 0x47, 0x82, 0xB0, 0x04, 0x00, 0x0D, 0x00, 0x16, 0x00, 0x00, 0x2C, 0x0F, 0xD1, 0x00, 0x20, 0x8D, 0xF8, 0x00, 0x00, 0x89, 0x23, 0xDF, 0xF8, 0x20, 0x23, 0xDF, 0xF8, 0x20, 0x13, 0xDF, 0xF8, 0x20, 0x03, 0xFF, 0xF7, 0x27, 0xFF, 0x9D, 0xF8, 0x00, 0x00, 0x00, 0x28, 0xFB, 0xD0, 0x00, 0x2D, 0x0F, 0xD1, 0x00, 0x20, 0x8D, 0xF8, 0x00, 0x00, 0x8A, 0x23, 0xDF, 0xF8, 0xFC, 0x22, 0xDF, 0xF8, 0x04, 0x13, 0xDF, 0xF8, 0xFC, 0x02, 0xFF, 0xF7};
    file_save(_path, data, sizeof(data));
    file_save_seek(_path, 1024, data, sizeof(data));

    //fflush(stdout);
}



