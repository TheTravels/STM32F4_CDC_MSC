/************************ (C) COPYLEFT 2018 Merafour *************************

* File Name          : ZKHY_Dev_upload.c
* Author             : Merafour
* Last Modified Date : 05/09/2020
* Description        : 正科环宇设备升级协议.该协议定义正科环宇设备的网络分包升级、FTP升级、远程烧号等功能
* Description        : 见《正科环宇设备升级协议.pdf》
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
******************************************************************************/
#include "ZKHY_Dev_upload.h"
#include "../BigLittleEndian.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../GB17691.h"

#define  debug_log     0
#ifndef debug_log
#define  debug_log     1
#endif
//#undef   debug_log
#include <stdio.h>

//#ifdef debug_log
#if debug_log
#define pr_debug(fmt, ...) printf(fmt, ##__VA_ARGS__); fflush(stdout);
#else
#define pr_debug(fmt, ...) ;
#endif

/*static inline uint16_t gb_cpy(void* const _Dst,const void* const _Src, const uint16_t _Size)
{
    memcpy(_Dst, _Src, _Size);
    return _Size;
}*/

/* -------------------------------------------------- 编码解码 ------------------------------------------------------------ */
// 登录
static inline int EnFrame_login(const struct ZKHY_Frame_login* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;                                                             
    index += gb_cpy(&buf[index], _info->sn, sizeof (_info->sn));
    index += gb_cpy(&buf[index], _info->VIN, sizeof (_info->VIN));
    index += gb_cpy(&buf[index], _info->Model, sizeof (_info->Model));
    index += gb_cpy(&buf[index], _info->ICCID, sizeof (_info->ICCID));
    index += gb_cpy(&buf[index], _info->Ver, sizeof (_info->Ver));
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_login(struct ZKHY_Frame_login* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;                                                             
    index += gb_cpy(_info->sn, &data[index], sizeof (_info->sn));
    index += gb_cpy(_info->VIN, &data[index], sizeof (_info->VIN));
    index += gb_cpy(_info->Model, &data[index], sizeof (_info->Model));
    index += gb_cpy(_info->ICCID, &data[index], sizeof (_info->ICCID));
    index += gb_cpy(_info->Ver, &data[index], sizeof (_info->Ver));
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_login_ack(const struct ZKHY_Frame_login_ack* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;                                                        
    index += bigend32_encode(&buf[index], _info->fw);   
    index += bigend32_encode(&buf[index], _info->cfg);   
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_login_ack(struct ZKHY_Frame_login_ack* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;                                                        
    index += bigend32_merge(&_info->fw, data[index], data[index+1], data[index+2], data[index+3]);   
    index += bigend32_merge(&_info->cfg, data[index], data[index+1], data[index+2], data[index+3]);   
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
// 下载
static inline int EnFrame_download(const struct ZKHY_Frame_download* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;                                                             
    index += gb_cpy(&buf[index], _info->key, sizeof (_info->key));
    index += bigend32_encode(&buf[index], _info->seek);
    index += bigend32_encode(&buf[index], _info->total);
    index += bigend16_encode(&buf[index], _info->block);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_download(struct ZKHY_Frame_download* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;                                                             
    index += gb_cpy(_info->key, &data[index], sizeof (_info->key));
    index += bigend32_merge(&_info->seek, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend32_merge(&_info->total, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend16_merge(&_info->block, data[index], data[index+1]);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_download_ack(const struct ZKHY_Frame_download_ack* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    uint16_t block;
    block = _info->block;
    if(block>sizeof(_info->data)) block=sizeof(_info->data);
    index=0;                                                        
    index += bigend32_encode(&buf[index], _info->seek);
    index += bigend32_encode(&buf[index], _info->total);
    index += bigend16_encode(&buf[index], block); 
    index += gb_cpy(&buf[index], _info->data, block);
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_download_ack(struct ZKHY_Frame_download_ack* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    uint16_t block;
    index=0;                                                        
    index += bigend32_merge(&_info->seek, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend32_merge(&_info->total, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend16_merge(&_info->block, data[index], data[index+1]); 
    block = _info->block;
    if(block>sizeof(_info->data)) block=sizeof(_info->data);
    index += gb_cpy(_info->data, &data[index], block);
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
// 更新请求
static inline int EnFrame_upload_req(const struct ZKHY_Frame_upload_req* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;                                                             
    index += gb_cpy(&buf[index], _info->sn, sizeof (_info->sn));
    index += gb_cpy(&buf[index], _info->Model, sizeof (_info->Model));
    index += bigend32_encode(&buf[index], _info->checksum);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_upload_req(struct ZKHY_Frame_upload_req* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;                                                             
    index += gb_cpy(_info->sn, &data[index], sizeof (_info->sn));
    index += gb_cpy(_info->Model, &data[index], sizeof (_info->Model));
    index += bigend32_merge(&_info->checksum, data[index], data[index+1], data[index+2], data[index+3]);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_upload_req_client(const struct ZKHY_Frame_upload_req_client* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;                                                             
    index += gb_cpy(&buf[index], _info->sn, sizeof (_info->sn));
    index += bigend32_encode(&buf[index], _info->checksum);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_upload_req_client(struct ZKHY_Frame_upload_req_client* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;                                                             
    index += gb_cpy(_info->sn, &data[index], sizeof (_info->sn));
    index += bigend32_merge(&_info->checksum, data[index], data[index+1], data[index+2], data[index+3]);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_upload_ack(const struct ZKHY_Frame_upload_ack* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;                                                             
    index += gb_cpy(&buf[index], _info->key, sizeof (_info->key));
    index += bigend32_encode(&buf[index], _info->checksum);
    index += bigend32_encode(&buf[index], _info->total);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_upload_ack(struct ZKHY_Frame_upload_ack* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;                                                             
    index += gb_cpy(_info->key, &data[index], sizeof (_info->key));
    index += bigend32_merge(&_info->checksum, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend32_merge(&_info->total, data[index], data[index+1], data[index+2], data[index+3]);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_upload_ackb(const struct ZKHY_Frame_upload_ackb* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;                                                             
    index += gb_cpy(&buf[index], _info->key, sizeof (_info->key));
    index += gb_cpy(&buf[index], _info->map, sizeof (_info->map));
    index += bigend32_encode(&buf[index], _info->checksum);
    index += bigend32_encode(&buf[index], _info->total);
    buf[index++] = _info->value;
    buf[index++] = _info->block;
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_upload_ackb(struct ZKHY_Frame_upload_ackb* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;                                                             
    index += gb_cpy(_info->key, &data[index], sizeof (_info->key));
    index += gb_cpy(_info->map, &data[index], sizeof (_info->map));
    index += bigend32_merge(&_info->checksum, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend32_merge(&_info->total, data[index], data[index+1], data[index+2], data[index+3]);
    _info->value = data[index++];
    _info->block = data[index++];
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}

/******************* 嵌入式升级 ********************/
static inline int EnFrame_Emb_sync(const struct ZKHY_Frame_Emb_sync* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    index += array32_encode(&buf[index], _info->KK1, 4);
    index += array32_encode(&buf[index], _info->KK2, 4);
    index += gb_cpy(&buf[index], _info->flag, sizeof (_info->flag));
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_Emb_sync(struct ZKHY_Frame_Emb_sync* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    index += array32_merge(_info->KK1, &data[index], 4);
    index += array32_merge(_info->KK2, &data[index], 4);
    index += gb_cpy(_info->flag, &data[index], sizeof (_info->flag));
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_Emb_synca(const struct ZKHY_Frame_Emb_synca* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    index += array32_encode(&buf[index], _info->Kk1, 4);
    index += array32_encode(&buf[index], _info->Kk3, 4);
    index += gb_cpy(&buf[index], _info->boot_ver, sizeof (_info->boot_ver));
    index += gb_cpy(&buf[index], _info->app_ver, sizeof (_info->app_ver));
    index += gb_cpy(&buf[index], _info->ID, sizeof (_info->ID));
    index += bigend32_encode(&buf[index], _info->crc);
    index += bigend16_encode(&buf[index], _info->block);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_Emb_synca(struct ZKHY_Frame_Emb_synca* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    index += array32_merge(_info->Kk1, &data[index], 4);
    index += array32_merge(_info->Kk3, &data[index], 4);
    index += gb_cpy(_info->boot_ver, &data[index], sizeof (_info->boot_ver));
    index += gb_cpy(_info->app_ver, &data[index], sizeof (_info->app_ver));
    index += gb_cpy(_info->ID, &data[index], sizeof (_info->ID));
    index += bigend32_merge(&_info->crc, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend16_merge(&_info->block, data[index], data[index+1]);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_Emb_erasea(const struct ZKHY_Frame_Emb_erasea* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    buf[index++] = (uint8_t)_info->MemNum;
    index += bigend32_encode(&buf[index], _info->volume);
    index += bigend32_encode(&buf[index], _info->erase);
    buf[index++] = _info->status;
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_Emb_erasea(struct ZKHY_Frame_Emb_erasea* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    _info->MemNum = data[index++];
    index += bigend32_merge(&_info->volume, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend32_merge(&_info->erase, data[index], data[index+1], data[index+2], data[index+3]);
    _info->status = data[index++];
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_Emb_write(const struct ZKHY_Frame_Emb_write* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    buf[index++] = (uint8_t)_info->MemNum;
    index += bigend32_encode(&buf[index], _info->seek);
    index += bigend32_encode(&buf[index], _info->total);
    index += bigend16_encode(&buf[index], _info->block);
    if((EMB_STORE_UART==_info->MemNum) && (0==_info->total)) // config uart
    {
        index += bigend32_encode(&buf[index], _info->uart.BaudRate);
        buf[index++] = _info->uart.DataWidth;
        buf[index++] = _info->uart.StopBits;
        buf[index++] = _info->uart.Parity;
    }
    else if(EMB_STORE_KEY==_info->MemNum) // write key
    {
        index += array32_encode(&buf[index], _info->key, 8);
    }
    else index += gb_cpy(&buf[index], _info->data, _info->block);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_Emb_write(struct ZKHY_Frame_Emb_write* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    _info->MemNum = data[index++];
    index += bigend32_merge(&_info->seek, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend32_merge(&_info->total, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend16_merge(&_info->block, data[index], data[index+1]);
    if((EMB_STORE_UART==_info->MemNum) && (0==_info->total)) // config uart
    {
        index += bigend32_merge(&_info->uart.BaudRate, data[index], data[index+1], data[index+2], data[index+3]);
        _info->uart.DataWidth = data[index++];
        _info->uart.StopBits = data[index++];
        _info->uart.Parity = data[index++];
    }
    else if(EMB_STORE_KEY==_info->MemNum) // write key
    {
        index += array32_encode(&data[index], _info->key, 8);
    }
    else index += gb_cpy(_info->data, &data[index], _info->block);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_Emb_writea(const struct ZKHY_Frame_Emb_writea* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    buf[index++] = (uint8_t)_info->MemNum;
    index += bigend32_encode(&buf[index], _info->volume);
    index += bigend32_encode(&buf[index], _info->write);
    buf[index++] = _info->status;
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_Emb_writea(struct ZKHY_Frame_Emb_writea* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    _info->MemNum = data[index++];
    index += bigend32_merge(&_info->volume, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend32_merge(&_info->write, data[index], data[index+1], data[index+2], data[index+3]);
    _info->status = data[index++];
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_Emb_read(const struct ZKHY_Frame_Emb_read* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    buf[index++] = (uint8_t)_info->MemNum;
    index += bigend32_encode(&buf[index], _info->seek);
    index += bigend16_encode(&buf[index], _info->block);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_Emb_read(struct ZKHY_Frame_Emb_read* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    _info->MemNum = data[index++];
    index += bigend32_merge(&_info->seek, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend16_merge(&_info->block, data[index], data[index+1]);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_Emb_reada(const struct ZKHY_Frame_Emb_reada* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    buf[index++] = (uint8_t)_info->MemNum;
    index += bigend32_encode(&buf[index], _info->volume);
    buf[index++] = _info->status;
    index += bigend32_encode(&buf[index], _info->seek);
    index += bigend16_encode(&buf[index], _info->block);
    if(EMB_STORE_KEY==_info->MemNum) // write key
    {
        index += array32_encode(&buf[index], _info->key, 8);
    }
    else index += gb_cpy(&buf[index], _info->data, _info->block);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_Emb_reada(struct ZKHY_Frame_Emb_reada* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    _info->MemNum = data[index++];
    index += bigend32_merge(&_info->volume, data[index], data[index+1], data[index+2], data[index+3]);
    _info->status = data[index++];
    index += bigend32_merge(&_info->seek, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend16_merge(&_info->block, data[index], data[index+1]);
    if(EMB_STORE_KEY==_info->MemNum) // write key
    {
        index += array32_encode(&data[index], _info->key, 8);
    }
    else index += gb_cpy(_info->data, &data[index], _info->block);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_Emb_boot(const struct ZKHY_Frame_Emb_boot* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    index += bigend32_encode(&buf[index], _info->total);
    index += bigend32_encode(&buf[index], _info->crc);
    index += gb_cpy(&buf[index], _info->data, sizeof(_info->data));
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_Emb_boot(struct ZKHY_Frame_Emb_boot* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    index += bigend32_merge(&_info->total, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend32_merge(&_info->crc, data[index], data[index+1], data[index+2], data[index+3]);
    index += gb_cpy(_info->data, &data[index], sizeof(_info->data));
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_Emb_boota(const struct ZKHY_Frame_Emb_boota* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    buf[index++] = _info->status;
    index += gb_cpy(&buf[index], _info->info, sizeof(_info->info));
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_Emb_boota(struct ZKHY_Frame_Emb_boota* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    _info->status = data[index++];
    index += gb_cpy(_info->info, &data[index], sizeof(_info->info));
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_Emb_reboot(const struct ZKHY_Frame_Emb_reboot* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    index += bigend32_encode(&buf[index], _info->C1);
    index += bigend32_encode(&buf[index], _info->C2);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_Emb_reboot(struct ZKHY_Frame_Emb_reboot* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    index += bigend32_merge(&_info->C1, data[index], data[index+1], data[index+2], data[index+3]);
    index += bigend32_merge(&_info->C2, data[index], data[index+1], data[index+2], data[index+3]);
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}
static inline int EnFrame_Emb_reboota(const struct ZKHY_Frame_Emb_reboota* const _info, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    buf[index++] = _info->status;
    index += gb_cpy(&buf[index], _info->info, sizeof(_info->info));
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_ENCODE_PACKL;
    return index; // len
}
static inline int DeFrame_Emb_reboota(struct ZKHY_Frame_Emb_reboota* const _info, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    index=0;
    _info->status = data[index++];
    index += gb_cpy(_info->info, &data[index], sizeof(_info->info));
    // buf 大小检查
    if(index>_size) return ZKHY_RESP_ERR_DECODE_PACKL;
    return index; // len
}


/**
 * 编码一帧数据
 */
int ZKHY_EnFrame_upload(const struct ZKHY_Frame_upload* const _frame, uint8_t buf[], const uint16_t _size)
{
    uint8_t bcc=0;
    uint16_t index=0;
    int data_len=0;
    if(_size<sizeof (struct ZKHY_Frame_upload)) return ZKHY_RESP_ERR_ENCODE_PACKL;
    memset(buf, 0, _size);
    index=0;
    buf[index++] = _frame->STR[0];           
    buf[index++] = _frame->STR[1];           
    buf[index++] = _frame->CMD;              
    buf[index++] = _frame->CNT;                
    buf[index++] = _frame->AVN;                
    index += bigend16_encode(&buf[index], _frame->LEN); 
    //printf("[%s--%d] index:%d \n", __func__, __LINE__, index);
    data_len=0;
    switch(_frame->CMD)
    {
    /******************* 通用指令 ********************/
    case ZKHY_UPLOAD_LOGIN:        // 登入
        data_len = EnFrame_login(&_frame->DAT.login, &buf[index], _size-index-1);
        break;
    case ZKHY_UPLOAD_LOGINA:       // 登入响应
        data_len = EnFrame_login_ack(&_frame->DAT.login_ack, &buf[index], _size-index-1);
        break;
    case ZKHY_UPLOAD_LOGOUT:       // 登出
    case ZKHY_UPLOAD_LOGOUTA:      // 登出响应
        //设备登出及登出响应数据单元为空。
        break;
    case ZKHY_UPLOAD_VIN_REQ:      // 请求VIN码
        data_len = gb_cpy(&buf[index], _frame->DAT.sn, sizeof(_frame->DAT.sn));
        break;
    case ZKHY_UPLOAD_VIN_ACK:      // 请求VIN码响应
        data_len = gb_cpy(&buf[index], _frame->DAT.VIN, sizeof(_frame->DAT.VIN));
        break;
    case ZKHY_UPLOAD_UTC_REQ:      // 校时请求
        //校时数据单元为空
        break;
    case ZKHY_UPLOAD_UTC_ACK:      // 校时请求响应
        data_len = gb_cpy(&buf[index], &_frame->DAT.UTC, sizeof(_frame->DAT.UTC));
        break;
        /******************* 分包下载指令 ********************/
    case ZKHY_UPLOAD_DOWN_REQ:     // 分包下载请求
        data_len = EnFrame_download(&_frame->DAT.download, &buf[index], _size-index-1);
        break;
    case ZKHY_UPLOAD_DOWN_ACK:     // 下发分包数据响应
        data_len = EnFrame_download_ack(&_frame->DAT.download_ack, &buf[index], _size-index-1);
        break;
    case ZKHY_UPLOAD_CFG_REQ:      // 查询配置文件更新
    case ZKHY_UPLOAD_FW_REQ:       // 查询固件更新
    case ZKHY_UPLOAD_FWB_REQ:      // 查询固件块信息
        data_len = EnFrame_upload_req(&_frame->DAT.upload_req, &buf[index], _size-index-1);
        break;
    case ZKHY_UPLOAD_CFG_ACK:      // 下发配置文件更新
    case ZKHY_UPLOAD_FW_ACK:       // 下发固件更新
        //case ZKHY_UPLOAD_CCFG_ACK:     // 下发配置文件更新
        //case ZKHY_UPLOAD_CFW_ACK:      // 下发固件更新
        data_len = EnFrame_upload_ack(&_frame->DAT.upload_ack, &buf[index], _size-index-1);
        break;
    case ZKHY_UPLOAD_FWB_ACK:      // 下发固件块信息
        //case ZKHY_UPLOAD_CFWB_ACK:     // 下发固件块信息
        data_len = EnFrame_upload_ackb(&_frame->DAT.upload_ackb, &buf[index], _size-index-1);
        break;
    case ZKHY_UPLOAD_CCFG_REQ:     // 查询配置文件更新
    case ZKHY_UPLOAD_CFW_REQ:      // 查询固件更新
    case ZKHY_UPLOAD_CFWB_REQ:     // 查询固件块信息
        data_len = EnFrame_upload_req_client(&_frame->DAT.upload_reqc, &buf[index], _size-index-1);
        break;
        /******************* 嵌入式升级 ********************/
    case ZKHY_EMB_SYNC:    // 设备同步
        data_len = EnFrame_Emb_sync(&_frame->DAT.Emb_sync, &buf[index], _size-index-1);
        break;
    case ZKHY_EMB_SYNCA:   // 设备同步响应
        data_len = EnFrame_Emb_synca(&_frame->DAT.Emb_synca, &buf[index], _size-index-1);
        break;
    case ZKHY_EMB_ERASE:   // 擦除设备
        data_len = 1;
        buf[index] = (uint8_t)_frame->DAT.Emb_erase.MemNum;
        break;
    case ZKHY_EMB_ERASEA:  // 擦除设备状态
        data_len = EnFrame_Emb_erasea(&_frame->DAT.Emb_erasea, &buf[index], _size-index-1);
        break;
    case ZKHY_EMB_WRITE:   // 分包写入
        data_len = EnFrame_Emb_write(&_frame->DAT.Emb_write, &buf[index], _size-index-1);
        break;
    case ZKHY_EMB_WRITEA:  // 分包写入状态
        data_len = EnFrame_Emb_writea(&_frame->DAT.Emb_writea, &buf[index], _size-index-1);
        break;
    case ZKHY_EMB_READ:    // 分包读取
        data_len = EnFrame_Emb_read(&_frame->DAT.Emb_read, &buf[index], _size-index-1);
        break;
    case ZKHY_EMB_READA:   // 分包读取返回数据
        data_len = EnFrame_Emb_reada(&_frame->DAT.Emb_reada, &buf[index], _size-index-1);
        break;
    case ZKHY_EMB_BOOT:    // 引导 APP
        data_len = EnFrame_Emb_boot(&_frame->DAT.Emb_boot, &buf[index], _size-index-1);
        break;
    case ZKHY_EMB_BOOTA:   // 引导 APP状态
        data_len = EnFrame_Emb_boota(&_frame->DAT.Emb_boota, &buf[index], _size-index-1);
        break;
    case ZKHY_EMB_REBOOT:  // 复位设备
        data_len = EnFrame_Emb_reboot(&_frame->DAT.Emb_reboot, &buf[index], _size-index-1);
        break;
    case ZKHY_EMB_REBOOTA: // 复位设备状态
        data_len = EnFrame_Emb_reboota(&_frame->DAT.Emb_reboota, &buf[index], _size-index-1);
        break;
    default:
        data_len = 0;
        break;
    }
    if(data_len<0) return ZKHY_RESP_ERR_ENCODE_PACKL;
    bigend16_encode(&buf[index-2], data_len); 
    index += data_len;
    // 倒数第 1  校验码  BYTE 采用 BCC（异或校验）法，校验范围聪明星单元的第一个字节开始，同后一个字节异或，直到校验码前一字节为止，校验码占用一个字节
    bcc = GB_17691_BCC_code(&buf[2], index-2);  // start: 2 byte
    //pr_debug("bcc[%d]:%02X \n", index-1, bcc);
    buf[index++] = bcc;
    return index; // len
}

/**
 * 解码一帧数据
 */
int ZKHY_DeFrame_upload(struct ZKHY_Frame_upload* const _frame, const uint8_t data[], const uint16_t _dsize)
{
    uint8_t bcc=0;
    uint16_t index=0;
    int msg_len = 0;
    int data_len=0;
    //const uint8_t* pdata=NULL;
    index=0;
    _frame->STR[0] = data[index++];
    _frame->STR[1] = data[index++];
    if((ZKHY_UPLOAD_FRAME_STR0!=_frame->STR[0]) || (ZKHY_UPLOAD_FRAME_STR1!=_frame->STR[1])) return ZKHY_RESP_ERR_DECODE_PACKS; // 包头错误
    _frame->CMD = data[index++];                                // 2  命令单元  BYTE  命令单元定义见  表 A2  命令单元定义
    _frame->CNT = data[index++];                                // 20  终端软件版本号  BYTE  终端软件版本号有效值范围 0~255
    _frame->AVN = data[index++];                                // 21  数据加密方式  BYTE
    index += bigend16_merge(&_frame->LEN, data[index], data[index+1]); // 22  数据单元长度  WORD 数据单元长度是数据单元的总字节数，有效范围：0~65531
    //printf("decode_pack_general _dsize:%d | %d \n\n", _dsize, (_frame->data_len+index+1));
    if(_dsize<(_frame->LEN+index+1)) return  ZKHY_RESP_ERR_DECODE_PACKL; // 包长度不够
    // 倒数第 1  校验码  BYTE 采用 BCC（异或校验）法，校验范围聪明星单元的第一个字节开始，同后一个字节异或，直到校验码前一字节为止，校验码占用一个字节
    bcc = GB_17691_BCC_code(&data[2], index+_frame->LEN-2);  // start: 2 byte
    _frame->BCC = data[index+_frame->LEN];
    //printf("BCC[%d]:%02X bcc:%02X \n\n", index+_frame->data_len, _frame->BCC, bcc);
    if(bcc != _frame->BCC) return ZKHY_RESP_ERR_DECODE_PACKBCC;                // BCC 校验错误
    // 数据解谜
    data_len = _frame->LEN;
    //pdata = &data[index];
    // 解码数据
    msg_len = 0;
    //printf("[%s--%d] index:%d \n", __func__, __LINE__, index);
    switch(_frame->CMD)
    {
    case ZKHY_UPLOAD_LOGIN:        // 登入
        msg_len = DeFrame_login(&_frame->DAT.login, &data[index], data_len);
        break;
    case ZKHY_UPLOAD_LOGINA:       // 登入响应
        msg_len = DeFrame_login_ack(&_frame->DAT.login_ack, &data[index], data_len);
        break;
    case ZKHY_UPLOAD_LOGOUT:       // 登出
    case ZKHY_UPLOAD_LOGOUTA:      // 登出响应
        //设备登出及登出响应数据单元为空。
        break;
    case ZKHY_UPLOAD_VIN_REQ:      // 请求VIN码
        msg_len = gb_cpy(_frame->DAT.sn, &data[index], sizeof(_frame->DAT.sn));
        break;
    case ZKHY_UPLOAD_VIN_ACK:      // 请求VIN码响应
        msg_len = gb_cpy(_frame->DAT.VIN, &data[index], sizeof(_frame->DAT.VIN));
        break;
    case ZKHY_UPLOAD_UTC_REQ:      // 校时请求
        //校时数据单元为空
        break;
    case ZKHY_UPLOAD_UTC_ACK:      // 校时请求响应
        msg_len = gb_cpy(&_frame->DAT.UTC, &data[index], sizeof(_frame->DAT.UTC));
        break;
    case ZKHY_UPLOAD_DOWN_REQ:     // 分包下载请求
        msg_len = DeFrame_download(&_frame->DAT.download, &data[index], data_len);
        break;
    case ZKHY_UPLOAD_DOWN_ACK:     // 下发分包数据响应
        msg_len = DeFrame_download_ack(&_frame->DAT.download_ack, &data[index], data_len);
        break;
    case ZKHY_UPLOAD_CFG_REQ:      // 查询配置文件更新
    case ZKHY_UPLOAD_FW_REQ:       // 查询固件更新
    case ZKHY_UPLOAD_FWB_REQ:      // 查询固件块信息
        msg_len = DeFrame_upload_req(&_frame->DAT.upload_req, &data[index], data_len);
        break;
    case ZKHY_UPLOAD_CFG_ACK:      // 下发配置文件更新
    case ZKHY_UPLOAD_FW_ACK:       // 下发固件更新
        //case ZKHY_UPLOAD_CCFG_ACK:     // 下发配置文件更新
        //case ZKHY_UPLOAD_CFW_ACK:      // 下发固件更新
        msg_len = DeFrame_upload_ack(&_frame->DAT.upload_ack, &data[index], data_len);
        break;
    case ZKHY_UPLOAD_FWB_ACK:      // 下发固件块信息
        //case ZKHY_UPLOAD_CFWB_ACK:     // 下发固件块信息
        msg_len = DeFrame_upload_ackb(&_frame->DAT.upload_ackb, &data[index], data_len);
        break;
    case ZKHY_UPLOAD_CCFG_REQ:     // 查询配置文件更新
    case ZKHY_UPLOAD_CFW_REQ:      // 查询固件更新
    case ZKHY_UPLOAD_CFWB_REQ:     // 查询固件块信息
        msg_len = DeFrame_upload_req_client(&_frame->DAT.upload_reqc, &data[index], data_len);
        break;
        /******************* 嵌入式升级 ********************/
    case ZKHY_EMB_SYNC:    // 设备同步
        msg_len = DeFrame_Emb_sync(&_frame->DAT.Emb_sync, &data[index], data_len);
        break;
    case ZKHY_EMB_SYNCA:   // 设备同步响应
        msg_len = DeFrame_Emb_synca(&_frame->DAT.Emb_synca, &data[index], data_len);
        break;
    case ZKHY_EMB_ERASE:   // 擦除设备
        msg_len = 1;
        _frame->DAT.Emb_erase.MemNum = data[index];
        break;
    case ZKHY_EMB_ERASEA:  // 擦除设备状态
        msg_len = DeFrame_Emb_erasea(&_frame->DAT.Emb_erasea, &data[index], data_len);
        break;
    case ZKHY_EMB_WRITE:   // 分包写入
        msg_len = DeFrame_Emb_write(&_frame->DAT.Emb_write, &data[index], data_len);
        break;
    case ZKHY_EMB_WRITEA:  // 分包写入状态
        msg_len = DeFrame_Emb_writea(&_frame->DAT.Emb_writea, &data[index], data_len);
        break;
    case ZKHY_EMB_READ:    // 分包读取
        msg_len = DeFrame_Emb_read(&_frame->DAT.Emb_read, &data[index], data_len);
        break;
    case ZKHY_EMB_READA:   // 分包读取返回数据
        msg_len = DeFrame_Emb_reada(&_frame->DAT.Emb_reada, &data[index], data_len);
        break;
    case ZKHY_EMB_BOOT:    // 引导 APP
        msg_len = DeFrame_Emb_boot(&_frame->DAT.Emb_boot, &data[index], data_len);
        break;
    case ZKHY_EMB_BOOTA:   // 引导 APP状态
        msg_len = DeFrame_Emb_boota(&_frame->DAT.Emb_boota, &data[index], data_len);
        break;
    case ZKHY_EMB_REBOOT:  // 复位设备
        msg_len = DeFrame_Emb_reboot(&_frame->DAT.Emb_reboot, &data[index], data_len);;
        break;
    case ZKHY_EMB_REBOOTA: // 复位设备状态
        msg_len = DeFrame_Emb_reboota(&_frame->DAT.Emb_reboota, &data[index], data_len);
        break;
    default:
        pr_debug("[%s--%d] switch default \n", __func__, __LINE__); //fflush(stdout);
        break;
    }
    //pr_debug("decode_pack_general msg_len %d \n\n", msg_len);
    if(msg_len<0) return msg_len;
    if(msg_len!=data_len) return ZKHY_RESP_ERR_DECODE_DATA;
    index += _frame->LEN;
    // 倒数第 1  校验码  BYTE 采用 BCC（异或校验）法，校验范围聪明星单元的第一个字节开始，同后一个字节异或，直到校验码前一字节为止，校验码占用一个字节
    bcc = GB_17691_BCC_code(&data[2], index-2);  // start: 2 byte
    _frame->BCC = data[index++];
    if(bcc != _frame->BCC) return ZKHY_RESP_ERR_DECODE_PACKBCC;                // BCC 校验错误
    //pr_debug("decode_pack_general return %d \n\n", index);
    return  index;
}

#define ZKHY_FRAME_CMD(cmd)  {(uint8_t)cmd, #cmd}
struct ZKHY_frame_cmd {
    const uint8_t cmd;
    const char* info;
};

static const struct ZKHY_frame_cmd frame_cmd_list[] = {
    // 通用指令
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_LOGIN         ),//= 0x10,       // 0x10  登入                     上行
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_LOGINA        ),//= 0x11,       // 0x11  登入响应                 下行
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_LOGOUT        ),//= 0x12,       // 0x12  登出                     上行  
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_LOGOUTA       ),//= 0x13,       // 0x13  登出响应                 下行  
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_VIN_REQ       ),//= 0x14,       // 0x14  请求VIN码                上行  
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_VIN_ACK       ),//= 0x15,       // 0x15  请求VIN码响应            下行  
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_UTC_REQ       ),//= 0x16,       // 0x16  校时请求                 上行  
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_UTC_ACK       ),//= 0x17,       // 0x17  校时请求响应             下行 
    // 分包下载指令
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_DOWN_REQ      ),//= 0x20,       // 0x20  分包下载请求             上行  
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_DOWN_ACK      ),//= 0x21,       // 0x21  下发分包数据响应         下行 
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_CFG_REQ       ),//= 0x22,       // 0x22  查询配置文件更新         上行  
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_CFG_ACK       ),//= 0x23,       // 0x23  下发配置文件更新         下行 
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_FW_REQ        ),//= 0x24,       // 0x24  查询固件更新             上行  
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_FW_ACK        ),//= 0x25,       // 0x25  下发固件更新             下行 
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_FWB_REQ       ),//= 0x26,       // 0x26  查询固件块信息           上行  
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_FWB_ACK       ),//= 0x27,       // 0x27  下发固件块信息           下行 
    // 分包下载客户(client)指令
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_CCFG_REQ      ),//= 0x30,       // 0x30  查询配置文件更新         上行  
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_CFW_REQ       ),//= 0x32,       // 0x32  查询固件更新             上行  
    ZKHY_FRAME_CMD(ZKHY_UPLOAD_CFWB_REQ      ),//= 0x34,       // 0x34  查询固件块信息           上行  
    // FTP 指令
    // 远程烧号(0x50-0x5F)
    // 嵌入式升级
};
static const int frame_cmd_list_size = sizeof(frame_cmd_list)/sizeof(frame_cmd_list[0]);
const char* ZKHY_Cmd_Info_upload(const enum ZKHY_cmd_Upload cmd)
{
    static const char err[] = "NULL";
    int i;
    for(i=0; i<frame_cmd_list_size; i++)
    {
        if(cmd == frame_cmd_list[i].cmd) return frame_cmd_list[i].info;
    }
    return err;
}

/**
 * 校验包函数
 */
int ZKHY_check_frame(const void* const _data, const uint16_t _dsize)
{
    uint8_t bcc=0;
    uint8_t BCC=0;
    uint16_t index=0;
    uint8_t buf[128];
    struct ZKHY_Frame_upload *const _pack = (struct ZKHY_Frame_upload *)buf;
    const uint8_t* const data = (const uint8_t*)_data;
    index=0;
    _pack->STR[0] = data[index++];
    _pack->STR[1] = data[index++];
    if((ZKHY_UPLOAD_FRAME_STR0!=_pack->STR[0]) || (ZKHY_UPLOAD_FRAME_STR1!=_pack->STR[1])) return ZKHY_RESP_ERR_DECODE_PACKS; // 包头错误
    _pack->CMD = data[index++];                                
    _pack->CNT = data[index++];                       
    _pack->AVN = data[index++];                                
    index += bigend16_merge(&_pack->LEN, data[index], data[index+1]); 
    if(_dsize<(_pack->LEN+index+1)) return  ZKHY_RESP_ERR_DECODE_PACKL; // 包长度不够
    // 倒数第 1  校验码  BYTE 采用 BCC（异或校验）法，校验范围聪明星单元的第一个字节开始，同后一个字节异或，直到校验码前一字节为止，校验码占用一个字节
    bcc = GB_17691_BCC_code(&data[2], index+_pack->LEN-2);  // start: 2 byte
    index += _pack->LEN;
    BCC = data[index++]; 
    if(bcc != BCC) return ZKHY_RESP_ERR_DECODE_PACKBCC;                // BCC 校验错误
    return  index;
}




