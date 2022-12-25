# Project Introduction

This is a GPS tracker device that periodically checks the GPS coordinates and sends them to a predefined remote server on the internet.

The idea is basically a microcontroller, STM32F030, that periodically sends AT commands via UART to the SIM808 GPS/GPRS chip to get GPS and time data and then sends these data to a Mosquitto MQTT server on AWS. This data gets also encrypted with AES before being sent to the server. 

The power input is regulated using a DC-DC down converter Texas Instruments TPS5430DDA. 

## PCB  
Below is a picture of the manufactured PCB. The front side is on the left and contains mainly the TPS5430DDA Power regulation circuit on top and the STM32F0 in the center. On the right you see the PCB's backside which contains the SIM808 and SIM card holder circuit.

![alt text](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Media/PCB.JPG)

