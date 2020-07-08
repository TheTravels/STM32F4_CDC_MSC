/************************ (C) COPYLEFT 2018 Merafour *************************

* File Name          : ZKHY_Dev_upload.c
* Author             : Merafour
* Last Modified Date : 05/09/2020
* Description        : 正科环宇设备升级协议设备端
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
#include "Periphs/uart.h"
#include "Periphs/ParamTable.h"
#include "Periphs/Flash.h"
#include "Periphs/crc.h"
#include "tea/tea.h"
#include "usb_device.h"
//#include "obd/version.h"
//#include "GB/GB17691_frame.h"
//#include "submodules/Files.h"
#define DOWNLOAD_TYPE_CFG    0
#define DOWNLOAD_TYPE_FW     1
#define DOWNLOAD_TYPE_FWB    2

#define ZKHY_DEV_FRAME_MASTER     1
#undef  ZKHY_DEV_FRAME_MASTER
#define ZKHY_DEV_FRAME_SLAVE      1
//#undef  ZKHY_DEV_FRAME_SLAVE
#define  ZKHY_emb_iteration       32

static const uint32_t def_key1[4]={0x01020304, 0x05060708, 0x090A0B0C, 0x0D0E0F10};
static uint32_t pc_key2[4]={0x010DCF04, 0x0506BEA8, 0x09033B0C, 0x0D555F10};  // 随机密钥
static uint32_t emb_key3[4]={0x010DCF04, 0x0506BEA8, 0x09033B0C, 0x0D555F10};   // 子密钥，通常位对方发过来的密钥

#ifdef ZKHY_DEV_FRAME_MASTER
static char filename[128];
static uint8_t buffer[2014];
static struct ZKHY_Frame_download _download;  // 下载信息缓存
static uint32_t server_checksum=0;             // 服务器校验码
static uint32_t server_crc_fw=0;               // 服务器固件校验码
static uint32_t server_crc_cfg=0;              // 服务器配置文件校验码
static uint8_t down_type=0;                    // 0:cfg, 1:fw
static uint8_t down_map[1024/8];               // 下载映射表,1bit表示1block=1024byte

static long app_size;
//static const char fw_app_path[] = "app/CCM3310S-T_Code.bin";
extern char download_fw[2*1024*1024];
char Emb_fw_sha[20];

uint32_t Emb_write_seek=0;         // 数据写入偏移
uint32_t Emb_write_total=0;        // 数据写入总大小
static uint16_t Emb_write_block;        // 数据写入长度,不能超过同步指令获取到的block大小


static long fw_read_size(const char *const path, void * const _data);
static void hash_sha1(const char *path, const uint32_t _size, char _sha[20])
{
    char app_fw[512];
    const long block_size = 512;
    long block,offset;
    char sha1[20];
    SHA1_CTX ctx;
    //long _size_fd;
    //_size_fd = file_read_size(path);
    /* calculate hash */
    SHA1Init(&ctx);
    offset=0;
    /*while(offset<_size_fd)
    {
        block = ((_size_fd-offset)>block_size?block_size:(_size_fd-offset));
        memset(app_fw, 0xFF, sizeof(app_fw));
        file_read_data_seek(path, app_size, offset, app_fw, block);
        SHA1Update( &ctx, (unsigned char const *)app_fw, block);
        offset += block;
    }*/
    while(offset<(long)_size)
    {
        block = (((long)_size-offset)>block_size?block_size:((long)_size-offset));
        memset(app_fw, 0xFF, sizeof(app_fw));
        file_read_data_seek(path, app_size, offset, app_fw, block);
        SHA1Update( &ctx, (unsigned char const *)app_fw, block);
        offset += block;
    }
    memset(sha1, 0, sizeof(sha1));
    SHA1Final((unsigned char*)sha1, &ctx);
    /*app_debug("const unsigned char calculate_hash_sha1[20]={");
    for(i=0; i<20; i++)
    {
        app_debug("0x%02x,", sha1[i]&0xFF);
    }
    app_debug("};\r\n");*/
    memcpy(_sha, sha1, sizeof (sha1));
}
#endif

long ZKHY_Slave_upload_init(const char path[])
{
#ifdef ZKHY_DEV_FRAME_MASTER
    //char fw_sha[20];
    tea_rand(pc_key2, 4); // 生成随机密钥
    tea_rand(emb_key3, 4); // 生成随机密钥
    memset(download_fw, 0xFF, sizeof(download_fw));
    //app_size = fw_read_size(fw_app_path, download_fw);
    app_size = fw_read_size(path, download_fw);
    app_debug("[%s-%d] app_size:%d \r\n", __func__, __LINE__, app_size);
    app_debug("[%s-%d] app_size&0x1FF:%d \r\n", __func__, __LINE__, app_size&0x1FF);
    // 对齐
    if((app_size&0x1FF)>0) app_size = app_size-(app_size&0x1FF) + 512;
    if(app_size>512*1024) app_size=512*1024;
    app_debug("[%s-%d] app_size:%d \r\n", __func__, __LINE__, app_size);
    memset(Emb_fw_sha, 0, sizeof(Emb_fw_sha));
    //calculate_hash_sha1(fw_app_path, app_size, Emb_fw_sha);
    hash_sha1(path, app_size, Emb_fw_sha);
    Emb_write_total = app_size;
    return app_size;
#endif

#ifdef ZKHY_DEV_FRAME_SLAVE
    tea_rand(emb_key3, 4); // 生成随机密钥
    return 0;
#endif
}

