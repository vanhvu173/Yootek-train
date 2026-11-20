#ifndef SPI_FULL_DUPLEX_H
#define SPI_FULL_DUPLEX_H

#include <Arduino.h>

// Gói tin trao đổi song công cố định kích thước
#pragma pack(push, 1)
typedef struct {
    uint8_t  packetId;
    uint16_t sensorValue;
    uint8_t  statusFlag;
} SpiPacket_t;
#pragma pack(pop)

void SpiFullDuplex_Init();
void SpiFullDuplex_Transfer(const SpiPacket_t* txData, SpiPacket_t* rxData);
void SpiFullDuplex_Task();

#endif