/******************** (C) COPYRIGHT 2015 merafour ********************
* File Name          : ec20.c
* Author             : 冷月追风 && Merafour
* Version            : V1.0.0
* Last Modified Date : 07/10/2020
* Description        : 移远 EC20 模块驱动.
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

static const char ReturnOK[] = "\r\nOK";
static const char ReturnERROR[] = "\r\nERROR";
static const char ReturnSENDFAIL[]="SEND FAIL";
static const char ReturnA[] = ">";

static const char ReturnPdpDeactivation[] = "\r\n+QIURC: \"pdpdeact\",";
static const char ReturnSocket_closed[] = "\r\n+QIURC: \"closed\",";
static const char ReturnRecv[] = "\r\n+QIURC: \"recv\",";

//static char _ccm __attribute__ ((aligned (4))) ec20_data[1024*4];

static struct ec20_ofps _ccm __attribute__ ((aligned (4))) _ec20_ofps = {
		._at = {
				.uart_send = uart1_send,
				.uart_read = uart1_read,
				.uart_size = uart1_size,
				.uart_is_empty = uart1_isempty,
		},
		.NetInfo = {
				.NADR = "39.108.51.99",
				.PORT = 9919,
				.APN  = "UNIM2M.NJM2MAPN",
				.IP   = "",
				.NUSER = "",
				.NPWD = "",
				.mult = 0,
		},
};

/*
判断指定字符串是否全为字母数字
*/
int checkAlNum(const char *const str, const uint16_t len)
{
	uint16_t i;
	for(i=0;i<len;i++)
	{
		if(0==isalnum(str[i])) return 0;
	}
	return 1;
}

enum ec20_resp EC20_Reset(struct ec20_ofps* const _ofps)
{
	enum ec20_resp ret = EC20_RESP_OK;
	int resp;
	// 状态重置
	_ofps->ModuleState = EC20_MODULE_RESET;
	_ofps->State.b.Connected = 0;
	_ofps->State.b.ReConnect = 1;
    _ofps->State.b.CPIN = 0;
    _ofps->State.b.ReqSetGNSS=1; // gnss重新设置
    _ofps->State.b.ReqSetWifi=1; // wifi重新设置
    // 发送关机指令
    at_print("AT+QPOWD=0\r\n");  // shotdown
    // 接收响应
    at_get_resps(ReturnOK, ReturnERROR, NULL, 3000, 100, &_ofps->_at);

    //app_debug("[%s-%d] AT<-- [%s] \r\n", __func__, __LINE__, _ofps->data);
    app_debug("[%s-%d] reboot EC20 ...\r\n", __func__, __LINE__);
    // 模块重新上电
    LL_GPIO_ResetOutputPin(PWR_EN_4G_GPIO_Port, PWR_EN_4G_Pin);
    HAL_Delay(2500);
    LL_GPIO_SetOutputPin(PWR_EN_4G_GPIO_Port, PWR_EN_4G_Pin);
    //
    _ofps->ModuleState = EC20_MODULE_SET;
    HAL_Delay(10000);  // 10s, 模块需要 10s 左右的时间来启动
    //resp = GetResponse("\r\nRDY\r\n", ReturnOK, 10000);
    // "\r\nRDY\r\n" 表示设备已正常启动
    app_debug("[%s-%d] read RDY ...\r\n", __func__, __LINE__);
    resp = at_get_resps("\r\nRDY\r\n", NULL, NULL, 10000, 100, &_ofps->_at);
    if(0!=resp)
    {
    	app_debug("[%s-%d] EC20 reboot timeout!\r\n", __func__, __LINE__);
    }

    return ret;
}

enum ec20_resp EC20_Set(struct ec20_ofps* const _ofps)
{
	enum ec20_resp ret = EC20_RESP_OK;
    //COMM_GPRS *module = (COMM_GPRS *)p;
    int resp;
    uint8_t state;
    uint8_t err;

    _ofps->State.b.CPIN = 0;
    //_ofps->socket2 = 0;
    state = 0;
    err = 3;
#if 1
    at_print("AT+IPR=460800\r\n");// 更改波特率
    USART1_Init(460800);
    HAL_Delay(500);
#endif
    while(err && (state != 0xFF))
    {
    	switch(state)
    	{
    	case 0:
    		at_print("ATE0\r\n");//("ATE0\r")
    		//resp = GetResponse(ReturnOK, ReturnERROR, 3000);
    		resp = at_get_resps(ReturnOK, ReturnERROR, NULL, 3000, 100, &_ofps->_at);
    		if(0 == resp)
    		{
    			err = 3;
    			state = 1;
    		}
    		else
    		{
    			app_debug("---ATE0 response error!\r\n");
    			err--;
    		}
    		break;
    	case 2:
    		at_print("AT+CCID\r\n");
    		//resp = GetResponse(ReturnOK, ReturnERROR, 3000);
    		resp = at_get_resps(ReturnOK, ReturnERROR, NULL, 3000, 100, &_ofps->_at);
    		if(0 == resp)
    		{
    			state = 0xFF;
    		}
    		else
    		{
    			HAL_Delay(10000);
    			app_debug("---AT+CCID response error!\r\n");
    			err--;
    		}
    		break;
    	case 1:
    		at_print("AT+CPIN?\r\n");  // 检测 SIM卡
    		//resp = GetResponse(ReturnOK, ReturnERROR, 3000);
    		// 检测到 SIM卡返回 "+CPIN: READY"
    		//resp = GetResponse("+CPIN: READY", ReturnERROR, 3000);
    		resp = at_get_resps("+CPIN: READY", ReturnERROR, NULL, 3000, 100, &_ofps->_at);
    		if(0 == resp)
    		{
    			//state = 0xFF;
    			state = 2;
    			_ofps->State.b.CPIN = 1;
    		}
    		else
    		{
    			HAL_Delay(10000);
    			app_debug("---AT+CCID response error!\r\n");
    			err--;
    		}
    		break;
    	default:
    		state = 0;
    		break;
    	}
    	HAL_Delay(300);
    }

