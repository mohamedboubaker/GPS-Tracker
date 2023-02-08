# 1. Introduction
## 1.1 Project overview

The objective of this project is to deliever a GPS tracking system that can be used to track a vehicule. The system includes a device that periodically sends its position and speed information to a remote server. The data is stored on the server where it can be accessed through a web browser for visualization.

The scope of the project is more focoused on the embedded side. In other words, the developement of the server and visualization platform are not mature. However, this work is mainly concerned with: designing a circuit, manufacturing the PCB, and developing firmware to power it, all while ensuring reliability. Below is a picture of the  produced PCB.


<b>Note: 
 
 click here to go directlty to the <a href="https://mohamedboubaker.github.io/GPS-Tracker/files.html">code documentation</a> 
 
 click here to jump to the <a href="https://github.com/mohamedboubaker/GPS-Tracker#41-Test-drive"> test drive results </a>
</b>

![](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Pictures/PCB.JPG)
*Figure 1. Produced PCB board*

#### Table of Contents
- [1. Introduction](#1-Introduction)
  * [1.1 Project overview](#11-Project-overview)
  * [1.2 Toolchain](#12-Toolchain)
  * [1.3 Project timeline](#13-Project-timeline)
- [2. System design](#2-System-design)
  * [2.1 System architecture](#21-System-architecture)
  * [2.2 Firmware design](#22-Firmware-design)
     + [2.2.1 Code documentation](#221-Code-documentation) 
  * [2.3 Server design](#23-Server-design)
  * [2.4 Circuit design](#24-Circuit-design)
     + [2.4.1 Voltage regulation](#241-Voltage-regulation)
     + [2.4.2 GPS/GPRS module](#242-gpsgprs-module)
     + [2.4.3 MCU](#243-MCU)
  * [2.5 PCB layout design](#25-PCB-layout-design)
- [3. Implementation](#3-Implementation)
  * [3.1 Manufacturing](#31-Manufacturing)
  * [3.2 Testing and debugging](#32-Testing-and-debugging)
- [4. Results and evaluation](#4-Results-and-evaluation)
  * [4.1 Test drive](#41-Test-drive)
  * [4.2 Performance evaluation](#42-Performance-evaluation)
- [5. Conclusion](#5-Conclusion)
  * [5.1 Summary](#51-Summary)
  * [5.2 Reference and bibliography](#52-Reference-and-bibliography)

## 1.2 Toolchain

- Developement boards: STM32F0 Discovery and SIM808 EVB-v3.2.4
- MCU Configuration: STMCubeMX
- IDE: STM32CubeIDE v1.10.1
- Compiler: arm-none-eabi-gcc 
- Flashing utilities: STM32 ST-LINK Utility and  ST-link programmer
- PCB Design: KiCAD version 6
- PCB Manufacturing: JLCPCB

## 1.3 Project timeline

The realization of this project was divided into 3 main phases. 

The first phase of the project involved selecting the Microcontroller (MCU) and GPS/GPRS module, and then designing a minimalistic proof of concept using development boards.

The second phase involved designing the circuit and PCB. After the hardware design phase completed, PCB prototypes were  ordered.

In the third phase, the firmware that was initially developed during the POC stage was further refined to fit the new custom PCB and enhanced to become more reliable. 


# 2. System design
## 2.1 System architecture

The system is composed of 3 main parts: a PCB, firmware and a server. 

* The PCB circuit contains 3 major elements: 
     * A power regulation circuit based on the Texas Instruments TPS5430DDA DC-DC down converter, which is configured to safely convert any voltage between 5.5V and 20V to 3.6V. 
     * A GPS/GPRS module: SIM808, which is capable of receiving GPS signals and also connecting to a GPRS network, which means connecting to the internet.
     * An STM32 MCU: STM32F030, which is the brains of the PCB. It controls the SIM808 module via UART.

* The firmware is composed of 4 main source files
     * sim808.c contains the functions to startup, reset and send commands to the SIM808 module. 
     * gps.c contains functions to enable/disable GPS functionality and get GPS position and speed information.
     * network_functions.c contains functions to set up GPRS, send raw data through a TCP session,  publish messages to an MQTT Broker.
     * aes_encryption.c contains an implementation of the AES-128 encryption algorithm. It is used to encrypt the MQTT messages before sending them to the server.

* The server, which is an EC2 instance that has the following services running:
     * Mosquitto MQTT Broker, which is used to receive messages from the remote GPS boards.
     * An MQTT subscriber, which is a python script running as a systemd daemon responsible for processing all incoming messages to the broker and saving the Board ID, position, speed and time stamp to a database.
     * An SQL database which is PostgreSQL used to store the position data of the boards.
     * A web application utilizing Angular and Spring Boot, hosted on Apache HTTP Server, used to display the historical location of the device on a map.
     
![alt text](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Pictures/Architecture.svg)

*Figure 2. Architecture overview*


## 2.2 Firmware design
### 2.2.1 Code documentation

The code features Doxygen-style documentation for all functions, providing clear explanations of their operations. This documentation has been compiled and can be accessed at the following link  <a href="https://mohamedboubaker.github.io/GPS-Tracker/files.html">Code documentation</a> 


## 2.3 Server design
## 2.4 Circuit design

![alt text](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Pictures/circuit_diagram.jpg)
*Figure 2. Circuit schematic*

### 2.4.1 Voltage regulation 

The voltage regulation circuit is designed to achieve the following goals:
- Be tolerant to reverse polarity.
- Protect the circuit in case of a short circuit.
- Be able to withstand input voltages that are typical of a car battery. i.e. 12V~20V.
- Provide peak currents of up to 2A. This is required by the SIM808 module. <a href="https://www.openhacks.com/uploadsproductos/sim808_hardware_design_v1.02.pdf"> [2]. </a> 
- Provide a stable output voltage of 3.6V because the operating voltage range of the STM32F0 MCU is 1.8V-3.6V <a href="https://www.st.com/resource/en/datasheet/stm32f030f4.pdf"> [3] </a> and that of the SIM808 module is 3.4V-4.4V <a href="https://www.openhacks.com/uploadsproductos/sim808_hardware_design_v1.02.pdf"> [2] </a>. 

Reverse polarity protection is achieved through the MOSFET transistor denoted as Q1 in the schematic above which has a Gate-Source voltage rating of 20V. This limits the maximum input voltage of the circuit to 20V. 

Protection against short circuits is provided through the Fuse denoted as F1 with a rating of 2.5A. (In practice, 2.5 A is a very high current for such a circuit. If a short circuit happens, many components would fail before the fuse will break the current. This value needs to be reconsidered.)

The rest of the requirements are met using the Texas Instruments TPS5430DDA DC-DC down converter. The components on the right side of TPS5430DDA in the schematic are designed according to the recommendations of the datasheet <a href="https://datasheet.octopart.com/TPS5430DDA-Texas-Instruments-datasheet-8428127.pdf">[1]</a>. 

The output voltage is determined by the values of the resistors that are connected above and below the feedback point (see the schematic above). In the datasheet, the resistor above the feedback point is denoted as R1, whereas that below the feedback point is denoted as R2. According to equation (12) in the datasheet the following formula can be derived:
<p align="center" > $\frac{R1}{R2}=0.819V_{out}-1$ </p>
Therefore, to achieve a regulated voltage of 3.6V, the ratio between the resistors should be as follows: <p align="center" > $\frac{R1}{R2}=1.95$ </p>

It is difficult to achieve that ratio using 2 standard resistor values, for example, 220Ω, 1kΩ or 2.2kΩ. So as a solution, R2 can be replaced by 2 standard value resistors in series. Hence the placement of 2 resistors in the circuit design, which are R10 and R11 which are equivalent to R2 in the datasheet. R9 in the design is equivaelent to R1 in the datasheet.

To achieve a ratio of 1.95, i.e. an output voltage of 3.6V the following values can be used. R9=8.2kΩ, R10=1.5kΩ and R11=2.7kΩ. Below is the equation to demonstrate this:

<p align="center" > $\frac{R9}{R10+R11}=\frac{8.2}{1.5+2.7}=1.95$ </p>



### 2.4.2 GPS/GPRS module 
The circuit design for the SIM808 module followed the guidelines in the SIM808 hardware design guide <a href="https://www.openhacks.com/uploadsproductos/sim808_hardware_design_v1.02.pdf"> [2] </a>. Bypass capacitors U20, U21, and U22 were added as recommended. The guide also suggested using a 5.1V Zener diode (D5) to protect against voltage surges. Although the diode was added as per the guidelines, it caused the circuit to malfunction after power on, so it was manually desoldered. This is documented in issue <a href="https://github.com/mohamedboubaker/GPS-Tracker/issues/35">#35 </a>. 

The SIM808 module is connected to a micro SIM card holder. The lines between the module and the holder are protected against voltage surges using Transient Voltage Surpression (TVS) diodes present in the integrated circuit U23. Capacitors U18, U19 and Resistors R4,R5 and R12 are recommended by the guide.

The GPS and GPRS antenna outputs are connected to 2 U.FL connectors respectively.

The SIM808 module and the STM32 MCU communicate using UART. The STM32 can also power on, reset and check the status of the module through its GPIO pins which are connected SIM_PWRKEY, SIM_RESET and STATUS pins on the module. These SIM808 pins including the UART are also exposed through the header pins connector on the PCB. Which enable controlling and communicating with the module from the outside, without having to write a program to do so on the STM32. 

### 2.4.3 MCU
The STM32 is connected to a Reset and a User button, and 2 LEDs: D1 and D2. The User button SW2 is surrounded by a typical debouncing circuit which is inspired from the debouncing circuit found on the STM32F4 Discovery board.

An 8MHz Crystal is used to synchronize the MCU's clock. U1, U2, U3, U4 and U5 are bypass capacitors added as per the recoomendations in the datasheet  <a href="https://www.st.com/resource/en/datasheet/stm32f030f4.pdf"> [3] </a>. 

SWD pins, required for programming the MCU, and several other GPIOs are exposed through header pins on the PCB. This enables the functionality of the board to be expanded. For example, it can be connected to external sensors and actuators.

## 2.5 PCB design
Below is a 3D picture of the manufactured PCB. The front side is on the left and contains mainly the TPS5430DDA Power regulation circuit on top and the STM32F0 in the center. On the right you see the PCB's backside which contains the SIM808 and SIM card holder circuit.

![alt text](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Pictures/Layout_FrontAndBack.png)
*Figure 3. Layout: front side on the left, back side on the right*


![](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Pictures/3D_FrontAndBack.png)
*Figure 4. 3D Model of the PCB*

# 4. Results and evaluation
## 4.1 Test Drive

![](https://github.com/mohamedboubaker/GPS-Tracker/blob/main/Pictures/test_drive_nuremberg_munich.png)
*Figure 5. Test Drive result from Nuremberg to Munich, Germany*

# 5. Conclusion
## 5.1 Summary
## 5.2 Reference and bibliography
- [1] <a href="https://datasheet.octopart.com/TPS5430DDA-Texas-Instruments-datasheet-8428127.pdf" > Texas Instruments TPS5430DDA datasheet. </a>
- [2] <a href="https://www.openhacks.com/uploadsproductos/sim808_hardware_design_v1.02.pdf"> SIM808_Hardware Design_V1.02 </a>
- [3] <a href="https://www.st.com/resource/en/datasheet/stm32f030f4.pdf"> STM32F030x8 datasheet </a>
