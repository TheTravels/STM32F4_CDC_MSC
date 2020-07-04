/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
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

#include "fatfs.h"
#include <string.h>

void fs_test(void)
{
	FATFS fs;           /* Filesystem object */
    FIL fil;            /* File object */
    FRESULT res;  /* API result code */
    UINT bw;            /* Bytes written */
    uint8_t work[1024]; /* Work area (larger is better for processing time) */
    uint8_t mm[50];
    uint8_t wtext[] = "\r\n测试数据！！！"; //
    UINT i;

	//uart_printf("文件系统测试开始:\r\n");
	/* 格式化文件系统 */
	res = f_mkfs("0:", FM_FAT|FM_SFD, 0, work, sizeof(work));//"0:"是卷标，来自于 #define SPI_FLASH		0
	if (res)
	{
		//uart_printf("文件系统格式化失败.\r\n");
		return ;
	}
	else
	{
		//uart_printf("文件系统格式化成功.\r\n");
	}
	/* 挂载文件系统 */
	res = f_mount(&fs, "0:", 0);
	if (res)
	{
		//uart_printf("文件系统挂载失败.\r\n");
	}
	else
	{
		//uart_printf("文件系统挂载成功.\r\n");
	}
	res = f_setlabel("0:SRAM");
	/* Create a file as new */
	res = f_open(&fil, "0:/123.txt", FA_CREATE_NEW|FA_WRITE|FA_READ);
	if (res)
	{
		//uart_printf("打开文件失败.\r\n");
	}
	else
	{
		//uart_printf("打开文件成功.\r\n");
	}
	/* Write a message */
	res = f_write(&fil, "Hello,World!", 12, &bw);
	res = f_write(&fil, wtext, sizeof(wtext), (void *)&bw);
	//uart_printf("res write:%d\r\n",res);
	if (bw == 12)
	{
		//uart_printf("写文件成功!\r\n");
	}
	else
	{
		//uart_printf("写文件失败!\r\n");
	}
	res = f_size(&fil);
	//uart_printf("文件大小:%d Bytes.\r\n",res);
	memset(mm,0x0,50);
	f_lseek(&fil,0);
	res = f_read(&fil,mm,12,&i);
	if (res == FR_OK)
	{
		//uart_printf("读文件成功!\r\n");
		//uart_printf("读到数据长度:%d Bytes.\r\n",i);
	}
	else
	{
		//uart_printf("读文件失败!\r\n");
	}
	//uart_printf("读到如下数据:\r\n");
	//buff_print((char *)mm,12);
	/* Close the file */
	f_close(&fil);
	/*卸载文件系统*/
	f_mount(0, "0:", 0);
	//uart_printf("文件系统测试完毕.\r\n");
}

void fs_test_sdio(void)
{
	FATFS fs;           /* Filesystem object */
    FIL fil;            /* File object */
    FRESULT res;  /* API result code */
    UINT bw;            /* Bytes written */
    uint8_t work[1024]; /* Work area (larger is better for processing time) */
    uint8_t mm[50];
    uint8_t wtext[] = "\r\n测试数据！！！"; //
    UINT i;

	//uart_printf("文件系统测试开始:\r\n");
	/* 格式化文件系统 */
	res = f_mkfs("1:", FM_FAT32, 0, work, sizeof(work));//"0:"是卷标，来自于 #define SPI_FLASH		0
	if (res)
	{
		//uart_printf("文件系统格式化失败.\r\n");
		return ;
	}
	else
	{
		//uart_printf("文件系统格式化成功.\r\n");
	}
	/* 挂载文件系统 */
	res = f_mount(&fs, "1:", 0);
	if (res)
	{
		//uart_printf("文件系统挂载失败.\r\n");
	}
	else
	{
		//uart_printf("文件系统挂载成功.\r\n");
	}
	res = f_setlabel("1:SD");
	/* Create a file as new */
	res = f_open(&fil, "1:/123.txt", FA_CREATE_NEW|FA_WRITE|FA_READ);
	if (res)
	{
		//uart_printf("打开文件失败.\r\n");
	}
	else
	{
		//uart_printf("打开文件成功.\r\n");
	}
	/* Write a message */
	res = f_write(&fil, "Hello,World!", 12, &bw);
	res = f_write(&fil, wtext, sizeof(wtext), (void *)&bw);
	//uart_printf("res write:%d\r\n",res);
	if (bw == 12)
	{
		//uart_printf("写文件成功!\r\n");
	}
	else
	{
		//uart_printf("写文件失败!\r\n");
	}
	res = f_size(&fil);
	//uart_printf("文件大小:%d Bytes.\r\n",res);
	memset(mm,0x0,50);
	f_lseek(&fil,0);
	res = f_read(&fil,mm,12,&i);
	if (res == FR_OK)
	{
		//uart_printf("读文件成功!\r\n");
		//uart_printf("读到数据长度:%d Bytes.\r\n",i);
	}
	else
	{
		//uart_printf("读文件失败!\r\n");
	}
	//uart_printf("读到如下数据:\r\n");
	//buff_print((char *)mm,12);
	/* Close the file */
	f_close(&fil);
	/*卸载文件系统*/
	f_mount(0, "1:", 0);
	//uart_printf("文件系统测试完毕.\r\n");
}


/* USER CODE BEGIN Application */
     
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
