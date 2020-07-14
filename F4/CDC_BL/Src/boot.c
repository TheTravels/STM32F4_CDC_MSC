/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//#include "usbd_cdc.h"
#include "fatfs.h"
#include "ff.h"
#include "sha1/sha1.h"
#include "Periphs/uart.h"
#include "Periphs/Flash.h"
#include "Periphs/crc.h"
#include "Periphs/ParamTable.h"
#include "version.h"
#include "GB/ZKHY/ZKHY_Dev_upload.h"
#include "core_cm4.h"
#include "Ini/Ini.h"
#include "Ini/Files.h"
#include "tea/tea.h"
#include "driver/ec20.h"
struct ZKHY_Frame_upload Frame_upload;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
static const char Param_name[] = "Param";
static const char Param_key_sn[] = "SN";
//static const char Param_key_host[] = "Host";
//static const char Param_key_port[] = "Port";
static const char Param_key_ftph[] = "FTPH";
static const char Param_key_ftpp[] = "FTPP";
static const char Param_key_user[] = "user";
static const char Param_key_passwd[] = "pass";
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
extern USBD_StatusTypeDef USBD_DeInit(USBD_HandleTypeDef *pdev);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static const char fw_name[] = "FW";
static const char fw_key_name[] = "Name";
static const char fw_key_total[] = "total";
static const char fw_key_crc[] = "CRC";
//static const char fw_key_sha1[] = "SHA1";
//static const char fw_key_time[] = "Time";
enum Interface_uart inter_uart=INTER_UART_NONE;
extern uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
extern USBD_HandleTypeDef hUsbDeviceFS;
//unsigned char cdc_rx_buffer[1024];
//int cdc_rx_flag = 0;
extern int rx_buf_get(void);
extern void fs_test(void);
extern void fs_test_sdio(void);

uint8_t vbus_high_count=0;
uint8_t vbus_low_count=0;
uint8_t vbus_connect=0;
uint32_t led_tick = 0;
extern void bl_entry(void);

void vbus_poll(const uint32_t _tick)
{
	uint32_t gpio = LL_GPIO_ReadInputPort(VBUS_GPIO_Port);
	if(gpio&VBUS_Pin)
	{
		vbus_high_count++;
		if(vbus_high_count>200) vbus_high_count = 200;
		vbus_low_count=0;
	}
	else
	{
		vbus_high_count=0;
		vbus_low_count++;
		if(vbus_low_count>200) vbus_low_count = 200;
	}
	if((led_tick>0) && (led_tick<=_tick))
	{
		led_tick = _tick + 100;
		LL_GPIO_TogglePin(GPIOD, LED_Pin);
	}
}

//uint8_t SD_GetCardInfo(HAL_SD_CardInfoTypeDef *cardinfo)
//{
//    uint8_t sta;
//    sta=HAL_SD_GetCardInfo(&hsd,cardinfo);
//    return sta;
//}

//static uint8_t _ccm __attribute__ ((aligned (4))) bl_data[1024*4];
static uint8_t __attribute__ ((aligned (4))) bl_data[1024*4];
//extern void HAL_SD_MspDeInit(SD_HandleTypeDef* sdHandle);
extern void boot_app(void);
void Periphs_DeInit(void)
{
	//USBD_DeInit(&hUsbDeviceFS);
	NVIC_EnableIRQ(OTG_HS_EP1_IN_IRQn);
	NVIC_EnableIRQ(OTG_HS_EP1_OUT_IRQn);
	NVIC_EnableIRQ(OTG_HS_WKUP_IRQn);
	NVIC_EnableIRQ(OTG_HS_IRQn);
	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_OTGHS);
	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
	SysTick->CTRL &= (~SysTick_CTRL_ENABLE_Msk);
    // EC20模块断电
    LL_GPIO_ResetOutputPin(PWR_EN_4G_GPIO_Port, PWR_EN_4G_Pin);
	LL_USART_DeInit(USART1);
	LL_USART_DeInit(USART2);
	LL_USART_DeInit(USART3);
	//HAL_SD_MspDeInit(&hsd);
	HAL_GPIO_DeInit(SD_NCD_GPIO_Port, SD_NCD_Pin);
	HAL_GPIO_DeInit(LED_GPIO_Port, LED_Pin);
	HAL_GPIO_DeInit(VBUS_GPIO_Port, VBUS_Pin);
	HAL_GPIO_DeInit(PWR_EN_GPS_GPIO_Port, PWR_EN_GPS_Pin);
	HAL_GPIO_DeInit(PWR_EN_4G_GPIO_Port, PWR_EN_4G_Pin);
	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOH);
	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOD);
}

