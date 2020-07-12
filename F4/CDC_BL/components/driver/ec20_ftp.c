/******************** (C) COPYRIGHT 2015 merafour ********************
* File Name          : ec20_ftp.c
* Author             : 冷月追风 && Merafour
* Version            : V1.0.0
* Last Modified Date : 07/10/2020
* Description        : 移远 EC20 模块驱动 - FTP.
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
#include "stm32f4xx_hal.h"

/*
判断指定字符串是否全为字母数字
*/
extern int checkAlNum(const char *const str, const uint16_t len);

extern char CellLocateData[];
//extern volatile uint8_t DataCompleteFlag;
//extern volatile uint8_t RecvCome;
//extern volatile u32 PositionTimer;
extern void set_utctime(const time_t time);

//static const char ReturnOK[] = "\r\nOK";
//static const char ReturnERROR[] = "\r\nERROR";
//static const char ReturnSENDFAIL[]="SEND FAIL";
//static const char ReturnA[] = ">";
//static const char ReturnRING[] = "\r\nRING\r\n";


//static const char ReturnPdpDeactivation[] = "\r\n+QIURC: \"pdpdeact\",";
//static const char ReturnSocket_closed[] = "\r\n+QIURC: \"closed\",";
//static const char ReturnRecv[] = "\r\n+QIURC: \"recv\",";

enum ec20_resp EC20_FTP_Login(struct ec20_ofps* const _ofps, const char host[], const int port, const int contextID, const char user[], const char passwd[])
{
    // 《Quectel_EC2x&EG9x&EM05_FTP(S)_AT_Commands_Manual_V1.0.pdf》
    // 3.1. Login to FTP Server
	enum ec20_resp ret=EC20_RESP_OK;
    int resp;
    uint8_t state;
    uint8_t err=0;
    uint8_t time_out = 0;
    int err_code=0;

