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

/**
 * @file
 * @brief Contains FreeRTOS Tasks
 */
#ifndef TASKS_HPP_
#define TASKS_HPP_

#include "scheduler_task.hpp"
#include "soft_timer.hpp"
#include "command_handler.hpp"
#include "wireless.h"
#include "char_dev.hpp"
#include "i2c2.hpp"
#include "storage.hpp"

#include "event_groups.h"
#include "sstream"
#include "string"
#include "FreeRTOS.h"
#include "semphr.h"
#include "stdio.h"
#include "iostream"
#include "printf_lib.h"
#include "LPC17xx.h"
//#include "eint.h"
#include "uart2.hpp"
#include "gps.hpp"

using namespace std;




/*Navigation task is used to set a starting coordinate and destination coordinate for a boat to travel.
Using GPS and compass sensor data servo and motor control is used to get the boat to the end coordinate.
 */

class navigation_task : public scheduler_task
{
    public:
        navigation_task(uint8_t priority);     ///< Constructor

        bool run(void *p);                  ///< The main loop
        bool init();
    private:

};



/**
 * Terminal task is our UART0 terminal that handles our commands into the board.
 * This also saves and restores the "disk" telemetry.  Disk telemetry variables
 * are automatically saved and restored across power-cycles to help us preserve
 * any non-volatile information.
 */
class terminalTask : public scheduler_task
{
    public:
        terminalTask(uint8_t priority);     ///< Constructor
        bool regTlm(void);                  ///< Registers telemetry
        bool taskEntry(void);               ///< Registers commands.
        bool run(void *p);                  ///< The main loop

    private:
        // Command channels device and input command str
        typedef struct {
            CharDev *iodev; ///< The IO channel
            str *cmdstr;    ///< The command string
            bool echo;      ///< If input should be echo'd back
        } cmdChan_t;

        VECTOR<cmdChan_t> mCmdIface;   ///< Command interfaces
        CommandProcessor mCmdProc;     ///< Command processor
        uint16_t mCommandCount;        ///< terminal command count
        uint16_t mDiskTlmSize;         ///< Size of disk variables in bytes
        char *mpBinaryDiskTlm;         ///< Binary disk telemetry
        SoftTimer mCmdTimer;           ///< Command timer

        cmdChan_t getCommand(void);
        void addCommandChannel(CharDev *channel, bool echo);
        void handleEchoAndBackspace(cmdChan_t *io, char c);
        bool saveDiskTlm(void);
};

/**
 * Remote task is the task that monitors the IR remote control signals.
 * It can "learn" remote control codes by typing "learn" into the UART0 terminal.
 * Thereafter, if a user enters a 2-digit number through a remote control, then
 * your function handleUserEntry() is called where you can take an action.
 */
class remoteTask : public scheduler_task
{
    public:
        remoteTask(uint8_t priority);   ///< Constructor
        bool init(void);                ///< Inits the task
        bool regTlm(void);              ///< Registers non-volatile variables
        bool taskEntry(void);           ///< One time entry function
        bool run(void *p);              ///< The main loop

    private:
        /** This function is called when a 2-digit number is decoded */
        void handleUserEntry(int num);
        
        /**
         * @param code  The IR code
         * @param num   The matched number 0-9 that mapped the IR code.
         * @returns true if the code has been successfully mapped to the num
         */
        bool getNumberFromCode(uint32_t code, uint32_t& num);

        uint32_t mNumCodes[10];      ///< IR Number codes
        uint32_t mIrNumber;          ///< Current IR number we're decoding
        SemaphoreHandle_t mLearnSem; ///< Semaphore to enable IR code learning
        SoftTimer mIrNumTimer;       ///< Time-out for user entry for 1st and 2nd digit
};

/**
 * Nordic wireless task to participate in the mesh network and handle retry logic
 * such that packets are resent if an ACK has not been received
 */
class wirelessTask : public scheduler_task
{
    public:
        wirelessTask(uint8_t priority) :
            scheduler_task("wireless", 512, priority)
        {
            /* Nothing to init */
        }

        bool run(void *p)
        {
            wireless_service(); ///< This is a non-polling function if FreeRTOS is running.
            return true;
        }
};

/**
 * Periodic callback dispatcher task
 * This task gives the semaphores that end up calling functions at periodic_callbacks.cpp
 */