int check_bl(uint8_t _buf[], const uint16_t _bsize, int (*const read_func)(uint8_t buf[], const uint32_t _size))
{
	int count;
	int index;
	int len;
	if(NULL==read_func) return -1;
	memset(_buf, 0, _bsize);
    len = read_func(_buf, 1);
    if(len>0)
    {
    	HAL_Delay(50);
    	len += read_func(&_buf[len], _bsize-len);
        // 检索连续的 0x7F
    	count = 0;
        for(index=0; index<len; index++)
        {
        	if(0x7F==_buf[index]) count++;
        	else count = 0;
        }
        if(count>32) return 0;
    }
    return -2;
}

void msc_upload(void)
{
	  char Name[64];
	  uint32_t total;
	  uint32_t crc16;
	  FATFS fs;           /* Filesystem object */
	  FRESULT res;  /* API result code */
	  struct Ini_Parse Ini = {
			  "0:/fw.Ini",
			  .text = (char *)bl_data,
			  ._bsize = sizeof(bl_data),
			  .pos = 0,
			  ._dsize = 0,
	  };
	    uint32_t checksum = 0x12345678;
	  //USBD_DeInit(&hUsbDeviceFS);
	  /* 挂载文件系统 */
	  res = f_mount(&fs, "0:", 0);
	  if(FR_OK==res)
	  {
		  //app_debug("mount OK.\r\n");
		  // 检测升级
		  if(0==Ini_load(&Ini))  // 检测到升级配置文件
		  {
			  uint32_t _crc16 = 0;
			  _crc16 = 0;
			  Ini_get_field(&Ini, fw_name, fw_key_name, "-", Name);
			  app_debug("[%s-%d] Name[%s] \r\n", __func__, __LINE__, Name);
			  total = Ini_get_int(&Ini, fw_name, fw_key_total, 0);
			  crc16 = Ini_get_int(&Ini, fw_name, fw_key_crc, 0);
#ifdef FAST_CRC16
			  _crc16 = fast_crc16(_crc16, (const unsigned char*)(param_flash_start), total);
#else
			  _crc16 = fw_crc(_crc16, (const unsigned char*)(param_flash_start), total);
#endif
			  checksum = _crc16;
			  app_debug("[%s-%d] total[0x%08X] crc16[0x%08X] checksum[0x%08X] \r\n", __func__, __LINE__, total, crc16, checksum);
			  if((checksum!=crc16) && (total>4096) && ('-'!=Name[0])) // upload
			  {
				  uint32_t seek;
				  uint32_t _size;
				  //long rlen;
				  char _path[64] = "0:/fw.Ini";
				  memset(&_path[3], 0x00, sizeof(_path)-3);
				  memcpy(&_path[3], Name, strlen(Name));
				  led_tick = 0;
				  // 刷入固件前有 3s 快闪提示
				  for(seek=0; seek<60; seek++)
				  {
					  HAL_Delay(50);
					  LL_GPIO_TogglePin(GPIOD, LED_Pin);
				  }
				  led_tick = 100;
				  param_write_erase();
				  // program
				  for(seek=0; seek<total; seek+=512)
				  {
					  _size = total - seek;
					  if(_size>512) _size = 512;
					  memset(bl_data, 0xFF, sizeof(bl_data));
					  /*rlen = */file_read_seek(_path, seek, bl_data, _size);
					  //app_debug("[%s-%d] read seek[0x%08X] rlen[0x%08X] \r\n", __func__, __LINE__, total, seek, rlen);
					  param_write_flash(bl_data, seek, _size);
				  }
			  }
		  }
	  }
	  //else app_debug("mount fail.\r\n");
	  /*卸载文件系统*/
	  f_mount(0, "0:", 0);
}

