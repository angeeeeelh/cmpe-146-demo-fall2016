/*
 * eint.c
 *
 *  Created on: Sep 29, 2016
 *      Author: angel
 */


#include "eint.h"
#include "LPC17xx.h"
#include "printf_lib.h"
#include "stdio.h"

void EINT3_IRQHandler(void)
{
	NVIC_DisableIRQ(EINT3_IRQn); //Disable interrupt handler
	LPC_GPIOINT->IO2IntEnR &= ~(1<<0); // Disable interrupt
	LPC_GPIOINT->IO2IntClr |= (1<<0); //clear the flag
	u0_dbg_printf("Interrupt triggered");
	printf("Triggered\n");
	LPC_GPIOINT->IO2IntEnR &= (1<<0); // Enable interrupt
	NVIC_EnableIRQ(EINT3_IRQn); //Enable interrupt handler
}
