/** @file network_functions.c
*  @brief GPRS, TCP and MQTT functions implementation.
*
*  @author Mohamed Boubaker
*  @bug issue #9
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sim808.h"
#include "network_functions.h"

uint8_t sim_insert_PIN(char * pin){
	
	char PIN_insert_cmd[13]= "AT+CPIN=";
	uint8_t PIN_status=0;
	char local_rx_buffer[RX_BUFFER_LENGTH]; 
	
	/* build string PIN_insert_cmd = AT+CPIN=XXXX   SIM_PIN length is assumed = 4 */
	strcat(PIN_insert_cmd,SIM_PIN);   
	
	/* build string PIN_insert_cmd = AT+CPIN=XXXX\r */
	strcat(PIN_insert_cmd,"\r");      
	

	if (send_AT_cmd(PIN_insert_cmd,"OK",0,NULL,RX_TIMEOUT))
		return SUCCESS;
	else
		return SUCCESS;
}

/**
 * @brief gprs_enable() enables GPRS connection. 
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
uint8_t enable_gprs(){
	
	#ifdef DEBUG_MODE
	send_debug("Enable GPRS: Start");
	#endif
	/* Make sure to clear the buffer after every use */
	char local_rx_buffer[RX_BUFFER_LENGTH]; 

	/* Some tests are performed 3 times, trials_counter keep track of how many trials took place */
	uint8_t trials_counter=0;
	
	
	
	/*** Check if phone functionality of the module is enabled ***/
	

	static const char phone_status_check_cmd[]= "AT+CFUN?\r";
	static const char enable_phone_function_cmd[]= "AT+CFUN=1\r";
	uint8_t is_phone_enabled=0;
	
	while(trials_counter <3){
		
		/* Check the reply of the module in local_rx_buffer to see if the phone is enabled.
		 * Save the status in phone_status
		 */
	
		#ifdef DEBUG_MODE
			send_debug("Check if phone functionality of the module is enabled: send AT+CFUN?");
		#endif
		is_phone_enabled= send_AT_cmd(phone_status_check_cmd,"+CFUN: 1",1,local_rx_buffer,RX_TIMEOUT); 

	
		/* Clear receive buffer*/
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		
			if (is_phone_enabled==1)
				break;
			else
				if (trials_counter>=3)
					return ERR_PHONE_FUNCTION;
				else
					{
						#ifdef DEBUG_MODE
							send_debug("Enable phone functionality: send AT+CFUN=1");
						#endif
						send_AT_cmd(enable_phone_function_cmd,"OK",FALSE,NULL,3*RX_TIMEOUT);
					}
					trials_counter++;
		}
		trials_counter=0;
		

	
	
	/*** Detect if SIM card is present ***/
		
	
	static const char SIM_detect_cmd[]= "AT+CSMINS?\r";

	#ifdef DEBUG_MODE
		send_debug("Detect if SIM card is present: send AT+CSMINS?");
	#endif
	send_AT_cmd(SIM_detect_cmd,"OK",1,local_rx_buffer,RX_TIMEOUT);
	
	if (!is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"+CSMINS: 0,1",sizeof("+CSMINS: 0,1")-1)) 
		return ERR_SIM_PRESENCE;
	/*clear buffer for next use*/
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH); 

	
	
	/*** Check if PIN code is required ***/
	

	static const char PIN_status_cmd[]= "AT+CPIN?\r";
	static char PIN_insert_cmd[13]= "AT+CPIN=";
	uint8_t pin_status=0;
	
	/* send command to Check if PIN is required*/
	#ifdef DEBUG_MODE
		send_debug("Check if PIN code is required: send AT+CPIN?");
	#endif
	send_AT_cmd(PIN_status_cmd,"OK",1,local_rx_buffer,RX_TIMEOUT);
	
	/* Note: the module replies READY if the PIN is not required*/
	/* If the PIN is required, then insert PIN, if the PIN is wrong then exit*/
	pin_status=is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"READY",sizeof("READY")-1); 
	if (pin_status==0){
		if (!sim_insert_PIN(SIM_PIN))
			return ERR_PIN_WRONG;
	}
	/* Clear receive buffer */
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH); 
	
	
	
	/*** Check GPRS signal quality ***/

	
	/* Note
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
	 * Note ! After the startup of the module, it will take around 5s to aquire signal strength report.
	 */
	static const char check_signal_cmd[]= "AT+CSQ\r";
	
	uint8_t signal_status=0;

		while(trials_counter <3){
			#ifdef DEBUG_MODE
			send_debug("Check GPRS signal quqality: send AT+CSQ");
			#endif
			send_AT_cmd(check_signal_cmd,"OK",1,local_rx_buffer,RX_TIMEOUT);
			/* Check the reply of the module in local_rx_buffer to see if the signal is weak.
			* Save the status in signal_status
			*/
			signal_status = !( is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"+CSQ: 0,",sizeof("+CSQ: 0,")-1) ||
				is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"99,",sizeof("99,")-1) );
			/* Clear receive buffer*/
			memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		
			if (signal_status)
				break;
			else
				if (trials_counter>=3)
					return ERR_WEAK_SIGNAL;
				/* if the signal is weak, wait 1 second before measuring again */
				else
					HAL_Delay(4000);
				trials_counter++;
		}
		trials_counter=0;
		
		
	/*** Check Network Registration Status ***/

	/* Note 
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
	uint8_t registration_status=0;
		
	while(trials_counter<3){
	/*Send command to check registration status*/
	#ifdef DEBUG_MODE
		send_debug("Check Network Registration Status: send AT+CREG?");
	#endif
	send_AT_cmd(check_registration_cmd,"OK",TRUE,local_rx_buffer,RX_TIMEOUT);
	
	/*check the reply to determine if ME is registered at home network or roaming */
		registration_status= is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)",1",sizeof(",1")-1) ||
			is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)",5",sizeof(",5")-1);
	
	/*Clear receive buffer*/
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);	
	
	if (registration_status)
		break;
	else{
		if (trials_counter >= 3)
			return ERR_REGISTRATION;
		/* send command to register ME to network */
			#ifdef DEBUG_MODE
				send_debug("Register to network: send AT+CREG=1");
			#endif
		send_AT_cmd(register_ME_cmd,"OK",0,NULL,5*RX_TIMEOUT);
	}
	trials_counter++;
	}
	trials_counter=0;
	
	
	
	/*** Check if GPRS is attached ***/

	
	static const char check_grps_attach_cmd[]= "AT+CGATT?\r";
	static const char grps_attach_cmd[]= "AT+CGATT=1\r";
	uint8_t is_pdp_attached=0;
	
			while(trials_counter <3){
				#ifdef DEBUG_MODE
					send_debug("Check if GPRS is attached: AT+CGATT?");
				#endif
				send_AT_cmd(check_grps_attach_cmd,"OK",TRUE,local_rx_buffer,RX_TIMEOUT);

			/* Check the reply of the module in local_rx_buffer to see if the signal is weak.
			* Save the status in signal_status
			*/
			is_pdp_attached = !is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"CGATT: 0",sizeof("CGATT: 0")-1);
			
				/* Clear receive buffer*/
			memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		
			if (is_pdp_attached)
				break;
			else
				if (trials_counter>=3)
					return ERR_GPRS_ATTACH;
				/* if the signal is weak, wait 1 second before measuring again */
				else{
					#ifdef DEBUG_MODE
					send_debug("Attach PDP: send AT+CGATT=1");
					#endif
					send_AT_cmd(grps_attach_cmd,"OK",FALSE,NULL,3*RX_TIMEOUT);
					}
					trials_counter++;
		}
		trials_counter=0;
		
	
		
	/* At this point, the module should be attached to GPRS.
	 * The sequence below of GPRS states  must be followed for a correct GPRS connection establishement
	 * [PDP DEACT] => AT+CIPSHUT => [IP INITIAL] => AT+CSTT="TM","","" => [IP START] => AT+CIICR => [IP GPRSACT] => AT+CIFSR => [IP STATUS]
	 */
		
	/*** Check if PDP context is deactivated [PDP DEACT] ***/
		

		
	/* This command can be used to check gprs or TCP state */
	static const char check_gprs_state_cmd[]= "AT+CIPSTATUS\r"; 
	static const char reset_PDP_cmd[]= "AT+CIPSHUT\r";
		
	uint8_t pdp_deactivated=0;
	
	#ifdef DEBUG_MODE
		send_debug("Check if PDP context is deactivated [PDP DEACT]: send  AT+CIPSTATUS");
	#endif
	send_AT_cmd(check_gprs_state_cmd,"OK",TRUE,local_rx_buffer,RX_TIMEOUT);

		
	/*When PDP is deactivated it is necessary to run  AT+CIPSHUT to bring the status to [IP INITIAL] */
	pdp_deactivated = is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"PDP DEACT",sizeof("PDP DEACT")-1);
	if ( pdp_deactivated ) {
		#ifdef DEBUG_MODE
		send_debug("[PDP DEACT] send  AT+CIPSHUT");
		#endif
		if(!send_AT_cmd(reset_PDP_cmd,"OK",FALSE,NULL,RX_TIMEOUT)) 
			return ERR_PDP_DEACTIVATED;
	}
	/*clear receive buffer*/
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
	
		
		
	/*** Check if PDP context is correctly defined ***/
		

		
	/* Note: 
	* command = AT+CSTT?
	* When no PDP context is not defined the reply is the default PDP: +CSTT: "CMNET","",""
	*/
	uint8_t pdp_defined=0;
	static const char check_PDP_context_cmd[]= "AT+CSTT?\r";
	
	#ifdef DEBUG_MODE
		send_debug("Check if PDP context is correctly defined: send AT+CSTT?");
	#endif
	send_AT_cmd(check_PDP_context_cmd,"OK",TRUE,local_rx_buffer,RX_TIMEOUT);
		
	pdp_defined = ! is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"CMNET",sizeof("CMNET")-1);
		
	if(pdp_defined==0){
		/* Build enable_PDP_context_cmd 
		 * APN Name length is very important in this implementation 
		 * APN_LENGTH must be adjusted when APN is changed in the header file
		 * 18 = the length of the command AT+CSTT="","",""\r without the APN name inserted
		 */
		static char define_PDP_context_cmd[APN_LENGTH+18]= "AT+CSTT=\"";
		strcat(define_PDP_context_cmd,APN); /* AT+CSTT="APN */
		strcat(define_PDP_context_cmd,"\",\"\",\"\"\r"); /* AT+CSTT="APN","","" */
		
		#ifdef DEBUG_MODE
			send_debug("define PDP context: send AT+CSTT=\"APN\",\"\",\"\"");
		#endif
		if(!send_AT_cmd(define_PDP_context_cmd,"OK",FALSE,NULL,RX_TIMEOUT)) 
			return ERR_PDP_DEFINE;
	}
	/*Clear receive buffer*/
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
	


	/*** Check if PDP context is active ***/ 
	

	
	/* Note:
	 * Check the GPRS status, if it is IP START, then activate PDP context
	 */
	
	uint8_t pdp_ready=0;
	static const char activate_PDP_context_cmd[]= "AT+CIICR\r";
		
	#ifdef DEBUG_MODE
	send_debug("Check if PDP context is active: send AT+CIPSTATUS");
	#endif
	send_AT_cmd(check_gprs_state_cmd,"OK",TRUE,local_rx_buffer,RX_TIMEOUT); 
	
	/* Check if PDP context is defined and ready to be activated == [IP START] */
	pdp_ready = is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"IP START",sizeof("IP START")-1) ;
	
	/* Clear receive buffer */
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);

	/*if PDP context is correctly defined then activate it */
	if ( pdp_ready==1 ){
			
	#ifdef DEBUG_MODE
		send_debug("Activate PDP:send AT+CIIR");
	#endif
		
		/* This command takes around 1 second to finish hence 1s wait time */ /*POSSIBLE BUG HERE*/
		if (!send_AT_cmd(activate_PDP_context_cmd,"",FALSE,NULL,1000)) {
			#ifdef DEBUG_MODE
			send_debug("Activate PDP: FAIL");
			#endif
			return ERR_PDP_ACTIVATE; 	
		}
	}	
	
	/*** Get IP address ***/ 
	/*
	 * check GPRS status, if it is IP GPRSACT, then request IP, otherwise abort.
	 * AT+CIFSR\r does not reply OK. it replies the IP address if successful or ERROR
	 */
	
	/* This command takes around 1 second to finish hence wait 1s after it is sent */
	uint8_t error=0;
	static const char get_IP_address_cmd[]= "AT+CIFSR\r";
	
	#ifdef DEBUG_MODE
		send_debug("Check IP status: send AT+CIPSTATUS");
	#endif
	send_AT_cmd(check_gprs_state_cmd,"OK",TRUE,local_rx_buffer,RX_TIMEOUT);
	
	if ( is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"IP GPRSACT",sizeof("IP GPRSACT")-1) ){
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		
		#ifdef DEBUG_MODE
			send_debug("Get IP address: send AT+CIFSR");
		#endif
		send_AT_cmd(get_IP_address_cmd,"OK",TRUE,local_rx_buffer,5*RX_TIMEOUT);
		
		error=is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"ERROR",sizeof("ERROR")-1);
		if ( error ) {
			#ifdef DEBUG_MODE
				send_debug("Get IP address: FAIL");
			#endif
			return ERR_GET_IP;
		}
	}
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
	
	
	
	/*** Check if GPRS connection is ready ***/
	
	uint8_t gprs_ready=0;
	
	#ifdef DEBUG_MODE
		send_debug(" Check if GPRS connection is ready: Send AT+CIPSTATUS");
	#endif
	
	send_AT_cmd(check_gprs_state_cmd,"OK",TRUE,local_rx_buffer,RX_TIMEOUT);
	
	gprs_ready=(  
		is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"IP STATUS",sizeof("IP STATUS")-1)  || 
		is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"TCP CONNECTING",sizeof("TCP CONNECTING")-1) || 
		is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"CONNECT",sizeof("CONNECT OK")-1) ||
		is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"ALREADY CONNECT",sizeof("ALREADY CONNECT")-1)
			);
	
	if (gprs_ready){
		#ifdef DEBUG_MODE
		send_debug("Internet Connection: Ready");
		#endif
		return SUCCESS;
	}
	else{ 
		#ifdef DEBUG_MODE
		
		send_debug("Internet Connection: FAIL");
		send_debug(local_rx_buffer);
		#endif
		return FAIL;
	}
}






