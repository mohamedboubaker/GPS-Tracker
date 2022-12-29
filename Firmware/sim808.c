/** @file sim808.c
 *  @brief Function implementation for the SIM808 STM32 driver.
 *
 *  @author Mohamed Boubaker
 *  @bug full of bugs
 *  @todo implement DEBUG mode
 */
#include <string.h>
#include <stdio.h>
#include "sim808.h"

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;


/*Generic reply that indicate the command is successful*/
static const char sim_ok[]= "OK";

static uint8_t rx_byte;
static uint8_t rx_index=0;
static uint8_t cmd_result=0;
static char sim_rx_buffer[RX_BUFFER_LENGTH];



/* Print RX buffer in debug UART */
void printrx(){
	uint8_t i=0;
	while(sim_rx_buffer[i]!='\0') i++;
	HAL_UART_Transmit(&DEBUG_UART,(uint8_t*)sim_rx_buffer,i,100);
	HAL_UART_Transmit(&DEBUG_UART,(uint8_t*)'\n',1,100);
}

/* Print to debug uart ****/
void print_debug(uint8_t * msg){
	HAL_UART_Transmit(&DEBUG_UART,msg,strlen((char*)msg),100);
	HAL_UART_Transmit(&DEBUG_UART,(uint8_t*)'\n',1,100);
}

/* send a command to the  SIM module through SIM UARTT*/
void send_cmd(const char * msg){
	HAL_UART_Transmit(&SIM_UART,(uint8_t *)msg,strlen(msg),TX_TIMEOUT);
	HAL_Delay(RX_WAIT);
	printrx();
}
/* send raw data to the  SIM module through SIM UARTT*/
void send_raw_cmd(uint8_t * data,uint8_t length){
	HAL_UART_Transmit(&SIM_UART,data,length,TX_TIMEOUT);
	HAL_Delay(RX_WAIT);
	printrx();
}

/* UART 2 RX buffer full interrupt callback */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if (huart->Instance==USART2){
		sim_rx_buffer[rx_index++]=rx_byte;
		HAL_UART_Receive_IT(&SIM_UART,&rx_byte,1);
	}
}	

/* clear_rx_buffer sets all buffer values to NULL*/
static void clear_rx_buffer(){
	uint8_t i;
	for(i=0;i<RX_BUFFER_LENGTH;i++) sim_rx_buffer[i]='\0';
	rx_index=0;
}
/* strcmpr(char *,char *,n) compares 2 char arrays of length n. If they are equal, it returns 1 otherwise 0 */
static uint8_t strcmpr(const char * a,const char * b, uint8_t n){
	uint8_t r=1;
	uint8_t i=0;
	for (i=0;i<n;i++) 
		if (a[i]!=b[i]){
			
				r=0;
				break;
		}
	return r;
	}

/*
 * function name: sim_init()
 * 
 * it initialises the UART (defined DEBUG_UART in the header file) that will be used to send debug messsages
 * it also initialises The UART, defined SIM_UART in the header file, that is used to communicate with the SIM808 module
 * it sends AT command = 'AT\r' to module
 * the module will reply OK if it is ready otherwise it will reply ERROR	
 * it uses the Glabal Array sim_rx_buffer to store the reply of the module.
 * The buffer is cleared after every receiving a new reply using the function clear_rx_buffer
 * 
 * if the reply is OK then the function returns 1 0therwise 0
 */
uint8_t sim_init(){
	const char sim_check_cmd[]= "AT\r"; 
	
	/* UART 1 initilized for debugging reasons -- debug will be sent to PC serial connector */
	huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
	
	/* UART 2 is used to control the SIM808 */
	huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
	
	/* Enable UART 2 receive interrupt for every received byte*/
	HAL_UART_Receive_IT(&SIM_UART,&rx_byte,1);
	clear_rx_buffer();
	
	/* Send AT command to check if module is available */
	send_cmd(sim_check_cmd);
	
	if (strstr(sim_rx_buffer,sim_ok)!=NULL)
		cmd_result=1;
	
	clear_rx_buffer();
	return cmd_result;

}																				 



/**** GPS Functions ****/

