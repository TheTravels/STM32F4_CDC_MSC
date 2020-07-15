/************************ (C) COPYLEFT 2018 Merafour *************************

* File Name          : GB17691.c
* Author             : Merafour
* Last Modified Date : 01/14/2020
* Description        : 国标 17691协议.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
******************************************************************************/
#include "GB17691.h"
#include "BigLittleEndian.h"
#ifdef BUILD_MCU_ENCRYPT
#include "../agreement/encrypt.h"
#endif
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

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
#if 0
uint16_t gb_cpy(void* const _Dst,const void* const _Src, const uint16_t _Size)
{
    memcpy(_Dst, _Src, _Size);
    return _Size;
}
#endif

// 获取 GMT+8时间，format: YYYY/MM/DD hh:mm:ss
#if 1
int GB_GMT8(const time_t _time, uint8_t UTC[6])
{
    struct tm* _tm=NULL;
    //time_t _time = time(NULL);
    _tm = localtime(&_time);
    if(NULL!=_tm)
    {
        UTC[0] = _tm->tm_year+1900-2000;
        UTC[1] = _tm->tm_mon+1;
        UTC[2] = _tm->tm_mday;
        UTC[3] = _tm->tm_hour;
        UTC[4] = _tm->tm_min;
        UTC[5] = _tm->tm_sec;
        return 0;
    }
    return -1;
}
#else
#include "DateTime.h"
int GB_GMT8(const time_t _time, uint8_t UTC[6])
{
    DateTime      utctime   = {.year = 1970, .month = 1, .day = 1, .hour = 0, .minute = 0, .second = 0};
    DateTime      localtime = {.year = 1970, .month = 1, .day = 1, .hour = 8, .minute = 0, .second = 0};
    if(_time > INT32_MAX)
    {
      utctime = GregorianCalendarDateAddSecond(utctime, INT32_MAX);
      utctime = GregorianCalendarDateAddSecond(utctime, (int)(_time - INT32_MAX));
    }
    else
    {
      utctime = GregorianCalendarDateAddSecond(utctime, (int)_time);
    }

    GregorianCalendarDateToModifiedJulianDate(utctime);
    localtime = GregorianCalendarDateAddHour(utctime, 8);
    //gpstime   = GregorianCalendarDateAddSecond(utctime, 18);
    //gpstimews = GregorianCalendarDateToGpsWeekSecond(gpstime);

#if 0
    printf("Local | %d-%.2d-%.2d %.2d:%.2d:%.2d | timezone UTC+8\n",
           localtime.year, localtime.month, localtime.day,
           localtime.hour, localtime.minute, localtime.second);

    printf("UTC   | %d-%.2d-%.2d %.2d:%.2d:%.2d \n",
           utctime.year, utctime.month, utctime.day,
           utctime.hour, utctime.minute, utctime.second);
    fflush(stdout);
#endif
    //snprintf((char *)buf, (size_t)_size, "%02d%02d%02d", localtime.hour, localtime.minute, localtime.second);
    //snprintf((char *)buf, (size_t)_size, "%1d%1d%1d%1d%1d%1d", localtime.year%100, localtime.month, localtime.day, localtime.hour, localtime.minute, localtime.second);
    UTC[0] = localtime.year%100;
    UTC[1] = localtime.month%13;
    UTC[2] = localtime.day%32;
    UTC[3] = localtime.hour%24;
    UTC[4] = localtime.minute%60;
    UTC[5] = localtime.second%60;
    return 0;
}
#endif
#if 1
const char* GB_GMT8_Format(void)
{
    static char format[64];
    struct tm* _tm=NULL;
    time_t _time = time(NULL);
    _tm = localtime(&_time);
    memset(format, 0, sizeof(format));
    snprintf((char *)format, sizeof(format)-1, "%02d-%02d-%02d %02d:%02d:%02d", _tm->tm_year+1900, _tm->tm_mon+1, _tm->tm_mday, _tm->tm_hour, _tm->tm_min, _tm->tm_sec);
    return format;
}
#else
const char* GB_GMT8_Format(void)
{
    static char format[64];
    uint8_t UTC[6];
    time_t _time = time(NULL);
    GB_GMT8(_time, UTC);
    memset(format, 0, sizeof(format));
    snprintf((char *)format, sizeof(format)-1, "%02d-%02d-%02d %02d:%02d:%02d", UTC[0]+2000, UTC[1], UTC[2], UTC[3], UTC[4], UTC[5]);
    return format;
}
#endif
// 将 GMT+8时间(format: YYYY/MM/DD hh:mm:ss)转为 UTC时间
time_t GB_UTC(const uint8_t UTC[6])
{
    struct tm _tm;
    time_t _time;
    _tm.tm_year = UTC[0] + 2000 - 1900;
    _tm.tm_mon  = UTC[1] - 1;
    _tm.tm_mday = UTC[2];
    _tm.tm_hour = UTC[3];
    _tm.tm_min  = UTC[4];
    _tm.tm_sec  = UTC[5];
    _tm.tm_isdst=0;
    _time = mktime(&_tm);
    // mktime 转换出来的时间有 8小时的时间差
    return _time-(8*3600);
}

// 校验码  倒数第 3  String[2]  2 采用 BCC 异或校验法，由两个 ASCII 码（高位在前，低位在后）组成 8 位校验码。校验范围从接入层协议类型开始到数据单元的最后一个字节。
uint8_t GB_17691_BCC_code(const uint8_t data[], const uint32_t len)
{
    uint8_t bcc=0;
    uint32_t count=0;
    bcc = data[0];
    for(count=1; count<len; count++)
    {
        bcc = bcc ^ data[count];
    }
    return bcc;
}

// 数据转换
// 精度转换 float
float pre_conv_float(const int _data, const float offset, const float precision)
{
#if 0
    float data = _data+offset;
    data = data * precision;  // 转换精度
#else
    float data = _data * precision;
    data = data+offset;  // 转换精度
#endif
    return data;
}
// 精度转换 int
int pre_conv_int(const int _data, const float offset, const float precision)
{
    float data = pre_conv_float(_data, offset, precision);
    return (int)data;
}
// 精度转换 double
double pre_conv_dint(const uint32_t _data, const double offset, const double precision)
{
    double data = _data;
    data = data * precision;
    data = data+offset;  // 转换精度
    return data;
}

int GB_17691_encode_login(const struct GB17691_login *const msg, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    uint16_t data_len=0;
    data_len = sizeof (msg->UTC) + sizeof (msg->count) + sizeof (msg->ICCID)-1;
    if(data_len>_size) return ERR_ENCODE_PACKL;
    //memset(buf, 0, _size);
    index=0;
    memcpy(&buf[index], msg->UTC, sizeof (msg->UTC));      // 0  数据采集时间  BYTE[6]  时间定义见 A4.4
    BUILD_BUG_ON(sizeof (msg->UTC) != 6);
    index += 6;
    index += bigend16_encode(&buf[index], msg->count);           // 6  登入流水号  WORD 车载终端每登入一次，登入流水号自动加 1，从 1开始循环累加，最大值为 65531，循环周期为天。
    memcpy(&buf[index], msg->ICCID, sizeof (msg->ICCID)-1); // 10  SIM 卡号  STRING SIM 卡 ICCID 号（ICCID 应为终端从 SIM 卡获取的值，不应人为填写或修改）。
    BUILD_BUG_ON(sizeof (msg->ICCID)-1 != 20);                                               // 倒数第 1  String     1 终止符,固定 ASCII 码“~”，用 0x7E 表示
    index += 20;
    return index; // len
}
int GB_17691_encode_logout(const struct GB17691_logout *const msg, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    uint16_t data_len=0;
    data_len = sizeof (msg->UTC) + sizeof (msg->count);
    if(data_len>_size) return ERR_ENCODE_PACKL;
    //memset(buf, 0, _size);
    index=0;
    memcpy(&buf[index], msg->UTC, sizeof (msg->UTC));       // 0  数据采集时间  BYTE[6]  时间定义见 A4.4
    BUILD_BUG_ON(sizeof (msg->UTC) != 6);
    index += sizeof (msg->UTC);
    index += bigend16_encode(&buf[index], msg->count);           // 6  登入流水号  WORD 车载终端每登入一次，登入流水号自动加 1，从 1开始循环累加，最大值为 65531，循环周期为天。
    return index; // len
}
int GB_17691_decode_logout(struct GB17691_logout *msg, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    uint16_t data_len = 0;
    index=0;
    pr_debug("decode_msg_logout...\n");
    // 数据的总字节长度
    data_len = sizeof (msg->UTC) + sizeof (msg->count);
    //printf("[%s--%d] data_len:%d _size:%d\n", __func__, __LINE__, data_len, _size);
    if(data_len>_size) return ERR_DECODE_LOGOUT;    // error
    // 0  数据采集时间  BYTE[6]  时间定义见 A4.4
    memcpy(msg->UTC, &data[index], 6);
    BUILD_BUG_ON(sizeof (msg->UTC) != 6);
    index += 6;
    // 6  登入流水号  WORD 车载终端每登入一次，登入流水号自动加 1，从 1开始循环累加，最大值为 65531，循环周期为天。
    index += bigend16_merge(&msg->count, data[index], data[index+1]);
    pr_debug("UTC:%s \n", msg->UTC);
    pr_debug("count:%d \n", msg->count);
    return index;
}