    state = 0;
    err = 3;
    while(err && (state != 0xFF))
    {
        switch(state)
        {
            // Configure the PDP context ID as 1. The PDP context ID must be activated first.
        case 0:
            at_print("AT+QFTPCFG=\"contextid\",%d\r\n", contextID);
            //resp=at_get_resps("\r\nOK\r\n", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,500);
            resp = at_get_resps("\r\nOK\r\n", "\r\n+CME ERROR: ", NULL, 1500, 100, &_ofps->_at);
            //app_debug("[%s-%d] _ofps->_at._rbuf:[%s] \r\n", __func__, __LINE__, _ofps->_at._rbuf);
            if(0 == resp)
            {
                state = 1;
                err = 3;
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR: ", &err_code, ',', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
            }
            break;
            // Step 2: Configure user account and transfer settings.
            // Set user name and password.
        case 1:
            at_print("AT+QFTPCFG=\"account\",\"%s\",\"%s\"\r\n", user, passwd);
            //resp=at_get_resps("\r\nOK\r\n", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,500);
            resp = at_get_resps("\r\nOK\r\n", "\r\n+CME ERROR: ", NULL, 1500, 100, &_ofps->_at);
            //app_debug("[%s-%d] _ofps->_at._rbuf:[%s] \r\n", __func__, __LINE__, _ofps->_at._rbuf);
            if(0 == resp)
            {
                state = 2;
                err = 3;
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR: ", &err_code, ',', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
            }
            break;
            // Set file type as binary.
        case 2:
            at_print("AT+QFTPCFG=\"filetype\",0\r\n");
            //resp=at_get_resps("\r\nOK\r\n", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,500);
            resp = at_get_resps("\r\nOK\r\n", "\r\n+CME ERROR: ", NULL, 1500, 100, &_ofps->_at);
            //app_debug("[%s-%d] _ofps->_at._rbuf:[%s] \r\n", __func__, __LINE__, _ofps->_at._rbuf);
            if(0 == resp)
            {
                state = 3;
                err = 3;
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR: ", &err_code, ',', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
            }
            break;
            // Set transfer mode as passive mode.
        case 3:
            at_print("AT+QFTPCFG=\"transmode\",1\r\n");
            //resp=at_get_resps("\r\nOK\r\n", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,500);
            resp = at_get_resps("\r\nOK\r\n", "\r\n+CME ERROR: ", NULL, 1500, 100, &_ofps->_at);
            //app_debug("[%s-%d] _ofps->_at._rbuf:[%s] \r\n", __func__, __LINE__, _ofps->_at._rbuf);
            if(0 == resp)
            {
                state = 4;
                err = 3;
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR: ", &err_code, ',', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
            }
            break;
            // Set response timeout value.
        case 4:
            at_print("AT+QFTPCFG=\"rsptimeout\",90\r\n");
            //resp=at_get_resps("\r\nOK\r\n", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,500);
            resp = at_get_resps("\r\nOK\r\n", "\r\n+CME ERROR: ", NULL, 1500, 100, &_ofps->_at);
            //app_debug("[%s-%d] _ofps->_at._rbuf:[%s] \r\n", __func__, __LINE__, _ofps->_at._rbuf);
            if(0 == resp)
            {
                state = 5;
                err = 3;
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR: ", &err_code, ',', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
            }
            break;
            // Step 3: Login to FTP server.
            // QFTPOPEN
        case 5:
            at_print("AT+QFTPOPEN=\"%s\",%d\r\n", host, port);
            //resp=at_get_resps("\r\n+QFTPOPEN: ", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,10000);
            resp = at_get_resps("\r\n+QFTPOPEN: ", "\r\n+CME ERROR: ", NULL, 10000, 100, &_ofps->_at);
            //app_debug("[%s-%d] _ofps->_at._rbuf:[%s] \r\n", __func__, __LINE__, _ofps->_at._rbuf);
            if(0 == resp)
            {
                state = 0xFF;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+QFTPOPEN: ", ',', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+QFTPOPEN: ", &err_code, ',', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n\r\n", __func__, __LINE__, err_code);
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR: ", &err_code, ',', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
            }
            break;
        default:
            state = 0;
            break;
        }
        HAL_Delay(30);
    }
    _ofps->State.b.PowerOn = 0;
    _ofps->State.b.Connected = 0;
    _ofps->State.b.ReConnect = 1;
    if(0 == err)
    {
        _ofps->ModuleState = EC20_MODULE_SET;
        _ofps->State.b.Connected = 0;
        _ofps->State.b.ReConnect = 1;
        ret=EC20_RESP_ERROR;
    }
    else
    {
        ret = EC20_RESP_OK;
    }
    return ret;
}
/*******************************************************************************
 * 功能:FTP 下载一个块
 *******************************************************************************/
