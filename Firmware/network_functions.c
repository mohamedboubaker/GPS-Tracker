/** @file network_functions.c
*  @brief GPRS, TCP and MQTT functions implementation.
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
	
	/* Make sure to clear the buffer after every use */
	char local_rx_buffer[RX_BUFFER_LENGTH]; 

	/* Some tests are performed 3 times, trials_counter keep track of how many trials took place */
	uint8_t trials_counter=0;
	
	
	
	/*** Check if phone functionality of the module is enabled ***/
	
	static const char phone_status_check_cmd[]= "AT+CFUN?\r";
	static const char enable_phone_function_cmd[]= "AT+CFUN=1\r";
	uint8_t phone_status=0;
	
	while(trials_counter <3){
		
		/* Check the reply of the module in local_rx_buffer to see if the phone is enabled.
		 * Save the status in phone_status
		 */
		phone_status= send_AT_cmd(phone_status_check_cmd,"+CFUN: 1",1,local_rx_buffer,RX_WAIT); 

	
		/* Clear receive buffer*/
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		
			if (phone_status)
				break;
			else
				if (trials_counter>=3)
					return ERR_PHONE_FUNCTION;
				else
					send_AT_cmd(enable_phone_function_cmd,"OK",FALSE,NULL,3*RX_WAIT);
				trials_counter++;
		}
		trials_counter=0;
		

	
	
	/*** Detect if SIM card is present ***/
		
	static const char SIM_detect_cmd[]= "AT+CSMINS?\r";
	send_AT_cmd(SIM_detect_cmd,"OK",1,local_rx_buffer,RX_WAIT);
	if (!strstr(local_rx_buffer,"+CSMINS: 0,1")) 
		return ERR_SIM_PRESENCE;
	/*clear buffer for next use*/
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH); 

	
	
	/*** Check if PIN code is required ***/
	
	static const char PIN_status_cmd[]= "AT+CPIN?\r";
	static char PIN_insert_cmd[13]= "AT+CPIN=";
	uint8_t pin_status=0;
	/* send command to Check if PIN is required*/
	send_AT_cmd(PIN_status_cmd,"OK",1,local_rx_buffer,RX_WAIT);
	
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
			send_AT_cmd(check_signal_cmd,"OK",1,local_rx_buffer,RX_WAIT);
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
	send_AT_cmd(check_registration_cmd,"OK",TRUE,local_rx_buffer,RX_WAIT);
	
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
		send_AT_cmd(register_ME_cmd,"OK",0,NULL,5*RX_WAIT);
	}
	trials_counter++;
	}
	trials_counter=0;
	
	
	
	/*** Check if GPRS is attached ***/
	
	static const char check_grps_attach_cmd[]= "AT+CGATT?\r";
	static const char grps_attach_cmd[]= "AT+CGATT=1\r";
	uint8_t attach_status=0;
	
			while(trials_counter <3){
				send_AT_cmd(check_grps_attach_cmd,"OK",TRUE,local_rx_buffer,RX_WAIT);

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
					send_AT_cmd(grps_attach_cmd,"OK",FALSE,NULL,4*RX_WAIT);
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
	
	send_AT_cmd(check_gprs_state_cmd,"OK",TRUE,local_rx_buffer,RX_WAIT);
		
	/*When PDP is deactivated it is necessary to run  AT+CIPSHUT to bring the status to [IP INITIAL] */
	pdp_deactivated = strstr(local_rx_buffer,"PDP DEACT")==NULL?0:1;
	if ( pdp_deactivated ) 
		if(!send_AT_cmd(reset_PDP_cmd,"OK",FALSE,NULL,RX_WAIT)) 
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
	send_AT_cmd(check_PDP_context_cmd,"OK",TRUE,local_rx_buffer,RX_WAIT);
		
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
		if(!send_AT_cmd(define_PDP_context_cmd,"OK",FALSE,NULL,RX_WAIT)) 
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
	
	send_AT_cmd(check_gprs_state_cmd,"OK",TRUE,local_rx_buffer,RX_WAIT); 
	
	/* Check if PDP context is defined and ready to be activated == [IP START] */
	pdp_ready = strstr(local_rx_buffer,"IP START")==NULL?0:1;
	
	/* Clear receive buffer */
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);

	/*if PDP context is correctly defined then activate it */
	if ( pdp_ready==1 ){
		
		/* This command takes around 1 second to finish hence 1s wait time */
		if (!send_AT_cmd(activate_PDP_context_cmd,"OK",FALSE,NULL,1000)) 
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
	
	send_AT_cmd(check_gprs_state_cmd,"OK",TRUE,local_rx_buffer,RX_WAIT);
	
	if ( strstr(local_rx_buffer,"IP GPRSACT") ){
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		
		send_AT_cmd(get_IP_address_cmd,"OK",TRUE,local_rx_buffer,5*RX_WAIT);
		
		error=strstr(local_rx_buffer,"ERROR")==NULL?0:1;
		if ( error ) 
			return ERR_GET_IP;
	}
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
	
	
	
	/*** Check if GPRS connection is ready ***/
	
	uint8_t gprs_ready=0;
	
	send_AT_cmd(check_gprs_state_cmd,"OK",TRUE,local_rx_buffer,RX_WAIT);
	
	gprs_ready=strstr(local_rx_buffer,"IP STATUS")==NULL?0:1;
	
	if (gprs_ready)
		return SUCCESS;
	else 
		return FAIL;
}



