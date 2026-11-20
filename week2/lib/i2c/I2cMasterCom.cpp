#include "I2cMasterCom.h"
#include "PinConfig.h"
#include <Wire.h>

static uint32_t lastI2cTime = 0;

void I2cMaster_Init() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ);
}

bool I2cMaster_WriteData(const I2cPayload_t* data) {
    Wire.beginTransmission(I2C_SLAVE_ADDR);
    Wire.write((const uint8_t*)data, sizeof(I2cPayload_t));
    return (Wire.endTransmission() == 0); // Trả về true nếu nhận được ACK
}

bool I2cMaster_ReadData(I2cPayload_t* data) {
    uint8_t bytesRead = Wire.requestFrom((uint8_t)I2C_SLAVE_ADDR, (uint8_t)sizeof(I2cPayload_t));
    if (bytesRead != sizeof(I2cPayload_t)) {
        return false;
    }

    uint8_t* ptr = (uint8_t*)data;
    for (size_t i = 0; i < sizeof(I2cPayload_t); i++) {
        ptr[i] = Wire.read();
    }
    return true;
}

void I2cMaster_Task() {
    // Định kỳ mỗi 2000ms giao tiếp với ESP32 Slave
    if (millis() - lastI2cTime >= 2000) {
        lastI2cTime = millis();

        I2cPayload_t txData = { 0x10, 28.5f, 0 };
        if (I2cMaster_WriteData(&txData)) {
            Serial.println("[I2C] Gui du lieu toi Slave thanh cong");
        }

        I2cPayload_t rxData;
        if (I2cMaster_ReadData(&rxData)) {
            Serial.printf("[I2C] Doc tu Slave -> Temp: %.2f | Err: %d\n",
                          rxData.temperature, rxData.errorCode);
        }
    }
}