int FTP_DownLoad_Block(struct ec20_ofps* const _ofps, const char filename[], const int seek, char _data[], const uint16_t _dsize, const uint16_t block)
{
    int resp;
    uint8_t state;
    uint8_t err;
    int transferlen;
    const char* data = NULL;
    const char CONNECT[] = "\r\nCONNECT\r\n";

    //uint16_t maxlen=recv->len;
    transferlen = -1;
    state = 0;
    err = 5;
    while(err && (state != 0xFF))
    {
        // AT+QFTPGET=<file_name>,"COM:"[,<startpos>[,<downloadlen>]]
        at_print("AT+QFTPGET=\"%s\",\"COM:\",%d,%d\r\n", filename, seek, block);
        //resp = GetResp(recv);
        //CONNECT
        //<Output file data>
        //OK
        //+QFTPGET: 0,<transferlen>
        //resp=at_get_resps("\r\nCONNECT\r\n", NULL, NULL, NULL, NULL, 1000,1000);
        resp = at_get_resps(CONNECT, NULL, NULL, 1500, 100, &_ofps->_at);
        if(0 == resp)
        {
            //usb_debug("[%s--%d] _ofps->_at._rbuf[%d]<%s> \r\n", __func__, __LINE__, _ofps->_at._rbufPosition, _ofps->_at._rbuf);
            //usb_debug("[%s--%d] _length<%d> \r\n", __func__, __LINE__, _length);
    		// 查找数据区偏移
    	    data=strstr(_ofps->_at._rbuf, CONNECT);
    	    if(NULL==data)
    	    {
    	    	transferlen = -1;
    	    	break;
    	    }
    	    data += strlen(CONNECT);
            //usb_debug("[%s--%d] _ofps->_at._rbufPosition<%d> _size<%d> \r\n", __func__, __LINE__, _ofps->_at._rbufPosition, _size);
    	    if(at_get_resp_split_int(&data[block], "+QFTPGET: ", &transferlen, ',', 2)>0)
    	    {
                if(transferlen>_dsize) transferlen=_dsize;
                memcpy(_data, data, transferlen);
    	    }
            state = 0xFF;
        }
        else
        {
            err--;
            HAL_Delay(1000);
        }
        HAL_Delay(10);
    }

    if(0 == err)
    {
    	transferlen = -1;
    }
    return transferlen;
}
static int startpos=0;
enum ec20_resp FTP_DownLoad(struct ec20_ofps* const _ofps, const char dir[], const char filename[], const char path[])
{
    // 《Quectel_EC2x&EG9x&EM05_FTP(S)_AT_Commands_Manual_V1.0.pdf》
    // 3.1. Login to FTP Server
	enum ec20_resp ret=EC20_RESP_OK;
    int resp;
    uint8_t state;
    uint8_t err=0;
    uint8_t time_out = 0;
    int err_code=0;
    int ftp_size;
    const int _size=512;
    int block=1024;
    int transferlen;
    char pdat[1460];

    state = 0;
    err = 3;
    while(err && (state != 0xFF))
    {
        switch(state)
        {
        case 0:
            // Set current directory.
            // AT+QFTPCWD="/"
            at_print("AT+QFTPCWD=\"%s\"\r\n", dir);
            //resp=at_get_resps("\r\n+QFTPCWD: ", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,1500);
            resp = at_get_resps("\r\n+QFTPCWD: ", "\r\n+CME ERROR: ", NULL, 1500, 100, &_ofps->_at);
            if(0 == resp)
            {
                state = 1;
                err = 3;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "\r\n+QFTPCWD: ", ',', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "\r\n+QFTPCWD: ", &err_code, ',', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR:", &err_code, ' ', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
            }
            break;
        case 1:
            // Query the size of “test.txt” on FTP(S) server.
            // 获取文件大小
            at_print("AT+QFTPSIZE=\"%s\"\r\n", filename);
            //resp=at_get_resps("\r\n+QFTPSIZE: ", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,1500);
            resp = at_get_resps("\r\n+QFTPSIZE: ", "\r\n+CME ERROR: ", NULL, 1500, 100, &_ofps->_at);
            //app_debug("[%s-%d] _ofps->_at._rbuf:[%s] \r\n", __func__, __LINE__, _ofps->_at._rbuf);
            if(0 == resp)
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "\r\n+QFTPSIZE: ", ',', 1, "%d", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+QFTPSIZE: ", &err_code, ',', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                if(0==err_code)
                {
                    state = 2;
                    err = 3;
                    startpos=0; // 断点续传
                    //at_resp_parse_split_args(_ofps->_at._rbuf, "\r\n+QFTPSIZE: ", ' ', 2, "%d", &ftp_size);
                    at_get_resp_split_int(_ofps->_at._rbuf, "+QFTPSIZE: ", &ftp_size, ',', 2);
                    app_debug("[%s-%d] ftp_size:[%d] \r\n", __func__, __LINE__, ftp_size);
                }
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR:", &err_code, ' ', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
            }
            break;
        case 2: // 下载
            // Solution 1: Output downloaded data directly via COM port.
            // Download a file from FTP(S) server and the data is outputted via COM port.
            // AT+QFTPGET="test.txt","COM:"
            // 分块下载
        	block = ftp_size - startpos;
            if(block>_size) block = _size;
            if(block>(sizeof(_ofps->_at._rbuf)-1)) block = sizeof(_ofps->_at._rbuf)-1;
            transferlen = FTP_DownLoad_Block(_ofps, filename, startpos, pdat, 1024, block);
            if(block==transferlen)
            {
                err = 3;
                startpos += block;
                //file_ll_save(filename, data, transferlen);
                //app_debug("[%s-%d] data<%s> \r\n", __func__, __LINE__, data);
                app_debug("[%s-%d] startpos:%d \tftp_size:%d transferlen:%d\r\n", __func__, __LINE__, startpos, ftp_size, transferlen);
                if(startpos>=ftp_size)
                {
                    app_debug("[%s-%d] download done! \r\n", __func__, __LINE__);
                    state = 0xFF;
                    break;
                }
            }
            else err--;
            break;
        default:
            state = 0;
            break;
        }
        HAL_Delay(30);
    }
    _ofps->State.b.PowerOn = 0;
    _ofps->State.b.Connected = 0;
    _ofps->State.b.ReConnect = 1;
    if(0 == err)
    {
        _ofps->ModuleState = EC20_MODULE_SET;
        _ofps->State.b.Connected = 0;
        _ofps->State.b.ReConnect = 1;
        ret=EC20_RESP_ERROR;
    }
    else
    {
        ret = EC20_RESP_OK;
    }
    return ret;
}
// FTP下载到内存
enum ec20_resp FTP_DownLoad_RAM(struct ec20_ofps* const _ofps, const char dir[], const char filename[], void(*const save_seek)(const int total, const uint32_t _seek, const char data[], const uint16_t block))
{
    // 《Quectel_EC2x&EG9x&EM05_FTP(S)_AT_Commands_Manual_V1.0.pdf》
    // 3.1. Login to FTP Server
	enum ec20_resp ret=EC20_RESP_OK;
    int resp;
    uint8_t state;
    uint8_t err=0;
    uint8_t time_out = 0;
    int err_code=0;
    int ftp_size;
    int startpos=0;
    const int _downloadlen=1024*3;
    int block=1024;
    const char* data=NULL;
    int filehandle = 0;
    const char CONNECT[] = "\r\nCONNECT ";

