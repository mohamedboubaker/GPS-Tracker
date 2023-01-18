# 1. Introduction
## 1.1 Project overview

The objective of this project is to deliever a GPS tracking system that can be used to track a vehicule. The system includes a device that periodically sends its position and speed information to a remote server. The data is stored on the server where it can be accessed through a web browser for visualization.

The scope of the project is more focoused on the embedded side. In other words, the developement of the server and visualization platform are not mature. However, this work is mainly concerned with: designing a circuit, manufacturing the PCB, and developing firmware to power it, all while ensuring reliability. Below is a picture of the  produced PCB.

![](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Media/PCB.JPG)
*Figure 1. Produced*

#### Table of Contents
- [1. Introduction](#1-Introduction)
  * [1.1 Project overview](#11-Project-overview)
  * [1.2 Toolchain](#12-Toolchain)
  * [1.3 Project timeline](#13-Project-timeline)
- [2. System design](#2-System-design)
  * [2.1 System architecture](#21-System-architecture)
  * [2.2 Firmware design](#22-Firmware-design)
  * [2.3 Server design](#23-Server-design)
  * [2.4 Circuit design](#24-Circuit-design)
  * [2.5 PCB design](#25-PCB-design)
- [3. Implementation](#3-Implementation)
  * [3.1 Manufacturing](#31-Manufacturing)
  * [3.2 Testing and debugging](#32-Testing-and-debugging)
- [4. Results and evaluation](#4-Results-and-evaluation)
  * [4.1 Test drive](#41-Test-drive)
  * [4.2 Performance evaluation](#42-Performance-evaluation)
- [5. Conclusion](#5-Conclusion)
  * [5.1 Summary](#51-Summary)
  * [5.2 Reference and bibliography](#52-Reference-and-bibliography)

## 1.3 Toolchain

Developement boards: STM32F0 Discovery and SIM808 EVB-v3.2.4
MCU Configuration: STMCubeMX
Development environement: ARM MDK-Keil version 5
Flashing utilities:  STM32 ST-LINK Utility and  ST-link programmer
PCB Design:KiCAD version 6

## 1.3 Project timeline

The realization of this project was divided into 3 main phases. 

The first phase of the project involved selecting the Microcontroller (MCU) and GPS/GPRS module, and then designing a minimalistic proof of concept using development boards.

The second phase involved designing the circuit and PCB. After the hardware design phase completed, PCB prototypes were  ordered.

In the third phase, the firmware that was initially developed during the POC stage was further refined to fit the new custom PCB and enhanced to become more reliable. 


# 2. System design
## 2.1 System architecture

The idea is basically a microcontroller, STM32F030, that periodically sends AT commands via UART to the SIM808 GPS/GPRS module to get GPS data and then sends these data to a Mosquitto MQTT server hosted on an AWS EC2. This data gets also encrypted with AES-128 in Electronic Code Book (ECB) mode before being sent to the server. AES algorithm was implemented from scratch for learning purposes. (Note: In a production application, it's better to use a known AES implementations. Example: WolfSSL)

The power input of the circuit is regulated using a DC-DC down converter Texas Instruments TPS5430DDA with input voltage ranging from 5.5V to 36V 

## 2.2 Firmware design
## 2.3 Server design
## 2.4 Circuit design

![alt text](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Media/circuit_diagram.jpg)
*Figure 2. Circuit Diagram*

## 2.5 PCB design
Below is a 3D picture of the manufactured PCB. The front side is on the left and contains mainly the TPS5430DDA Power regulation circuit on top and the STM32F0 in the center. On the right you see the PCB's backside which contains the SIM808 and SIM card holder circuit.

![alt text](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Media/Layout_FrontAndBack.png)
*Figure 3. Layout: front side on the left, back side on the right*


![](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Media/3D_FrontAndBack.png)
*Figure 4. 3D Model of the PCB*

# 4. Results and evaluation
## 4.1 Test Drive

![](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Media/test_drive_1.jpeg)
*Figure 5. Test Drive result in Grombalia, Tunisia*
