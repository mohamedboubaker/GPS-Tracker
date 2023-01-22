/** @file gps.c
*  @brief This file contains GPS function implementations
*
*  @author Mohamed Boubaker
*  
*/
#include <string.h>
#include <stdio.h>
#include "gps.h"


/*******************************************************/
/*                     GPS functions                   */
/*******************************************************/

/**
 * @brief sim_gps_enable() enables the GPS functionality of the SIM808 module.
 * @return 1 if the GPS is successfully enabled, 0 otherwise
 */
uint8_t sim_gps_enable(){
	const char gps_power_on_cmd[]= "AT+CGPSPWR=1\r";
	const char gps_set_mode_cold_cmd[]= "AT+CGPSRST=0\r";
	uint8_t power_on_status=0;
	uint8_t set_mode_status=0;

	power_on_status=send_cmd(gps_power_on_cmd,RX_WAIT); 
	set_mode_status=send_cmd(gps_set_mode_cold_cmd,RX_WAIT); 

	return (power_on_status && set_mode_status);
}

/**
* @brief sim_gps_get_location() checks if the module has a GPS fix and querries the GPS module for the current position.
* The modules reply inculde: 
* -GPS time
* -latitue and longitute in DMM.MM (Degrees Minutes.Minutes)
* -the prefered format used on the server is the decimal degrees dd. The transformation dd = d + mm.mm/60 
* needs to be done on the server
* @param char * coordinates is an array that will store the GPS position
* @return 1 if the GPS position is calculated correctly, 0 if the module doesn't have a fix or an error occurs.
*/
uint8_t sim_gps_get_location(char * coordinates){

	/* GPS Status can be:
	 * "Location Unknown" if GPS is not enabled
	 * "Location not Fix" 
	 * "Location 2D Fix"
	 * "Location 3D Fix"
	 */
	static const char gps_get_status_cmd[]= "AT+CGPSSTATUS?\r";	
	const char gps_get_location_cmd[]= "AT+CGPSINF=0\r";
	char local_rx_buffer[RX_BUFFER_LENGTH];
	uint8_t err_status=0;


	/*Check GPS Fix status*/
	sim_get_cmd_reply(gps_get_status_cmd,local_rx_buffer,RX_WAIT);

	if (strstr(local_rx_buffer,"Location 3D Fix")!=NULL){
		
		/*clear local buffer*/
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);

		err_status=sim_get_cmd_reply(gps_get_location_cmd,local_rx_buffer,RX_WAIT);
		
		/* Example reply 
		* AT+CGPSINF=0 +CGPSINF: 0,4927.656000,1106.059700,319.200000,20220816200132.000,0,12,1.592720,351
		* Actuall coordinates start at charachter 27
		*/

		/* Extract the coordinates from the cmd reply and copy only 
		*	the coordinates into function parameter char * coordinates
		*/
		memcpy(local_rx_buffer+27,coordinates,GPS_COORDINATES_LENGTH);
		
		return err_status;
	}
	else
		/* GPS has no fix, return 0 to indicate failure*/
		return 0;
}


