#include <Arduino.h>
#include "I2cSlaveCom.h"

void setup() {
    Serial.begin(115200);
    Serial.println("=== KHOI DONG ESP32 I2C SLAVE ===");

    I2cSlave_Init();
}

void loop() {
    I2cSlave_Task();        // Task 1: Xử lý lệnh nhận từ I2C Master
}