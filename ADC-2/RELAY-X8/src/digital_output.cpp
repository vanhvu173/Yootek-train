#include "digital_output.h"

// Hàm nội bộ (static): Trực tiếp xuất điện áp ra chân GPIO và cập nhật bộ đếm đóng cắt
static void Output_WriteHardware(DigitalOutput *out, bool newState) {
    if (out->currentState == newState) return;

    out->currentState = newState;       
    out->lastSwitchMs = millis();       

    if (newState == true) {
        out->switchCount++;             
        out->lastSecTickMs = millis();  
    }

    uint8_t pinLevel = (out->activeLow) ? (!newState) : (newState);
    digitalWrite(out->pin, pinLevel);   // Xuất tín hiệu ra chân vi điều khiển
}

// Ngày weekday (1..7) có nằm trong dayMask không. weekday = 0 (chưa biết) -> coi như có.
static bool daySelected(uint8_t mask, uint8_t weekday) {
    if (weekday < 1 || weekday > 7) return true;
    return (mask & (1 << (weekday - 1))) != 0;
}

// Ngày hôm trước của weekday (T2 -> CN). weekday = 0 giữ nguyên 0.
static uint8_t prevDay(uint8_t weekday) {
    if (weekday < 1 || weekday > 7) return 0;
    return (weekday == 1) ? 7 : (weekday - 1);
}

// Khung giờ này có đang yêu cầu ON tại thời điểm hiện tại không
static bool slotActive(const ScheduleSlot *s, uint16_t m, uint8_t weekday) {
    if (!s->enabled || s->onMin == s->offMin) return false;

    if (s->onMin < s->offMin) {
        // Khung trong ngày (VD 06:00 -> 18:00)
        return daySelected(s->dayMask, weekday) && m >= s->onMin && m < s->offMin;
    }
    // Khung qua nửa đêm (VD 22:00 -> 06:00)
    if (m >= s->onMin) return daySelected(s->dayMask, weekday);
    if (m < s->offMin) return daySelected(s->dayMask, prevDay(weekday));
    return false;
}

// Hàm khởi tạo các thông số mặc định ban đầu cho đối tượng DigitalOutput
void Output_Init(DigitalOutput *out, uint8_t id, uint8_t pin, bool activeLow) {
    out->channelId = id;
    out->pin = pin;
    out->activeLow = activeLow;

    out->mode = OUT_MODE_MANUAL;        // Mặc định khởi động ở chế độ điều khiển tay
    out->currentState = false;
    out->requestState = false;
    out->isInterlocked = false;

    out->minOffTimeMs = 5000;           // Nghỉ tối thiểu 5 giây giữa 2 lần bật
    out->minOnTimeMs = 1000;            // Giữ ON tối thiểu 1 giây
    out->maxRunTimeMs = 0;              // 0 = Không giới hạn thời gian chạy tối đa
    out->lastSwitchMs = 0;
    out->isTimeoutFault = false;

    // Khung 1 mặc định: 06:00 -> 18:00 mọi ngày. Các khung còn lại để trống (tắt).
    for (int i = 0; i < MAX_SCHEDULE_SLOTS; i++) {
        out->slots[i].enabled = (i == 0);
        out->slots[i].onMin = (i == 0) ? 360 : 0;
        out->slots[i].offMin = (i == 0) ? 1080 : 0;
        out->slots[i].dayMask = DAYS_ALL;
    }
    out->pulseDurationMs = 2000;        // Độ rộng xung mặc định 2 giây

    out->switchCount = 0;
    out->totalRunSeconds = 0;
    out->lastSecTickMs = 0;

    // Ghi mức TẮT trước rồi mới đặt chế độ OUTPUT để tránh xung nhiễu lúc khởi động
    uint8_t safeLevel = (out->activeLow) ? HIGH : LOW;
    digitalWrite(out->pin, safeLevel);
    pinMode(out->pin, OUTPUT);
    digitalWrite(out->pin, safeLevel);
}

// Đổi chế độ vận hành. Luôn xóa yêu cầu cũ và cờ lỗi để tránh relay "bật bất ngờ" sau khi đổi mode
void Output_SetMode(DigitalOutput *out, OutputMode mode) {
    out->mode = mode;
    out->requestState = false;
    out->isTimeoutFault = false;
}

// Hàm nhận yêu cầu Bật/Tắt từ chương trình bên ngoài
bool Output_RequestSet(DigitalOutput *out, bool wantState) {
    if ((out->mode == OUT_MODE_DISABLE || out->isInterlocked) && wantState == true) {
        return false;                   // Từ chối lệnh BẬT khi đang khóa
    }

    out->requestState = wantState;

    // Lệnh OFF chủ động cho phép xóa cờ lỗi quá thời gian
    if (wantState == false) {
        out->isTimeoutFault = false;
    }
    return true;
}

// Quét kiểm tra điều kiện bảo vệ, lịch trình và thực thi đóng cắt
void Output_UpdateLoop(DigitalOutput *out, uint16_t currentMinuteOfDay, uint8_t weekday, bool timeValid) {
    uint32_t now = millis();

    // ƯU TIÊN AN TOÀN CAO NHẤT: Disable hoặc Interlock -> Ngắt điện ngay lập tức
    if (out->mode == OUT_MODE_DISABLE || out->isInterlocked) {
        out->requestState = false;
        Output_WriteHardware(out, false);
        return;
    }

    // CHẾ ĐỘ HẸN GIỜ: BẬT nếu có ít nhất một khung giờ đang kích hoạt
    if (out->mode == OUT_MODE_SCHEDULE) {
        bool want = false;
        if (timeValid) {
            for (int i = 0; i < MAX_SCHEDULE_SLOTS; i++) {
                if (slotActive(&out->slots[i], currentMinuteOfDay, weekday)) { want = true; break; }
            }
        }
        // timeValid == false -> want = false -> giữ relay TẮT cho an toàn
        out->requestState = want;
    }

    //BẢO VỆ CHẠY QUÁ THỜI GIAN TỐI ĐA
    if (out->currentState == true && out->maxRunTimeMs > 0) {
        if (now - out->lastSwitchMs >= out->maxRunTimeMs) {
            out->isTimeoutFault = true;
            out->requestState = false;
            Output_WriteHardware(out, false);
            return;
        }
    }

    // CHẾ ĐỘ TẠO XUNG
    if (out->mode == OUT_MODE_PULSE && out->currentState == true) {
        if (now - out->lastSwitchMs >= out->pulseDurationMs) {
            out->requestState = false;
            Output_WriteHardware(out, false);
            return;
        }
    }

    // THỜI GIAN TRỄ BẢO VỆ (minOffTimeMs & minOnTimeMs)
    if (out->currentState == false && out->requestState == true && !out->isTimeoutFault) {
        if (out->switchCount == 0 || (now - out->lastSwitchMs >= out->minOffTimeMs)) {
            Output_WriteHardware(out, true);
        }
    }
    else if (out->currentState == true && out->requestState == false) {
        if (now - out->lastSwitchMs >= out->minOnTimeMs) {
            Output_WriteHardware(out, false);
        }
    }

    // THỐNG KÊ TỔNG THỜI GIAN HOẠT ĐỘNG
    if (out->currentState == true) {
        if (now - out->lastSecTickMs >= 1000) {
            out->totalRunSeconds++;
            out->lastSecTickMs += 1000;
        }
    }
}