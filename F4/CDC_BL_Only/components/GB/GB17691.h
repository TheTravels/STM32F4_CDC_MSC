/************************ (C) COPYLEFT 2018 Merafour *************************

* File Name          : GB17691.h
* Author             : Merafour
* Last Modified Date : 01/14/2020
* Description        : 国标 17691协议.
* Description        : 见《GB17691-2018 重型柴油车污染物排放限值及测量方法（中国第六阶段）》中附录 Q 规定的有关“远程排放管理车载终端的技术要求及通信数据格式”/《GB17691-2018重型柴油车污染物排放限值及测量方法.pdf》
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
******************************************************************************/
#ifndef _GB17691_H_
#define _GB17691_H_

#include <stdint.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
 extern "C" {
#endif

#ifndef __cplusplus

#define bool _Bool
#define true 1
#define false 0

// 编译软件加密算法时需要定义该宏,使用加密芯片时不使用
#define BUILD_MCU_ENCRYPT  1
#undef  BUILD_MCU_ENCRYPT
#define BUILD_MCU_CCM3310S 1
//#undef  BUILD_MCU_CCM3310S

#endif /* !__cplusplus */

 #define protocol_type_str(_type)   (#_type)

 #define GB17691_FRAME_CMD(cmd)  {(uint8_t)cmd, #cmd}

 struct GB17691_frame_cmd {
     const uint8_t cmd;
     const char* info;
 };

 // "##"
#define GB17691_FRAME_START0     0x23
#define GB17691_FRAME_START1     0x23

#if 0
 // GB17691协议命令单元定义
 enum GB17691_cmd_unit{
     GB17691_LOGIN         = 0x01,       // 0x01       上行 车辆登入
     GB17691_REPORT_REAL   = 0x02,       // 0x02       上行 实时信息上报
     GB17691_REPORT_LATER  = 0x03,       // 0x03       上行 补发信息上报
     GB17691_LOGOUT        = 0x04,       // 0x04       上行 车辆登出
     GB17691_UTC           = 0x05,       // 0x05       上行 终端校时
     GB17691_REV           = 0x06,       // 0x06~0x7F  上行 行数据系统预留
     GB17691_USERDEF       = 0x81,       // 0x81~0xFE      用户自定义
     GB17691_NULL          = 0x00,       // error
 };
#endif
#define GB17691_LOGIN          0x01      // 0x01       上行 车辆登入
#define GB17691_REPORT_REAL    0x02      // 0x02       上行 实时信息上报
#define GB17691_REPORT_LATER   0x03      // 0x03       上行 补发信息上报
#define GB17691_LOGOUT         0x04      // 0x04       上行 车辆登出
#define GB17691_UTC            0x05      // 0x05       上行 终端校时
#define GB17691_REV            0x06      // 0x06~0x7F  上行 行数据系统预留
#define GB17691_UDEF           0x81      // 0x81~0xFE      用户自定义, udef: user define
#define GB17691_NULL           0x00      // error

 // Q.6.4.5.1 车辆登入
 struct GB17691_login{
     uint8_t UTC[6];         // 0   数据采集时间  BYTE[6] 时间定义 Q6.5.4
     uint16_t count;         // 6   登入流水号    WORD 车载终端每登入一次，登入流水号自动加 1，从 1开始循环累加，最大值为 65531，循环周期为天。
     uint8_t ICCID[20+1];    // 10  SIM 卡号     STRING SIM 卡 ICCID 号（ICCID 应为终端从 SIM 卡获取的值，不应人为填写或修改）。
 };
 // Q.6.4.5.3 车辆登出信息
 struct GB17691_logout{
     uint8_t UTC[6];         // 0  数据采集时间  BYTE[6]  时间定义见 Q.6.5.4
     uint16_t count;         // 6  登入流水号    WORD     登出流水号与当次登入流水号一致
 };

 // Q.6.4.5.2 实时信息上报
 /**
        表 Q.6 信息类型
     类型编码       说明
     0x01         OBD 信息
     0x02         数据流信息
     0x03-0x7F     预留
     0x80～0xFE    用户自定义
 */
 /*********************** Q.6.5.5.2.3 信息体 **********************/
#define GB17691_FAUL_MAX    32
 // 1） OBD 信息数据格式和定义见表 Q.7 所示
 struct GB17691_real_obd{
     uint8_t protocol;         // OBD 诊断协议     1   BYTE    有效范围 0~2，“0”代表 IOS15765，“1”代表 IOS27145，“2”代表SAEJ1939，“0xFE”表示无效。
     uint8_t MIL;              // MIL 状态        1   BYTE    有效范围 0~1，“0”代表未点亮，“1”代表点亮，“0xFE”表示无效。
     uint16_t support;         // 诊断支持状态      2   WORD    每一位的含义：0=不支持；1=支持
     uint16_t ready;           // 诊断就绪状态      2   WORD    每一位的含义：0=测试完成或者不支持；1=测试未完成
     uint8_t VIN[17+1];        // 车辆识别码（VIN） 17  STRING   车辆识别码是识别的唯一标识，由 17 位字码构成，字码应符合GB16735 中 4.5 的规定
     uint8_t SVIN[18+1];       // 软件标定识别号    18  STRING   软件标定识别号由生产企业自定义，字母或数字组成，不足后面补字符“0”。
     uint8_t CVN[18+1];        // 标定验证码（CVN） 18  STRING   标定验证码由生产企业自定义，字母或数字组成，不足后面补字符“0”。
     uint16_t IUPR[18];        // IUPR 值         36  DSTRING  定义参考 SAE J 1979-DA 表 G11.
     uint8_t fault_total;      // 故障码总数        1  BYTE     有效值范围：0~253，“0xFE”表示无效。
     uint32_t fault_list[GB17691_FAUL_MAX];  // 故障码信息列表 ∑每个故障码信息长度    N*BYTE（4） 每个故障码为四字节，可按故障实际顺序进行排序。
 };
 // 2）数据流信息数据格式和定义见表 Q.8 所示
 struct GB17691_real_stream{ // 起始字节 数据项 数据类型 描述及要求
     uint16_t speed;          // 0   车速  WORD  km/h 数据长度：2btyes 精度：1/256km/h/ bit 偏移量：0 数据范围：0~250.996km/h “0xFF,0xFF”表示无效
     uint8_t  kPa;            // 2   大气压力  BYTE  kPa 数据长度：1btyes 精度：0.5/bit 偏移量：0 数据范围：0~125kPa “0xFF”表示无效
     uint8_t   Nm;            // 3   发动机净输出扭矩（实际扭矩百分比）  BYTE  %  数据长度：1btyes精度：1%/bit 偏移量：-125 数据范围：-125~125% “0xFF”表示无效
     uint8_t  Nmf;            // 4   摩擦扭矩（摩擦扭矩百分比） BYTE  % 数据长度：1btyes 精度：1%/bit 偏移量：-125 数据范围：-1250~125% “0xFF”表示无效
     uint16_t  rpm;           // 5   发动机转速  WORD  rpm 数据长度：2btyes 精度：0.125rpm/bit 偏移量：0 数据范围：0~8031.875rpm “0xFF,0xFF”表示无效
     uint16_t  Lh;            // 7   发动机燃料流量  WORD  L/h 数据长度：2btyes 精度：0.05L/h 偏移量：0 数据范围：0~3212.75L/h “0xFF,0xFF”表示无效
     uint16_t ppm_up;         // 9   SCR 上游 NOx 传感器输出值（后处理上游氮氧浓度） WORD  ppm 数据长度：2btyes 精度：0.05ppm/bit 偏移量：-200 数据范围：-200~3212.75ppm “0xFF,0xFF”表示无效
     uint16_t ppm_down;       // 11  SCR 下游 NOx 传感器输出值（后处理下游氮氧浓度） WORD  ppm 数据长度：2btyes 精度：0.05ppm/bit 偏移量：-200 数据范围：-200~3212.75ppm  “0xFF,0xFF”表示无效
     uint8_t urea_level;      // 13  反应剂余量 （尿素箱液位） BYTE  % 数据长度：1btyes 精度：0.4%/bit 偏移量：0 数据范围：0~100% “0xFF”表示无效
     uint16_t kgh;            // 14  进气量  WORD  kg/h 数据长度：2btyes 精度：0.05kg/h per bit 偏移量：0 数据范围：0~3212.75ppm “0xFF,0xFF”表示无效
     uint16_t SCR_in;         // 16  SCR 入口温度（后处理上游排气温度） WORD  ℃ 数据长度：2btyes 精度：0.03125 ℃ per bit 偏移量：-273 数据范围：-273~1734.96875℃ “0xFF,0xFF”表示无效
     uint16_t SCR_out;        // 18  SCR 出口温度（后处理下游排气温度） WORD  ℃ 数据长度：2btyes 精度：0.03125 ℃ per bit 偏移量：-273 数据范围：-273~1734.96875℃ “0xFF,0xFF”表示无效
     uint16_t DPF;            // 20  DPF 压差（或 DPF排气背压） WORD  kPa 数据长度：2btyes 精度：0.1 kPa per bit 偏移量：0 数据范围：0~6425.5 kPa “0xFF,0xFF”表示无效
     uint8_t coolant_temp;    // 22  发动机冷却液温度  BYTE  ℃ 数据长度：1btyes 精度：1 ℃/bit 偏移量：-40 数据范围：-40~210℃ “0xFF”表示无效
     uint8_t tank_level;      // 23  油箱液位  BYTE  % 数据长度：1btyes 精度：0.4% /bit 偏移量：0 数据范围：0~100% “0xFF”表示无效
     uint8_t gps_status;      // 24  定位状态  BYTE    数据长度：1btyes
     uint32_t longitude;      // 25  经度  DWORD   数据长度：4btyes 精度：0.000001° per bit 偏移量：0 数据范围：0~180.000000° “0xFF,0xFF,0xFF,0xFF”表示无效
     uint32_t latitude;       // 29  纬度  DWORD   数据长度：4btyes 精度：0.000001 度  per bit 偏移量：0 数据范围：0~180.000000° “0xFF,0xFF,0xFF,0xFF”表示无效
     uint32_t mileages_total; // 33  累计里程 （总行驶里程） DWORD  km 数据长度：4btyes 精度：0.1km per bit 偏移量：0 “0xFF,0xFF,0xFF,0xFF”表示无效
 };
 /*
    表 Q.9 状态位定义
   位   状态
   0    0:有效定位；1:无效定位（当数据通信正常，而不能获取定位信息时，发送最后一次有效定位信息，并将定位状态置为无效）。
   1    0:北纬；1:南纬。
   2    0:东经；1:西经。
   3-7  保留。
 */
 // 表 A.8 补充数据流数据格式和定义
 struct GB17691_real_att{
     uint8_t Nm_mode;           // 0   发动机扭矩模式  BYTE    0：超速失效 1：转速控制 2：扭矩控制 3：转速/扭矩控制 9：正常
     uint8_t  accelerator;      // 1   油门踏板  BYTE  % 数据长度：1btyes 精度：0.4%/bit 偏移量：0 数据范围：0~100% “0xFF”表示无效
     uint32_t oil_consume;      // 2   累计油耗 （总油耗） DWORD  L 数据长度：4btyes 精度：0.5L per bit 偏移量：0 数据范围：0~2 105 540 607.5L “0xFF,0xFF,0xFF,0xFF”表示无效
     uint8_t urea_tank_temp;    // 6   尿素箱温度  BYTE  ℃ 数据长度：1btyes 精度：1 ℃/bit 偏移量：-40 数据范围：-40~210℃ “0xFF”表示无效
     uint32_t mlh_urea_actual;  // 7   实际尿素喷射量  DWORD  ml/h 数据长度：4btyes 精度：0.01 ml/h per bit 偏移量：0 数据范围：0 “0xFF,0xFF,0xFF,0xFF”表示无效
     uint32_t mlh_urea_total;   // 11  累计尿素消耗 （总尿素消耗） DWORD  g 数据长度：4btyes 精度：1 g per bit 偏移量：0 数据范围：0  “0xFF,0xFF,0xFF,0xFF”表示无效
     uint16_t exit_gas_temp;    // 15  DPF 排气温度  WORD  ℃ 数据长度：2btyes 精度：0.03125 ℃ per bit 偏移量：-273 数据范围：-273~1734.96875℃ “0xFF,0xFF”表示无效
     // ;
 };
 struct GB17691_real_smoke{
     uint16_t temperature;   // 0  烟雾排温, 数据长度：2 btyes, 精度：1℃ /bit, 偏移量：0, 数据范围：“0xFF，0xFF”表示无效
     uint16_t fault;         // 2  OBD（烟雾故障码）,数据长度：2 btyes 精度：1/bit偏移量：0数据范围：“0xFF，0xFF”表示无效
     uint16_t kpa;           // 4  背压, 数据长度：2 btyes, 精度：1 kpa/bit,
     uint16_t m_l;           // 6  光吸收系数,数据长度：2 btyes,精度：0.01 m-l/bit
     uint16_t opacity;       // 8  不透光度,数据长度：2 btyes,精度：0.1%/bit
     uint16_t mg_per_m3;     // 10 颗粒物浓度,数据长度：2 btyes,精度：0.1mg/m3 /bit
     uint16_t light_alarm;   // 12 光吸收系数超标报警,数据长度：2 btyes,精度：1
     uint16_t pressure_alarm;// 14 背压报警,数据长度：2 btyes,精度：1
     uint16_t ppm;           // 16 N0x 值,数据长度：2 btyes,精度：1ppm
};
 enum GB17691_real_type{
     GB17691_MSG_OBD        = 0x01,   // 0x01  OBD 信息
     GB17691_MSG_STREAM     = 0x02,   // 0x02  数据流信息
     // 0x03-0x7F  预留
     GB17691_MSG_STREAM_ATT = 0x80,   // 0x80  补充数据流
     // 0x81~0xFE  用户自定义
     GB17691_MSG_SMOKE      = 0x81,   // 0x81  包含烟雾的数据流信息(自定义)
 };
 // Q.6.5.5.2.1 实时信息上报格式
 #define GB17691_REAL_ITEM_MAX    2
 //#define GB17691_FRAME_SIZE       1024  // 帧数据单元大小
 #define GB17691_FRAME_SIZE       (1024+512)  // 帧数据单元大小
 struct GB17691_real_msgs{                     // 实时信息
     struct {                 // OBD
         uint8_t type;        // 消息类型
         struct GB17691_real_obd info; // 信息体
     }obd;
     struct {                 // 流
         uint8_t type;        // 消息类型
         struct GB17691_real_stream info; // 信息体
     }stream;
     struct {                 // 附加数据
         uint8_t type;        // 消息类型
         struct GB17691_real_att info;    // 信息体
     }att;
     struct {                 // 烟感数据
         uint8_t type;        // 消息类型
         struct GB17691_real_smoke info;  // 信息体
     }smoke;
 };
 struct GB17691_real{
     uint8_t UTC[6];              // 6  数据采集时间  BYTE[6]  时间定义 Q6.5.4
     uint16_t count;              // 2  信息流水号    WORD     以天为单位，每包实时信息流水号唯一，从 1 开始累加。
     struct GB17691_real_msgs msg[GB17691_REAL_ITEM_MAX];
 };

 // 0x81~0xFE  用户自定义, udef: user define
 struct GB17691_udef{
     uint16_t size;
     uint8_t data[GB17691_FRAME_SIZE-4];
 };

 // 加密定义
 // 0x01 = 不加密；0x02 = 非对称 RSA 算法；0x03 = 非对称国密 SM2 算法；0x04 =对称 SM4 算法；0x05 = 对称 AES128 算法；0xFE = 异常；0xFF = 无效。
 enum GB17691_SSL
 {
     GB_SSL_NULL    = 0x00,  // default GB_SSL_SM2
     GB_SSL_INFO    = 0x01,
     GB_SSL_RSA     = 0x02,
     GB_SSL_SM2     = 0x03,
     GB_SSL_SM4     = 0x04,
     GB_SSL_AES128  = 0x05,
     GB_SSL_FAIL    = 0xFE,
     GB_SSL_SM2_V1  = 0xFF,
     GB_SSL_INVALID = 0xFD,  // 用于提取已解密数据
 };
 // GB17691协议帧结构
 struct GB17691_frame{
     uint8_t start[2];     //  0       起始符        STRING   固定为 ASCII 字符’##’，用“0x23，0x23”表示
     uint8_t cmd;          //  2       命令单元       BYTE    命令单元定义见表 Q.3
     uint8_t VIN[17+1];    //  3       车辆识别号     STRING   车辆识别码是识别的唯一标识，由 17 位字码组成，字码应符合 GB16735 中 4.5 的规定
     uint8_t SWV;          // 20       终端软件版本号  BYTE     终端软件版本号有效值范围 0~255
     uint8_t ssl;          // 21       数据加密方式    BYTE    0x01：数据不加密；0x02：数据经过 RSA 算法; 加密；0x03：数据经过国密 SM2 算法加密；“0xFE”标识异常，“0xFF”表示无效，其他预留
     uint16_t data_len;    // 22       数据单元长度    WORD     数据单元长度是数据单元的总字节数，有效范围：0~65531
     //void*   data;       // 24       数据单元       BYTE[]   数据单元格式和定义见 Q.6.5.5
     union {               // 24       数据单元       BYTE[]   数据单元格式和定义见 Q.6.5.5
         uint8_t buf[GB17691_FRAME_SIZE];
         struct GB17691_login login;
         struct GB17691_logout logout;
         struct GB17691_real real;
         struct GB17691_udef udef;  // 用户自定义
     }data;
     uint8_t BCC;          // 倒数第 1  校验码         BYTE     采用 BCC（异或校验）法，校验范围从命令单元的第一个字节开始，同后一字节异或，直到校验码前一字节为止，校验码占用一个字节
 };
 // GB17691_frame_check结构用于包校验，无数据单元
 struct GB17691_frame_check{
     uint8_t start[2];     //  0       起始符        STRING   固定为 ASCII 字符’##’，用“0x23，0x23”表示
     uint8_t cmd;          //  2       命令单元       BYTE    命令单元定义见表 Q.3
     uint8_t VIN[17+1];    //  3       车辆识别号     STRING   车辆识别码是识别的唯一标识，由 17 位字码组成，字码应符合 GB16735 中 4.5 的规定
     uint8_t SWV;          // 20       终端软件版本号  BYTE     终端软件版本号有效值范围 0~255
     uint8_t ssl;          // 21       数据加密方式    BYTE    0x01：数据不加密；0x02：数据经过 RSA 算法; 加密；0x03：数据经过国密 SM2 算法加密；“0xFE”标识异常，“0xFF”表示无效，其他预留
     uint16_t data_len;    // 22       数据单元长度    WORD     数据单元长度是数据单元的总字节数，有效范围：0~65531
     uint8_t BCC;          // 倒数第 1  校验码         BYTE     采用 BCC（异或校验）法，校验范围从命令单元的第一个字节开始，同后一字节异或，直到校验码前一字节为止，校验码占用一个字节
 };

#ifndef ERR_DECODE_PACKS
#define ERR_DECODE_PACKS         -1      // 包头错误
#define ERR_DECODE_PACKL         -2      // buffer长度错误
#define ERR_DECODE_PACKBCC       -3      // BCC校验错误
#define ERR_DECODE_LOGIN         -4      // 登录包错误
#define ERR_DECODE_LOGOUT        -5      // 登出包错误
#define ERR_DECODE_OBD           -6      // OBD包错误
#define ERR_DECODE_ERR_ENCRYPT   -7      // 加密/解密错误
#define ERR_DECODE_ERR_SIGN      -8      // 签名/验签错误

#define ERR_ENCODE_PACKL         -31     // buffer长度错误
 
#define ERR_DECODE_NODATA        -40     // 没有数据包错误
#define ERR_DECODE_QUERY         -41     // 请求包错误
#define ERR_DECODE_DOWNLOAD      -42     // 下载包错误
#define ERR_DECODE_QUERY_ACK     -43     // 请求响应包错误
#define ERR_DECODE_DOWNLOAD_ACK  -44     // 下载响应包错误
#define ERR_DECODE_UTC_ACK       -45     // 校时响应包错误
#define ERR_DECODE_GPS_ACK       -46     // GPS响应包错误

#define ERR_CLIENT_PACKS         -1      // 包错误
#define ERR_CLIENT_CMD           -2      //
#define ERR_CLIENT_DOWN          -31     //
#define STATUS_CLIENT_NULL       0       //
#define STATUS_CLIENT_DOWN       1       //
#define STATUS_CLIENT_DONE       2       //
#define STATUS_CLIENT_PUSH       3       //
#endif
 
#define  GBL_RESP_SUC                     0         // 
#define  GBL_RESP_ERR_CMD                -1         // 命令错误
#define  GBL_RESP_ERR_PACK               -2         // 包错误
#define  GBL_RESP_ERR_VER                -3         // 版本错误
#define  GBL_RESP_ERR_ENCODE             -4         // 编码错误
#define  GBL_RESP_ERR_DECODE             -5         // 解码错误
#define  GBL_RESP_ERR_READ               -6         // ---
#define  GBL_RESP_ERR_DATA               -7         // 数据错误
#define  GBL_RESP_ERR_UNDATA             -8         // 无数据更新
#define  GBL_RESP_ERR_UPLOAD             -9         // 更新中
#define  GBL_RESP_ERR_DECODE_PACKS       -10        // 包头错误
#define  GBL_RESP_ERR_DECODE_PACKL       -11        // buffer长度错误
#define  GBL_RESP_ERR_ENCODE_PACKL       -12        // BCC校验错误
#define  GBL_RESP_ERR_DECODE_PACKBCC     -13        // buffer长度错误
#define  GBL_RESP_ERR_DECODE_DATA        -14        // 数据单元错误

// Verify that this architecture packs as expected.
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

// 校验码  倒数第 3  String[2]  2 采用 BCC 异或校验法，由两个 ASCII 码（高位在前，低位在后）组成 8 位校验码。校验范围从接入层协议类型开始到数据单元的最后一个字节。
extern uint8_t GB_17691_BCC_code(const uint8_t data[], const uint32_t len);
// 编码一帧数据
extern int GB_17691_frame_encode(const struct GB17691_frame* const _frame, uint8_t buf[], const uint16_t _size);
// 登录
extern int GB_17691_encode_login(const struct GB17691_login *const msg, uint8_t buf[], const uint16_t _size);
// 登出
extern int GB_17691_encode_logout(const struct GB17691_logout *const msg, uint8_t buf[], const uint16_t _size);
// 实时数据OBD
extern int GB_17691_encode_real_obd(const struct GB17691_real_obd* const obd, uint8_t buf[]);
// 实时数据流
extern int GB_17691_encode_real_stream(const struct GB17691_real_stream* const stream, uint8_t buf[]);
// 实时数据补充数据
extern int GB_17691_encode_real_att(const struct GB17691_real_att* const att, uint8_t buf[]);
extern int GB_17691_encode_real_smoke(const struct GB17691_real_smoke* const smoke, uint8_t buf[]);
// 实时
extern int GB_17691_encode_real(const struct GB17691_real* const msg, uint8_t buf[], const uint16_t _size);
// 补发
extern int GB_17691_encode_later(const struct GB17691_real* const _real, uint8_t buf[], const uint16_t _size);
// 解码一帧数据
extern int GB_17691_frame_decode(struct GB17691_frame* const _frame, const uint8_t data[], const uint16_t _dsize);
extern int GB_17691_decode_login(struct GB17691_login *msg, const uint8_t data[], const uint16_t _size);
extern int GB_17691_decode_logout(struct GB17691_logout *msg, const uint8_t data[], const uint16_t _size);
extern int GB_17691_decode_real_obd(struct GB17691_real_obd* const obd, const uint8_t data[]);
extern int GB_17691_decode_real_stream(struct GB17691_real_stream* const stream, const uint8_t data[]);
extern int GB_17691_decode_real_att(struct GB17691_real_att* const att, uint8_t const data[]);
extern int GB_17691_decode_real_smoke(struct GB17691_real_smoke* const smoke, uint8_t const data[]);
extern int GB_17691_decode_real(struct GB17691_real* const msg, const uint8_t data[], const uint16_t _size);
// 自定义数据
extern int GB17691_encode_udef(const struct GB17691_udef *const msg, uint8_t buf[], const uint16_t _size);
extern int GB17691_decode_udef(struct GB17691_udef *const msg, const uint8_t data[], const uint16_t _size);

extern int GB_17691_check_frame(const void* const _data, const uint16_t _dsize);

extern void __GB17691_frame_test(const uint8_t cmd);
extern void GB17691_frame_test(void);

/******************************************************** 内联函数 *************************************************************************/
// copy函数,返回拷贝数据长度
#if 1
static inline uint16_t gb_cpy(void* const _Dst,const void* const _Src, const uint16_t _Size)
{
    memcpy(_Dst, _Src, _Size);
    return _Size;
}
#else
extern uint16_t gb_cpy(void* const _Dst,const void* const _Src, const uint16_t _Size);
#endif


#ifdef __cplusplus
}
#endif


#endif // _GB17691_H_
