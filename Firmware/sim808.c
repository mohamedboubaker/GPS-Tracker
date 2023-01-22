/** @file sim808.c
*  @brief Function implementation for the SIM808 STM32 driver.
*
*  @author Mohamed Boubaker
*  @bug issue #9
*/
#include <string.h>
#include <stdio.h>
#include "sim808.h"


UART_HandleTypeDef huart1; 
UART_HandleTypeDef huart2;
#define AT_uart huart1
#define debug_uart huart2


/* These Global variables should only be touched by sim_send_cmd, get_cmd_reply, UART_receive_IT,*/
static volatile uint8_t rx_byte; /* The receive interupt routine uses rx_byte to store a copy of the received byte*/
static volatile uint8_t rx_index=0; /*track the number of received bytes.*/
static volatile char sim_rx_buffer[RX_BUFFER_LENGTH];


/**
 * @brief send_serial() sends raw serial data to the SIM808 module through the AT_uart peripheral. Then it deletes the receive buffer.
 * @param const char * cmd contains the command 
 * @param uint32_t rx_wait waiting time before exit. To make sure the reply is received.
 * @returns void
 */
void send_serial(uint8_t * data, uint8_t length, uint32_t rx_wait){
	HAL_UART_Transmit(&AT_uart,data,length,TX_TIMEOUT);
	
	/* Wait for command to be processed */
	HAL_Delay(rx_wait);
	
	/* Clear the sim_rx_buffer, reset the receive counter rx_index 
	 * and then return 1 to acknowledge the success of the command
	 */
	memset((void *)sim_rx_buffer,NULL,RX_BUFFER_LENGTH);
	rx_index=0;
}


/**
 * @brief send_cmd() sends AT commands to the SIM808 module through the AT_uart peripheral.
 * in case the module replies with an error, or doesn't reply anything, it will try to send the command 2 more times
 * after a timeout period.
 * @param const char * cmd contains the command 
 * @param uint32_t rx_wait waiting time before checking the receive buffer.
 * @returns 1 if the module replies with OK, 0 otherwise
 */
uint8_t send_cmd(const char * cmd, uint32_t rx_wait){

	/* Variable used to count how many times the command is sent*/
	uint8_t trials=0;

	while(trials < 3){

		HAL_UART_Transmit(&AT_uart,(uint8_t *)cmd,strlen(cmd),TX_TIMEOUT);

		/* Wait for the module to finish transmitting the reply.
		 * in the first try, wait RX_WAIT, second try, wait more: 2*RX_WAIT, 3rd try, wait even more: 3*RX_WAIT
		 */
		HAL_Delay((1+trials)*rx_wait);

		/* Note: 
		 * The reply from the module will be captured by the UART receive interrupt callback routine HAL_UART_RxCpltCallback() 
		 * and stored in the global array sim_rx_buffer[RX_BUFFER_LENGTH];
		 */	


		/* Check if the reply contains the word "OK"*/
		if (strstr((const char *)sim_rx_buffer,"OK")!=NULL){ 
		
			/* Clear the sim_rx_buffer, reset the receive counter rx_index 
			 * and then return 1 to acknowledge the success of the command
			 */
			memset((void *)sim_rx_buffer,NULL,RX_BUFFER_LENGTH);
			rx_index=0;
			return 1; 
		}
		/* If the reply did not include the word OK, clear the buffer then try again*/
		else {
			memset((void *)sim_rx_buffer,NULL,RX_BUFFER_LENGTH);
			rx_index=0;
			trials++;
		}
}

	/* This line is reached if the command was sent 3 times and the module never replied with OK.
	 * return 0 to indicate failure
	 */
	return 0;
}


/**
 * @brief sim_get_cmd_reply() sends a cmd to the module and copies the reply into the parameter char * cmd_reply
 * @param char * cmd is the command
 * @param char * cmd_reply is an array where to modules reply will be copied. 
 * @returns 1 if the module replies with OK, 0 otherwise
 */
uint8_t sim_get_cmd_reply(const char * cmd, char * cmd_reply,uint32_t rx_wait){

	/* the implementation of this function is very similar to send_cmd. The difference is
	 * that this function sends the command only 1 time and copies the reply from the receive buffer 
	 * to the array parameter cmd_reply before clearing the receive buffer sim_rx_buffer. 
	 */

	HAL_UART_Transmit(&AT_uart,(uint8_t *)cmd,strlen(cmd),TX_TIMEOUT);

	/* Wait for the module to finish transmitting the reply.*/
	HAL_Delay(rx_wait);

	/* Note: 
	 * The reply from the module will be captured by the UART receive interrupt callback routine HAL_UART_RxCpltCallback() 
	 * and stored in the global array sim_rx_buffer[RX_BUFFER_LENGTH];
	 */	


	/* Check if the reply contains the word "OK"*/
	if (strstr((const char *)sim_rx_buffer,"OK")!=NULL){ 

		/* Copy buffer, clear the sim_rx_buffer, reset the receive counter rx_index 
		 * and then return 1 to acknowledge the success of the command		 
		 */
		memcpy(cmd_reply,(const char *)sim_rx_buffer,RX_BUFFER_LENGTH);
		memset((void *)sim_rx_buffer,NULL,RX_BUFFER_LENGTH);
		rx_index=0;
		return 1; 
	}
	/* If the reply did not include the word OK, clear the buffer then clear buffer, reset counter, return 0 */
	else {
		memset((void *)sim_rx_buffer,NULL,RX_BUFFER_LENGTH);
		rx_index=0;
		return 0;
	}


}