    if(0 == err)
    {
    	_ofps->ModuleState = EC20_MODULE_RESET;
    	ret=EC20_RESP_ERROR;
    }
    else
    {
    	_ofps->ModuleState = EC20_MODULE_CHK_REG;
    	_ofps->State.b.PowerOn = 1;
    }
    return ret;
}
enum ec20_resp EC20_CheckRegister(struct ec20_ofps* const _ofps)
{
	enum ec20_resp ret=EC20_RESP_OK;
    int resp;
    uint8_t state;
    uint8_t err;
    uint8_t time_out = 0;

    state = 0;
    err = 3;
    while(err && (state != 0xFF))
    {
        switch(state)
        {
            case 0:
            	at_print("AT+CREG?\r");
                //resp = GetResWithCheck(ReturnOK, ReturnERROR, 1000);
            	resp = at_get_resps(ReturnOK, ReturnERROR, NULL, 1000, 100, &_ofps->_at);
                if(0 == resp)
                {
                    char *pStr;
                    pStr = strstr(_ofps->_at._rbuf, "+CREG: ");
                    if(NULL == pStr)
                    {
                      err--;
                      break;
                    }
                    if((pStr[9] == '1') || (pStr[9] == '5'))//已注册
                    {
                        state = 1;
                    }
                    else
                    {
                        time_out++;
                        HAL_Delay(1000);
                    }
                }
                else
                {
                	app_debug("---AT+CREG? response error!\r\n");
                    err--;
                }
                break;
            case 1:
            	at_print("AT+CGATT?\r");
                //resp = GetResWithCheck(ReturnOK, ReturnERROR, 4000);
            	resp = at_get_resps(ReturnOK, ReturnERROR, NULL, 4000, 100, &_ofps->_at);
                if(0 == resp)
                {
                    char *pStr;
                    pStr = strstr(_ofps->_at._rbuf, "+CGATT: ");
                    if(NULL == pStr)
                    {
                      err--;
                      break;
                    }
                    if(pStr[8] == '1')
                        state = 0xFF;
                    else
                    {
                        err = 3;
                        state = 2;
                    }
                }
                else
                {
                	app_debug("---AT+CGATT? response error!\r\n");
                    err--;
                }
                break;
            case 2:
            	at_print("AT+CGATT=1\r");
                //resp = GetResponse(ReturnOK, ReturnERROR, 4000);
            	resp = at_get_resps(ReturnOK, ReturnERROR, NULL, 4000, 100, &_ofps->_at);
                if(0 <= resp)
                {
                    err = 3;
                    state = 1;
                }
                else
                {
                	app_debug("---AT+CGATT=1 response error!\r\n");
                    err--;
                }
                break;
            default:
                state = 0;
                break;
        }
        HAL_Delay(100);

        if(time_out > 200)
            err = 0;
    }

    if(0 == err)
    {
#if 0
    	_ofps->ModuleState = EC20_MODULE_RESET;
        ret=EC20_RESP_ERROR;
#else       // 联通没有 2G网络
        _ofps->ModuleState = EC20_MODULE_PPP;
#endif
    }
    else
    {
    	_ofps->ModuleState = EC20_MODULE_PPP;
    }
    return ret;
}

enum ec20_resp EC20_Ppp(struct ec20_ofps* const _ofps)
{
	enum ec20_resp ret=EC20_RESP_OK;
    int resp;
    uint8_t state;
    uint8_t err;
    uint8_t time_out = 0;

