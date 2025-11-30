#include "wifi_basic_driver.h"
#include "wifi_http_support.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();    // USART CONNECTED TO ESP-01
    MX_USART3_UART_Init();    // SERIAL TERMINAL FOR LOGS

    WiFi_Init(&huart1);       // INITIALIZE WI-FI MODULE
    WiFi_Connect("MySSID", "MyPassword"); // CONNECT TO WIFI

    char ip[64];
    WiFi_GetIP(ip, sizeof(ip));    // FETCH THE IP ADDRESS
    printf("IP: %s\r\n", ip);

    WiFi_SendTCP("192.168.1.105", 5000, "Hello from STM32!\r\n");

    // SIMPLE HTTP HELPERS
    // Create an optional header list. Terminate with {NULL, NULL}.
    wifi_http_header_t headers[] = {
        {"Content-Type", "application/json"},
        {NULL, NULL}
    };

    // GET without a body
    WiFi_HTTP_GET("192.168.1.200", 80, "/status", NULL, 5000, 3);

    // POST with a JSON body
    WiFi_HTTP_POST("192.168.1.200", 80, "/items", headers, "{\"name\":\"demo\"}", 5000, 3);

    // PUT to update a resource
    WiFi_HTTP_PUT("192.168.1.200", 80, "/items/1", headers, "{\"name\":\"updated\"}", 5000, 3);

    // DELETE without a body
    WiFi_HTTP_DELETE("192.168.1.200", 80, "/items/1", NULL, 5000, 3);
}
