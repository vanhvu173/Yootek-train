#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "Digital_Input.h"
#include "Analog_Input.h"

#define WIFI_SSID        "SONGNAM-STAFF"
#define WIFI_PASS        "songnam@123"
#define GMT_OFFSET_SEC   (7 * 3600)      // GMT+7 (Việt Nam)
#define TIME_SEND_MS     10000UL         
#define BUTTON_ALWAYS_ON false           

#define UART2_RX_PIN 16    
#define UART2_TX_PIN 17    

DigitalInput nutBoot;
AnalogInput  camBienNhietDo;

String pcCmdBuffer = "";
String relayRespBuffer = "";
bool toggleRelay1 = false;
AlarmState prevAlarm = ALARM_NORMAL;
uint32_t lastTimeSendMs = 0;

// Lấy giờ địa phương từ NTP. w: 1 = Thứ 2 ... 7 = Chủ nhật
bool layGioThuc(int &h, int &m, int &s, int &w) {
    time_t now = time(nullptr);
    if (now < 1700000000) return false;        
    struct tm t;
    localtime_r(&now, &t);
    h = t.tm_hour; m = t.tm_min; s = t.tm_sec;
    w = (t.tm_wday == 0) ? 7 : t.tm_wday;      // tm_wday: 0 = Chủ nhật -> đổi thành 7
    return true;
}

// Gửi giờ hiện tại sang Relay X8
void guiGioSangRelay() {
    int h, m, s, w;
    if (!layGioThuc(h, m, s, w)) return;
    char buf[28];
    snprintf(buf, sizeof(buf), "TIME %02d:%02d:%02d %d", h, m, s, w);
    Serial2.println(buf);
}

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
    } else if (cmd == "NTP") {
        int h, m, s, w;
        Serial.printf("[WiFi] %s | ", WiFi.status() == WL_CONNECTED ? "Da ket noi" : "CHUA ket noi");
        if (layGioThuc(h, m, s, w)) Serial.printf("Gio hien tai: %02d:%02d:%02d (thu %d, 1=T2..7=CN)\n", h, m, s, w);
        else Serial.println("Chua dong bo gio NTP");
    } else {
        // Chuyển tiếp mọi lệnh khác sang Relay X8 (MODE, SCHED, PULSE, ON, OFF, STATUS, ...)
        Serial.println("[WROOM -> Relay X8]: " + cmd);
        Serial2.println(cmd);
    }
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);

    DigitalIn_Init(&nutBoot, 0, INPUT_PULLUP, true);
    AnalogIn_Init(&camBienNhietDo, 34, 0.0f, 100.0f);

    // Kết nối WiFi không chặn + bật đồng bộ giờ NTP
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    configTime(GMT_OFFSET_SEC, 0, "pool.ntp.org", "time.google.com");

    Serial.println("\n=== WROOM MASTER KHOI DONG ===");
    Serial.println("Lenh: INPUT | NTP | STATUS | ON n | OFF n | ALL ON | ALL OFF | MODE n m | SCHED ... | PULSE n ms | MINOFF n ms | MAXRUN n ms | FACTORY | HELP");
}

void loop() {

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

    //Nhận phản hồi từ Relay X8
    while (Serial2.available() > 0) {
        char c = Serial2.read();
        if (c == '\n' || c == '\r') {
            if (relayRespBuffer.length() > 0) {
                Serial.println("\n" + relayRespBuffer);
                relayRespBuffer = "";
            }
        } else {
            relayRespBuffer += c;
        }
    }

    //Gửi giờ thật sang Relay X8 định kỳ 
    if (millis() - lastTimeSendMs >= TIME_SEND_MS) {
        lastTimeSendMs = millis();
        guiGioSangRelay();
    }

    //Digital Input: nhấn nút BOOT điều khiển Relay 1
    DigitalIn_UpdateLoop(&nutBoot);
    if (nutBoot.risingEdge) {
        if (BUTTON_ALWAYS_ON) {
            Serial2.println("ON 1");
        } else {
            toggleRelay1 = !toggleRelay1;
            Serial2.println(toggleRelay1 ? "ON 1" : "OFF 1");
        }
    }

    //Analog Input: chỉ gửi lệnh khi trạng thái alarm thay đổi
    AnalogIn_UpdateLoop(&camBienNhietDo);
    if (camBienNhietDo.alarmStatus != prevAlarm) {
        if (camBienNhietDo.alarmStatus == ALARM_HIGH_CRIT) {
            Serial2.println("ON 8");     // Quá nhiệt -> Bật Relay 8
        } else if (prevAlarm == ALARM_HIGH_CRIT) {
            Serial2.println("OFF 8");    // Hết quá nhiệt -> Tắt Relay 8
        }
        prevAlarm = camBienNhietDo.alarmStatus;
    }
}