int GB_17691_encode_real_obd(const struct GB17691_real_obd* const obd, uint8_t buf[])
{
    uint16_t index=0;
    uint8_t i=0;
    uint8_t fault_total;
    //buf[index++] = GB17691_MSG_OBD; // nmsg->type_msg;
    buf[index++] = obd->protocol;  // OBD 诊断协议  1  BYTE 有效范围 0~2，“0”代表 IOS15765，“1”代表IOS27145，“2”代表 SAEJ1939，“0xFE”表示无效。
    buf[index++] = obd->MIL;       // MIL 状态  1  BYTE 有效范围 0~1，“0”代表未点亮，“1”代表点亮。“0xFE”表示无效。
    index += bigend16_encode(&buf[index], obd->support); // 诊断支持状态  2  WORD
    index += bigend16_encode(&buf[index], obd->ready); // 诊断就绪状态  2  WORD
    // 车辆识别码（VIN） 17  STRING
    memcpy(&buf[index], obd->VIN, sizeof (obd->VIN)-1);
    BUILD_BUG_ON(sizeof (obd->VIN)-1 != 17);
    index += 17;
    // 软件标定识别号 18  STRING 软件标定识别号由生产企业自定义，字母或数字组成，不足后面补字符“0”。
    memcpy(&buf[index], obd->SVIN, sizeof (obd->SVIN)-1);
    BUILD_BUG_ON(sizeof (obd->SVIN)-1 != 18);
    index += 18;
    // 标定验证码（CVN） 18  STRING 标定验证码由生产企业自定义，字母或数字组成，不足后面补字符“0”。
    memcpy(&buf[index], obd->CVN, sizeof (obd->CVN)-1);
    BUILD_BUG_ON(sizeof (obd->CVN)-1 != 18);
    pr_debug("obd->VIN: %s \n", obd->VIN);
    pr_debug("obd->SVIN: %s \n", obd->SVIN);
    pr_debug("obd->CVN: %s \n", obd->SVIN);
    index += 18;
    // IUPR 值  36  DSTRING  定义参考 SAE J 1979-DA 表 G11.
    //memcpy(&buf[index], obd->IUPR, sizeof (obd->IUPR)-1);
    for(i=0; i<18; i++)
    {
        index += bigend16_encode(&buf[index], obd->IUPR[i]);
    }
    BUILD_BUG_ON(sizeof (obd->IUPR) != 36);
    fault_total = obd->fault_total;
    if(fault_total>GB17691_FAUL_MAX) fault_total = GB17691_FAUL_MAX;
    buf[index++] = fault_total;// 故障码总数  1  BYTE  有效值范围：0~253，“0xFE”表示无效。
    // 故障码信息列表 ∑每个故障码信息长度 N*BYTE（4） 每个故障码为四字节，可按故障实际顺序进行排序。
    //printf("[%s--%d] index:%d fault_total:%d\n", __func__, __LINE__, index, fault_total);
    for(i=0; i<fault_total; i++)
    {
        //printf("encode_msg_report_real[%d]: %d %c \n", obd->fault_total, fault_count, fault->data);
        index += bigend32_encode(&buf[index], obd->fault_list[i]);  // 故障码, BYTE（4）
    }
    //printf("[%s--%d] index:%d\n", __func__, __LINE__, index);
    return index;
}
int GB_17691_decode_real_obd(struct GB17691_real_obd* const obd, const uint8_t data[])
{
    uint16_t index=0;
    uint8_t i=0;
    obd->protocol = data[index++];                                     // OBD 诊断协议  1  BYTE 有效范围 0~2，“0”代表 IOS15765，“1”代表IOS27145，“2”代表 SAEJ1939，“0xFE”表示无效。
    obd->MIL = data[index++];                                          // MIL 状态  1  BYTE 有效范围 0~1，“0”代表未点亮，“1”代表点亮。“0xFE”表示无效。
    index += bigend16_merge(&obd->support, data[index], data[index+1]);  // 诊断支持状态  2  WORD
    index += bigend16_merge(&obd->ready, data[index], data[index+1]); // 诊断就绪状态  2  WORD
    // 车辆识别码（VIN） 17  STRING
    memcpy(obd->VIN, &data[index], sizeof (obd->VIN)-1);
    BUILD_BUG_ON(sizeof (obd->VIN)-1 != 17);
    index += 17;
    // 软件标定识别号 18  STRING 软件标定识别号由生产企业自定义，字母或数字组成，不足后面补字符“0”。
    memcpy(obd->SVIN, &data[index], sizeof (obd->SVIN)-1);
    BUILD_BUG_ON(sizeof (obd->SVIN)-1 != 18);
    index += 18;
    // 标定验证码（CVN） 18  STRING 标定验证码由生产企业自定义，字母或数字组成，不足后面补字符“0”。
    memcpy(obd->CVN, &data[index], sizeof (obd->CVN)-1);
    BUILD_BUG_ON(sizeof (obd->CVN)-1 != 18);
    index += 18;
    // IUPR 值  36  DSTRING  定义参考 SAE J 1979-DA 表 G11.
    //memcpy(obd->IUPR, &data[index], sizeof (obd->IUPR)-1);
    for(i=0; i<18; i++)
    {
        index += bigend16_merge(&obd->IUPR[i], data[index], data[index+1]);
    }
    BUILD_BUG_ON(sizeof (obd->IUPR) != 36);
    pr_debug("MSG_OBD VIN: %s \n", obd->VIN);
    pr_debug("MSG_OBD SVIN: %s \n", obd->SVIN);
    pr_debug("MSG_OBD CVN: %s \n", obd->CVN);
    obd->fault_total = data[index++];                                   // 故障码总数  1  BYTE  有效值范围：0~253，“0xFE”表示无效。
    // 故障码信息列表 ∑每个故障码信息长度 N*BYTE（4） 每个故障码为四字节，可按故障实际顺序进行排序。
    //printf("[%s--%d] index:%d fault_total:%d\n", __func__, __LINE__, index, obd->fault_total);
    for(i=0; i<obd->fault_total; i++)
    {
        //printf("decode_msg_report_real[%d]: %d \n", obd->fault_total, fault_count);
        if(i<GB17691_FAUL_MAX) // 最多只保存 GB17691_FAUL_MAX 个故障码
        {
            index += bigend32_merge(&obd->fault_list[i], data[index], data[index+1], data[index+2], data[index+3]);  // 故障码, BYTE（4）
        }
        else index += 4;
    }
    //printf("[%s--%d] index:%d\n", __func__, __LINE__, index);
    return index;
}

