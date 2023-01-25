		while(trials_counter <3){
			sim_get_cmd_reply(cmd,local_rx_buffer,RX_WAIT);
			/* Check the reply of the module in local_rx_buffer to see if the signal is weak.
			* Save the status in signal_status
			*/
			status = (strstr(local_rx_buffer,"+CSQ: 0,")==NULL?0:1);
			/* Clear receive buffer*/
			memset(local_rx_buffer,NULL,RX_BUFFER_LENGTH);
		
			if (status)
				break;
			else
				if (trials_counter>=3)
					return ERR;
				/* if the signal is weak, wait 1 second before measuring again */
				else
					take_action();
				trials_counter++;
		}
		trials_counter=0;