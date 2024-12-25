#include "SerialHandle.hpp"
#include <UART.hpp>
#include <ctype.h>

#include <HardwareSerial.h>
extern HardwareSerial Serial;

#define HEADER1 0xFD
#define HEADER2 0xFC
#define HEADER3 0xFB
#define HEADER4 0xFA
#define END1 0x04
#define END2 0x03
#define END3 0x02
#define END4 0x01
#define HEADER HEADER1, HEADER2, HEADER3, HEADER4
#define END END1, END2, END3, END4

static const uint8_t HEADER_FRAME[] = {HEADER};
static const uint8_t END_FRAME[] = {END};
static const uint8_t ACK_ENABLE_CONFIG_FRAME[] = {0x08, 0x00, 0xFF, 0x01, 0x00, 0x00};
static const uint8_t ACK_END_CONFIG_FRAME[] = {0x04, 0x00, 0xFE, 0x01};
static const uint8_t ACK_DISTANCE_DURATION_FRAME[] = {0x04, 0x00, 0x60, 0x01, 0x00, 0x00};
static const uint8_t ACK_SENSITIVITY_FRAME[] = {0x04, 0x00, 0x64, 0x01, 0x00, 0x00};
static const uint8_t ACK_READ_PARAM_FRAME[] = {0x1C, 0x00, 0x61, 0x01, 0x00, 0x00, 0xAA}; // 0x1C = 28 characters long, header and end frame included
// static const uint8_t ACK_SENSITIVITY_FRAME

static const uint8_t COMMAND_ENABLE_CONFIG[] = {HEADER, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, END};
static const uint8_t COMMAND_END_CONFIG[] = {HEADER, 0x02, 0x00, 0xFE, 0x00, END};
static const uint8_t COMMAND_READ_PARAMS[] = {HEADER, 0x02, 0x00, 0x61, 0x00, END};

static uint8_t frameBuffer[50] = {0};
static uint16_t frameBufferLength = 0;

static ackReceiveCallback receiveCallback = NULL;

static void UARTReceiveHandle(uint8_t *buffer, uint16_t length);
static void HandleAckFrame(uint8_t *buffer, uint8_t length);
static void SetHeader(uint8_t buffer[], uint16_t *length);
static void SetEndFrame(uint8_t buffer[], uint16_t *length);
static bool CompareBuffer(const uint8_t *buffer1, const uint8_t *buffer2, uint16_t length);

void SerialHandle_Init(ackReceiveCallback cb)
{
    UART_Init(UARTReceiveHandle);
    receiveCallback = cb;
}

void SerialHandle_SendDistanceDurationConfig(DISTANCE distance, DURATION duration)
{
    UART_EnableRx();
    UART_SendData((uint8_t *)COMMAND_ENABLE_CONFIG, sizeof(COMMAND_ENABLE_CONFIG));

    uint8_t buffer[50];
    uint16_t length = 0;

    SetHeader(buffer, &length);

    buffer[length++] = 0x14;
    buffer[length++] = 0x00;

    buffer[length++] = 0x60;
    buffer[length++] = 0x00;

    buffer[length++] = 0x00;
    buffer[length++] = 0x00;

    buffer[length++] = (uint8_t)distance;
    buffer[length++] = 0x00;
    buffer[length++] = 0x00;
    buffer[length++] = 0x00;

    buffer[length++] = 0x01;
    buffer[length++] = 0x00;

    buffer[length++] = (uint8_t)distance;
    buffer[length++] = 0x00;
    buffer[length++] = 0x00;
    buffer[length++] = 0x00;

    buffer[length++] = 0x02;
    buffer[length++] = 0x00;

    buffer[length++] = (uint8_t)duration;
    buffer[length++] = 0x00;
    buffer[length++] = 0x00;
    buffer[length++] = 0x00;

    SetEndFrame(buffer, &length);
    UART_SendData(buffer, length);

    UART_SendData((uint8_t *)COMMAND_END_CONFIG, sizeof(COMMAND_END_CONFIG));
}
void SerialHandle_SendReadParams(void)
{
    UART_EnableRx();
    UART_SendData((uint8_t *)COMMAND_ENABLE_CONFIG, sizeof(COMMAND_ENABLE_CONFIG));
    UART_SendData((uint8_t *)COMMAND_READ_PARAMS, sizeof(COMMAND_READ_PARAMS));
    UART_SendData((uint8_t *)COMMAND_END_CONFIG, sizeof(COMMAND_END_CONFIG));
}
void SerialHandle_SendSensitivityConfig(uint8_t sen)
{
    UART_EnableRx();
    UART_SendData((uint8_t *)COMMAND_ENABLE_CONFIG, sizeof(COMMAND_ENABLE_CONFIG));

    uint8_t buffer[50];
    uint16_t length = 0;

    SetHeader(buffer, &length);

    buffer[length++] = 0x14;
    buffer[length++] = 0x00;

    buffer[length++] = 0x64;
    buffer[length++] = 0x00;

    buffer[length++] = 0x00;
    buffer[length++] = 0x00;

    buffer[length++] = 0xFF;
    buffer[length++] = 0xFF;
    buffer[length++] = 0x00;
    buffer[length++] = 0x00;

    buffer[length++] = 0x01;
    buffer[length++] = 0x00;

    buffer[length++] = sen;
    buffer[length++] = 0x00;
    buffer[length++] = 0x00;
    buffer[length++] = 0x00;

    buffer[length++] = 0x02;
    buffer[length++] = 0x00;

    buffer[length++] = sen;
    buffer[length++] = 0x00;
    buffer[length++] = 0x00;
    buffer[length++] = 0x00;

    SetEndFrame(buffer, &length);
    UART_SendData(buffer, length);

    UART_SendData((uint8_t *)COMMAND_END_CONFIG, sizeof(COMMAND_END_CONFIG));
}
void SerialHandle_HandleConfig(uint8_t* buffer, uint16_t length){
    //buffer[0] is command Id, else is function payload
    if(buffer[0] == 0x60){ // distance_duration config
        SerialHandle_SendDistanceDurationConfig(DISTANCE(buffer[1]), DURATION(buffer[2]));
    }
    else if(buffer[0] == 0x61){ //read params config
        SerialHandle_SendReadParams();
    }
    else if(buffer[0] == 0x64){
        SerialHandle_SendSensitivityConfig(buffer[1]);
    }
}