uint8_t open_tcp_connection(char * server_address, char * port){
	static const char get_tcp_status_cmd[]="AT+CIPSTATUS\r";
	char tcp_connect_cmd[128]= "AT+CIPSTART=\"TCP\",\"";
	char send_tcp_data_cmd[24]= "AT+CIPSEND=";
	static const char tcp_disconnect_cmd[]= "AT+CIPCLOSE\r";
	
	uint8_t tcp_ready=0;
	
	/* make sure to clear the buffer after every use */
	char local_rx_buffer[RX_BUFFER_LENGTH]; 

	/* Note 
	 * At this point, all TCP connections should be closed. but in case of imporpper termination TCP connection
	 * There is a chance that the connection is still open on the client's side or server's side
	 * There is a chance that the connection from the client is closed, but on the server it is open 
	 * In this case, if the client tries to reopen a new connection using the same source TCP port, maybe the server will deny
	 * To avoid this case, it is recommended that the TCP keepalive timeout is should be decreased to 10 seconds on the server
	 * Which means, if the server doesn't receive any data through a tcp connection in 10 seconds, then it is closed.
	 */
	
		#ifdef DEBUG_MODE
			send_debug("Open TCP connection: START");
		#endif
	
	
	/*** Check TCP/GPRS Status ***/
		#ifdef DEBUG_MODE
			send_debug("Check current TCP status: send AT+CIPSTATUS");
		#endif
	send_AT_cmd(get_tcp_status_cmd,"OK",TRUE,local_rx_buffer,RX_TIMEOUT);
	
	/* If the TCP/GPRS stack is not in usable status, then enable GPRS 
	 * else if there is an open TCP connection then close it.
	 */
	 
	 send_debug(local_rx_buffer);
	if ( 
		is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"IP INITIAL",sizeof("IP INITIAL")-1)	||
		is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"IP START",sizeof("IP START")-1)	||
		is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"IP CONFIG",sizeof("IP CONFIG")-1) || 
		is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"IP GPRSACT",sizeof("IP GPRSACT")-1)	|| 
		is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"PDP DEACT",sizeof("PDP DEACT")-1)
		)			
	{	
		#ifdef DEBUG_MODE
		send_debug("TCP cannot begin because GPRS is not ready: call enable_gprs();");
		#endif	
		enable_gprs();
	}
	else if ( 
		is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"TCP CONNECTING",sizeof("TCP CONNECTING")-1)	||
		is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"CONNECT OK",sizeof("CONNECT OK")-1)	||
		is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"ALREADY CONNECT",sizeof("ALREADY CONNECT")-1) 
		)
	{
		#ifdef DEBUG_MODE
			send_debug("Open TCP connection detected, terminating it. send: AT+CIPCLOSE");
		#endif	
		send_AT_cmd(tcp_disconnect_cmd,"OK",FALSE,NULL,RX_TIMEOUT);
	}
	/*clear receive buffer */
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);

	
	/*** Open TCP connection ***/
	
	/* build the connect command by adding address and port */
	strcat(tcp_connect_cmd,server_address);    /* AT+CIPSTART=\"TCP\","host.com           */
	strcat(tcp_connect_cmd,"\",\""); /* AT+CIPSTART=\"TCP\","host.com","        */
	strcat(tcp_connect_cmd,port);    /* AT+CIPSTART=\"TCP\","host.com","port    */
	strcat(tcp_connect_cmd,"\"\r");  /* AT+CIPSTART=\"TCP\","host.com","port"\r */
		
	/*Send open connection command Wait for connection to establish or fail*/
		#ifdef DEBUG_MODE
			send_debug("Attempt to open TCP connection");
		#endif	
	if (send_AT_cmd(tcp_connect_cmd,"CONNECT OK",1,local_rx_buffer,RX_TIMEOUT)){
		#ifdef DEBUG_MODE
			send_debug("Open TCP connection : OK");
		#endif
		return SUCCESS;
	}
		else{
			#ifdef DEBUG_MODE
				send_debug("Open TCP connection : FAIL ");
				send_debug("Buffer content below:");
				send_debug(local_rx_buffer);	
			#endif
		}
	/* check the reply, if CONNECT FAIL or ERROR is returned, it means the connection failed to establish. 
	 * If TCP CONNECTING is returned, it means the module initiated TCP handshake but still waiting for handshake acknoledgement
	 * usuallly it means the peer server is offline, so in this case, close the connection and exit */


	if ( is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"CONNECT FAIL",sizeof("CONNECT FAIL")-1) || is_subarray_present((uint8_t*)local_rx_buffer,RX_BUFFER_LENGTH,(uint8_t*)"ERROR",sizeof("ERROR")-1)){
		return FAIL;
	}
	else 	{
		#ifdef DEBUG_MODE
		send_debug("Open TCP connection timeout. disconnecting. send: AT+CIPCLOSE");
		#endif
		send_AT_cmd(tcp_disconnect_cmd,"CLOSED OK",0,NULL,RX_TIMEOUT);{
		return FAIL;
		}
	}
	
	
}