uint8_t publish_mqtt_msg(char * ip_address, char *  tcp_port, char * topic, char * client_id, char * message){
	
	/* sanity check */
	/* if topic or client_id or message is an empty string , return an error */
	if ((sizeof(topic)==0) || (sizeof(client_id)==0) ||  (sizeof(message)==0))
			return ERR_MQTT_EMPTY_PARAM;
	
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
	
	

uint8_t disconnect_packet[] = {
	0xe0, // Packet type = DISCONNECT
	0x00  // Remaining length = 0
};

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

	

	uint16_t topic_length = strlen(topic);
	uint8_t publish_packet_remaining_length=0;

uint8_t publish_packet[MAX_LENGTH_MQTT_PACKET] ={
	0x30, // Packet type = Publish + DUP+QOS+retain=0
	0x1d, // Remaining length = 29
	0x00, 0x04, // Topic name length
	0x46, 0x46, 0x46, 0x46, // Topic name = Client ID
	0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46 // dummy payload
};
	
	/*insert remaining length */
	publish_packet_remaining_length = 2 + topic_length + strlen(message);
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
	
	
	/*** Sending Data ***/
	
		if (open_tcp_connection(ip_address,tcp_port)){
			send_tcp_data(connect_packet,14+client_id_length);
			send_tcp_data(publish_packet,publish_packet_remaining_length+2);
			send_tcp_data(disconnect_packet,2);
			close_tcp_connection();
			return SUCCESS;
		}
			
	return FAIL;
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
	
	
	
	/*** Check TCP/GPRS Status ***/
	send_AT_cmd(get_tcp_status_cmd,"OK",TRUE,local_rx_buffer,RX_WAIT);
	
	/* If the TCP/GPRS stack is not in usable status, then enable GPRS 
	 * else if there is an open TCP connection then close it.
	 */
	 
	if ( (strstr(local_rx_buffer,"IP INITIAL")==NULL?0:1) || (strstr(local_rx_buffer,"IP START")==NULL?0:1) || (strstr(local_rx_buffer,"IP CONFIG")==NULL?0:1)  || (strstr(local_rx_buffer,"IP GPRSACT")==NULL?0:1) || (strstr(local_rx_buffer,"PDP DEACT")==NULL?0:1) ) 
		enable_gprs();
		
	else if ( (strstr(local_rx_buffer,"TCP CONNECTING")==NULL?0:1) || (strstr(local_rx_buffer,"CONNECT OK")==NULL?0:1)   || (strstr(local_rx_buffer,"ALREADY CONNECT")==NULL?0:1)  )
		send_AT_cmd(tcp_disconnect_cmd,"OK",FALSE,NULL,RX_WAIT);
	
	/*clear receive buffer */
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);

	
	/*** Open TCP connection ***/
	
	/* build the connect command by adding address and port */
	strcat(tcp_connect_cmd,server_address);    /* AT+CIPSTART=\"TCP\","host.com           */
	strcat(tcp_connect_cmd,"\",\""); /* AT+CIPSTART=\"TCP\","host.com","        */
	strcat(tcp_connect_cmd,port);    /* AT+CIPSTART=\"TCP\","host.com","port    */
	strcat(tcp_connect_cmd,"\"\r");  /* AT+CIPSTART=\"TCP\","host.com","port"\r */
		
	/*Send open connection command Wait for connection to establish or fail*/
	send_AT_cmd(tcp_connect_cmd,"CONNECT OK",1,local_rx_buffer,5*RX_WAIT);
	send_debug(local_rx_buffer);	
	
	/* check the reply, if CONNECT FAIL or ERROR is returned, it means the connection failed to establish. 
	 * If TCP CONNECTING is returned, it means the module initiated TCP handshake but still waiting for handshake acknoledgement
	 * usuallly it means the peer server is offline, so in this case, close the connection and exit */

	if ((strstr(local_rx_buffer,"CONNECT OK")==NULL?0:1))
		return SUCCESS;
	else if ((strstr(local_rx_buffer,"CONNECT FAIL")==NULL?0:1) || (strstr(local_rx_buffer,"ERROR")==NULL?0:1))
		return FAIL;
	else 	{
		send_cmd(tcp_disconnect_cmd,RX_WAIT);
		return FAIL;
	}
	
	
}


uint8_t close_tcp_connection(){
	
		char local_rx_buffer[RX_BUFFER_LENGTH]; 
		static const char tcp_disconnect_cmd[]= "AT+CIPCLOSE\r";


		/* send Close TCP connection command */
		/* and return if TCP connection was closed correctly */	
	
	if ((send_AT_cmd(tcp_disconnect_cmd,"CLOSE OK",FALSE,NULL,RX_WAIT)) )
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
	
	/* tell the module how many bytes to expect */
	send_AT_cmd(send_tcp_data_cmd,"OK",FALSE,NULL,RX_WAIT);

	/*Send the actual data and return the status of transmission*/
	return send_serial_data(data,data_length,local_rx_buffer,5*RX_WAIT); 
	

}