static void UARTReceiveHandle(uint8_t *buffer, uint16_t length)
{
    bool startDetect = false;
    bool endCase = false;
    for (uint16_t i = 0; i < length - 4; i++)
    {
        // extract the frame by header and end frame
        if (startDetect == false && buffer[i] != HEADER_FRAME[0])
        {
            continue;
        }

        if (startDetect == false && CompareBuffer(buffer + i, HEADER_FRAME, sizeof(HEADER_FRAME)))
        {
            startDetect = true;
            i += 3; // skip header
            continue;
        }

        // process when header is found
        if (startDetect == true)
        {
            // push frame to buffer
            //  frame length is actually uint16_t, but the LSB is all 0s
            uint8_t frameLength = buffer[i] + 2; // word length included (2 bytes)

            // handle the frame
            HandleAckFrame(buffer + i, frameLength);
            endCase = true;
        }

        // do reset
        if (endCase == true)
        {
            startDetect = false;
            frameBufferLength = 0;
            endCase = false;
            i += 4; // skip end of frame
        }
    }
}

static void HandleAckFrame(uint8_t *buffer, uint8_t length)
{
    if (receiveCallback == NULL)
        return;
    // now have the frame, compare to see which ack is this
    // enable config
    if (CompareBuffer(buffer, ACK_ENABLE_CONFIG_FRAME, sizeof(ACK_ENABLE_CONFIG_FRAME)))
    {
        receiveCallback(COMMAND_TYPE::ENABLE_CONFIG, NULL, 0);
    }
    // end config
    if (CompareBuffer(buffer, ACK_END_CONFIG_FRAME, sizeof(ACK_END_CONFIG_FRAME)))
    {
        receiveCallback(COMMAND_TYPE::END_CONFIG, NULL, 0);
        // should disable rx now
        UART_DisableRx();
    }
    // distance
    if (CompareBuffer(buffer, ACK_DISTANCE_DURATION_FRAME, sizeof(ACK_DISTANCE_DURATION_FRAME)))
    {
        receiveCallback(COMMAND_TYPE::DISTANCE_DURATION, NULL, 0);
    }
    // read param
    if (CompareBuffer(buffer, ACK_READ_PARAM_FRAME, sizeof(ACK_READ_PARAM_FRAME)))
    {
        receiveCallback(COMMAND_TYPE::READ_PARAMS, buffer, length);
    }
    if(CompareBuffer(buffer, ACK_SENSITIVITY_FRAME, sizeof(ACK_SENSITIVITY_FRAME))){
        receiveCallback(COMMAND_TYPE::SENSITIVITY, NULL, 0);
    }
}

static void SetHeader(uint8_t buffer[], uint16_t *length)
{
    buffer[(*(length))++] = HEADER1;
    buffer[(*(length))++] = HEADER2;
    buffer[(*(length))++] = HEADER3;
    buffer[(*(length))++] = HEADER4;
}
static void SetEndFrame(uint8_t buffer[], uint16_t *length)
{
    buffer[(*(length))++] = END1;
    buffer[(*(length))++] = END2;
    buffer[(*(length))++] = END3;
    buffer[(*(length))++] = END4;
}
static bool CompareBuffer(const uint8_t *buffer1, const uint8_t *buffer2, uint16_t length)
{
    uint16_t i = 0;
    while (i < length && buffer1[i] == buffer2[i])
    {
        i++;
    }
    return i == length;
}