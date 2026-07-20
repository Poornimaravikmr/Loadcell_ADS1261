# Loadcell_ADS1261

## Overview

This repository contains firmware and Python utilities for **load cell data acquisition, calibration, and Center of Pressure (COP) analysis** using the **Teensy 4.1** microcontroller and the **ADS1261 Amplifier**.

The repository includes:

- Firmware for **single-board** and **dual-board** COP acquisition systems.
- A Python-based **calibration** for determining load cell calibration coefficients using known reference weights, automatically updating the firmware, and saving calibration results to Excel. :contentReference[oaicite:0]{index=0}
- ADS1261 Chip diagnostic firmware for verifying communication with the ADC.
- Calibration Code for Loadcell.
- A real-time Python application for **Center of Pressure (COP)** visualization from one or two acquisition boards. :contentReference[oaicite:1]{index=1}

---

# Hardware

## Teensy 4.1
## ADS1261 Precision ADC

---

# Hardware Connection

| Teensy 4.1 Pin | ADS1261 Pin |
|---------------:|-------------|
| 3.3 V | DVDD |
| GND | GND |
| GND | AVSS |
| 10 | CS |
| 11 | DIN (MOSI) |
| 12 | DOUT (MISO) |
| 13 | SCK |
| 2 | DRDY |
| 6 | RESET |

### Board 2 (Secondary ADS1261)

The second ADS1261 shares the SPI bus with the first board. Only the control pins differ.

### SPI Bus Configuration

Both ADS1261 boards share the same SPI communication lines:

- **MOSI (DIN)** → Teensy Pin **11**
- **MISO (DOUT)** → Teensy Pin **12**
- **SCK** → Teensy Pin **13**

| Signal | Board 1 | Board 2 |
|---------|---------|---------|
| CS | Pin 10 | Pin 0 |
| DRDY | Pin 2 | Pin 4 |
| RESET | Pin 6 | Pin 5 |

---


# Repository Contents

## Calibration/

Contains the Arduino firmware and Python calibration software for load cell calibration.
---

## OneBoard_Dynamic/

Firmware for a **single ADS1261 acquisition board**.

Features include:

- Continuous acquisition from four load cells
- Force calculation
- Center of Pressure (COP) computation
- Serial transmission of COP, total weight, and individual load cell forces

---

## TwoBoard_Dynamic/

Firmware for a **dual ADS1261 acquisition system**.

Features include:

- Simultaneous acquisition from two ADS1261 boards
- Eight load cell inputs
- Independent board calibration
- Weight computation
- COP calculation for multiple boards

---

## FP_LDC_Cal2/

Calibration firmware.

Used for Calibrating the loadcell

---

## Chip_ID/

Diagnostic firmware used to verify communication with the ADS1261.

The program reads the ADS1261 identification registers to confirm that the ADC is correctly connected and functioning before performing data acquisition.

---

## COP_vis.py

Python application for **real-time Center of Pressure (COP) visualization**.

Features include:

- Supports one-board and two-board configurations
- Real-time COP visualization
- Displays total weight
- Displays individual load cell forces
- Live serial communication with the Teensy
- Graphical force plate representation with COP marker :contentReference[oaicite:3]{index=3}

---

# Software Components

- Arduino IDE for firmware development
- Python 3.x
- NumPy
- Pandas
- Matplotlib
- PySerial
- OpenPyXL

---


# Author

**Poornima Ravikumar**
