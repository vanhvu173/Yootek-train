#include "Digital_Input.h" 

// Hàm khởi tạo giá trị mặc định và cấu hình chân cho DigitalInput
void DigitalIn_Init(DigitalInput *in, uint8_t pin, uint8_t pullMode, bool invertLogic) {
    in->pin = pin;                      
    in->pullMode = pullMode;            
    in->invertLogic = invertLogic;      
    // Cài đặt các thông số thời gian mặc định
    in->debounceMs = 30;                
    in->onDelayMs = 0;
    in->offDelayMs = 0;
    in->holdTimeMs = 2000;              

    pinMode(in->pin, in->pullMode);
    // Đọc trạng thái thực tế ngay lúc khởi động để làm mốc ban đầu
    bool initVal = digitalRead(in->pin);
    if (in->invertLogic) {
        initVal = !initVal;             // Đảo ngược trạng thái nếu dùng nút nhấn nối GND
    }

    in->rawState = initVal;             // Gán trạng thái thô ban đầu
    in->lastRawState = initVal;         // Gán trạng thái thô cũ
    in->stableState = initVal;          // Gán trạng thái sạch ban đầu
    in->risingEdge = false;             // Xóa cờ sườn lên
    in->fallingEdge = false;            // Xóa cờ sườn xuống
    in->isLongPressed = false;          // Xóa nhấn giữ lâu
    in->pulseCounter = 0;               // Đặt bộ đếm xung về 0
    in->lastRawChangeMs = millis();     // Lấy mốc thời gian hiện tại
    in->stableOnStartMs = millis();     // Lấy mốc thời gian hiện tại
}

// Hàm quét cập nhật trạng thái ngõ vào số
void DigitalIn_UpdateLoop(DigitalInput *in) {
    uint32_t now = millis();

    in->risingEdge = false;
    in->fallingEdge = false;

    // Đọc tín hiệu điện vật lý tại chân GPIO
    bool readVal = digitalRead(in->pin);
    if (in->invertLogic) {
        readVal = !readVal;
    }
    in->rawState = readVal;

    // Nếu trạng thái thô thay đổi so với lần đọc trước
    if (in->rawState != in->lastRawState) {
        in->lastRawChangeMs = now;       
        in->lastRawState = in->rawState; 
    }
    // Tính khoảng thời gian mà tín hiệu đã giữ nguyên liên tục không đổi
    uint32_t stableDuration = now - in->lastRawChangeMs;

    if (in->rawState != in->stableState) {
        // Muốn chuyển từ OFF -> ON: Phải giữ ổn định vượt qua (debounceMs + onDelayMs)
        if (in->rawState == true && stableDuration >= (in->debounceMs + in->onDelayMs)) {
            in->stableState = true;
            in->risingEdge = true;
            in->stableOnStartMs = now;
            in->pulseCounter++;
        }
        //ON -> OFF
        else if (in->rawState == false && stableDuration >= (in->debounceMs + in->offDelayMs)) {
            in->stableState = false;
            in->fallingEdge = true;
            in->isLongPressed = false;
        }
    }

    // Kiểm tra điều kiện Long Press khi đang ở trạng thái ON
    if (in->stableState == true && !in->isLongPressed) {
        if (now - in->stableOnStartMs >= in->holdTimeMs) {
            in->isLongPressed = true;  
        }
    }
}

// Hàm xóa bộ đếm xung về 0 khi cần
void DigitalIn_ResetCounter(DigitalInput *in) {
    in->pulseCounter = 0;                
}