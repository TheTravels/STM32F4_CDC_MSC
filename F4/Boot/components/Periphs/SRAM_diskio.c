/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    SRAM_diskio.c
  * @brief   SD Disk I/O driver
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

/* Note: code generation based on SRAM_diskio_template_bspv1.c v2.1.4
   as "Use dma template" is disabled. */

/* USER CODE BEGIN firstSection */
/* can be used to modify / undefine following code or add new definitions */
/* USER CODE END firstSection*/

/* Includes ------------------------------------------------------------------*/
#include "ff_gen_drv.h"
#include <string.h>
#include "SRAM_diskio.h"
#include "main.h"
#include "Flash.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* use the default SD timout as defined in the platform BSP driver*/
#if defined(SDMMC_DATATIMEOUT)
#define SRAM_TIMEOUT SDMMC_DATATIMEOUT
#elif defined(SRAM_DATATIMEOUT)
#define SRAM_TIMEOUT SRAM_DATATIMEOUT
#else
#define SRAM_TIMEOUT 30 * 1000
#endif


//#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) 	// 128 Kbytes
//#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) 	// 128 Kbytes

#define FLASH_SECTOR_ADDR_MAP     ((uint32_t)0x08040000) 	// 128 Kbytes
//#undef  FLASH_SECTOR_ADDR_MAP

#define SRAM_DEFAULT_BLOCK_SIZE 512

/*
 * Depending on the use case, the SD card initialization could be done at the
 * application level: if it is the case define the flag below to disable
 * the BSP_SRAM_Init() call in the SRAM_Initialize() and add a call to
 * BSP_SRAM_Init() elsewhere in the application.
 */
/* USER CODE BEGIN disableSDInit */
/* #define DISABLE_SRAM_INIT */

#if 0
static char sram_disk[1024*96];
const uint32_t sram_disk_size = sizeof(sram_disk);
void sram_disk_init(void)
{
  /* USER CODE BEGIN 6 */
  memset(sram_disk, 0, sram_disk_size);
  /* USER CODE END 6 */
}
uint16_t sram_disk_read(uint8_t *const buf, const uint32_t _addr, const uint16_t _len)
{
  /* USER CODE BEGIN 6 */
  memcpy(buf, &sram_disk[_addr], _len);
  return _len;
  /* USER CODE END 6 */
}
uint16_t sram_disk_write(const uint8_t *const buf, const uint32_t _addr, const uint16_t _len)
{
  /* USER CODE BEGIN 7 */
  memcpy(&sram_disk[_addr], buf, _len);
  return _len;
  /* USER CODE END 7 */
}
#else

#ifndef FLASH_SECTOR_ADDR_MAP
static char _ccm ccm_disk[1024*64];  // 0-64K
static char sram_disk[1024*32];      // 64-96K
static const uint32_t ccm_size = sizeof(ccm_disk);
static const uint32_t sram_size = sizeof(sram_disk);
const uint32_t sram_disk_size = sizeof(ccm_disk)+sizeof(sram_disk);
#else
static char _ccm ccm_disk[1024*64];  // 0-64K
static uint32_t flash_sector_addr_map = FLASH_SECTOR_ADDR_MAP;      // 64-96K
static const uint32_t ccm_size = sizeof(ccm_disk);
#define FLASH_SIZE    (1024*128+1024*128)    // 128KB + 128KB
static const uint32_t flash_size = FLASH_SIZE;  // 128KB + 128KB
const uint32_t sram_disk_size = sizeof(ccm_disk)+FLASH_SIZE;
#endif
//void SRAM_Init(void)
//{
//	int i = 0;
//	for(i=0; i<SRAM_LEN_K*1024; i++)
//	{
//		SRAM_BUF[i] = 0;
//	}
//}

static int __SRAM_Write(const uint8_t* const pBuffer, const uint32_t WriteAddr, const uint16_t NumByteToWrite)
{
	int i=0;
	//dprintf("WriteAddr: %08X\r\n", WriteAddr);
	for(i=0;i<NumByteToWrite;i++)// Write SRAM
	{
		if((WriteAddr+i)>=sizeof(ccm_disk)) return -1;
		ccm_disk[WriteAddr+i] = pBuffer[i];
	}
	return 0;
}
static int __SRAM_Read(uint8_t* const pBuffer, const uint32_t ReadAddr, const uint16_t NumByteToRead)
{
	int i=0;
	//dprintf("ReadAddr: %08X\r\n", ReadAddr);
	for(i=0;i<NumByteToRead;i++)// Read SRAM
	{
		if((ReadAddr+i)>=sizeof(ccm_disk)) return -1;
		pBuffer[i] = ccm_disk[ReadAddr+i];
	}
	return 0;
}
void flash_disk_init(void)
{
  /* USER CODE BEGIN 6 */
#ifdef FLASH_SECTOR_ADDR_MAP
  FLASH_Erase(FLASH_SECTOR_ADDR_MAP, FLASH_SECTOR_ADDR_MAP+flash_size-1);
#endif
  /* USER CODE END 6 */
}

