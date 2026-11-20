#ifndef UART_RELAY_PROTOCOL_H
#define UART_RELAY_PROTOCOL_H

#include <Arduino.h>

#define RELAY_FRAME_HEADER  0xAA
#define RELAY_CMD_SET       0x01
#define RELAY_CMD_STATUS    0x02

// Đóng gói khung truyền 4 byte (không padding)
#pragma pack(push, 1)
typedef struct {
    uint8_t header;
    uint8_t command;
    uint8_t relayMask; // Bit 0..7 tương ứng Relay 1..8
    uint8_t checksum;  // XOR của 3 byte đầu
} RelayFrame_t;
#pragma pack(pop)

void UartRelay_Init();
void UartRelay_SendCommand(uint8_t cmd, uint8_t mask);
void UartRelay_Task();

#endif