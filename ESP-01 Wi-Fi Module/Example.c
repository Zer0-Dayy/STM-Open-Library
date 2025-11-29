#include "wifi_basic_driver.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();    //IN THIS CASE THE USART-1 WILL BE CONNECTED TO THE ESP-01 MODULE
    MX_USART3_UART_Init();    //INITIALIZE ST-LINK IN USRAT-3 TO PRINT RESULTS IN SERIAL MONITOR

    WiFi_Init(&huart1);    //INITIALIZE WIFI
    WiFi_Connect("MySSID", "MyPassword"); //CONNECT TO WIFI

    char ip[64];
    WiFi_GetIP(ip, sizeof(ip));    //FETCH THE IP ADDRESS AND COPY RESULT IN A STRING
    printf("IP: %s\r\n", ip);

    WiFi_SendTCP("192.168.1.105", 5000, "Hello from STM32!\r\n");
}
