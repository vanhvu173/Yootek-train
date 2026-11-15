#include "digital_output.h" // Nhúng file header của Digital Output

// Hàm nội bộ (static): Trực tiếp xuất điện áp ra chân GPIO và cập nhật bộ đếm đóng cắt
static void Output_WriteHardware(DigitalOutput *out, bool newState) {
    // Nếu trạng thái mới giống hệt trạng thái hiện tại thì không cần xuất lại
    if (out->currentState == newState) return;

    out->currentState = newState;       // Cập nhật trạng thái thực tế mới
    out->lastSwitchMs = millis();       // Ghi nhận mốc thời gian vừa chuyển mạch

    // Nếu chuyển từ OFF sang ON -> Tăng số lần đóng cắt thêm 1
    if (newState == true) {
        out->switchCount++;
        out->lastSecTickMs = millis();  // Đặt lại mốc đếm giây chạy
    }

    // Xác định mức điện áp xuất ra chân GPIO dựa trên cấu hình kích thấp (activeLow) hay kích cao
    uint8_t pinLevel = (out->activeLow) ? (!newState) : (newState);
    digitalWrite(out->pin, pinLevel);   // Xuất tín hiệu ra chân vi điều khiển
}

// Hàm khởi tạo các thông số mặc định ban đầu cho đối tượng DigitalOutput
void Output_Init(DigitalOutput *out, uint8_t id, uint8_t pin, bool activeLow) {
    out->channelId = id;                // Gán số thứ tự kênh Relay (1..8)
    out->pin = pin;                     // Gán chân GPIO điều khiển
    out->activeLow = activeLow;         // Gán kiểu kích mức logic

    out->mode = OUT_MODE_MANUAL;        // Mặc định khởi động ở chế độ điều khiển tay (Manual)
    out->currentState = false;          // Trạng thái thực tế ban đầu là OFF
    out->requestState = false;          // Chưa có yêu cầu bật
    out->isInterlocked = false;         // Không bị khóa chéo an toàn

    out->minOffTimeMs = 5000;           // Mặc định bắt nghỉ tối thiểu 5 giây (5000ms) sau khi tắt mới cho bật lại
    out->minOnTimeMs = 1000;            // Mặc định khi đã bật thì giữ tối thiểu 1 giây (1000ms)
    out->maxRunTimeMs = 0;              // Mặc định 0 = Không giới hạn thời gian chạy tối đa
    out->lastSwitchMs = 0;              // Mốc thời gian chuyển mạch ban đầu = 0
    out->isTimeoutFault = false;        // Xóa cờ lỗi chạy quá giờ

    out->scheduleOnMin = 360;           // Giờ bật mặc định: 06h00 sáng (360 phút)
    out->scheduleOffMin = 1080;         // Giờ tắt mặc định: 18h00 chiều (1080 phút)
    out->pulseDurationMs = 2000;        // Độ rộng xung mặc định: Bật 2 giây (2000ms) rồi tự tắt

    out->switchCount = 0;               // Khởi tạo bộ đếm số lần đóng cắt = 0
    out->totalRunSeconds = 0;           // Khởi tạo tổng số giây chạy = 0
    out->lastSecTickMs = 0;             // Mốc đếm giây = 0

    pinMode(out->pin, OUTPUT);          // Cấu hình chân GPIO làm Output
    // Đảm bảo ngay khi mới cấp nguồn, chân GPIO xuất mức điện áp TẮT Relay an toàn
    uint8_t safeLevel = (out->activeLow) ? HIGH : LOW;
    digitalWrite(out->pin, safeLevel);
}

// Hàm nhận yêu cầu Bật/Tắt từ chương trình bên ngoài (chưa bật ngay mà chờ hàm UpdateLoop duyệt)
bool Output_RequestSet(DigitalOutput *out, bool wantState) {
    // Nếu đang ở chế độ Khóa cứng (DISABLE) hoặc đang bị Khóa an toàn (Interlock) thì từ chối lệnh BẬT
    if ((out->mode == OUT_MODE_DISABLE || out->isInterlocked) && wantState == true) {
        return false;                   // Trả về false báo hiệu từ chối yêu cầu
    }

    out->requestState = wantState;      // Lưu yêu cầu vào biến chờ duyệt

    // Nếu người dùng chủ động gửi lệnh TẮT (false), cho phép xóa luôn cờ lỗi quá thời gian (nếu có)
    if (wantState == false) {
        out->isTimeoutFault = false;
    }
    return true;                        // Trả về true báo hiệu đã tiếp nhận yêu cầu
}

