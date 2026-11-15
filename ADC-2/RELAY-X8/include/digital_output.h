#ifndef DIGITAL_OUTPUT_H
#define DIGITAL_OUTPUT_H

#include <Arduino.h>

// Các chế độ vận hành của Đối tượng Ngõ ra Số (Relay / Van / Động cơ)
enum OutputMode {
    OUT_MODE_DISABLE = 0, // Chế độ Khóa cứng: Cấm bật dưới mọi hình thức (dùng khi bảo trì)
    OUT_MODE_MANUAL,      // Chế độ Bằng tay: Điều khiển trực tiếp qua lệnh ON / OFF
    OUT_MODE_SCHEDULE,    // Chế độ Lịch trình: Tự động bật/tắt theo khung giờ cài đặt trong ngày
    OUT_MODE_PULSE        // Chế độ Tạo xung: Khi bật chỉ giữ ON một khoảng thời gian rồi tự tắt
};

// Cấu trúc dữ liệu của Đối tượng Ngõ ra Số (Digital Output)
struct DigitalOutput {
    // --- 1. Cấu hình phần cứng ---
    uint8_t channelId;          // Số thứ tự kênh (VD: Relay 1 đến Relay 8)
    uint8_t pin;                // Chân GPIO điều khiển trực tiếp kênh Relay này
    bool activeLow;             // true: kích mức thấp (0V=ON), false: kích mức cao (3.3V=ON)

    // --- 2. Trạng thái & Chế độ vận hành ---
    OutputMode mode;            // Chế độ hoạt động hiện tại (Disable / Manual / Schedule / Pulse)
    bool currentState;          // Trạng thái thực tế ĐANG XUẤT RA chân phần cứng (true=ON, false=OFF)
    bool requestState;          // Trạng thái MONG MUỐN (đang chờ xét duyệt qua điều kiện thời gian trễ)
    bool isInterlocked;         // Cờ khóa chéo an toàn (true = đang bị chặn bởi điều kiện an toàn khác)

    // --- 3. Tham số Bảo vệ đóng ngắt & Thời gian trễ (Delay Time) ---
    uint32_t minOffTimeMs;      // Thời gian nghỉ tối thiểu giữa 2 lần bật (ms) - chống bật tắt liên tục gây cháy lốc/bơm
    uint32_t minOnTimeMs;       // Thời gian phải giữ ON tối thiểu sau khi đã bật (ms)
    uint32_t maxRunTimeMs;      // Thời gian chạy liên tục tối đa (0 = không giới hạn, >0 = chạy quá giờ tự ngắt bảo vệ)
    uint32_t lastSwitchMs;      // Mốc thời gian (millis) của lần chuyển đổi trạng thái ON/OFF gần nhất
    bool isTimeoutFault;        // Cờ báo lỗi: Thiết bị vừa bị ngắt cưỡng bức do chạy quá maxRunTimeMs

    // --- 4. Tham số Hẹn giờ (Schedule) & Tạo xung (Pulse) ---
    uint16_t scheduleOnMin;     // Thời điểm bật trong ngày tính bằng phút (VD: 06h00 = 360)
    uint16_t scheduleOffMin;    // Thời điểm tắt trong ngày tính bằng phút (VD: 18h00 = 1080)
    uint32_t pulseDurationMs;   // Thời gian giữ ON (ms) trước khi tự tắt khi chạy ở chế độ OUT_MODE_PULSE

    // --- 5. Thống kê vận hành & Bảo trì (Maintenance) ---
    uint32_t switchCount;       // Đếm tổng số lần Relay đã đóng tiếp điểm (theo dõi tuổi thọ cơ khí)
    uint32_t totalRunSeconds;   // Đếm tổng thời gian thiết bị đã hoạt động (tính bằng giây)
    uint32_t lastSecTickMs;     // Biến nội bộ lưu mốc millis() để đếm từng giây hoạt động
};

// Khai báo các hàm tương tác với đối tượng DigitalOutput
void Output_Init(DigitalOutput *out, uint8_t id, uint8_t pin, bool activeLow);
bool Output_RequestSet(DigitalOutput *out, bool wantState);
void Output_UpdateLoop(DigitalOutput *out, uint16_t currentMinuteOfDay);

#endif