int GB_17691_encode_real_stream(const struct GB17691_real_stream* const stream, uint8_t buf[])
{
    uint16_t index=0;
    //buf[index++] = GB17691_MSG_STREAM; // nmsg->type_msg;
    index += bigend16_encode(&buf[index], stream->speed);           // 0  车速  WORD  km/h 数据长度：2btyes 精度：1/256km/h/ bit 偏移量：0 数据范围：0~250.996km/h “0xFF,0xFF”表示无效
    buf[index++] = stream->kPa;                                // 2  大气压力  BYTE  kPa 数据长度：1btyes 精度：0.5/bit 偏移量：0 数据范围：0~125kPa “0xFF”表示无效
    buf[index++] = stream->Nm;                                 // 3  发动机净输出扭矩（实际扭矩百分比）  BYTE  %  数据长度：1btyes精度：1%/bit 偏移量：-125 数据范围：-125~125% “0xFF”表示无效
    buf[index++] = stream->Nmf;                                // 4 摩擦扭矩（摩擦扭矩百分比） BYTE  % 数据长度：1btyes 精度：1%/bit 偏移量：-125 数据范围：-1250~125% “0xFF”表示无效
    index += bigend16_encode(&buf[index], stream->rpm);             // 5  发动机转速  WORD  rpm 数据长度：2btyes 精度：0.125rpm/bit 偏移量：0 数据范围：0~8031.875rpm “0xFF,0xFF”表示无效
    index += bigend16_encode(&buf[index], stream->Lh);              // 7  发动机燃料流量  WORD  L/h 数据长度：2btyes 精度：0.05L/h 偏移量：0 数据范围：0~3212.75L/h “0xFF,0xFF”表示无效
    index += bigend16_encode(&buf[index], stream->ppm_up);          // 9 SCR 上游 NOx 传感器输出值（后处理上游氮氧浓度） WORD  ppm 数据长度：2btyes 精度：0.05ppm/bit 偏移量：-200 数据范围：-200~3212.75ppm “0xFF,0xFF”表示无效
    index += bigend16_encode(&buf[index], stream->ppm_down);        // 11 SCR 下游 NOx 传感器输出值（后处理下游氮氧浓度） WORD  ppm 数据长度：2btyes 精度：0.05ppm/bit 偏移量：-200 数据范围：-200~3212.75ppm  “0xFF,0xFF”表示无效
    buf[index++] = stream->urea_level;                         // 13 反应剂余量 （尿素箱液位） BYTE  % 数据长度：1btyes 精度：0.4%/bit 偏移量：0 数据范围：0~100% “0xFF”表示无效
    index += bigend16_encode(&buf[index], stream->kgh);             // 14  进气量  WORD  kg/h 数据长度：2btyes 精度：0.05kg/h per bit 偏移量：0 数据范围：0~3212.75ppm “0xFF,0xFF”表示无效
    index += bigend16_encode(&buf[index], stream->SCR_in);          // 16 SCR 入口温度（后处理上游排气温度） WORD  ℃ 数据长度：2btyes 精度：0.03125 ℃ per bit 偏移量：-273 数据范围：-273~1734.96875℃ “0xFF,0xFF”表示无效
    index += bigend16_encode(&buf[index], stream->SCR_out);         // 18 SCR 出口温度（后处理下游排气温度） WORD  ℃ 数据长度：2btyes 精度：0.03125 ℃ per bit 偏移量：-273 数据范围：-273~1734.96875℃ “0xFF,0xFF”表示无效
    index += bigend16_encode(&buf[index], stream->DPF);             // 20 DPF 压差（或 DPF排气背压） WORD  kPa 数据长度：2btyes 精度：0.1 kPa per bit 偏移量：0 数据范围：0~6425.5 kPa “0xFF,0xFF”表示无效
    buf[index++] = stream->coolant_temp;                       // 22  发动机冷却液温度  BYTE  ℃ 数据长度：1btyes 精度：1 ℃/bit 偏移量：-40 数据范围：-40~210℃ “0xFF”表示无效
    buf[index++] = stream->tank_level;                         // 23  油箱液位  BYTE  % 数据长度：1btyes 精度：0.4% /bit 偏移量：0 数据范围：0~100% “0xFF”表示无效
    buf[index++] = stream->gps_status;                         // 24  定位状态  BYTE    数据长度：1btyes
    index += bigend32_encode(&buf[index], stream->longitude);       // 25  经度  DWORD   数据长度：4btyes 精度：0.000001° per bit 偏移量：0 数据范围：0~180.000000° “0xFF,0xFF,0xFF,0xFF”表示无效
    index += bigend32_encode(&buf[index], stream->latitude);        // 29  纬度  DWORD   数据长度：4btyes 精度：0.000001 度  per bit 偏移量：0 数据范围：0~180.000000° “0xFF,0xFF,0xFF,0xFF”表示无效
    index += bigend32_encode(&buf[index], stream->mileages_total);  // 33 累计里程 （总行驶里程） DWORD  km 数据长度：4btyes 精度：0.1km per bit 偏移量：0 “0xFF,0xFF,0xFF,0xFF”表示无效
    return index;
}
int GB_17691_encode_real_att(const struct GB17691_real_att* const att, uint8_t buf[])
{
    uint16_t index=0;
    //buf[index++] = GB17691_MSG_STREAM_ATT; // nmsg->type_msg;
    buf[index++] = att->Nm_mode;                            // 0  发动机扭矩模式  BYTE    0：超速失效 1：转速控制 2：扭矩控制 3：转速/扭矩控制 9：正常
    buf[index++] = att->accelerator;                        // 1  油门踏板  BYTE  % 数据长度：1btyes 精度：0.4%/bit 偏移量：0 数据范围：0~100% “0xFF”表示无效
    index += bigend32_encode(&buf[index], att->oil_consume);     // 2 累计油耗 （总油耗） DWORD  L 数据长度：4btyes 精度：0.5L per bit 偏移量：0 数据范围：0~2 105 540 607.5L “0xFF,0xFF,0xFF,0xFF”表示无效
    buf[index++] = att->urea_tank_temp;                     // 6  尿素箱温度  BYTE  ℃ 数据长度：1btyes 精度：1 ℃/bit 偏移量：-40 数据范围：-40~210℃ “0xFF”表示无效
    index += bigend32_encode(&buf[index], att->mlh_urea_actual); // 7  实际尿素喷射量  DWORD  ml/h 数据长度：4btyes 精度：0.01 ml/h per bit 偏移量：0 数据范围：0 “0xFF,0xFF,0xFF,0xFF”表示无效
    index += bigend32_encode(&buf[index], att->mlh_urea_total);  // 11 累计尿素消耗 （总尿素消耗） DWORD  g 数据长度：4btyes 精度：1 g per bit 偏移量：0 数据范围：0  “0xFF,0xFF,0xFF,0xFF”表示无效
    index += bigend16_encode(&buf[index], att->exit_gas_temp);   // 15  DPF 排气温度  WORD  ℃ 数据长度：2btyes 精度：0.03125 ℃ per bit 偏移量：-273 数据范围：-273~1734.96875℃ “0xFF,0xFF”表示无效
    return index;
}
int GB_17691_encode_real_smoke(const struct GB17691_real_smoke* const smoke, uint8_t buf[])
{
    uint16_t index=0;
    index += bigend16_encode(&buf[index], smoke->temperature);   // 0  烟雾排温, 数据长度：2 btyes, 精度：1℃ /bit,
    index += bigend16_encode(&buf[index], smoke->fault);         // 2  OBD（烟雾故障码）,数据长度：2 btyes 精度：1/bit偏移量：0数据范围：“0xFF，0xFF”表示无效
    index += bigend16_encode(&buf[index], smoke->kpa);           // 4  背压, 数据长度：2 btyes, 精度：1 kpa/bit,
    index += bigend16_encode(&buf[index], smoke->m_l);           // 6  光吸收系数,数据长度：2 btyes,精度：0.01 m-l/bit
    index += bigend16_encode(&buf[index], smoke->opacity);       // 8  不透光度,数据长度：2 btyes,精度：0.1%/bit
    index += bigend16_encode(&buf[index], smoke->mg_per_m3);     // 10 颗粒物浓度,数据长度：2 btyes,精度：0.1mg/m3 /bit
    index += bigend16_encode(&buf[index], smoke->light_alarm);   // 12 光吸收系数超标报警,数据长度：2 btyes,精度：1
    index += bigend16_encode(&buf[index], smoke->pressure_alarm);// 14 背压报警,数据长度：2 btyes,精度：1
    index += bigend16_encode(&buf[index], smoke->ppm);
    return index;
}
// A4.5.2  实时信息上报
int GB_17691_encode_real(const struct GB17691_real* const msg, uint8_t buf[], const uint16_t _size)
{
    uint16_t index=0;
    uint16_t data_len=0;
    uint8_t i=0;
    data_len = sizeof (struct GB17691_real);
    if(data_len>_size) return ERR_ENCODE_PACKL;
    memset(buf, 0, _size);
    index=0;
    memcpy(&buf[index], msg->UTC, sizeof (msg->UTC));     // 数据采集时间  6  BYTE[6]  时间定义见 A4.4
    BUILD_BUG_ON(sizeof (msg->UTC) != 6);
    index += 6;
    index += bigend16_encode(&buf[index], msg->count);           // 信息流水号  2  WORD
    // 消息，可包含多条
    for(i=0; i<sizeof(msg->msg)/sizeof(msg->msg[0]); i++)
    {
        if(GB17691_MSG_OBD == msg->msg[i].obd.type)            // 0x01  OBD 信息
        {
            if((index+sizeof (struct GB17691_real_obd))>_size) return ERR_ENCODE_PACKL;
            pr_debug("MSG_OBD : %d \n", index);
            buf[index++] = GB17691_MSG_OBD; // nmsg->type_msg;
            index += GB_17691_encode_real_obd(&msg->msg[i].obd.info, &buf[index]);
        }
        if(GB17691_MSG_STREAM == msg->msg[i].stream.type)      // 0x02  数据流信息
        {
            if((index+sizeof (struct GB17691_real_stream))>_size) return ERR_ENCODE_PACKL;
            pr_debug("MSG_STREAM : %d \n", index);
            buf[index++] = GB17691_MSG_STREAM; // nmsg->type_msg;
            index += GB_17691_encode_real_stream(&msg->msg[i].stream.info, &buf[index]);
        }
        if(GB17691_MSG_STREAM_ATT == msg->msg[i].att.type)     // 0x80  补充数据流
        {
            if((index+sizeof (struct GB17691_real_att))>_size) return ERR_ENCODE_PACKL;
            pr_debug("MSG_STREAM_ATT : %d \n", index);
            buf[index++] = GB17691_MSG_STREAM_ATT; // nmsg->type_msg;
            index += GB_17691_encode_real_att(&msg->msg[i].att.info, &buf[index]);
        }
        if(GB17691_MSG_SMOKE == msg->msg[i].smoke.type)     // 0x80  补充数据流
        {
            if((index+sizeof (struct GB17691_real_smoke))>_size) return ERR_ENCODE_PACKL;
            pr_debug("GB17691_MSG_SMOKE : %d \n", index);
            buf[index++] = GB17691_MSG_SMOKE; // nmsg->type_msg;
            index += GB_17691_encode_real_smoke(&msg->msg[i].smoke.info, &buf[index]);
        }
    }
    return index; // len
}
// 补发信息上报
int GB_17691_encode_later(const struct GB17691_real* const _real, uint8_t buf[], const uint16_t _size)
{
    return GB_17691_encode_real(_real, buf, _size);
}

