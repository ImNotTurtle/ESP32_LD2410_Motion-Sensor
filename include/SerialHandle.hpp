#ifndef SERIAL_HANDLE_HPP
#define SERIAL_HANDLE_HPP
#include <stdint.h>

enum class COMMAND_TYPE{
    EMPTY,
    ENABLE_CONFIG,
    END_CONFIG,
    DISTANCE_DURATION,
    SENSITIVITY,
    NO_ONE_DURATION,
    READ_PARAMS
};
enum class DISTANCE{
    _1M = 1,
    _2M,
    _3M,
    _4M,
    _5M,
    _6M,
    _7M,
    _8M
};

enum class DURATION{
    _1S = 1,
    _2S,
    _3S,
    _4S,
    _5S
};

typedef void (*ackReceiveCallback)(COMMAND_TYPE type, uint8_t* buffer, uint16_t length);


void SerialHandle_Init(ackReceiveCallback cb);
void SerialHandle_SendDistanceDurationConfig(DISTANCE distance, DURATION duration);
void SerialHandle_SendReadParams(void);
void SerialHandle_SendSensitivityConfig(uint8_t sen); // range 0 - 100
void SerialHandle_HandleConfig(uint8_t* buffer, uint16_t length);


#endif