struct from_app_upload_data{
	uint32_t flag;
	uint32_t total;
	uint32_t crc16;
	uint32_t recv[16];
};
// 从 App 升级标志
#define  FROM_APP_FLAG     0x12345678

#if 0  // 严格的校验会给 调试带来麻烦
// 来自固件中的更新
void from_app_upload(void)
{
	uint32_t total;
	uint32_t crc16;
	uint32_t checksum = 0x12345678;
	const char* const flash_download = (const char*)param_download_start;
	// 使用 Ini 格式保存固件信息
	struct Ini_Parse Ini = {
			"0:/fw.Ini",
			.text = (char *)bl_data,
			._bsize = 1024,//sizeof(bl_data),
			.pos = 0,
			._dsize = 0,
	};
	memcpy(bl_data, flash_download, 1024);
	Ini._dsize = strlen(Ini.text);
	if(Ini._dsize>1024) Ini._dsize = 1024;
	// 检测升级
	total = Ini_get_int(&Ini, fw_name, fw_key_total, 0);
	crc16 = Ini_get_int(&Ini, fw_name, fw_key_crc, 0);
	checksum = fw_crc(0, (const unsigned char*)(param_flash_start), total);
	app_debug("[%s-%d] total[0x%08X] crc16[0x%08X] checksum[0x%08X] \r\n", __func__, __LINE__, total, crc16, checksum);
	if((checksum!=crc16) && (total>4096)) // upload
	{
		uint32_t seek;
		uint32_t _size;
		led_tick = 0;
		// 刷入固件前有 3s 快闪提示
		for(seek=0; seek<60; seek++)
		{
			HAL_Delay(50);
			LL_GPIO_TogglePin(GPIOD, LED_Pin);
		}
		led_tick = 100;
		param_write_erase();
		// program,从下载扇区拷贝到 app 扇区
		for(seek=0; seek<total; seek+=1024)
		{
			_size = total - seek;
			if(_size>1024) _size = 1024;
			memset(bl_data, 0xFF, sizeof(bl_data));
			// 前 1024 为固件参数区
			memcpy(bl_data, flash_download+1024+seek, _size);
			//app_debug("[%s-%d] read seek[0x%08X] rlen[0x%08X] \r\n", __func__, __LINE__, total, seek, rlen);
			param_write_flash(bl_data, 1024+seek, _size);
		}
		// 写入固件信息
		memcpy(bl_data, flash_download, 1024);
		param_write_flash(bl_data, 0, 1024);
	}
}
#else
// 来自固件中的更新
void from_app_upload(void)
{
	uint32_t total;
	uint32_t crc16;
	uint32_t checksum = 0x12345678;
	const char* const flash_download = (const char*)param_download_start;
	const struct from_app_upload_data* const app_data = (const struct from_app_upload_data*)param_download_start;
	// 检测升级
	total = app_data->total;
	crc16 = app_data->crc16;
	// 参数检查
	if((FROM_APP_FLAG!=app_data->flag) || (total>param_flash_size) || (total<4096) || (0==crc16)) return;
	checksum = fw_crc(0, (const unsigned char*)(param_flash_start), total);
	app_debug("[%s-%d] total[0x%08X] crc16[0x%08X] checksum[0x%08X] \r\n", __func__, __LINE__, total, crc16, checksum);
	if((checksum!=crc16) && (total>4096)) // upload
	{
		uint32_t seek;
		uint32_t _size;
		led_tick = 0;
		// 刷入固件前有 3s 快闪提示
		for(seek=0; seek<60; seek++)
		{
			HAL_Delay(50);
			LL_GPIO_TogglePin(GPIOD, LED_Pin);
		}
		led_tick = 100;
		param_write_erase();
		// program,从下载扇区拷贝到 app 扇区
		for(seek=0; seek<total; seek+=1024)
		{
			_size = total - seek;
			if(_size>1024) _size = 1024;
			memset(bl_data, 0xFF, sizeof(bl_data));
			// 前 1024 为固件参数区, 即 Ini 数据
			memcpy(bl_data, flash_download+1024+seek, _size);
			//app_debug("[%s-%d] read seek[0x%08X] rlen[0x%08X] \r\n", __func__, __LINE__, total, seek, rlen);
			param_write_flash(bl_data, seek, _size);
		}
	}
}
#endif

