# STM-Open-Library
A modular collection of open-source STM32 firmware components.
**STM Open Library** is a growing collection of open-source modules for STM32 microcontrollers.  
Each module is written using the official **STM32 HAL** layer and focuses on **clarity, reusability, and educational value**.

The goal is to create a clean, accessible reference for embedded engineers and students who want to learn how to:
- Build modular firmware,
- Integrate hardware peripherals,
- Communicate over UART, SPI, I²C, or Wi-Fi,
- Connect STM32 devices to higher-level systems such as Python servers or databases.

---

## Modules included
| Module | Description |
|---------|-------------|
| ESP-01 Wi-Fi Module| Interrupt-driven ESP8266/ESP-01 Wi-Fi interface using HAL UART. |
| HC-SR04 And HY-SRF05 Ultrasonic Sensors| Hardware-timer-based distance driver with PWM trigger and input capture. |
| BME-280 Environmental Sensor | Forced-mode Bosch BME280 environmental driver with calibration and compensation helpers. |
| MQ-2 Gas Sensor | Blocking MQ-2 helper that averages ADC samples and reports Rs/R0 after clean-air calibration. |

---

## How to use the drivers
- Each module folder contains the driver header/implementation, an `Example.c` snippet, and a module-level README.
- Copy the files for the module you need into your STM32 project, wire the HAL handle references in the example, and follow the README for configuration notes.

## 🧾 License
All modules are released under the **MIT License**.  
Contributions are welcome — submit a pull request with a short description of your module or enhancement.

---