class periodicSchedulerTask : public scheduler_task
{
    public:
        periodicSchedulerTask(void);
        bool init(void);
        bool regTlm(void);
        bool run(void *p);

    private:
        bool handlePeriodicSemaphore(const uint8_t index, const uint8_t frequency);
};

/**
 *
 *
 */
class gpio_task : public scheduler_task
{
	public:
		gpio_task(uint8_t priority) : scheduler_task("gpio", 2000, priority)
	{

	}
		bool run(void * p )
		{
			//vTaskDelay(1);

			LPC_GPIO1->FIOPIN |= (1<<0);  // set p1.0 high
			vTaskDelay(100);
			LPC_GPIO1->FIOPIN &= ~(1<<0); //set p1.0 low
			vTaskDelay(100);

			//External Switch
			if( LPC_GPIO2->FIOPIN & (1 << 4) ) // check if pin 2.4 is high
			{
				LPC_GPIO1->FIOPIN |= (1<<22);  // set p1.0 high
				vTaskDelay(100);
				printf("LED ON");
			}

			else
			{
				LPC_GPIO1->FIOPIN &= ~(1<<22);  // set p1.0 low
				vTaskDelay(100);
				printf("LED OFF");
			}

			//Internal Switch
			if( LPC_GPIO1->FIOPIN & (1 << 10) ) // check if pin 1.10 is high
			{
				LPC_GPIO1->FIOPIN &= ~(1<<1);  // set p1.0 low
				vTaskDelay(100);
				printf("LED OFF");
			}

			else
			{

				LPC_GPIO1->FIOPIN |= (1<<1);  // set p1.0 high
				vTaskDelay(100);
				printf("LED ON");
			}


			return true;
		}
		bool init(void)
		{

			LPC_PINCON->PINSEL2 &= ~(3<<0);  // GPIO port 1.0
			LPC_GPIO1->FIODIR |= (1<<0); 	// GPIO port 1.0 output ;  0 = input, 1 = output

			////EXTERNAL
			//p2.4 pin 54 input switch
			LPC_PINCON->PINSEL2 &= ~(3<<8);  // GPIO port 2.4
			LPC_GPIO2->FIODIR &= ~(1<<4); 	// GPIO port2.4 input    1's are affected by 0;  0 = input, 1 = output

			//p1.22 pin 28 led output
			LPC_PINCON->PINSEL3 &= ~(3<<12);  // GPIO port 0.11
			LPC_GPIO1->FIODIR |= (1<<22); 	// GPIO port 0.11 output    1's are affected by 0;  0 = input, 1 = output

			////INTERNAL
			//switch
			//p1.10
			LPC_PINCON->PINSEL2 &= ~(3<<20);  // GPIO port 1.10
			LPC_GPIO1->FIODIR &= ~(1<<10); 	// GPIO port 1.10 output    1's are affected by 0;  0 = input, 1 = output
			//led
			//p1.1
			LPC_PINCON->PINSEL2 &= ~(3<<2);  // GPIO port 1.1
			LPC_GPIO1->FIODIR |= (1<<1); 	// GPIO port 1.1 output ;  0 = input, 1 = output

			//printf("init GPIO");
			return true;
		}
};



/**
 *
 *
 */
