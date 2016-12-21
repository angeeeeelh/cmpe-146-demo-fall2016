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

#include <string.h>         // memcpy

#include "i2c_base.hpp"
#include "lpc_sys.h"
#include <stdint.h>
#include <stdio.h>
#include <printf_lib.h>

/**
 * Instead of using a dedicated variable for read vs. write, we just use the LSB of
 * the user address to indicate read or write mode.
 */
#define I2C_SET_READ_MODE(addr)     (addr |= 1)     ///< Set the LSB to indicate read-mode
#define I2C_SET_WRITE_MODE(addr)    (addr &= 0xFE)  ///< Reset the LSB to indicate write-mode
#define I2C_READ_MODE(addr)         (addr & 1)      ///< Read address is ODD
#define I2C_WRITE_ADDR(addr)        (addr & 0xFE)   ///< Write address is EVEN
#define I2C_READ_ADDR(addr)         (addr | 1)      ///< Read address is ODD


void I2C_Base::handleInterrupt()
{
    /* If transfer finished (not busy), then give the signal */
    if (busy != i2cStateMachine()) {
        long higherPriorityTaskWaiting = 0;
        xSemaphoreGiveFromISR(mTransferCompleteSignal, &higherPriorityTaskWaiting);
        portEND_SWITCHING_ISR(higherPriorityTaskWaiting);
    }
}

uint8_t I2C_Base::readReg(uint8_t deviceAddress, uint8_t registerAddress)
{
    uint8_t byte = 0;
    readRegisters(deviceAddress, registerAddress, &byte, 1);
    return byte;
}

bool I2C_Base::readRegisters(uint8_t deviceAddress, uint8_t firstReg, uint8_t* pData, uint32_t bytesToRead)
{
    I2C_SET_READ_MODE(deviceAddress);
    return transfer(deviceAddress, firstReg, pData, bytesToRead);
}

bool I2C_Base::writeReg(uint8_t deviceAddress, uint8_t registerAddress, uint8_t value)
{
    return writeRegisters(deviceAddress, registerAddress, &value, 1);
}

bool I2C_Base::writeRegisters(uint8_t deviceAddress, uint8_t firstReg, uint8_t* pData, uint32_t bytesToWrite)
{
    I2C_SET_WRITE_MODE(deviceAddress);
    return transfer(deviceAddress, firstReg, pData, bytesToWrite);
}

bool I2C_Base::transfer(uint8_t deviceAddress, uint8_t firstReg, uint8_t* pData, uint32_t transferSize)
{
    bool status = false;
    if(mDisableOperation || !pData)
    {
        return status;
    }

    // If scheduler not running, perform polling transaction
    if(taskSCHEDULER_RUNNING != xTaskGetSchedulerState())
    {
        i2cKickOffTransfer(deviceAddress, firstReg, pData, transferSize);

        // Wait for transfer to finish
        const uint64_t timeout = sys_get_uptime_ms() + I2C_TIMEOUT_MS;
        while (!xSemaphoreTake(mTransferCompleteSignal, 0)) {
            if (sys_get_uptime_ms() > timeout) {
                break;
            }
        }

        status = (0 == mTransaction.error);
    }
    else if (xSemaphoreTake(mI2CMutex, OS_MS(I2C_TIMEOUT_MS)))
    {
        // Clear potential stale signal and start the transfer
        xSemaphoreTake(mTransferCompleteSignal, 0);
        i2cKickOffTransfer(deviceAddress, firstReg, pData, transferSize);

        // Wait for transfer to finish and copy the data if it was read mode
        if (xSemaphoreTake(mTransferCompleteSignal, OS_MS(I2C_TIMEOUT_MS))) {
            status = (0 == mTransaction.error);
        }

        xSemaphoreGive(mI2CMutex);
    }

    return status;
}

bool I2C_Base::checkDeviceResponse(uint8_t deviceAddress)
{
    uint8_t dummyReg = 0;
    uint8_t notUsed = 0;

    // The I2C State machine will not continue after 1st state when length is set to 0
    uint32_t lenZeroToTestDeviceReady = 0;

    return readRegisters(deviceAddress, dummyReg, &notUsed, lenZeroToTestDeviceReady);
}

