#ifndef UART_HPP
#define UART_HPP
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/uart.h"

#define UART_TX_PIN                                          4
#define UART_RX_PIN                                          22
#define UART_CTS_PIN                                         UART_PIN_NO_CHANGE
#define UART_RTS_PIN                                         UART_PIN_NO_CHANGE
#define UART_QUEUE_BUFFER_SIZE                               128
#define RX_BUF_SIZE                                          ((uint16_t)(1024)) //1024
#define UART_BAUD_RATE                                       256000
#define UART_INSTANCE                                        UART_NUM_1



typedef void (*uartReceiveCallback)(uint8_t* buffer, uint16_t length);

void UART_Init(uartReceiveCallback cb);
void UART_EnableRx();
void UART_DisableRx();  
void UART_SendData(uint8_t* buffer, uint16_t length);




#endif