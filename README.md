# Vision-Based Smart Cashier AIoT System

An AIoT smart checkout system that combines Computer Vision, ESP32 hardware, MQTT/TLS communication, ThingsBoard Cloud, OTA updates, and offline event buffering.

The system detects products using a camera and AI model, updates the cashier cart, sends checkout events securely to ThingsBoard, and controls an ESP32 device remotely using RPC commands.

---

## Project Overview

The project converts a traditional embedded smart cashier into a complete AIoT system.

The system consists of:

- USB Camera
- Laptop running the AI model
- FastAPI backend
- YOLOv8 object detection
- Product verification
- Web-based cashier interface
- ESP32
- LCD 16x2 I2C
- Green and Red LEDs
- Buzzer
- ThingsBoard Cloud
- MQTT over TLS
- RPC commands
- OTA firmware updates
- Offline event buffering and resend
- Wi-Fi power saving

---

## System Architecture

```text
USB Camera
    |
    v
Laptop
    |
    +--> YOLOv8 Detection
    |
    +--> Product Verification
    |
    +--> FastAPI Backend
    |
    +--> Cashier Web Interface
    |
    v
MQTTS / TLS
    |
    v
ThingsBoard Cloud
    |
    +--> Dashboard
    |
    +--> Telemetry
    |
    +--> Rule Engine
    |
    v
RPC Command
    |
    v
ESP32
    |
    +--> LCD
    +--> Green LED
    +--> Red LED
    +--> Buzzer
Main Features
AI Product Detection

The system uses YOLOv8 to detect products from a live USB camera.

The detected products are processed by the FastAPI backend and displayed in the cashier interface.

Smart Checkout

The cashier interface supports:

Product detection
Add product
Delete product
Multiple products
Product quantity
Cart count
Cart total
Invoice ID
Payment validation
Invoice printing
ThingsBoard Integration

The laptop sends checkout telemetry to ThingsBoard Cloud.

Example telemetry fields:

product_name
event_type
confidence
quantity
cart_count
cart_total
invoice_id

The ESP32 sends:

esp32_status
wifi_rssi
MQTT/TLS Security

Communication with ThingsBoard uses secure MQTT over TLS.

Server: mqtt.eu.thingsboard.cloud
Port: 8883
Protocol: MQTTS

A trusted root CA certificate is used to verify the ThingsBoard server.

Device credentials are not stored directly in the public source code.

ESP32 Hardware

Hardware used:

Component	Connection
ESP32-WROOM-32	Main controller
LCD 16x2 I2C	Display
SDA	GPIO 21
SCL	GPIO 22
Buzzer	GPIO 25
Green LED	GPIO 26
Red LED	GPIO 27

The buzzer is configured as Active LOW.

ESP32 RPC Control

ThingsBoard sends RPC commands to the ESP32.

RPC method:

setStatus

Supported commands:

SUCCESS
SUCCESS

Result:

LCD displays Product Found
Green LED turns on
Short buzzer notification
REJECT
REJECT

Result:

LCD displays Unknown Product
Red LED turns on
Longer buzzer notification

The ESP32 sends an acknowledgement response after executing the RPC command.

ThingsBoard Rule Engine

The ThingsBoard Rule Engine connects AI cashier events to the ESP32.

Flow:

AI-Cashier-01
      |
      v
Post Telemetry
      |
      v
Smart Cashier Automation
      |
      v
Check Relation
      |
      v
ESP32-Checkout-01
      |
      v
Create RPC Command
      |
      v
RPC Call Request

Behavior:

ADD
  -> SUCCESS

REJECT
  -> REJECT
Offline Reliability

If the laptop loses Internet connectivity, cashier events are stored locally in:

data/pending_thingsboard_events.jsonl

The local cashier system continues operating even when ThingsBoard is temporarily unavailable.

When another publish attempt occurs after the connection returns, the system:

Detects pending events.
Resends the stored events.
Sends the new event.
Removes successfully delivered events from the local queue.

This implements a Store-and-Forward reliability mechanism.

OTA Firmware Updates

The ESP32 supports Over-The-Air firmware updates using ArduinoOTA.

OTA hostname:

ESP32-Checkout-01

OTA updates are protected using an OTA password stored locally in:

secrets.h

This allows ESP32 firmware to be updated through the local Wi-Fi network without reconnecting the USB cable.

Energy Management

The ESP32 enables Wi-Fi modem sleep:

WiFi.setSleep(true);

This reduces Wi-Fi power consumption while maintaining the network connection.

The Serial Monitor confirms:

WiFi Power Saving: ENABLED
Secrets Management

Real credentials must never be uploaded to GitHub.

The real configuration file is:

esp32/ESP32_Checkout_OTA/secrets.h

This file is ignored by Git.

Use:

secrets.example.h

as a template.

Example:

#pragma once

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

const char* accessToken = "YOUR_ESP32_THINGSBOARD_TOKEN";

const char* otaPassword = "YOUR_OTA_PASSWORD";

The laptop ThingsBoard token is supplied using an environment variable:

TB_AI_TOKEN
Project Structure
vision-smart-cashier-system-main/
â”‚
â”œâ”€â”€ api/
â”‚   â”œâ”€â”€ main.py
â”‚   â””â”€â”€ thingsboard_sender.py
â”‚
â”œâ”€â”€ dataset/
â”‚   â”œâ”€â”€ train/
â”‚   â”œâ”€â”€ valid/
â”‚   â”œâ”€â”€ test/
â”‚   â””â”€â”€ data.yaml
â”‚
â”œâ”€â”€ docs/
â”‚
â”œâ”€â”€ esp32/
â”‚   â””â”€â”€ ESP32_Checkout_OTA/
â”‚       â”œâ”€â”€ ESP32_Checkout_OTA.ino
â”‚       â””â”€â”€ secrets.example.h
â”‚
â”œâ”€â”€ frontend/
â”‚   â”œâ”€â”€ index.html
â”‚   â””â”€â”€ images/
â”‚
â”œâ”€â”€ model/
â”‚   â””â”€â”€ best.pt
â”‚
â”œâ”€â”€ product_images/
â”‚
â”œâ”€â”€ runs/
â”‚
â”œâ”€â”€ tools/
â”‚
â”œâ”€â”€ .gitignore
â”œâ”€â”€ requirements.txt
â”œâ”€â”€ train_yolov8.py
â””â”€â”€ README.md
Python Requirements

The tested Python environment uses:

fastapi==0.127.0
uvicorn==0.40.0
torch==2.12.0+cpu
torchvision==0.27.0+cpu
ultralytics==8.4.42
numpy==1.26.4
pillow==12.2.0
opencv-python>=4.10,<4.11

Install dependencies using:

pip install -r requirements.txt
Running the Backend

Open PowerShell in the project directory:

cd vision-smart-cashier-system

Set the ThingsBoard token:

$env:TB_AI_TOKEN = Read-Host "Paste AI-Cashier-01 Access Token"

Run FastAPI:

python -m uvicorn api.main:app --host 127.0.0.1 --port 8000

Open:

http://127.0.0.1:8000
ThingsBoard Devices

The project uses two ThingsBoard devices:

AI-Cashier-01
ESP32-Checkout-01

The AI device sends checkout events.

The ESP32 device sends hardware status and receives RPC commands.

Dashboard

The ThingsBoard dashboard displays:

ESP32 Status
Wi-Fi Signal
Last Product
Last Event
Invoice ID
Cart Count
Cart Total
AI Confidence
Transaction History
AI Confidence Trend
Cart Value Trend
Checkout Event information
Live Dashboard
https://eu.thingsboard.cloud/dashboard/f3f9ac10-a8e1-11f1-b747-f73439e59bea?publicId=987e7c80-bd22-11f1-890c-f73439e59bea

Replace this placeholder with the final read-only ThingsBoard dashboard link before submission.

Tested Functions

The following functions were tested on real hardware:

Product detection
Cart update
Multiple product handling
ThingsBoard telemetry
MQTT/TLS communication
ThingsBoard Rule Engine
ESP32 RPC control
LCD notifications
Green LED SUCCESS indication
Red LED REJECT indication
Buzzer notifications
ESP32 telemetry
Wi-Fi RSSI monitoring
Offline event storage
Pending event resend
OTA wireless firmware update
Wi-Fi power saving

No hardware simulation is used in the final system.

Security Features

The system includes:

MQTT over TLS
Root CA verification
Separate ThingsBoard device credentials
Environment variable for laptop token
Git-ignored ESP32 secrets
Password-protected OTA
NTP synchronization for TLS certificate verification
No credentials stored in the public repository
Reliability Features

The system includes:

Automatic Wi-Fi reconnection
Automatic MQTT reconnection
Non-blocking MQTT retry
Local offline event buffering
Store-and-Forward event resend
ESP32 status telemetry
Wi-Fi signal monitoring
RPC acknowledgement
OTA maintenance
Wi-Fi power saving
Technologies
Python
FastAPI
YOLOv8
PyTorch
OpenCV
HTML / CSS / JavaScript
ESP32
Arduino IDE
MQTT
TLS
ThingsBoard Cloud
ArduinoOTA
Final System

The final project demonstrates a complete AIoT workflow:

Computer Vision
      +
Web Application
      +
Cloud IoT Platform
      +
Secure Communication
      +
Real ESP32 Hardware
      +
Reliability
      +
OTA
      +
Energy Management

The system has been tested using real hardware and real ThingsBoard Cloud communication.


