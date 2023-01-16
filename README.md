# 1. Project overview

This is a GPS tracker board that periodically checks the GPS coordinates and sends them to a predefined remote server on the internet.

![](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Media/PCB.JPG)
*Figure 1. Manufactured PCB*



The idea is basically a microcontroller, STM32F030, that periodically sends AT commands via UART to the SIM808 GPS/GPRS module to get GPS data and then sends these data to a Mosquitto MQTT server hosted on an AWS EC2. This data gets also encrypted with AES-128 in Electronic Code Book (ECB) mode before being sent to the server. AES algorithm was implemented from scratch for learning purposes. (Note: In a production application, it's better to use a known AES implementations. Example: WolfSSL)

The power input of the circuit is regulated using a DC-DC down converter Texas Instruments TPS5430DDA with input voltage ranging from 5.5V to 36V 


#### Table of Contents
- [1. Project overview](#1-Project-overview)
- [2. System design](#2-System-design)
  * [2.1 System architecture](#2.1-System-architecture)
  * [2.2 Firmware design](#2.2-Firmware-design)
  * [2.3 Server design](#2.3-Server-design)
  * [2.4 Circuit design](#2.4-Circuit-design)
  * [2.5 PCB design](#2.5-PCB-design)
- [3. Implementation](#Implementation)
  * [3.1 Manufacturing](#Manufacturing)
  * [3.2 Testing and debugging](#Testing-and-debugging)
- [4. Results and evaluation](#Results-and-evaluation)
  * [4.1 Test drive](#Test-drive)
  * [4.2 Performance evaluation](#Performance-evaluation)
- [5. Conclusion](#Conclusion)
  * [5.1 Summary](#Summary)
  * [5.2 Reference and bibliography](#Reference-and-bibliography)



# 2. System design
## 2.1 System architecture
## 2.2 Firmware design
## 2.3 Server design
## 2.4 Circuit design

![alt text](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Media/circuit_diagram.jpg)
*Figure 2. Circuit Diagram*

## PCB design
Below is a 3D picture of the manufactured PCB. The front side is on the left and contains mainly the TPS5430DDA Power regulation circuit on top and the STM32F0 in the center. On the right you see the PCB's backside which contains the SIM808 and SIM card holder circuit.

![alt text](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Media/Layout_FrontAndBack.png)
*Figure 3. Layout: front side on the left, back side on the right*


![](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Media/3D_FrontAndBack.png)
*Figure 4. 3D Model of the PCB*
