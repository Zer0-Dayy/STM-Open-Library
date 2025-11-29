# ESP-01 (ESP8266) Wi-Fi Driver for STM32F439ZI

This library shows how to talk to an ESP-01/ESP8266 Wi-Fi module from the STM32F439ZI using the STM32 HAL UART driver. It keeps the CPU free by relying on interrupts instead of blocking `HAL_UART_Transmit`/`HAL_UART_Receive` calls.

## How the driver works
- **UART + interrupts**: `WiFi_Init()` stores the UART handle and starts `HAL_UARTEx_ReceiveToIdle_IT()`. Whenever the line goes idle or the buffer fills, HAL calls `HAL_UARTEx_RxEventCallback()` to copy data into a safe shadow buffer and mark it ready to read. Transmission completion is tracked in `HAL_UART_TxCpltCallback()`.
- **Buffers and flags**:
  - `wifi_rx_buffer` receives raw bytes; the callback copies them into `wifi_rx_shadow_buffer` and null-terminates the string.
  - `wifi_rx_ready` tells the code new data arrived, `wifi_rx_size` stores its length, and `wifi_tx_done` indicates the UART is free to send again.
- **AT commands**: `WiFi_Send_Command()` writes an AT command asynchronously, waits for the expected response text, and returns a status (`WIFI_OK`, `WIFI_ERROR`, `WIFI_TIMEOUT`, `WIFI_BUSY`).
- **Helper routines**:
  - `WiFi_Connect(ssid, password)` joins an access point (`AT+CWJAP`).
  - `WiFi_GetIP(out_buf, buf_len)` asks the module for its IP address (`AT+CIFSR`) and copies the reply.
  - `WiFi_SendTCP(ip, port, message)` opens a TCP socket, allocates send space (`AT+CIPSEND`), transmits your payload, then closes the connection.

## Pinout and setup
- Connect the ESP-01 UART to an STM32 UART (e.g., `USART1`) and supply 3.3 V power. See `Pinout.png` for an example wiring.
- In `main.c` (or your application), initialize HAL and your UARTs, then pass the Wi-Fi UART handle to `WiFi_Init()`.
- Optional: The init routine sends `AT` and `AT+CWMODE=1` to confirm the module is alive and set STA mode.

## Minimal usage example
```c
#include "wifi_basic_driver.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init(); // UART connected to ESP-01

    WiFi_Init(&huart1);
    WiFi_Connect("MySSID", "MyPassword");

    char ip[64];
    WiFi_GetIP(ip, sizeof(ip));
    printf("IP: %s\r\n", ip);

    WiFi_SendTCP("192.168.1.105", 5000, "Hello from STM32!\r\n");
}
```

## HTTP helper
If you prefer not to handle raw sockets, `wifi_http_support.h` exposes simple wrappers for GET, POST, PUT, and DELETE requests. Include it alongside
`wifi_basic_driver.h` to build higher-level HTTP calls without changing the driver workflow shown above. A minimal pattern looks like:

```c
#include "wifi_http_support.h"

wifi_http_header_t headers[] = {
    {"Content-Type", "application/json"},
    {NULL, NULL} // terminator
};

WiFi_HTTP_GET("192.168.1.200", 80, "/status", NULL, 5000, 3);
WiFi_HTTP_POST("192.168.1.200", 80, "/items", headers, "{\"name\":\"demo\"}", 5000, 3);
WiFi_HTTP_PUT("192.168.1.200", 80, "/items/1", headers, "{\"name\":\"updated\"}", 5000, 3);
WiFi_HTTP_DELETE("192.168.1.200", 80, "/items/1", NULL, 5000, 3);
```

## Tips for beginners
- Always wait for `WIFI_BUSY` to clear before sending another command. The driver does this internally by checking `wifi_tx_done` before transmitting.
- If the module replies with `busy p...`, `WiFi_GetIP()` retries after a short delay; you can add similar checks to other functions if needed.
- Increase `WIFI_RX_BUF_LEN` in `wifi_driver.h` if you expect longer responses.
- Use a logic analyzer or serial terminal during bring-up to watch the AT traffic and confirm wiring.