void sram_disk_init(void)
{
  /* USER CODE BEGIN 6 */
  memset(ccm_disk, 0, ccm_size);
#ifndef FLASH_SECTOR_ADDR_MAP
  memset(sram_disk, 0, sram_size);
#else
  FLASH_Erase(FLASH_SECTOR_ADDR_MAP, FLASH_SECTOR_ADDR_MAP+flash_size-1);
#endif
  /* USER CODE END 6 */
}
uint16_t sram_disk_read(uint8_t *const buf, const uint32_t blk_addr, const uint16_t blk_len)
{
  /* USER CODE BEGIN 6 */
	  const uint32_t _addr = blk_addr*SRAM_DEFAULT_BLOCK_SIZE;
	  const uint32_t _len = blk_len*SRAM_DEFAULT_BLOCK_SIZE;
	  //app_debug("@%s \t\tblk_addr: %4d | %3d \r\n", __func__, blk_addr, blk_len);
#ifndef FLASH_SECTOR_ADDR_MAP
  // sram
  if(ccm_size<=_addr) memcpy(buf, &sram_disk[_addr-ccm_size], _len);
  else
  {
	  uint32_t len;
	  uint32_t offset;
	  len = ccm_size - _addr;
	  // ccm
	  if(len>=_len) memcpy(buf, &ccm_disk[_addr], _len);
	  else // sram && ccm
	  {
		  offset = _addr-ccm_size;
		  if(offset>=sram_size) return 0;
		  memcpy(buf, &ccm_disk[_addr], len);
		  memcpy(&buf[len], &sram_disk[offset], _len-len);
	  }
  }
#else
  // flash
#if 1
//  if(ccm_size<=_addr) Flash_Read(flash_sector_addr_map+_addr-ccm_size, (uint32_t*)buf, (blk_len*512)/4);  // /4
//  else
//  {
////	  uint32_t len;
////	  //uint32_t offset;
////	  len = ccm_size - _addr;
////	  // ccm
////	  if(len>=_len) SRAM_Read(buf, _addr, _len);
////	  else // len<_len flash && ccm
////	  {
////		  //offset = _addr-ccm_size;
////		  //if(offset>=flash_size) return 0;
////		  //memcpy(buf, &ccm_disk[_addr], len);
////		  SRAM_Read(buf, _addr, len);
////		  //memcpy(&buf[len], &sram_disk[offset], _len-len);
////		  Flash_Read(flash_sector_addr_map+0, (uint32_t*)&buf[len], (_len-len)>>2);  // /4
////	  }
//	  SRAM_Read(buf, _addr, _len);
//  }
	uint32_t Memory_Offset = blk_addr*512;
	//printf("@%s [%d]addr: %03dK | %04d\r\n", __func__, lun, (blk_addr*512)/1024, blk_len*512);
	if(ccm_size>Memory_Offset) // FAT32 ==> SRAM
	{
		__SRAM_Read(buf, blk_addr*512, blk_len*512) ;
	}
	else
	{
		Memory_Offset -= (uint32_t)ccm_size;
		Flash_Read(flash_sector_addr_map+Memory_Offset,(uint32_t*)buf, (blk_len*512)/4);
	}
#else
	uint32_t Memory_Offset = blk_addr*512;
	//printf("@%s [%d]addr: %03dK | %04d\r\n", __func__, lun, (blk_addr*512)/1024, blk_len*512);
	if(ccm_size>Memory_Offset) // FAT32 ==> SRAM
	{
		__SRAM_Read(buf, blk_addr*512, blk_len*512) ;
	}
	else
	{
		Memory_Offset -= (uint32_t)ccm_size;
		Flash_Read(flash_sector_addr_map+Memory_Offset,(uint32_t*)buf, (blk_len*512)/4);
	}
#endif
#endif
  return _len;
  /* USER CODE END 6 */
}
uint16_t sram_disk_write(const uint8_t *const buf, const uint32_t blk_addr, const uint16_t blk_len)
{
  /* USER CODE BEGIN 7 */
	  const uint32_t _addr = blk_addr*SRAM_DEFAULT_BLOCK_SIZE;
	  const uint32_t _len = blk_len*SRAM_DEFAULT_BLOCK_SIZE;
	  //app_debug("@%s \t\tblk_addr: %4d | %3d \r\n", __func__, blk_addr, blk_len);
#ifndef FLASH_SECTOR_ADDR_MAP
  // sram
  if(ccm_size<=_addr) memcpy(&sram_disk[_addr-ccm_size], buf, _len);
  else
  {
	  uint32_t len;
	  uint32_t offset;
	  len = ccm_size - _addr;
	  // ccm
	  if(len>=_len) memcpy(&ccm_disk[_addr], buf, _len);
	  else // sram && ccm
	  {
		  offset = _addr-ccm_size;
		  if(offset>=sram_size) return 0;
		  memcpy(&ccm_disk[_addr], buf, len);
		  memcpy(&sram_disk[offset], &buf[len], _len-len);
	  }
  }
#else
  // flash
#if 1
  //uint32_t offset;
//  if(ccm_size<=_addr)
//  {
//	  uint32_t offset = _addr-ccm_size;
//	  Flash_Write(flash_sector_addr_map+offset, (const uint32_t*)buf, (blk_len*512)/4);   // /4
//  }
//  else // ccm_size>_addr
//  {
////	  uint32_t len;
////	  len = ccm_size - _addr;
////	  // ccm
////	  if(len>=_len) SRAM_Write(buf, _addr, _len);
////	  else // len<_len  flash && ccm
////	  {
////		  //offset = _len-len;
////		  //if(offset>=flash_size) return 0;
////		  //memcpy(&ccm_disk[_addr], buf, len);
////		  SRAM_Write(buf, _addr, len);
////		  //memcpy(&sram_disk[offset], &buf[len], _len-len);
////		  Flash_Write(flash_sector_addr_map+0, (const uint32_t*)&buf[len], (_len-len)>>2);   // /4
////	  }
//	  SRAM_Write(buf, _addr, _len);
//  }
	uint32_t Memory_Offset = blk_addr*512;
	//printf("@%s [%d]addr: %03dK | %04d\r\n", __func__, lun, (blk_addr*512)/1024, blk_len*512);
	//check_Sojiro(Memory_Offset, (char*)buf, blk_len*512);
	if(ccm_size>Memory_Offset) // FAT32 ==> SRAM
	{
		__SRAM_Write(buf, blk_addr*512, blk_len*512);
	}
	else
	{
		//DownLoadStatus();
		Memory_Offset -= (uint32_t)ccm_size;
		Flash_Write(flash_sector_addr_map+Memory_Offset,(const uint32_t*)buf, (blk_len*512)/4);
	}
#else
	uint32_t Memory_Offset = blk_addr*512;
	//printf("@%s [%d]addr: %03dK | %04d\r\n", __func__, lun, (blk_addr*512)/1024, blk_len*512);
	//check_Sojiro(Memory_Offset, (char*)buf, blk_len*512);
	if(ccm_size>Memory_Offset) // FAT32 ==> SRAM
	{
		__SRAM_Write(buf, blk_addr*512, blk_len*512);
	}
	else
	{
		//DownLoadStatus();
		Memory_Offset -= (uint32_t)ccm_size;
		Flash_Write(flash_sector_addr_map+Memory_Offset,(const uint32_t*)buf, (blk_len*512)/4);
	}
#endif
#endif
  return _len;
  /* USER CODE END 7 */
}
#endif

