# Usage Guide

## Hardware
- ESP32 DevKit v1
- Up to 4 potentiometers

Connect:
- Wiper → GPIO32, 33, 34, 35
- One outer leg → 3.3V
- Other outer leg → GND

## Software
- Arduino IDE with ESP32 board support
- Install driver (CP2102 for DevKit v1)

Upload the sketch, then open:
- **Serial Monitor** @115200 baud → detailed view
- **Serial Plotter** @115200 baud → graphs