#ifdef ZKHY_DEV_FRAME_MASTER
static long fw_read_size(const char *const path, void * const _data)
{
    FILE* fd = NULL;
    long _size=0;

    fd = fopen(path, "r");  //
    if(NULL==fd)
    {
        printf("fopen fail!\n");
        return -1;
    }
    fseek(fd, 0, SEEK_END);
    _size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    fread(_data, _size, 1, fd);
    //app_debug("_size[%s] _size :%ld \n", path, _size);
    fclose(fd);
    return _size;
}
// 编码帧
int ZKHY_Dev_Frame_upload(struct ZKHY_Frame_upload* const _frame, const enum ZKHY_cmd_Upload cmd, uint8_t _buf[], const uint16_t _bsize)
{
    int enlen;
    memset(_buf, 0, _bsize);
    memset(_frame, 0, sizeof(struct ZKHY_Frame_upload));
    ZKHY_frame_upload_init(_frame, cmd);
    _frame->AVN = 0x00;    // 协议版本号
    switch(cmd)
    {
    case ZKHY_UPLOAD_LOGIN:        // 登入
    {
        struct ZKHY_Frame_login* const login = &_frame->DAT.login;
        GB_read_SN((uint8_t*)login->sn);
        GB_read_vin((uint8_t*)login->VIN);
        memcpy(login->Model, version.Model, sizeof(login->Model));
        GB_read_ICCID((uint8_t*)login->ICCID);
        memcpy(login->Ver, downloadVersion, strlen(downloadVersion));
    }
        break;
    case ZKHY_UPLOAD_LOGOUT:       // 登出
        //设备登出及登出响应数据单元为空。
        break;
    case ZKHY_UPLOAD_VIN_REQ:      // 请求VIN码
        GB_read_SN((uint8_t*)_frame->DAT.sn);
        break;
    case ZKHY_UPLOAD_UTC_REQ:      // 校时请求
        //校时数据单元为空
        break;
    case ZKHY_UPLOAD_DOWN_REQ:     // 分包下载请求
        memcpy(&_frame->DAT.download, &_download, sizeof(_download));
        break;
    case ZKHY_UPLOAD_CFG_REQ:      // 查询配置文件更新
        GB_read_SN((uint8_t*)&_frame->DAT.upload_req.sn);
        memcpy(&_frame->DAT.upload_req.Model, version.Model, strlen(version.Model));
        _frame->DAT.upload_req.checksum = file_crc16(config_path, _buf, _bsize);
        // 不需要更新
        if((0==server_crc_cfg) || (server_crc_fw==_frame->DAT.upload_req.checksum)) return ZKHY_RESP_ERR_UNDATA;
        break;
    case ZKHY_UPLOAD_FW_REQ:       // 查询固件更新
    case ZKHY_UPLOAD_FWB_REQ:      // 查询固件块信息
        GB_read_SN((uint8_t*)&_frame->DAT.upload_req.sn);
        memcpy(&_frame->DAT.upload_req.Model, version.Model, strlen(version.Model));
        _frame->DAT.upload_req.checksum = file_crc16(fw_path, _buf, _bsize);
        // 不需要更新
        if((0==server_crc_fw) || (server_crc_fw==_frame->DAT.upload_req.checksum)) return ZKHY_RESP_ERR_UNDATA;
        break;
    case ZKHY_UPLOAD_CCFG_REQ:     // 查询配置文件更新
        GB_read_SN((uint8_t*)&_frame->DAT.upload_reqc.sn);
        _frame->DAT.upload_reqc.checksum = file_crc16(config_path, _buf, _bsize);
        // 不需要更新
        if((0==server_crc_cfg) || (server_crc_fw==_frame->DAT.upload_req.checksum)) return ZKHY_RESP_ERR_UNDATA;
        break;
    case ZKHY_UPLOAD_CFW_REQ:      // 查询固件更新
    case ZKHY_UPLOAD_CFWB_REQ:     // 查询固件块信息
        GB_read_SN((uint8_t*)&_frame->DAT.upload_reqc.sn);
        _frame->DAT.upload_reqc.checksum = file_crc16(fw_path, _buf, _bsize);
        // 不需要更新
        if((0==server_crc_fw) || (server_crc_fw==_frame->DAT.upload_req.checksum)) return ZKHY_RESP_ERR_UNDATA;
        break;
        /** ------------------------------- 下行数据,测试用 ------------------------------- **/
    case ZKHY_UPLOAD_LOGINA:       // 登入响应
        break;
    case ZKHY_UPLOAD_LOGOUTA:      // 登出响应
        break;
    case ZKHY_UPLOAD_VIN_ACK:      // 请求VIN码响应
        break;
    case ZKHY_UPLOAD_UTC_ACK:      // 校时请求响应
        break;
    case ZKHY_UPLOAD_DOWN_ACK:     // 下发分包数据响应
        break;
    case ZKHY_UPLOAD_CFG_ACK:      // 下发配置文件更新
        break;
    case ZKHY_UPLOAD_FW_ACK:       // 下发固件更新
        break;
    case ZKHY_UPLOAD_FWB_ACK:      // 下发固件块信息
        break;
    default:
        return ZKHY_RESP_ERR_CMD;
        break;
    }
    enlen = ZKHY_EnFrame_upload(_frame, _buf, _bsize);
    return enlen;
}
// _sub_cmd:读写命令有子命令
extern char Emb_fw_sha[20];
int ZKHY_Dev_Frame_upload_Emb(struct ZKHY_Frame_upload* const _frame, const enum ZKHY_cmd_Upload cmd, const enum Emb_Store_number _sub_cmd, const union upload_Emb_Arg* const Arg, uint8_t _buf[], const uint16_t _bsize)
{
    int enlen;
    memset(_buf, 0, _bsize);
    memset(_frame, 0, sizeof(struct ZKHY_Frame_upload));
    ZKHY_frame_upload_init(_frame, cmd);
    _frame->AVN = 0x00;    // 协议版本号
    switch(cmd)
    {
    /******************* 嵌入式升级 ********************/
    case ZKHY_EMB_SYNC:    // 设备同步
    {
        struct ZKHY_Frame_Emb_sync* const _sync = &_frame->DAT.Emb_sync;
        memcpy(_sync->KK1, def_key1, sizeof(_sync->KK1));
        memcpy(_sync->KK2, pc_key2, sizeof(_sync->KK2));
        //app_debug("[%s--%d]  K1:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, def_key1[0], def_key1[1], def_key1[2], def_key1[3]);
        //app_debug("[%s--%d] KK1:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, _sync->KK1[0], _sync->KK1[1], _sync->KK1[2], _sync->KK1[3]);
        //app_debug("[%s--%d] KK2:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, _sync->KK2[0], _sync->KK2[1], _sync->KK2[2], _sync->KK2[3]);
        // K1密文，协商密钥
        tea_encrypt(_sync->KK1, sizeof(_sync->KK1), pc_key2, ZKHY_emb_iteration);
        // K2密文PC端密钥
        tea_encrypt(_sync->KK2, sizeof(_sync->KK2), def_key1, ZKHY_emb_iteration);
        //app_debug("[%s--%d] KK1:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, _sync->KK1[0], _sync->KK1[1], _sync->KK1[2], _sync->KK1[3]);
        //app_debug("[%s--%d] KK2:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, _sync->KK2[0], _sync->KK2[1], _sync->KK2[2], _sync->KK2[3]);
        // 以0x7F填充，作为同步检测标志，主要用于手动获取设备版本信息
        memset(_sync->flag, 0x7F, sizeof(_sync->flag));
    }
        break;
    case ZKHY_EMB_ERASE:   // 擦除设备
        _frame->DAT.Emb_erase.MemNum = _sub_cmd;  // flash
        break;
        break;
    case ZKHY_EMB_WRITE:   // 分包写入
    {
        struct ZKHY_Frame_Emb_write* const _write = &_frame->DAT.Emb_write;
        _write->MemNum = _sub_cmd;
        _write->total = Emb_write_total;
        _write->seek = Emb_write_seek;
        switch(_sub_cmd)
        {
        case EMB_STORE_FLASH:     // 读写入固件
            _write->block = _write->total-_write->seek;
            _write->block = Emb_write_block;
            if(_write->block>Emb_write_block) _write->block = Emb_write_block;
            //memcpy(_write->data, &download_fw[Emb_write_seek], Emb_write_block);
            memcpy(_write->data, &download_fw[_write->seek], _write->block);
            //Emb_write_seek += Emb_write_block;
            break;
        case EMB_STORE_OTP:
            _write->total = Emb_write_total;
            _write->seek = 0;
            _write->block = Emb_write_block;
            break;
        case EMB_STORE_KEY:       // 读写入的密钥
            _write->total = Emb_write_total;
            _write->seek = 0;
            memset(_write->key, 0x12, sizeof(_write->key));
            _write->block = sizeof(_write->key);
            break;
        case EMB_STORE_PARAM:     // 读参数表
            _write->total = Emb_write_total;
            _write->seek = 0;
            _write->block = strlen((char*)Arg->param);
            if(_write->block>sizeof(Arg->param)) _write->block=sizeof(Arg->param);
            memcpy(_write->data, Arg->param, _write->block);
            break;
        case EMB_STORE_UART:  // 读串口, seek为串口号
            //if((EMB_STORE_UART==Emb_write_MemNum) && (0==_write->total))
            _write->block = Arg->at.len;
            if(0==_write->total)
            {
                _write->uart.BaudRate = Arg->uart.BaudRate;
                _write->uart.DataWidth = Arg->uart.DataWidth;
                _write->uart.StopBits = Arg->uart.StopBits;
                _write->uart.Parity = Arg->uart.Parity;
            }
            else memcpy(_write->data, Arg->at.data, _write->block);
            break;
        case EMB_STORE_UID:       // 芯片ID
        case EMB_STORE_FLASI_SPI:
        case EMB_STORE_FLASH_SDIO:
        case EMB_STORE_ROM1:
        case EMB_STORE_ROM2:
        case EMB_STORE_ROM3:
        case EMB_STORE_CHIP:
        default:
            _write->total = 0;
            _write->seek = 0;
            _write->block = 0;
            break;
        }
    }
        break;
    case ZKHY_EMB_READ:    // 分包读取
    {
        struct ZKHY_Frame_Emb_read* const _read = &_frame->DAT.Emb_read;
        //Emb_write_MemNum = _sub_cmd;
        _read->MemNum = _sub_cmd;
        _read->seek = Emb_write_seek;
        _read->block = Emb_write_block;
        switch(_sub_cmd)
        {
        case EMB_STORE_FLASH:     // 读写入固件
            break;
        case EMB_STORE_OTP:
            _read->seek = 0;
            _read->block = Emb_write_block;
            break;
        case EMB_STORE_UID:       // 芯片ID
            _read->seek = 0;
            _read->block = Emb_write_block;
            break;
        case EMB_STORE_KEY:       // 读写入的密钥
            _read->seek = 0;
            _read->block = Emb_write_block;
            break;
        case EMB_STORE_PARAM:     // 读参数表
            _read->seek = 0;
            _read->block = Emb_write_block;
            break;
        case EMB_STORE_UART:  // 读串口, seek为串口号
            _read->seek = 1;
            _read->block = Emb_write_block;
            break;
        case EMB_STORE_FLASI_SPI:
        case EMB_STORE_FLASH_SDIO:
        case EMB_STORE_ROM1:
        case EMB_STORE_ROM2:
        case EMB_STORE_ROM3:
        case EMB_STORE_CHIP:
        default:
            _read->seek = 0;
            _read->block = 0;
            break;
        }
    }
        break;
    case ZKHY_EMB_BOOT:    // 引导 APP
    {
        struct ZKHY_Frame_Emb_boot* const _boot = &_frame->DAT.Emb_boot;
        unsigned short crc16 = 0;
        crc16 = 0;
        crc16 = fast_crc16(crc16, (unsigned char *)download_fw, app_size);
        _boot->total = Emb_write_total;
        _boot->crc = crc16;
        memcpy(_boot->data, Emb_fw_sha, sizeof(_boot->data));
    }
        break;
    case ZKHY_EMB_REBOOT:  // 复位设备
    {
        struct ZKHY_Frame_Emb_reboot* const _reboot = &_frame->DAT.Emb_reboot;
        _reboot->C1 = 0xA5A5A5A5;
        _reboot->C2 = 0x5A5A5A5A;
    }
        break;
    default:
        return ZKHY_RESP_ERR_CMD;
        break;
    }
    enlen = ZKHY_EnFrame_upload(_frame, _buf, _bsize);
    return enlen;
}