class SSP1_task : public scheduler_task
{
	public:
		SSP1_task(uint8_t priority) : scheduler_task("SSP1", 2000, priority){}
		bool run(void * p )
		{
			printf("Running reading task\n");
			// SSEL1 function with 00 and then 10 value,
			LPC_PINCON->PINSEL0 &= ~(3<<12); //clear to zero
			//LPC_PINCON->PINSEL0 |= (0<<12); //set to 10

			LPC_GPIO0->FIODIR |= (1<<6); // output
			LPC_GPIO0->FIOCLR |= (1<<6);


			char rx;

			//manufacturing ID
//			for (int temp = 0; temp <6; temp++)
//				{
//					rx = ssp_byte_transfer(0x9F);
//					if (temp >0) printf("%x \n", rx);
//				}

			LPC_GPIO0->FIOSET |= (1<<6);
			LPC_GPIO0->FIOCLR |= (1<<6);



			rx = ssp_byte_transfer(0xD7);
			rx = ssp_byte_transfer(0xD7);

/*			printf("first status byte %x \n", rx);


			if(rx & (1<<0))
				printf("power of 2 binary \n");
			else
				printf("standard dataflash \n");

			if(rx & (1<<1))
				printf("Sector protection enabled \n");
			else
				printf("Sector protection disabled \n");

			if(rx & (11<<2))
				printf("Density is 16 Mbit \n");
			else
				printf("Other Density\n");
			if(rx & (1<<6))
				printf("Main memory does not match buffer data \n");
			else
				printf("Memory matches buffer data \n");

			if(rx & (1<<1))
				printf("Device is ready \n");
			else
				printf("Device is busy \n");*/




			rx = ssp_byte_transfer(0xD7);

			//printf("%x \n", rx);

/*			if(rx & (1<<0))
				printf("Sector is erase suspended \n");
			else
				printf("no sectors are erase suspended \n");

			if(rx & (1<<1))
				printf("A sector is suspended while using buffer 1 \n");
			else
				printf("A sector is not suspended while using buffer 1 \n");

			if(rx & (1<<2))
				printf("A sector is suspended while using buffer 2 \n");
			else
				printf("A sector is not suspended while using buffer 2 \n");
			if(rx & (1<<3))
				printf("Sector lockdown is enabled \n");
			else
				printf("Sector lockdown is disabled \n");

			if(rx & (1<<5))
				printf("Erase or program error detected \n");
			else
				printf("Device is busy \n");
			if(rx & (1<<7))
				printf("Device is ready \n");
			else
				printf("Device is busy \n");*/


//			ssp_byte_transfer(0xD2);
//			ssp_byte_transfer(0x00);
//			ssp_byte_transfer(0x00);
//			ssp_byte_transfer(0x00);
//
//			//dummy bytes
//			ssp_byte_transfer(0x00);
//			ssp_byte_transfer(0x00);
//			ssp_byte_transfer(0x00);
//			ssp_byte_transfer(0x00);
//
//			for(int bytep = 0; bytep < 528; bytep++)
//				printf("%c", ssp_byte_transfer(0x00));

			while(1);
			return true;
		}
		bool init(void)
		{
			ssp_init();
			return true;
		}
		void ssp_init(void){

			// activating the peripheral with power using pconp
			// Setting the Clock value of SSP1 peripheral
			//

			LPC_SC->PCONP |= (1 << 10);     // SSP1 activate power to peripheral
			LPC_SC->PCLKSEL0 &= ~(3 << 20); // SSP1 clock value = 00
			LPC_SC->PCLKSEL0 |=  (1 << 20); // SSP1 clock set to the CCLK by setting value to 01
			///
			///	Setting the pin function
			///


			// SCK1 function with 00 and then 10 value,
			LPC_PINCON->PINSEL0 &= ~(3<<14); //clear to zero
			LPC_PINCON->PINSEL0 |= (2<<14); //set to 10
			// MISO1 function with 00 and then 10 value,
			LPC_PINCON->PINSEL0 &= ~(3<<16); //clear to zero
			LPC_PINCON->PINSEL0 |= (2<<16); //set to 10
			// MOSI1 function with 00 and then 10 value,
			LPC_PINCON->PINSEL0 &= ~(3<<18); //clear to zero
			LPC_PINCON->PINSEL0 |= (2<<18); //set to 10


		    LPC_SSP1->CR0 = 7;          // 8-bit transfer, FRF = SPI (00) frame format, Clock out polarity = bus clock low between frames (0),
		    //Clock out phase = first clock transition to capture frame (0), Serial clock rate = do not multiply clock divisor
		    LPC_SSP1->CR1 = (1 << 1);   // Loop back mode during normal operation (0), SSP enable = true, MS = 0 to work as Master, SOD = irrelavent for master

		    LPC_SSP1->CPSR = 8;         // Prescaler factor used to divide the Periph clock  8

		    ////// The result is Pclock = sck / 8
		}

		char ssp_byte_transfer (char out)
		{
			LPC_SSP1->DR = out;
			while(LPC_SSP1->SR & (1 << 4)); // loop until SSP busy status flag is inactive
			return LPC_SSP1->DR;
		}

};


/**
 *
 *
 */
class uart_task : public scheduler_task
{
	public:


		uart_task(uint8_t priority) : scheduler_task("UART2", 2000, priority){}
		bool run(void * p )
		{
			return true;
		}

		bool init(void)
				{
					uart2_init();

					return true;
				}
				void uart2_init(void){

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
				}

