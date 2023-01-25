/** @file sim808.h
 *  @brief Function prototypes for the SIM808 STM32 driver.
 *
 *  @author Mohamed Boubaker
 *  @bug full of bugs
 */
#ifndef SIM808_H
#define SIM808_H

#include <stdint.h>
#include "main.h"

#define RX_WAIT 100 /*After sending AT command, wait RX_WAIT ms  to ensure that the reply is receeived in the buffer*/
#define TX_TIMEOUT 100
#define BAUD_RATE 38400 /*BAUD_RATE=38400 => it take 26 ms to send 100 bytes */
#define RX_BUFFER_LENGTH 128
#define SIM_UART huart2
#define DEBUG_UART huart1
#define TCP_CONNECT_TIMEOUT 5 /*value in second*/
#define GPS_COORDINATES_LENGTH 23 /*4927.656000,1106.059700,319.200000\n*/


/* SIM808_typedef is used to abstract the SIM808 module. 
 * It will decouple the functions from the hardware (uart+pins) used to interface with the module.
 * The UART peripheral used to send AT commands to the module and the UART used to send Debug messages
 * and the pins used to power on, reset and check status of the module are combined together in this struct.
 * This struct will be passed to all the functions that perform actions on the module.
 */

typedef struct {
	USART_TypeDef * AT_uart_instance; /*send AT commands through this UART.*/
	USART_TypeDef * debug_uart_instance;
	GPIO_TypeDef  * power_on_gpio;
	uint16_t power_on_pin;
	GPIO_TypeDef  * reset_gpio;
	uint16_t reset_pin;
	GPIO_TypeDef  * status_gpio;
	uint16_t status_pin;
} SIM808_typedef;



/**
 * @brief sim_init()  it initialises the SIM808_typedef struct members and powers on the module.
 *  initializes the UARTs and the GPIOs used to power on, reset and check the status of the module.
 *	Which UART peripheral is used for what is specified in the parameter SIM808_typedef members: AT_uart and debug_uart.
 * @param SIM808_typedef sim is the definition of the sim808 hardware
 * @return 1 if module is ready for use, 0 otherwise
 */
uint8_t sim_init(SIM808_typedef * sim);

/**
 * @brief enables the GPS functionality of the SIM808 module
 * @return 1 if the GPS is successfully enabled, 0 otherwise
 */
uint8_t sim_gps_enable();

/*
 * function name: sim_gps_get_status()
 * 
 * it checks if the GPS has a location fix. 
 * Possible replies from the SIM808 are
 *	"Location Unknown" if GPS is not enabled
 *	"Location not Fix" 
 *	"Location 2D Fix"
 *	"Location 3D Fix"
*/
uint8_t sim_gps_get_status();
/**
 * @brief checks if the GPS has a location fix (position known).
 * @return 1 if the GPS has a fix, 0 otherwise
 */
uint8_t sim_gps_fix_status();

/** 
 * @brief returns the GPS coordinates. The format of the output is longitude,latitude Example: 3937.656010,1406.059400
 * @param position is used to store the coordinates. 
 * @return always 1.
 */
uint8_t sim_gps_get_location(char* position);

/** 
 * @brief returns the GPS time
 * @param time is used to store the time. 
 * @return always returns 1.
 */
uint8_t sim_gps_get_time(char * time);
 
uint8_t sim_gps_power_off();

/**
 * @brief enables GPRS modem. 
 * GPRS modem must be enabled before trying to connect to the internet.
 * @return 1 if modem is successfully enabled, 0 otherwise.
 */
uint8_t sim_gprs_enable();

/**
 * @brief insert PIN code in the SIM CARD
 * @param pin is used to provide the PIN
 * @return 1 if PIN is accepted, 0 otherwise.
 */
uint8_t sim_gprs_insert_PIN(char * pin);

/**
 * @brief set Access Point Name (APN) for the GPRS 
 * @param apn is the APN name
 * @param username is the name user to access the GPRS network
 * @param password is the password
 * @return 1 if login is successfully, 0 otherwise 
 */
uint8_t sim_gprs_set_APN(char * apn, char * username, char * password);


/**
 * @brief sends raw data over a TCP connection. 
 * @param server_address is the remote TCP peer address. it can be an IP adress or DNS name
 * @param port is the remote TCP port
 * @param data is the data to be sent
 * @param length is the length of the data to be sent in bytes
 * @param keep_con_open if it is set to 1 then the connection will not be closed after sending the data. if it is set to 0, the connection will closed after sending the data. Other values are not defined.
 *        if the function is called when there is already an open TCP connection then it sends data over this connection, otherwise it creates a new connection. 
 * @return 1 if data is successfully sent, 0 otherwise
 */
uint8_t sim_tcp_send(char * server_address, char * port, uint8_t * data, uint8_t length, uint8_t keep_con_open);


/**
 * @brief publishes a message to an MQTT topic with QoS 1 by default.
 * @param server_address is the MQTT server IP address or DNS name.
 * @param port is the MQTT server port 
 * @param client_id is the MQTT client id
 * @param topic is the MQTT topic name
 * @param message is the message to be sent
 * @return 1 if the message is successfully delivered, 0 otherwise.
 */
uint8_t sim_mqtt_publish(char * server_address, char * port, char * client_id, char * message);
#endif