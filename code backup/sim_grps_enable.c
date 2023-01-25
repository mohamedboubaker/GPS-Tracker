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