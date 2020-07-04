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
//#include "obd/version.h"
//#include "GB/GB17691_frame.h"
//#include "submodules/Files.h"
#if 0
#define DOWNLOAD_TYPE_CFG    0
#define DOWNLOAD_TYPE_FW     1
#define DOWNLOAD_TYPE_FWB    2

static char filename[128];
static uint8_t buffer[2014];
static struct ZKHY_Frame_download _download;  // 下载信息缓存
static uint32_t server_checksum=0;             // 服务器校验码
static uint32_t server_crc_fw=0;               // 服务器固件校验码
static uint32_t server_crc_cfg=0;              // 服务器配置文件校验码
static uint8_t down_type=0;                    // 0:cfg, 1:fw
static uint8_t down_map[1024/8];               // 下载映射表,1bit表示1block=1024byte

extern char download_fw[2*1024*1024];

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

