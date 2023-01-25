/** @file sim808.h
 *  @brief Function prototypes for the SIM808 STM32 driver.
 *
 *  @author Mohamed Boubaker
 *  @bug issue #9
 */
#ifndef SIM808_H
#define SIM808_H

#include <stdint.h>
#include "main.h"

/* Error codes */
#define FAIL 0
#define SUCCESS 1
#define TRUE 1
#define FALSE 0



#define RX_WAIT 200 /*After sending AT command, wait RX_WAIT ms  to ensure that the reply is receeived in the buffer*/
#define TX_TIMEOUT 100
#define BAUD_RATE 38400 /*BAUD_RATE=38400 => it take 26 ms to send 100 bytes */
#define RX_BUFFER_LENGTH 256
#define SIM_UART huart2
#define DEBUG_UART huart1
#define TCP_CONNECT_TIMEOUT 5 /*value in second*/
#define GPS_COORDINATES_LENGTH 23 /*4927.656000,1106.059700,319.200000\n*/

/*GPRS definitions*/
#define APN "TM"
#define APN_LENGTH 2 /* Length of APN string */
#define SIM_PIN ""



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
 * @brief send_cmd() sends AT commands to the SIM808 module through the AT_uart peripheral.
 * in case the module replies with an error, or doesn't reply anything, it will try to send the command 2 more times
 * after a timeout period.
 * @param const char * cmd contains the command 
 * @param uint32_t rx_wait waiting time before checking the receive buffer.
 * @returns 1 if the module replies with OK, 0 otherwise
 */
uint8_t send_cmd(const char * cmd, uint32_t rx_wait);

void send_debug(const char * debug_msg);
void send_serial(uint8_t * data, uint8_t length, char * cmd_reply, uint32_t rx_wait);

	
/**
 * @brief sim_get_cmd_reply() sends a cmd to the module and copies the reply into the parameter char * cmd_reply
 * @param char * cmd is the command
 * @param char * cmd_reply is an array where to modules reply will be copied. 
 * @returns 1 if the module replies with OK, 0 otherwise
 */
uint8_t sim_get_cmd_reply(const char * cmd, char * cmd_reply,uint32_t rx_wait);

/******************* GPS functions ********************/

#endif