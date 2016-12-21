/*
 * gps.hpp
 *
 *  Created on: Nov 17, 2016
 *      Author: angel
 */

#ifndef L2_DRIVERS_GPS_HPP_
#define L2_DRIVERS_GPS_HPP_

#include "singleton_template.hpp"
#include "uart2.hpp"
#include "queue.h"


class gps :  public SingletonTemplate<gps>
{
	Uart2& uart2 = Uart2::getInstance();
	bool success;
	bool nmeachange;
	char msg[124];
	bool gpsfixCS;

    public:
        /// Initializes I2C2 at the given @param speedInKhz
        bool init();
        bool run();
    private:
        gps(); ///< Private constructor for this singleton class
        void getGPS();
        void cleanGPS(char*buffer, int index);
        bool checksumGPSfix();
        friend class SingletonTemplate<gps>;  ///< Friend class used for Singleton Template

};


#endif /* L2_DRIVERS_GPS_HPP_ */
