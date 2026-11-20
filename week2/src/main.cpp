#include <Arduino.h>
#include "UartRelayProtocol.h"
#include "SpiFullDuplex.h"
#include "I2cMasterCom.h"

void setup() {
    Serial.begin(115200);
    Serial.println("=== KHOI DONG HE THONG DA GIAO THUC ===");

    UartRelay_Init();
    SpiFullDuplex_Init();
    I2cMaster_Init();
}

void loop() {
    UartRelay_Task();       // Task 1: Giao tiếp Binary với Relay-X8 qua UART2
    SpiFullDuplex_Task();   // Task 2: Trao đổi dữ liệu song công qua VSPI
    I2cMaster_Task();       // Task 3: Giao tiếp Master-Slave qua I2C
}