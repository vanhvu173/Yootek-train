#include <Arduino.h>
#include "relay_driver.h"

const int RX2_PIN = 16;
const int TX2_PIN = 17;

bool isChasing = false;
unsigned long lastChaseMillis = 0;
const unsigned long CHASE_INTERVAL = 150;

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
    
    relay_driver_init();
    Serial.println("=== BO ESP32 RELAY X8 SLAVE SAN SANG ===");
}

void loop() {
    if (Serial2.available() > 0) {
        String cmd = Serial2.readStringUntil('\n');
        cmd.trim();
        cmd.toUpperCase();

        if (cmd.length() > 0) {
            Serial.println("[UART2 Nhan]: " + cmd);

            if (cmd == "R1_ON") {
                isChasing = false;
                relay_set_channel(1, true);
            } else if (cmd == "R1_OFF") {
                isChasing = false;
                relay_set_channel(1, false);
            } else if (cmd == "ALL_ON") {
                isChasing = false;
                relay_set_all(true);
            } else if (cmd == "ALL_OFF") {
                isChasing = false;
                relay_set_all(false);
            } else if (cmd == "CHASE") {
                isChasing = true;
            }
        }
    }

    unsigned long currentMillis = millis();
    if (isChasing && (currentMillis - lastChaseMillis >= CHASE_INTERVAL)) {
        lastChaseMillis = currentMillis;
        relay_run_chase_step();
    }
}