    state = 1;
    err = 3;
    while(err && (state != 0xFF))
    {
        switch(state)
        {
//                case 0:
//                    ModulePrintf("AT+CIPMUX?\r");//查询IP连接单路还是多路
//                    resp = GetResWithCheck(ReturnOK, ReturnERROR, 200);
//                    if(0 == resp)
//                    {
//                        char *pStr = rt_strstr((const char *)module->Line, "+CIPMUX: ");
//                        if(NULL != pStr)
//                        {
//                            if(pStr[9]=='1'){
//                                state = 11;
//                            }
//                            else{
//                                state = 10;
//                            }
//                            err = 3;
//                        }
//                    }
//                    else
//                    {
//                        app_debug("---AT+CIPMUX? response error!\r\n");
//                        err--;
//                    }
//                    break;
            case 1:
            	at_print("AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",3\r",
                		_ofps->NetInfo.APN,
						_ofps->NetInfo.NUSER,
						_ofps->NetInfo.NPWD);//设置APN名字
                //resp = GetResponse(ReturnOK, ReturnERROR, 200);
            	resp = at_get_resps(ReturnOK, ReturnERROR, NULL, 200, 100, &_ofps->_at);
                if(0 == resp)
                {
                    state = 0xff;
                }
                else
                {
                	app_debug("---AT+CSTT= response error!\r\n");
                    err--;
                }
                break;
//                case 2:
//                  {
//                    ModulePrintf("AT+QIACT=1\r");//激活移动场景
////                    resp = GetResponse(ReturnOK, ReturnERROR, 3000);
//                    const char *res[]={ReturnOK, ReturnERROR};
//                    resp=GetRes(res,sizeof(res)/sizeof(const char*),3000,2000);
//                    if(0 == resp)
//                    {
//                        err = 3;
//                        state = 3;
//                    }
//                    else
//                    {
//                        app_debug("---AT+CIICR response error!\r\n");
//                        err--;
//                    }
//                  }
//                    break;
//                case 3:
//                   ModulePrintf("AT+CIFSR\r");//获得本地IP
////                    resp = GetRes(2000);
//                    resp =GetRes(0,0,2000,100);
//                    if(-1 != resp)
//                    {
//                        state = 0xFF;
//                    }
//                    else
//                    {
//                        app_debug("---AT+CIFSR response error!\r\n");
//                        time_out++;
//                        err--;
//                    }
//                    break;
//                case 10://切换多路
//                   ModulePrintf("AT+CIPMUX=1\r");
//                    resp = GetResponse(ReturnOK, ReturnERROR, 2000);
//                    if(0 == resp)
//                    {
//                        err = 3;
//                        state = 0;
//                    }
//                    else
//                    {
//                        app_debug("---AT+CIPMUX=1 response error!\r\n");
//                        err--;
//                    }
//                    break;
//                case 11://读状态
//                    ModulePrintf("AT+CIPSTATUS\r");
////                    resp = GetRes(2000);
//                    resp = GetRes(0,0,2000,100);
//                    if(-1 != resp)
//                    {
//                        if(RT_NULL != rt_strstr((const char *)CommGprs.Line,"IP INITIAL"))
//                        {
//                            err = 3;
//                            state = 12;
//                        }
//                        else
//                        {
//                            state = 0xFF;
//                        }
//                    }
//                    else
//                    {
//                        app_debug("---AT+CIPSTATUS response error!\r\n");
//                        err--;
//                    }
//                    break;
//                case 12://
//                    ModulePrintf("AT+CIPRXGET=1\r");//手动接收数据
//                    resp = GetResponse(ReturnOK, ReturnERROR, 2000);
//                    if(0==resp)
//                    {
//                        state =1;
//                        err=3;
//                    }
//                    else
//                    {
//                        app_debug("---AT+CIPRXGET=1 response error!\r\n");
//                        err--;
//                    }
//                    break;
            default:
                state = 0;
                break;
        }
        HAL_Delay(30);

        if(time_out > 10)
            err = 0;
    }

    if(0 == err)
    {
    	_ofps->ModuleState = EC20_MODULE_RESET;
        ret=EC20_RESP_ERROR;
    }
    else
    {
    	_ofps->ModuleState = EC20_MODULE_GET_INFORMATION;
    }
    return ret;
}

enum ec20_resp EC20_GetInformation(struct ec20_ofps* const _ofps)
{
	enum ec20_resp ret=EC20_RESP_OK;
    int resp;
    uint8_t state;
    uint8_t err;
//    u8 i;
    //char *pStr;

    state = 0;
    err = 3;
    while(err && (state != 0xFF))
    {
        switch(state)
        {
            case 0:
            	at_print("AT+CGSN\r");
                //resp = GetResWithCheck(ReturnOK, ReturnERROR, 200);
            	resp = at_get_resps(ReturnOK, ReturnERROR, NULL, 200, 100, &_ofps->_at);
                if(0 == resp)
                {
#if 0
                    pStr=&_ofps->_at._rbuf[2];
                    if(checkAlNum(pStr,LENTH_IMEI))
                    {
                      memcpy(_ofps->IMEI, pStr, LENTH_IMEI);
                      err = 3;
                      state = 1;
                    }
#else
                    char IMEI[32];
                    if(at_get_str_split(_ofps->_at._rbuf, IMEI, '\n', 2, sizeof(IMEI))>0)
                    {
                    	memcpy(_ofps->IMEI, IMEI, LENTH_IMEI);
                        err = 3;
                        state = 1;
                    }
#endif
                }
                else
                {
                	app_debug("---AT+CGSN error!\r\n");
                    err--;
                }
                break;
            case 1:
            	at_print("AT+CCID\r");
                //resp = GetResWithCheck(ReturnOK, ReturnERROR, 2000);
            	resp = at_get_resps(ReturnOK, ReturnERROR, NULL, 200, 100, &_ofps->_at);
                if(0 == resp)
                {
#if 0
                    pStr=&_ofps->_at._rbuf[9];
                    if(checkAlNum(pStr, LENTH_SIM))
                    {
                      memcpy(_ofps->SIM, pStr, LENTH_SIM);
                      state = 0xFF;
                    }
#else
                    char SIM[32];
                    if(at_get_resp_split(_ofps->_at._rbuf, "+CCID: ", SIM, sizeof(SIM))>0)
                    {
                      memcpy(_ofps->SIM, SIM, LENTH_SIM);
                      state = 0xFF;
                    }
#endif
                }
                else
                {
                	app_debug("---AT+CCID error!\r\n");
                    err--;
                }
                break;
            default:
                state = 0;
                break;
        }
        HAL_Delay(30);
    }