// 对芯片签名, 签名长度 32B
extern uint8_t read_uid(uint8_t uid[]);
static uint8_t send_buf[256];
uint32_t sign_flag = 0;
static inline uint32_t sign_chip(uint32_t sign[8])
{
	// 对齐
	uint32_t uid[8];
	uint16_t count, i;
	uint32_t crc16 = 0;
	uint32_t crc;
	uint32_t addr = (uint32_t)&bl_entry;
	const uint32_t* flash = (const uint32_t*)0x08000400;
	const uint32_t key[4]={0x89480304, 0x60670708, 0x090A0B0C, 0x68270F10};
	const uint16_t emb_iteration = 64;

	addr = addr-(addr&0x03);  // 对齐
	flash = (const uint32_t*)(addr+((flash[0]&0x7F)<<2));
	//memset(uid, 0x5A, sizeof(uid));   // 填默认值
	memcpy(uid, flash, sizeof(uid));  // 将代码作为随机数使用
	// 读取 ID
	read_uid((uint8_t*)uid);  // 96 bit
	// 使用 TEA 加密, 迭代处理,将对称加密变成不可解密加密算法
	for(count=0; count< 16; count++)
	{
		memcpy(&sign[0], key, sizeof(key));
		memcpy(&sign[4], key, sizeof(key));
		tea_encrypt(sign, 4*8, uid, emb_iteration);
		// 叠加,将 sign 和 key 带入 uid,从而让数据不可解密
		for(i=0; i<8; i++) uid[i] = uid[i] + ((uid[i]>>i)&0xFFFF) + ((sign[i]<<i)&0xFFFF0000);
		tea_encrypt(uid, sizeof(uid), key, emb_iteration);
		sign_flag++;
	}
	// 计算 CRC
	crc16 = 0;
#ifdef FAST_CRC16
	crc16 = fast_crc16(crc16, (const unsigned char*)(0x08000000+0x0400), 1024*32); // 32K 代码校验
#else
	crc16 = fw_crc(crc16, (const unsigned char*)(0x08000000+0x0400), 1024*32); // 32K 代码校验
#endif
	crc = crc16;
	memcpy(&sign[0], uid, sizeof(uid));
	MX_USART3_UART_Init();
	/*app_debug("[%s--%d] crc:0x%08X sign_flag:%d \r\n", __func__, __LINE__, crc, sign_flag);
	app_debug("[%s--%d] sign: \r\n", __func__, __LINE__);
	for(i=0; i<8; i++) app_debug("0x%08X \r\n", sign[i]);
	app_debug("\r\n");*/
	return crc;
}
/****************************************************************
* Function:    Flash_EnableReadProtection
* Description: Enable the read protection of user flash area.
* Input:        NONE
* Output:        NONE
* Return:  NONE
*****************************************************************/

