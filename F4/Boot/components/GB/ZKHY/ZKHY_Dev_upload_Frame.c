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
#include "tea/tea.h"
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
struct ZKHY_Frame_Emb_write Emb_write;

void ZKHY_Slave_upload_init(void)
{
#ifdef ZKHY_DEV_FRAME_MASTER
    char fw_sha[20];
    tea_rand(pc_key2, 4); // 生成随机密钥
    tea_rand(emb_key3, 4); // 生成随机密钥
    memset(download_fw, 0, sizeof(download_fw));
    app_size = fw_read_size(fw_app_path, download_fw);

    // 对齐
    if((app_size&0x1FF)>0) app_size = app_size-(app_size&0x1FF) + 512;
    if(app_size>512*1024) app_size=512*1024;
    app_debug("[%s-%d] app_size:%d \r\n", __func__, __LINE__, app_size);
    memset(fw_sha, 0, sizeof(fw_sha));
    calculate_hash_sha1(fw_app_path, app_size, fw_sha);
#endif

#ifdef ZKHY_DEV_FRAME_SLAVE
    tea_rand(emb_key3, 4); // 生成随机密钥
#endif
}

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
static const char fw_app_path[] = "app/CCM3310S-T_Code.bin";
extern char download_fw[2*1024*1024];
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
        /******************* 嵌入式升级 ********************/
    case ZKHY_EMB_SYNC:    // 设备同步
    {
        struct ZKHY_Frame_Emb_sync* const _sync = &_frame->DAT.Emb_sync;
        memcpy(_sync->KK1, def_key1, sizeof(_sync->KK1));
        memcpy(_sync->KK2, pc_key2, sizeof(_sync->KK2));
        // K1密文，协商密钥
        tea_encrypt(_sync->KK1, 4, pc_key2, ZKHY_emb_iteration);
        // K2密文PC端密钥
        tea_encrypt(_sync->KK2, 4, def_key1, ZKHY_emb_iteration);
        // 以0x7F填充，作为同步检测标志，主要用于手动获取设备版本信息
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
#ifndef ZKHY_DEV_FRAME_SLAVE
        memset(_synca->boot_ver, 0x7F, sizeof(_synca->boot_ver));
        memset(_synca->app_ver, 0x7F, sizeof(_synca->app_ver));
        memset(_synca->ID, 0x7F, sizeof(_synca->ID));
#else
        memset(_synca->boot_ver, 0x7F, sizeof(_synca->boot_ver));
#endif
        _synca->CRC = 0;
        _synca->block = 1024;
    }
        break;
    case ZKHY_EMB_ERASE:   // 擦除设备
        _frame->DAT.Emb_erase.MemNum = Emb_write.MemNum;  // flash
        break;
    case ZKHY_EMB_ERASEA:  // 擦除设备状态
    {
        struct ZKHY_Frame_Emb_erasea* const _erasea = &_frame->DAT.Emb_erasea;
        _erasea->MemNum = Emb_write.MemNum;
        _erasea->volume = 1024;
        _erasea->erase = 1024;
        _erasea->status = 0;
    }
        break;
    case ZKHY_EMB_WRITE:   // 分包写入
    {
        struct ZKHY_Frame_Emb_write* const _write = &_frame->DAT.Emb_write;
        _write->MemNum = Emb_write.MemNum;
        _write->total = Emb_write.total;
        _write->seek = Emb_write.seek;
        Emb_write.seek += Emb_write.block;
        _write->block = Emb_write.block;
        if((EMB_STORE_UART==Emb_write.MemNum) && (0==_write->total))
        {
            _write->uart.BaudRate = Emb_write.uart.BaudRate;
            _write->uart.DataWidth = Emb_write.uart.DataWidth;
            _write->uart.StopBits = Emb_write.uart.StopBits;
            _write->uart.Parity = Emb_write.uart.Parity;
        }
        else memcpy(_write->data, &download_fw[_write->seek], _write->block);
    }
        break;
    case ZKHY_EMB_WRITEA:  // 分包写入状态
    {
        struct ZKHY_Frame_Emb_writea* const _writea = &_frame->DAT.Emb_writea;
        _writea->MemNum = Emb_write.MemNum;
        _writea->volume = 1024;
        _writea->write = Emb_write.seek;
        _writea->status = 0;
    }
        break;
    case ZKHY_EMB_READ:    // 分包读取
    {
        struct ZKHY_Frame_Emb_read* const _read = &_frame->DAT.Emb_read;
        _read->MemNum = Emb_write.MemNum;
        _read->seek = Emb_write.seek;
        _read->block = Emb_write.block;
    }
        break;
    case ZKHY_EMB_READA:   // 分包读取返回数据
    {
        struct ZKHY_Frame_Emb_reada* const _reada = &_frame->DAT.Emb_reada;
        _reada->volume = Emb_write.MemNum;
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
        memset(_boota->info, 0x00, sizeof(_boota->info));
    }
        break;
    case ZKHY_EMB_REBOOT:  // 复位设备
        break;
    case ZKHY_EMB_REBOOTA: // 复位设备状态
    {
        struct ZKHY_Frame_Emb_reboota* const _reboota = &_frame->DAT.Emb_reboota;
        _reboota->status = 0x00; // 状态：0成功、1不支持复位、2其它错误
        memset(_reboota->info, 0x00, sizeof(_reboota->info));
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
    if(cmd!=_frame->CMD) return ZKHY_RESP_ERR_CMD;
    if(delen<=0) return ZKHY_RESP_ERR_PACK;
    suc = ZKHY_RESP_ERR_CMD;
    switch(cmd)
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
#endif

#ifdef ZKHY_DEV_FRAME_SLAVE
#include "version.h"
// 编码帧
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
// 用于返回数据
static uint8_t _ccm bl_buf[1024*4];
//解码帧
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
        memcpy(K1, _sync->KK1, sizeof(_sync->KK1));
        memcpy(pc_key2, _sync->KK2, sizeof(_sync->KK2));
        app_debug("[%s--%d] KK1:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, K1[0], K1[1], K1[2], K1[3]);
        app_debug("[%s--%d] KK2:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, pc_key2[0], pc_key2[1], pc_key2[2], pc_key2[3]);
        app_debug("[%s--%d] tea_decrypt...\r\n", __func__, __LINE__);
        tea_decrypt(pc_key2, sizeof(pc_key2), def_key1, ZKHY_emb_iteration);
        tea_decrypt(K1, sizeof(K1), pc_key2, ZKHY_emb_iteration);
        app_debug("[%s--%d]  K1:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, def_key1[0], def_key1[1], def_key1[2], def_key1[3]);
        app_debug("[%s--%d] KK1:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, K1[0], K1[1], K1[2], K1[3]);
        app_debug("[%s--%d] KK2:[0x%08X 0x%08X 0x%08X 0x%08X]\r\n", __func__, __LINE__, pc_key2[0], pc_key2[1], pc_key2[2], pc_key2[3]);
        // 匹配密钥
        if(0!=memcmp(K1, def_key1, sizeof(def_key1))) return ZKHY_RESP_ERR_CMD;
        app_debug("[%s--%d] match 0x7F ...\r\n", __func__, __LINE__);
        // 匹配 0x7F
        for(i=0; i<sizeof(_sync->flag); i++)
        {
            if(0x7F!=_sync->flag[i])
            {
            	app_debug("[%s--%d] match fail! %d 0x%02X ...\r\n", __func__, __LINE__, i, _sync->flag[i]);
            	return ZKHY_RESP_ERR_CMD;
            }
        }
        app_debug("[%s--%d] ACK ZKHY_EMB_SYNCA ...\r\n", __func__, __LINE__);
        // 返回数据
        enlen = ZKHY_Slave_Frame_upload(_frame, ZKHY_EMB_SYNCA, bl_buf, sizeof(bl_buf));
        app_debug("[%s--%d] ACK:%d\r\n", __func__, __LINE__, enlen);
        suc = ZKHY_RESP_SUC;
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
        //suc = ZKHY_RESP_ERR_CMD;
        break;
    }
    //app_debug("[%s-%d] enlen[%d] des [%s]: \r\n", __func__, __LINE__, enlen, des);
    //print_hex(__func__, __LINE__, "", _buf, enlen);
    if((NULL!=send_func) && (enlen>0))
    {
    	app_debug("[%s--%d] ACK:%d\r\n", __func__, __LINE__, enlen);
    	send_func(bl_buf, enlen);
    }
    return suc;
}
#endif

