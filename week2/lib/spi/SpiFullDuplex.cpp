#include "SpiFullDuplex.h"
#include "PinConfig.h"
#include <SPI.h>

static SPIClass* vspi = nullptr;
static uint32_t lastSpiTime = 0;
static uint8_t counter = 0;

void SpiFullDuplex_Init() {
    pinMode(SPI_CS_PIN, OUTPUT);
    digitalWrite(SPI_CS_PIN, HIGH);

    vspi = new SPIClass(VSPI);
    vspi->begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CS_PIN);
}

void SpiFullDuplex_Transfer(const SpiPacket_t* txData, SpiPacket_t* rxData) {
    const uint8_t* txBytes = (const uint8_t*)txData;
    uint8_t* rxBytes = (uint8_t*)rxData;

    vspi->beginTransaction(SPISettings(SPI_CLOCK_HZ, MSBFIRST, SPI_MODE0));
    digitalWrite(SPI_CS_PIN, LOW);

    // Vừa đẩy từng byte đi (MOSI) vừa nhận từng byte về (MISO) đồng thời
    for (size_t i = 0; i < sizeof(SpiPacket_t); i++) {
        rxBytes[i] = vspi->transfer(txBytes[i]);
    }

    digitalWrite(SPI_CS_PIN, HIGH);
    vspi->endTransaction();
}

void SpiFullDuplex_Task() {
    // Định kỳ mỗi 500ms thực hiện 1 phiên trao đổi song công
    if (millis() - lastSpiTime >= 500) {
        lastSpiTime = millis();

        SpiPacket_t txPkt = { counter++, (uint16_t)analogRead(34), 0x01 };
        SpiPacket_t rxPkt = { 0, 0, 0 };

        SpiFullDuplex_Transfer(&txPkt, &rxPkt);

        Serial.printf("[SPI] Gui ID: %d | Nhan ID: %d, Val: %d\n",
                      txPkt.packetId, rxPkt.packetId, rxPkt.sensorValue);
    }
}