void Flash_EnableReadProtection(void)
{

	FLASH_OBProgramInitTypeDef OBInit;

	__HAL_FLASH_PREFETCH_BUFFER_DISABLE();

	HAL_FLASHEx_OBGetConfig(&OBInit);
	if(OBInit.RDPLevel == OB_RDP_LEVEL_0)
	{
		OBInit.OptionType = OPTIONBYTE_RDP;
		OBInit.RDPLevel = OB_RDP_LEVEL_1;
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		HAL_FLASHEx_OBProgram(&OBInit);
		HAL_FLASH_OB_Launch();
		HAL_FLASH_OB_Lock();
		HAL_FLASH_Lock();
	}
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();

}
void Flash_EnableWriteProtection(void)
{

	FLASH_OBProgramInitTypeDef OBInit;

	__HAL_FLASH_PREFETCH_BUFFER_DISABLE();

	HAL_FLASHEx_OBGetConfig(&OBInit);
	app_debug("[%s--%d] WRPSector[0x%08X]\r\n", __func__, __LINE__, OBInit.WRPSector);
	// 锁住 boot 扇区, 0是保护
	if(0x00 != (OBInit.WRPSector&0x07))
	{
		OBInit.OptionType = OPTIONBYTE_WRP;
		OBInit.WRPState = OB_WRPSTATE_ENABLE;
		OBInit.WRPSector = 0x07;
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		HAL_FLASHEx_OBProgram(&OBInit);
		HAL_FLASH_OB_Launch();
		HAL_FLASH_OB_Lock();
		HAL_FLASH_Lock();
	}
	app_debug("[%s--%d] WRPSector[0x%08X]\r\n", __func__, __LINE__, OBInit.WRPSector);
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();

}

/****************************************************************
* Function:    Flash_DisableReadProtection
* Description: Disable the read protection of user flash area.
* Input:        NONE
* Output:        NONE
* Return:  NONE
*****************************************************************/
void Flash_DisableReadProtection(void)
{

  FLASH_OBProgramInitTypeDef OBInit;

  __HAL_FLASH_PREFETCH_BUFFER_DISABLE();

  HAL_FLASHEx_OBGetConfig(&OBInit);
  if(OBInit.RDPLevel != OB_RDP_LEVEL_0)
  {
    OBInit.OptionType = OPTIONBYTE_RDP;
    OBInit.RDPLevel = OB_RDP_LEVEL_0;
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    HAL_FLASHEx_OBProgram(&OBInit);
    HAL_FLASH_OB_Launch();
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
  }
  __HAL_FLASH_PREFETCH_BUFFER_ENABLE();

}
void Flash_DisableWriteProtection(void)
{

	FLASH_OBProgramInitTypeDef OBInit;

	__HAL_FLASH_PREFETCH_BUFFER_DISABLE();

	HAL_FLASHEx_OBGetConfig(&OBInit);
	app_debug("[%s--%d] WRPSector[0x%08X]\r\n", __func__, __LINE__, OBInit.WRPSector);
	// 解锁
	if(0x7FF != (OBInit.WRPSector&0x7FF))
	{
		OBInit.OptionType = OPTIONBYTE_WRP;
		OBInit.WRPState = OB_WRPSTATE_DISABLE;
		OBInit.WRPSector = 0x7FF;
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		HAL_FLASHEx_OBProgram(&OBInit);
		HAL_FLASH_OB_Launch();
		HAL_FLASH_OB_Lock();
		HAL_FLASH_Lock();
	}
	app_debug("[%s--%d] WRPSector[0x%08X]\r\n", __func__, __LINE__, OBInit.WRPSector);
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();

}