// 自定义数据
int GB17691_encode_udef(const struct GB17691_udef *const msg, uint8_t buf[], const uint16_t _size)
{
    if(msg->size > _size) return ERR_ENCODE_PACKL;
    memcpy(buf, msg->data, msg->size);
    //printf_hex(msg->data, msg->size);
    return msg->size;//+sizeof (msg->size);
}
int GB17691_decode_udef(struct GB17691_udef *const msg, const uint8_t data[], const uint16_t _size)
{
    if(_size > sizeof(msg->data)) return ERR_ENCODE_PACKL;
    msg->size = _size;//-sizeof (msg->size);
    memcpy(msg->data, data, msg->size);
    //printf_hex(msg->data, msg->size);
    //app_debug("[%s-%d] msg->size[%d] size[%d] \r\n", __func__, __LINE__, msg->size, _size);
    return msg->size;//+sizeof (msg->size);
}


/**
 * 编码一帧数据
 */
int GB_17691_frame_encode(const struct GB17691_frame* const _frame, uint8_t buf[], const uint16_t _size)
{
    uint8_t bcc=0;
    uint16_t index=0;
    int data_len=0;
#ifdef BUILD_MCU_ENCRYPT
    uint8_t rsa_buffer[2048];
#endif
    if(_size<sizeof (struct GB17691_frame)) return ERR_ENCODE_PACKL;
    memset(buf, 0, _size);
    index=0;
    pr_debug("encode_pack_general...\n");
    buf[index++] = _frame->start[0];           // 0  起始符  STRING  固定为 ASCII 字符’##’，用“0x23，0x23”表示
    buf[index++] = _frame->start[1];           // 0  起始符  STRING  固定为 ASCII 字符’##’，用“0x23，0x23”表示
    buf[index++]   = _frame->cmd;              // 2  命令单元  BYTE  命令单元定义见  表 A2  命令单元定义
    memcpy(&buf[index], _frame->VIN, 17);      // 3  车辆识别号  STRING 车辆识别码是识别的唯一标识，由 17 位字码组成，字码应符合 GB16735 中 4.5 的规定
    BUILD_BUG_ON(sizeof (_frame->VIN)-1 != 17);
    index += 17;
    buf[index++] = _frame->SWV;                // 20  终端软件版本号  BYTE  终端软件版本号有效值范围 0~255
    buf[index++] = _frame->ssl;                // 21  数据加密方式  BYTE
    index += bigend16_encode(&buf[index], _frame->data_len); // 22  数据单元长度  WORD
    // 24  数据单元
    switch(_frame->cmd)
    {
        case GB17691_LOGIN:        // 车辆登入
            data_len = GB_17691_encode_login(&_frame->data.login, &buf[index], _size-index-1);
            break;
        case GB17691_REPORT_REAL:  // 实时信息上报
            data_len = GB_17691_encode_real(&_frame->data.real, &buf[index], _size-index-1);
            break;
        case GB17691_REPORT_LATER: // 补发信息上报
            data_len = GB_17691_encode_later(&_frame->data.real, &buf[index], _size-index-1);
            break;
        case GB17691_LOGOUT:       // 车辆登出
            data_len = GB_17691_encode_logout(&_frame->data.logout, &buf[index], _size-index-1);
            break;
        case GB17691_UTC:          // 终端校时
            data_len = 0;      // 车载终端校时的数据单元为空。
            break;
        case GB17691_UDEF:      // 用户自定义
            data_len = GB17691_encode_udef(&_frame->data.udef, &buf[index], _size-index-1);
            break;
        default:
            data_len = 0;
            break;
    }
    if(data_len<0) return ERR_ENCODE_PACKL;
#ifdef BUILD_MCU_ENCRYPT
    // 数据加密
    switch (_frame->ssl)
    {
    /*
        {0x01, "INFO"},  // 0x01：数据不加密
        {0x02, "RSA" },  // 0x02：数据经过 RSA 算法加密
        {0x03, "SM2" },  // 0x03：数据经过国密 SM2 算法加密
        {0xFF, "NULL"},  // “0xFE”标识异常，“0xFF”表示无效，其他预留
     */
        case 0x02:  // RSA
            pr_debug("encode_pack_general RSA data_len: %d\n", data_len);
            data_len = rsa_encrypt(rsa_buffer, sizeof(rsa_buffer), &buf[index], data_len);
            pr_debug("after encode_pack_general RSA data_len: %d\n", data_len);
            if(data_len>0) memcpy(&buf[index], rsa_buffer, data_len);
            break;
        case 0x03:   // SM2
            ;
            break;
        case 0x01:
        default:
            break;
    }
    if(data_len<0) return ERR_ENCODE_PACKL;
#endif
    bigend16_encode(&buf[index-2], data_len); // 22  数据单元长度  WORD
    pr_debug("encode_pack_general data_len: %d\n", data_len);
    index += data_len;
    // 倒数第 1  校验码  BYTE 采用 BCC（异或校验）法，校验范围聪明星单元的第一个字节开始，同后一个字节异或，直到校验码前一字节为止，校验码占用一个字节
    bcc = GB_17691_BCC_code(&buf[2], index-2);  // start: 2 byte
    pr_debug("bcc[%d]:%02X \n", index-1, bcc);
    buf[index++] = bcc;
    return index; // len
}

// 解码
int GB_17691_decode_login(struct GB17691_login *msg, const uint8_t data[], const uint16_t _size)
{
    //pthread_mutex_init();
    uint16_t index=0;
    uint16_t data_len = 0;
    index=0;
    pr_debug("decode_msg_login...\n");
    // 数据的总字节长度,  UTC + count + ICCID
    data_len = sizeof (msg->UTC) + sizeof (msg->count) + sizeof (msg->ICCID) -1;
    if(data_len>_size) return ERR_DECODE_LOGIN;    // error
    // 0  数据采集时间  BYTE[6]  时间定义见 A4.4
    memcpy(msg->UTC, &data[index], 6);
    BUILD_BUG_ON(sizeof (msg->UTC) != 6);
    index += 6;
    // 6  登入流水号  WORD 车载终端每登入一次，登入流水号自动加 1，从 1开始循环累加，最大值为 65531，循环周期为天。
    index += bigend16_merge(&msg->count, data[index], data[index+1]);
    // 10  SIM 卡号  STRING SIM 卡 ICCID 号（ICCID 应为终端从 SIM 卡获取的值，不应人为填写或修改）
    memcpy(msg->ICCID, &data[index], 20);// SIM 卡号  2  String[20]  20  SIM 卡的 ICCID 号（集成电路卡识别码）。由 20 位数字的 ASCII 码构成。
    BUILD_BUG_ON(sizeof (msg->ICCID)-1 != 20);
    index += 20;
    pr_debug("UTC:%s \n", msg->UTC);
    pr_debug("count:%d \n", msg->count);
    pr_debug("ICCID:%s \n", msg->ICCID);
    return index;
}

