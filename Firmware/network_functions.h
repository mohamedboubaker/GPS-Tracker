/** 
*	@file network_functions.h
*	@brief GPRS, TCP and MQTT functions' prototypes and definition of related parameters.
*
*	@author Mohamed Boubaker
*	@bug issue #9
*/
#ifndef NETWORK_FUNCTIONS_H
#define NETWORK_FUNCTIONS_H


#define DEBUG_MODE 1
/* TCP/GPRS error code*/
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

/* MQTT error code*/
#define ERR_MQTT_EMPTY_PARAM 100

/*GPRS definitions*/
#define APN "TM"
#define APN_LENGTH 2 /* Length of the APN string */
#define SIM_PIN ""

/* MQTT packet variable definitions*/
#define MAX_LENGTH_MQTT_PACKET 128
#define MQTT_KEEP_ALIVE 15

/**
 * @brief enables the GPRS connection. 
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
uint8_t enable_gprs();


 /**
 * @brief disables the GPRS functionality and the whole radio module.
 * @return 1 if the GPRS is successfully disabled, 0 otherwise.
 */
uint8_t disable_gprs();



/**
 * @brief opens a new TCP connection. 
 * if the function is called when there is already an open TCP connection then it closes it and opens a new connection. 
 * @param ip_address is the IP address of the server. it can be an IP adress or DNS hostname.
 * @param tcp_port is the remote TCP port.
 * @return SUCCESS if the connection was successfully opened, FAIL otherwise
 */
uint8_t open_tcp_connection(char * ip_address, char * tcp_port);

/**
 * @brief sends data over an open TCP connection.
 * @param data is the byte array to be sent
 * @param data_length is the length of the byte array to be sent.
 * @return SUCCESS if the data was successfully sent, FAIL otherwise.
 */
uint8_t send_tcp_data(uint8_t * data, uint8_t data_length);

/**
 * @brief closes an open TCP connection. 
 * @return SUCCESS if the connection was successfully closed, FAIL otherwise.
 */
uint8_t close_tcp_connection();


/**
 * @brief publishes a message to an MQTT topic with QoS 1 by default.
 * @param ip_address is the MQTT server IP address or DNS hostname.
 * @param tcp_port is the MQTT server port 
 * @param client_id is the MQTT client ID.
 * @param topic is the MQTT topic name.
 * @param message is the message to be sent.
 * @return SUCCESS if the message is successfully delivered, 0 otherwise.
 */
uint8_t publish_mqtt_msg(char * ip_address, char * tcp_port, char * topic, char * client_id, char * message);


#endif