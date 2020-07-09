/******************** (C) COPYRIGHT 2015 merafour ********************
* File Name          : version.h
* Author             : 冷月追风 && Merafour
* Version            : V1.0.0
* Last Modified Date : 07/04/2020
* Description        : 版本信息.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
*******************************************************************************/
#ifndef __VERSION_H__
#define __VERSION_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 *        ...| Board name      | ...H
 *           |                 | {4*4}B
 *           |-----------------|
 *         22| signDate        | 58H
 *        ...|                 | ...H
 *           |                 | {8*4}B
 *           |-----------------|
 *         32| empty           | 80H
 *           |                 | ...H
 *           |                 |	   40*4:VERSION
 */
// 256B
//const uint32_t ConfigExamples[(0x0F0)/4] = {
//	   0x00000000, // __initial_sp
//	   0x00000000, // Reset_Handler
//       0x01010000, // default //0K	| swd| erase| flashH| flashL|
//	   //0x00010100, // default //256K	| swd| erase| flashH| flashL|
//	   0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12, //mtext
//	   0x12, // empty
//	   'E', 0x00000004,// KEY :PE4,High
//	   'F', 0xFFFF0009,// LED1:PF9 ,Low
//	   'F', 0xFFFF000A,// LED2:PF10 ,Low
//	   0x72466951, 0x00006565, 0x00000000, 0x00000000, // Board:QiFree
//	   // signDate
//	   0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
//	   // IO default
//	   0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
//	   // License
//	   0x990951ba,
//	   0x00000000, 0x00000000, 0x00000000, 0x00000000,
//	   0x00000000, 0x00000000, 0x00000000, 0x00000000,
//	   0x00000000, 0x00000000, 0x00000000, 0x00000000,
//};
//__attribute__ ((section (".version"))) const uint8_t _version[0x10] __attribute__((used)) = "V "VERSION"."PATCHLEVEL"."SUBLEVEL"\r\n\0";

// 嵌入式设备版本号定义
struct Emb_Device_Version {
	const char version[32];       // Boot Version, eg:"2.1.10-hw2.0-HSH.Gen"
	const char model[16];         // Device Model,eg: "0A0"
	const char author[16];        // Developers
	const char hardware[16];      // Board name
	const uint32_t mtext[8];      // boot signature Key
	const uint32_t signData[8];   // boot signature
	const uint32_t signApp[8];    // firmware signature
	//const uint8_t swd;            // Debug enable
	const uint8_t erase;          //
	const uint8_t flash;          // Flash map addr, 2K
	const uint8_t mount;          // Flash mount size, 4K
	const uint8_t vbus;           // VBUS Port && Pin
	const uint8_t led;            // LED Port && Pin
	const union {
		uint8_t _cfg;
		struct{
			const uint8_t swd: 1;     // Debug enable
			const uint8_t vbus: 1;    // Logic level, vbus, 0:low, 1:high
			const uint8_t led: 1;     // Logic level, led, 0:low on, 1:high on
			const uint8_t debug: 3;   // Debug Info out, 0: off, 1:UART1, 2:UART2, 3:UART3, 4:UART4, 5:UART5, 6:UART6, 7:CDC
			const uint8_t reserve: 2; // reserve
		};
	}cfg;
	const uint8_t reserve1;       // reserve
	const uint8_t reserve2;       // reserve
};

#define EMB_DEBUG_NONE      0
#define EMB_DEBUG_UART1     1
#define EMB_DEBUG_UART2     2
#define EMB_DEBUG_UART3     3
#define EMB_DEBUG_UART4     4
#define EMB_DEBUG_UART5     5
#define EMB_DEBUG_UART6     6
#define EMB_DEBUG_CDC       7


extern const struct Emb_Device_Version  Emb_Version;

#ifdef __cplusplus
}
#endif

#endif /* __VERSION_H__ */

/************************ (C) COPYRIGHT Merafour *****END OF FILE****/

