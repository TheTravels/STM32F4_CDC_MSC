/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    SRAM_diskio.h
  * @brief   Header for SRAM_diskio.c module
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

/* Note: code generation based on SRAM_diskio_template.h */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SRAM_DISKIO_H
#define __SRAM_DISKIO_H

/* USER CODE BEGIN firstSection */ 
/* can be used to modify / undefine following code or add new definitions */
/* USER CODE END firstSection */

/* Includes ------------------------------------------------------------------*/
//#include "bsp_driver_sd.h"
#include "ff_gen_drv.h"
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
extern const Diskio_drvTypeDef  SRAM_Driver;
extern const uint32_t sram_disk_size;
extern uint16_t sram_disk_read(uint8_t *const buf, const uint32_t _addr, const uint16_t _len);
extern uint16_t sram_disk_write(const uint8_t *const buf, const uint32_t _addr, const uint16_t _len);
/* USER CODE BEGIN lastSection */ 
/* can be used to modify / undefine previous code or add new definitions */
/* USER CODE END lastSection */

#endif /* __SRAM_DISKIO_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

