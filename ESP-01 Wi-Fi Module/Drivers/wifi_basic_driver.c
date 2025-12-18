#include "wifi_basic_driver.h"

static UART_HandleTypeDef *wifi_uart;
static uint8_t wifi_rx_buffer[WIFI_RX_BUF_LEN];            // Primary buffer used to receive data.
static uint8_t wifi_rx_shadow_buffer[WIFI_RX_BUF_LEN];     // Staging buffer used while parsing responses.
volatile uint8_t wifi_rx_ready = 0;                        // Tracks whether a response has arrived.
volatile uint16_t wifi_rx_size = 0;                        // Number of bytes received from the module.
volatile uint8_t wifi_tx_done = 1;                         // Tracks whether a transmission is finished.
volatile uint8_t wifi_response_ready = 0;

// The interrupt handlers are weak by default, so the driver owns them instead of main.c.
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
// Once HAL_UART_Transmit_IT finishes transmitting bytes, HAL raises the TC interrupt and calls
// this callback to mark the UART as free.
{
    if (huart == wifi_uart)
    {
        wifi_tx_done = 1; // Mark transmission as complete when the target UART matches.
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
// HAL calls this callback after IDLE or buffer-full events. The handler null-terminates data so
// string functions remain safe.
{
    if (huart == wifi_uart)
    {
        memcpy(wifi_rx_shadow_buffer, wifi_rx_buffer, size);
        wifi_rx_shadow_buffer[size] = '\0';

        wifi_rx_size = size;
        wifi_rx_ready = 1; // Ready to process the response and resume reception.

        memset(wifi_rx_buffer, 0, WIFI_RX_BUF_LEN); // Clear memory before re-arming reception.
        HAL_UARTEx_ReceiveToIdle_IT(wifi_uart, wifi_rx_buffer, WIFI_RX_BUF_LEN);
    }
}

// Initialize the ESP-01 module with basic defaults.
void WiFi_Init(UART_HandleTypeDef *huart)
{
    wifi_uart = huart;
    HAL_UARTEx_ReceiveToIdle_IT(wifi_uart, wifi_rx_buffer, WIFI_RX_BUF_LEN);
    // Start receiving bytes into the buffer so HAL can trigger HAL_UARTEx_RxEventCallback on IDLE.
    HAL_Delay(1000);
    WiFi_Send_Command("AT\r\n", "OK", 1000);
    WiFi_Send_Command("AT+CWMODE=1\r\n", "OK", 1000);
    printf("Wi-Fi module ready in STA mode. \r\n");
}

// Send an AT command to the ESP8266 module.
wifi_status_t WiFi_Send_Command(const char *Command, const char *expected, uint32_t timeout_ms)
{
    if (Command == NULL || expected == NULL)
    {
        return WIFI_ERROR;
    }
    if (!wifi_tx_done) // Check whether UART is available for a new command.
    {
        return WIFI_BUSY;
    }

    wifi_tx_done = 0; // Mark UART as busy until HAL_UART_TxCpltCallback runs.
    wifi_rx_ready = 0; // Clear the ready flag before waiting for a new response.
    memset(wifi_rx_shadow_buffer, 0, sizeof(wifi_rx_shadow_buffer));

    HAL_UART_Transmit_IT(wifi_uart, (uint8_t *)Command, (uint16_t)strlen(Command));
    // Non-blocking transmit that triggers the TC interrupt on completion.

    uint32_t tickstart = HAL_GetTick();
    while ((HAL_GetTick() - tickstart) < timeout_ms) // Wait until timeout expires.
    {
        if (wifi_rx_ready) // Check whether STM32 received a response.
        {
            if (strstr((char *)wifi_rx_shadow_buffer, expected)) // Verify expected substring.
            {
                wifi_rx_ready = 0;
                return WIFI_OK;
            }
        }
        if (strstr((char *)wifi_rx_shadow_buffer, "ERROR")) // Surface module error responses early.
        {
            return WIFI_ERROR;
        }
    }

    return WIFI_TIMEOUT;
}

// Connect directly to the configured Wi-Fi network.
wifi_status_t WiFi_Connect(const char *ssid, const char *password)
{
    char cmd[128];
    char ip[64];

    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    if (WiFi_Send_Command(cmd, "OK", 15000) != WIFI_OK)
    {
        return WIFI_ERROR;
    }

    memset(ip, 0, sizeof(ip));
    WiFi_GetIP(ip, sizeof(ip));
    if (ip[0] != '\0')
    {
        WIFI_LOG("WiFi_Connect: connected, IP = %s\r\n", ip);
        return WIFI_OK;
    }

    WIFI_LOG("WiFi_Connect: no IP, connect failed\r\n");
    return WIFI_ERROR;
}

wifi_status_t WiFi_GetIP(char *out_buf, uint16_t buf_len)
{
    if (out_buf == NULL || buf_len == 0)
    {
        return WIFI_ERROR;
    }

    wifi_status_t status = WiFi_Send_Command("AT+CIFSR\r\n", "OK", 2000); // Ask the module for its IP address.
    if (status != WIFI_OK)
    {
        return status;
    }

    strncpy(out_buf, (char *)wifi_rx_shadow_buffer, buf_len); // Copy the full text into out_buf.
    while (strstr((char *)wifi_rx_shadow_buffer, "busy p")) // If the Wi-Fi module is busy, retry after 200 ms.
    {
        HAL_Delay(200);
        status = WiFi_Send_Command("AT+CIFSR\r\n", "OK", 2000);
        if (status != WIFI_OK)
        {
            return status;
        }
        strncpy(out_buf, (char *)wifi_rx_shadow_buffer, buf_len);
    }

    return WIFI_OK;
}

wifi_status_t WiFi_SendTCP(const char *ip, uint16_t port, const char *message)
{
    if (ip == NULL || message == NULL)
    {
        return WIFI_ERROR;
    }

    char cmd[128];

    // Open a TCP connection.
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%u\r\n", ip, port); // AT+CIPSTART opens a TCP socket.
    wifi_status_t result = WiFi_Send_Command(cmd, "OK", 5000);            // Try to open a TCP socket.
    if (result != WIFI_OK && !strstr((char *)wifi_rx_shadow_buffer, "CONNECT")) // If no connection, return the error.
    {
        return result;
    }

    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u\r\n", (unsigned int)strlen(message)); // Allocate space in the Wi-Fi module.
    result = WiFi_Send_Command(cmd, ">", 2000);
    if (result != WIFI_OK)
    {
        return result;
    }

    result = WiFi_Send_Command(message, "SEND OK", 5000); // Send payload.
    if (result != WIFI_OK)
    {
        return result;
    }

    WiFi_Send_Command("AT+CIPCLOSE\r\n", "OK", 2000); // Close the connection.
    return WIFI_OK;
}

wifi_status_t WiFi_SendRaw(const uint8_t *data, uint16_t length)
{
    while (wifi_tx_done == 0)
    {
        HAL_Delay(1); // Wait until TX line is available.
    }

    if (wifi_tx_done == 1)
    {
        wifi_tx_done = 0;                                       // Lock TX line once it is available.
        HAL_UART_Transmit_IT(wifi_uart, data, length);           // Transmit data to ESP-01 module.
        while (wifi_tx_done == 0)
        {
            HAL_Delay(1); // Wait until data is sent.
        }
    }

    return WIFI_OK;
}

wifi_status_t WiFi_Expect(const char *expected, uint32_t timeout_ms)
{
    wifi_rx_ready = 0;
    memset(wifi_rx_shadow_buffer, 0, sizeof(wifi_rx_shadow_buffer));

    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < timeout_ms)
    {
        if (wifi_rx_ready == 1)
        {
            WIFI_LOG("WiFi_Expect: received -> %s\r\n", wifi_rx_shadow_buffer);
            if (strstr((char *)wifi_rx_shadow_buffer, expected))
            {
                memset(wifi_rx_shadow_buffer, 0, sizeof(wifi_rx_shadow_buffer));
                wifi_rx_ready = 0;
                return WIFI_OK;
            }
            if (strstr((char *)wifi_rx_shadow_buffer, "ERROR"))
            {
                memset(wifi_rx_shadow_buffer, 0, sizeof(wifi_rx_shadow_buffer));
                wifi_rx_ready = 0;
                return WIFI_ERROR;
            }
            wifi_rx_ready = 0;
            memset(wifi_rx_shadow_buffer, 0, sizeof(wifi_rx_shadow_buffer));
        }
    }

    WIFI_LOG("WiFi_Expect: TIMEOUT waiting for \"%s\"\r\n", expected);
    return WIFI_TIMEOUT;
}

const char *WiFi_StatusToString(wifi_status_t status)
{
    switch (status)
    {
    case WIFI_OK:
        return "OK";
    case WIFI_ERROR:
        return "ERROR";
    case WIFI_TIMEOUT:
        return "TIMEOUT";
    case WIFI_BUSY:
        return "BUSY";
    default:
        return "UNKNOWN";
    }
}
