#include <Arduino.h>
#include <Preferences.h>     
#include "digital_output.h"

#define NUM_RELAYS 8
const uint8_t RELAY_PINS[NUM_RELAYS] = {32, 33, 25, 26, 27, 14, 12, 13};

#define RELAY_ACTIVE_LOW true

#define UART2_RX_PIN 16    
#define UART2_TX_PIN 17    

#define TIME_TIMEOUT_MS 60000UL  
#define CFG_VERSION 2            

DigitalOutput relays[NUM_RELAYS];
Preferences prefs;
String cmdBuffer = "";      // Buffer lệnh từ cổng USB
String uartBuffer = "";     // Buffer lệnh từ WROOM qua UART2

// Giờ nhận từ WROOM
uint16_t currentMinute = 0;      
uint8_t  currentWeekday = 0;      // 1 = T2 ... 7 = CN, 0 = chưa biết
uint32_t lastTimeRxMs = 0;
bool everGotTime = false;

const char *MODE_NAMES[] = {"DISABLE", "MANUAL", "SCHEDULE", "PULSE"};

// Phần cấu hình của mỗi kênh được lưu vào flash
struct ChannelConfig {
    uint8_t version;
    uint8_t mode;
    ScheduleSlot slots[MAX_SCHEDULE_SLOTS];
    uint32_t pulseDurationMs;
    uint32_t minOffTimeMs;
    uint32_t minOnTimeMs;
    uint32_t maxRunTimeMs;
};

// LƯU/NẠP CẤU HÌNH 
void luuKenh(int idx) {
    ChannelConfig cfg;
    cfg.version = CFG_VERSION;
    cfg.mode = (uint8_t)relays[idx].mode;
    memcpy(cfg.slots, relays[idx].slots, sizeof(cfg.slots));
    cfg.pulseDurationMs = relays[idx].pulseDurationMs;
    cfg.minOffTimeMs = relays[idx].minOffTimeMs;
    cfg.minOnTimeMs = relays[idx].minOnTimeMs;
    cfg.maxRunTimeMs = relays[idx].maxRunTimeMs;

    char key[16];
    snprintf(key, sizeof(key), "ch%d", idx + 1);
    prefs.begin("relayx8", false);
    prefs.putBytes(key, &cfg, sizeof(cfg));
    prefs.end();
}

void napCauHinh() {
    prefs.begin("relayx8", false);
    for (int i = 0; i < NUM_RELAYS; i++) {
        char key[16];
        snprintf(key, sizeof(key), "ch%d", i + 1);
        if (prefs.getBytesLength(key) != sizeof(ChannelConfig)) continue;

        ChannelConfig cfg;
        prefs.getBytes(key, &cfg, sizeof(cfg));
        if (cfg.version != CFG_VERSION || cfg.mode > 3) continue;

        Output_SetMode(&relays[i], (OutputMode)cfg.mode);
        for (int s = 0; s < MAX_SCHEDULE_SLOTS; s++) {
            relays[i].slots[s] = cfg.slots[s];
            if (relays[i].slots[s].onMin > 1439)  relays[i].slots[s].onMin = 0;    // Chống dữ liệu hỏng
            if (relays[i].slots[s].offMin > 1439) relays[i].slots[s].offMin = 0;
            relays[i].slots[s].dayMask &= DAYS_ALL;
        }
        relays[i].pulseDurationMs = cfg.pulseDurationMs;
        relays[i].minOffTimeMs = cfg.minOffTimeMs;
        relays[i].minOnTimeMs = cfg.minOnTimeMs;
        relays[i].maxRunTimeMs = cfg.maxRunTimeMs;
    }
    prefs.end();
}

void xoaCauHinh() {
    prefs.begin("relayx8", false);
    prefs.clear();
    prefs.end();
}

// TIỆN ÍCH
void guiPhanHoi(String msg) {
    Serial.println(msg);
    Serial2.println(msg);
}

bool gioConHieuLuc() {
    return everGotTime && (millis() - lastTimeRxMs < TIME_TIMEOUT_MS);
}

// Tách chuỗi theo khoảng trắng, trả về số phần tử
int chiaTu(const String &s, String out[], int maxN) {
    int n = 0, i = 0, len = s.length();
    while (i < len && n < maxN) {
        while (i < len && s[i] == ' ') i++;
        if (i >= len) break;
        int j = i;
        while (j < len && s[j] != ' ') j++;
        out[n++] = s.substring(i, j);
        i = j;
    }
    return n;
}

// "06:30" -> 390 phút
bool docGioPhut(const String &s, uint16_t &minute) {
    int h = -1, m = -1;
    if (sscanf(s.c_str(), "%d:%d", &h, &m) != 2) return false;
    if (h < 0 || h > 23 || m < 0 || m > 59) return false;
    minute = (uint16_t)(h * 60 + m);
    return true;
}

