/** @file network_functions.c
*  @brief this file include the implementation of functions related to GPRS, TCP and MQTT
*
*  @author Mohamed Boubaker
*  @bug issue #9
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "network_functions.h"


uint8_t sim_insert_PIN(char * pin){
	
	char PIN_insert_cmd[13]= "AT+CPIN=";
	uint8_t PIN_status=0;
	char local_rx_buffer[RX_BUFFER_LENGTH]; 
	
	/* build string PIN_insert_cmd = AT+CPIN=XXXX   SIM_PIN length is assumed = 4 */
	strcat(PIN_insert_cmd,SIM_PIN);   
	
	/* build string PIN_insert_cmd = AT+CPIN=XXXX\r */
	strcat(PIN_insert_cmd,"\r");      
	

	sim_get_cmd_reply(PIN_insert_cmd,local_rx_buffer,RX_WAIT);
	if (!strstr(local_rx_buffer,"OK")) 
		return FAIL;

	return SUCCESS;
}

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
uint8_t sim_enable_gprs(){
	
	/* Make sure to clear the buffer after every use */
	char local_rx_buffer[RX_BUFFER_LENGTH]; 

	/* Some tests are performed 3 times, trials_counter keep track of how many trials took place */
	uint8_t trials_counter=0;
	
	
	
	/*** Check if phone functionality of the module is enabled ***/
	
	static const char phone_status_check_cmd[]= "AT+CFUN?\r";
	static const char enable_phone_function_cmd[]= "AT+CFUN=1\r";
	uint8_t phone_status=0;
	
	while(trials_counter <3){
		sim_get_cmd_reply(phone_status_check_cmd,local_rx_buffer,RX_WAIT); 
		/* Check the reply of the module in local_rx_buffer to see if the phone is enabled.
		 * Save the status in phone_status
		 */
		phone_status = (strstr(local_rx_buffer,"+CFUN: 1")==NULL?0:1);
			
		/* Clear receive buffer*/
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		
			if (phone_status)
				break;
			else
				if (trials_counter>=3)
					return ERR_PHONE_FUNCTION;
				else
					send_cmd(enable_phone_function_cmd,3*RX_WAIT);
				trials_counter++;
		}
		trials_counter=0;
		

	
	
	/*** Detect if SIM card is present ***/
		
	static const char SIM_detect_cmd[]= "AT+CSMINS?\r";
	sim_get_cmd_reply(SIM_detect_cmd,local_rx_buffer,RX_WAIT);
	if (!strstr(local_rx_buffer,"+CSMINS: 0,1")) 
		return ERR_SIM_PRESENCE;
	/*clear buffer for next use*/
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH); 

	
	
	/*** Check if PIN code is required ***/
	
	static const char PIN_status_cmd[]= "AT+CPIN?\r";
	static char PIN_insert_cmd[13]= "AT+CPIN=";
	uint8_t pin_status=0;
	/* send command to Check if PIN is required*/
	sim_get_cmd_reply(PIN_status_cmd,local_rx_buffer,RX_WAIT);
	
	/* Note: the module replies READY if the PIN is not required*/
	/* If the PIN is required, then insert PIN, if the PIN is wrong then exit*/
	pin_status=strstr(local_rx_buffer,"READY")==NULL?0:1;
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
			sim_get_cmd_reply(check_signal_cmd,local_rx_buffer,RX_WAIT);
			/* Check the reply of the module in local_rx_buffer to see if the signal is weak.
			* Save the status in signal_status
			*/
			signal_status = !( (strstr(local_rx_buffer,"+CSQ: 0,")==NULL?0:1) || (strstr(local_rx_buffer,"99,")==NULL?0:1)) ;
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
	sim_get_cmd_reply(check_registration_cmd,local_rx_buffer,RX_WAIT);
	
	/*check the reply to determine if ME is registered at home network or roaming */
		registration_status= (strstr(local_rx_buffer,",1")==NULL?0:1) || (strstr(local_rx_buffer,",5")==NULL?0:1);
	
	/*Clear receive buffer*/
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);	
	
	if (registration_status)
		break;
	else{
		if (trials_counter >= 3)
			return ERR_REGISTRATION;
		/* send command to register ME to network */
		send_cmd(register_ME_cmd,5*RX_WAIT);
	}
	trials_counter++;
	}
	trials_counter=0;
	
	
	
	/*** Check if GPRS is attached ***/
	
	static const char check_grps_attach_cmd[]= "AT+CGATT?\r";
	static const char grps_attach_cmd[]= "AT+CGATT=1\r";
	uint8_t attach_status=0;
	
			while(trials_counter <3){
				sim_get_cmd_reply(check_grps_attach_cmd,local_rx_buffer,RX_WAIT);

			/* Check the reply of the module in local_rx_buffer to see if the signal is weak.
			* Save the status in signal_status
			*/
			attach_status = !(strstr(local_rx_buffer,"CGATT: 0")==NULL?0:1);
			
				/* Clear receive buffer*/
			memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		
			if (attach_status)
				break;
			else
				if (trials_counter>=3)
					return ERR_GPRS_ATTACH;
				/* if the signal is weak, wait 1 second before measuring again */
				else
					send_cmd(grps_attach_cmd,4*RX_WAIT);
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
	
	sim_get_cmd_reply(check_gprs_state_cmd,local_rx_buffer,RX_WAIT);
		
	/*When PDP is deactivated it is necessary to run  AT+CIPSHUT to bring the status to [IP INITIAL] */
	pdp_deactivated = strstr(local_rx_buffer,"PDP DEACT")==NULL?0:1;
	if ( pdp_deactivated ) 
		if(!send_cmd(reset_PDP_cmd,RX_WAIT)) 
			return ERR_PDP_DEACTIVATED;
	
	/*clear receive buffer*/
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
	
		
		
	/*** Check if PDP context is correctly defined ***/
	/* Note: 
	* command = AT+CSTT?
	* When no PDP context is not defined the reply is the default PDP: +CSTT: "CMNET","",""
	*/
	uint8_t pdp_defined=0;
	static const char check_PDP_context_cmd[]= "AT+CSTT?\r";
	sim_get_cmd_reply(check_PDP_context_cmd,local_rx_buffer,RX_WAIT);
		
	pdp_defined = strstr(local_rx_buffer,"CMNET")==NULL?1:0;
		
	if(pdp_defined==0){
		/* Build enable_PDP_context_cmd 
		 * APN Name length is very important in this implementation 
		 * APN_LENGTH must be adjusted when APN is changed in the header file
		 * 18 = the length of the command AT+CSTT="","",""\r without the APN name inserted
		 */
		static char define_PDP_context_cmd[APN_LENGTH+18]= "AT+CSTT=\"";
		strcat(define_PDP_context_cmd,APN); /* AT+CSTT="APN */
		strcat(define_PDP_context_cmd,"\",\"\",\"\"\r"); /* AT+CSTT="APN","","" */
		if(!send_cmd(define_PDP_context_cmd,RX_WAIT)) 
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
	
	sim_get_cmd_reply(check_gprs_state_cmd,local_rx_buffer,RX_WAIT); 
	
	/* Check if PDP context is defined and ready to be activated == [IP START] */
	pdp_ready = strstr(local_rx_buffer,"IP START")==NULL?0:1;
	
	/* Clear receive buffer */
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);

	/*if PDP context is correctly defined then activate it */
	if ( pdp_ready==1 ){
		
		/* This command takes around 1 second to finish hence 1s wait time */
		if (!send_cmd(activate_PDP_context_cmd,1000)) 
			return ERR_PDP_ACTIVATE; 	
	}	
	
	/*** Get IP address ***/ 
	/*
	 * check GPRS status, if it is IP GPRSACT, then request IP, otherwise abort.
	 * AT+CIFSR\r does not reply OK. it replies the IP address if successful or ERROR
	 */
	
	/* This command takes around 1 second to finish hence wait 1s after it is sent */
	uint8_t error=0;
	static const char get_IP_address_cmd[]= "AT+CIFSR\r";
	
	sim_get_cmd_reply(check_gprs_state_cmd,local_rx_buffer,RX_WAIT);
	
	if ( strstr(local_rx_buffer,"IP GPRSACT") ){
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		
		sim_get_cmd_reply(get_IP_address_cmd,local_rx_buffer,5*RX_WAIT);
		
		error=strstr(local_rx_buffer,"ERROR")==NULL?0:1;
		if ( error ) 
			return ERR_GET_IP;
	}
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
	
	
	
	/*** Check if GPRS connection is ready ***/
	
	uint8_t gprs_ready=0;
	
	sim_get_cmd_reply(check_gprs_state_cmd,local_rx_buffer,RX_WAIT);
	
	gprs_ready=strstr(local_rx_buffer,"IP STATUS")==NULL?0:1;
	
	if (gprs_ready)
		return SUCCESS;
	else 
		return FAIL;
}