/* USER CODE END disableSDInit */

/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

/* Private function prototypes -----------------------------------------------*/
static DSTATUS SRAM_CheckStatus(BYTE lun);
DSTATUS SRAM_initialize (BYTE);
DSTATUS SRAM_status (BYTE);
DRESULT SRAM_read (BYTE, BYTE*, DWORD, UINT);
#if _USE_WRITE == 1
DRESULT SRAM_write (BYTE, const BYTE*, DWORD, UINT);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
DRESULT SRAM_ioctl (BYTE, BYTE, void*);
#endif  /* _USE_IOCTL == 1 */

const Diskio_drvTypeDef  SRAM_Driver =
{
  SRAM_initialize,
  SRAM_status,
  SRAM_read,
#if  _USE_WRITE == 1
  SRAM_write,
#endif /* _USE_WRITE == 1 */

#if  _USE_IOCTL == 1
  SRAM_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* USER CODE BEGIN beforeFunctionSection */
/* can be used to modify / undefine following code or add new code */
/* USER CODE END beforeFunctionSection */

/* Private functions ---------------------------------------------------------*/

static DSTATUS SRAM_CheckStatus(BYTE lun)
{
  Stat = STA_NOINIT;

  //if(BSP_SRAM_GetCardState() == MSRAM_OK)
  {
    Stat &= ~STA_NOINIT;
  }

  return Stat;
}

/**
  * @brief  Initializes a Drive
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS SRAM_initialize(BYTE lun)
{
  Stat = STA_NOINIT;

  //memset(sram_disk, 0, sram_disk_size);
  sram_disk_init();
  Stat = SRAM_CheckStatus(lun);

  return Stat;
}

/**
  * @brief  Gets Disk Status
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS SRAM_status(BYTE lun)
{
  return SRAM_CheckStatus(lun);
}

/* USER CODE BEGIN beforeReadSection */
/* can be used to modify previous code / undefine following code / add new code */
/* USER CODE END beforeReadSection */
/**
  * @brief  Reads Sector(s)
  * @param  lun : not used
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
              
DRESULT SRAM_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
  DRESULT res = RES_ERROR;

//  if(BSP_SRAM_ReadBlocks((uint32_t*)buff,
//                       (uint32_t) (sector),
//                       count, SRAM_TIMEOUT) == MSRAM_OK)
  {
    /* wait until the read operation is finished */
//    while(BSP_SRAM_GetCardState()!= MSRAM_OK)
//    {
//    }
	  //memcpy(buff, &sram_disk[sector*SRAM_DEFAULT_BLOCK_SIZE], count*SRAM_DEFAULT_BLOCK_SIZE);
	  app_debug("@%s \tsector: %4d | %3d \r\n", __func__, sector, count);
	  sram_disk_read(buff, sector, count);
    res = RES_OK;
  }

  return res;
}