static inline int ZKHY_upload_query(struct ZKHY_Frame_upload_ack* const _qure_ack)
{
    int suc;
    memset(down_map, 0, sizeof(down_map));
    if( (0!=_qure_ack->total) && (strlen(_qure_ack->key)>strlen("/*.bin")+1) ) // update
    {
        server_checksum=_qure_ack->checksum;
        memset(&_download, 0, sizeof(_download));
        memcpy(_download.key, _qure_ack->key, strlen(_qure_ack->key));
        _download.seek = 0;
        _download.total = _qure_ack->total;
        _download.block = FRAME_DATA_LEN;
        if(_download.block>_download.total) _download.block=_download.total;
        server_checksum = _qure_ack->checksum;
        // 生成一个空文件
        //create_empty_path(download_cache_path, _download.total, buffer, 1024);
#if 0
        memset(buffer, 0xFF, sizeof (buffer));
        file_create_empty(download_cache_path, _download.total, buffer, 1024);
#else
        memset(download_fw, 0xFF, sizeof(download_fw));
#endif
        memset(filename, 0, sizeof (filename));
        memcpy(filename, _qure_ack->key, strlen((const char *)_qure_ack->key));
        app_debug("Download: %s\r\n", filename);
        //mkdir("./upload");
        //rename(filename, "./upload/Download.dat");
        suc = ZKHY_RESP_ERR_UPLOAD;
        app_debug("[%s--%d] _download.key:%s \r\n", __func__, __LINE__, _download.key);
    }
    else  suc = ZKHY_RESP_SUC;
    app_debug("[%s-%d] total:%d crc:0x%08X Key:%s\n", __func__, __LINE__, _qure_ack->total, _qure_ack->checksum, _qure_ack->key);
    return suc;
}
//解码帧
int ZKHY_Dev_unFrame_upload(struct ZKHY_Frame_upload* const _frame, const enum ZKHY_cmd_Upload cmd, const  uint8_t data[], const uint16_t _dsize)
{
    int delen;
    int suc;
    //uint8_t status;                 // 状态码
    memset(_frame, 0, sizeof(struct ZKHY_Frame_upload));
    delen = ZKHY_DeFrame_upload(_frame, (uint8_t*)data, _dsize);
    //print_hex(__func__, __LINE__, "", data, delen);
    app_debug("[%s--%d] delen:%d cmd:0x%02X _frame->CMD:0x%02X \r\n", __func__, __LINE__, delen, cmd, _frame->CMD);
    if(cmd!=_frame->CMD) return ZKHY_RESP_ERR_CMD;
    if(delen<=0) return ZKHY_RESP_ERR_PACK;
    suc = ZKHY_RESP_ERR_CMD;
    // 4G (0x10-0x3F)
    //if((_frame->CMD>=0x10) && (_frame->CMD<=0x3F)) suc = ZKHY_Dev_unFrame_upload_4G(_frame);
    // Emb (0x60-0x6F)
    //else if((_frame->CMD>=0x60) && (_frame->CMD<=0x6F)) suc = ZKHY_Dev_unFrame_upload_Emb(_frame);
    switch(_frame->CMD)
    {
        case ZKHY_UPLOAD_LOGINA:       // 登入响应
            // 判断是否需要升级
            if((0==_frame->DAT.login_ack.fw) || (0==_frame->DAT.login_ack.cfg))
            {
                suc = ZKHY_RESP_ERR_UNDATA;
                break;
            }
            server_crc_fw = _frame->DAT.login_ack.fw;
            server_crc_cfg = _frame->DAT.login_ack.cfg;
            suc = ZKHY_RESP_SUC;
            break;
        case ZKHY_UPLOAD_LOGOUTA:      // 登出响应
            suc = ZKHY_RESP_SUC;
            break;
        case ZKHY_UPLOAD_VIN_ACK:      // 请求VIN码响应
            suc = ZKHY_RESP_ERR_DATA;
            if(17==_frame->LEN)
            {
                //save_vin(_frame->DAT.VIN);
                app_debug("[%s--%d] _frame->DAT.VIN[%s] \r\n", __func__, __LINE__, _frame->DAT.VIN);
                suc = ZKHY_RESP_SUC;
            }
            break;
        case ZKHY_UPLOAD_UTC_ACK:      // 校时请求响应
            {
                uint8_t UTC[6];
                //server_sync_time(GB_UTC((uint8_t*)_frame->DAT.UTC));  // 更新本地时间
                memcpy(UTC, _frame->DAT.UTC, 6);
                app_debug("[%s--%d] GB_GMT8[%d-%d-%d %02d:%02d:%02d] \r\n", __func__, __LINE__, UTC[0]+2000, UTC[1], UTC[2], UTC[3], UTC[4], UTC[5]);
                suc = ZKHY_RESP_SUC;
            }
            break;
        case ZKHY_UPLOAD_DOWN_ACK:     // 下发分包数据响应
            suc = ZKHY_RESP_ERR_UPLOAD;
            {
                uint32_t checksum;
                uint32_t _size=0;
                const struct ZKHY_Frame_download_ack* const _ack_download = &_frame->DAT.download_ack;
                app_debug("[%s-%d] userdef DownLoad: total:%d seek:%d size:%d\r\n", __func__, __LINE__, _ack_download->total, _ack_download->seek, _ack_download->block);
                if( (0!=_ack_download->total) && (0!=_ack_download->block) && (_download.block==_ack_download->block) ) // update
                {
                    /*app_debug("[%s-%d] _ack_download->block[%d]: \r\n", __func__, __LINE__, _ack_download->block);
                    for(int i=0; i<_ack_download->block; i++)
                    {
                        app_debug("0x%02X, ", _ack_download->data[i]&0xFF);
                    }
                    app_debug("\r\n");*/
                    //save_to_file(filename, decode->pack_index, decode->data, decode->down_len);
                    //save_to_path_seek(download_cache_path, 0, _ack_download->seek, _ack_download->data, _ack_download->block);
                    memcpy(&download_fw[_ack_download->seek], _ack_download->data, _ack_download->block);
                    _download.seek = _download.seek + _ack_download->block;
                    while(_download.seek<_download.total)
                    {
                        int _nframe;
                        uint8_t map;
                        _nframe = _download.seek/FRAME_DATA_LEN;
                        map = down_map[_nframe>>3];
                        map = map>>(_nframe&0x07);
                        if(1!=(map&0x1)) break; // 需要下载
                        _download.seek += FRAME_DATA_LEN;
                    }
                    _size = _download.total - _download.seek;
                    if(_size>FRAME_DATA_LEN) _size = FRAME_DATA_LEN;
                    if((0==_size) || (_download.total<=_download.seek))  // download done
                    {
                        const char* _path = NULL;
                        //file_save(download_cache_path, download_fw, _ack_download->_total);
                        memset(buffer, 0, sizeof (buffer));
                        if(DOWNLOAD_TYPE_CFG==down_type) _path = download_cfg_temp;
                        // fw
                        else _path = download_fw_temp;
#if 1
                        file_save(download_cache_path, download_fw, _ack_download->total);
#endif
                        // 匹配校验码
                        //server_checksum=decode->checksum;
                        checksum = file_crc16(download_cache_path, buffer, sizeof (buffer));
                        app_debug("[%s-%d] Download checksum: 0x%04X 0x%04X\r\n", __func__, __LINE__, checksum, server_checksum);
                        if(checksum==server_checksum)
                        {
                            _file_rename(download_cache_path, _path);
                        }
                        //app_debug("\n\nDownload  copy_to_file\n"); fflush(stdout);
                        //copy_to_file("./upload/Download.dat", filename);
                        //save_to_file("./upload/Download.dat", 0, decode->data, 0);
                        app_debug("\n\nDownload Done!\n");
                        app_debug("[%s-%d] Time:%s Download Done! checksum: 0x%04X 0x%04X\r\n", __func__, __LINE__, GB_GMT8_Format(), checksum, server_checksum);
                        suc = ZKHY_RESP_SUC;
                    }
                    else
                    {
                        _download.block = _size;
                    }
                }
                else suc = ZKHY_RESP_ERR_CMD;
            }
            break;
        case ZKHY_UPLOAD_CFG_ACK:      // 下发配置文件更新
            app_debug("update Config: total:%d\n", _frame->DAT.upload_ack.total);
            down_type = DOWNLOAD_TYPE_CFG;
            suc = ZKHY_upload_query(&_frame->DAT.upload_ack);
            break;
        case ZKHY_UPLOAD_FW_ACK:       // 下发固件更新
            //memset(&_download, 0, sizeof(struct ZKHY_Frame_download));
            //memcpy(_download.key, _frame->DAT.upload_ack.key, sizeof(_download.key));
            //_download.total = _frame->DAT.upload_ack.total;
            //server_checksum = _frame->DAT.upload_ack.checksum;
            app_debug("update Firmware: total:%d\n", _frame->DAT.upload_ack.total);
            down_type = DOWNLOAD_TYPE_FW;
            suc = ZKHY_upload_query(&_frame->DAT.upload_ack);
            break;
        case ZKHY_UPLOAD_FWB_ACK:      // 下发固件块信息
            {
                const struct ZKHY_Frame_upload_ackb* const _qureb_ack =  &_frame->DAT.upload_ackb;
                app_debug("userdef Firmware\n");
                app_debug("userdef Firmware: total:%d\n", _qureb_ack->total);
                down_type = DOWNLOAD_TYPE_FW;
                int _nframe, i, j, m;
                uint32_t _block;
                uint16_t block;
                uint8_t map;
                //uint8_t _mask;
                uint8_t _bmask;
                memset(down_map, 0, sizeof(down_map));
                //down_map_index=0;
                if( (0!=_qureb_ack->total) && (strlen(_qureb_ack->key)>strlen("/*.bin")+1) ) // update
                {
                    server_checksum=_qureb_ack->checksum;
                    memset(&_download, 0, sizeof(_download));
                    memcpy(_download.key, _qureb_ack->key, strlen(_qureb_ack->key));
                    //down_type = 1;
                    _download.seek = 0;
                    _download.total = _qureb_ack->total;
                    _download.block = FRAME_DATA_LEN;
                    if(_download.block>_download.total) _download.block=_download.total;
                    // 处理下载映射表
                    block = _qureb_ack->block*1024;
                    _nframe = block/FRAME_DATA_LEN;
                    if(_nframe<1) _nframe = 1; // 帧数. 服务器端 block对应本地 FRAME_DATA_LEN块的块数量
                    i=0; j=0;
                    // 映射表换算.将服务器端 block 大小的块映射表换算成本地 FRAME_DATA_LEN 大小的块映射表
                    for(_block=0; _block<_qureb_ack->total; _block+=block)
                    {
                        map = _qureb_ack->map[j>>3]; // j/8
                        _bmask = (map>>(j&0x07)) & 0x1; // 0/1
                        //down_map[i>>3] |= (_bmask<<(i&0x07)); i++;
                        for(m=0; m<_nframe; m++)
                        {
                            down_map[i>>3] |= (_bmask<<(i&0x07));
                            i++;
                        }
                        j++;
                    }
                    // 生成一个空文件
                    memset(buffer, 0xFF&_qureb_ack->value, sizeof (buffer));
                    //create_empty_path(download_cache_path, _download.total, buffer, 1024);
#if 0
                    memset(buffer, 0xFF, sizeof (buffer));
                    file_create_empty(download_cache_path, _download.total, buffer, 1024);
#else
                    memset(download_fw, 0xFF, sizeof(download_fw));
#endif
                    //memset(download_fw, 0xFF, sizeof(download_fw));
                    memset(filename, 0, sizeof (filename));
                    memcpy(filename, _qureb_ack->key, strlen(_qureb_ack->key));
                    app_debug("Download: %s\r\n", filename);
                    suc = ZKHY_RESP_ERR_UPLOAD;
                }
                else  suc = ZKHY_RESP_SUC;
            }
            break;
        default:
            //suc = ZKHY_RESP_ERR_CMD;
            break;
    }
    //app_debug("[%s-%d] enlen[%d] des [%s]: \r\n", __func__, __LINE__, enlen, des);
    //print_hex(__func__, __LINE__, "", _buf, enlen);
    return suc;
}
// 嵌入式升级
int ZKHY_Dev_unFrame_upload_Emb(struct ZKHY_Frame_upload* const _frame, const enum ZKHY_cmd_Upload cmd, const  uint8_t data[], const uint16_t _dsize)
{
    int delen;
    int suc;
    //uint8_t status;                 // 状态码
    memset(_frame, 0, sizeof(struct ZKHY_Frame_upload));
    delen = ZKHY_DeFrame_upload(_frame, (uint8_t*)data, _dsize);
    //print_hex(__func__, __LINE__, "", data, delen);
    app_debug("[%s--%d] delen:%d cmd:0x%02X _frame->CMD:0x%02X \r\n", __func__, __LINE__, delen, cmd, _frame->CMD);
    if(cmd!=_frame->CMD) return ZKHY_RESP_ERR_CMD;
    if(delen<=0) return ZKHY_RESP_ERR_PACK;
    suc = ZKHY_RESP_ERR_CMD;
    switch(_frame->CMD)
    {
    /******************* 嵌入式升级 ********************/
    case ZKHY_EMB_SYNCA:   // 设备同步响应
    {
        uint32_t K1[4];   //
        struct ZKHY_Frame_Emb_synca* const _synca = &_frame->DAT.Emb_synca;
        memcpy(K1, _synca->Kk1, sizeof(_synca->Kk1));
        memcpy(emb_key3, _synca->Kk3, sizeof(_synca->Kk3));
        //app_debug("[%s--%d] KK1:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, K1[0], K1[1], K1[2], K1[3]);
        //app_debug("[%s--%d] KK3:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, emb_key3[0], emb_key3[1], emb_key3[2], emb_key3[3]);
        //app_debug("[%s--%d] tea_decrypt...\r\n", __func__, __LINE__);
        tea_decrypt(emb_key3, sizeof(emb_key3), def_key1, ZKHY_emb_iteration);
        tea_decrypt(K1, sizeof(K1), emb_key3, ZKHY_emb_iteration);
        //app_debug("[%s--%d]  K1:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, def_key1[0], def_key1[1], def_key1[2], def_key1[3]);
        //app_debug("[%s--%d] KK1:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, K1[0], K1[1], K1[2], K1[3]);
        //app_debug("[%s--%d] KK3:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, emb_key3[0], emb_key3[1], emb_key3[2], emb_key3[3]);
        //
        //app_debug("[%s--%d] boot_ver:[%s]\r\n", __func__, __LINE__, _synca->boot_ver);
        //app_debug("[%s--%d] app_ver:[%s]\r\n", __func__, __LINE__, _synca->app_ver);
        //app_debug("[%s--%d] app_ver:[", __func__, __LINE__);
        //for(int i=0; i<32; i++) app_debug("%02X ", _synca->app_ver[i]&0xFF);
        //app_debug("]\r\n");
        //app_debug("[%s--%d] ID:[", __func__, __LINE__);
        //for(int i=0; i<32; i++) app_debug("%02X ", _synca->ID[i]&0xFF);
        //app_debug("]\r\n");
        //app_debug("[%s--%d] CRC:0x%08X _synca->block:%d \r\n", __func__, __LINE__, _synca->crc, _synca->block);
        // 匹配密钥
        if(0!=memcmp(K1, def_key1, sizeof(def_key1))) suc = ZKHY_RESP_SUC;
        Emb_write_block = _synca->block;
        Emb_write_seek = 0;
    }
        break;
    case ZKHY_EMB_ERASEA:  // 擦除设备状态
    {
        struct ZKHY_Frame_Emb_erasea* const _erasea = &_frame->DAT.Emb_erasea;
        //_erasea->MemNum = _sub_cmd;
        _erasea->volume = 1024;
        _erasea->erase = 1024;
        _erasea->status = 0;
    }
        break;
        break;
    case ZKHY_EMB_WRITEA:  // 分包写入状态
    {
        struct ZKHY_Frame_Emb_writea* const _writea = &_frame->DAT.Emb_writea;
#if 0
        app_debug("[%s--%d] _writea->MemNum:%d\r\n", __func__, __LINE__, _writea->MemNum);
        app_debug("[%s--%d] _writea->volume:%d\r\n", __func__, __LINE__, _writea->volume);
        app_debug("[%s--%d] _writea->write:%d\r\n", __func__, __LINE__, _writea->write);
        app_debug("[%s--%d] _writea->status:%d\r\n", __func__, __LINE__, _writea->status);
#endif
        suc = ZKHY_RESP_ERR_UNDATA;
        if(0==_writea->status)
        {
            suc = ZKHY_RESP_ERR_UPLOAD;
            Emb_write_seek = _writea->write;
            // 下载完成
            if(Emb_write_seek==Emb_write_total) suc = ZKHY_RESP_SUC;
        }
    }
        break;
    case ZKHY_EMB_READA:   // 分包读取返回数据
    {
#if 0
        struct ZKHY_Frame_Emb_reada* const _reada = &_frame->DAT.Emb_reada;
        app_debug("[%s--%d] _reada->MemNum:%d\r\n", __func__, __LINE__, _reada->MemNum);
        app_debug("[%s--%d] _reada->volume:%d\r\n", __func__, __LINE__, _reada->volume);
        app_debug("[%s--%d] _reada->status:%d\r\n", __func__, __LINE__, _reada->status);
        app_debug("[%s--%d] _reada->seek:%d\r\n", __func__, __LINE__, _reada->seek);
        app_debug("[%s--%d] _reada->block:%d\r\n", __func__, __LINE__, _reada->block);
#endif
        suc = ZKHY_RESP_SUC;
    }
        break;
    case ZKHY_EMB_BOOTA:   // 引导 APP状态
    {
        struct ZKHY_Frame_Emb_boota* const _boota = &_frame->DAT.Emb_boota;
        //_boota->status = 0x00; // 状态：0成功、1固件错误、2校验失败、3其它错误
        suc = ZKHY_RESP_SUC;
    }
        break;
    case ZKHY_EMB_REBOOTA: // 复位设备状态
    {
        struct ZKHY_Frame_Emb_reboota* const _reboota = &_frame->DAT.Emb_reboota;
        _reboota->status = 0x00; // 状态：0成功、1不支持复位、2其它错误
        memset(_reboota->info, 0x00, sizeof(_reboota->info));
    }
        break;
    default:
        //suc = ZKHY_RESP_ERR_CMD;
        break;
    }
    //app_debug("[%s-%d] enlen[%d] des [%s]: \r\n", __func__, __LINE__, enlen, des);
    //print_hex(__func__, __LINE__, "", _buf, enlen);
    return suc;
}
#endif

