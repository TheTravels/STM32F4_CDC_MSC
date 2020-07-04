/************************ (C) COPYLEFT 2018 Merafour *************************

* File Name          : ZKHY_Dev_upload.c
* Author             : Merafour
* Last Modified Date : 05/09/2020
* Description        : 正科环宇设备升级协议编解码测试部分
* Description        : 见《正科环宇设备升级协议.pdf》
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
******************************************************************************/
#include "ZKHY_Dev_upload.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// 编解码测试
void __ZKHY_frame_upload_test(const enum ZKHY_cmd_Upload cmd)
{
    struct ZKHY_Frame_upload frame;
    struct ZKHY_Frame_upload pack;
    uint8_t data[2048];
    int enlen;
    int delen;
    int i;
    const uint8_t* const en = (const uint8_t*)(&frame);
    const uint8_t* const de = (const uint8_t*)(&pack);

    memset(&frame, 0, sizeof(frame));
    memset(&pack, 0, sizeof(pack));
    memset(data, 0, sizeof(data));

    printf("\r\n[%s--%d] cmd[%02X]:%s\r\n", __func__, __LINE__, cmd, ZKHY_Cmd_Info_upload(cmd));
    ZKHY_frame_upload_init(&frame, cmd);
    // encode
    enlen = ZKHY_Dev_Frame_upload(&frame, cmd, data, sizeof(data));
    printf("ENCODE encode[%d] BCC[0x%02X]: ", enlen, frame.BCC);
    for(i=0; i<42; i++)
    {
        printf("%02X ", en[i]);
    }
    printf("\r\n");
#if 0
    printf("DECODE data[%d] BCC[0x%02X]: ", enlen, frame.BCC);
    for(i=0; i<enlen; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\r\n");
#endif
    // decode
    delen = ZKHY_DeFrame_upload(&pack, data, enlen);
    printf("DECODE decode[%d] BCC[0x%02X]: ", delen, pack.BCC);
    for(i=0; i<42; i++)
    {
        printf("%02X ", de[i]);
    }
    printf("\r\n");
    for(i=0; i<(int)sizeof(struct ZKHY_Frame_upload); i++)
    {
        if(en[i] != de[i])
        {
            if((6==i) || (7==i)) continue; // LEN
            break;
        }
    }
    printf("DECODE match[%d | %d] [%02X | %02X] de:%d\r\n", (int)sizeof(struct ZKHY_Frame_upload), i, en[i], de[i], de[i]);
    if(1044!=i)
    {
        printf("\r\n[%s--%d] cmd[%02X]:%s Test ERR! -----------------------------------------\r\n", __func__, __LINE__, cmd, ZKHY_Cmd_Info_upload(cmd));
    }
    // decode
    switch(cmd)
    {
        case ZKHY_UPLOAD_LOGIN:          // 车辆登入  上行
            /*memcpy(frame.data.login.UTC, "123456", 6);
            memcpy(frame.data.login.ICCID, "ICCID1234567890ABCDEFGH", 20);
            frame.data.login.count = 10;
            frame.data_len = 28;*/
            break;
        case ZKHY_UPLOAD_LOGINA:    // 实时信息上报  上行
            break;
            break;
        default:
            //frame.data_len = 1;
            //printf("[%s--%d] switch default \n", __func__, __LINE__); fflush(stdout);
            break;
    }
    printf("frame.data_len:[%d] pack.data_len:[%d]\r\n", frame.LEN, pack.LEN);
    fflush(stdout);
}
void ZKHY_frame_upload_test(void)
{
    int i;
#if 0
    uint8_t cmd_list[] = {
        ZKHY_UPLOAD_LOGIN,
        ZKHY_UPLOAD_LOGINA,
        ZKHY_UPLOAD_LOGOUT,
        ZKHY_UPLOAD_LOGOUTA,
        ZKHY_UPLOAD_VIN_REQ,
        ZKHY_UPLOAD_VIN_ACK,
        ZKHY_UPLOAD_UTC_REQ,
        ZKHY_UPLOAD_UTC_ACK,
        // 分包下载指令
        ZKHY_UPLOAD_DOWN_REQ,
        ZKHY_UPLOAD_DOWN_ACK,
        ZKHY_UPLOAD_CFG_REQ,
        ZKHY_UPLOAD_CFG_ACK,
        ZKHY_UPLOAD_FW_REQ,
        ZKHY_UPLOAD_FW_ACK ,
        ZKHY_UPLOAD_FWB_REQ ,
        ZKHY_UPLOAD_FWB_ACK ,
        // 分包下载客户(client)指令
        ZKHY_UPLOAD_CCFG_REQ ,
        //ZKHY_UPLOAD_CCFG_ACK,
        ZKHY_UPLOAD_CFW_REQ ,
        //ZKHY_UPLOAD_CFW_ACK ,
        ZKHY_UPLOAD_CFWB_REQ,
        //ZKHY_UPLOAD_CFWB_ACK,
    };
#else
    uint8_t cmd_list[] = {
        ZKHY_UPLOAD_CFG_REQ,
        ZKHY_UPLOAD_FW_REQ,
        ZKHY_UPLOAD_FWB_REQ,
        ZKHY_UPLOAD_CCFG_REQ,
        ZKHY_UPLOAD_CFW_REQ,
        ZKHY_UPLOAD_CFWB_REQ,
    };
#endif
    const int cmd_list_size = sizeof(cmd_list)/sizeof(cmd_list[0]);
    for(i=0; i<cmd_list_size; i++)
    {
        __ZKHY_frame_upload_test(cmd_list[i]);
    }
}