I2C_Base::I2C_Base(LPC_I2C_TypeDef* pI2CBaseAddr) :
        mpI2CRegs(pI2CBaseAddr), mDisableOperation(false)
{

	slavePtr = slaveDataBuffer;
	//u0_dbg_printf("I2Cbase constructor");

	slaveDataCounter = 0;// because compiler insists initializing in this constructor

    mI2CMutex = xSemaphoreCreateMutex();
    mTransferCompleteSignal = xSemaphoreCreateBinary();

    /// Binary semaphore needs to be taken after creating it
    xSemaphoreTake(mTransferCompleteSignal, 0);

    if((unsigned int)mpI2CRegs == LPC_I2C0_BASE)
    {
        mIRQ = I2C0_IRQn;
    }
    else if((unsigned int)mpI2CRegs == LPC_I2C1_BASE)
    {
        mIRQ = I2C1_IRQn;
    }
    else if((unsigned int)mpI2CRegs == LPC_I2C2_BASE)
    {
        mIRQ = I2C2_IRQn;
    }
    else {
        mIRQ = (IRQn_Type)99; // Using invalid IRQ on purpose
    }
}

bool I2C_Base::init(uint32_t pclk, uint32_t busRateInKhz)
{
	//for(int temp =0; temp <256; temp++)


	slaveDataCounter = 0;
    // Power on I2C
   switch(mIRQ)
   {
        case I2C0_IRQn: lpc_pconp(pconp_i2c0, true);  break;
        case I2C1_IRQn: lpc_pconp(pconp_i2c1, true);  break;
        case I2C2_IRQn: lpc_pconp(pconp_i2c2, true);  break;
        default: return false;
    }

    mpI2CRegs->I2CONCLR = 0x6C;           // Clear ALL I2C Flags
    /**
     * Per I2C high speed mode:
     * HS mode master devices generate a serial clock signal with a HIGH to LOW ratio of 1 to 2.
     * So to be able to optimize speed, we use different duty cycle for high/low
     *
     * Compute the I2C clock dividers.
     * The LOW period can be longer than the HIGH period because the rise time
     * of SDA/SCL is an RC curve, whereas the fall time is a sharper curve.
     */
    const uint32_t percent_high = 40;
    const uint32_t percent_low = (100 - percent_high);
    const uint32_t freq_hz = (busRateInKhz > 1000) ? (100 * 1000) : (busRateInKhz * 1000);
    const uint32_t half_clock_divider = (pclk / freq_hz) / 2;
    mpI2CRegs->I2SCLH = (half_clock_divider * percent_high) / 100;
    mpI2CRegs->I2SCLL = (half_clock_divider * percent_low ) / 100;

    // Set I2C slave address and enable I2C
    mpI2CRegs->I2ADR0 = 0;
    mpI2CRegs->I2ADR1 = 0;
    mpI2CRegs->I2ADR2 = 0;
    mpI2CRegs->I2ADR3 = 0;

    // Enable I2C and the interrupt for it
    mpI2CRegs->I2CONSET = 0x40;
    NVIC_EnableIRQ(mIRQ);



    return true;
}

//Master initializes slave device
bool I2C_Base::slaveinit(void)
{

	//load I2ADR registers and I2MASK registers with values to configure the own
	//Slave Address, enable General Call recognition if needed.
	mpI2CRegs->I2ADR2    =  0x80; //addr;//LSB = 0
	mpI2CRegs->I2MASK2   =  0x80;//addr;

    NVIC_EnableIRQ(mIRQ);//enable interrupts
	mpI2CRegs->I2CONSET  = 0x44; //sets I2EN and AA bits

	return true;
}

/// Private ///

void I2C_Base::i2cKickOffTransfer(uint8_t devAddr, uint8_t regStart, uint8_t* pBytes, uint32_t len)
{
    mTransaction.error     = 0;
    mTransaction.slaveAddr = devAddr;
    mTransaction.firstReg  = regStart;
    mTransaction.trxSize   = len;
    mTransaction.pMasterData   = pBytes;

    // Send START, I2C State Machine will finish the rest.
    mpI2CRegs->I2CONSET = 0x20;
}

/*
 * I2CONSET bits
 * 0x04 AA
 * 0x08 SI
 * 0x10 STOP
 * 0x20 START
 * 0x40 ENABLE
 *
 * I2CONCLR bits
 * 0x04 AA
 * 0x08 SI
 * 0x20 START
 * 0x40 ENABLE
 */