int GB_17691_decode_real_stream(struct GB17691_real_stream* const stream, const uint8_t data[])
{
    uint16_t index=0;
    //stream->head.type_msg = data[index++];
    index += bigend16_merge(&stream->speed, data[index], data[index+1]);      // 0  车速  WORD  km/h 数据长度：2btyes 精度：1/256km/h/ bit 偏移量：0 数据范围：0~250.996km/h “0xFF,0xFF”表示无效
    stream->kPa = data[index++];                                             // 2  大气压力  BYTE  kPa 数据长度：1btyes 精度：0.5/bit 偏移量：0 数据范围：0~125kPa “0xFF”表示无效
    stream->Nm = data[index++];                                              // 3  发动机净输出扭矩（实际扭矩百分比）  BYTE  %  数据长度：1btyes精度：1%/bit 偏移量：-125 数据范围：-125~125% “0xFF”表示无效
    stream->Nmf = data[index++];                                             // 4 摩擦扭矩（摩擦扭矩百分比） BYTE  % 数据长度：1btyes 精度：1%/bit 偏移量：-125 数据范围：-1250~125% “0xFF”表示无效
    index += bigend16_merge(&stream->rpm, data[index], data[index+1]);       // 5  发动机转速  WORD  rpm 数据长度：2btyes 精度：0.125rpm/bit 偏移量：0 数据范围：0~8031.875rpm “0xFF,0xFF”表示无效
    index += bigend16_merge(&stream->Lh, data[index], data[index+1]);        // 7  发动机燃料流量  WORD  L/h 数据长度：2btyes 精度：0.05L/h 偏移量：0 数据范围：0~3212.75L/h “0xFF,0xFF”表示无效
    index += bigend16_merge(&stream->ppm_up, data[index], data[index+1]);    // 9 SCR 上游 NOx 传感器输出值（后处理上游氮氧浓度） WORD  ppm 数据长度：2btyes 精度：0.05ppm/bit 偏移量：-200 数据范围：-200~3212.75ppm “0xFF,0xFF”表示无效
    index += bigend16_merge(&stream->ppm_down, data[index], data[index+1]);  // 11 SCR 下游 NOx 传感器输出值（后处理下游氮氧浓度） WORD  ppm 数据长度：2btyes 精度：0.05ppm/bit 偏移量：-200 数据范围：-200~3212.75ppm  “0xFF,0xFF”表示无效
    stream->urea_level = data[index++];                                      // 13 反应剂余量 （尿素箱液位） BYTE  % 数据长度：1btyes 精度：0.4%/bit 偏移量：0 数据范围：0~100% “0xFF”表示无效
    index += bigend16_merge(&stream->kgh, data[index], data[index+1]);       // 14  进气量  WORD  kg/h 数据长度：2btyes 精度：0.05kg/h per bit 偏移量：0 数据范围：0~3212.75ppm “0xFF,0xFF”表示无效
    index += bigend16_merge(&stream->SCR_in, data[index], data[index+1]);    // 16 SCR 入口温度（后处理上游排气温度） WORD  ℃ 数据长度：2btyes 精度：0.03125 ℃ per bit 偏移量：-273 数据范围：-273~1734.96875℃ “0xFF,0xFF”表示无效
    index += bigend16_merge(&stream->SCR_out, data[index], data[index+1]);   // 18 SCR 出口温度（后处理下游排气温度） WORD  ℃ 数据长度：2btyes 精度：0.03125 ℃ per bit 偏移量：-273 数据范围：-273~1734.96875℃ “0xFF,0xFF”表示无效
    index += bigend16_merge(&stream->DPF, data[index], data[index+1]);       // 20 DPF 压差（或 DPF排气背压） WORD  kPa 数据长度：2btyes 精度：0.1 kPa per bit 偏移量：0 数据范围：0~6425.5 kPa “0xFF,0xFF”表示无效
    stream->coolant_temp = data[index++];                                    // 22  发动机冷却液温度  BYTE  ℃ 数据长度：1btyes 精度：1 ℃/bit 偏移量：-40 数据范围：-40~210℃ “0xFF”表示无效
    stream->tank_level = data[index++];                                      // 23  油箱液位  BYTE  % 数据长度：1btyes 精度：0.4% /bit 偏移量：0 数据范围：0~100% “0xFF”表示无效
    stream->gps_status = data[index++];                                      // 24  定位状态  BYTE    数据长度：1btyes
    index += bigend32_merge(&stream->longitude, data[index], data[index+1], data[index+2], data[index+3]);       // 25  经度  DWORD   数据长度：4btyes 精度：0.000001° per bit 偏移量：0 数据范围：0~180.000000° “0xFF,0xFF,0xFF,0xFF”表示无效
    index += bigend32_merge(&stream->latitude, data[index], data[index+1], data[index+2], data[index+3]);        // 29  纬度  DWORD   数据长度：4btyes 精度：0.000001 度  per bit 偏移量：0 数据范围：0~180.000000° “0xFF,0xFF,0xFF,0xFF”表示无效
    index += bigend32_merge(&stream->mileages_total, data[index], data[index+1], data[index+2], data[index+3]);  // 33 累计里程 （总行驶里程） DWORD  km 数据长度：4btyes 精度：0.1km per bit 偏移量：0 “0xFF,0xFF,0xFF,0xFF”表示无效
    return index;
}
int GB_17691_decode_real_att(struct GB17691_real_att* const att, uint8_t const data[])
{
    uint16_t index=0;
    //att->head.type_msg = data[index++];
    att->Nm_mode = data[index++];                                 // 0  发动机扭矩模式  BYTE    0：超速失效 1：转速控制 2：扭矩控制 3：转速/扭矩控制 9：正常
    att->accelerator = data[index++];                             // 1  油门踏板  BYTE  % 数据长度：1btyes 精度：0.4%/bit 偏移量：0 数据范围：0~100% “0xFF”表示无效
    index += bigend32_merge(&att->oil_consume, data[index], data[index+1], data[index+2], data[index+3]);     // 2 累计油耗 （总油耗） DWORD  L 数据长度：4btyes 精度：0.5L per bit 偏移量：0 数据范围：0~2 105 540 607.5L “0xFF,0xFF,0xFF,0xFF”表示无效
    att->urea_tank_temp = data[index++];                          // 6  尿素箱温度  BYTE  ℃ 数据长度：1btyes 精度：1 ℃/bit 偏移量：-40 数据范围：-40~210℃ “0xFF”表示无效
    index += bigend32_merge(&att->mlh_urea_actual, data[index], data[index+1], data[index+2], data[index+3]); // 7  实际尿素喷射量  DWORD  ml/h 数据长度：4btyes 精度：0.01 ml/h per bit 偏移量：0 数据范围：0 “0xFF,0xFF,0xFF,0xFF”表示无效
    index += bigend32_merge(&att->mlh_urea_total, data[index], data[index+1], data[index+2], data[index+3]); // 11 累计尿素消耗 （总尿素消耗） DWORD  g 数据长度：4btyes 精度：1 g per bit 偏移量：0 数据范围：0  “0xFF,0xFF,0xFF,0xFF”表示无效
    index += bigend16_merge(&att->exit_gas_temp, data[index], data[index+1]); // 15  DPF 排气温度  WORD  ℃ 数据长度：2btyes 精度：0.03125 ℃ per bit 偏移量：-273 数据范围：-273~1734.96875℃ “0xFF,0xFF”表示无效
    return index;
}
int GB_17691_decode_real_smoke(struct GB17691_real_smoke* const smoke, uint8_t const data[])
{
    uint16_t index=0;
    //smoke->head.type_msg = data[index++];
    index += bigend16_merge(&smoke->temperature, data[index], data[index+1]);   // 0  烟雾排温, 数据长度：2 btyes, 精度：1℃ /bit,
    index += bigend16_merge(&smoke->fault, data[index], data[index+1]);         // 2  OBD（烟雾故障码）,数据长度：2 btyes 精度：1/bit偏移量：0数据范围：“0xFF，0xFF”表示无效
    index += bigend16_merge(&smoke->kpa, data[index], data[index+1]);           // 4  背压, 数据长度：2 btyes, 精度：1 kpa/bit,
    index += bigend16_merge(&smoke->m_l, data[index], data[index+1]);           // 6  光吸收系数,数据长度：2 btyes,精度：0.01 m-l/bit
    index += bigend16_merge(&smoke->opacity, data[index], data[index+1]);       // 8  不透光度,数据长度：2 btyes,精度：0.1%/bit
    index += bigend16_merge(&smoke->mg_per_m3, data[index], data[index+1]);     // 10 颗粒物浓度,数据长度：2 btyes,精度：0.1mg/m3 /bit
    index += bigend16_merge(&smoke->light_alarm, data[index], data[index+1]);   // 12 光吸收系数超标报警,数据长度：2 btyes,精度：1
    index += bigend16_merge(&smoke->pressure_alarm, data[index], data[index+1]); // 14 背压报警,数据长度：2 btyes,精度：1
    index += bigend16_merge(&smoke->ppm, data[index], data[index+1]);            // 16 N0x 值,数据长度：2 btyes,精度：1ppm
    return index;
}
int GB_17691_decode_real(struct GB17691_real* const msg, const uint8_t data[], const uint16_t _size)
{
    uint16_t index=0;
    uint16_t data_len=0;
    struct {
        uint8_t obd;
        uint8_t stream;
        uint8_t att;
        uint8_t smoke;
    }count;
    //struct shanghai_report_real *const msg = (struct shanghai_report_real *const )msg_buf;
    // 缓冲器指针，msg_buf必须足够大，链表将直接使用 msg_buf上面的空间，以避免动态申请内存.
    data_len = sizeof (msg->UTC) + sizeof (msg->count);
    //pr_debug("decode_msg_report_real data_len:%d _size:%d \n", data_len, _size);
    if(data_len>_size) return ERR_DECODE_PACKL;
    memset(msg, 0, sizeof(struct GB17691_real));
    index=0;
    memcpy(msg->UTC, &data[index], sizeof (msg->UTC));     // 数据采集时间  6  BYTE[6]  时间定义见 A4.4
    BUILD_BUG_ON(sizeof (msg->UTC) != 6);
    index += 6;
    index += bigend16_merge(&msg->count, data[index], data[index+1]);           // 信息流水号  2  WORD
    memset(&count, 0, sizeof (count));
    // 消息，可包含多条
    while(index<_size)
    {
        if(GB17691_MSG_OBD == data[index])              // 0x01  OBD 信息
        {
            pr_debug("\nMSG_OBD : %d \n", index);
            msg->msg[count.obd].obd.type = data[index++];
            index += GB_17691_decode_real_obd(&msg->msg[count.obd].obd.info, &data[index]);
            count.obd++;
            if(count.obd>=GB17691_REAL_ITEM_MAX) count.obd=GB17691_REAL_ITEM_MAX-1;
        }
        else if(GB17691_MSG_STREAM == data[index])      // 0x02  数据流信息
        {
            pr_debug("\nMSG_STREAM : %d \n", index);
            msg->msg[count.stream].stream.type = data[index++];
            index += GB_17691_decode_real_stream(&msg->msg[count.stream].stream.info, &data[index]);
            count.stream++;
            if(count.stream>=GB17691_REAL_ITEM_MAX) count.stream=GB17691_REAL_ITEM_MAX-1;
        }
        else if(GB17691_MSG_STREAM_ATT == data[index])  // 0x80  补充数据流
        {
            pr_debug("\nMSG_STREAM_ATT : %d \n", index);
            msg->msg[count.att].att.type = data[index++];
            index += GB_17691_decode_real_att(&msg->msg[count.att].att.info, &data[index]);
            count.att++;
            if(count.att>=GB17691_REAL_ITEM_MAX) count.att=GB17691_REAL_ITEM_MAX-1;
        }
        else if(GB17691_MSG_SMOKE == data[index])  // 0x80  补充数据流
        {
            pr_debug("\nGB17691_MSG_SMOKE : %d \n", index);
            msg->msg[count.smoke].smoke.type = data[index++];
            index += GB_17691_decode_real_smoke(&msg->msg[count.smoke].smoke.info, &data[index]);
            count.smoke++;
            if(count.smoke>=GB17691_REAL_ITEM_MAX) count.smoke=GB17691_REAL_ITEM_MAX-1;
        }
        else
        {
            pr_debug("\nERR_DECODE_OBD[%02X] : %d \n", data[index]&0xFF, index);
            return ERR_DECODE_OBD;
        }
    }
    //pr_debug("msg.msg : %d \n", (NULL!=msg->msg));
    return index; // len
}
/**
 * 解码一帧数据
 */