    if(0 == err)
    {
    	_ofps->ModuleState = EC20_MODULE_RESET;
        ret=EC20_RESP_ERROR;
    }
    else
    {
    	_ofps->ModuleState = EC20_MODULE_CONNECT;
    }

    return ret;
}
enum ec20_resp EC20_Connect(struct ec20_ofps* const _ofps)
{
	enum ec20_resp ret=EC20_RESP_OK;
    int resp;
    uint8_t state;
    uint8_t err;
   // char *pStr;
    char *addr;
    uint8_t time_out = 0;
    int socket;
    int ErrCode;

    state = 0;
    err = 3;
    while(err && (state != 0xFF))
    {
        switch(state)
        {
            case 0:
              {
            	  at_print("AT+QIACT?\r");//激活移动场景
//                    resp = GetResponse(ReturnOK, ReturnERROR, 3000);
                //const char *res[]={"+QIACT: 1,",ReturnOK,ReturnERROR};
                //resp=GetRes(res,sizeof(res)/sizeof(const char*),3000,2000);
                resp = at_get_resps("+QIACT: 1,", ReturnOK, ReturnERROR, 3000, 200, &_ofps->_at);
                if(0 == resp)
                {
                    err = 3;
                    state = 2;
                }
                else if(1 == resp)
                {
                    err = 3;
                    state = 1;
                }
                else
                {
                    app_debug("---AT+QIACT? response error!\r\n");
                    err--;
                }
              }
                break;
            case 1:
              {
            	  at_print("AT+QIACT=1\r");//激活移动场景
//                    resp = GetResponse(ReturnOK, ReturnERROR, 3000);
                //const char *res[]={ReturnOK, ReturnERROR};
                //resp=GetRes(res,sizeof(res)/sizeof(const char*),3000,2000);
                resp = at_get_resps(ReturnOK, ReturnERROR, NULL, 3000, 200, &_ofps->_at);
                if(0 == resp)
                {
                    err = 3;
                    state =2;
                }
                else if(resp==1){
                    err=3;
                    state=4;
                }
                else
                {
                    app_debug("---AT+QIACT=1 response error!\r\n");
                    err--;
                }
              }
                break;
            case 2:
              {
                  addr=_ofps->NetInfo.IP;
                  if(0 != strlen((char const*)_ofps->NetInfo.NADR))
                  {
                     addr=_ofps->NetInfo.NADR;
                  }

                  const char *res[]={"+QIOPEN: 0,"};
                  at_print("AT+QIOPEN=1,%d,\"TCP\",\"%s\",%d,0,0\r",
                		  _ofps->SocketID,addr, _ofps->NetInfo.PORT);

                  //resp=GetResWait1(res,sizeof(res)/sizeof(const char*),15000,100);
//                      resp = GetResWithCheck(ReturnOK, ReturnERROR,4000);
//                      resp = GetRes(0,0,4000,2000);
                  resp = at_get_resps("+QIOPEN: 0,", NULL, NULL, 15000, 200, &_ofps->_at);
                  if(resp==0)
                  {
                        char *pStr= strstr((const char *)_ofps->_at._rbuf, res[0]);
                        if(NULL == pStr)
                        {
                          err--;
                          break;
                        }
#if 0
                        if(pStr[11]=='0'){
                            state =0xff;
                            // 多路 TCP连接
                            if(module->Net2.PORT>1024) state = 3;
                        }
                        else{
                            err--;
                            break;
                        }
#else
                        // +QIOPEN: <connectID>,<err>
                        sscanf(pStr, "+QIOPEN: %d,%d", &socket, &ErrCode);
                        if(563==ErrCode) // ErrCode 563:Socket identity has been used
                        {
                        	at_print("AT+QICLOSE=%d\r", _ofps->SocketID);
                            //resp = GetResponse("OK\r\n", ReturnERROR, 3000);
                        	resp = at_get_resps("OK\r\n", ReturnERROR, NULL, 3000, 200, &_ofps->_at);
                            err--;
                        }
                        // 0:Operation successful
                        else if(0==ErrCode)//if(pStr[11]=='0')
                        {
                            state =0xff;
                            // 多路 TCP连接
                            if(_ofps->Net2.PORT>1024) state = 3;
                        }
                        else
                        {
                            err--;
                            break;
                        }
#endif
                  }
                  else
                  {
                    app_debug("---AT+QIOPEN= error!\r\n");
                    err--;
                  }
              }
              break;
            case 3: // 多路 TCP连接
              {
                  addr=_ofps->Net2.IP;
                  if(0 != strlen((char const*)_ofps->Net2.NADR))
                  {
                     addr=_ofps->Net2.NADR;
                  }
                  _ofps->socket2 = _ofps->SocketID + 1;
                  const char *res[]={"+QIOPEN: 1,"};
                  at_print("AT+QIOPEN=1,%d,\"TCP\",\"%s\",%d,0,0\r",
                		  _ofps->socket2,addr,_ofps->Net2.PORT);

                  //resp=GetResWait1(res,sizeof(res)/sizeof(const char*),15000,100);
//                      resp = GetResWithCheck(ReturnOK, ReturnERROR,4000);
//                      resp = GetRes(0,0,4000,2000);
                  resp = at_get_resps("+QIOPEN: 1,", NULL, NULL, 15000, 200, &_ofps->_at);
                  if(resp==0)
                  {
                        char *pStr= strstr((const char *)_ofps->_at._rbuf, res[0]);
                        if(NULL == pStr)
                        {
                          err--;
                          break;
                        }
                        // +QIOPEN: <connectID>,<err>
                        sscanf(pStr, "+QIOPEN: %d,%d", &socket, &ErrCode);
                        if(563==ErrCode) // ErrCode 563:Socket identity has been used
                        {
                        	at_print("AT+QICLOSE=%d\r", _ofps->socket2);
                            //resp = GetResponse("OK\r\n", ReturnERROR, 3000);
                            resp = at_get_resps("OK\r\n", ReturnERROR, NULL, 3000, 200, &_ofps->_at);
                            err--;
                        }
                        // 0:Operation successful
                        else if(0==ErrCode)//if(pStr[11]=='0')
                        {
                            state =0xff;
                        }
                        else
                        {
                            err--;
                            break;
                        }
                  }
                  else
                  {
                    app_debug("---AT+QIOPEN= error!\r\n");
                    err--;
                  }
              }
              break;
             case 4:
              {
            	  at_print("AT+QIDEACT=1\r");//激活移动场景
//                    resp = GetResponse(ReturnOK, ReturnERROR, 3000);
                //const char *res[]={ReturnOK, ReturnERROR};
                //resp=GetRes(res,sizeof(res)/sizeof(const char*),3000,2000);
                resp = at_get_resps(ReturnOK, ReturnERROR, NULL, 3000, 200, &_ofps->_at);
                if(0 == resp)
                {
                    state =0xff;
                }
                else
                {
                    app_debug("---AT+QIDEACT=1 response error!\r\n");
                    err--;
                }
              }
                break;
            default:
                state = 0;
                break;
        }
        HAL_Delay(100);

        if(time_out > 10)
            err = 0;
    }

