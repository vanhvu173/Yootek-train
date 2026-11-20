#ifndef I2C_MASTER_COM_H
#define I2C_MASTER_COM_H

#include <Arduino.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t command;
    float temperature;
    uint8_t errorCode;
} I2cPayload_t;
#pragma pack(pop)

void I2cMaster_Init();
bool I2cMaster_WriteData(const I2cPayload_t* data);
bool I2cMaster_ReadData(I2cPayload_t* data);
void I2cMaster_Task();

#endif