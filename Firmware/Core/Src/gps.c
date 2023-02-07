/** @file gps.c
*  @brief GPS functions implementations
*
*  @author Mohamed Boubaker
*  
*/
#include <string.h>
#include <stdio.h>
#include "gps.h"



uint8_t enable_gps(){
	const char gps_power_on_cmd[]= "AT+CGPSPWR=1\r";
	const char gps_set_mode_cold_cmd[]= "AT+CGPSRST=2\r";
	uint8_t power_on_status=0;
	uint8_t set_mode_status=0;

	power_on_status=send_AT_cmd(gps_power_on_cmd,"OK",0,NULL,RX_WAIT); 
	set_mode_status=send_AT_cmd(gps_set_mode_cold_cmd,"OK",0,NULL,RX_WAIT); 

	return (power_on_status && set_mode_status);
}


uint8_t get_gps_location(char * coordinates){

	/* 
	 * The modules reply inculde: 
	 * -GPS time
	 * -latitue and longitute in DMM.MM (Degrees Minutes.Minutes)
	 * -the prefered format used on the server is the decimal degrees dd. The transformation dd = d + mm.mm/60 
	 * needs to be done on the server
	 * GPS Status can be:
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
	if (send_AT_cmd(gps_get_status_cmd,"Location 3D Fix",0,NULL,RX_WAIT)){

		err_status=send_AT_cmd(gps_get_location_cmd,"OK",1,local_rx_buffer,RX_WAIT);
		
		/* Example reply 
		* AT+CGPSINF=0 +CGPSINF: 0,4927.656000,1106.059700,319.200000,20220816200132.000,0,12,1.592720,351
		* Actuall coordinates start at charachter 27
		*/

		/* Extract the coordinates from the cmd reply and copy only 
		*	the coordinates into function parameter char * coordinates
		*/
		memcpy(coordinates,local_rx_buffer+27,GPS_COORDINATES_LENGTH);
		
		return err_status;
	}
	else
		/* GPS has no fix, return 0 to indicate failure*/
		return FAIL;
}