uint8_t close_tcp_connection(){
	
		char local_rx_buffer[RX_BUFFER_LENGTH]; 
		static const char tcp_disconnect_cmd[]= "AT+CIPCLOSE\r";


		/* send Close TCP connection command */
		/* and return if TCP connection was closed correctly */	
		#ifdef DEBUG_MODE
			send_debug("Close TCP connection: send AT+CIPCLOSE");
		#endif
	if ((send_AT_cmd(tcp_disconnect_cmd,"CLOSE OK",FALSE,NULL,RX_TIMEOUT)) )
		return SUCCESS;
	else 
		return FAIL;
	
}


uint8_t send_tcp_data(uint8_t * data, uint8_t data_length){

	static const char get_tcp_status_cmd[]="AT+CIPSTATUS\r";
	char send_tcp_data_cmd[24]= "AT+CIPSEND=";
	char local_rx_buffer[RX_BUFFER_LENGTH]; 
	

	/*Construct the command that sends "data_length" bytes */
	sprintf(send_tcp_data_cmd,"AT+CIPSEND=%d\r",(int)data_length); 
	
	
	#ifdef DEBUG_MODE
	send_debug("Initiate TCP transmission: send AT+CIPSEND=");
	#endif
	
	/* tell the module how many bytes to expect */
	send_AT_cmd(send_tcp_data_cmd,">",FALSE,NULL,RX_TIMEOUT);

	#ifdef DEBUG_MODE
	send_debug("Sending TCP load");
	#endif
	
	/*Send the actual data and return the status of transmission*/
	return send_serial_data(data,data_length,local_rx_buffer,RX_TIMEOUT); 
	

}


