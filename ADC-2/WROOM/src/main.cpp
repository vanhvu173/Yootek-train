#include <Arduino.h>
#include "Digital_Input.h"  
#include "Analog_Input.h"   

#define UART2_RX_PIN 16    // Nối với TX (17) của mạch Relay X8
#define UART2_TX_PIN 17    // Nối với RX (16) của mạch Relay X8

// Khai báo các đối tượng Input quản lý trên mạch WROOM
DigitalInput nutBoot;
AnalogInput  camBienNhietDo;

String pcCmdBuffer = "";
bool toggleRelay1 = false;

// Hàm xử lý lệnh gõ từ cmd
void xuLyLenhMayTinh(String cmd) {
    cmd.trim();
    cmd.toUpperCase();
    if (cmd.length() == 0) return;

    if (cmd == "INPUT") {
        Serial.println("========== TRANG THAI INPUT (WROOM) ==========");
        Serial.printf("[DigitalIn] Trang thai: %s | Giu lau: %s | Dem xung: %lu\n",
                      nutBoot.stableState ? "ON" : "OFF",
                      nutBoot.isLongPressed ? "CO" : "KHONG",
                      nutBoot.pulseCounter);
        Serial.printf("[AnalogIn]  ADC Tho: %u | ADC Loc: %.1f | Gia tri: %.2f C | Alarm: %d\n",
                      camBienNhietDo.rawAdc,
                      camBienNhietDo.filteredAdc,
                      camBienNhietDo.engineeringValue,
                      camBienNhietDo.alarmStatus);
        Serial.println("==============================================");
    } else {
        // Chuyển tiếp lệnh sang mạch Relay X8 qua UART2
        Serial.println("[WROOM -> Relay X8]: " + cmd);
        Serial2.println(cmd);
    }
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);

    // Khởi tạo các đối tượng Input bằng hàm của từng module
    DigitalIn_Init(&nutBoot, 0, INPUT_PULLUP, true);
    AnalogIn_Init(&camBienNhietDo, 34, 0.0f, 100.0f);

    Serial.println("\n=== WROOM MASTER KHOI DONG ===");
}

void loop() {
    // Đọc lệnh Serial từ máy tính 
    while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (pcCmdBuffer.length() > 0) {
                Serial.println();
                xuLyLenhMayTinh(pcCmdBuffer);
                pcCmdBuffer = "";
            }
        } else if (c == '\b' || c == 127) {
            if (pcCmdBuffer.length() > 0) {
                pcCmdBuffer.remove(pcCmdBuffer.length() - 1);
                Serial.print("\b \b");
            }
        } else {
            pcCmdBuffer += c;
            Serial.print(c);
        }
    }

    // Nhận phản hồi từ mạch Relay X8 gửi về
    while (Serial2.available() > 0) {
        String resp = Serial2.readStringUntil('\n');
        resp.trim();
        if (resp.length() > 0) Serial.println("\n" + resp);
    }

    // Cập nhật module Digital Input
    DigitalIn_UpdateLoop(&nutBoot);
    if (nutBoot.risingEdge) {
        toggleRelay1 = !toggleRelay1;
        Serial2.println(toggleRelay1 ? "ON 1" : "OFF 1");
    }

    // Cập nhật module Analog Input
    AnalogIn_UpdateLoop(&camBienNhietDo);
    if (camBienNhietDo.alarmStatus == ALARM_HIGH_CRIT && camBienNhietDo.lastAlarmStatus != ALARM_HIGH_CRIT) {
        Serial2.println("ON 8");  // Quá nhiệt -> Bật Relay 8
    } else if (camBienNhietDo.alarmStatus == ALARM_NORMAL && camBienNhietDo.lastAlarmStatus == ALARM_HIGH_CRIT) {
        Serial2.println("OFF 8"); // Hết quá nhiệt -> Tắt Relay 8
    }
}