/**
 * @brief HAL_UART_RxCpltCallback() is called when the receive buffer of any UART has received 1 byte.
 * it copies the received byte into the receive buffer which is a global variable: sim_rx_buffer. 
 * it also increases by one the counter of received bytes: rx_index. Which points to the next free index in the receive buffer.
 * usart1 is always used to communicate with SIM module therefore AT_uart is defined as usart1.
 * @returns void
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if (huart->Instance==USART1){
		sim_rx_buffer[rx_index++ % RX_BUFFER_LENGTH]=rx_byte; /*rx_index++ modulo RX_BUFFER_LENGTH to ensure it always stays smaller then RX_BUFFER_LENGTH*/
		
		/* Enable UART receive interrupt again*/
		HAL_UART_Receive_IT(&AT_uart,(uint8_t *)&rx_byte,1);
	}
}	


/**
 * @brief sim_init()  it initialises the SIM808_typedef struct members and powers on the module.
 * initializes the UARTs and the GPIOs used to power on, reset and check the status of the module.
 * Which UART peripheral is used for what is specified in the parameter SIM808_typedef members: AT_uart and debug_uart.
 * @param SIM808_typedef sim is the definition of the sim808 hardware
 * @return 1 if module is ready for use, 0 otherwise
 */			 
uint8_t sim_init(SIM808_typedef * sim){

	/*Initialize UART peripherals*/

	debug_uart.Instance = USART2;
	debug_uart.Init.BaudRate = BAUD_RATE;
	debug_uart.Init.WordLength = UART_WORDLENGTH_8B;
	debug_uart.Init.StopBits = UART_STOPBITS_1;
	debug_uart.Init.Parity = UART_PARITY_NONE;
	debug_uart.Init.Mode = UART_MODE_TX_RX;
	debug_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	debug_uart.Init.OverSampling = UART_OVERSAMPLING_16;
	debug_uart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	debug_uart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&debug_uart) != HAL_OK){
		Error_Handler();
	}	


	AT_uart.Instance = USART1;
	AT_uart.Init.BaudRate = BAUD_RATE;
	AT_uart.Init.WordLength = UART_WORDLENGTH_8B;
	AT_uart.Init.StopBits = UART_STOPBITS_1;
	AT_uart.Init.Parity = UART_PARITY_NONE;
	AT_uart.Init.Mode = UART_MODE_TX_RX;
	AT_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	AT_uart.Init.OverSampling = UART_OVERSAMPLING_16;
	AT_uart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	AT_uart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&AT_uart) != HAL_OK){
	Error_Handler();
	}

	/* trigger an interrupt for every received byte on AT_uart*/
	HAL_UART_Receive_IT(&AT_uart,(uint8_t *)&rx_byte,1);  

	/*Initialize GPIOs*/
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/*Configure Reset GPIO pin */
	HAL_GPIO_WritePin(sim->reset_gpio, sim->reset_pin, GPIO_PIN_SET);
	GPIO_InitStruct.Pin = sim->reset_pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(sim->reset_gpio, &GPIO_InitStruct);

	/*Configure  power on GPIO pin */
	HAL_GPIO_WritePin(sim->power_on_gpio, sim->power_on_pin, GPIO_PIN_SET);
	GPIO_InitStruct.Pin = sim->power_on_pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(sim->power_on_gpio, &GPIO_InitStruct);

	/*Configure  STATUS GPIO pin */
	GPIO_InitStruct.Pin = sim->status_pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(sim->status_gpio, &GPIO_InitStruct);

	/* Power on module:
	 * Check if the module is Powered on by reading the value of STATUS pin.
	 * If the module is powered off then pull down the power pin of sim808 for at least 1 second to power it on.
	 * Then read the STATUS pin again. If the device is not powered on, try 2 more times.
	 */

	/* Count number of power on trials */
	uint8_t trials=0;  

	while(!HAL_GPIO_ReadPin(sim->status_gpio,sim->status_pin) && trials<3){
		HAL_GPIO_WritePin(sim->power_on_gpio,sim->power_on_pin,GPIO_PIN_RESET);
		/* Keep pin down for 1.5 s */
		HAL_Delay(1500);
		HAL_GPIO_WritePin(sim->power_on_gpio,sim->power_on_pin,GPIO_PIN_SET);
		HAL_Delay(500);
		trials++;
	}
	
	/* Read STATUS pin of the SIM808 to check the power on status. if the module is ON then power on indicator LED*/
	if (HAL_GPIO_ReadPin(sim->status_gpio,sim->status_pin)){
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_SET);

	/* Send the string "AT" to synchronize baude rate of the SIM808 module*/
	HAL_UART_Transmit(&AT_uart,(uint8_t *)"AT\r",3,TX_TIMEOUT);

	/*It is recommended to wait 3 to 5 seconds before sending the first AT character. */
	HAL_Delay(3000);	
	} 
	else{
		/*Turn down the indicator LED and return FAIL */
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);
		return FAIL;
	}

	/*Send the First AT Command to check if the module is responding*/
	return send_cmd("AT\r",RX_WAIT);
}



