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
uint8_t sim_get_cmd_reply(const char * cmd, char * cmd_reply){

	/* the implementation of this function is very similar to send_cmd. The difference is
	 * that this function sends the command only 1 time and copies the reply from the receive buffer 
	 * to the array parameter cmd_reply before clearing the receive buffer sim_rx_buffer. 
	 */

	HAL_UART_Transmit(&AT_uart,(uint8_t *)cmd,strlen(cmd),TX_TIMEOUT);

	/* Wait for the module to finish transmitting the reply.*/
	HAL_Delay(RX_WAIT);

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
 * @brief This function is called when the receive buffer of any UART has received 1 byte.
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




/*******************************************************/
/*                     GPS functions                   */
/*******************************************************/

/**
 * @brief sim_gps_enable() enables the GPS functionality of the SIM808 module.
 * @return 1 if the GPS is successfully enabled, 0 otherwise
 */
uint8_t sim_gps_enable(){
	const char gps_power_on_cmd[]= "AT+CGPSPWR=1\r";
	const char gps_set_mode_cold_cmd[]= "AT+CGPSRST=2\r";
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
	sim_get_cmd_reply(gps_get_status_cmd,local_rx_buffer);

	if (strstr(local_rx_buffer,"Location 3D Fix")!=NULL){
		
		/*clear local buffer*/
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);

		err_status=sim_get_cmd_reply(gps_get_location_cmd,local_rx_buffer);
		
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

/*******************************************************/
/*                    GPRS functions                   */
/*******************************************************/

/**
 * @brief sim_gprs_enable() enables GPRS connection. 
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
uint8_t sim_gprs_enable(){

	char local_rx_buffer[RX_BUFFER_LENGTH]; /* make sure to clear the buffer after every use */

	/* Detect if SIM card is present*/
	static const char SIM_detect_cmd[]= "AT+CSMINS?\r";
	sim_get_cmd_reply(SIM_detect_cmd,local_rx_buffer);
	if (!strstr(local_rx_buffer,"+CSMINS: 0,1")) 
		return 2;
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH); /*clear buffer for next use*/

	/* Check if PIN code is required */
	static const char PIN_status_cmd[]= "AT+CPIN?\r";
	static char PIN_insert_cmd[13]= "AT+CPIN=";
	sim_get_cmd_reply(PIN_status_cmd,local_rx_buffer);
	if (!strstr(local_rx_buffer,"READY")){
		strcat(PIN_insert_cmd,SIM_PIN);   /* build string = AT+CPIN=XXXX   SIM_PIN length is assumed = 4 */
		strcat(PIN_insert_cmd,"\r");      /* build string = AT+CPIN=XXXX\r */
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		sim_get_cmd_reply(PIN_insert_cmd,local_rx_buffer);
		if (!strstr(local_rx_buffer,"OK")) 
			return 3;
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
	}

	/* Check Signal Quality 
	 * command = AT+CSQ
	 * the reply is +CSQ: RSSI,BER
	 * RSSI = Received Signal Strength Indicator
	 * BER  = Bit Error Rate
	 * When antenna is disconnected, RSSI would be 0
	 * RSSI typical values
	 * 0 => -115 dBm or less
	 * 1 => -111 dBm
	 * [2 : 30] => [-110 : -54] dBm
	 * 31 => -52 dBm or greater
	 * 99 not known or not detectable
	 * BER (in percent):
	 * 0...7 As RXQUAL values in the table in GSM 05.08 subclause 7.2.4
	 * 99 Not known or not detectable
	 * Note ! After first reboot, the first querry will always result in 0,0.
	 */
	static const char check_signal_cmd[]= "AT+CSQ\r";
	sim_get_cmd_reply(check_signal_cmd,local_rx_buffer);
	if (strstr(local_rx_buffer,"+CSQ: 0,") || strstr(local_rx_buffer,"99,")) 
		return 3; /* if signal is weak return 3 */
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);

	/* Check Network Registration Status
	 * Command = AT+CREG?
	 * the reply is +CREG: 0,5
	 * First integer is status of result code presentation (Not sure what is this)
	 * Second integer is the Network registration status
	 * 0 => Not registered, Mobile Equipment (ME) is not currently searching a new operator to register to
	 * 1 => Registered, home network
	 * 2 => Not registered, but ME is currently searching a new operator to register to
	 * 3 => Registration denied
	 * 4 => Unknown
	 * 5 => Registered, roaming
	 */
	static const char check_registration_cmd[]= "AT+CREG?\r";
	static const char register_ME_cmd[]= "AT+CREG=1\r";
	sim_get_cmd_reply(check_registration_cmd,local_rx_buffer);
	if (!(strstr(local_rx_buffer,",1") || strstr(local_rx_buffer,",5"))) 
		if (!send_cmd(register_ME_cmd,RX_WAIT)) 
			return FAIL; /* Try to register, if registration fails return 0*/
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);

	/* Check if GPRS is attached */
	static const char check_grps_attach_cmd[]= "AT+CGATT?\r";
	static const char grps_attach_cmd[]= "AT+CGATT=1\r";
	sim_get_cmd_reply(check_grps_attach_cmd,local_rx_buffer);
	if ( strstr(local_rx_buffer,"CGATT: 0") )/* if GPRS is not attached, try to attach and return FAIL if cmd fails */
		if (!send_cmd(grps_attach_cmd,RX_WAIT)) return FAIL; 
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);

	/* 
	 * The sequence of GPRS states that must be followed for a correct GPRS connection establishement
	 * [PDP DEACT] => AT+CIPSHUT => [IP INITIAL] => AT+CSTT="TM","","" => [IP START] => AT+CIICR => [IP GPRSACT] => AT+CIFSR => [IP STATUS]
	 */

	static const char check_gprs_state_cmd[]= "AT+CIPSTATUS\r"; /* This command can be used to check gprs or TCP state */

	/*Check if PDP context is deactivated*/
	static const char deactivate_PDP_cmd[]= "AT+CIPSHUT\r";
	sim_get_cmd_reply(check_gprs_state_cmd,local_rx_buffer);
	if ( strstr(local_rx_buffer,"PDP DEACT") ) /*When PDP is deactivated it is necessary to run  AT+CIPSHUT to bring the status to [IP INITIAL] */
		if(!send_cmd(deactivate_PDP_cmd,RX_WAIT)) 
			return FAIL;
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);

	/* Check if PDP context is correctly defined and define it.
	 * when no PDP context is not defined the reply is the default PDP +CSTT: "CMNET","",""
	 */
	static const char check_PDP_context_cmd[]= "AT+CSTT?\r";
	sim_get_cmd_reply(check_PDP_context_cmd,local_rx_buffer);
	if ( strstr(local_rx_buffer,"CMNET") ){
		/* Build enable_PDP_context_cmd 
		 * APN Name length is very important in this implementation 
		 * APN_LENGTH must be adjusted when APN is changed in the header file
		 * 18 = the length of the command AT+CSTT="","",""\r without the APN name inserted
		 */
		static char define_PDP_context_cmd[APN_LENGTH+18]= "AT+CSTT=\"";
		strcat(define_PDP_context_cmd,APN); /* AT+CSTT="APN */
		strcat(define_PDP_context_cmd,"\",\"\",\"\"\r"); /* AT+CSTT="APN","","" */
		if(!send_cmd(define_PDP_context_cmd,RX_WAIT)) 
			return FAIL;
	}
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);

	/* Enable PDP context 
	 * Check the GPRS status, if it is IP START, then enable PDP context, otherwise jump to next test
	 */
	static const char enable_PDP_context_cmd[]= "AT+CIICR\r";
	sim_get_cmd_reply(check_gprs_state_cmd,local_rx_buffer); 
	if ( strstr(local_rx_buffer,"IP START") ){
		if (!send_cmd(enable_PDP_context_cmd,1000)) /* This command takes around 1 second to finish hence 1s wait time */
			return FAIL; 	
	}
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);

	/* Get IP address 
	 * check GPRS status, if it is IP GPRSACT, then request IP, otherwise abort.
	 * AT+CIFSR\r does not reply OK. it replies the IP address if successful or ERROR
	 */
	static const char get_IP_address_cmd[]= "AT+CIFSR\r";
	sim_get_cmd_reply(check_gprs_state_cmd,local_rx_buffer);
	if ( strstr(local_rx_buffer,"IP GPRSACT") ){
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		sim_get_cmd_reply(get_IP_address_cmd,local_rx_buffer);
		HAL_Delay(1000); /* This command takes around 1 second to finish hence 1s wait time */
		if ( strstr(local_rx_buffer,"ERROR")) 
			return FAIL;
	}
	else 
		return FAIL;
	
	return 1;
}

