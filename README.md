Project Intorudction

The idea is basically a microcontroller, STM32F030, that periodically sends AT commands via UART to the SIM808 GPS/GPRS chip to get GPS and time data and then sends these data to a Mosquitto MQTT server on AWS. This data gets also encrypted with AES before being sent to the server. 

The power input is regulated using a DC-DC down converter Texas Instruments TPS5430DDA. 


![alt text](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Media/PCB.JPG)
