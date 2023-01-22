/** @file network_functions.c
*  @brief this file include the declaration of functions related to GPRS, TCP and MQTT
*
*  @author Mohamed Boubaker
*  @bug issue #9
*/
#ifndef NETWORK_FUNCTIONS_H
#define NETWORK_FUNCTIONS_H
#include "sim808.h"

#define ERR_PHONE_FUNCTION 48
#define ERR_SIM_PRESENCE 49
#define ERR_PIN_WRONG 50
#define ERR_WEAK_SIGNAL 51
#define ERR_REGISTRATION 52
#define ERR_GPRS_ATTACH 52
#define ERR_PDP_DEACTIVATED 53
#define ERR_PDP_DEFINE 54
#define ERR_PDP_ACTIVATE 55
#define ERR_GET_IP 56


/**
 * @brief sim_enable_gprs() enables the GPRS connection. 
 * GPRS must be enabled before trying to establish TCP connection.
 * This function goes through a series of the tests below. action is taken depending on the test result
 * 1. Checks if the SIM card is detected
 * 2. Checks if the SIM card requires PIN code
 * 3. Checks the signal stregth. If it is very low, it returns an error value.
 * 4. Checks if the Mobile Equipement (ME) is registered to the Network. If not, it tries to register.
 * 5. Checks if ME is attached to GPRS service. if not it tries to attach.
 * 6. Checks if GPRS PDP context is defined, if not it tries to define it, enable it, and get IP address.
 * 
 * @return 1 if gprs is active, 2 if SIM card is not detected, 3 if SIM PIN is incorrect, 4 if the signal is weak, 0 otherwise
 */
uint8_t sim_enable_gprs();


 /**
 * @brief sim_gprs_disable () disables the GPRS functionality and the whole radio module.
 * @return 1 if the GPRS is successfully disabled, 0 otherwise
 */
uint8_t sim_gprs_disable();



/**
 * @brief sim_tcp_send() sends raw data over a TCP connection. 
 * if the function is called when there is already an open TCP connection (for some reason)
 * then it closes it and opens a new connection. 
 * @param server_address is the remote TCP peer address. it can be an IP adress or DNS name
 * @param port is the remote TCP port
 * @param data is the data to be sent
 * @param length is the length of the data to be sent in bytes
 * @return 1 if data is successfully sent, 0 otherwise
 */
uint8_t sim_tcp_send(char * server_address, char * port, char * data, char * length);


/**
 * @brief sim_mqtt_publish() publishes a message to an MQTT topic with QoS 1 by default.
 * @param server_address is the MQTT server IP address or DNS name.
 * @param port is the MQTT server port 
 * @param client_id is the MQTT client id
 * @param topic is the MQTT topic name
 * @param message is the message to be sent
 * @return 1 if the message is successfully delivered, 0 otherwise.
 */
uint8_t sim_mqtt_publish(char * server_address, char * port, char * client_id, char * message);


#endif