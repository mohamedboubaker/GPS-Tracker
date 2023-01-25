uint8_t sim_tcp_send(char * host, char * port, uint8_t * data, uint8_t length, uint8_t keep_con_open){
	
	static const char get_tcp_status_cmd[]="AT+CIPSTATUS\r";
	char tcp_connect_cmd[64]= "AT+CIPSTART=\"TCP\",\"";
	char send_tcp_data_cmd[24]= "AT+CIPSEND=";
	static const char tcp_disconnect_cmd[16]= "AT+CIPCLOSE\r";
	
	/* make sure to clear the buffer after every use */
	char local_rx_buffer[RX_BUFFER_LENGTH]; 

	/* Check underlying GPRS connection before trying to open a TCP connection.
	 * If it is disconnected, then try to enable it.
	 * if enabling GPRS fails, then return FAIL.
	 */
	sim_get_cmd_reply(get_tcp_status_cmd,local_rx_buffer,RX_WAIT);
	if ( !(strstr(local_rx_buffer,"IP STATUS") || strstr(local_rx_buffer,"CONNECT OK") || strstr(local_rx_buffer,"ALREADY CONNECT") || strstr(local_rx_buffer,"TCP CLOSED") ) )
		if (!sim_gprs_enable())
			return FAIL;
	/* clear receive buffer */
	memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
	
	/* Check connection Status.
	 * if there is no open connection, then open a new TCP connection.
	 */
	sim_get_cmd_reply(get_tcp_status_cmd,local_rx_buffer,RX_WAIT);
	if( !(strstr(local_rx_buffer,"ALREADY CONNECT") || strstr(local_rx_buffer,"CONNECT OK"))){
		
		/* clear receive buffer */
		memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		
		/* build the connect command by adding address and port */
		strcat(tcp_connect_cmd,host);    /* AT+CIPSTART=\"TCP\","host.com           */
		strcat(tcp_connect_cmd,"\",\""); /* AT+CIPSTART=\"TCP\","host.com","        */
		strcat(tcp_connect_cmd,port);    /* AT+CIPSTART=\"TCP\","host.com","port    */
		strcat(tcp_connect_cmd,"\"\r");  /* AT+CIPSTART=\"TCP\","host.com","port"\r */
		
		/*Send open connection command */
		sim_get_cmd_reply(tcp_connect_cmd,local_rx_buffer,RX_WAIT);
		
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