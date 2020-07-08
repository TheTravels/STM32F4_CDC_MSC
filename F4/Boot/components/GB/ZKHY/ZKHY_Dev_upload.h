/************************ (C) COPYLEFT 2018 Merafour *************************

* File Name          : ZKHY_Dev_upload.h
* Author             : Merafour
* Last Modified Date : 05/09/2020
* Description        : 正科环宇设备升级协议.该协议定义正科环宇设备的网络分包升级、FTP升级、远程烧号等功能
* Description        : 见《正科环宇设备升级协议.pdf》
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
******************************************************************************/
#ifndef _ZKHY_DEV_UPLOAD_H_
#define _ZKHY_DEV_UPLOAD_H_

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
 extern "C" {
#endif

// STR
#define ZKHY_UPLOAD_FRAME_STR0     0x5A
#define ZKHY_UPLOAD_FRAME_STR1     0x20
// 命令单元定义 
enum ZKHY_cmd_Upload
{
    //                                      // 编码   定义                    方向     描述及要求
    // 通用指令
    ZKHY_UPLOAD_LOGIN         = 0x10,       // 0x10  登入                     上行
    ZKHY_UPLOAD_LOGINA        = 0x11,       // 0x11  登入响应                 下行
    ZKHY_UPLOAD_LOGOUT        = 0x12,       // 0x12  登出                     上行  
    ZKHY_UPLOAD_LOGOUTA       = 0x13,       // 0x13  登出响应                 下行  
    ZKHY_UPLOAD_VIN_REQ       = 0x14,       // 0x14  请求VIN码                上行  
    ZKHY_UPLOAD_VIN_ACK       = 0x15,       // 0x15  请求VIN码响应            下行  
    ZKHY_UPLOAD_UTC_REQ       = 0x16,       // 0x16  校时请求                 上行  
    ZKHY_UPLOAD_UTC_ACK       = 0x17,       // 0x17  校时请求响应             下行 
    // 分包下载指令
    ZKHY_UPLOAD_DOWN_REQ      = 0x20,       // 0x20  分包下载请求             上行  
    ZKHY_UPLOAD_DOWN_ACK      = 0x21,       // 0x21  下发分包数据响应         下行 
    ZKHY_UPLOAD_CFG_REQ       = 0x22,       // 0x22  查询配置文件更新         上行  
    ZKHY_UPLOAD_CFG_ACK       = 0x23,       // 0x23  下发配置文件更新         下行 
    ZKHY_UPLOAD_FW_REQ        = 0x24,       // 0x24  查询固件更新             上行  
    ZKHY_UPLOAD_FW_ACK        = 0x25,       // 0x25  下发固件更新             下行 
    ZKHY_UPLOAD_FWB_REQ       = 0x26,       // 0x26  查询固件块信息           上行  
    ZKHY_UPLOAD_FWB_ACK       = 0x27,       // 0x27  下发固件块信息           下行 
    // 分包下载客户(client)指令
    ZKHY_UPLOAD_CCFG_REQ      = 0x30,       // 0x30  查询配置文件更新         上行  
    //ZKHY_UPLOAD_CCFG_ACK      = 0x23,       // 0x23  下发配置文件更新         下行 
    ZKHY_UPLOAD_CFW_REQ       = 0x32,       // 0x32  查询固件更新             上行  
    //ZKHY_UPLOAD_CFW_ACK       = 0x25,       // 0x25  下发固件更新             下行 
    ZKHY_UPLOAD_CFWB_REQ      = 0x34,       // 0x34  查询固件块信息           上行  
    //ZKHY_UPLOAD_CFWB_ACK      = 0x27,       // 0x27  下发固件块信息           下行 
    // FTP 指令
    // 远程烧号(0x50-0x5F)
    // 嵌入式升级 (0x60-0x6F)
    ZKHY_EMB_SYNC             = 0x60,       // 0x60  设备同步
    ZKHY_EMB_SYNCA            = 0x61,       // 0x61  设备同步响应
    ZKHY_EMB_ERASE            = 0x62,       // 0x62  擦除设备
    ZKHY_EMB_ERASEA           = 0x63,       // 0x63  擦除设备状态
    ZKHY_EMB_WRITE            = 0x64,       // 0x64  分包写入
    ZKHY_EMB_WRITEA           = 0x65,       // 0x65  分包写入状态
    ZKHY_EMB_READ             = 0x66,       // 0x66  分包读取
    ZKHY_EMB_READA            = 0x67,       // 0x67  分包读取返回数据
    ZKHY_EMB_BOOT             = 0x68,       // 0x68  引导 APP
    ZKHY_EMB_BOOTA            = 0x69,       // 0x69  引导 APP状态
    ZKHY_EMB_REBOOT           = 0x6A,       // 0x6A  复位设备
    ZKHY_EMB_REBOOTA          = 0x6B,       // 0x6B  复位设备状态
};

#define  ZKHY_RESP_SUC           0         // 
#define  ZKHY_RESP_ERR_CMD      -1         // 命令错误
#define  ZKHY_RESP_ERR_PACK     -2         // 包错误
#define  ZKHY_RESP_ERR_VER      -3         // 版本错误
#define  ZKHY_RESP_ERR_ENCODE   -4         // 编码错误
#define  ZKHY_RESP_ERR_DECODE   -5         // 解码错误
#define  ZKHY_RESP_ERR_READ     -6         // 
#define  ZKHY_RESP_ERR_DATA     -7         // 数据错误
#define  ZKHY_RESP_ERR_UNDATA   -8         // 无数据更新
#define  ZKHY_RESP_ERR_UPLOAD   -9         // 更新中
#define  ZKHY_RESP_ERR_DECODE_PACKS       -10        // 包头错误
#define  ZKHY_RESP_ERR_DECODE_PACKL       -11        // buffer长度错误
#define  ZKHY_RESP_ERR_ENCODE_PACKL       -12        // BCC校验错误
#define  ZKHY_RESP_ERR_DECODE_PACKBCC     -13        // buffer长度错误
#define  ZKHY_RESP_ERR_DECODE_DATA        -14        // 数据单元错误

// 登录
struct ZKHY_Frame_login{
    char sn[32];       // 32位序列号，不足时用0x00补足
    char VIN[17];      // 车辆识别码是识别的唯一标识，由17位字码组成，字符符合GB16735中4.5的规定
    char Model[16];    // 设备型号标识，不足时用 0x00补足
    char ICCID[20];    // SIM卡号
    char Ver[32];      // 设备软件版本号，设备软件的文本标识，不足时用 0x00补足，如："2.3.1-hw2.0-HSH.Gen"
};
struct ZKHY_Frame_login_ack{
    uint32_t fw;        // 服务器固件校验码
    uint32_t cfg;       // 服务器配置文件校验码
};
// 下载
struct ZKHY_Frame_download{
    char key[32];       // 关键字/文件索引 ID
    uint32_t seek;      // 文件偏移
    uint32_t total;     // 下载文件大小
    uint16_t block;     // 该次下载的数据大小,建议为 512*n(n为整数)
};
struct ZKHY_Frame_download_ack{
    uint32_t seek;      // 文件偏移
    uint32_t total;     // 下载文件大小
    uint16_t block;     // 返回的数据长度,一般与下载请求中block值相同，不足时为实际长度
    char data[1024];    // 下载的数据,block字节
};
// 分包请求
struct ZKHY_Frame_upload_req{
    char sn[32];       // 32位序列号，不足时用0x00补足
    char Model[16];    // 设备型号标识，不足时用0x00补足
    uint32_t checksum;  // 需要更新的设备中的配置文件/固件的校验码
};
// 客户分包请求
struct ZKHY_Frame_upload_req_client{
    char sn[32];       // 32位序列号，不足时用0x00补足
    uint32_t checksum;  // 需要更新的设备中的配置文件/固件的校验码
};
// 请求响应
struct ZKHY_Frame_upload_ack{
    char key[32];       // 关键字/文件索引 ID
    uint32_t checksum;  // 需要更新的设备中的配置文件/固件的校验码
    uint32_t total;     // 预下载文件大小,0表示不需要更新
};
// 块请求响应
struct ZKHY_Frame_upload_ackb{
    char key[32];       // 关键字/文件索引 ID
    uint8_t map[128];   // 块信息表, 1bit表示一个块是否需要下载的标志,0表示该块区有数据,1表示该块区全为value可跳过下载
    uint32_t checksum;  // 需要更新的设备中的配置文件/固件的校验码
    uint32_t total;     // 预下载文件大小,0表示不需要更新
    uint8_t value;      // 块填充值
    uint8_t block;      // 块大小,map中1bit所代表的大小,为 block*1024
};

/******************* 嵌入式升级 ********************/
/*******************
附录D：存储区编号
0	"Chip"	对编号0存储区擦除芯片将变成裸片,且不支持读写操作,慎重！！
1	"Flash"	芯片内部Flash，通常为App存储区域，固件升级用
2	"OTP"	一次性存储区，通常为芯片内部,且不可擦除
3	"UID"	芯片ID(唯一设备)存储区,通常为芯片内部,且为只读
4	"Key"	加密数据，通常用于对软件进行保护
5	"Param Table"	参数表存储区，用于存储设备参数表如序列号等,应为可读可写
6	"SPI Flash"	外部Flash，以SPI挂载，通常容量较小
7	"SDIO Flash"	外部Flash，以SDIO挂载，通常容量较大
8	"ROM1"	映射存储区1
9	"ROM2"	映射存储区2
10	"ROM3"	映射存储区3
11	"UART"	UART外设存储区,用于操作 UART进行数据收发
********************/
enum Emb_Store_number{
    EMB_STORE_CHIP        = 0,   // 0	"Chip"	对编号0存储区擦除芯片将变成裸片,且不支持读写操作,慎重！！
    EMB_STORE_FLASH       = 1,   // 1	"Flash"	芯片内部Flash，通常为App存储区域，固件升级用
    EMB_STORE_OTP         = 2,   // 2	"OTP"	一次性存储区，通常为芯片内部,且不可擦除
    EMB_STORE_UID         = 3,   // 3	"UID"	芯片ID(唯一设备)存储区,通常为芯片内部,且为只读
    EMB_STORE_KEY         = 4,   // 4	"Key"	加密数据，通常用于对软件进行保护
    EMB_STORE_PARAM       = 5,   // 5	"Param Table"	参数表存储区，用于存储设备参数表如序列号等,应为可读可写
    EMB_STORE_FLASI_SPI   = 6,   // 6	"SPI Flash"	外部Flash，以SPI挂载，通常容量较小
    EMB_STORE_FLASH_SDIO  = 7,   // 7	"SDIO Flash"	外部Flash，以SDIO挂载，通常容量较大
    EMB_STORE_ROM1        = 8,   // 8	"ROM1"	映射存储区1
    EMB_STORE_ROM2        = 9,   // 9	"ROM2"	映射存储区2
    EMB_STORE_ROM3        = 10,  // 10	"ROM3"	映射存储区3
    EMB_STORE_UART        = 11,  // 11	"UART"	UART外设存储区,用于操作 UART进行数据收发
    EMB_STORE_NONE        = 255, // 255	空,不操作
};
// 设备同步
struct ZKHY_Frame_Emb_sync{
    uint32_t KK1[4];       // K1密文，协商密钥
    uint32_t KK2[4];       // K2密文PC端密钥
    uint8_t flag[64];       // 以0x7F填充，作为同步检测标志，主要用于手动获取设备版本信息
};
// 注：校验码 0 为无效值。Boot返回时无app返回0。
struct ZKHY_Frame_Emb_synca{
    uint32_t Kk1[4];       // K1密文，协商密钥
    uint32_t Kk3[4];       // K3密文，设备端密钥
    char boot_ver[32];     // Boot版本号
    char app_ver[32];      // App版本号
    uint8_t ID[32];        // 芯片ID号,超过32字节时只取32字节
    uint32_t crc;          // 设备中固件校验码
    uint16_t block;        // 每次分包写入的数据大小,即设备中接收数据的buf大小，建议为 512*n(n为整数)
};

// 擦除设备
struct ZKHY_Frame_Emb_erase{
    enum Emb_Store_number MemNum;        // 存储区编号，见附录D
};
// 注:erase和volume可用于计算擦除进度。
struct ZKHY_Frame_Emb_erasea{
    enum Emb_Store_number MemNum;        // 存储区编号，见附录D
    uint32_t volume;       // 存储区容量，单位Byte
    uint32_t erase;        // 已擦除容量，单位Byte
    uint8_t status;        // 擦除状态：0成功、1擦除中、2存储区不支持、3参数错误、4其它错误
};

// 分包写入
struct ZKHY_Frame_Emb_write{
    enum Emb_Store_number MemNum;        // 存储区编号，见附录D
    uint32_t seek;         // 数据写入偏移
    uint32_t total;        // 数据写入总大小
    uint16_t block;        // 数据写入长度,不能超过同步指令获取到的block大小
    union{
        uint8_t data[1024];       // 写入的数据,block字节
        struct{
            uint32_t BaudRate;    // 波特率,范围:1000bps-460800bps
            uint8_t DataWidth;    // 数据宽度,取值: 8 8位, 9 9位
            uint8_t StopBits;     // 停止位,取值: 1 1位停止位, 2 2位停止位
            uint8_t Parity;       // 奇偶校验,取值：0 disabled, 1 Even Parity, 2 Odd Parity
        }uart;
        uint32_t key[8];          // 参数表
        //uint32_t param[128];      // 参数表
    };
};
// 注:erase和volume可用于计算擦除进度。
struct ZKHY_Frame_Emb_writea{
    enum Emb_Store_number MemNum;        // 存储区编号，见附录D
    uint32_t volume;       // 存储区容量，单位Byte
    uint32_t write;        // 已写入容量，单位Byte
    uint8_t status;        // 写入状态：0成功、1写入中、2存储区不支持、3参数错误、4其它错误
};

// 分包读取
struct ZKHY_Frame_Emb_read{
    enum Emb_Store_number MemNum;        // 存储区编号，见附录D
    uint32_t seek;         // 数据读取偏移
    uint16_t block;        // 数据读取长度,不能超过同步指令获取到的block大小
};
// 注:erase和volume可用于计算擦除进度。
struct ZKHY_Frame_Emb_reada{
    enum Emb_Store_number MemNum;        // 存储区编号，见附录D
    uint32_t volume;       // 存储区容量，单位Byte
    uint8_t status;        // 读取状态：0成功、1读取中、2存储区不支持、3参数错误、4其它错误
    uint32_t seek;         // 数据读取偏移
    uint16_t block;        // 数据实际读取长度
    union{
        uint8_t data[1024];    // 读取的数据,block字节
        uint32_t key[8];          // 参数表
        //uint32_t param[128];      // 参数表
    };
};

// 引导 APP
struct ZKHY_Frame_Emb_boot{
    uint32_t total;        // 固件大小
    uint32_t crc;          // 固件校验码
    uint8_t data[20];      // SHA1哈希,与CRC一样用于校验固件
};
// 注:erase和volume可用于计算擦除进度。
struct ZKHY_Frame_Emb_boota{
    uint8_t status;        // 状态：0成功、1固件错误、2校验失败、3其它错误
    char info[32];         // 有限的错误信息
};

// 复位设备
struct ZKHY_Frame_Emb_reboot{
    // 复位机器必须校验,C1=0xA5A5A5A5,C1=0x5A5A5A5A.
    uint32_t C1;
    uint32_t C2;
};
struct ZKHY_Frame_Emb_reboota{
    uint8_t status;        // 状态：0成功、1不支持复位、2其它错误
    char info[32];         // 有限的错误信息
};

struct ZKHY_Frame_upload{
    //                    //  定义	数据类型	字节长度	描述及要求
    uint8_t STR[2];       //  起始符	String	2	十六进制 "0x5A 0x20"表示
    uint8_t CMD;          //  命令单元	Byte	1	定义数据单元的功能。详见表 2-32
    uint8_t CNT;          //  流水号	Byte	1	设备每发一包流水号自动加1
    uint8_t AVN;          //  协议版本号	Byte	1	随协议内容更新而更新范围0~255
    uint16_t LEN;         //  数据单元长度	Word	2	数据单元的总字节数，范围 0~65535。
    union {               //  数据单元	Byte[n]	n	--
        uint8_t buf[1024];
        struct ZKHY_Frame_login login;
        struct ZKHY_Frame_login_ack login_ack;
        char sn[32];      // VIN请求包,32位序列号，不足时用0x00补足
        char VIN[17];     // VIN请求响应包,车辆识别码是识别的唯一标识，由17位字码组成，字符符合GB16735中4.5的规定
        char UTC[6];      // 校时请求响应包,采用GMT+8时间，时间定义符合GB/T32960.3-2016第6.4条的要求
        struct ZKHY_Frame_download download;
        struct ZKHY_Frame_download_ack download_ack;
        // 分包查询请求
        struct ZKHY_Frame_upload_req upload_req;
        // 客户分包请求
        struct ZKHY_Frame_upload_req_client upload_reqc;
        // 请求响应
        struct ZKHY_Frame_upload_ack upload_ack;
        // 块请求响应
        struct ZKHY_Frame_upload_ackb upload_ackb;
        /******************* 嵌入式升级 ********************/
        struct ZKHY_Frame_Emb_sync      Emb_sync;
        struct ZKHY_Frame_Emb_synca     Emb_synca;
        struct ZKHY_Frame_Emb_erase     Emb_erase;
        struct ZKHY_Frame_Emb_erasea    Emb_erasea;
        struct ZKHY_Frame_Emb_write     Emb_write;
        struct ZKHY_Frame_Emb_writea    Emb_writea;
        struct ZKHY_Frame_Emb_read      Emb_read;
        struct ZKHY_Frame_Emb_reada     Emb_reada;
        struct ZKHY_Frame_Emb_boot      Emb_boot;
        struct ZKHY_Frame_Emb_boota     Emb_boota;
        struct ZKHY_Frame_Emb_reboot    Emb_reboot;
        struct ZKHY_Frame_Emb_reboota   Emb_reboota;
    }DAT;
    uint8_t BCC;          // 校验码	Byte	1	BCC 异或校验。校验范围从命令单元到数据单元的最后一个字节。
};

union upload_Emb_Arg{
    //char sn[32]; // 烧号
    char param[256];      // 参数表
    struct{      // 串口配置参数
        uint32_t BaudRate;    // 波特率,范围:1000bps-460800bps
        uint8_t DataWidth;    // 数据宽度,取值: 8 8位, 9 9位
        uint8_t StopBits;     // 停止位,取值: 1 1位停止位, 2 2位停止位
        uint8_t Parity;       // 奇偶校验,取值：0 disabled, 1 Even Parity, 2 Odd Parity
    }uart;
};

// 编码一帧数据
extern int ZKHY_EnFrame_upload(const struct ZKHY_Frame_upload* const _frame, uint8_t buf[], const uint16_t _size);
// 解码一帧数据
extern int ZKHY_DeFrame_upload(struct ZKHY_Frame_upload* const _frame, const uint8_t data[], const uint16_t _dsize);
extern int ZKHY_check_frame(const void* const _data, const uint16_t _dsize);
// 数据帧初始化,上行
static inline int ZKHY_frame_upload_init(struct ZKHY_Frame_upload* const _frame, const enum ZKHY_cmd_Upload cmd)
{
    memset(_frame, 0, sizeof(struct ZKHY_Frame_upload));
    _frame->STR[0] = ZKHY_UPLOAD_FRAME_STR0;
    _frame->STR[1] = ZKHY_UPLOAD_FRAME_STR1;
    _frame->CMD = cmd;
    //_frame->AVN = 0;
    _frame->AVN = 0x01;    // 协议版本号
    return 0;
}

extern void ZKHY_Slave_upload_init(void);
extern int ZKHY_Dev_Frame_upload(struct ZKHY_Frame_upload* const _frame, const enum ZKHY_cmd_Upload cmd, uint8_t _buf[], const uint16_t _bsize);
// _sub_cmd:读写命令有子命令
extern int ZKHY_Dev_Frame_upload_Emb(struct ZKHY_Frame_upload* const _frame, const enum ZKHY_cmd_Upload cmd, const enum Emb_Store_number _sub_cmd, const union upload_Emb_Arg* const Arg, uint8_t _buf[], const uint16_t _bsize);
extern int ZKHY_Dev_unFrame_upload(struct ZKHY_Frame_upload* const _frame, const enum ZKHY_cmd_Upload cmd, const  uint8_t data[], const uint16_t _dsize);
// 嵌入式升级
extern int ZKHY_Dev_unFrame_upload_Emb(struct ZKHY_Frame_upload* const _frame, const enum ZKHY_cmd_Upload cmd, const  uint8_t data[], const uint16_t _dsize);
extern int ZKHY_Slave_Frame_upload(struct ZKHY_Frame_upload* const _frame, const enum ZKHY_cmd_Upload cmd, uint8_t _buf[], const uint16_t _bsize);
extern int ZKHY_Slave_unFrame_upload(struct ZKHY_Frame_upload* const _frame, const  uint8_t data[], const uint16_t _dsize, int (*const send_func)(const uint8_t data[], const uint32_t _size));

// 编解码测试
extern void __ZKHY_frame_upload_test(const enum ZKHY_cmd_Upload cmd);
extern void ZKHY_frame_upload_test(void);
// 获取命令的文本信息
extern const char* ZKHY_Cmd_Info_upload(const enum ZKHY_cmd_Upload cmd);

#ifdef __cplusplus
}
#endif

#endif // _ZKHY_DEV_UPLOAD_H_
