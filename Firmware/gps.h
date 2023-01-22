/** @file network_functions.c
*  @brief This file include the declaration of functions related to GPS 
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
uint8_t sim_gps_enable();



/** 
 * @brief returns the GPS coordinates. The format of the output is longitude,latitude Example: 3937.656010,1406.059400
 * @param position is used to store the coordinates. 
 * @return always 1.
 */
uint8_t sim_gps_get_location(char* position);



/** 
 * @brief returns the speed 
 * @param char * speed is used to store speed. 
 * @return always returns 1.
 */
uint8_t sim_gps_get_speed(char * speed);
 
 
 
 /**
 * @brief disables the GPS functionality of the SIM808 module and the active antenna power supply
 * @return 1 if the GPS is successfully disabled, 0 otherwise
 */
uint8_t sim_gps_disable();

#endif