				int uart2_putchar(char out)
				{

				    LPC_UART2->THR = out;
					while (!LPC_UART2->LSR & (1 << 5)); // if 1 is empty
				    return 1;

				}

				char uart2_getchar(void)
				{

					if (LPC_UART2->LSR & (1 << 0))
						return LPC_UART2->RBR;
					else return '-';
				}


		/*
		bool run(void * p )
		{
			printf("Running Uart task\n");

			printf("Rx or Tx???\n");
			string rxtx;
			cin >> rxtx;
			cout << rxtx;
			if(rxtx.compare("rx") == 0)
			{
				char t;
				t = uart2_getchar();
				while (t != '-')
					{ cout << t;
					t = uart2_getchar();
					}
				cout <<endl;
			}
			else
			{
				printf("Input the line you'd like to print\n");
				string temp;
				cin >> temp;

				cout << "putting string: \n" + temp;
				for(int t = 0; t < temp.length(); t++)
					{
						uart2_putchar(temp.at(t));
					}

				temp = "";
			}

		}*/



};

class EINT_task : public scheduler_task
{
	public:
		EINT_task(uint8_t priority) : scheduler_task("EINT", 2000, priority){};
		bool run(void * p )
		{
			printf("We are running\n");
			while(1);

			return true;

		}
		bool init(void)
		{
			//set gpio input for port 2 pin 0
			//LPC_PINCON->PINSEL4 &= ~(3 << 0); // set to GPIO mode
			LPC_GPIO2->FIODIR  &= ~(3 << 0);  // set as input

			LPC_GPIOINT->IO2IntEnR &= ~(1<<0);
			LPC_GPIOINT->IO2IntEnR |= (1<<0);  //Enable interrupt to detect rising edge
			isr_register(EINT3_IRQn, &callback);
					NVIC_EnableIRQ(EINT3_IRQn);
			printf("We are done initializing\n");

			return true;
		}

		static void callback(void) {

			//NVIC_DisableIRQ(EINT3_IRQn); //Disable interrupt handler
			//LPC_GPIOINT->IO2IntEnR &= ~(1<<0); // Disable interrupt
			LPC_GPIOINT->IO2IntClr |= (1<<0); //clear the flag
			u0_dbg_printf("Interrupt triggered");
			printf("Triggered\n");
			//LPC_GPIOINT->IO2IntEnR &= (1<<0); // Enable interrupt
			//NVIC_EnableIRQ(EINT3_IRQn); //Enable interrupt handler
		}
};


//class I2C_Master : public I2C_Base
//{
//	public:
//	//I2C_Base();
//	I2C_Master();    			//: I2C_Base((LPC_I2C_TypeDef*) LPC_I2C2_BASE){};
//
//
//};


class I2C_task : public scheduler_task
{
	public:
		I2C_task(uint8_t priority) : scheduler_task("I2C", 2000, priority){};
		bool run(void * p )
		{
			//printf("We are running task I2C master\n");
			while(1);
			return true;

		}
		bool init(void)
		{
			//I2C_Master i2cmaster;
			I2C2& i2c2m = I2C2::getInstance();
			i2c2m.slaveinit();

			return true;
		}


};

//
// Slave address is 0x38
//
class task1 : public scheduler_task
{
	public:

		typedef enum {
		   AccelQueueId,
		} sharedHandleId_t;

		/// Orientation type enumeration
		typedef enum {
			invalid,
			up,
			down,
			left,
			right,
		} direction_t;


		task1(uint8_t priority) : scheduler_task("task1", 2048, priority){

			ok = false;
			dir = invalid;
			QueueHandle_t q1 = xQueueCreate(1, sizeof(direction_t));
			addSharedObject(AccelQueueId, q1);
		};
		bool run(void * p )
		{

			uint8_t change = 0;

			while(1)
			{

				dir = invalid;
				ok &= I2C2::getInstance().readRegisters(0x39, 0x00, &change, 1);
				if (change & (1<<2))
				{
					ok &= I2C2::getInstance().readRegisters(0x39, 0x01, &xyzoutput[0], 6);
					if (xyzoutput[0] > 180  )
					{dir = left;}
					else if ( xyzoutput[0] < 60)
					{dir = right;}
					else if  (xyzoutput[4] > 180  )
					{dir = up;}
					else if (xyzoutput[4] < 60)
					{dir = down;}
					else dir = invalid;

					u0_dbg_printf("Sent: direction = %d \n", dir);
					xQueueSend(getSharedObject(AccelQueueId), &dir, portMAX_DELAY);
				}

//					u0_dbg_printf("Byte: Xmsb = %u \n"
//							"Byte: Xlsb  = %u \n"
//							"Byte: Ymsb = %u \n"
//							"Byte: Ylsb = %u \n"
//							"Byte: Zmsb = %u \n"
//							"Byte: Zlsb = %u \n", xyzoutput[0], xyzoutput[1], xyzoutput[2],
//												  xyzoutput[3], xyzoutput[4], xyzoutput[5]);

				vTaskDelay(1000);

			}
			return ok;
		}

