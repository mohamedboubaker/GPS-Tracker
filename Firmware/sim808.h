#ifndef SIM808_H
#define SIM808_H

#include <stdint.h>
#include "main.h"

#define RX_WAIT 200 /*After transmitting wait RX_WAIT ms  to ensure that all the reply is receeived in the buffer*/
#define TX_TIMEOUT 100
#define RX_BUFFER_LENGTH 128
#define SIM_UART huart2
#define DEBUG_UART huart1
#define TCP_CONNECT_TIMEOUT 5 /*value in second*/
#define GPS_COORDINATES_LENGTH 23 /*4927.656000,1106.059700,319.200000\n*/





/*
 * Function name: sim_init()
 * 
 * it initilizes the SIM808 module
 * it returns 1 if the module is Ready and 0 otherwise
 */
uint8_t sim_init();

/*
 * Function name: sim_gps_power_on()
 *
 * it enables the GPS functionality of the SIM808 module
 * it returns 1 if the GPS was successfully enabled and 0 otherwise
 */
uint8_t sim_gps_enable();


/*
 * function name: sim_gps_get_status()
 * 
 * it checks if the GPS has a location fix. which means, it knows its position.
 * it returns 1 if the GPS has a fix and 0 otherwise
*/
uint8_t sim_gps_get_status();

/*
 * function name: sim_gps_get_location(char*) 
 * 
 * it returns the GPS coordinates as a char array. The format of the output is Longitude,Latitude Example: 3937.656010,1406.059400
 * 
 * char * position: it is used to store the coordinates. 
 *
 * it always returns 1.
*/
uint8_t sim_gps_get_location(char* position);

/* 
 * function name: sim_gps_get_time
 *
 * it returns the GPS time as a char array.
 * 
 * char * time: it is used to store the time. 
 *
 * it always returns 1.
*/
uint8_t sim_gps_get_time(char * time);
 
uint8_t sim_gps_power_off();

/* Enable GPRS modem*/
uint8_t sim_gprs_enable();

/* put PIN code */
uint8_t sim_gprs_insert_PIN(char * pin);

/* set APN */
uint8_t sim_gprs_set_APN(char * apn, char * username, char * password);


/* 
Send data over TCP, 
if TCP connection is already open then it closes it and opens a new connection 
then sends data, then closes the connection.
param char * host can be IP adress or DNS name
if parameter keep_con_open=1 the connection will not be closed 
*/
uint8_t sim_tcp_send(char * host, char * port, uint8_t * data, uint8_t length, uint8_t keep_con_open);


/* Publish payload to MQTT topic with QoS 1 by default*/
uint8_t sim_mqtt_publish(char * server_address, char * port, char * client_id, char * payload);
#endif