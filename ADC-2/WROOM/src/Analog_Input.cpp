#include "Analog_Input.h"
#include <math.h>

// Hàm khởi tạo các thông số mặc định cho đối tượng AnalogInput
void AnalogIn_Init(AnalogInput *ain, uint8_t pin, float engMin, float engMax) {
    ain->pin = pin;                     // Gán chân đọc ADC
    ain->sampleIntervalMs = 100;        // Mặc định 100ms đọc ADC 1 lần
    ain->lastSampleMs = 0;              // Mốc thời gian ban đầu

    ain->rawAdc = 0;                    // Khởi tạo ADC thô
    ain->filteredAdc = 0.0f;            // Khởi tạo ADC sau lọc
    ain->filterAlpha = 0.2f;            // Hệ số lọc EMA mặc định 
    ain->deadband = 0.1f;               // Vùng chết mặc định
    ain->isFirstSample = true;          // Đánh dấu chưa đọc mẫu nào

    ain->adcMin = 0;                    // Mức ADC thấp nhất mặc định
    ain->adcMax = 4095;                 // Mức ADC cao nhất của ESP32
    ain->engMin = engMin;               // Gán giới hạn đo vật lý dưới
    ain->engMax = engMax;               // Gán giới hạn đo vật lý trên
    ain->calibScale = 1.0f;             // Hệ số nhân hiệu chuẩn mặc định
    ain->calibOffset = 0.0f;            // Độ lệch bù trừ mặc định
    ain->engineeringValue = engMin;     // Kết quả đo ban đầu

    // Các ngưỡng cảnh báo mặc định theo phần trăm dải đo
    ain->highCritThreshold = 80.0f;     // Báo động đỏ trên 
    ain->lowCritThreshold = 5.0f;       // Báo động đỏ dưới 
    ain->highWarnThreshold = 65.0f;     // Cảnh báo vàng trên 
    ain->lowWarnThreshold = 15.0f;      // Cảnh báo vàng dưới 
    ain->hysteresis = 2.0f;             // Độ trễ chống dao động ngưỡng
    ain->alarmStatus = ALARM_NORMAL;    // Trạng thái hiện tại: Bình thường
    ain->lastAlarmStatus = ALARM_NORMAL;// Trạng thái trước đó: Bình thường

    ain->faultAdcUnder = 0;             // Ngưỡng dưới bắt lỗi đứt dây
    ain->faultAdcOver = 4095;           // Ngưỡng trên bắt lỗi chạm chập
    ain->isSensorFault = false;         // Mặc định không có lỗi phần cứng

    pinMode(ain->pin, INPUT);           // Cấu hình chân GPIO làm ngõ vào Analog
}

// Hàm quét đọc ADC, lọc nhiễu, tính ra đơn vị thực và xét cảnh báo
void AnalogIn_UpdateLoop(AnalogInput *ain) {
    uint32_t now = millis();

    // Kiểm tra chu kỳ lấy mẫu
    if (now - ain->lastSampleMs < ain->sampleIntervalMs) return;
    ain->lastSampleMs = now;
    ain->lastAlarmStatus = ain->alarmStatus;
    ain->rawAdc = analogRead(ain->pin);

    //Kiểm tra lỗi phần cứng cảm biến
    if (ain->rawAdc < ain->faultAdcUnder || ain->rawAdc > ain->faultAdcOver) {
        ain->isSensorFault = true;
        ain->alarmStatus = ALARM_SENSOR_FAULT;
        return;
    }
    ain->isSensorFault = false;

    //Lọc nhiễu bằng thuật toán EMA
    if (ain->isFirstSample) {
        ain->filteredAdc = (float)ain->rawAdc;
        ain->isFirstSample = false;
    } else {
        // Công thức EMA
        ain->filteredAdc = (ain->filterAlpha * (float)ain->rawAdc) + ((1.0f - ain->filterAlpha) * ain->filteredAdc);
    }

    // Quy đổi tuyến tính (Scaling) từ giá trị ADC sang đơn vị đo thực tế
    if (ain->adcMax <= ain->adcMin) return;     // Bảo vệ chống lỗi chia cho 0
    float ratio = (ain->filteredAdc - (float)ain->adcMin) / (float)(ain->adcMax - ain->adcMin);
    float rawEng = ain->engMin + ratio * (ain->engMax - ain->engMin);

    // Áp dụng công thức Hiệu chuẩn (Calibration): Kết_quả = (Giá_trị_thô * Scale) + Offset
    float calibratedEng = (rawEng * ain->calibScale) + ain->calibOffset;

    // Kiểm tra vùng chết (Deadband): Chỉ cập nhật khi độ thay đổi lớn hơn hoặc bằng deadband
    if (fabs(calibratedEng - ain->engineeringValue) >= ain->deadband) {
        ain->engineeringValue = calibratedEng;
    }

    // Xét duyệt các mức Cảnh báo áp dụng đỗ trễ
    float val = ain->engineeringValue;
    float hys = ain->hysteresis;

    if (val >= ain->highCritThreshold) {
        ain->alarmStatus = ALARM_HIGH_CRIT;
    } else if (val <= ain->lowCritThreshold) {
        ain->alarmStatus = ALARM_LOW_CRIT;
    } else if (val >= ain->highWarnThreshold) {

        if (ain->alarmStatus == ALARM_HIGH_CRIT && val > (ain->highCritThreshold - hys)) {
            ain->alarmStatus = ALARM_HIGH_CRIT;
        } else {
            ain->alarmStatus = ALARM_HIGH_WARN;
        }
    } else if (val <= ain->lowWarnThreshold) {

        if (ain->alarmStatus == ALARM_LOW_CRIT && val < (ain->lowCritThreshold + hys)) {
            ain->alarmStatus = ALARM_LOW_CRIT;
        } else {
            ain->alarmStatus = ALARM_LOW_WARN;
        }
    } else {

        if (ain->alarmStatus == ALARM_HIGH_WARN && val > (ain->highWarnThreshold - hys)) {
            ain->alarmStatus = ALARM_HIGH_WARN;
        } else if (ain->alarmStatus == ALARM_LOW_WARN && val < (ain->lowWarnThreshold + hys)) {
            ain->alarmStatus = ALARM_LOW_WARN;
        } else {
            ain->alarmStatus = ALARM_NORMAL;
        }
    }
}

// Hàm hỗ trợ cài đặt nhanh thông số hiệu chuẩn sai số cảm biến
void AnalogIn_SetCalibration(AnalogInput *ain, float scale, float offset) {
    ain->calibScale = scale;
    ain->calibOffset = offset;
}