    if(0 == err)
    {
    	_ofps->ModuleState = EC20_MODULE_DISCONNECT;
        ret=EC20_RESP_ERROR;
    }
    else
    {
    	_ofps->State.b.Connected = 1;
    	_ofps->ModuleState = EC20_MODULE_IDLE;
    }

    return ret;
}
enum ec20_resp EC20_Disconnect(struct ec20_ofps* const _ofps)
{
	enum ec20_resp ret=EC20_RESP_OK;
    int resp;
    uint8_t state;
    uint8_t err;

    state = 0;
    err = 3;
    while(err && (state != 0xFF))
    {
    	at_print("AT+QICLOSE=%d\r", _ofps->SocketID);
        //resp = GetResponse("OK\r\n", ReturnERROR, 3000);
    	resp = at_get_resps("OK\r\n", ReturnERROR, NULL, 3000, 200, &_ofps->_at);
        if(0 <= resp)
        {
            state = 0xFF;
        }
        else
        {
            app_debug("---AT+CIPCLOSE= error!\r\n");
            err--;
            HAL_Delay(30);
        }
    }
    err = 3;
    while((_ofps->socket2>0) && err && (state != 0xFF))
    {
    	at_print("AT+QICLOSE=%d\r", _ofps->socket2);
        //resp = GetResponse("OK\r\n", ReturnERROR, 3000);
    	resp = at_get_resps("OK\r\n", ReturnERROR, NULL, 3000, 200, &_ofps->_at);
        if(0 <= resp)
        {
            state = 0xFF;
            _ofps->socket2 = 0;
        }
        else
        {
            app_debug("---AT+CIPCLOSE= error!\r\n");
            err--;
            HAL_Delay(30);
        }
    }
    HAL_Delay(30);
    if(0 == err)
    {
    	_ofps->ModuleState = EC20_MODULE_RESET;
        ret=EC20_RESP_ERROR;
    }
    else
    {
    	_ofps->ModuleState = EC20_MODULE_SET;
    }

    return ret;
}
enum ec20_resp EC20_Idle(struct ec20_ofps* const _ofps)
{
	enum ec20_resp ret=EC20_RESP_OK;
    int resp;

