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

//static char _ccm __attribute__ ((aligned (4))) ec20_data[1024*4];

static struct ec20_ofps _ccm __attribute__ ((aligned (4))) _ec20_ofps = {
		.NetInfo = {
				.NADR = "39.108.51.99",
				.PORT = 9910,
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
extern enum ec20_resp EC20_Idle(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_GetCSQ(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_GetPosition(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_Send(struct ec20_ofps* const _ofps);
extern enum ec20_resp EC20_PowerOff(struct ec20_ofps* const _ofps);

void EC20_Test(void)
{
	enum ec20_resp ret;
	int resp;
	app_debug("[%s--%d] EC20 Test Start!\r\n", __func__, __LINE__);
    app_debug("[%s-%d] read RDY ...\r\n", __func__, __LINE__);
    resp = at_get_resps("\r\nRDY\r\n", NULL, NULL, 12000, 1000, &_ec20_ofps._at);
	if(0!=resp) EC20_Reset(&_ec20_ofps);
	app_debug("[%s-%d] EC20_Set ...\r\n", __func__, __LINE__);
	ret = EC20_Set(&_ec20_ofps);
	app_debug("[%s-%d] EC20_CheckRegister ...\r\n", __func__, __LINE__);
	if(EC20_RESP_OK==ret) ret = EC20_CheckRegister(&_ec20_ofps);
	app_debug("[%s-%d] EC20_Ppp ...\r\n", __func__, __LINE__);
	if(EC20_RESP_OK==ret) ret = EC20_Ppp(&_ec20_ofps);
	app_debug("[%s-%d] EC20_GetInformation ...\r\n", __func__, __LINE__);
	if(EC20_RESP_OK==ret) ret = EC20_GetInformation(&_ec20_ofps);
	app_debug("[%s-%d] EC20_Connect ...\r\n", __func__, __LINE__);
	if(EC20_RESP_OK==ret) ret = EC20_Connect(&_ec20_ofps);
	HAL_Delay(500);
	app_debug("[%s-%d] EC20_Disconnect ...\r\n", __func__, __LINE__);
	if(EC20_RESP_OK==ret) ret = EC20_Disconnect(&_ec20_ofps);
}

/************************ (C) COPYRIGHT Merafour *****END OF FILE****/

