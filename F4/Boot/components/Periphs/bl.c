/******************** (C) COPYRIGHT 2015 merafour ********************
* File Name          : Param Table.c
* Author             : 冷月追风 && Merafour
* Version            : V1.0.0
* Last Modified Date : 07/03/2020
* Description        : STM32F4 片内 Flash 操作接口.
********************************************************************************
* https://merafour.blog.163.com
* merafour@163.com
* https://github.com/merafour
*******************************************************************************/
#include <stdint.h>
#include "version.h"
#include "Periphs/uart.h"

extern const uint32_t param_flash_start;      // App 地址
extern const uint32_t param_flash_size;       // App 大小

extern void Periphs_DeInit(void);

/*
 * 中断向量表:
 * isr_vector|-----------------| start
 * uint32_t 0| _estack         | 0000H
 *          1| Reset_Handler   | 0004H
 *          2| NMI_Handler     | 0008H
 *        ...| Ini0            | ...H
 *           |-----------------|
 *           | code            |
 *           |                 |
 *           |                 |
 *           |                 |
 *           |-------End-------|
 */

typedef  void (*pFunction)(void);
//static inline void __set_MSP(uint32_t topOfMainStack)
//{
//  register uint32_t __regMainStackPointer     __ASM("msp");
//  __regMainStackPointer = topOfMainStack;
//}
void boot_app(void)
{
	static pFunction Jump_To_Application;
	const uint32_t *const _isr_vector = (const uint32_t *)param_flash_start;
	// sp
    if((_isr_vector[0] & 0x2FFE0000 ) != 0x20000000) return ;
    // Reset_Handler
    if((_isr_vector[1]<param_flash_start) || (_isr_vector[1]>(param_flash_start+param_flash_size))) return ;
    Periphs_DeInit();
	/* Jump to user application */
	Jump_To_Application = (pFunction) _isr_vector[1];
	/* Initialize user application's Stack Pointer */
	__set_MSP(_isr_vector[0]);
	Jump_To_Application();
	while(1) NVIC_SystemReset();	// Program Reset
}
static inline void do_jump(uint32_t stacktop, uint32_t entrypoint)
{
	asm volatile(
		"msr msp, %0	\n"
		"bx	%1	\n"
		: : "r"(stacktop), "r"(entrypoint) :);

	// just to keep noreturn happy
	for (;;) NVIC_SystemReset();	// Program Reset
}
void jump_to_app()
{
	const uint32_t *const app_base = (const uint32_t *)param_flash_start;

	/*
	 * We refuse to program the first word of the app until the upload is marked
	 * complete by the host.  So if it's not 0xffffffff, we should try booting it.
	 */
	// check sp point
	if (app_base[0] == 0xffffffff) {
		return;
	}
    if(0x20000000 == (app_base[0] & 0x2FFE0000 ))
    {
    	return;
    }
	/*
	 * The second word of the app is the entrypoint; it must point within the
	 * flash area (or we have a bad flash).
	 */
	if (app_base[1] < param_flash_start) {
		return;
	}

	if (app_base[1] >= (param_flash_start + param_flash_size)) {
		return;
	}
#if 0
	/* just for paranoia's sake */
	arch_flash_lock();

	/* kill the systick interrupt */
	arch_systic_deinit();

	/* deinitialise the interface */
	cfini();

	/* reset the clock */
	clock_deinit();

	/* deinitialise the board */
	board_deinit();

	/* switch exception handlers to the application */
	arch_setvtor(APP_LOAD_ADDRESS);
#endif
	Periphs_DeInit();
	/* extract the stack and entrypoint from the app vector table and go */
	do_jump(app_base[0], app_base[1]);
}