uint8_t sim_gps_enable(){
	const char gps_power_on_cmd[]= "AT+CGPSPWR=1\r";
	const char gps_set_mode_cold_cmd[]= "AT+CGPSRST=0\r";
	uint8_t power_on_status=0;
	uint8_t set_mode_status=0;
	
	send_cmd(gps_power_on_cmd); /* Send power on cmd to module*/
	
		
	if (strcmpr(sim_ok,sim_rx_buffer+15,2))  /* Check if reply is OK , 15=13+2 check explanation above*/
		power_on_status=1;
	
	clear_rx_buffer();
		
	send_cmd(gps_set_mode_cold_cmd); /*Set cold mode*/
	
	if (strstr(sim_rx_buffer,sim_ok)!=NULL)
		set_mode_status=1;	
	
	
	clear_rx_buffer();
	return (power_on_status & set_mode_status);
}

/*Power off gps*/
uint8_t sim_gps_power_off(){
	const char gps_power_off_cmd[]= "AT+CGPSPWR=0\r";
	uint8_t power_off_status=0;
	
	send_cmd(gps_power_off_cmd);
	
	if (strstr(sim_rx_buffer,sim_ok)!=NULL)
		power_off_status=1;
	
	clear_rx_buffer();
	return power_off_status;
}


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
uint8_t sim_gps_get_status(){
	static const char gps_get_status_cmd[]= "AT+CGPSSTATUS?\r";
	uint8_t gps_status=0;
	
	send_cmd(gps_get_status_cmd);
	
	if (strstr(sim_rx_buffer,"Location 3D Fix")!=NULL)
		gps_status=1;
	clear_rx_buffer();
	return gps_status;
}

/*
 * sim_gps_get_location(char*) querries the GPS module for the current position. The GPS module's reply include
 * current time
 * latitue and longitute in DMM.MM (Degrees Minutes.Minutes)
 * the format used on the server is the decimal degrees dd
 * transformation dd = d + mm.mm/60 need to be done on the server
*/
uint8_t sim_gps_get_location(char * coordinates){
	const char gps_get_location_cmd[]= "AT+CGPSINF=0\r";
	
	send_cmd(gps_get_location_cmd);
	
	
	/*Example return 
	AT+CGPSINF=0 +CGPSINF: 0,4927.656000,1106.059700,319.200000,20220816200132.000,0,12,1.592720,351
	Actuall coordinates start at charachter 27
	*/
	
	/*Copy coordinates into parameter*/
	for(uint8_t i=0;i<GPS_COORDINATES_LENGTH;i++)
		coordinates[i]=sim_rx_buffer[i+27];
		
	clear_rx_buffer();
	return 1;
}	



/**** GPRS functions ****/
/*
 * Function name: sim_gprs_enable()
 * 
 * it enables the GPRS modem of the SIM808 module. Before accessing the internet, GPRS must be enabled.
 *
 * it returns 1 if the GPRS was successfully enabled and 0 otherwise
 */

uint8_t sim_gprs_enable(){
	const char gprs_enable_cmd[]= "AT+CFUN=1\r";
	uint8_t gprs_status=0;
	send_cmd(gprs_enable_cmd);
	
	if (strstr((char*)sim_rx_buffer,sim_ok)!=NULL)
		gprs_status=1;
	clear_rx_buffer();
	return gprs_status;
}


uint8_t sim_gprs_insert_PIN(char * pin){
	char gprs_PIN_insert_cmd[13]= "AT+CPIN=";
	const char gprs_PIN_status_cmd[]= "AT+CPIN?\r";
	uint8_t PIN_status=0;
	
	send_cmd(gprs_PIN_status_cmd);
	
	
	if (strstr(sim_rx_buffer,"READY")!=NULL)
		PIN_status=1;
	
	clear_rx_buffer();
	
	if (PIN_status==0){
		strcat(gprs_PIN_insert_cmd,pin);
		strcat(gprs_PIN_insert_cmd,"\r");
		send_cmd(gprs_PIN_insert_cmd);

		if (strstr(sim_rx_buffer,sim_ok)!=NULL)
			PIN_status=1;
		clear_rx_buffer();
	}
	
	return PIN_status;
}

