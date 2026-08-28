# NIGHTGUARD – Urban Night Safety Inspection Rover

## About the Project

NIGHTGUARD is an IoT-based rover designed for night-time urban safety inspection.

The system combines an ESP32-CAM, GPS module, IR illumination and a web-based monitoring interface to help remotely observe and monitor locations during low-light conditions.

## Key Features

* Live camera streaming using ESP32-CAM
* Night-time IR illumination
* GPS location tracking
* GPS satellite information
* Web-based monitoring dashboard
* Google Maps location access
* ESP32-based Wi-Fi hotspot
* Manual rover control using FlySky FS-i6X

## Hardware Components

* AI-Thinker ESP32-CAM
* OV2640 Camera
* NEO-6M GPS Module
* IR Illumination
* FlySky FS-i6X Transmitter and Receiver
* Rover chassis and motor drive system

## Software and Technologies

* Arduino IDE
* Embedded C/C++
* ESP32
* Wi-Fi
* HTML
* CSS
* JavaScript

### Libraries Used

* `esp_camera.h`
* `WiFi.h`
* `WebServer.h`
* `TinyGPSPlus.h`

## System Working

The ESP32-CAM captures live video from the surroundings and provides the video through a web interface.

The NEO-6M GPS module provides the rover's geographical coordinates and satellite information.

The IR illumination can be controlled through the web interface to improve visibility during low-light conditions.

The ESP32 creates its own Wi-Fi access point, allowing the monitoring webpage to be accessed directly by a connected device.

## Project Architecture

```text
Camera ───────────────┐
                      │
GPS ──────────────────┤
                      ├──> ESP32-CAM ──> Web Interface
IR Illumination ──────┘          │
                                 ├──> Live Video
                                 ├──> GPS Location
                                 └──> IR Control

FlySky FS-i6X ──> Rover Motor Control
```

## Camera

The system uses the OV2640 camera connected to the AI-Thinker ESP32-CAM.

The maximum practical resolution used by the project is UXGA (1600 × 1200). True 4K video is not supported by the OV2640 camera.

## Future Improvements

* AI-based object and hazard detection
* Automatic anomaly detection
* Obstacle detection
* Improved low-light vision
* Mobile application
* Cloud-based monitoring
* Autonomous navigation

## Project Status

Working prototype developed and tested using ESP32-CAM, GPS and IR illumination.

## Author

Aafreen Marzuqa A

B.Tech Information Technology
