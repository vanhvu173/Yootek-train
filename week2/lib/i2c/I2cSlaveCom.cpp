#include "I2cSlaveCom.h"
#include "PinConfig.h"
#include <Wire.h>

// ===================== Kiểu dữ liệu nội bộ =====================

typedef void (*I2cCmdCb)(const I2cPayload_t* payload);

// Bảng lệnh: mỗi mã lệnh từ Master ứng với 1 callback
typedef struct {
    uint8_t  command;
    I2cCmdCb onReceive;
} I2cCmdEntry_t;

// Cấu hình I2C Slave
typedef struct {
    uint8_t  addr;
    uint8_t  sdaPin;
    uint8_t  sclPin;
    uint32_t freqHz;
} I2cSlaveCfg_t;

// Trạng thái trung gian
typedef struct {
    volatile bool rxPending; // có gói mới cần log
    I2cPayload_t  rx;        // gói vừa nhận từ Master
    I2cPayload_t  tx;        // gói trả lời khi Master đọc
    float         setpoint;  // nhiệt độ Master đã đặt
} I2cSlaveState_t;

typedef struct {
    I2cSlaveCfg_t   cfg;
    I2cSlaveState_t st;
} I2cSlaveCtx_t;

static I2cSlaveCtx_t ctx = {
    { I2C_SLAVE_ADDR, I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ },
    { false, { 0, 0, 0 }, { I2C_CMD_REPORT, 0, I2C_ERR_NONE }, 0 }
};

// ===================== Callback xử lý lệnh =====================
// Chạy trong ngữ cảnh ngắt/Wire nên chỉ cập nhật biến, không in log

static void OnSetTemp(const I2cPayload_t* p) {
    ctx.st.setpoint     = p->temperature;
    ctx.st.tx.errorCode = I2C_ERR_NONE;
}

static const I2cCmdEntry_t kCmdTable[] = {
    { I2C_CMD_SET_TEMP, OnSetTemp },
};

static void DispatchCommand(const I2cPayload_t* p) {
    for (const auto& e : kCmdTable) {
        if (e.command != p->command) continue;
        e.onReceive(p);
        return;
    }
    ctx.st.tx.errorCode = I2C_ERR_BAD_CMD;
}

// Cập nhật gói trả lời: Slave phản hồi lại nhiệt độ đã nhận để Master kiểm chứng
static void RefreshReply() {
    ctx.st.tx.command     = I2C_CMD_REPORT;
    ctx.st.tx.temperature = ctx.st.setpoint;
}

// ===================== Callback của Wire =====================

static void DrainWire() {
    while (Wire.available()) Wire.read();
}

static void OnWireReceive(int len) {
    if (len != (int)sizeof(I2cPayload_t)) {
        DrainWire();
        ctx.st.tx.errorCode = I2C_ERR_BAD_LEN;
        return;
    }
    Wire.readBytes((uint8_t*)&ctx.st.rx, sizeof(I2cPayload_t));
    DispatchCommand(&ctx.st.rx);
    RefreshReply();
    ctx.st.rxPending = true;
}

static void OnWireRequest() {
    Wire.write((const uint8_t*)&ctx.st.tx, sizeof(I2cPayload_t));
}

// ===================== API công khai =====================

void I2cSlave_Init() {
    // Đăng ký callback TRƯỚC khi begin()
    Wire.onReceive(OnWireReceive);
    Wire.onRequest(OnWireRequest);
    Wire.begin((uint8_t)ctx.cfg.addr, (int)ctx.cfg.sdaPin,
               (int)ctx.cfg.sclPin, (uint32_t)ctx.cfg.freqHz);
}

static void LogReceived() {
    Serial.printf("[I2C-S] Nhan CMD: 0x%02X | Temp: %.2f | Err: %d\n",
                  ctx.st.rx.command, ctx.st.rx.temperature, ctx.st.tx.errorCode);
}

void I2cSlave_Task() {
    if (!ctx.st.rxPending) return;
    ctx.st.rxPending = false;
    LogReceived();
}