int GB_17691_frame_decode(struct GB17691_frame* const _frame, const uint8_t data[], const uint16_t _dsize)
{
    uint8_t bcc=0;
    uint16_t index=0;
    int msg_len = 0;
    int data_len=0;
#ifdef BUILD_MCU_ENCRYPT
    uint8_t rsa_buffer[2048];
#endif
    const uint8_t* pdata=NULL;
    index=0;
    // 0  起始符  STRING  固定为 ASCII 字符’##’，用“0x23，0x23”表示
    _frame->start[0] = data[index++];
    _frame->start[1] = data[index++];
    if((GB17691_FRAME_START0!=_frame->start[0]) || (GB17691_FRAME_START1!=_frame->start[1])) return ERR_DECODE_PACKS; // 包头错误
    _frame->cmd = data[index++];                                // 2  命令单元  BYTE  命令单元定义见  表 A2  命令单元定义
    memcpy(_frame->VIN, &data[index], 17);                      // 3  车辆识别号  STRING 车辆识别码是识别的唯一标识，由 17 位字码组成，字码应符合 GB16735 中 4.5 的规定
    index += 17;
    _frame->SWV = data[index++];                                // 20  终端软件版本号  BYTE  终端软件版本号有效值范围 0~255
    _frame->ssl = data[index++];                                // 21  数据加密方式  BYTE
    index += bigend16_merge(&_frame->data_len, data[index], data[index+1]); // 22  数据单元长度  WORD 数据单元长度是数据单元的总字节数，有效范围：0~65531

    //printf("decode_pack_general _dsize:%d | %d \n\n", _dsize, (_frame->data_len+index+1));
    if(_dsize<(_frame->data_len+index+1)) return  ERR_DECODE_PACKL; // 包长度不够
    // 倒数第 1  校验码  BYTE 采用 BCC（异或校验）法，校验范围聪明星单元的第一个字节开始，同后一个字节异或，直到校验码前一字节为止，校验码占用一个字节
    //bcc = BCC_check_code(&data[2], index+_frame->data_len-1);
    bcc = GB_17691_BCC_code(&data[2], index+_frame->data_len-2);  // start: 2 byte
    _frame->BCC = data[index+_frame->data_len];
    //printf("BCC[%d]:%02X bcc:%02X \n\n", index+_frame->data_len, _frame->BCC, bcc);
    if(bcc != _frame->BCC) return ERR_DECODE_PACKBCC;                // BCC 校验错误
    // 数据解谜
    data_len = _frame->data_len;
#ifdef BUILD_MCU_ENCRYPT
    switch (_frame->ssl)
    {
    /*
        {0x01, "INFO"},  // 0x01：数据不加密
        {0x02, "RSA" },  // 0x02：数据经过 RSA 算法加密
        {0x03, "SM2" },  // 0x03：数据经过国密 SM2 算法加密
        {0xFF, "NULL"},  // “0xFE”标识异常，“0xFF”表示无效，其他预留
     */
        case 0x02:  // RSA
            pr_debug("encode_pack_general RSA data_len: %d\n", data_len);
            data_len = rsa_decrypt(rsa_buffer, sizeof(rsa_buffer), &data[index], data_len);
            pdata = rsa_buffer;
            break;
        case 0x03:   // SM2
            pdata = &data[index];
            break;
        case 0x01:
        default:
            pdata = &data[index];
            break;
    }
#else
    pdata = &data[index];
#endif
    // 解码数据
    msg_len = 0;
    switch(_frame->cmd)
    {
        case GB17691_LOGIN:          // 车辆登入  上行
            pr_debug("decode_pack_general CMD_LOGIN \n");
            msg_len = GB_17691_decode_login(&_frame->data.login, pdata, data_len);
            break;
        case GB17691_REPORT_REAL:    // 实时信息上报  上行
            pr_debug("decode_pack_general CMD_REPORT_REAL \n");
            msg_len = GB_17691_decode_real(&_frame->data.real, pdata, data_len);
            break;
        case GB17691_REPORT_LATER:   // 补发信息上报  上行
            pr_debug("decode_pack_general CMD_REPORT_LATER \n");
            msg_len = GB_17691_decode_real(&_frame->data.real, pdata, data_len);
            break;
        case GB17691_LOGOUT:         // 车辆登出  上行
            pr_debug("decode_pack_general CMD_LOGOUT \n");
            msg_len = GB_17691_decode_logout(&_frame->data.logout, pdata, data_len);
            break;
        case GB17691_UTC:            // 终端校时  上行
            pr_debug("decode_pack_general CMD_UTC \n");
            msg_len = 0;
            break;
        case GB17691_UDEF:      // 用户自定义
            pr_debug("decode_pack_general CMD_USERDEF \n");
            msg_len = GB17691_decode_udef(&_frame->data.udef, pdata, data_len);
            break;
        default:
            printf("decode_pack_general default \n");
            break;
    }
    //pr_debug("decode_pack_general msg_len %d \n\n", msg_len);
    if(msg_len<0) return msg_len;
    if(msg_len!=data_len) return ERR_DECODE_OBD;
    index += _frame->data_len;
    // 倒数第 1  校验码  BYTE 采用 BCC（异或校验）法，校验范围聪明星单元的第一个字节开始，同后一个字节异或，直到校验码前一字节为止，校验码占用一个字节
    bcc = GB_17691_BCC_code(&data[2], index-2);  // start: 2 byte
    _frame->BCC = data[index++];
    if(bcc != _frame->BCC) return ERR_DECODE_PACKBCC;                // BCC 校验错误
    //pr_debug("decode_pack_general return %d \n\n", index);
    return  index;
}