		bool init(void)
		{


			for( int temp = 0; temp<7; temp++)
				{xyzoutput[temp] = 0;}

			ok = I2C2::getInstance().writeReg(0x38, 0x2A, 0x03); //used to set fast read and active symbol. writing to ctrl reg 1
			ok &= I2C2::getInstance().writeReg(0x38, 0x2C, (1<<5)); //  orientation interrupt wakes system
			ok &= I2C2::getInstance().writeReg(0x38, 0x2D, (1<<4)); // enabling orientation interrupt
			return ok;
		}

		uint8_t xyzoutput[7];
		bool ok;
		direction_t dir;

};

typedef enum {
	   IRDataQueueId,
	   AccelQueueId,
	   pcQueueId
	} sharedHandleId_t;

/// Orientation type enumeration
typedef enum {
	invalid,
	up,
	down,
	leftdir,
	rightdir,
} direction_t;

class task2 : public scheduler_task
{
		direction_t dir;
	public:
		task2(uint8_t priority) : scheduler_task("task2", 2000, priority){
			dir = invalid;
		};
		bool run(void * p )
		{
			dir = invalid;
			QueueHandle_t qid = getSharedObject(AccelQueueId);
			//printf("We are running task I2C master\n");
			while(1)

			{
				if( xQueueReceive(qid, &dir, portMAX_DELAY)  )
				{
					u0_dbg_printf("Receive: direction = %d \n", dir);
					//p1.1
					if(dir == leftdir)
					{
						LPC_GPIO1->FIOPIN |= (1<<1);  // set p1.0 high
						vTaskDelay(100);
						//printf("LED ON");

						LPC_GPIO1->FIOPIN |= (1<<0);  // set p1.0 high
									vTaskDelay(100);
					}
					if (dir == rightdir)
					{
						LPC_GPIO1->FIOPIN &= ~(1<<22);  // set p1.0 low
						vTaskDelay(100);
						//printf("LED OFF");

						LPC_GPIO1->FIOPIN &= ~(1<<0); //set p1.0 low
						vTaskDelay(100);
					}
				}
			}
			return true;
		}
		bool init(void)
		{

			LPC_PINCON->PINSEL2 &= ~(3<<0);  // GPIO port 1.0
			LPC_GPIO1->FIODIR |= (1<<0); 	// GPIO port 1.0 output ;  0 = input, 1 = output

			return true;
		}

};




///// watchdog ////
    // producer sets bit 1, consumer sets bit 2


    	const uint32_t prodtaskId = (1 << 0); // bit 1
    	const uint32_t constaskId = (1 << 1); // bit 2
    	const uint32_t watchdogBits = prodtaskId | constaskId;
/////////////////////