// addr必须是32位对齐的, wlen:32位宽度数据长度
static void erase_chip(const uint32_t addr, const uint8_t wlen)
{
	uint32_t data[32];
	uint8_t len;
	//FLASH_OBProgramInitTypeDef pOBInit;
	// 通过写 0 的方式擦除数据,这是 flash 的特性,数据写入是将 1 编程为 0 的过程
	len = wlen;
	if(len>32) len = 32;
	memset(data, 0x00, sizeof(data));
	Flash_Write_Force(addr, data, len);
	HAL_Delay(100);
	//app_debug("[%s--%d] bl_entry\r\n", __func__, __LINE__);
#if 0
	// 设置 BOR 复位级别
	pOBInit.OptionType = OPTIONBYTE_BOR;
	pOBInit.BORLevel = OB_BOR_LEVEL1;
	HAL_FLASHEx_OBProgram(&pOBInit);
#endif
#if 0
	// 设置读保护级别 1,级别 2不可恢复尽量不要设置
	Flash_EnableReadProtection();
	Flash_EnableWriteProtection();
#endif
	// jump to app
	bl_entry();
	NVIC_SystemReset();
	while(1)
	{
		asm("mov r0,r0");
	}
}
// 签名
void __attribute__((unused, section(".sign_chip"))) first_sign_chip(void)
{
	uint32_t sign[8];
	uint32_t crc;
	uint32_t addr = (uint32_t)&first_sign_chip;
	addr = addr-(addr&0x03);  // 对齐
	// 延时加密,烧录器会设置校验,若代码执行加密更改了芯片中的数据将导致代码校验失败,
	// 故此处延时为延缓加密进程,已让烧录器正常校验
	HAL_Delay(500);
	// 签名
	crc = sign_chip(sign);
	// 写入 CRC
	Flash_Write_Force((const uint32_t)&Emb_Version.crc, &crc, 1);
	// 写入签名
	Flash_Write_Force((const uint32_t)&Emb_Version.signData, sign, 8);
	// 擦除 first_sign_chip 函数, 80 为 first_sign_chip 函数的大小
	erase_chip(addr, 70/4);
}
// 验签代码
void verify_chip(void)
{
	uint32_t sign[8];
	uint32_t crc;
	int led;
	// 签名
	crc = sign_chip(sign);
	//app_debug("[%s--%d] uid: \r\n", __func__, __LINE__);
	if((16==sign_flag) && (Emb_Version.crc==crc) && (0==memcmp(sign, Emb_Version.signData, sizeof(Emb_Version.signData))))
	{
		//app_debug("[%s--%d] uid: \r\n", __func__, __LINE__);
		bl_entry();
	}
	//app_debug("[%s--%d] uid: \r\n", __func__, __LINE__);
	// 加密验证错误, 错误提示:3s快闪,3s慢闪
	led_tick = 0;
	MX_GPIO_Init();
	while(1)  // nop
	{
		// 使用";"会出现 warning: this 'for' clause does not guard... [-Wmisleading-indentation]
		//asm("mov r0,r0");
		// 刷入固件前有 3s 快闪提示
		for(led=0; led<60; led++) // 3s
		{
			HAL_Delay(50);
			LL_GPIO_TogglePin(GPIOD, LED_Pin);
		}
		for(led=0; led<6; led++)  // 3s
		{
			HAL_Delay(500);
			LL_GPIO_TogglePin(GPIOD, LED_Pin);
		}
	}
}