    state = 0;
    err = 3;
    while(err && (state != 0xFF))
    {
        switch(state)
        {
        case 0:
            // Set current directory.
            // AT+QFTPCWD="/"
        	// 先切换到根目录
            at_print("AT+QFTPCWD=\"/\"\r\n");
            //resp=at_get_resps("\r\n+QFTPCWD: ", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,1500);
            resp = at_get_resps("\r\n+QFTPCWD: ", "\r\n+CME ERROR: ", NULL, 1500, 100, &_ofps->_at);
            // 再切换到工作目录
            at_print("AT+QFTPCWD=\"%s\"\r\n", dir);
            //resp=at_get_resps("\r\n+QFTPCWD: ", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,1500);
            resp = at_get_resps("\r\n+QFTPCWD: ", "\r\n+CME ERROR: ", NULL, 1500, 100, &_ofps->_at);
            //app_debug("[%s-%d] _ofps->_at._rbuf:[%s] \r\n", __func__, __LINE__, _ofps->_at._rbuf);
            if(0 == resp)
            {
                state = 1;
                err = 3;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "\r\n+QFTPCWD: ", ',', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "\r\n+QFTPCWD: ", &err_code, ',', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR:", &err_code, ' ', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
            }
            break;
        case 1:
            // Query the size of “test.txt” on FTP(S) server.
            // 获取文件大小
            at_print("AT+QFTPSIZE=\"%s\"\r\n", filename);
            //resp=at_get_resps("\r\n+QFTPSIZE: ", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,1500);
            resp = at_get_resps("\r\n+QFTPSIZE: ", "\r\n+CME ERROR: ", NULL, 1500, 100, &_ofps->_at);
            if(0 == resp)
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "\r\n+QFTPSIZE: ", ',', 1, "%d", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+QFTPSIZE: ", &err_code, ',', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                if(0==err_code)
                {
                    state = 2;
                    err = 3;
                    startpos=0; // 断点续传
                    //at_resp_parse_split_args(_ofps->_at._rbuf, "\r\n+QFTPSIZE: ", ' ', 2, "%d", &ftp_size);
                    at_get_resp_split_int(_ofps->_at._rbuf, "+QFTPSIZE: ", &ftp_size, ',', 2);
                    app_debug("[%s-%d] ftp_size:[%d] \r\n", __func__, __LINE__, ftp_size);
                    // 生成一个空文件
                    //memset(_ofps->_at._rbuf, 0xFF, sizeof (_ofps->_at._rbuf));
                    //create_empty_path(path, ftp_size, _ofps->_at._rbuf, 1024);
                }
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR:", &err_code, ' ', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
            }
            break;
        case 2: // 下载到内存
            //Solution 2: Save downloaded data to RAM file.
            //Download a file from FTP(S) server and save it to RAM.
            //AT+QFTPGET="test_my1.txt","RAM:test.txt"              //Download file and save it to RAM as “test.txt”.
            at_print("AT+QFTPGET=\"%s\",\"RAM:%s\"\r\n", filename, filename);
            //resp=at_get_resps("\r\n+QFTPGET: ", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,7500);
            resp = at_get_resps("\r\n+QFTPGET: ", "\r\n+CME ERROR: ", NULL, 7500, 100, &_ofps->_at);
            if(0 == resp)
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "\r\n+QFTPSIZE: ", ',', 1, "%d", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+QFTPGET: ", &err_code, ',', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                if(0==err_code)
                {
                    state = 3;
                    err = 3;
                    startpos=0; // 断点续传
                    //at_resp_parse_split_args(_ofps->_at._rbuf, "\r\n+QFTPSIZE: ", ' ', 2, "%d", &ftp_size);
                    at_get_resp_split_int(_ofps->_at._rbuf, "+QFTPGET: ", &ftp_size, ',', 2);
                    app_debug("[%s-%d] ftp_size:[%d] \r\n", __func__, __LINE__, ftp_size);
                    //at_print("AT+QFTPCLOSE\r\n"); // 关闭 FTP
                    //resp = at_get_resps("\r\nOK", NULL, NULL, 1500, 100, &_ofps->_at);
                }
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR:", &err_code, ' ', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
            }
            break;
        case 3: //3.2. Write and Read a File
            // AT+QFLDS="RAM"                                  //Query the space information of RAM.
            at_print("AT+QFLDS=\"%s\"\r\n", "RAM");
            //resp=at_get_resps("\r\n+QFLDS: ", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,1500);
            resp = at_get_resps("\r\n+QFLDS: ", "\r\n+CME ERROR: ", NULL, 1500, 100, &_ofps->_at);
            //AT+QFOPEN="test",0          //Open the file to get the file handle.
            at_print("AT+QFOPEN=\"RAM:%s\",0\r\n", filename);
            //resp=at_get_resps("\r\n+QFOPEN: ", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,1500);
            resp = at_get_resps("\r\n+QFOPEN: ", "\r\n+CME ERROR: ", NULL, 1500, 100, &_ofps->_at);
            if(0 == resp)
            {
                state = 4;
                err = 3;
                at_get_resp_split_int(_ofps->_at._rbuf, "\r\n+QFOPEN: ", &filehandle, ',', 1);
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR: ", &err_code, ',', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
            }
            break;
        case 4:
            //AT+QFSEEK=0,0,0            //Set the file pointer to the beginning of the file.
            at_print("AT+QFSEEK=%d,0,0\r\n", filehandle);
            //resp=at_get_resps("\r\nOK", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,1500);
            resp = at_get_resps("\r\nOK", "\r\n+CME ERROR: ", NULL, 1500, 100, &_ofps->_at);
            if(0 == resp)
            {
                    state = 5;
                    err = 3;
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR: ", &err_code, ',', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
            }
            break;
        case 5:
            // AT+QFREAD=0,10            //Read 10 bytes from the file.
        	block = ftp_size - startpos;
            if(block>_downloadlen) block = _downloadlen;
            at_print("AT+QFREAD=%d,%d\r\n", filehandle, block);
            //resp=at_get_resps("\r\nCONNECT ", NULL, NULL, NULL, NULL, 20,500);
            resp = at_get_resps(CONNECT, NULL, NULL, 500, 40, &_ofps->_at);
            //resp=at_get_resps("\r\n+QFTPGET: ", NULL, NULL, NULL, NULL, 50,1500);
            //resp=GetResponse("+QFTPGET: ", NULL, 3000);
            //app_debug("[%s-%d] _ofps->_at._rbuf:[%s] \r\n", __func__, __LINE__, _ofps->_at._rbuf);
            if(0 == resp)
            {
                int count;
                // 获取数据区指针
                data=at_get_data(_ofps->_at._rbuf, CONNECT);
                if(NULL==data)
                {
                	err--;
                	continue;
                }
                at_get_resp_split_int(_ofps->_at._rbuf, CONNECT, &block, '\r', 1);
                data=at_get_data(data, "\r\n"); // "\r\n"
                if(NULL==data)
                {
                	err--;
                	continue;
                }
                err_code = -1;
                //app_debug("[%s-%d] pdat:[%s] \r\n", __func__, __LINE__, pdat);
                if(0==resp)
                {
                    //save_to_path_seek(path, 0, startpos, data, block);
                	save_seek(ftp_size, startpos, data, block);
                    err = 3;
                    startpos += block;
                    //file_ll_save(filename, data, transferlen);
                    app_debug("[%s-%d] startpos:%d \tftp_size:%d block:%d\r\n", __func__, __LINE__, startpos, ftp_size, block);
                    if(startpos>=ftp_size)
                    {
                        app_debug("[%s-%d] download done! \r\n", __func__, __LINE__);
                        app_debug("[%s-%d] ftp_size:[%d] \r\n", __func__, __LINE__, ftp_size);
                        state = 6;//0xFF;
                        break;
                    }
                }
                else
                {
                    app_debug("[%s-%d] count:%d pdat[%d]:<%s> \r\n", __func__, __LINE__, count, _ofps->_at._rsize, data);
                    err--;
                    //at_get_resp_split_int(&data[downloadlen], "\r\n+QFTPGET: ", &err_code, ',', 1);
                    HAL_Delay(3000);
                }
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                app_debug("[%s-%d] _ofps->_at._rbuf:[%s] \r\n", __func__, __LINE__, _ofps->_at._rbuf);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR: ", &err_code, ' ', 1);
                app_debug("[%s-%d] err:[%d] err_code:[%d] \r\n", __func__, __LINE__, err, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
                HAL_Delay(1000);
            }
            break;
        case 6:
            //AT+QFCLOSE=0            //Close the file.
            at_print("AT+QFCLOSE=%d\r\n", filehandle);
            //resp=at_get_resps("\r\nOK", "\r\n+CME ERROR: ", NULL, NULL, NULL, 100,1500);
            resp = at_get_resps("\r\nOK", "\r\n+CME ERROR: ", NULL, 1500, 100, &_ofps->_at);
            if(0 == resp)
            {
                    state = 0xFF;
                    err = 3;
            }
            else
            {
                err--;
                err_code = -1;
                //at_resp_parse_split_args(_ofps->_at._rbuf, "+CME ERROR:", ' ', 1, "%d,", &err_code);
                at_get_resp_split_int(_ofps->_at._rbuf, "+CME ERROR:", &err_code, ' ', 1);
                app_debug("[%s-%d] err_code:[%d] \r\n", __func__, __LINE__, err_code);
                time_out++;
                if(time_out > 4)
                    state = 0xFF;
            }
            break;
        default:
            state = 0;
            break;
        }
        HAL_Delay(15);
    }
    _ofps->State.b.PowerOn = 0;
    _ofps->State.b.Connected = 0;
    _ofps->State.b.ReConnect = 1;

    if(0 == err)
    {
    	_ofps->ModuleState = EC20_MODULE_SET;
        _ofps->State.b.Connected = 0;
        _ofps->State.b.ReConnect = 1;
        ret=EC20_RESP_ERROR;
    }
    else
    {
        ret = EC20_RESP_OK;
    }
    return ret;
}

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
}
int EC20_FTP_Test(void)
{
    int resp=-1;
    int count;
	uint32_t tick_start = HAL_GetTick();
	uint32_t tick_end;
    startpos=0;
    //EC20_Init();
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
        app_debug("\r\n[%s-%d] FTP_DownLoad_RAM ...\r\n", __func__, __LINE__);
        //resp=FTP_DownLoad_RAM(&_ec20_ofps, "TEST", "OBD.cpp", "0:/OBD.cpp");
        tick_start = HAL_GetTick();
        memset(ini_data, 0, sizeof(ini_data));
        ini_size=0;
        //resp=FTP_DownLoad_RAM(&_ec20_ofps, "EPS418/Debug", "fw.Ini", "0:/OBD.cpp");
        //resp=FTP_DownLoad_RAM(&_ec20_ofps, "EPS418/Debug", "fw.Ini", ini_save_seek);
        resp=FTP_DownLoad_RAM(&_ec20_ofps, "/", "EPS418/Debug/fw.Ini", ini_save_seek);
        //resp=FTP_DownLoad(&_ec20_ofps, "TEST", "OBD.cpp", "0:/OBD.cpp");
        app_debug("\r\n[%s-%d] ini_data[%d]:<\r\n%s>\r\n", __func__, __LINE__, ini_size, ini_data);
    	tick_end = HAL_GetTick();
    	app_debug("[%s-%d] tick_start[%d] tick_start[%d] time[%d] ms\r\n", __func__, __LINE__, tick_start, tick_end, tick_end-tick_start);
        if(EC20_RESP_OK!=resp)
        {
            at_print("AT+QFTPCLOSE\r\n");
            //at_get_resps("\r\nOK", NULL, NULL, NULL, NULL, 100,1500);
            resp = at_get_resps("\r\nOK", NULL, NULL, 1500, 100, &_ec20_ofps._at);
            _ec20_ofps.ModuleState = EC20_MODULE_RESET;
            //_ofps->ModuleState = MODULE_FTP_LOGIN;
            //rt_thread_delay(RT_DELAY(3000));
            break;
        }
        memset(ini_data, 0, sizeof(ini_data));
        ini_size=0;
        resp=FTP_DownLoad_RAM(&_ec20_ofps, "TEST", "OBD.cpp", ini_save_seek);
        //resp=FTP_DownLoad(&_ec20_ofps, "TEST", "OBD.cpp", "0:/OBD.cpp");
    	tick_end = HAL_GetTick();
    	app_debug("[%s-%d] tick_start[%d] tick_start[%d] time[%d] ms\r\n", __func__, __LINE__, tick_start, tick_end, tick_end-tick_start);
        if(EC20_RESP_OK!=resp)
        {
            at_print("AT+QFTPCLOSE\r\n");
            //at_get_resps("\r\nOK", NULL, NULL, NULL, NULL, 100,1500);
            resp = at_get_resps("\r\nOK", NULL, NULL, 1500, 100, &_ec20_ofps._at);
            _ec20_ofps.ModuleState = EC20_MODULE_RESET;
            //_ofps->ModuleState = MODULE_FTP_LOGIN;
            //rt_thread_delay(RT_DELAY(3000));
            break;
        }
        at_print("AT+QFTPCLOSE\r\n");
        //at_get_resps("\r\nOK", NULL, NULL, NULL, NULL, 100,1500);
        resp = at_get_resps("\r\nOK", NULL, NULL, 1500, 100, &_ec20_ofps._at);
    }
    return 0;
}


/************************ (C) COPYRIGHT Merafour *****END OF FILE****/

