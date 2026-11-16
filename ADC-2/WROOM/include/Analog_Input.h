#ifndef ANALOG_INPUT_H
#define ANALOG_INPUT_H

#include <Arduino.h>

// Các mức trạng thái cảnh báo của kênh Analog
enum AlarmState {
    ALARM_NORMAL = 0,     // Giá trị đo nằm trong vùng an toàn bình thường
    ALARM_LOW_WARN,       // Cảnh báo sớm mức thấp (Vàng)
    ALARM_LOW_CRIT,       // Báo động nguy hiểm mức cực thấp (Đỏ)
    ALARM_HIGH_WARN,      // Cảnh báo sớm mức cao (Vàng)
    ALARM_HIGH_CRIT,      // Báo động nguy hiểm mức cực cao (Đỏ - VD: Quá nhiệt)
    ALARM_SENSOR_FAULT    // Lỗi phần cứng cảm biến (Đứt dây hoặc chập dây)
};

struct AnalogInput {
    // Cấu hình phần cứng & Chu kỳ lấy mẫu
    uint8_t pin;                // Chân GPIO đọc ADC
    uint32_t sampleIntervalMs;  // Thời gian giữa 2 lần đọc ADC liên tiếp
    uint32_t lastSampleMs;      // Mốc thời gian của lần lấy mẫu gần nhất

    // Giá trị ADC & Tham số Lọc nhiễu (Filtering)
    uint16_t rawAdc;            // Giá trị ADC thô đọc trực tiếp từ phần cứng
    float filteredAdc;          // Giá trị ADC sau khi đã làm mượt bằng bộ lọc EMA
    float filterAlpha;          // Hệ số lọc EMA (0.01 -> 1.0)
    float deadband;             // Vùng chết: Nếu kết quả thay đổi nhỏ hơn thì giữ nguyên số cũ
    bool isFirstSample;         // Cờ đánh dấu lần đọc đầu tiên khi mới cấp nguồn

    // Quy đổi đơn vị thực tế (Scaling) & Hiệu chuẩn (Calibration)
    uint16_t adcMin;            // Giá trị ADC tương ứng với mức thấp nhất của cảm biến
    uint16_t adcMax;            // Giá trị ADC tương ứng với mức cao nhất của cảm biến
    float engMin;               // Giá trị vật lý thấp nhất
    float engMax;               // Giá trị vật lý cao nhất
    float calibScale;           // Hệ số nhân hiệu chuẩn độ nghiêng
    float calibOffset;          // Độ lệch cộng bù hiệu chuẩn sai số tĩnh
    float engineeringValue;     // GIÁ TRỊ ĐO CUỐI CÙNG

    // Cảnh báo ngưỡng (Alarms) & Độ trễ chống dao động (Hysteresis)
    float highCritThreshold;    // Ngưỡng báo động nguy hiểm mức CAO
    float highWarnThreshold;    // Ngưỡng cảnh báo mức CAO
    float lowWarnThreshold;     // Ngưỡng cảnh báo mức THẤP
    float lowCritThreshold;     // Ngưỡng báo động nguy hiểm mức THẤP
    float hysteresis;           // Độ trễ ngưỡng
    AlarmState alarmStatus;     // Trạng thái cảnh báo hiện tại
    AlarmState lastAlarmStatus; // Trạng thái cảnh báo ở chu kỳ trước (dùng để bắt sự kiện vừa đổi mức cảnh báo)

    // Phát hiện lỗi phần cứng cảm biến (Fault Detection)
    uint16_t faultAdcUnder;     // Ngưỡng báo đứt dây (Nếu rawAdc < -> lỗi cảm biến)
    uint16_t faultAdcOver;      // Ngưỡng báo chập dây (Nếu rawAdc > -> lỗi cảm biến)
    bool isSensorFault;         // Cờ báo hiệu cảm biến đang bị hỏng phần cứng
};

// Khai báo các hàm tương tác với đối tượng AnalogInput
void AnalogIn_Init(AnalogInput *ain, uint8_t pin, float engMin, float engMax);
void AnalogIn_UpdateLoop(AnalogInput *ain);
void AnalogIn_SetCalibration(AnalogInput *ain, float scale, float offset);

#endif