// 真正的入口
void bl_entry(void)
{
	int bl_len;
	//uint32_t signApp[8];
	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	//MX_USB_DEVICE_Init();
	//MX_SDIO_SD_Init();
	//MX_USART1_UART_Init();
	//MX_USART2_UART_Init();
	//MX_USART3_UART_Init();
	MX_FATFS_Init();
	/* USER CODE BEGIN 2 */
	USART1_Init(115200);
	USART2_Init(115200);
	USART3_Init(115200);
	//MX_FATFS_Init();
	//SD_initialize(0);
	//fs_test();
	//fs_test_sdio();
	//MX_USB_DEVICE_Init();
	//SHA1(NULL, "Hello", 5); // -Os Optimize code, add code 4K
	LL_GPIO_ResetOutputPin(GPIOD, LED_Pin);
	//fs_test(); // 格式化 Flash
	memset(send_buf, 0, sizeof(send_buf));
	// 利用 flash的写 0特点擦除原有数据
	//len = Flash_Write_Force(0x08000200, (uint32_t *)send_buf, 8);
	//app_debug("[%s--%d] len:%d \r\n", __func__, __LINE__, len);
	led_tick = HAL_GetTick() + 200;
	//HAL_Delay(200);  // delay, check VBUS
	//app_debug("[%s--%d] system start!\r\n", __func__, __LINE__);
	app_debug("[%s--%d] Ver[%d | 0x%08X]:%s\r\n", __func__, __LINE__, sizeof(Emb_Version), &Emb_Version, Emb_Version.version);
	//param_read_key(signApp);
	//for(int i=0; i<8; i++) app_debug("[%s--%d] signApp[%d]:0x%08X \r\n", __func__, __LINE__, i, signApp[i]);
#if 1  // 未避免读保护设置失败,在这里每次都检查设置
	// 设置读保护级别 1,级别 2不可恢复尽量不要设置
	Flash_EnableReadProtection();
#endif
	// 检测是否需要升级
	for(bl_len=0; bl_len<100; bl_len++)
	{
		HAL_Delay(10);
		if((0==vbus_connect) && (vbus_high_count>100)) // usb connect
		{
			bl_len=0;
			break;
		}
#if 0 // 暂时不支持
		if(0==check_bl(send_buf, sizeof(send_buf), uart1_read))
		{
			bl_len=0;
			break;
		}
		if(0==check_bl(send_buf, sizeof(send_buf), uart2_read))
		{
			bl_len=0;
			break;
		}
		if(0==check_bl(send_buf, sizeof(send_buf), uart3_read))
		{
			bl_len=0;
			break;
		}
#endif
	}
	//app_debug("[%s--%d] bl_len:%d \r\n", __func__, __LINE__, bl_len);
	if(0!=bl_len)
	{
		// 未质检设备不连 FTP升级,即生产中的设备不升级
		if(1==ParamTable_quality())
		{
			int port;
			char param_buf[512];
			char sn[32];
			char ftp[32];
			char user[32];
			char passwd[32];
		    struct Ini_Parse Ini = {
		        "fw.Ini",
		        .text = param_buf,
		         ._bsize = sizeof(param_buf),
		         .pos = 0,
		         ._dsize = 0,
		    };
		    ParamTable_Read(param_buf, 0, sizeof(param_buf));
		    Ini._dsize = strlen(Ini.text);
		    // 解析参数
		    memset(sn, 0, sizeof(sn));
		    Ini_get_field(&Ini, Param_name, Param_key_sn, "-", sn);
		    Ini_get_field(&Ini, Param_name, Param_key_ftph, "39.108.51.99", ftp);
		    Ini_get_field(&Ini, Param_name, Param_key_user, "obd4g", user);
		    Ini_get_field(&Ini, Param_name, Param_key_passwd, "obd.4g", passwd);
		    port = Ini_get_int(&Ini, Param_name, Param_key_ftpp, 21);
		    app_debug("[%s--%d] SN:%s\r\n", __func__, __LINE__, sn);
		    //app_debug("[%s--%d] ftp:%s port:%d user:%s passwd:%s \r\n", __func__, __LINE__, ftp, port, user, passwd);
		    app_debug("[%s--%d] ftp:%s port:%d \r\n", __func__, __LINE__, ftp, port);
			//EC20_FTP_Upload(Emb_Version.hardware, "0A0CK90N4123", "39.108.51.99", 21, "obd4g", "obd.4g");
		    // 序列号必须有效
		    if('-'!=sn[0]) EC20_FTP_Upload(Emb_Version.hardware, sn, ftp, port, user, passwd);
		}
		app_debug("[%s--%d] boot_app!\r\n", __func__, __LINE__);
		// 检查 app 下载的新固件,有则刷入 app 存储区
		from_app_upload();
		boot_app();
	}
	//fs_test(); // 格式化 Flash
	//  ret = FLASH_Erase(0x08020000, 0x08030000);
	//  app_debug("[%s--%d] FLASH_Erase[%d]\r\n", __func__, __LINE__, ret);
	//  ret = Flash_Write(0x08010000, data, 3);
	//  app_debug("[%s--%d] FLASH_write<%d>[%08X %08X %08X]\r\n", __func__, __LINE__, ret, data[0], data[1], data[2]);
	//  memset(data, 0, sizeof(data));
	//  ret = Flash_Read(0x08010000, data, 3);
	//  app_debug("[%s--%d] FLASH_read<%d>[%08X %08X %08X]\r\n", __func__, __LINE__, ret, data[0], data[1], data[2]);
	//Flash_Test(0x08010000, 0x08020000);
	//flash_disk_init();
	//sram_disk_init();
	//msc_upload();
	ZKHY_Slave_upload_init(NULL);
#if 0
	//SD_GetCardInfo(&cardinfo);
	BSP_SD_GetCardInfo(&cardinfo);
	app_debug("[%s--%d] Specifies the card Type :%d \r\n", __func__, __LINE__, cardinfo.CardType);
	app_debug("[%s--%d] Specifies the card version :%d \r\n", __func__, __LINE__, cardinfo.CardVersion);
	app_debug("[%s--%d] Specifies the class of the card class :%d \r\n", __func__, __LINE__, cardinfo.Class);
	app_debug("[%s--%d] Specifies the Relative Card Address :%d \r\n", __func__, __LINE__, cardinfo.RelCardAdd);
	app_debug("[%s--%d] Specifies the Card Capacity in blocks :%d | %d | %d \r\n", __func__, __LINE__, cardinfo.BlockNbr, cardinfo.BlockNbr*cardinfo.BlockSize, cardinfo.BlockNbr*cardinfo.BlockSize/1024/1024);
	app_debug("[%s--%d] Specifies one block size in bytes :%d \r\n", __func__, __LINE__, cardinfo.BlockSize);
	app_debug("[%s--%d] Specifies the Card logical Capacity in blocks :%d \r\n", __func__, __LINE__, cardinfo.LogBlockNbr);
	app_debug("[%s--%d] Specifies logical block size in bytes  :%d \r\n", __func__, __LINE__, cardinfo.LogBlockSize);
#endif
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		if((0==vbus_connect) && (vbus_high_count>100)) // usb connect
		{
			vbus_connect = 1;
			MX_USB_DEVICE_Init();
		}
		if((1==vbus_connect) && (vbus_low_count>100)) // usb disconnect
		{
			vbus_connect = 0;
			USBD_DeInit(&hUsbDeviceFS);
			// 检测升级
			//msc_upload();
			boot_app();
		}
		//      switch(inter_uart)
		//      {
		//      case 1:
		//    	  break;
		//      case 2:
		//    	  break;
		//      case 3:
		//    	  break;
		//      case 4:
		//    	  break;
		//      default:
		//    	  break;
		//      }
		memset(bl_data, 0, sizeof(bl_data));
		bl_len = cdc_read(bl_data, 1);
		if(bl_len>0)
		{
			int resp;
			int i;
			HAL_Delay(50); //
			bl_len += cdc_read(&bl_data[bl_len], sizeof(bl_data)-bl_len);
			for(i=0; i<5; i++)
			{
				app_debug("[%s--%d] bl_len :%d \r\n", __func__, __LINE__, bl_len);
				resp = ZKHY_Slave_unFrame_upload(&Frame_upload, bl_data, bl_len, cdc_send);
				if(ZKHY_RESP_ERR_PACK==resp) // 解包错误
				{
					HAL_Delay(20); //
					bl_len += cdc_read(&bl_data[bl_len], sizeof(bl_data)-bl_len);
					continue;
				}
				break;
			}
		}
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
}

/* USER CODE END 0 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
