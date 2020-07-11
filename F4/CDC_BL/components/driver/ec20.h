/******************** (C) COPYRIGHT 2015 merafour ********************
* File Name          : ec20.h
* Author             : 冷月追风 && Merafour
* Version            : V1.0.0
* Last Modified Date : 07/10/2020
* Description        : 移远 EC20 模块驱动.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
*******************************************************************************/
#ifndef __EC20_H__
#define __EC20_H__

#include <stdint.h>
#include "at.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LENTH_DSN       (12) //设备序列号
#define LENTH_DKEY      (32) //设备密钥
#define LENTH_IMEI      (15) //设备IMEI码
#define LENTH_IMSI      (15) //设备IMEI码
#define LENTH_SIM       (20) //SIM卡号
#define LENTH_MODEL     (10) //模块名
#define LENTH_TOKEN     (32) //令牌
#define LENTH_UID       (32)

enum ec20_module_state{
    EC20_MODULE_RESET = 0,//模块复位
	EC20_MODULE_SET,//模块基本设置，如回显设置
	EC20_MODULE_CHK_REG,//模块手机网络注册查询
	EC20_MODULE_PPP,//模块拨号上网设置
	EC20_MODULE_GET_INFORMATION,//获取模块信息
	EC20_MODULE_CONNECT,//模块连接服务器
	EC20_MODULE_DISCONNECT,//模块断开服务器连接
	EC20_MODULE_IDLE,//模块空闲
	EC20_MODULE_GET_CSQ,//模块信号强度查询
	EC20_MODULE_GET_POSITION,//模块定位信息获取
	EC20_MODULE_SEND,//模块数据发送
	EC20_MODULE_READ,//模块数据接收
	EC20_MODULE_POWER_OFF,//关闭电源
	EC20_MODULE_SET_WIFI,//设置WIFI
	EC20_MODULE_SET_GNSS,//设置定位
	EC20_MODULE_GET_CLIENTS,//获取客户端连接数
	EC20_MODULE_GET_TIME,//获取时间
	EC20_MODULE_SEND2,   //多路TCP
	EC20_MODULE_READ2,   //多路TCP
	EC20_MODULE_FTP_LOGIN,   //
	EC20_MODULE_FTP_DOWNLOAD,   //
};

enum ec20_resp{
    EC20_RESP_OK = 0,//模块复位
	EC20_RESP_ERROR = -1,
};

struct ec20_ofps{
	//char data[1024*4];
	struct at_ofps _at;
	enum ec20_module_state ModuleState;
	union
	{
		uint32_t u32Flags;
		struct
		{
			uint32_t InitOK		:1;//初始化OK指示 可以取硬件相关
			uint32_t CSQOK		:1;//信号强度获取OK指示
			uint32_t Connected		:1;//SOCKET连接OK指示,断开清掉
			uint32_t CellLocate		:1;//基站定位信息获取OK指示
			uint32_t GetLocate		:1;//取位置信息指示 主线程正在读指示
			uint32_t ReConnect		:1;//SOCKET有重连指示
			uint32_t Sending		:1;//发送中
			uint32_t ReqReConnect		:1;//请求Socket重连
			uint32_t ReqPowerOff  :1;//请求模块关机
			uint32_t ReqPowerOn   :1;//请求模块开机
			uint32_t PowerOn      :1;//模块开机或关机状态指示 1 开机 0 关机
	        uint32_t Sended       :1;//有发送过数据
	        uint32_t Opened       :1;//模块已打开
	        uint32_t Testing      :1;//模块测试中
	        uint32_t EnTest       :1;//使能测试
	        uint32_t AutoAnswer   :1;//天线测试,自动接听
	        uint32_t ReqSetWifi   :1;//请求设置WIFI
	        uint32_t SetWifi      :1;//WIFI设置状态
	        uint32_t ReqSetGNSS   :1;//请求设置GPS
	        uint32_t SetGNSS      :1;//GNSS状态
	        uint32_t CPIN         :1;//SIM卡状态
	        uint32_t FTP          :1;//FTP下载
		}b;
	}State;

	struct
	{
		char  NADR[64];		    //服务器网址
	    uint16_t PORT;	        //服务器端口号
		char  APN[32];		    //接入点名称
		char  IP[16];		    //服务器IP地址
		char  NUSER[32];	    //网络登陆用户名
		char  NPWD[32];		    //网络登陆用户密码
		uint8_t mult;           //使用多路 TCP时作为多路 TCP的索引号
	}NetInfo, Net2;                   //连接服务器相关参数
	char IMSI[LENTH_IMSI];
	char IMEI[LENTH_IMEI];
	char SIM[LENTH_SIM];
	char UID[LENTH_UID];
	char Token[LENTH_TOKEN];//令牌
	uint8_t SocketID;//套接字ID
	uint8_t socket2; //多路 TCP 连接
	//GPRS_NET_ADDR Net2; //多路 TCP连接服务器相关参数
	uint8_t Recv_flag;    // 接收标志
};

/*******************************************************************************
 * EC20 模块接口
 *******************************************************************************/
extern enum ec20_resp EC20_Reset(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_Set(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_CheckRegister(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_Ppp(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_GetInformation(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_Connect(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_Disconnect(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_Idle(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_GetCSQ(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_GetPosition(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_Send(struct ec20_ofps* const _ofps, const void* const _data, const uint16_t _len);
extern enum ec20_resp EC20_Read(struct ec20_ofps* const _ofps, void* const _data, const uint16_t _len, uint16_t* const _rlen);
extern enum ec20_resp EC20_PowerOff(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_SetWIFI(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_SetGNSS(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_GetClients(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_GetTime(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_Send2(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_Read2(struct ec20_ofps* const _ofps);

extern enum ec20_resp EC20_Init(void);

extern void EC20_Test(void);

/*******************************************************************************
 * EC20 模块 FTP 接口
 *******************************************************************************/
extern enum ec20_resp EC20_FTP_Login(struct ec20_ofps* const _ofps, const char host[], const int port, const int contextID, const char user[], const char passwd[]);
extern enum ec20_resp FTP_DownLoad_RAM(struct ec20_ofps* const _ofps, const char dir[], const char filename[], const char path[]);
int EC20_FTP_Test(void);



struct ec20_ofps _ec20_ofps;

#ifdef __cplusplus
}
#endif

#endif /* __EC20_H__ */

/************************ (C) COPYRIGHT Merafour *****END OF FILE****/