/* USER CODE BEGIN beforeWriteSection */
/* can be used to modify previous code / undefine following code / add new code */
/* USER CODE END beforeWriteSection */
/**
  * @brief  Writes Sector(s)
  * @param  lun : not used
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
              
DRESULT SRAM_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
  DRESULT res = RES_ERROR;

//  if(BSP_SRAM_WriteBlocks((uint32_t*)buff,
//                        (uint32_t)(sector),
//                        count, SRAM_TIMEOUT) == MSRAM_OK)
  {
	/* wait until the Write operation is finished */
//    while(BSP_SRAM_GetCardState() != MSRAM_OK)
//    {
//    }
	  //memcpy(&sram_disk[sector*SRAM_DEFAULT_BLOCK_SIZE], buff, count*SRAM_DEFAULT_BLOCK_SIZE);
	  app_debug("@%s \tsector: %4d | %3d \r\n", __func__, sector, count);
	  sram_disk_write(buff, sector, count);
    res = RES_OK;
  }

  return res;
}
#endif /* _USE_WRITE == 1 */

/* USER CODE BEGIN beforeIoctlSection */
/* can be used to modify previous code / undefine following code / add new code */
/* USER CODE END beforeIoctlSection */
/**
  * @brief  I/O control operation
  * @param  lun : not used
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT SRAM_ioctl(BYTE lun, BYTE cmd, void *buff)
{
  DRESULT res = RES_ERROR;
  //BSP_SRAM_CardInfo CardInfo;

  if (Stat & STA_NOINIT) return RES_NOTRDY;

  switch (cmd)
  {
  /* Make sure that no pending write process */
  case CTRL_SYNC :
    res = RES_OK;
    break;

  /* Get number of sectors on the disk (DWORD) */
  case GET_SECTOR_COUNT :
    //BSP_SRAM_GetCardInfo(&CardInfo);
    *(DWORD*)buff = sram_disk_size / SRAM_DEFAULT_BLOCK_SIZE;
    res = RES_OK;
    break;

  /* Get R/W sector size (WORD) */
  case GET_SECTOR_SIZE :
    //BSP_SRAM_GetCardInfo(&CardInfo);
    *(WORD*)buff = SRAM_DEFAULT_BLOCK_SIZE;
    res = RES_OK;
    break;

  /* Get erase block size in unit of sector (DWORD) */
  case GET_BLOCK_SIZE :
    //BSP_SRAM_GetCardInfo(&CardInfo);
    *(DWORD*)buff = sram_disk_size / SRAM_DEFAULT_BLOCK_SIZE;
    res = RES_OK;
    break;

  default:
    res = RES_PARERR;
  }

  return res;
}
#endif /* _USE_IOCTL == 1 */

/* USER CODE BEGIN afterIoctlSection */
/* can be used to modify previous code / undefine following code / add new code */
/* USER CODE END afterIoctlSection */

/* USER CODE BEGIN lastSection */ 
/* can be used to modify / undefine previous code or add new code */
/* USER CODE END lastSection */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