class producertask : public scheduler_task
{
    		EventGroupHandle_t wdhandle;
    		 QueueHandle_t pcQueueHandle  = 0;
	double average;
	public:
		producertask(uint8_t priority) : scheduler_task("producer", 4000, priority){
			average = 0;
			QueueHandle_t q1 = xQueueCreate(10, sizeof(average));
			addSharedObject(pcQueueId, q1);
		};
		bool run(void * p )
		{
			LPC_SC->PCONP |= (1<<12); // enables adc power before pdn bit
			LPC_SC->PCLKSEL0 |= (3 << 24 ); // set adc pclk to cclk/8
			LPC_PINCON->PINSEL1 |= (1<<18);  // GPIO port 0 pin 25 enabled with 01 for ADC

			//130000 = (sys_get_cpu_clock() / 8) / prescale;
			//prescale = (sys_get_cpu_clock() / 8) / 13000
			int prescale = (sys_get_cpu_clock() / 8) / 13000000;
			prescale +=1000;  // Trying to match prescale divisor so the adc clock is as close to 13MHz as possible


			LPC_ADC->ADCR  |= (1 << 21);  //  pdn opertional
			LPC_ADC->ADCR  |= (prescale << 8); //prescaler
			LPC_ADC->ADCR |= (1<<2);  //select ADc[2]
			LPC_ADC->ADCR |= (1 << 24); //   start conversion

				 average = 0;
				//xQueueHandle = 0;
				// xQueueCreate(getSharedObject(AccelQueueId), 10, sizeof(average));
				 //addSharedObject("pcQueueHandle", pcQueueHandle);
				while (1){
					int temp = 0;
					//double res = 0;
					int r = 0;
					average = 0; // zero average value each run
					for(temp = 0; temp  < 100; temp++) // loop 100 times
					{
								r = LPC_ADC->ADGDR;
								r = (r>>4) & 0x00000FFF;
								//res = r/ 0xFFF;
								// find the fraction of the adc value
								//u0_dbg_printf("Sent: average = %d \n", r);
								average = average + r;
					}

					average = average / 100;		// compute average
					//u0_dbg_printf("Sent: average = %f \n", average);
					xQueueSend(getSharedObject(AccelQueueId), &average, portMAX_DELAY);

					xEventGroupSetBits(wdhandle, prodtaskId);
					}


		}
		bool init(void)
		{
			return true;
		}

};


class consumertask : public scheduler_task
{
	private:

	QueueHandle_t pcQueueHandle  = 0;
	EventGroupHandle_t wdhandle;
	QueueHandle_t qid;
	public:
		consumertask(uint8_t priority) : scheduler_task("consumer", 4000, priority){
//			s = "";
//			counter = 0;
//			for(int temp = 0; temp < 10; temp++)
//				{averages[temp]=0;
//				time[temp] = 0;} //clearing 10 sets of averages and time slots
//
		qid = getSharedObject(pcQueueId);

		};
		bool run(void * p )
		{
			double averages [10];
			string s;
			int time [10];
			int counter;


					s = "";
					counter = 0;
					for(int temp = 0; temp < 10; temp++)
						{averages[temp]=0;
						time[temp] = 0;} //clearing 10 sets of averages and time slots

					//qid = getSharedObject(IRDataQueueId); // retrieve queue id handle
					Storage sdstorage;

					//LPC_GPIO1->FIODIR3 |= (1 << 1); // cs 1.25 pin
					//LPC_GPIO1->FIOCLR3 |= (1 << 1); // set pin low to cs the sd card

					while(1)
					{
					FILE *fd = fopen("1:sensor.txt", "a+");
					ostringstream os;
					for(int temp = 0; temp < 10; temp++)
						{averages[temp]=0;
						time[temp] = 0;}

					for( int temp = 0; temp < 10; temp++)
					{
						xQueueReceive(qid, &averages[temp], portMAX_DELAY);// portMAX_DELAY will cause the task to block indefinitely (without a timeout).
						counter+=100; // running counter in ms.
						time[temp] = counter;
					}
					for(int temp = 0; temp < 10; temp++)
								{
									os << averages[temp] << ", " << time[temp] << '\n';   // place averages, times and new lines in neat order
								}
					s = os.str(); // create a single string

					//string temps = "Hello \n";

					if(fd != NULL)
					{
						//u0_dbg_printf("appended sensor.txt file \n");
						fputs(s.c_str(), fd);
						fclose(fd);
					}
						//sdstorage.append("1:sensor.txt", &s, sizeof(s), 0 ); // append the string of data to a file on an SD card
						//cout << s;



					xEventGroupSetBits(wdhandle, constaskId);
					}
		}
		bool init(void)
		{			return true;
		}
};





class watchdogtask : public scheduler_task
{
	EventGroupHandle_t wdhandle;

public:
    	watchdogtask(uint8_t priority) : scheduler_task("watchdog", 2000, priority){
    		wdhandle = xEventGroupCreate();
    	}

bool run(void * p)
{
	while(1)
	{
		uint32_t res = xEventGroupWaitBits(wdhandle, watchdogBits, pdTRUE, pdTRUE, 2000);

		if (!res & prodtaskId)
		{
			FILE *fd = fopen("1:stuck.txt", "a+");
			fputs("Producer stopped working \n", fd);
			fclose(fd);
		}
		if(!res & constaskId)
		{
			FILE *fd = fopen("1:stuck.txt", "a+");
			fputs("Consumer stopped working \n", fd);
			fclose(fd);
		}
		vTaskDelay(1000);
	}
}

bool init(){return true;}

};





