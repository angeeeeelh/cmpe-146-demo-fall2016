/*
 * gps.cpp
 *
 *  Created on: Nov 17, 2016
 *      Author: angel
 */

#include "gps.hpp"
//#include "uart2.hpp"
#include <stdio.h>
#include <printf_lib.h>
#include <string>
#include <cstring>
#include "queue.h"
#include <stdio.h>
using namespace std;

bool gps::init()
{

	success = false;
	success = uart2.init(9600, 124, 124); // 9600 baud, 64 bit rx and tx

	nmeachange = true;
	return success;


}

bool gps::checksumGPSfix()
{
	  string field = string(msg);
	  gpsfixCS = false;
	  char CSvalue = 0;
	  char comp = 0;

	  int starPos = field.find('*');
	  //u0_dbg_printf("Is this star? %c\n", field.at(starPos));
	  //u0_dbg_printf("Is this our string to checksum? %s\n", field.substr(1, starPos).c_str() ) ;
	  for(int sentpos = 1 ; sentpos < starPos; sentpos++)
	  {
		  CSvalue = CSvalue ^ field.at(sentpos);
	  }

	  comp = field.substr(starPos, field.length() - 1).at(0) ;
	  //u0_dbg_printf("Orig: %x vs Cval: %x\n", comp, CSvalue);
	  if (comp == CSvalue)
		  gpsfixCS = true;
	  else
		  gpsfixCS = false;
}

void gps::getGPS()
{

	  char field[20];
	  cleanGPS(field, 0);
	  if (strcmp(field, "$GPRMC") == 0 )
	  {
		  u0_dbg_printf("Lat: ");
	    cleanGPS(field, 3);  // number
	    u0_dbg_printf("%s\n", field);
	    cleanGPS(field, 4); // N/S
	    u0_dbg_printf("%s\n", field);

	    u0_dbg_printf(" Long: ");
	    cleanGPS(field, 5);  // number
	    u0_dbg_printf("%s\n", field);
	    cleanGPS(field, 6);  // E/W
	    u0_dbg_printf("%s\n", field);	  }

}

void gps::cleanGPS(char* buffer, int index)
{
	  int sentencePos = 0;
	  int fieldPos = 0;
	  int commaCount = 0;
	  while (sentencePos < sizeof(msg))
	  {
	    if (msg[sentencePos] == ',')
	    {
	      commaCount ++;
	      sentencePos ++;
	    }
	    if (commaCount == index)
	    {
	      buffer[fieldPos] = msg[sentencePos];
	      fieldPos ++;
	    }
	    sentencePos ++;
	  }
	  buffer[fieldPos] = '\0';

}
bool gps::run()
{

	//A0 A1 00 09 08 01 01 01 00 01 00 00 00 08 0D 0A
//	if(nmeachange == true)
//	{
	vTaskDelay(100);
	uart2.putChar(0xA0);
	vTaskDelay(100);
	uart2.putChar(0xA1);
	vTaskDelay(100);
	uart2.putChar(0x00);
	vTaskDelay(100);
	uart2.putChar(0x09);
	vTaskDelay(100);
	uart2.putChar(0x08);
	vTaskDelay(100);
	uart2.putChar(0x00);
	vTaskDelay(100);
	uart2.putChar(0x00);
	vTaskDelay(100);
	uart2.putChar(0x00);
	vTaskDelay(100);
	uart2.putChar(0x00);
	vTaskDelay(100);
	uart2.putChar(0x01);
	vTaskDelay(100);
	uart2.putChar(0x00);
	vTaskDelay(100);
	uart2.putChar(0x00);
	vTaskDelay(100);
	uart2.putChar(0x00);
	vTaskDelay(100);
	uart2.putChar(0x09);
	vTaskDelay(100);
	uart2.putChar(0x0D);
	vTaskDelay(100);
	uart2.putChar(0x0A);
	vTaskDelay(100);
	nmeachange = false;
	//}
	//A0 A1 00 03 37 01 00 36 0D 0A

	vTaskDelay(100);
	uart2.putChar(0xA0);
	vTaskDelay(100);
	uart2.putChar(0xA1);
	vTaskDelay(100);
	uart2.putChar(0x00);
	vTaskDelay(100);
	uart2.putChar(0x03);
	vTaskDelay(100);
	uart2.putChar(0x37);
	vTaskDelay(100);
	uart2.putChar(0x01);
	vTaskDelay(100);
	uart2.putChar(0x00);
	vTaskDelay(100);
	uart2.putChar(0x36);
	vTaskDelay(100);
	uart2.putChar(0x0D);
	vTaskDelay(100);
	uart2.putChar(0x0A);
	vTaskDelay(100);

	//vTaskDelay(250);

	int temp = 0;
	for(temp; temp <124; temp++)
		msg[temp] = 0;
	temp = 0;
	//vTaskDelay(100);
	while(uart2.getChar(&msg[temp], 1000) && temp < 124 )
		temp++;


	//u0_dbg_printf("before first clean:\n%s\n", msg);
	//u0_dbg_printf("before first clean:\n%s\n", msg);

	string s = string(msg);
	size_t dollarsign, endofmsg = string::npos;

	dollarsign = s.find('$');

	endofmsg = s.find('$', dollarsign);

	if(dollarsign != string::npos && endofmsg !=  string::npos)
		{
			string news = s.substr(dollarsign, endofmsg);

			for(int t = 0; t < 124 && t < news.length() ; t++)
			{
				msg[t] = news.at(t);
			}
			//u0_dbg_printf("after first clean:\n%s\n", msg);

			if (checksumGPSfix() == true)
					getGPS();
		}


	return true;
}
gps::gps()
{

}


