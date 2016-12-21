/*
 *     SocialLedge.com - Copyright (C) 2013
 *
 *     This file is part of free software framework for embedded processors.
 *     You can use it and/or distribute it as long as this copyright header
 *     remains unmodified.  The code is free for personal use and requires
 *     permission to use in a commercial product.
 *
 *      THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 *      OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 *      MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 *      I SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 *      CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *     You can reach the author of this software at :
 *          p r e e t . w i k i @ g m a i l . c o m
 */


#include "tasks.hpp"
#include "examples/examples.hpp"
#include "LPC17xx.h"
#include "lpc_pwm.hpp"


SemaphoreHandle_t xSemaphore;

class compass_task : public scheduler_task {
    public:

        compass_task(uint8_t priority) : scheduler_task("compass", 512*4, priority){}

        bool init(void){
            BIT(LPC_SC->PCONP).b25 = 1;                     //UART3 power enable
            BIT(LPC_SC->PCLKSEL1).b19_18 = 1;               //UART3 peripheral clock selection (PCLK_peripheral = CCLK)
            BIT(LPC_PINCON->PINSEL9).b25_24 = 3;            //TXD3 function
            BIT(LPC_PINCON->PINSEL9).b27_26 = 3;            //RXD3 function
            uint16_t DL = sys_get_cpu_clock()/(9600*16);   //divisor latch value
            BIT(LPC_UART3->LCR).b7 = 1;                     //set DLAB to one to change baud rate divisor value
            LPC_UART3->DLM = DL>>8;                         //set 8 most significant bits
            LPC_UART3->DLL = DL>>0;                         //set 8 least significant bits
            BIT(LPC_UART3->LCR).b1_0 = 3;                   //set 8-bit character word length
            BIT(LPC_UART3->LCR).b7 = 0;                     /*set DLAB back to zero to read from the receiver buffer
                                                            register and write to the transmit holding register*/
            return true;
        }

        bool run(void *p){
        	PWM pwm(PWM::pwm1,50);
            while(1){
                int compass = 0;
                LPC_UART3->THR = 0x13;                  //send byte "i" from the message string
                while(!(LPC_UART3->LSR & (1 << 5)));    //wait for empty transmitter holding register
                while(!(LPC_UART3->LSR & (1 << 0)));
                compass += LPC_UART3->RBR*256;
                while(!(LPC_UART3->LSR & (1 << 0)));
                compass += LPC_UART3->RBR;
                pwm.set((compass%900)/180.0f+5);
                vTaskDelay(100);
            }
            return true;
        }
};

int main(void)
{

	xSemaphore = xSemaphoreCreateBinary();

	//Set PWM Pin 2.0
	LPC_PINCON -> PINSEL4 &= ~(3<<0);
	LPC_PINCON -> PINSEL4 |= (1<<0);


    //scheduler_add_task(new terminalTask(PRIORITY_HIGH));
    scheduler_add_task(new compass_task(PRIORITY_MEDIUM));
    scheduler_add_task(new gps_task(PRIORITY_MEDIUM));
    //scheduler_add_task(new uart_task(PRIORITY_MEDIUM));
    //scheduler_add_task(new navigation_task(PRIORITY_MEDIUM));



    scheduler_start(); ///< This shouldn't return
    return -1;
}