#ifdef ZKHY_DEV_FRAME_SLAVE
#include "version.h"
// 编码帧
#if 0
int ZKHY_Slave_Frame_upload(struct ZKHY_Frame_upload* const _frame, const enum ZKHY_cmd_Upload cmd, uint8_t _buf[], const uint16_t _bsize)
{
    int enlen;
    memset(_buf, 0, _bsize);
    memset(_frame, 0, sizeof(struct ZKHY_Frame_upload));
    ZKHY_frame_upload_init(_frame, cmd);
    _frame->AVN = 0x00;    // 协议版本号
    switch(cmd)
    {
        /******************* 嵌入式升级 ********************/
    case ZKHY_EMB_SYNC:    // 设备同步
    {
        struct ZKHY_Frame_Emb_sync* const _sync = &_frame->DAT.Emb_sync;
        _sync->KK1[0] = 0x12345678;
        memset(_sync->flag, 0x7F, sizeof(_sync->flag));
    }
        break;
    case ZKHY_EMB_SYNCA:   // 设备同步响应
    {
        struct ZKHY_Frame_Emb_synca* const _synca = &_frame->DAT.Emb_synca;
        memcpy(_synca->Kk1, def_key1, sizeof(_synca->Kk1));
        memcpy(_synca->Kk3, emb_key3, sizeof(_synca->Kk3));
        // K1密文，协商密钥
        tea_encrypt(_synca->Kk1, 4, def_key1, ZKHY_emb_iteration);
        // K2密文PC端密钥
        tea_encrypt(_synca->Kk3, 4, pc_key2, ZKHY_emb_iteration);
        memcpy(_synca->boot_ver, Emb_Version.version, sizeof(Emb_Version.version));
        memset(_synca->app_ver, 0x7F, sizeof(_synca->app_ver));
        memset(_synca->ID, 0x00, sizeof(_synca->ID));
        memcpy(_synca->ID, (char*)0x1FFF7A10, 12);  // 96 bit
        _synca->crc = 0;
        _synca->block = 1024;
    }
        break;
    case ZKHY_EMB_ERASE:   // 擦除设备
        _frame->DAT.Emb_erase.MemNum = EMB_STORE_FLASH;  // flash
        break;
    case ZKHY_EMB_ERASEA:  // 擦除设备状态
    {
        struct ZKHY_Frame_Emb_erasea* const _erasea = &_frame->DAT.Emb_erasea;
        _erasea->MemNum = EMB_STORE_FLASH;
        _erasea->volume = 1024;
        _erasea->erase = 1024;
        _erasea->status = 0;
    }
        break;
    case ZKHY_EMB_WRITE:   // 分包写入
    {
        struct ZKHY_Frame_Emb_write* const _write = &_frame->DAT.Emb_write;
        _write->MemNum = EMB_STORE_FLASH;
        _write->block = 1024;
        memset(_write->data, 0x7F, sizeof(_write->data));
    }
        break;
    case ZKHY_EMB_WRITEA:  // 分包写入状态
    {
        struct ZKHY_Frame_Emb_writea* const _writea = &_frame->DAT.Emb_writea;
        _writea->MemNum = EMB_STORE_FLASH;
        _writea->volume = 1024;
        _writea->write = 1024;
        _writea->status = 0;
    }
        break;
    case ZKHY_EMB_READ:    // 分包读取
    {
        struct ZKHY_Frame_Emb_read* const _read = &_frame->DAT.Emb_read;
        _read->MemNum = EMB_STORE_FLASH;
        _read->seek = 0;
        _read->block = 1024;
    }
        break;
    case ZKHY_EMB_READA:   // 分包读取返回数据
    {
        struct ZKHY_Frame_Emb_reada* const _reada = &_frame->DAT.Emb_reada;
        _reada->volume = EMB_STORE_FLASH;
        _reada->status = 0;
        _reada->seek = 1024;
        _reada->block = 1024;
        memset(_reada->data, 0x7F, sizeof(_reada->data));
    }
        break;
    case ZKHY_EMB_BOOT:    // 引导 APP
    {
        struct ZKHY_Frame_Emb_boot* const _boot = &_frame->DAT.Emb_boot;
        _boot->total = 0x12345678;
        _boot->crc = 0x12345678;
        memset(_boot->data, 0x7F, sizeof(_boot->data));
    }
        break;
    case ZKHY_EMB_BOOTA:   // 引导 APP状态
    {
        struct ZKHY_Frame_Emb_boota* const _boota = &_frame->DAT.Emb_boota;
        _boota->status = 0x00; // 状态：0成功、1固件错误、2校验失败、3其它错误
        memset(_boota->info, 0x7F, sizeof(_boota->info));
    }
        break;
    case ZKHY_EMB_REBOOT:  // 复位设备
        break;
    case ZKHY_EMB_REBOOTA: // 复位设备状态
    {
        struct ZKHY_Frame_Emb_reboota* const _reboota = &_frame->DAT.Emb_reboota;
        _reboota->status = 0x00; // 状态：0成功、1不支持复位、2其它错误
        memset(_reboota->info, 0x7F, sizeof(_reboota->info));
    }
        break;
    default:
        return ZKHY_RESP_ERR_CMD;
        break;
    }
    enlen = ZKHY_EnFrame_upload(_frame, _buf, _bsize);
    return enlen;
}
#endif
// 用于返回数据
static uint8_t _ccm bl_buf[1024*4];
static uint8_t read_uid(uint8_t uid[])
{
	const char* const id_addr = (const char*)0x1FFF7A10;
	memcpy(uid, id_addr, 12);  // 96 bit
	return 12;
}
//解码帧
extern USBD_StatusTypeDef USBD_DeInit(USBD_HandleTypeDef *pdev);
// 用于在升级固件的时候保存中断向量表信息
static uint32_t pfnVectors[2]={0x00, 0x00};
int ZKHY_Slave_unFrame_upload(struct ZKHY_Frame_upload* const _frame, const  uint8_t data[], const uint16_t _dsize, int (*const send_func)(const uint8_t data[], const uint32_t _size))
{
	int enlen;
    int delen;
    int suc;
    //uint8_t status;                 // 状态码
    memset(_frame, 0, sizeof(struct ZKHY_Frame_upload));
    delen = ZKHY_DeFrame_upload(_frame, data, _dsize);
    //print_hex(__func__, __LINE__, "", data, delen);
    //if(cmd!=_frame->CMD) return ZKHY_RESP_ERR_CMD;
    if(delen<=0) return ZKHY_RESP_ERR_PACK;
    suc = ZKHY_RESP_ERR_CMD;
    enlen = 0;
    switch(_frame->CMD)
    {
    /******************* 嵌入式升级 ********************/
    case ZKHY_EMB_SYNC:    // 设备同步
    {
        int i;
        uint32_t K1[4];   //
        struct ZKHY_Frame_Emb_sync* const _sync = &_frame->DAT.Emb_sync;
        // 0x08010000 为  App 地址
        const struct Emb_Device_Version* const _Version = (const struct Emb_Device_Version*)0x08010000;
        suc = ZKHY_RESP_SUC;
        memcpy(K1, _sync->KK1, sizeof(_sync->KK1));
        memcpy(pc_key2, _sync->KK2, sizeof(_sync->KK2));
        //app_debug("[%s--%d] KK1:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, K1[0], K1[1], K1[2], K1[3]);
        //app_debug("[%s--%d] KK2:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, pc_key2[0], pc_key2[1], pc_key2[2], pc_key2[3]);
        //app_debug("[%s--%d] tea_decrypt...\r\n", __func__, __LINE__);
        tea_decrypt(pc_key2, sizeof(pc_key2), def_key1, ZKHY_emb_iteration);
        tea_decrypt(K1, sizeof(K1), pc_key2, ZKHY_emb_iteration);
        //app_debug("[%s--%d]  K1:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, def_key1[0], def_key1[1], def_key1[2], def_key1[3]);
        //app_debug("[%s--%d] KK1:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, K1[0], K1[1], K1[2], K1[3]);
        //app_debug("[%s--%d] KK2:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, pc_key2[0], pc_key2[1], pc_key2[2], pc_key2[3]);
        // 匹配密钥
        if(0!=memcmp(K1, def_key1, sizeof(def_key1))) suc = ZKHY_RESP_ERR_CMD;//return ZKHY_RESP_ERR_CMD;
        //app_debug("[%s--%d] match 0x7F ...\r\n", __func__, __LINE__);
        // 匹配 0x7F
        for(i=0; i<sizeof(_sync->flag); i++)
        {
            if(0x7F!=_sync->flag[i])
            {
            	//app_debug("[%s--%d] match fail! %d 0x%02X ...\r\n", __func__, __LINE__, i, _sync->flag[i]);
            	suc = ZKHY_RESP_ERR_CMD;
            	break;//return ZKHY_RESP_ERR_CMD;
            }
        }
        //app_debug("[%s--%d] ACK ZKHY_EMB_SYNCA ...\r\n", __func__, __LINE__);
        // 返回数据
        //enlen = ZKHY_Slave_Frame_upload(_frame, ZKHY_EMB_SYNCA, bl_buf, sizeof(bl_buf));
        // ACK
        {
            struct ZKHY_Frame_Emb_synca* const _synca = &_frame->DAT.Emb_synca;
            memset(_frame, 0, sizeof(struct ZKHY_Frame_upload));
            ZKHY_frame_upload_init(_frame, ZKHY_EMB_SYNCA);
            memcpy(_synca->Kk1, def_key1, sizeof(_synca->Kk1));
            memcpy(_synca->Kk3, emb_key3, sizeof(_synca->Kk3));
            // K1密文，协商密钥
            tea_encrypt(_synca->Kk1, 4, def_key1, ZKHY_emb_iteration);
            // K2密文PC端密钥
            tea_encrypt(_synca->Kk3, 4, pc_key2, ZKHY_emb_iteration);
            if(ZKHY_RESP_ERR_CMD==suc)
            {
            	memset(_synca->Kk1, 0, sizeof(_synca->Kk1));
            	memset(_synca->Kk3, 0, sizeof(_synca->Kk3));
            }
            memcpy(_synca->boot_ver, Emb_Version.version, sizeof(Emb_Version.version));
            memcpy(_synca->app_ver, _Version->version, sizeof(_synca->app_ver));
            //memset(_synca->ID, 0x00, sizeof(_synca->ID));
            //memcpy(_synca->ID, (char*)0x1FFF7A10, 12);  // 96 bit
            read_uid(_synca->ID);
            _synca->crc = 0;
            _synca->block = 1024;
        }
        //app_debug("[%s--%d] ACK:%d\r\n", __func__, __LINE__, enlen);
        //suc = ZKHY_RESP_SUC;
    }
    break;
    case ZKHY_EMB_ERASE:   // 擦除设备
    {
        struct ZKHY_Frame_Emb_erasea* const _erasea = &_frame->DAT.Emb_erasea;
    	const enum Emb_Store_number MemNum = _frame->DAT.Emb_erase.MemNum;
        memset(_frame, 0, sizeof(struct ZKHY_Frame_upload));
        ZKHY_frame_upload_init(_frame, ZKHY_EMB_ERASEA);
        if(EMB_STORE_FLASH==MemNum) // flash
        {
        	_erasea->volume = 256*1024;
        	_erasea->erase = 256*1024;
        	_erasea->status = 0;
        	FLASH_Erase(param_flash_start, param_flash_start+param_flash_size-1);
        }
        else
        {
        	_erasea->volume = 0;
        	_erasea->erase = 0;
        	_erasea->status = 2; // 擦除状态：0成功、1擦除中、2存储区不支持、3参数错误、4其它错误
        }
    }
        break;
    case ZKHY_EMB_ERASEA:  // 擦除设备状态
    {
        struct ZKHY_Frame_Emb_erasea* const _erasea = &_frame->DAT.Emb_erasea;
        _erasea->MemNum = EMB_STORE_FLASH;
        _erasea->volume = 1024;
        _erasea->erase = 1024;
        _erasea->status = 0;
    }
    break;
    case ZKHY_EMB_WRITE:   // 分包写入
    {
        struct ZKHY_Frame_Emb_write Write;
        memcpy(&Write, &_frame->DAT.Emb_write, sizeof(struct ZKHY_Frame_Emb_write));
        // ACK
        {
            struct ZKHY_Frame_Emb_writea* const _writea = &_frame->DAT.Emb_writea;
            memset(_frame, 0, sizeof(struct ZKHY_Frame_upload));
            ZKHY_frame_upload_init(_frame, ZKHY_EMB_WRITEA);
            _writea->MemNum = Write.MemNum;
            switch(Write.MemNum)
            {
            case EMB_STORE_FLASH:     // 读写入固件
            {
            	int len = 0;
            	_writea->volume = 256*1024;
            	if(0==Write.seek) // 保存中断向量表
            	{
            		memcpy(pfnVectors, Write.data, 8);
            		memset(Write.data, 0xFF, 8);
            	}
            	len = param_write_flash(Write.data, Write.seek, Write.block);
            	if(len==Write.block)
            	{
                	_writea->status = 0;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
                	_writea->write = Write.seek + Write.block;
            	}
            	else
            	{
                	_writea->status = 4;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
                	_writea->write = Write.seek;
            	}
            }
            	break;
            case EMB_STORE_OTP:
            	_writea->volume = 0;
            	_writea->status = 2;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
            	_writea->write = 0;
            	break;
            case EMB_STORE_KEY:       // 读写入的密钥
            	_writea->status = 0;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
            	_writea->write = 0;
            	_writea->volume = param_write_key(Write.key);
            	break;
            case EMB_STORE_PARAM:     // 读参数表
            	_writea->status = 0;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
            	_writea->write = ParamTable_Write(Write.data, Write.block);
            	_writea->volume = ParamTable_Size();
            	break;
            case EMB_STORE_UART:  // 读串口, seek为串口号
            	if(1==Write.seek)  // UART1
            	{
            		_writea->status = 0;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
            		_writea->write = uart1_send(Write.data, Write.block);
            		_writea->volume = _writea->write;
            	}
            	else if(2==Write.seek)  // UART1
            	{
            		_writea->status = 0;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
                	_writea->write = uart2_send(Write.data, Write.block);
                	_writea->volume = _writea->write;
            	}
            	else if(3==Write.seek)  // UART1
            	{
            		_writea->status = 0;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
                	_writea->write = uart3_send(Write.data, Write.block);
                	_writea->volume = _writea->write;
            	}
            	else
            	{
            		_writea->status = 2;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
            		_writea->write = 0;
            		_writea->volume = 0;
            	}
            	break;
            case EMB_STORE_UID:
            case EMB_STORE_FLASI_SPI:
            case EMB_STORE_FLASH_SDIO:
            case EMB_STORE_ROM1:
            case EMB_STORE_ROM2:
            case EMB_STORE_ROM3:
            case EMB_STORE_CHIP:
            default:
            	_writea->volume = 0;
            	_writea->status = 2; // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
            	_writea->write = 0;
            	break;
            }
            suc = ZKHY_RESP_SUC;
        }
    }
    break;
    case ZKHY_EMB_WRITEA:  // 分包写入状态
    {
        struct ZKHY_Frame_Emb_writea* const _writea = &_frame->DAT.Emb_writea;
        _writea->MemNum = EMB_STORE_FLASH;
        _writea->volume = 1024;
        _writea->write = 1024;
        _writea->status = 0;
    }
    break;
    case ZKHY_EMB_READ:    // 分包读取
    {
    	struct ZKHY_Frame_Emb_read Read;
        memcpy(&Read, &_frame->DAT.Emb_read, sizeof(struct ZKHY_Frame_Emb_read));
        // ACK
        {
            struct ZKHY_Frame_Emb_reada* const _reada = &_frame->DAT.Emb_reada;
            memset(_frame, 0, sizeof(struct ZKHY_Frame_upload));
            ZKHY_frame_upload_init(_frame, ZKHY_EMB_READA);
            _reada->MemNum = Read.MemNum;
            app_debug("[%s--%d] Read.MemNum:%d\r\n", __func__, __LINE__, Read.MemNum);
            switch(Read.MemNum)
            {
            case EMB_STORE_FLASH:     // 读写入固件
            	_reada->volume = 256*1024;  // 96 bit
            	_reada->status = 0;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
            	_reada->seek = Read.seek;
            	_reada->block = param_read_flash(_reada->data, Read.seek, Read.block);
            	break;
            case EMB_STORE_OTP:
            	_reada->volume = 0;
            	_reada->status = 2;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
            	_reada->seek = 0;
            	_reada->block = 0;
            	break;
            case EMB_STORE_UID:       // 芯片ID
            	_reada->status = 0;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
            	_reada->seek = 0;
            	_reada->block = read_uid(_reada->data);
            	_reada->volume = _reada->block;
            	break;
            case EMB_STORE_KEY:       // 读写入的密钥
            	_reada->status = 0;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
            	_reada->seek = 0;
            	_reada->block = param_read_key(_reada->key);
            	_reada->volume = _reada->block;
            	break;
            case EMB_STORE_PARAM:     // 读参数表
            	_reada->status = 0;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
            	_reada->seek = 0;
            	_reada->block = ParamTable_Read(_reada->data, Read.block);
            	_reada->volume = ParamTable_Size();
            	break;
            case EMB_STORE_UART:  // 读串口, seek为串口号
            	_reada->seek = Read.seek;
            	if(1==Read.seek)  // UART1
            	{
                	_reada->status = 0;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
                	_reada->block = uart1_read(_reada->data, Read.block);
                	_reada->volume = _reada->block;
            	}
            	else if(2==Read.seek)  // UART1
            	{
                	_reada->status = 0;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
                	_reada->block = uart2_read(_reada->data, Read.block);
                	_reada->volume = _reada->block;
            	}
            	else if(3==Read.seek)  // UART1
            	{
                	_reada->status = 0;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
                	_reada->block = uart3_read(_reada->data, Read.block);
                	_reada->volume = _reada->block;
            	}
            	else
            	{
                	_reada->status = 2;   // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
                	_reada->block = 0;
                	_reada->volume = 0;
            	}
            	break;
            case EMB_STORE_FLASI_SPI:
            case EMB_STORE_FLASH_SDIO:
            case EMB_STORE_ROM1:
            case EMB_STORE_ROM2:
            case EMB_STORE_ROM3:
            case EMB_STORE_CHIP:
            default:
            	_reada->volume = 0;
            	_reada->status = 2; // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
            	_reada->seek = 0;
            	_reada->block = 0;
            	break;
            }
            suc = ZKHY_RESP_SUC;
        }
    }
    break;
    case ZKHY_EMB_BOOT:    // 引导 APP
    {
    	struct ZKHY_Frame_Emb_boot Boot;
    	memcpy(&Boot, &_frame->DAT.Emb_boot, sizeof(struct ZKHY_Frame_Emb_boot));
        struct ZKHY_Frame_Emb_boota* const _boota = &_frame->DAT.Emb_boota;
        memset(_frame, 0, sizeof(struct ZKHY_Frame_upload));
        ZKHY_frame_upload_init(_frame, ZKHY_EMB_WRITEA);
        unsigned short crc16 = 0;
        crc16 = 0;
        crc16 = fast_crc16(crc16, (const unsigned char*)pfnVectors, 8);
        crc16 = fast_crc16(crc16, (const unsigned char*)(param_flash_start+8), Boot.total-8);
        if(Boot.crc==crc16)
        {
        	Flash_Write_Force(param_flash_start, pfnVectors, 8);
        	_boota->status = 0x00; // 状态：0成功、1固件错误、2校验失败、3其它错误
            memset(bl_buf, 0, sizeof(bl_buf));
            enlen = ZKHY_EnFrame_upload(_frame, bl_buf, sizeof(bl_buf));
            if((NULL!=send_func) && (enlen>0))
            {
            	app_debug("[%s--%d] ACK:%d\r\n", __func__, __LINE__, enlen);
            	send_func(bl_buf, enlen);
            	HAL_Delay(500); // 100 B
            	USBD_DeInit(&hUsbDeviceFS);
            }
        	// boot
        }
        else
        {
        	const char info[] = "Reset instruction error!";
        	_boota->status = 0x02; // 状态：0成功、1固件错误、2校验失败、3其它错误
            memcpy(_boota->info, info, sizeof(info));
        }
    }
    break;
    case ZKHY_EMB_BOOTA:   // 引导 APP状态
    {
        struct ZKHY_Frame_Emb_boota* const _boota = &_frame->DAT.Emb_boota;
        _boota->status = 0x00; // 状态：0成功、1固件错误、2校验失败、3其它错误
        memset(_boota->info, 0x7F, sizeof(_boota->info));
    }
    break;
    case ZKHY_EMB_REBOOT:  // 复位设备
    {
    	struct ZKHY_Frame_Emb_reboot Reboot;
    	memcpy(&Reboot, &_frame->DAT.Emb_reboot, sizeof(struct ZKHY_Frame_Emb_reboot));
        struct ZKHY_Frame_Emb_reboota* const _reboota = &_frame->DAT.Emb_reboota;
        memset(_frame, 0, sizeof(struct ZKHY_Frame_upload));
        ZKHY_frame_upload_init(_frame, ZKHY_EMB_WRITEA);
        if((0xA5A5A5A5==Reboot.C1) && (0x5A5A5A5A==Reboot.C2))
        {
        	const char info[] = "Reboot now!";
            _reboota->status = 0x00; // 状态：0成功、1不支持复位、2其它错误
            memcpy(_reboota->info, info, sizeof(info));
        }
        else
        {
        	const char info[] = "Reset instruction error!";
            _reboota->status = 0x02; // 状态：0成功、1不支持复位、2其它错误
            memcpy(_reboota->info, info, sizeof(info));
        }
        memset(bl_buf, 0, sizeof(bl_buf));
        enlen = ZKHY_EnFrame_upload(_frame, bl_buf, sizeof(bl_buf));
        if((NULL!=send_func) && (enlen>0))
        {
        	app_debug("[%s--%d] ACK:%d\r\n", __func__, __LINE__, enlen);
        	send_func(bl_buf, enlen);
        	HAL_Delay(500); // 100 B
        	USBD_DeInit(&hUsbDeviceFS);
        }
        NVIC_SystemReset();
    }
        break;
    case ZKHY_EMB_REBOOTA: // 复位设备状态
    break;
    default:
        //suc = ZKHY_RESP_ERR_CMD;
        break;
    }
    //app_debug("[%s-%d] enlen[%d] des [%s]: \r\n", __func__, __LINE__, enlen, des);
    //print_hex(__func__, __LINE__, "", _buf, enlen);
    memset(bl_buf, 0, sizeof(bl_buf));
    enlen = ZKHY_EnFrame_upload(_frame, bl_buf, sizeof(bl_buf));
    if((NULL!=send_func) && (enlen>0))
    {
    	app_debug("[%s--%d] ACK:%d\r\n", __func__, __LINE__, enlen);
    	send_func(bl_buf, enlen);
    }
    return suc;
}
#endif

