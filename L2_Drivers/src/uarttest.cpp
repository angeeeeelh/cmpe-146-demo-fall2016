/*
 * uarttest.c
 *
 *  Created on: Dec 19, 2016
 *      Author: angel
 */
#include "uarttest.hpp"
#include "LPC17xx.h"
#include "sys_config.h"

using namespace std;



bool uarttest:: uart_getstring(string* b)
{
	string total;
	if (!LPC_UART2->LSR & (1 << 0))
	{
		//int temp = 0;
		while (!LPC_UART2->LSR & (1 << 0))
			{
				//&b += &b->at(temp);
			}
		return true;
	}
	else return false;
};
bool uarttest:: uart_putstring(string* b)
{
	//for(int temp = 0; temp < b->length(); temp++)
		{
		//LPC_UART2->THR = (uint_t) b->at(temp);
		while (!LPC_UART2->LSR & (1 << 5)); // if 1 is empty
		}
	    return 1;
};

 uarttest:: uarttest()
{

		// activating the peripheral with power using pconp

			LPC_SC->PCONP |= (1 << 24);     // UART2 activate power to peripheral
			LPC_SC->PCLKSEL1 &= ~(3 << 16); // UART2 clock value reset first = 00
			LPC_SC->PCLKSEL1 |=  (1 << 16); // UART2 clock set to the CCLK by setting value = 01
			///
			///	Setting the pin function
			///

			// Line control register
			LPC_UART2->LCR |= (3 << 0); // UART2 8 bit char length
			LPC_UART2->LCR |= (1 << 7); // UART2 DLAB enabled

			// Fifo control register
			LPC_UART2->FCR |= (1 << 0); // UART1 RX and TX FIFO enabled and the rest FIFO control register options enabled


			// TX functions selected with 00 and then 10 value,
			LPC_PINCON->PINSEL4 &= ~(3<<16); //clear to zero
			LPC_PINCON->PINSEL4 |= (2<<16); //set to 01
			// rX functions selected with 00 and then 10 value,
			LPC_PINCON->PINSEL4 &= ~(3<<18); //clear to zero
			LPC_PINCON->PINSEL4 |= (2<<18); //set to 01


			LPC_UART2->DLM = 0;
			// cout << sys_get_cpu_clock();
			LPC_UART2->DLL = sys_get_cpu_clock() / (16 * 38400);

			LPC_UART2->LCR &= ~(1 << 7); // UART2 DLAB disabled


			//rx_gpsHandle = QueueCreate(30, sizeof(void*));
			//tx_gpsHandle = QueueCreate(30, sizeof(void*));

}




