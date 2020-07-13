/******************** (C) COPYRIGHT 2015 merafour ********************
* File Name          : version.c
* Author             : 冷月追风 && Merafour
* Version            : V1.0.0
* Last Modified Date : 07/04/2020
* Description        : 版本信息.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
*******************************************************************************/
#include "version.h"

// define version data section
#define _version __attribute__((unused, section(".version")))


/*
 * Flash map
 * bin       |-----------------| start
 * uint32_t 0| __initial_sp    | 000H
 *          1| Reset_Handler   | 004H
 *          2| Flash KB        | 008H  | swd| erase| flashH| flashL|
 *           |-----------------|
 *          3|                 | 00CH
 *        ...| mtext           | ...H
 *         10|                 | {8*4}B
 *           |-----------------|
 *         11| empty           |
 *           |-----------------|
 *         12| KEY_GPIO        | 030H
 *         13| KEY_Pin         |       | Low| Pin|
 *         14| LED1_GPIO       |
 *         15| LED1_Pin        |       | Low| Pin|
 *         16| LED2_GPIO       |
 *         17| LED2_Pin        |       | Low| Pin|
 *           |-----------------|
 *         18|                 | 48H
 */
//static int led_low=0;
//static int key_low=0;
//static uint32_t swd=1;

const struct Emb_Device_Version  _version __attribute__ ((aligned (512))) Emb_Version = {
		.version = "1.1.10-hw2.0-HSH.BL",    // Boot Version, eg:"2.1.10-hw2.0-HSH.Gen"
		.model = "45",                       // Device Model,eg: "0A0"
		.author = "Merafour",                // Developers
		.hardware = "EPS418",                // Board name
		.mtext = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},      // boot signature Key
		.signData = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},   // boot signature
		.signApp = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},    // firmware signature
		.crc = 0xFFFFFFFF,
		//.swd = 0,                          // Debug enable
		.erase = 0,                          //
		.flash = 256/2,                      // Flash map addr, 2K
		.mount = 256/2,                      // Flash mount size, 4K
		.vbus = 0xA9,                        // VBUS Port && Pin, PA9
		.led  = 0xDD,                        // LED Port && Pin,  PD13
		.cfg = {
				.swd = 1,
				.vbus = 0,
				.led = 1,
				//.debug = EMB_DEBUG_UART3,    // Debug out UART3
				.debug = EMB_DEBUG_NONE,     // 无调试信息
		},
		.reserve1 = 0x55,
		.reserve2 = 0x55,
};


/************************ (C) COPYRIGHT Merafour *****END OF FILE****/