/**
 * gps task
 */
class gps_task : public scheduler_task
{

public:

	gps& GPS =  GPS.getInstance();

     gps_task(uint8_t priority) : scheduler_task("gps", 2000, priority)
		{ }
     bool run(void *p)
     {
    	 GPS.run();
    	 return true;
     }
     bool init(void)
     {

    	 GPS.init();
     	return true;
     }

};




/*class gps_task : public scheduler_task
{

    public:
		Uart2& uart2 = Uart2::getInstance();
		char msg [64];
		int m_ptr;
		bool swversion;
        gps_task(uint8_t priority) : scheduler_task("gps", 2000, priority)
		{
        	m_ptr = 0;
        	for (int temp = 0; temp < 64; temp ++)
        		msg[temp] = 0;
        	swversion = true;
		}
        bool run(void *p)
        {
        	cout<< "GPS: ";

        	if (swversion == true )
        	{
        		uart2.putChar(0xA0);// start sequence
        		uart2.putChar(0xA1);
        		uart2.putChar(0x00);// payload length 2 bytes
        		uart2.putChar(0x02);
        		uart2.putChar(0x02);// message body
        		uart2.putChar(0x01); // system code
        		uart2.putChar(0x02);	//Check sum
        		uart2.putChar(0x0D);	//end sequence
        		uart2.putChar(0x0A);

        		for (int temp = 0; temp == 21; temp ++)
        			{uart2.getChar(&msg[temp]);}

        		if (msg[4] == 0x80)
        		{
        			cout << "sw version successful" << endl;
        		}
        	swversion = false;
        	}
        	return true;
        }
        bool init(void)
        {

			bool success = uart2.init(9600, 64, 64); // 9600 baud, 64 bit rx and tx
			uart2.setReady(true);
        	return success;
        }
    private:

};*/


//class EINT_task : public scheduler_task
//{
//	public:
//		EINT_task(uint8_t priority) : scheduler_task("EINT", 2000, priority){};
//		bool run(void * p )
//		{
//			printf("We are running\n");
//			while(1);
//
//			return true;
//
//		}
//		bool init(void)
//		{
//
//
//
//			//cout << "Please choose which Port you'd like to activate\n" ;
//			//string s;
//			//cin >> s;
//			//int temp = atoi(s.c_str());
//			//cout << "Please choose which pin you'd like to activate\n" ;
//			//string t;
//			//cin >> t;
//			//int temp2 = atoi(t.c_str());
//
//			// Pin select GPIO
//			//Enable Interrupts
//			// port 2 pin 0 and 1
//			LPC_PINCON->PINSEL4 &= ~(3 << 0); // set to GPIO mode
//			LPC_GPIO2->FIODIR  &= ~(3 << 0);  // set as input
//
//
//
//			NVIC_EnableIRQ(EINT3_IRQn);
//			LPC_GPIOINT->IO2IntEnR &= (1<<0); //Enable interrupt to detect rising edge
//
//			printf("We are done initializing\n");
//
//			//setupInt(temp, temp2, 0);
//
//
//
////			while (temp >= 0 && temp <= 3)
////			{
////				switch ( temp )
////				{
////					case 0:
////
////						break;
////					case 1:
////						break;
////					case 2:
////						break;
////					default:
////						cout << "Please choose which Port you'd like to activate\n";
////						cin >> s;
////						temp = atoi(s.c_str());
////					break;
////				}
////			}
//			return true;
//		}
//
//		void setupInt(int port, int pin, int activelevel)
//		{
////							switch ( port )
////							{
////								case 0:
////									if (pin <16)
////											//LPC_PINCON->PINSEL0 |= (0 << pin);
////											LPC_
////									else
////											//LPC_PINCON->PINSEL1 |= ();
////									break;
////								case 1:
////									break;
////								case 2:
////									break;
////								default:
////									cout << "Please choose which Port you'd like to activate\n";
////									cin >> s;
////									temp = atoi(s.c_str());
////								break;
////							}
//		}
//};


#endif /* TASKS_HPP_ */
