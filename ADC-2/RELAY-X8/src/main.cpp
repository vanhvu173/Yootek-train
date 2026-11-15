#include <Arduino.h>
#include "digital_output.h" // Nhúng module Digital Output riêng

#define NUM_RELAYS 8
const uint8_t RELAY_PINS[NUM_RELAYS] = {32, 33, 25, 26, 27, 14, 12, 13};

#define UART2_RX_PIN 16    // Nối với TX (17) của mạch WROOM
#define UART2_TX_PIN 17    // Nối với RX (16) của mạch WROOM

// Mảng 8 đối tượng DigitalOutput đại diện cho 8 Relay
DigitalOutput relays[NUM_RELAYS];
String cmdBuffer = "";

void guiPhanHoi(String msg) {
    Serial.println(msg);
    Serial2.println(msg);
}

// Hàm xử lý lệnh nhận được từ UART2 hoặc từ cổng USB
void xuLyLenh(String cmd) {
    cmd.trim();
    cmd.toUpperCase();
    if (cmd.length() == 0) return;

    guiPhanHoi("[SLAVE Nhan lenh]: " + cmd);

    if (cmd == "STATUS") {
        for (int i = 0; i < NUM_RELAYS; i++) {
            char buf[120];
            snprintf(buf, sizeof(buf),
                     "Relay %d (Pin %2d) | State: %-3s | Req: %-3s | Mode: %d | MinOff: %lums | Count: %lu | Run: %lus",
                     relays[i].channelId, relays[i].pin,
                     relays[i].currentState ? "ON" : "OFF",
                     relays[i].requestState ? "ON" : "OFF",
                     relays[i].mode, relays[i].minOffTimeMs,
                     relays[i].switchCount, relays[i].totalRunSeconds);
            guiPhanHoi(String(buf));
        }
    } 
    else if (cmd.startsWith("ON ")) {
        int ch = cmd.substring(3).toInt();
        if (ch >= 1 && ch <= NUM_RELAYS) {
            Output_RequestSet(&relays[ch - 1], true);
        }
    } 
    else if (cmd.startsWith("OFF ")) {
        int ch = cmd.substring(4).toInt();
        if (ch >= 1 && ch <= NUM_RELAYS) {
            Output_RequestSet(&relays[ch - 1], false);
        }
    }
    else if (cmd == "ALL ON") {
        for (int i = 0; i < NUM_RELAYS; i++) Output_RequestSet(&relays[i], true);
    }
    else if (cmd == "ALL OFF") {
        for (int i = 0; i < NUM_RELAYS; i++) Output_RequestSet(&relays[i], false);
    }
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);

    // Khởi tạo cả 8 đối tượng Relay bằng hàm của module DigitalOutput
    for (int i = 0; i < NUM_RELAYS; i++) {
        Output_Init(&relays[i], i + 1, RELAY_PINS[i], false);
    }

    Serial.println("\n=== RELAY X8 SLAVE KHOI DONG ===");
}

void loop() {
    // 1. Nhận lệnh từ WROOM qua UART2
    while (Serial2.available() > 0) {
        String uartCmd = Serial2.readStringUntil('\n');
        xuLyLenh(uartCmd);
    }

    // 2. Nhận lệnh trực tiếp từ cổng USB (Có Echo hiện chữ đang gõ & đợi nhấn Enter)
    while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (cmdBuffer.length() > 0) {
                Serial.println();
                xuLyLenh(cmdBuffer);
                cmdBuffer = "";
            }
        } else if (c == '\b' || c == 127) {
            if (cmdBuffer.length() > 0) {
                cmdBuffer.remove(cmdBuffer.length() - 1);
                Serial.print("\b \b");
            }
        } else {
            cmdBuffer += c;
            Serial.print(c);
        }
    }

    // 3. Quét cập nhật cả 8 đối tượng Relay liên tục
    for (int i = 0; i < NUM_RELAYS; i++) {
        Output_UpdateLoop(&relays[i], 400);
    }
}