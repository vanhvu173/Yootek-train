#ifndef I2C_PAYLOAD_H
#define I2C_PAYLOAD_H

#include <Arduino.h>

// Gói tin dùng chung cho cả Master và Slave

// --- Mã lệnh ---
#define I2C_CMD_SET_TEMP    0x10    // Master -> Slave: đặt nhiệt độ
#define I2C_CMD_REPORT      0x20    // Slave -> Master: báo cáo trạng thái

// --- Mã lỗi ---
#define I2C_ERR_NONE        0
#define I2C_ERR_BAD_CMD     1       // Slave không biết lệnh này
#define I2C_ERR_BAD_LEN     2       // Độ dài gói nhận sai

#pragma pack(push, 1)
typedef struct {
    uint8_t command;
    float   temperature;
    uint8_t errorCode;
} I2cPayload_t;
#pragma pack(pop)

#endif