    //if(rt_sem_trytake(module->RxIntSem) == RT_EOK)
    if(0==_ofps->_at.uart_is_empty())
    {
        while(1)
        {
            //const char * res[]={ReturnRecv,ReturnSocket_closed,ReturnPdpDeactivation};
            //resp=GetRes(res,sizeof(res)/sizeof(const char*),100,100);
            //resp=at_get_resps(ReturnRecv, ReturnSocket_closed, ReturnPdpDeactivation, "OK\r\n", NULL, 100,100);
            resp = at_get_resp4s(ReturnRecv, ReturnSocket_closed, ReturnPdpDeactivation, "OK\r\n", 3000, 200, &_ofps->_at);
            //usb_debug("[%s--%d] CommGprs.Line<%s> \r\n", __func__, __LINE__, CommGprs.Line);
            if(0 == resp)//下发数据
            {
                int connectID;
                // +QIURC: "recv",<connectID>
                //if(1==at_resp_parse_args(CommGprs.Line, "+QIURC: ", "\"recv\",%d", &connectID))
                if(at_get_resp_args_int(_ofps->_at._rbuf, "+QIURC: ", "\"recv\",%d", &connectID)>0)
                {
                    // 主连接断开,重连
                    if(connectID == _ofps->SocketID) _ofps->Recv_flag = 1;
                    //if(connectID == module->SocketID) SeverRecv(module);
                }
            }
            else if(1 == resp)//socket关闭，重连
            {
                int connectID;
                // +QIURC: "closed",<connectID>
                //if(1==at_resp_parse_args(CommGprs.Line, "+QIURC: ", "\"closed\",%d", &connectID))
                if(at_get_resp_args_int(_ofps->_at._rbuf, "+QIURC: ", "\"closed\",%d", &connectID)>0)
                {
                    // 主连接断开,重连
                    if(connectID == _ofps->SocketID) _ofps->State.b.ReqReConnect=1;
                }
            }
            else if(3==resp)
            {
                break;
            }
            else if(resp > 1)
            {
            	_ofps->ModuleState = EC20_MODULE_SET;
            	_ofps->State.b.Connected = 0;
            	_ofps->State.b.ReConnect = 1;
                break;
            }
            else
            {
                // AT+QISTATE=<query_type>,<connectID>
                /*
+QISTATE:
<connectID>,<service_type>,<IP_address>,<remote_port>
,<local_port>,<socket_state>,<contextID>,<serverID>,<ac
cess_mode>,<AT_port>

OK
                */
            	at_print("AT+QISTATE=1,%d\r", _ofps->SocketID);
                //resp=at_get_resps("+QISTATE: ", NULL, NULL, NULL, NULL, 100,100);
            	resp = at_get_resps("+QISTATE: ", ReturnERROR, NULL, 100,100, &_ofps->_at);
                if(0==resp)
                {
                    int connectID=-1;
                    int socket_state=-1;
                    //int num1 = 0;
                    //int num2 = 0;
                    //num1 = at_resp_parse_split_args(CommGprs.Line, "+QISTATE: ", ',', 1, "%d,", &connectID);
                    //num2 = at_resp_parse_split_args(CommGprs.Line, "+QISTATE: ", ',', 6, "%d,", &socket_state);
                    at_get_resp_split_int(_ofps->_at._rbuf, "+QISTATE: ", &connectID, ',', 1);
                    at_get_resp_split_int(_ofps->_at._rbuf, "+QISTATE: ", &socket_state, ',', 6);
                    //   2        “Connected”: client/incoming connection has been established
                    //if((1==num1) && (1==num2) && (connectID==module->SocketID) && (2!=socket_state))
                    if((connectID==_ofps->SocketID) && (2!=socket_state) && (-1!=socket_state))
                    {
                        // 主连接断开,重新设置模块
                    	_ofps->ModuleState = EC20_MODULE_SET;
                    	_ofps->State.b.Connected = 0;
                    	_ofps->State.b.ReConnect = 1;
                    }
                }
                break;
            }
        }
    }