// Ngày: ALL | WEEKDAYS | WEEKEND | chuỗi chữ số, ví dụ 12345 (1 = T2 ... 6 = T7, 7 = CN)
bool docNgay(const String &s, uint8_t &mask) {
    if (s == "ALL")      { mask = DAYS_ALL; return true; }
    if (s == "WEEKDAYS") { mask = 0x1F;     return true; }   // T2..T6
    if (s == "WEEKEND")  { mask = 0x60;     return true; }   // T7, CN
    uint8_t m = 0;
    for (unsigned i = 0; i < s.length(); i++) {
        char c = s[i];
        if (c < '1' || c > '7') return false;
        m |= (1 << (c - '1'));
    }
    if (m == 0) return false;
    mask = m;
    return true;
}

String ngayRaChuoi(uint8_t mask) {
    String r = "";
    for (int d = 0; d < 7; d++) r += (mask & (1 << d)) ? char('1' + d) : '-';
    return r;
}

String gioPhutRaChuoi(uint16_t minute) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%02u:%02u", minute / 60, minute % 60);
    return String(buf);
}

void inLich(int idx) {
    for (int s = 0; s < MAX_SCHEDULE_SLOTS; s++) {
        const ScheduleSlot &sl = relays[idx].slots[s];
        String line = "Relay " + String(idx + 1) + " | Slot " + String(s + 1) + ": ";
        if (sl.enabled)
            line += gioPhutRaChuoi(sl.onMin) + " -> " + gioPhutRaChuoi(sl.offMin) + " | Ngay: " + ngayRaChuoi(sl.dayMask);
        else
            line += "(trong)";
        guiPhanHoi(line);
    }
}

void inHuongDan() {
    guiPhanHoi("--- LENH RELAY X8 ---");
    guiPhanHoi("STATUS                         : xem trang thai 8 relay");
    guiPhanHoi("ON n / OFF n                   : bat / tat relay n (1..8)");
    guiPhanHoi("ALL ON / ALL OFF               : bat / tat tat ca");
    guiPhanHoi("MODE n m                       : m = 0 DISABLE, 1 MANUAL, 2 SCHEDULE, 3 PULSE");
    guiPhanHoi("SCHED                          : xem lich tat ca relay;  SCHED n : xem lich relay n");
    guiPhanHoi("SCHED n s hh:mm hh:mm [ngay]   : dat khung s (1..4) cho relay n");
    guiPhanHoi("   ngay = ALL (mac dinh) | WEEKDAYS (T2-T6) | WEEKEND (T7,CN) | chu so 1..7, VD 135 = T2,T4,T6 (1=T2 .. 6=T7, 7=CN)");
    guiPhanHoi("SCHED n s CLR                  : xoa khung s cua relay n");
    guiPhanHoi("   gio bat > gio tat = khung qua nua dem (VD 22:00 06:00), ngay tinh theo ngay bat dau");
    guiPhanHoi("PULSE n ms                     : dat do rong xung (ms)");
    guiPhanHoi("MINOFF n ms                    : thoi gian nghi toi thieu giua 2 lan bat");
    guiPhanHoi("MAXRUN n ms                    : thoi gian chay toi da (0 = khong gioi han)");
    guiPhanHoi("TIME hh:mm:ss [thu]            : dat gio thu cong (WROOM tu gui qua NTP)");
    guiPhanHoi("FACTORY                        : xoa toan bo cau hinh da luu, ve mac dinh");
    guiPhanHoi("Moi thay doi MODE/SCHED/PULSE/MINOFF/MAXRUN duoc luu tu dong, khong mat khi cup dien.");

}