/**
 * @brief sends raw data over a TCP connection. 
 * @param server_address is the remote TCP peer address. it can be an IP adress or DNS name
 * @param port is the remote TCP port
 * @param data is the data to be sent
 * @param length is the length of the data to be sent in bytes
 * @param keep_con_open if it is set to 1 then the connection will not be closed after sending the data. if it is set to 0, the connection will closed after sending the data. Other values are not defined.
 * if the function is called when there is already an open TCP connection then it sends data over this connection, otherwise it creates a new connection. 
 * @return 1 if data is successfully sent, 0 otherwise
 */
uint8_t sim_tcp_send(char * host, char * port, uint8_t * data, uint8_t length, uint8_t keep_con_open){
	
	static const char get_tcp_status_cmd[]="AT+CIPSTATUS\r";
	static const char tcp_disconnect_cmd[16]= "AT+CIPCLOSE\r";
	char send_tcp_data_cmd[24]= "AT+CIPSEND=";
	char tcp_connect_cmd[64]= "AT+CIPSTART=\"TCP\",\"";
	
	char local_rx_buffer[RX_BUFFER_LENGTH]; /* make sure to clear the buffer after every use */

	/* Check underlying GPRS connection before trying to open a TCP connection.
	 * If it is disconnected, then try to enable it.
	 * if enabling GPRS fails, then return FAIL.
	 */
	sim_get_cmd_reply(get_tcp_status_cmd,local_rx_buffer);
	if ( !(strstr(local_rx_buffer,"IP STATUS") || strstr(local_rx_buffer,"CONNECT OK") || strstr(local_rx_buffer,"ALREADY CONNECT") || strstr(local_rx_buffer,"TCP CLOSED") ) )
		if (!sim_gprs_enable())
			return FAIL;
	/* clear receive buffer */
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
	
	/* Check connection Status.
	 * if there is no open connection, then open a new TCP connection.
	 */
	sim_get_cmd_reply(get_tcp_status_cmd,local_rx_buffer);
	if( !(strstr(local_rx_buffer,"ALREADY CONNECT") || strstr(local_rx_buffer,"CONNECT OK"))){
		
		/* clear receive buffer */
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		
		/* build the connect command by adding address and port */
		strcat(tcp_connect_cmd,host);    /* AT+CIPSTART=\"TCP\","host.com           */
		strcat(tcp_connect_cmd,"\",\""); /* AT+CIPSTART=\"TCP\","host.com","        */
		strcat(tcp_connect_cmd,port);    /* AT+CIPSTART=\"TCP\","host.com","port    */
		strcat(tcp_connect_cmd,"\"\r");  /* AT+CIPSTART=\"TCP\","host.com","port"\r */
		
		/*Send open connection command */
		sim_get_cmd_reply(tcp_connect_cmd,local_rx_buffer);
		
		/* Wait for connection to establish or fail*/
		HAL_Delay(1000);
		if (strstr(local_rx_buffer,"CONNECT FAIL") || strstr(local_rx_buffer,"ERROR"))
			return FAIL;
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
	}
	
	/* Send TCP data */
	
//	/* build command */
//	strcat(send_tcp_data_cmd,length); /* AT+CIPSEND=L where L is an uint8_t */ 
//	strcat(send_tcp_data_cmd,"\r");       /* AT+CIPSEND=L\r                     */ 
//	
//	/* start data transmission */
//	send_serial(send_tcp_data_cmd,24,RX_WAIT);
//	
//	/* Send TCP data */
//	sim_get_cmd_reply(data,local_rx_buffer);
//	
//	if (!strstr(local_rx_buffer,"SEND OK"))
//		return FAIL;
//	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
	
	if (keep_con_open == 0 )
		send_cmd(tcp_disconnect_cmd,RX_WAIT);
	
	return SUCCESS;
}
