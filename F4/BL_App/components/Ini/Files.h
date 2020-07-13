/************************ (C) COPYLEFT 2018 Merafour *************************
* File Name          : Files.h
* Author             : Merafour
* Last Modified Date : 03/23/2020
* Description        : 文件操作.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
******************************************************************************/

#include <stdint.h>

#ifndef  __FILES__
#define  __FILES__

#ifdef __cplusplus
 extern "C" {
#endif

 // 下载临时路径
 extern const char download_cfg_temp[];
 extern const char download_fw_temp[];
 extern const char download_cache_path[];
 // 下载路径
 extern const char config_path[];
 extern const char fw_path[];
 // 临时 VIN
 extern const char Temporary_vin[];
 // 生成一个空文件
 extern int file_create_empty(const char* const _path, long _total, const void* const _buf, const uint16_t _bsize);
 extern long file_save(const char *path, const void * const _data, const uint32_t _len);
 //     long file_save_seek(const char _path[], const uint32_t _seek, const void* const data, const uint16_t len)
 extern long file_save_seek(const char _path[], const uint32_t _seek, const void* const data, const uint32_t len);
 extern long file_size(const char *path);
 extern long file_read_seek(const char *path, const uint32_t _seek, void * const _data, const uint32_t _len);
 extern long file_read(const char *path, void * const _data, const uint32_t _len);
 // 文件删除
 extern int file_rm(const char _path[]);
 // 文件重命名
 extern int _file_rename(const char _old_path[], const char _new_path[]);
 extern uint32_t file_crc16(const char* filename, void* const buffer, const uint16_t bsize);

 extern void file_test(void);


#ifdef __cplusplus
 }
#endif

#endif  //__FILES__
