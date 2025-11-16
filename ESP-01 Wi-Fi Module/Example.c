#include "wifi_driver.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();     
    MX_USART3_UART_Init();     

    WiFi_Init(&huart1);
    WiFi_Connect("MySSID", "MyPassword");

    char ip[64];
    WiFi_GetIP(ip, sizeof(ip));
    printf("IP: %s\r\n", ip);

    WiFi_SendTCP("192.168.1.105", 5000, "Hello from STM32!\r\n");
}