/**
 * 校验包函数,仅校验数据包格式,不对数据单元内容校验
 */
int GB_17691_check_frame(const void* const _data, const uint16_t _dsize)
{
    uint8_t bcc=0;
    uint8_t BCC=0;
    uint16_t index=0;
    struct GB17691_frame_check _pack;
    const uint8_t* const data = (const uint8_t*)_data;
    index=0;
    // 0  起始符  STRING  固定为 ASCII 字符’##’，用“0x23，0x23”表示
    _pack.start[0] = data[index++];
    _pack.start[1] = data[index++];
    if(('#'!=_pack.start[0]) || ('#'!=_pack.start[1])) return ERR_DECODE_PACKS; // 包头错误
    _pack.cmd = data[index++];                                // 2  命令单元  BYTE  命令单元定义见  表 A2  命令单元定义
    //memcpy(_pack.VIN, &data[index], 17);                      // 3  车辆识别号  STRING 车辆识别码是识别的唯一标识，由 17 位字码组成，字码应符合 GB16735 中 4.5 的规定
    index += gb_cpy(_pack.VIN, &data[index], 17);             // 3  车辆识别号  STRING 车辆识别码是识别的唯一标识，由 17 位字码组成，字码应符合 GB16735 中 4.5 的规定
    _pack.SWV = data[index++];                                // 20  终端软件版本号  BYTE  终端软件版本号有效值范围 0~255
    _pack.ssl = data[index++];                                // 21  数据加密方式  BYTE
    index += bigend16_merge(&_pack.data_len, data[index], data[index+1]); // 22  数据单元长度  WORD 数据单元长度是数据单元的总字节数，有效范围：0~65531
    //pr_debug("decode_pack_general _dsize:%d | %d \n\n", _dsize, (_pack.data_len+index+1));
    if(_dsize<(_pack.data_len+index+1)) return  ERR_DECODE_PACKL; // 包长度不够
    // 倒数第 1  校验码  BYTE 采用 BCC（异或校验）法，校验范围聪明星单元的第一个字节开始，同后一个字节异或，直到校验码前一字节为止，校验码占用一个字节
    bcc = GB_17691_BCC_code(&data[2], index+_pack.data_len-2);    // start: 2 byte
    index += _pack.data_len;
    BCC = data[index++]; // data[index+_pack.data_len];
    if(bcc != BCC) return ERR_DECODE_PACKBCC;                     // BCC 校验错误
    return  index;
}

/***************************************************** 协议测试代码 ************************************************************/
static const struct GB17691_frame_cmd frame_cmd_list[] = {
    GB17691_FRAME_CMD(GB17691_LOGIN),
    GB17691_FRAME_CMD(GB17691_REPORT_REAL),
    GB17691_FRAME_CMD(GB17691_REPORT_LATER),
    GB17691_FRAME_CMD(GB17691_LOGOUT),
    GB17691_FRAME_CMD(GB17691_UTC),
    GB17691_FRAME_CMD(GB17691_UDEF),
};
static const int frame_cmd_list_size = sizeof(frame_cmd_list)/sizeof(frame_cmd_list[0]);
const char* GB17691_Info(const uint8_t cmd)
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
 * 解码一帧数据
 */
