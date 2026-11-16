#ifndef DIGITAL_INPUT_H
#define DIGITAL_INPUT_H

#include <Arduino.h>

// Cấu trúc dữ liệu của Đối tượng Ngõ vào Số (Digital Input)
struct DigitalInput {
    // --- 1. Cấu hình phần cứng ---
    uint8_t pin;                // Chân GPIO nối với nút nhấn / công tắc / cảm biến số
    uint8_t pullMode;           // Chế độ điện trở kéo: INPUT, INPUT_PULLUP, INPUT_PULLDOWN
    bool invertLogic;           // Đảo mức logic

    // Biến trạng thái & Bắt xung
    bool rawState;              // Trạng thái vật lý thô đọc trực tiếp từ chân GPIO (chưa lọc)
    bool lastRawState;          // Trạng thái thô ở vòng lặp trước (dùng để phát hiện sự thay đổi)
    bool stableState;           // Trạng thái sạch sau khi đã qua bộ lọc chống nhiễu (dùng để điều khiển)
    bool risingEdge;            // Cờ báo lên (vừa chuyển từ OFF -> ON), chỉ bật true trong đúng 1 chu kỳ
    bool fallingEdge;           // Cờ báo sườn xuống (vừa chuyển từ ON -> OFF), chỉ bật true trong đúng 1 chu kỳ

    // Tham số Lọc nhiễu (Debounce) & Trễ xác nhận (Delay)
    uint32_t debounceMs;        // Thời gian chống rung tiếp điểm cơ khí
    uint32_t onDelayMs;         // Thời gian trễ xác nhận BẬT
    uint32_t offDelayMs;        // Thời gian trễ xác nhận TẮT
    uint32_t lastRawChangeMs;   // Mốc thời gian lần cuối cùng tín hiệu thô bị thay đổi

    // Tính năng nâng cao (Nhấn giữ & Bộ đếm)
    uint32_t holdTimeMs;        // Ngưỡng thời gian để xác nhận là Long Press
    uint32_t stableOnStartMs;   // Mốc thời gian bắt đầu bước vào trạng thái ON ổn định
    bool isLongPressed;         // Cờ báo hiệu nút đang được nhấn giữ vượt quá ngưỡng holdTimeMs
    uint32_t pulseCounter;      // Bộ đếm tổng số lần kích hoạt
};

// Khai báo các hàm tương tác với đối tượng DigitalInput
void DigitalIn_Init(DigitalInput *in, uint8_t pin, uint8_t pullMode, bool invertLogic);
void DigitalIn_UpdateLoop(DigitalInput *in);
void DigitalIn_ResetCounter(DigitalInput *in);

#endif