his module implements a non-blocking, interrupt-driven UART driver for the ESP8266/ESP-01 Wi-Fi module, using the STM32 HAL API.  
All transmissions and receptions run asynchronously through interrupt callbacks, allowing the MCU to perform other tasks while maintaining a responsive Wi-Fi link.