// Hàm quét kiểm tra toàn bộ điều kiện bảo vệ, lịch trình và thực thi đóng cắt (Gọi liên tục trong loop)
void Output_UpdateLoop(DigitalOutput *out, uint16_t currentMinuteOfDay) {
    uint32_t now = millis();            // Lấy thời gian hệ thống hiện tại (ms)

    // 1. ƯU TIÊN AN TOÀN CAO NHẤT: Nếu bị Disable hoặc Interlock -> Ngắt điện ngay lập tức
    if (out->mode == OUT_MODE_DISABLE || out->isInterlocked) {
        out->requestState = false;
        Output_WriteHardware(out, false);
        return;                         // Thoát ngay, không xét các phần dưới
    }

    // 2. XỬ LÝ CHẾ ĐỘ HẸN GIỜ (SCHEDULE MODE)
    if (out->mode == OUT_MODE_SCHEDULE) {
        // Nếu thời gian hiện tại nằm trong khung giờ hẹn -> Tự động đặt yêu cầu BẬT
        if (currentMinuteOfDay >= out->scheduleOnMin && currentMinuteOfDay < out->scheduleOffMin) {
            out->requestState = true;
        } else {
            out->requestState = false;
        }
    }

    // 3. KIỂM TRA BẢO VỆ CHẠY QUÁ THỜI GIAN TỐI ĐA (Max Run Time Protection)
    if (out->currentState == true && out->maxRunTimeMs > 0) {
        if (now - out->lastSwitchMs >= out->maxRunTimeMs) {
            out->isTimeoutFault = true;       // Bật cờ báo lỗi quá thời gian chạy
            out->requestState = false;        // Hủy yêu cầu chạy
            Output_WriteHardware(out, false); // Cưỡng bức tắt Relay để bảo vệ thiết bị
            return;
        }
    }

    // 4. XỬ LÝ CHẾ ĐỘ TẠO XUNG (PULSE MODE)
    if (out->mode == OUT_MODE_PULSE && out->currentState == true) {
        // Khi đã giữ ON đủ khoảng thời gian pulseDurationMs -> Tự động tắt
        if (now - out->lastSwitchMs >= out->pulseDurationMs) {
            out->requestState = false;
            Output_WriteHardware(out, false);
            return;
        }
    }

    // 5. KIỂM TRA THỜI GIAN TRỄ BẢO VỆ (minOffTimeMs & minOnTimeMs) TRƯỚC KHI ĐÓNG/NGẮT THẬT
    // Trường hợp đang TẮT và có yêu cầu muốn BẬT:
    if (out->currentState == false && out->requestState == true && !out->isTimeoutFault) {
        // Chỉ cho bật nếu đây là lần bật đầu tiên (switchCount == 0) HOẶC đã nghỉ đủ thời gian minOffTimeMs
        if (out->switchCount == 0 || (now - out->lastSwitchMs >= out->minOffTimeMs)) {
            Output_WriteHardware(out, true);  // Đủ điều kiện -> Đóng Relay
        }
    }
    // Trường hợp đang BẬT và có yêu cầu muốn TẮT:
    else if (out->currentState == true && out->requestState == false) {
        // Chỉ cho tắt nếu đã chạy đủ thời gian giữ tối thiểu minOnTimeMs
        if (now - out->lastSwitchMs >= out->minOnTimeMs) {
            Output_WriteHardware(out, false); // Đủ điều kiện -> Ngắt Relay
        }
    }

    // 6. CẬP NHẬT THỐNG KÊ TỔNG THỜI GIAN HOẠT ĐỘNG (Tính bằng giây)
    if (out->currentState == true) {
        if (now - out->lastSecTickMs >= 1000) {
            out->totalRunSeconds++;           // Cộng thêm 1 giây vào tổng thời gian chạy
            out->lastSecTickMs += 1000;       // Dịch mốc thời gian thêm 1000ms
        }
    }
}