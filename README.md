# 🚪 Automatic Door Control System

A complete **embedded hardware and software project** developed during my internship at **ENOVA Robotics**, combining **circuit design**, **embedded control**, and **IoT communication** to automate door operation.

---

## 🧠 Overview

This project implements an **automatic door system** controlled by an **ESP32-WROOM-32** module.  
The system detects obstacles, drives a DC motor to open/close the door, and exchanges status information through **MQTT** communication.

The work includes:
- Full **hardware schematic and PCB design**
- **Firmware** development using **ESP-IDF (C)**
- **MQTT IoT integration** for real-time monitoring and control

---

## ⚙️ Features

- MQTT-based control and feedback (via Wi-Fi)  
- Manual override mode  
- Safety relay and isolation stages  
- Auto-reset and USB-UART programming interface  
- Modular power and communication circuits  

---

## 🧱 Circuit Design

### 🖼️ Circuit Schematic
See [`Circuit schematic.pdf`](./hardware/Circuit%20schematic.pdf) for the complete schematic.

The design integrates all major subsystems:
1. **Power Supply Block**
   - Converts 220 V AC → 6 V DC → regulated **3.3 V** for logic circuits  
   - Uses AMS1117-3.3 regulator with filtering capacitors (22 µF, 100 nF)  
   - Relay drivers powered through 6 V rail  
   - Proper decoupling and noise suppression included  

2. **Microcontroller Unit (MCU)**
   - **ESP32-WROOM-32** as the main control unit  
   - Provides GPIOs for motor, sensors, and MQTT communication  
   - Includes reset and boot pushbuttons with pull-up resistors  
   - UART interface connected to USB converter for flashing/debug  

3. **USB-UART Interface**
   - **CP2102N** for programming and serial debugging  
   - Integrated DTR/RTS circuit for automatic reset and bootloader enable  
   - Type-C connector compliant with ESD and EMI protection  

4. **RS232 Communication Interface**
   - Implemented using **MAX3232**  
   - Enables serial connection with external control modules  

5. **Ethernet PHY Interface**
   - Optional **LAN8720A** for wired MQTT communication  
   - Connected to ESP32 via RMII interface  

6. **Relay Driver Stage**
   - Two relays (**SRD-05VDC-SL-C**) for motor direction control  
   - Each driven by **SS8050** NPN transistor and protected with **1N4007** diode  
   - Provides electrical isolation and safety switching  

7. **Sensor and Control Inputs**
   - Ultrasonic sensor for obstacle detection  
   - Limit switches for open/close position sensing  
   - Status LEDs for visual feedback  

---

## 💡 Hardware Summary

| Subsystem | Key Components | Function |
|------------|----------------|-----------|
| Power Supply | AMS1117-3.3, capacitors | 220V→3.3V conversion |
| MCU | ESP32-WROOM-32 | System control & communication |
| USB-UART | CP2102N | Programming & debugging |
| RS232 | MAX3232 | Serial external communication |
| Ethernet | LAN8720A | Network interface |
| Relays | SRD-05VDC-SL-C, SS8050, 1N4007 | Motor control |
| Sensors | Ultrasonic, limit switches | Detection & feedback |

---

## 💻 Software Implementation

Developed under **ESP-IDF (C)** with modular architecture:
- **main.c** – task initialization and state machine  
- **mqtt_client.c** – handles MQTT publish/subscribe  
- **motor_control.c** – relay control for open/close logic  
- **sensors.c** – obstacle detection and limit sensing  
- **config.h** – pins, topics, and parameters definition  

---

## 📡 MQTT Communication

| Topic | Direction | Description |
|--------|------------|-------------|
| `/dorra/door/control` | Subscribe | Receives door commands |
| `/dorra/door/state` | Publish | Sends door status |
| `/dorra/status` | Publish (retain) | Connection & LWT |
| `/dorra/logs` | Publish | Debug info |

- **QoS:** 1 (At least once)  
- **LWT:** `"ESP Disconnected"` retained on `/dorra/status`

---

## 🧰 Tools Used

- **Altium Designer** – schematic design  
- **ESP-IDF (C)** – firmware development  
- **MQTT Broker (Mosquitto / HiveMQ)**  
- **VS Code** / **Linux terminal** for build and debugging  