    return ret;
}
extern enum ec20_resp EC20_GetCSQ(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_GetPosition(struct ec20_ofps* const _ofps);
enum ec20_resp EC20_Send(struct ec20_ofps* const _ofps, const void* const _data, const uint16_t _len)
{
	enum ec20_resp ret=EC20_RESP_OK;
	int resp;
	uint8_t state;
	uint8_t err;
	const char *const data0 = (const char *)_data;//((COMM_T*)p)->data;
	uint16_t len0 = _len;//((COMM_T*)p)->len;
	uint16_t n=0;
	//char * res[]={"SEND OK",(char*)ReturnSENDFAIL,(char*)ReturnERROR};

	//char num[20];
#if 0
	if(RT_EOK != module_gprs_take(RT_WAITING_FOREVER))
	{
		((COMM_T*)p)->ret = -RT_ERROR;
		return RT_EIO;
	}

	if(module->ModuleState != MODULE_IDLE)
	{
		((COMM_T*)p)->ret = -RT_ERROR;
		return RT_EBUSY;
	}
#endif

	state = 1;
	err = 3;
	const char *data=data0;
	uint16_t len=len0;
	while(err && (state != 0xFF))
	{
		switch(state)
		{

		case 1:
			if(len > 0)
			{
				int total_send_length=-1, ackedbytes=-1, unackedbytes=-1;
				uint16_t alen=0;
				int flag=0;
				at_print("AT+QISEND=%d,0\r",_ofps->SocketID);
				//resp = GetRes(0,0,2000,100);
				resp = at_get_resps("+QISEND: ", ReturnERROR, NULL, 2000, 50, &_ofps->_at);
				if(-1 != resp)
				{
					// +QISEND:
					//<total_send_length>,<ackedbytes>,<unackedbytes>
					//if(3==at_resp_parse_args(CommGprs.Line, "+QISEND: ", "%d,%d,%d", &total_send_length, &ackedbytes, &unackedbytes))
					at_get_resp_split_int(_ofps->_at._rbuf, "+QISEND: ", &total_send_length, ',', 1);
					at_get_resp_split_int(_ofps->_at._rbuf, "+QISEND: ", &ackedbytes, ',', 2);
					at_get_resp_split_int(_ofps->_at._rbuf, "+QISEND: ", &unackedbytes, ',', 3);
					if(unackedbytes>=0)
					{
						alen=unackedbytes;
						flag=1;
					}
				}
				if(!flag)
				{
					err--;
					break;
				}
				// +QISEND: (0-11),(0-1460) ,这里取 1024为保留部分缓存
				uint16_t slen=1024;
				if(alen<slen) slen-=alen;
				//usb_debug("[%s--%d] CommGprs.Line<%s> \r\n", __func__, __LINE__, CommGprs.Line);
				//usb_debug("[%s--%d] len0<%d> len<%d> alen<%d> slen<%d>\r\n", __func__, __LINE__, len0, len, alen, slen);
				//usb_debug("[%s--%d] total_send_length<%d> ackedbytes<%d> unackedbytes<%d>\r\n", __func__, __LINE__, total_send_length, ackedbytes, unackedbytes);
				if(len > slen)
				{
					n = slen;
					len -= slen;
				}
				else
				{
					n = len;
					len = 0;
				}
				state = 2;
			}
			else
				state = 0xFF;
			break;
		case 2:
			at_print("AT+QISEND=%d,%d\r", _ofps->SocketID, n);
			//resp = GetResponse(ReturnA,ReturnERROR, 200);
			resp = at_get_resps(ReturnA, ReturnERROR, NULL, 200, 50, &_ofps->_at);
			if(1 == resp)
			{
				state = 0xFF;
				err = 0;
			}
			else
			{
				//                        err = 3;
				state = 3;
			}
			break;
		case 3:
			//rt_snprintf(num,sizeof(num),"DATA ACCEPT:%d,%d",module->SocketID,n);
			//rt_device_write(module->Dev, 0, data, n);
			//app_debug("[%s--%d] uart_send ... _data[0x%08X] data0[0x%08X] data[0x%08X] \r\n", __func__, __LINE__, _data, data0, data);
			//app_debug("[%s--%d] ...data[%d]:<%s>\r\n", __func__, __LINE__, n, data);
			_ofps->_at.uart_send((const uint8_t*)data, n);
			//uart1_send((const uint8_t*)data, n);
			//app_debug("[%s--%d] ...\r\n", __func__, __LINE__);
			//                resp = GetResponse(num, ReturnERROR, 3000);
			//char * res[]={"SEND OK",(char*)ReturnSENDFAIL,(char*)ReturnERROR};
			//resp=GetRes((const char **)res,sizeof(res)/sizeof(char*),3000,500);
			resp = at_get_resps("SEND OK",(char*)ReturnSENDFAIL,(char*)ReturnERROR, 3000, 50, &_ofps->_at);
			//app_debug("[%s--%d] uart_send ... end\r\n", __func__, __LINE__);
			if((0 == resp))
			{
				/*{
                        rt_kprintf("---send num:%d\r\n",n);
                        uint16_t i;
                        for(i=0;i<n;i++)
                        {
                            rt_kprintf("%02x ",*((uint8_t*)&data[i]));
                        }
                        rt_kprintf("\r\n");
                    }*/
				//                    rt_sprintf(num, "%d", n);
				//                    pStr = rt_strstr((const char *)module->Line, num);
				//                    if(pStr != RT_NULL)
				{
					_ofps->State.b.Sended=1;
					data += n;
					err = 3;
					state = 1;
				}
				//                    else
				//                    {
				//                        err = 0;
				//                        state = 0xFF;
				//                    }
			}
			else
			{
				app_debug("--DATA ACCEPT:...ERROR!\r\n");
				err --;
				state = 1;
				//重传
				data=data0;
				len=len0;
			}
			break;
		default:
			state = 0;
			break;
		}
		HAL_Delay(30);
	}

	if(0 == err)
	{
		_ofps->ModuleState = EC20_MODULE_DISCONNECT;
		//((COMM_T*)p)->ret = -RT_ERROR;
		_ofps->State.b.Connected = 0;
		_ofps->State.b.ReConnect = 1;
		ret=EC20_RESP_ERROR;
	}
	//    else
	//    {
	//        ((COMM_T*)p)->ret = RT_EOK;
	//    }

	return ret;
}
static enum ec20_resp __EC20_Read(struct ec20_ofps* const _ofps, const uint8_t socket, void* const _data, const uint16_t _len, uint16_t* const _rlen)
{
	enum ec20_resp ret=EC20_RESP_OK;
    //COMM_GPRS *const module = ((COMM_T*)p)->module;
    int resp;
    uint8_t state;
    uint8_t err;
    uint16_t len;
    uint16_t maxlen=_len;//recv->len;
    const char* data = NULL;
    const char QIRD[] = "+QIRD: ";
    len = 0;
    //recv->len = 0;
    state = 0;
    err = 5;
    while(err && (state != 0xFF))
    {
        int _length=-1;
        //int rlen=-1;
        uint16_t dlen=maxlen-len;
        // AT+QIRD=<connectID>[,<read_length>]
        at_print("AT+QIRD=%d,%d\r", socket,dlen);
        //resp = GetResp(recv);
        //+QIRD: <read_actual_length><CR><LF><data>
        //
        //OK
        //resp=at_get_resps("+QIRD: ", NULL, NULL, NULL, NULL, 1000,100);
        resp = at_get_resps("+QIRD: ", NULL, NULL, 1000, 100, &_ofps->_at);
        if(0 == resp)
        {
        	//int i;
            //if(1==at_resp_parse_args(CommGprs.Line, "+QIRD: ", "%d\r\n", &_length))
        	if(at_get_resp_split_int(_ofps->_at._rbuf, QIRD, &_length, '\r', 1)>0)
            {
        		// 查找数据区偏移
        	    data=strstr(_ofps->_at._rbuf, QIRD);
        	    if(NULL==data)
        	    {
        	    	ret=EC20_RESP_ERROR;
        	    	break;
        	    }
        	    data += strlen(QIRD);
        	    data=strstr(data, "\r\n");
        	    if(NULL==data)
        	    {
        	    	ret=EC20_RESP_ERROR;
        	    	break;
        	    }
        	    data += 2;
        	    //app_debug("[%s--%d] _rsize[%d] _rbuf[%s] data[%s]\r\n", __func__, __LINE__, _ofps->_at._rsize, _ofps->_at._rbuf, data);
        	    //app_debug("[%s--%d] _rsize[%d]: \r\n", __func__, __LINE__, _ofps->_at._rsize);
        	    //for(i=0; i<_ofps->_at._rsize; i++) app_debug("%c", _ofps->_at._rbuf[i]);
        	    //app_debug("[%s--%d] i[%d]\r\n", __func__, __LINE__, i);
        	    if(NULL!=_data)
        	    {
        	    	memset(_data, 0, _len);
        	    	if(_length>=_len) _length = _len;
        	    	memcpy(_data, data, _length);
        	    	*_rlen = _length;
        	    }
                state = 0xFF;
            }
            else state = 0xFF;
//            if(recv->len==maxlen){//缓冲区满
//                state=0xff;
//                continue;
//            }
        }
        else
        {
        	//app_debug("AT+QIRD=%d,1024 error!\r\n", socket);
            err--;
            HAL_Delay(100);
        }
        HAL_Delay(10);
    }

    if(0 == err)
    {
        //recv->ret = -RT_ERROR;
        _ofps->ModuleState = EC20_MODULE_SET;
        _ofps->State.b.Connected = 0;
        _ofps->State.b.ReConnect = 1;
        ret=EC20_RESP_ERROR;
    }
//    else
//    {
//        //recv->len=len;
//        ret = RT_EOK;
//    }
    return ret;
}
enum ec20_resp EC20_Read(struct ec20_ofps* const _ofps, void* const _data, const uint16_t _len, uint16_t* const _rlen)
{
	return __EC20_Read(_ofps, _ofps->SocketID, _data, _len, _rlen);
}

extern enum ec20_resp EC20_PowerOff(struct ec20_ofps* const _ofps);

void EC20_Test(void)
{
	enum ec20_resp ret;
	char data[1024+1];
	int resp;
	uint32_t tick_start = HAL_GetTick();
	uint32_t tick_end;
	//app_debug("[%s--%d] EC20 Test Start!\r\n", __func__, __LINE__);
    //app_debug("[%s-%d] read RDY ...\r\n", __func__, __LINE__);
    resp = at_get_resps("\r\nRDY\r\n", NULL, NULL, 12000, 1000, &_ec20_ofps._at);
	if(0!=resp) EC20_Reset(&_ec20_ofps);
	//app_debug("[%s-%d] EC20_Set ...\r\n", __func__, __LINE__);
	ret = EC20_Set(&_ec20_ofps);
	//app_debug("[%s-%d] EC20_CheckRegister ...\r\n", __func__, __LINE__);
	if(EC20_RESP_OK==ret) ret = EC20_CheckRegister(&_ec20_ofps);
	app_debug("[%s-%d] EC20_Ppp ...\r\n", __func__, __LINE__);
	if(EC20_RESP_OK==ret) ret = EC20_Ppp(&_ec20_ofps);
	app_debug("[%s-%d] EC20_GetInformation ...\r\n", __func__, __LINE__);
	if(EC20_RESP_OK==ret) ret = EC20_GetInformation(&_ec20_ofps);
	app_debug("[%s-%d] EC20_Connect ...\r\n", __func__, __LINE__);
	if(EC20_RESP_OK==ret) ret = EC20_Connect(&_ec20_ofps);
	app_debug("[%s-%d] EC20_Send ...\r\n", __func__, __LINE__);
	//if(EC20_RESP_OK==ret) ret = EC20_Send(&_ec20_ofps, "#####", 5);
	//if(EC20_RESP_OK==ret) ret = EC20_Read(&_ec20_ofps, NULL, 100);
	// 收发测试
	tick_start = HAL_GetTick();
	app_debug("[%s-%d] tick_start[%d] ...\r\n", __func__, __LINE__, tick_start);
	for(int i=0; i<2; i++)
	{
		uint16_t _rlen=0;
		memset(data, '0'+(i%10), sizeof(data));
		if(EC20_RESP_OK==ret) ret = EC20_Send(&_ec20_ofps, data, sizeof(data)-1);
		memset(data, 0, sizeof(data));
		if(EC20_RESP_OK==ret) ret = EC20_Read(&_ec20_ofps, data, sizeof(data), &_rlen);
		app_debug("[%s-%d] _rlen[%d] data:[%s]\r\n", __func__, __LINE__, _rlen, data);
	}
	tick_end = HAL_GetTick();
	app_debug("[%s-%d] tick_start[%d] tick_start[%d] time[%d] ms\r\n", __func__, __LINE__, tick_start, tick_end, tick_end-tick_start);
	HAL_Delay(500);
	app_debug("[%s-%d] EC20_Disconnect ...\r\n", __func__, __LINE__);
	if(EC20_RESP_OK==ret) ret = EC20_Disconnect(&_ec20_ofps);
}

/************************ (C) COPYRIGHT Merafour *****END OF FILE****/