/**
 * @brief sends raw data over a TCP connection. 
 * if the function is called when there is already an open TCP connection (for some reason)
 * then it closes it and opens a new connection. 
 * @param server_address is the remote TCP peer address. it can be an IP adress or DNS name
 * @param port is the remote TCP port
 * @param data is the data to be sent
 * @param length is the length of the data to be sent in bytes
 * @return 1 if data is successfully sent, 0 otherwise
 */
uint8_t sim_tcp_send(char * host, char * port, char * data, char * length){
	
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
	
	
	
	/*** Check TCP/GPRS Status ***/
	sim_get_cmd_reply(get_tcp_status_cmd,local_rx_buffer,RX_WAIT);
	
	/* If the TCP/GPRS stack is not in usable status, then enable GPRS 
	 * else if there is an open TCP connection then close it.
	 */
	 

	if ( (strstr(local_rx_buffer,"IP INITIAL")==NULL?0:1) || (strstr(local_rx_buffer,"IP START")==NULL?0:1) || (strstr(local_rx_buffer,"IP CONFIG")==NULL?0:1)  || (strstr(local_rx_buffer,"IP GPRSACT")==NULL?0:1) || (strstr(local_rx_buffer,"PDP DEACT")==NULL?0:1) ) 
		sim_enable_gprs();
		
	else if ( (strstr(local_rx_buffer,"TCP CONNECTING")==NULL?0:1) || (strstr(local_rx_buffer,"CONNECT OK")==NULL?0:1)   || (strstr(local_rx_buffer,"ALREADY CONNECT")==NULL?0:1)  )
		send_cmd(tcp_disconnect_cmd,RX_WAIT);
	
	/*clear receive buffer */
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);



	
	/*** Open TCP connection ***/
	
	/* build the connect command by adding address and port */
	strcat(tcp_connect_cmd,host);    /* AT+CIPSTART=\"TCP\","host.com           */
	strcat(tcp_connect_cmd,"\",\""); /* AT+CIPSTART=\"TCP\","host.com","        */
	strcat(tcp_connect_cmd,port);    /* AT+CIPSTART=\"TCP\","host.com","port    */
	strcat(tcp_connect_cmd,"\"\r");  /* AT+CIPSTART=\"TCP\","host.com","port"\r */
		
		/*Send open connection command */
		sim_get_cmd_reply(tcp_connect_cmd,local_rx_buffer,RX_WAIT);
		
		/* Wait for connection to establish or fail*/
	HAL_Delay(1000);
	
	/*clear buffer*/
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		
	/*check if the connection was established or failed */	
	sim_get_cmd_reply(get_tcp_status_cmd,local_rx_buffer,RX_WAIT);

	uint8_t tcp_fail_to_open = 0;

	tcp_fail_to_open = (strstr(local_rx_buffer,"FAIL")==NULL?0:1);
	send_debug(local_rx_buffer,RX_WAIT);

	if (tcp_fail_to_open==1){
		return FAIL;
	}
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
	
	/*Construct the command that sends N bytes */
	strcat(send_tcp_data_cmd,length);
	strcat(send_tcp_data_cmd,"\r");
	
	/*Send open TCP connection command */
	sim_get_cmd_reply(send_tcp_data_cmd,local_rx_buffer,RX_WAIT);
	
	/*Send the data */
	send_serial(data,(uint8_t)atoi(length),RX_WAIT);
	HAL_Delay(1000);

	/*** close TCP connections ***/
	send_cmd(tcp_disconnect_cmd,RX_WAIT);
	
	return SUCCESS;
}