void __GB17691_frame_test(const uint8_t cmd)
{
    struct GB17691_frame frame;
    struct GB17691_frame pack;
    uint8_t data[2048];
    int enlen;
    int delen;
    int i;
    const uint8_t* const en = (const uint8_t*)(&frame);
    const uint8_t* const de = (const uint8_t*)(&pack);

    memset(&frame, 0, sizeof(frame));
    memset(&pack, 0, sizeof(pack));
    memset(data, 0, sizeof(data));
#if 0
    printf("[%s--%d] sizeof(struct GB17691_frame)=%d\r\n", __func__, __LINE__, sizeof(struct GB17691_frame));
    printf("[%s--%d] sizeof(struct GB17691_login)=%d\r\n", __func__, __LINE__, sizeof(struct GB17691_login));
    printf("[%s--%d] sizeof(struct GB17691_logout)=%d\r\n", __func__, __LINE__, sizeof(struct GB17691_logout));
    printf("[%s--%d] sizeof(struct GB17691_real)=%d\r\n", __func__, __LINE__, sizeof(struct GB17691_real));
    printf("[%s--%d]    sizeof(struct GB17691_real_obd)=%d\r\n", __func__, __LINE__, sizeof(struct GB17691_real_obd));
    printf("[%s--%d]    sizeof(struct GB17691_real_stream)=%d\r\n", __func__, __LINE__, sizeof(struct GB17691_real_stream));
    printf("[%s--%d]    sizeof(struct GB17691_real_att)=%d\r\n", __func__, __LINE__, sizeof(struct GB17691_real_att));
#endif

    printf("\r\n[%s--%d] cmd[%02X]:%s\r\n", __func__, __LINE__, cmd, GB17691_Info(cmd));
    frame.start[0] = GB17691_FRAME_START0;
    frame.start[1] = GB17691_FRAME_START1;
    frame.cmd = cmd;
    memcpy(frame.VIN, "VIN1234567890ABCDEFGH", 17);
    frame.SWV = 1;
    frame.ssl = 0;
    // encode
    switch(cmd)
    {
        case GB17691_LOGIN:          // 车辆登入  上行
            memcpy(frame.data.login.UTC, "123456", 6);
            memcpy(frame.data.login.ICCID, "ICCID1234567890ABCDEFGH", 20);
            frame.data.login.count = 10;
            frame.data_len = 28;
            break;
        case GB17691_REPORT_REAL:    // 实时信息上报  上行
            memcpy(frame.data.real.UTC, "123457", 6);
            frame.data.real.count = 0;
            frame.data.real.msg[0].att.type = GB17691_MSG_STREAM_ATT;
            frame.data.real.msg[0].obd.type = GB17691_MSG_OBD;
            frame.data.real.msg[0].stream.type = GB17691_MSG_STREAM;
            //frame.data.real.msg[1].obd.type = GB17691_MSG_OBD;
            frame.data.real.msg[0].obd.info.protocol = 1;
            frame.data.real.msg[0].obd.info.MIL = 1;
            frame.data.real.msg[0].obd.info.support = 123;
            frame.data.real.msg[0].obd.info.ready = 123;
            memcpy(frame.data.real.msg[0].obd.info.VIN, "VIN1234567890ABCDEFGH", 17);
            memcpy(frame.data.real.msg[0].obd.info.SVIN, "SVIN1234567890ABCDEFGH", 18);
            memcpy(frame.data.real.msg[0].obd.info.CVN, "CVN1234567890ABCDEFGH", 18);
            memcpy(frame.data.real.msg[0].obd.info.IUPR, "IUPR1234567890ABCDEFGHIUPR1234567890ABCDEFGH", 36);
            frame.data.real.msg[0].obd.info.fault_total = 3;
            frame.data.real.msg[0].obd.info.fault_list[0] = 0x12345678;
            frame.data.real.msg[0].obd.info.fault_list[1] = 0x16782345;
            frame.data.real.msg[0].obd.info.fault_list[2] = 0x12567348;
            frame.data_len = 173;
            break;
        case GB17691_REPORT_LATER:   // 补发信息上报  上行
            memcpy(frame.data.real.UTC, "123456", 6);
            frame.data.real.count = 11;
            frame.data.real.msg[0].att.type = GB17691_MSG_STREAM_ATT;
            frame.data_len = 162;
            break;
        case GB17691_LOGOUT:         // 车辆登出  上行
            memcpy(frame.data.logout.UTC, "123456", 6);
            frame.data.logout.count = 12;
            frame.data_len = 8;
            break;
        case GB17691_UTC:            // 终端校时  上行
            frame.data_len = 0;
            break;
        case GB17691_UDEF:
            frame.data.udef.size = 20;
            memcpy(frame.data.udef.data, "IUPR1234567890ABCDEFGHIUPR1234567890ABCDEFGH", frame.data.udef.size);
            break;
        default:
            frame.data_len = 1;
            printf("[%s--%d] switch default \n", __func__, __LINE__); fflush(stdout);
            break;
    }
    enlen = GB_17691_frame_encode(&frame, data, sizeof(data));
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
    delen = GB_17691_frame_decode(&pack, data, enlen);
    printf("DECODE decode[%d] BCC[0x%02X]: ", delen, pack.BCC);
    for(i=0; i<42; i++)
    {
        printf("%02X ", de[i]);
    }
    printf("\r\n");
    for(i=0; i<(int)sizeof(struct GB17691_frame); i++)
    {
        if(en[i] != de[i])
        {
            if((24==i) || (25==i)) continue; // LEN
            break;
        }
    }
    printf("DECODE match[%d | %d] [%02X | %02X] de:%d\r\n", (int)sizeof(struct GB17691_frame), i, en[i], de[i], de[i]);
    // decode
    switch(cmd)
    {
        case GB17691_LOGIN:          // 车辆登入  上行
            /*memcpy(frame.data.login.UTC, "123456", 6);
            memcpy(frame.data.login.ICCID, "ICCID1234567890ABCDEFGH", 20);
            frame.data.login.count = 10;
            frame.data_len = 28;*/
            break;
        case GB17691_REPORT_REAL:    // 实时信息上报  上行
#if 0
            printf(".cmd:frame[%d] pack[%d]\r\n", frame.cmd, pack.cmd);
            printf(".VIN:frame[%s] pack[%s]\r\n", frame.VIN, pack.VIN);
            printf(".SWV:frame[%d] pack[%d]\r\n", frame.SWV, pack.SWV);
            printf(".ssl:frame[%d] pack[%d]\r\n", frame.ssl, pack.ssl);
            printf(".data.real.UTC:frame[%s] pack[%s]\r\n", frame.data.real.UTC, pack.data.real.UTC);
            printf(".data.real.count:frame[%d] pack[%d]\r\n", frame.data.real.count, pack.data.real.count);
            printf(".data.real.msg[0].att.type:frame[%02X] pack[%02X]\r\n", frame.data.real.msg[0].att.type, pack.data.real.msg[0].att.type);
            printf(".data.real.msg[0].obd.type:frame[%02X] pack[%02X]\r\n", frame.data.real.msg[0].obd.type, pack.data.real.msg[0].obd.type);
            printf(".data.real.msg[0].stream.type:frame[%02X] pack[%02X]\r\n", frame.data.real.msg[0].stream.type, pack.data.real.msg[0].stream.type);
            printf(".data.real.msg[1].att.type:frame[%02X] pack[%02X]\r\n", frame.data.real.msg[1].att.type, pack.data.real.msg[1].att.type);
            printf(".data.real.msg[1].obd.type:frame[%02X] pack[%02X]\r\n", frame.data.real.msg[1].obd.type, pack.data.real.msg[1].obd.type);
            printf(".data.real.msg[1].stream.type:frame[%02X] pack[%02X]\r\n", frame.data.real.msg[1].stream.type, pack.data.real.msg[1].stream.type);
            printf(".data.real.msg[0].obd.info.protocol:frame[%02X] pack[%02X]\r\n", frame.data.real.msg[0].obd.info.protocol, pack.data.real.msg[0].obd.info.protocol);
            printf(".data.real.msg[0].obd.info.MIL:frame[%02X] pack[%02X]\r\n", frame.data.real.msg[0].obd.info.MIL, pack.data.real.msg[0].obd.info.MIL);
            printf(".data.real.msg[0].obd.info.support:frame[%02X] pack[%02X]\r\n", frame.data.real.msg[0].obd.info.support, pack.data.real.msg[0].obd.info.support);
            printf(".data.real.msg[0].obd.info.ready:frame[%02X] pack[%02X]\r\n", frame.data.real.msg[0].obd.info.protocol, pack.data.real.msg[0].obd.info.ready);
            printf(".data.real.msg[0].obd.info.VIN:frame[%s] pack[%s]\r\n", frame.data.real.msg[0].obd.info.VIN, pack.data.real.msg[0].obd.info.VIN);
            printf(".data.real.msg[0].obd.info.SVIN:frame[%s] pack[%s]\r\n", frame.data.real.msg[0].obd.info.SVIN, pack.data.real.msg[0].obd.info.SVIN);
            printf(".data.real.msg[0].obd.info.CVN:frame[%s] pack[%s]\r\n", frame.data.real.msg[0].obd.info.CVN, pack.data.real.msg[0].obd.info.CVN);
            printf(".data.real.msg[0].obd.info.IUPR:frame[%s] pack[%s]\r\n", frame.data.real.msg[0].obd.info.IUPR, pack.data.real.msg[0].obd.info.IUPR);
            printf(".data.real.msg[0].obd.info.fault_total:frame[%02X] pack[%02X]\r\n", frame.data.real.msg[0].obd.info.fault_total, pack.data.real.msg[0].obd.info.fault_total);
#endif
            break;
        case GB17691_REPORT_LATER:   // 补发信息上报  上行
            /*memcpy(frame.data.real.UTC, "123456", 6);
            frame.data.real.count = 11;
            frame.data.real.msg[0].att.type = GB17691_MSG_STREAM_ATT;
            frame.data_len = 162;*/
            break;
        case GB17691_LOGOUT:         // 车辆登出  上行
            printf(".data.logout.UTC:frame[%s] pack[%s]\r\n", frame.data.logout.UTC, pack.data.logout.UTC);
            printf(".data.logout.count:frame[%d] pack[%d]\r\n", frame.data.logout.count, pack.data.logout.count);
            break;
        case GB17691_UTC:            // 终端校时  上行
            //frame.data_len = 0;
            break;
        case GB17691_UDEF:
            break;
        default:
            //frame.data_len = 1;
            printf("[%s--%d] switch default \n", __func__, __LINE__); fflush(stdout);
            break;
    }
    printf("frame.data_len:[%d] pack.data_len:[%d]\r\n", frame.data_len, pack.data_len);
    fflush(stdout);
}

void GB17691_frame_test(void)
{
    int i;
#if 0
    for(i=0; i<frame_cmd_list_size; i++)
    {
        __ZhengZhou_frame_test(frame_cmd_list[i].cmd);
    }
#else
    //printf("[%s--%d] \n", __func__, __LINE__); fflush(stdout);
    uint8_t cmd_list[] = {
        GB17691_LOGIN,
        GB17691_REPORT_REAL,
        GB17691_REPORT_LATER,
        GB17691_LOGOUT,
        GB17691_UTC,
        GB17691_UDEF,
    };
    const int cmd_list_size = sizeof(cmd_list)/sizeof(cmd_list[0]);
    //printf("[%s--%d] cmd_list_size:%d \n", __func__, __LINE__, cmd_list_size); fflush(stdout);
    for(i=0; i<cmd_list_size; i++)
    {
        //printf("[%s--%d] cmd_list[%d]:%02X\n", __func__, __LINE__, i, cmd_list[i]); fflush(stdout);
        __GB17691_frame_test(cmd_list[i]);
    }
    //printf("[%s--%d] \n", __func__, __LINE__); fflush(stdout);
    //__GB17691_frame_test(GB17691_REPORT_REAL);
#endif
}

