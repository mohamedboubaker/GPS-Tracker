/** @file gps.h
*  @brief GPS functions' prototypes and definition of related parameters
*
*  @author Mohamed Boubaker
*  
*/
#ifndef GPS_H
#define GPS_H

#include "sim808.h"



/**
 * @brief enables the GPS functionality of the SIM808 module
 * @return 1 if the GPS is successfully enabled, 0 otherwise
 */
uint8_t enable_gps();



/**
* @brief checks if the module has a GPS fix, if yes, it querries the GPS module for the current position. 
* The format of the output is longitude,latitude Example: 3937.656010,1406.059400
* @param coordinates is an array that will store the GPS position.
* @return 1 if the GPS position is calculated correctly, 0 if the module doesn't have a fix or an error occurs.
*/
uint8_t get_gps_location(char* position);



/** 
 * @brief returns the speed relative to ground.
 * @param speed is used to store speed. 
 * @return always returns 1.
 */
uint8_t sim_gps_get_speed(char * speed);
 
 
 
 /**
 * @brief disables the GPS functionality of the SIM808 module and the active antenna power supply
 * @return 1 if the GPS is successfully disabled, 0 otherwise
 */
uint8_t sim_gps_disable();

#endif
