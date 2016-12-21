/*
 * uarttest.h
 *
 *  Created on: Dec 19, 2016
 *      Author: angel
 */

#ifndef L2_DRIVERS_UARTTEST_H_
#define L2_DRIVERS_UARTTEST_H_

#include <string>
#include "FreeRTOS.h"
#include "queue.h"

using namespace std;

class uarttest
{
	public:
	uarttest();
	bool uart_getstring(string* b);
	bool uart_putstring(string* b);

	private:
	QueueHandle_t rx_gpsHandle;
	QueueHandle_t tx_gpsHandle;

};


#endif /* L2_DRIVERS_UARTTEST_H_ */