uint8_t publish_mqtt_msg(char * ip_address, char *  tcp_port, char * topic, char * client_id, char * message){
	
	#ifdef DEBUG_MODE
		send_debug("MQTT protocol: START");
	#endif
	
	/*** Construct the Connect packet ***/

	/* Connect Packet structure:  
	 * 1 byte          : [Packet type] = 0x10
	 * 1 byte          : [Remaining length] 
	 * 2 byte          : [Protocol name length] = 0x00, 0x04 (4 in decimal)
	 * 4 byte          : [Protocol name] = 0x4d, 0x51, 0x54, 0x54 (MQTT is ASCII)
	 * 1 byte          : [Protocol Version] = 0x04 (4 in decimal) 
	 * 1 byte          : [Connect flags] 
	 * 2 bytes         : [Keep alive timeout]
	 * 2 bytes         : [Client ID length]
	 * remaining bytes : [Client ID]
	 */
	
	
	uint16_t client_id_length = strlen(client_id);
	uint8_t connect_packet_remaining_length = 12 + client_id_length;;
	uint16_t keep_alive = MQTT_KEEP_ALIVE;
		
	uint8_t connect_packet[MAX_LENGTH_MQTT_PACKET]= {
	0x10, // Packet type = CONNECT
	0x10, // Remaining length = 16
	0x00, 0x04, // Protocol name length  
	0x4d, 0x51, 0x54, 0x54, // Protocol name = MQTT
	0x04, // Protocol Version 
	0x02 // Connect flags
	};



	connect_packet[1]=connect_packet_remaining_length;
	
	/* Insert Keep alive time most signinficant byte in the packet by shifting keep_alive 8 bits to the right and casting into uint8_t */
	connect_packet[10]= (uint8_t) (keep_alive>>8);

	/* Insert Keep alive time least significant byte in the packet by directly casting the uint16_t variable to uint8_t which will will clamp the left 8 bits */
	connect_packet[11]= (uint8_t) keep_alive;

	/* Insert client ID length into the packet with same way used for keep_alive */ 
		connect_packet[12]= (uint8_t) (client_id_length>>8);
		connect_packet[13]= (uint8_t) client_id_length;
	
	/* Copy the client ID char by char into the connect packet */
	for(uint8_t i = 0; i< client_id_length ; i++)
		connect_packet[14+i]=(uint8_t)client_id[i];




	/*** Construct the Publish Packet ***/

	uint16_t topic_length = strlen(topic);
	uint8_t publish_packet_remaining_length= 2 + topic_length + (uint8_t)strlen(message) ;
	

	uint8_t publish_packet[MAX_LENGTH_MQTT_PACKET] ={
		0x30, // Packet type = Publish + DUP+QOS+retain=0
		0x1d, // Remaining length dummy example 0x1d = 29
		0x00, 0x04 // Topic name length dummy example = 4
	};
	
	/*insert remaining length */
	publish_packet[1] = publish_packet_remaining_length;
	
	/* Insert topic name length into the packet with same way used for keep_alive */ 
	publish_packet[2]= (uint8_t) (topic_length>>8);
	publish_packet[3]= (uint8_t) topic_length;


	/* Copy the topic  name char by char into the publish packet */
	for(uint8_t i = 0; i< topic_length ; i++)
		publish_packet[4+i]=(uint8_t)topic[i];

	/* Copy the message   char by char into the publish packet */
	for(uint8_t i = 0; i< strlen(message); i++)
		publish_packet[4+topic_length+i]=(uint8_t)message[i];
	

	
/*** Construct Disconnect Packet ***/
	
	uint8_t disconnect_packet[] = {
		0xe0, // Packet type = DISCONNECT
		0x00 // Remaining length = 0
	};	

	
	
	#ifdef DEBUG_MODE
		send_debug("***CONNECT packet content:***");
		send_raw_debug(connect_packet,14+client_id_length);	
		send_debug("***PUBLISH packet content:***");
		send_raw_debug(publish_packet,publish_packet_remaining_length+2);
	#endif
	
		#ifdef DEBUG_MODE
	//	send_debug("PUBLLISH packet content:");
	//	send_debug(publish_packet);
	#endif
	
		#ifdef DEBUG_MODE
	//	send_debug("DISCONNECT packet content:");
		//send_debug(disconnect_packet);
	#endif
	
	/*** Sending Data ***/
		if (open_tcp_connection(ip_address,tcp_port)){
			
			//send_tcp_data((uint8_t *)"hello",5);
			#ifdef DEBUG_MODE
				send_debug("Sending MQTT CONNECT Packet");
			#endif
			send_tcp_data(connect_packet,2+connect_packet_remaining_length);

			#ifdef DEBUG_MODE
				send_debug("Sending MQTT PUBLISH Packet");
			#endif
				send_tcp_data(publish_packet,2+publish_packet_remaining_length);
			//send_tcp_data((uint8_t *)"hello",5);
			#ifdef DEBUG_MODE
				send_debug("Sending MQTT DISCONNECT Packet");
			#endif
			send_tcp_data(disconnect_packet,2);
			
			close_tcp_connection();
			return SUCCESS;
		}
			
	return FAIL;
}