/*Send TCP data*/
uint8_t sim_tcp_send(char * host, char * port, uint8_t * data,uint8_t length, uint8_t keep_con_open){
	
	const char gprs_TCP_get_status_cmd[]="AT+CIPSTATUS\r";
	const char gprs_TCP_disconnect_cmd[16]= "AT+CIPCLOSE\r";
	char gprs_data_send_cmd[24]= "AT+CIPSEND=";
	char gprs_TCP_connect_cmd[64]= {};
	uint8_t con_status =0;
	uint8_t send_status =0;
	uint8_t timeout_counter=0;
	
	/*
	Check current TCP connection status
	if TCP connection is not already  established then create a connetion 
	*/
	send_cmd(gprs_TCP_get_status_cmd);
	if((strstr((char*)sim_rx_buffer,"ALREADY CONNECT")!=NULL) || (strstr((char*)sim_rx_buffer,"CONNECT OK")!=NULL))
		con_status =1;
	clear_rx_buffer();
	
			/*
		Establish TCP connection
		Example AT+CIPSTART="TCP","52.58.13.47","4455"
		*/
	if (con_status == 0) {
		sprintf(gprs_TCP_connect_cmd,"AT+CIPSTART=\"TCP\",\"%s\",\"%s\"\r",host,port);
		send_cmd(gprs_TCP_connect_cmd);
	}
			
	
	
	/* Wait for TCP connection to be established */
	while(con_status==0 && timeout_counter<=TCP_CONNECT_TIMEOUT){
		HAL_Delay(500);/*Wait for connection to finish*/
		if(strstr(sim_rx_buffer,"CONNECT OK")!=NULL)
			con_status=1;
		else if (strstr(sim_rx_buffer,"CONNECT FAIL")!=NULL){ /*Connect fail means server port is not listening*/
			con_status=2; 
			break;
		}
		timeout_counter++;
	}
	clear_rx_buffer();
	timeout_counter=0;

		/*if connection is established send Data over TCP connection  */

	if (con_status==1){
		sprintf(gprs_data_send_cmd,"AT+CIPSEND=%d\r",length); 
		
		send_cmd(gprs_data_send_cmd);
		send_raw_cmd(data,length);
		
			
		/* Wait for send to finish */
	while(send_status==0 && timeout_counter<=TCP_CONNECT_TIMEOUT){
		HAL_Delay(200);/*Wait for send to finish*/
		if(strstr(sim_rx_buffer,"SEND OK")!=NULL)
			send_status=1;
		timeout_counter++;
	}
	
	clear_rx_buffer();
	
		/* TCP Disconnect */
		if (keep_con_open==0)
			send_cmd(gprs_TCP_disconnect_cmd);
		
		
		clear_rx_buffer();
	}
	
	return con_status;
}


uint8_t sim_mqtt_publish(char * host, char * port, char * topic, char * payload){
uint8_t connect_packet[]= {
	0x10, // Packet type = CONNECT
	0x10, // Remaining length = 16
	0x00, 0x04, // Protocol name length  
	0x4d, 0x51, 0x54, 0x54, // Protocol name = MQTT
	0x04, // Protocol Version 
	0x02, // Connect flags
	0x00, 0x3c, // Keep alive time in secondes = 60 s
	0x00, 0x04, // Client ID length
	0x46, 0x46, 0x46, 0x46 // Client ID
};
	

uint8_t disconnect_packet[] = {
	0xe0, // Packet type = DISCONNECT
	0x00  // Remaining length = 0
};

uint8_t publish_packet[] ={
	0x30, // Packet type = Publish + DUP+QOS+retain=0
	0x1d, // Remaining length = 29
	0x00, 0x04, // Topic name length
	0x46, 0x46, 0x46, 0x46, // Topic name = Client ID
	0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46 // dummy payload
};

for(uint8_t i=0;i<GPS_COORDINATES_LENGTH;i++){
publish_packet[i+8]=(uint8_t)payload[i];
}


sim_tcp_send(host,port,connect_packet,18,1);


sim_tcp_send(host,port,publish_packet,31,1);

sim_tcp_send(host,port,disconnect_packet,2,0);

return 0;
}