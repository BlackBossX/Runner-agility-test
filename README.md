# Agility Runner Timer

An Arduino-based agility timer system designed to test a runner's reaction time and speed. The system uses four ultrasonic sensors to track the runner's progress, an OLED screen for real-time display, and randomized path logic to test agility.

## Features

- **4-Point Tracking**: Measures start, midway (V-Bend), and two finish lines.
- **Randomized Agility Paths**: At the midway point, the system randomly selects one of two paths for the runner to take.
- **Visual Indicators**: LEDs light up to instantly signal the chosen path to the runner.
- **OLED Interface**: Real-time display of states, distances, and lap times.
- **Calibration Mode**: View live distances of all 4 sensors at once to easily set up your 1-meter trigger gates.

## Hardware Requirements

- 1x Arduino Uno
- 4x HC-SR04 Ultrasonic Sensors
- 1x SSD1306 SPI OLED Display (128x64)
- 2x LEDs (with suitable current-limiting resistors)
- 1x Push Button

## Pinout Guide

### 1. SSD1306 OLED Display (SPI)

| Display Pin | Arduino Pin |
| ----------- | ----------- |
| GND         | GND         |
| VDD / VCC   | 5V / 3.3V   |
| SCK / D0    | Pin 13      |
| SDA / D1    | Pin 11      |
| DC          | Pin 8       |
| CS          | Pin 7       |
| RES         | Pin 6       |

### 2. HC-SR04 Ultrasonic Sensors

_Note: All sensors share the 5V and GND connections._

| Sensor | Position   | TRIG Pin | ECHO Pin |
| ------ | ---------- | -------- | -------- |
| **S1** | Start Line | Pin 9    | Pin 10   |
| **S2** | V-Bend     | Pin A0   | Pin A1   |
| **S3** | Path A End | Pin A2   | Pin A3   |
| **S4** | Path B End | Pin A4   | Pin A5   |

### 3. Indicator LEDs & Controls

| Component    | Arduino Pin | Notes                                                           |
| ------------ | ----------- | --------------------------------------------------------------- |
| Path A LED   | Pin 3       | Connect positive leg to Pin 3, negative to GND through resistor |
| Path B LED   | Pin 4       | Connect positive leg to Pin 4, negative to GND through resistor |
| Reset Button | Pin 2       | Connect between Pin 2 and GND (Internal pull-up is used)        |

## How to Operate

1. **Calibration**: Upon booting, the system enters `CALIBRATING` mode. Use the live distance readouts on the OLED to position the sidebars for each of the 4 gates so that the gap is exactly 1 meter.
2. **Ready**: Press the physical reset button (or send `R` via the Serial Monitor) to enter the `READY` state.
3. **Run**: The runner gets into position at the start line (Sensor 1 distance < 1m). The screen shows `SET...`. As soon as the runner leaves the start line, the timer begins.
4. **The V-Bend**: The runner reaches the 2.5m mark (Sensor 2). The system randomly selects Path A or Path B and lights up the corresponding LED.
5. **Finish**: The runner completes the correct path (triggering Sensor 3 or 4). The timer stops, the LED turns off, and the final time is displayed on the screen.
6. **Reset**: Press the button again to reset the timer for the next run.
