#include "UartRelayProtocol.h"
#include "PinConfig.h"

static uint32_t lastTxTime = 0;
static uint8_t currentRelayMask = 0x01;

// Biến phục vụ State Machine nhận dữ liệu non-blocking
static uint8_t rxBuffer[sizeof(RelayFrame_t)];
static uint8_t rxIndex = 0;

static uint8_t CalculateChecksum(const RelayFrame_t* frame) {
    return frame->header ^ frame->command ^ frame->relayMask;
}

static void ProcessReceivedFrame(const RelayFrame_t* frame) {
    if (frame->checksum == CalculateChecksum(frame)) {
        Serial.printf("[UART] Nhan phan hoi Relay-X8 | CMD: 0x%02X | Mask: 0x%02X\n",
                      frame->command, frame->relayMask);
    } else {
        Serial.println("[UART] Loi Checksum!");
    }
}

static void HandleIncomingBytes() {
    while (Serial2.available() > 0) {
        uint8_t incomingByte = Serial2.read();

        if (rxIndex == 0 && incomingByte != RELAY_FRAME_HEADER) {
            continue; // Bỏ qua byte rác cho đến khi gặp Header 0xAA
        }

        rxBuffer[rxIndex++] = incomingByte;

        if (rxIndex >= sizeof(RelayFrame_t)) {
            RelayFrame_t* rxFrame = (RelayFrame_t*)rxBuffer;
            ProcessReceivedFrame(rxFrame);
            rxIndex = 0; // Reset chờ gói tiếp theo
        }
    }
}

void UartRelay_Init() {
    Serial2.begin(UART_RELAY_BAUD, SERIAL_8N1, UART_RELAY_RX, UART_RELAY_TX);
}

void UartRelay_SendCommand(uint8_t cmd, uint8_t mask) {
    RelayFrame_t txFrame;
    txFrame.header = RELAY_FRAME_HEADER;
    txFrame.command = cmd;
    txFrame.relayMask = mask;
    txFrame.checksum = CalculateChecksum(&txFrame);

    Serial2.write((uint8_t*)&txFrame, sizeof(RelayFrame_t));
}

void UartRelay_Task() {
    // 1. Luôn lắng nghe phản hồi từ Relay-X8
    HandleIncomingBytes();

    // 2. Định kỳ mỗi 1000ms gửi lệnh dịch bit điều khiển 8 relay tuần tự
    if (millis() - lastTxTime >= 1000) {
        lastTxTime = millis();
        UartRelay_SendCommand(RELAY_CMD_SET, currentRelayMask);
        
        currentRelayMask <<= 1;
        if (currentRelayMask == 0) currentRelayMask = 0x01;
    }
}