I2C_Base::mStateMachineStatus_t I2C_Base::i2cStateMachine()
{


	//Define state machine states corresponding to
	//modes of operation
    enum {
        // General states :
        busError        = 0x00,
        start           = 0x08,
        repeatStart     = 0x10,
        arbitrationLost = 0x38,

        // Master Transmitter States:
        slaveAddressAcked  = 0x18,
        slaveAddressNacked = 0x20,
        dataAckedBySlave   = 0x28,
        dataNackedBySlave  = 0x30,

        // Master Receiver States:
        readAckedBySlave      = 0x40,
        readModeNackedBySlave = 0x48,
        dataAvailableAckSent  = 0x50,
        dataAvailableNackSent = 0x58,


		/******* LAB 5 CODE ******/


		//Slave Transmitter States:
		slaveAckSLARfromMaster = 0xA8,
        masterAckDataFromSalve = 0xB8,
        masterNacksToSlave     = 0xC0,

		//Slave Receiver States:
		slaveAckSLAWfromMaster = 0x60,
        slaveAckDatafromMaster = 0x80,
		slavePorSfromMaster = 0xA0

    };

    //The status of I2C is returned from the I2C function that handles state machine
    mStateMachineStatus_t state = busy;
   //mTransaction === The I2C Input Output frame that contains I2C transaction information
   //mTransaction  MASTER TRANSMIT BUFFER

    /*
     ***********************************************************************************************************
     * Write-mode state transition :
     * start --> slaveAddressAcked --> dataAckedBySlave --> ... (dataAckedBySlave) --> (stop)
     *
     * Read-mode state transition :
     * start --> slaveAddressAcked --> dataAcked --> repeatStart --> readAckedBySlave
     *  For 2+ bytes:  dataAvailableAckSent --> ... (dataAvailableAckSent) --> dataAvailableNackSent --> (stop)
     *  For 1  byte :  dataAvailableNackSent --> (stop)
     ***********************************************************************************************************
     */

    /* #defines instead of inline functions :( */
    #define clearSIFlag()       mpI2CRegs->I2CONCLR = (1<<3)
    #define setSTARTFlag()      mpI2CRegs->I2CONSET = (1<<5)
    #define clearSTARTFlag()    mpI2CRegs->I2CONCLR = (1<<5)
    #define setAckFlag()        mpI2CRegs->I2CONSET = (1<<2)
    #define setNackFlag()       mpI2CRegs->I2CONCLR = (1<<2)
    #define setStop()           clearSTARTFlag();                           \
                                mpI2CRegs->I2CONSET = (1<<4);               \
                                clearSIFlag();                              \
                                while((mpI2CRegs->I2CONSET&(1<<4)));        \
                                if(I2C_READ_MODE(mTransaction.slaveAddr))   \
                                    state = readComplete;                   \
                                else                                        \
                                    state = writeComplete;
    //mpI2CRegs -- Pointer to I2C memory map
    switch (mpI2CRegs->I2STAT)
    {
        case start:
        	//u0_dbg_printf("In START.\n");

            mpI2CRegs->I2DAT = I2C_WRITE_ADDR(mTransaction.slaveAddr);
            clearSIFlag();
            break;

        case repeatStart:
        	//u0_dbg_printf("In repeated START.\n");
        	//u0_dbg_printf("Read address is : %x\n",  I2C_READ_ADDR(mTransaction.slaveAddr));
            mpI2CRegs->I2DAT = I2C_READ_ADDR(mTransaction.slaveAddr);
            clearSIFlag();
            break;

        case slaveAddressAcked:
        	//u0_dbg_printf("Slave Address ACK.\n");

            clearSTARTFlag();
            // No data to transfer, this is used just to test if the slave responds
            if(0 == mTransaction.trxSize) {
                setStop();
            }
            else {
                mpI2CRegs->I2DAT = mTransaction.firstReg; //LOAD I2DAT with first byte from Master Transmit Buffer
                clearSIFlag();
            }
            break;

        case dataAckedBySlave:
        	//u0_dbg_printf("Slave ACK data.\n");

            if (I2C_READ_MODE(mTransaction.slaveAddr)) {
                setSTARTFlag();// Send Repeat-start for read-mode
                clearSIFlag(); //
            }
            else {
                if(0 == mTransaction.trxSize) {
                    setStop();
                    //
                }
                else {
                    mpI2CRegs->I2DAT = *(mTransaction.pMasterData);
                    ++mTransaction.pMasterData;
                    --mTransaction.trxSize;
                    clearSIFlag();
                }
            }
            break;

        /* In this state, we are about to initiate the transfer of data from slave to us
         * so we are just setting the ACK or NACK that we'll do AFTER the byte is received.
         */
        case readAckedBySlave:
        	//u0_dbg_printf("Slave read ACK.\n");

            clearSTARTFlag();
            if(mTransaction.trxSize > 1) {
                setAckFlag();  // 1+ bytes: Send ACK to receive a byte and transition to dataAvailableAckSent
            }
            else {
                setNackFlag();  //  1 byte : NACK next byte to go to dataAvailableNackSent for 1-byte read.
            }
            clearSIFlag();
            break;
        case dataAvailableAckSent:
        	//u0_dbg_printf("Data Available ACK sent.\n");

            *mTransaction.pMasterData = mpI2CRegs->I2DAT;
            ++mTransaction.pMasterData;
            --mTransaction.trxSize;

            if(1 == mTransaction.trxSize) { // Only 1 more byte remaining
                setNackFlag();// NACK next byte --> Next state: dataAvailableNackSent
            }
            else {
                setAckFlag(); // ACK next byte --> Next state: dataAvailableAckSent(back to this state)
            }

            clearSIFlag();
            break;

        case dataAvailableNackSent: // Read last-byte from Slave
        	//u0_dbg_printf("Data Available NACK sent.\n");

            *mTransaction.pMasterData = mpI2CRegs->I2DAT;
            setStop();
            //print slave data buffer
            //u0_dbg_printf("This is the read byte %x \n", *mTransaction.pMasterData);

            break;

        case arbitrationLost:
        	//u0_dbg_printf("Arbitration lost.\n");

            // We should not issue stop() in this condition, but we still need to end our  transaction.
            state = I2C_READ_MODE(mTransaction.slaveAddr) ? readComplete : writeComplete;
            mTransaction.error = mpI2CRegs->I2STAT;
            break;

		/***** ADD SLAVE TX AND RX cases ****/
		/*******       LAB 5 CODE      ******/
		//Slave Transmitter States:


           ///////////////SLave state

            // complete
		case slaveAckSLARfromMaster: //0xA8,
			//u0_dbg_printf("In state 0xA8.\n");


//			1. Load I2DAT from Slave Transmit buffer with first data byte.

//			2. Write 0x04 to I2CONSET to set the AA bit.
//			3. Write 0x08 to I2CONCLR to clear the SI flag.
//			4. Set up Slave Transmit mode data buffer.
//			5. Increment Slave Transmit buffer pointer.
//			6. Exit

				slavePtr--;
				slavePtr--;
			 //1. Load I2DAT from Slave Transmit buffer with first data byte.
			  u0_dbg_printf("trasmit buffer values at slave ptr: %x \n and ptr addr: %d\n", *slavePtr, slavePtr);
			  mpI2CRegs->I2DAT = *slavePtr; //should this not be register#??
			 //2. Write 0x04 to I2CONSET to set the AA bit.
			  setAckFlag();
			 //3. Write 0x08 to I2CONCLR to clear the SI flag.
			  clearSIFlag();
			 //4. Set up Slave Transmit mode data buffer. //?????
//        	  for(int temp = 0; temp <256; temp++)
//        	       {slaveDataBuffer[temp] = 0;}
			//slaveDataBuffer[3]= 0x22;//77908810;
			 //5. Increment Slave Transmit buffer pointer.
			 slavePtr++;
			 //6. Exit
			//vTaskDelay(1000);
			break;

			// complete
		case masterAckDataFromSalve: //0xB8,
			//u0_dbg_printf("In state 0xB8.\n");
		//Data has been transmitted, ACK has been received.
		//Data will be transmitted, ACK bit will be received.

	//			1. Load I2DAT from Slave Transmit buffer with data byte.
	//			2. Write 0x04 to I2CONSET to set the AA bit.
	//			3. Write 0x08 to I2CONCLR to clear the SI flag.
	//			4. Increment Slave Transmit buffer pointer.
	//			5. Exit

			//1. Load I2DAT from Slave Transmit buffer with data byte.
			mpI2CRegs->I2DAT = *slavePtr;//???
			//2. Write 0x04 to I2CONSET to set the AA bit.
			setAckFlag();
			//3. Write 0x08 to I2CONCLR to clear the SI flag.
			clearSIFlag();
			//4. Increment Slave Transmit buffer pointer.
			slavePtr++;
			//5. Exit
			//vTaskDelay(1000);
			break;


				// complete
		case masterNacksToSlave:     //0xC0,
			//u0_dbg_printf("In state 0xC0.\n");


			//			Data has been transmitted, NOT ACK has been received. Not addressed Slave mode is
			//			entered.
			//			1. Write 0x04 to I2CONSET to set the AA bit.
			//			2. Write 0x08 to I2CONCLR to clear the SI flag.
			//			3. Exit.

			//vTaskDelay(1000);

		//1. Write 0x04 to I2CONSET to set the AA bit.
			setAckFlag();
		//2. Write 0x08 to I2CONCLR to clear the SI flag.
			clearSIFlag();
		//3. Exit.
			//vTaskDelay(1000);
			break;


			///////////Slave Receiver States:

			//complete
		case slaveAckSLAWfromMaster: //0x60,
			//u0_dbg_printf("In state 0x60.\n");

		//Own Slave Address + Write has been received, ACK has been returned.
		//Data will be received and ACK returned

//			1. Write 0x04 to I2CONSET to set the AA bit.
//			2. Write 0x08 to I2CONCLR to clear the SI flag.
//			3. Set up Slave Receive mode data buffer.
//			4. Initialize Slave data counter.
//			5. Exit

			//1. Write 0x04 to I2CONSET to set the AA bit.
			setAckFlag();
			//2. Write 0x08 to I2CONCLR to clear the SI flag.
			clearSIFlag();
			//3. Set up Slave Receive mode data buffer.///???
			  for(int temp = 0; temp <256; temp++)
				  {slaveDataBuffer[temp] = 0;}
			  slaveDataBuffer[2] = 0x22;
			  //slaveDataBuffer
			//4. Initialize Slave data counter.
			slaveDataCounter = 1;
			slavePtr = slaveDataBuffer;
			//5. Exit
			break;


		case slaveAckDatafromMaster: //0x80,
			//u0_dbg_printf("In state 0x80\n");
			//Previously addressed with own Slave Address.
			//Data has been received and ACK has been returned. Additional data will be read.

//			1. Read data byte from I2DAT into the Slave Receive buffer.
//			2. Decrement the Slave data counter, skip to step 5 if not the last data byte.
//			3. Write 0x0C to I2CONCLR to clear the SI flag and the AA bit.
//			4. Exit.
//			5. Write 0x04 to I2CONSET to set the AA bit.
//			6. Write 0x08 to I2CONCLR to clear the SI flag.
//			7. Increment Slave Receive buffer pointer.
//			8. Exit

			//1. Read data byte from I2DAT into the Slave Receive buffer.
			*slavePtr  = mpI2CRegs->I2DAT;  // slavedatabuffer[0] for first round
			//u0_dbg_printf("Test hex number: %x \n SLAVE DATA BUFFER: %x \n Slaveptr addr: %d \n", 8, *slavePtr, slavePtr);

			//2. Decrement the Slave data counter, skip to step 5 if not the last data byte.

			if(slaveDataCounter == 1)
				slavePtr += slaveDataBuffer[0];

			slaveDataCounter--;

			if(slaveDataCounter != 1 )//??
			{
				//5. Write 0x04 to I2CONSET to set the AA bit.
				setAckFlag();
				//6. Write 0x08 to I2CONCLR to clear the SI flag.
				clearSIFlag();
				//7. Increment Slave Receive buffer pointer.
				slavePtr++;
			}
			else
			{
				//3. Write 0x0C to I2CONCLR to clear the SI flag and the AA bit.
				mpI2CRegs->I2CONCLR = 0x0C;
			}
			//4. Exit. //8. Exit
			break;

		case slavePorSfromMaster:    // 0xA0
			//u0_dbg_printf("In state 0xA0\n");
//			A STOP condition or repeated START has been received, while still addressed as a
//			Slave. Data will not be saved. Not addressed Slave mode is entered.
//			1. Write 0x04 to I2CONSET to set the AA bit.
//			2. Write 0x08 to I2CONCLR to clear the SI flag.
//			3. Exit


//			1. Write 0x04 to I2CONSET to set the AA bit.
			setAckFlag();
//			2. Write 0x08 to I2CONCLR to clear the SI flag.
			clearSIFlag();
//			3. Exit
			break;


        case slaveAddressNacked:    // no break
        	u0_dbg_printf("Slave Address Nacked.\n");
        	break;

        case dataNackedBySlave:     // no break
        	u0_dbg_printf("Data NACKED by slave.\n");
        	break;

        case readModeNackedBySlave: // no break
        	u0_dbg_printf("Read mode NACKED by Slave.\n");
        	break;
        case busError:              // no breaK
        	u0_dbg_printf("Bus error.\n");
        	break;

        default:
            mTransaction.error = mpI2CRegs->I2STAT;
            setStop();
            break;
    }


    return state;
}