//XỬ LÝ LỆNH
void xuLyLenh(String cmd) {
    cmd.trim();
    cmd.toUpperCase();
    if (cmd.length() == 0) return;

    // Lệnh TIME do WROOM gửi định kỳ -> xử lý im lặng, không echo để khỏi spam màn hình
    if (cmd.startsWith("TIME ")) {
        int h = -1, m = -1, s = 0, w = 0;
        int n = sscanf(cmd.c_str() + 5, "%d:%d:%d %d", &h, &m, &s, &w);
        if (n >= 2 && h >= 0 && h <= 23 && m >= 0 && m <= 59) {
            currentMinute = (uint16_t)(h * 60 + m);
            currentWeekday = (n >= 4 && w >= 1 && w <= 7) ? (uint8_t)w : 0;
            lastTimeRxMs = millis();
            everGotTime = true;
        }
        return;
    }

    guiPhanHoi("[SLAVE Nhan lenh]: " + cmd);

    if (cmd == "STATUS") {
        if (gioConHieuLuc()) {
            String thu = (currentWeekday == 0) ? "?" : (currentWeekday == 7 ? "CN" : "T" + String(currentWeekday + 1));
            guiPhanHoi("Gio hien tai: " + gioPhutRaChuoi(currentMinute) + " (" + thu + ")");
        } else {
            guiPhanHoi("Gio hien tai: CHUA CO (Schedule se giu relay TAT)");
        }

        for (int i = 0; i < NUM_RELAYS; i++) {
            int soKhung = 0;
            for (int s = 0; s < MAX_SCHEDULE_SLOTS; s++) if (relays[i].slots[s].enabled) soKhung++;

            char buf[200];
            snprintf(buf, sizeof(buf),
                     "Relay %d (Pin %2d) | State: %-3s | Req: %-3s | Mode: %-8s | Slots: %d | Pulse: %lums | MinOff: %lums | MaxRun: %lums | Fault: %s | Count: %lu | Run: %lus",
                     relays[i].channelId, relays[i].pin,
                     relays[i].currentState ? "ON" : "OFF",
                     relays[i].requestState ? "ON" : "OFF",
                     MODE_NAMES[relays[i].mode],
                     soKhung,
                     (unsigned long)relays[i].pulseDurationMs,
                     (unsigned long)relays[i].minOffTimeMs,
                     (unsigned long)relays[i].maxRunTimeMs,
                     relays[i].isTimeoutFault ? "TIMEOUT" : "no",
                     (unsigned long)relays[i].switchCount,
                     (unsigned long)relays[i].totalRunSeconds);
            guiPhanHoi(String(buf));
        }
    }
    else if (cmd.startsWith("ON ")) {
        int ch = cmd.substring(3).toInt();
        if (ch >= 1 && ch <= NUM_RELAYS) {
            if (!Output_RequestSet(&relays[ch - 1], true))
                guiPhanHoi("REJECT: Relay " + String(ch) + " dang bi khoa (DISABLE/Interlock)");
        } else {
            guiPhanHoi("ERR: ON <1..8>");
        }
    }
    else if (cmd.startsWith("OFF ")) {
        int ch = cmd.substring(4).toInt();
        if (ch >= 1 && ch <= NUM_RELAYS) {
            Output_RequestSet(&relays[ch - 1], false);
        } else {
            guiPhanHoi("ERR: OFF <1..8>");
        }
    }
    else if (cmd == "ALL ON") {
        for (int i = 0; i < NUM_RELAYS; i++) Output_RequestSet(&relays[i], true);
    }
    else if (cmd == "ALL OFF") {
        for (int i = 0; i < NUM_RELAYS; i++) Output_RequestSet(&relays[i], false);
    }
    else if (cmd.startsWith("MODE ")) {
        int ch = 0, m = -1;
        if (sscanf(cmd.c_str() + 5, "%d %d", &ch, &m) == 2 &&
            ch >= 1 && ch <= NUM_RELAYS && m >= 0 && m <= 3) {
            Output_SetMode(&relays[ch - 1], (OutputMode)m);
            luuKenh(ch - 1);
            guiPhanHoi("OK: Relay " + String(ch) + " -> " + MODE_NAMES[m]);
        } else {
            guiPhanHoi("ERR: MODE <1..8> <0..3>");
        }
    }
    else if (cmd == "SCHED" || cmd.startsWith("SCHED ")) {
        String t[8];
        int n = chiaTu(cmd, t, 8);
        const char *usage = "ERR: SCHED | SCHED n | SCHED n s hh:mm hh:mm [ngay] | SCHED n s CLR  (go HELP de xem chi tiet)";

        if (n == 1) {
            for (int i = 0; i < NUM_RELAYS; i++) inLich(i);
        } else {
            int ch = t[1].toInt();
            if (ch < 1 || ch > NUM_RELAYS) {
                guiPhanHoi(usage);
            } else if (n == 2) {
                inLich(ch - 1);
            } else {
                int s = t[2].toInt();
                uint16_t on, off;
                uint8_t mask = DAYS_ALL;
                if (s < 1 || s > MAX_SCHEDULE_SLOTS) {
                    guiPhanHoi(usage);
                } else if (n == 4 && (t[3] == "CLR" || t[3] == "DEL")) {
                    relays[ch - 1].slots[s - 1].enabled = false;
                    luuKenh(ch - 1);
                    guiPhanHoi("OK: da xoa Relay " + String(ch) + " Slot " + String(s));
                } else if (n >= 5 && docGioPhut(t[3], on) && docGioPhut(t[4], off) &&
                           (n < 6 || docNgay(t[5], mask))) {
                    if (on == off) {
                        guiPhanHoi("ERR: gio bat va gio tat khong duoc trung nhau");
                    } else {
                        ScheduleSlot &sl = relays[ch - 1].slots[s - 1];
                        sl.enabled = true;
                        sl.onMin = on;
                        sl.offMin = off;
                        sl.dayMask = mask;
                        luuKenh(ch - 1);
                        guiPhanHoi("OK: Relay " + String(ch) + " Slot " + String(s) + ": " +
                                   gioPhutRaChuoi(on) + " -> " + gioPhutRaChuoi(off) + " | Ngay: " + ngayRaChuoi(mask));
                        if (relays[ch - 1].mode != OUT_MODE_SCHEDULE)
                            guiPhanHoi("LUU Y: Relay " + String(ch) + " chua o che do SCHEDULE. Go: MODE " + String(ch) + " 2");
                    }
                } else {
                    guiPhanHoi(usage);
                }
            }
        }
    }
    else if (cmd.startsWith("PULSE ")) {
        int ch = 0; long ms = 0;
        if (sscanf(cmd.c_str() + 6, "%d %ld", &ch, &ms) == 2 &&
            ch >= 1 && ch <= NUM_RELAYS && ms > 0) {
            relays[ch - 1].pulseDurationMs = (uint32_t)ms;
            luuKenh(ch - 1);
            guiPhanHoi("OK: Relay " + String(ch) + " xung " + String(ms) + "ms");
        } else {
            guiPhanHoi("ERR: PULSE <1..8> <ms>");
        }
    }
    else if (cmd.startsWith("MINOFF ")) {
        int ch = 0; long ms = -1;
        if (sscanf(cmd.c_str() + 7, "%d %ld", &ch, &ms) == 2 &&
            ch >= 1 && ch <= NUM_RELAYS && ms >= 0) {
            relays[ch - 1].minOffTimeMs = (uint32_t)ms;
            luuKenh(ch - 1);
            guiPhanHoi("OK: Relay " + String(ch) + " MinOff " + String(ms) + "ms");
        } else {
            guiPhanHoi("ERR: MINOFF <1..8> <ms>");
        }
    }
    else if (cmd.startsWith("MAXRUN ")) {
        int ch = 0; long ms = -1;
        if (sscanf(cmd.c_str() + 7, "%d %ld", &ch, &ms) == 2 &&
            ch >= 1 && ch <= NUM_RELAYS && ms >= 0) {
            relays[ch - 1].maxRunTimeMs = (uint32_t)ms;
            luuKenh(ch - 1);
            guiPhanHoi("OK: Relay " + String(ch) + " MaxRun " + String(ms) + "ms");
        } else {
            guiPhanHoi("ERR: MAXRUN <1..8> <ms> (0 = khong gioi han)");
        }
    }
    else if (cmd == "FACTORY") {
        xoaCauHinh();
        for (int i = 0; i < NUM_RELAYS; i++) {
            Output_RequestSet(&relays[i], false);
            // Giữ nguyên chân/activeLow, chỉ đưa các tham số về mặc định
            Output_Init(&relays[i], i + 1, RELAY_PINS[i], RELAY_ACTIVE_LOW);
        }
        guiPhanHoi("OK: da xoa cau hinh, ve mac dinh (tat ca MANUAL, lich 06:00-18:00)");
    }
    else if (cmd == "HELP") {
        inHuongDan();
    }
    else {
        guiPhanHoi("ERR: lenh khong hop le. Go HELP de xem danh sach lenh");
    }
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);

    for (int i = 0; i < NUM_RELAYS; i++) {
        Output_Init(&relays[i], i + 1, RELAY_PINS[i], RELAY_ACTIVE_LOW);
    }
    napCauHinh();   // Nạp lại mode + lịch + tham số đã lưu trong flash

    Serial.println("\n=== RELAY X8 SLAVE KHOI DONG === (go HELP de xem lenh)");
}

void loop() {
    // Nhận lệnh từ WROOM qua UART2
    while (Serial2.available() > 0) {
        char c = Serial2.read();
        if (c == '\n' || c == '\r') {
            if (uartBuffer.length() > 0) {
                xuLyLenh(uartBuffer);
                uartBuffer = "";
            }
        } else {
            uartBuffer += c;
        }
    }

    // Nhận lệnh trực tiếp từ cổng USB
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

    //Quét cập nhật cả 8 relay liên tục, dùng giờ + thứ nhận từ WROOM
    bool timeOk = gioConHieuLuc();
    for (int i = 0; i < NUM_RELAYS; i++) {
        Output_UpdateLoop(&relays[i], currentMinute, currentWeekday, timeOk);
    }
}