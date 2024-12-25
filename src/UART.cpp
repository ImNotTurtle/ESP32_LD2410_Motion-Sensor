#include "UART.hpp"
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static uartReceiveCallback receiveCallback = NULL;
static uint8_t *rxBuffer = NULL;
static SemaphoreHandle_t rxSemaphore = NULL; // Semaphore to control rxTask

static void rxTask(void *arg);

void UART_Init(uartReceiveCallback cb)
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_driver_install(UART_INSTANCE, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_INSTANCE, &uart_config);
    uart_set_pin(UART_INSTANCE, UART_TX_PIN, UART_RX_PIN, UART_RTS_PIN, UART_CTS_PIN);

    // Create the semaphore
    rxSemaphore = xSemaphoreCreateBinary();
    if (rxSemaphore == NULL)
    {
        // Handle semaphore creation failure
        return;
    }

    // Start with the semaphore taken (rxTask will pause initially)
    xSemaphoreTake(rxSemaphore, 0);

    xTaskCreate(&rxTask, "UART_rxTask", RX_BUF_SIZE * 2, NULL, 5, NULL);

    receiveCallback = cb;
}

void UART_EnableRx()
{
    // Give the semaphore to enable rxTask
    if (rxSemaphore != NULL)
    {
        xSemaphoreGive(rxSemaphore);
    }
}

void UART_DisableRx()
{
    // Take the semaphore to disable rxTask
    if (rxSemaphore != NULL)
    {
        xSemaphoreTake(rxSemaphore, portMAX_DELAY);
    }
}

void UART_SendData(uint8_t *buffer, uint16_t length)
{
    uart_write_bytes(UART_INSTANCE, buffer, length);
}

static void rxTask(void *arg)
{
    rxBuffer = new uint8_t[RX_BUF_SIZE + 1];

    // Wait for initialization
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (true)
    {
        // Wait for the semaphore to be given in UART_EnableRx
        if (xSemaphoreTake(rxSemaphore, portMAX_DELAY) == pdTRUE)
        {
            if (receiveCallback == NULL) break;

            const uint16_t rxBytes = uart_read_bytes(UART_INSTANCE, rxBuffer, RX_BUF_SIZE, 50);
            if (rxBytes > 0)
            {
                receiveCallback(rxBuffer, rxBytes);
            }

            // Give the semaphore back so rxTask can pause again
            xSemaphoreGive(rxSemaphore);
        }
    }

    delete rxBuffer;
    vTaskDelete(NULL);
}
