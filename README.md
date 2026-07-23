# Loadcell_ADS1261

## Overview

This repository contains firmware and Python utilities for **load cell data acquisition, calibration, and Center of Pressure (COP) analysis** using the **Teensy 4.1** microcontroller and the **ADS1261 amplifier**.

The repository includes:

- Firmware for **single-board** and **dual-board** load cell acquisition systems.
- Python-based load cell calibration using known reference weights.
- Automatic calculation of calibration coefficients.
- Automatic updating of calibration coefficients in the firmware.
- Saving calibration results to Excel.
- ADS1261 Chip ID diagnostic firmware for verifying SPI communication.
- Load cell calibration firmware.
- Real-time Center of Pressure (COP) visualization software.

---

# Hardware

- Teensy 4.1
- ADS1261 amplifier
- Load Cells

---

# Hardware Connections

## Board 1

| Teensy 4.1 | ADS1261 |
|------------|----------|
| 3.3 V | DVDD |
| GND | GND |
| GND | AVSS |
| Pin 10 | CS |
| Pin 11 | DIN (MOSI) |
| Pin 12 | DOUT (MISO) |
| Pin 13 | SCK |
| Pin 2 | DRDY |
| Pin 3 | RESET |

---

## Board 2 & Board 3

The second and third ADS1261 shares the same SPI bus as Board 1.

### Shared SPI Connections

| Signal | Teensy Pin |
|---------|------------|
| MOSI | Pin 11 |
| MISO | Pin 12 |
| SCK | Pin 13 |

### Independent Control Pins

| Signal | Board 1 | Board 2 | Board 3 |
|---------|---------|---------|---------|
| CS | Pin 10 | Pin 0 | Pin 44 |
| DRDY | Pin 2 | Pin 4 | Pin 6 |
| RESET | Pin 3 | Pin 5 | Pin 7 |

---

# Repository Structure

```
Loadcell_ADS1261
│
├── Calibration/
│   ├── Calib.ino
│   └── Calib.py
│
├── Chip_ID/
│   └── Chip_ID.ino
│
├── FP_LDC_Cal2/
│   └── FP_LDC_Cal2.ino
│
├── OneBoard_Dynamic/
│
├── TwoBoard_Dynamic/
│
├── ADS1261_debugging.docx
│
├── COP_vis.py
│
└── README.md
```

---

# Repository Contents

## Calibration

Contains the Arduino firmware and Python software for load cell calibration.

### Features

- Collects calibration data from the load cells.
- Calculates calibration coefficients using known reference weights.
- Automatically updates the firmware calibration values.
- Saves calibration results to Excel.

---

## OneBoard_Dynamic

Firmware for a **single ADS1261** acquisition board.

### Features

- Reads four load cells.
- Computes individual forces.
- Calculates total weight.
- Calculates Center of Pressure (COP).
- Sends all measurements through Serial.

---

## TwoBoard_Dynamic

Firmware for a **dual ADS1261** acquisition system.

### Features

- Simultaneous acquisition from two ADS1261 boards.
- Supports eight load cells.
- Independent calibration for each board.
- Calculates total weight.
- Computes Center of Pressure (COP).

---

## FP_LDC_Cal2

Arduino firmware used for load cell calibration.

---

## Chip_ID

Diagnostic firmware used to verify communication with the ADS1261.

### Features

- Reads the ADS1261 Chip ID.
- Verifies SPI communication.
- Confirms that the ADC is responding correctly.
- Useful for debugging communication-related issues before running the main firmware.

---

## ADS1261_debugging.docx

Step-by-step debugging guide for troubleshooting ADS1261 hardware.

---

## COP_vis.py

Python application for real-time Center of Pressure visualization.

### Features

- Supports one-board and two-board configurations.
- Real-time COP visualization.
- Displays total weight.
- Displays individual load cell forces.
- Live serial communication with the Teensy.
- Graphical force plate representation.

---

# Software Requirements

## Arduino

- Arduino IDE
- Teensyduino

---

## Python

Install the required Python packages:

```bash
pip install numpy pandas matplotlib pyserial openpyxl
```

Required libraries:

- NumPy
- Pandas
- Matplotlib
- PySerial
- OpenPyXL

---

# Author

**Poornima Ravikumar**
