#ifndef DIGITAL_OUTPUT_H
#define DIGITAL_OUTPUT_H

#include <Arduino.h>

#define MAX_SCHEDULE_SLOTS 4     // Số khung giờ tối đa cho mỗi relay
#define DAYS_ALL 0x7F            // Bit0 = Thứ 2 ... Bit5 = Thứ 7, Bit6 = Chủ nhật

// Các chế độ vận hành của Đối tượng Ngõ ra Số (Relay / Van / Động cơ)
enum OutputMode {
    OUT_MODE_DISABLE = 0, // Chế độ Khóa cứng: Cấm bật dưới mọi hình thức (dùng khi bảo trì)
    OUT_MODE_MANUAL,      // Chế độ Bằng tay: Điều khiển trực tiếp qua lệnh ON / OFF
    OUT_MODE_SCHEDULE,    // Chế độ Lịch trình: Tự động bật/tắt theo các khung giờ cài đặt
    OUT_MODE_PULSE        // Chế độ Tạo xung: Khi bật chỉ giữ ON một khoảng thời gian rồi tự tắt
};

// Một khung giờ hẹn.
struct ScheduleSlot {
    bool     enabled;    // true: khung đang được dùng
    uint16_t onMin;      // Giờ bật, tính bằng phút từ 00:00 (VD 06:00 = 360)
    uint16_t offMin;     // Giờ tắt, tính bằng phút từ 00:00 (VD 18:00 = 1080)
    uint8_t  dayMask;    // Ngày áp dụng: bit0 = T2, bit1 = T3, ... bit5 = T7, bit6 = CN
};

// Cấu trúc dữ liệu của Đối tượng Ngõ ra Số (Digital Output)
struct DigitalOutput {
    //Cấu hình phần cứng
    uint8_t channelId;          // Số thứ tự kênh (VD: Relay 1 đến Relay 8)
    uint8_t pin;                // Chân GPIO điều khiển trực tiếp kênh Relay này
    bool activeLow;             // true: kích mức thấp (0V=ON), false: kích mức cao (3.3V=ON)

    //Trạng thái & Chế độ vận hành
    OutputMode mode;            // Chế độ hoạt động hiện tại (Disable / Manual / Schedule / Pulse)
    bool currentState;          // Trạng thái thực tế đang xuất ra chân phần cứng (true=ON, false=OFF)
    bool requestState;          // Trạng thái mong muốn
    bool isInterlocked;         // Cờ khóa chéo an toàn 

    //Tham số Bảo vệ đóng ngắt & Thời gian trễ (Delay Time)
    uint32_t minOffTimeMs;      // Thời gian nghỉ tối thiểu giữa 2 lần bật
    uint32_t minOnTimeMs;       // Thời gian phải giữ ON tối thiểu sau khi đã bật
    uint32_t maxRunTimeMs;      // Thời gian chạy liên tục tối đa
    uint32_t lastSwitchMs;      // Mốc thời gian của lần chuyển đổi trạng thái ON/OFF gần nhất
    bool isTimeoutFault;        // Cờ báo lỗi: Thiết bị vừa bị ngắt cưỡng bức do chạy quá maxRunTimeMs

    //Tham số Hẹn giờ (Schedule) & Tạo xung (Pulse)
    ScheduleSlot slots[MAX_SCHEDULE_SLOTS]; // Các khung giờ hẹn
    uint32_t pulseDurationMs;   // Thời gian giữ ON trước khi tự tắt khi chạy ở chế độ OUT_MODE_PULSE

    //Thống kê vận hành & Bảo trì (Maintenance)
    uint32_t switchCount;       // Đếm tổng số lần Relay đã đóng tiếp điểm
    uint32_t totalRunSeconds;   // Đếm tổng thời gian thiết bị đã hoạt động
    uint32_t lastSecTickMs;     // Biến nội bộ lưu mốc millis() để đếm từng giây hoạt động
};

void Output_Init(DigitalOutput *out, uint8_t id, uint8_t pin, bool activeLow);
bool Output_RequestSet(DigitalOutput *out, bool wantState);
void Output_SetMode(DigitalOutput *out, OutputMode mode);
void Output_UpdateLoop(DigitalOutput *out, uint16_t currentMinuteOfDay, uint8_t weekday, bool timeValid);

#endif