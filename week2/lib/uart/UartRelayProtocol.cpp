#include "UartRelayProtocol.h"
#include "PinConfig.h"
#include "SoftTimer.h"

// ===================== Kiểu dữ liệu nội bộ =====================

typedef void (*RelayCmdCb)(const RelayFrame_t* frame);

// Bảng lệnh: mỗi mã lệnh phản hồi từ Relay-X8 ứng với 1 callback
typedef struct {
    uint8_t    command;
    RelayCmdCb onReceive;
} RelayCmdEntry_t;

// Cấu hình UART
typedef struct {
    uint8_t  rxPin;
    uint8_t  txPin;
    uint32_t baud;
} UartRelayCfg_t;

// Trạng thái nhận (State Machine non-blocking)
typedef struct {
    uint8_t buf[sizeof(RelayFrame_t)];
    uint8_t idx;
} UartRelayRx_t;

// Trạng thái gửi
typedef struct {
    SoftTimer_t timer;
    uint8_t     mask;
} UartRelayTx_t;

typedef struct {
    UartRelayCfg_t cfg;
    UartRelayRx_t  rx;
    UartRelayTx_t  tx;
} UartRelayCtx_t;

static UartRelayCtx_t ctx = {
    { UART_RELAY_RX, UART_RELAY_TX, UART_RELAY_BAUD },
    { { 0 }, 0 },
    { { UART_RELAY_PERIOD_MS, 0 }, 0x01 }
};

// ===================== Callback xử lý lệnh =====================

static void OnSetAck(const RelayFrame_t* f) {
    Serial.printf("[UART] SET ack | Mask: 0x%02X\n", f->relayMask);
}

static void OnStatus(const RelayFrame_t* f) {
    Serial.printf("[UART] STATUS | Mask: 0x%02X\n", f->relayMask);
}

static const RelayCmdEntry_t kCmdTable[] = {
    { RELAY_CMD_SET,    OnSetAck },
    { RELAY_CMD_STATUS, OnStatus },
};

// ===================== Khung truyền =====================

static uint8_t CalculateChecksum(const RelayFrame_t* f) {
    return f->header ^ f->command ^ f->relayMask;
}

static bool IsFrameValid(const RelayFrame_t* f) {
    return f->checksum == CalculateChecksum(f);
}

static void DispatchFrame(const RelayFrame_t* f) {
    for (const auto& e : kCmdTable) {
        if (e.command != f->command) continue;
        e.onReceive(f);
        return;
    }
    Serial.printf("[UART] Lenh la: 0x%02X\n", f->command);
}

// Trả về true khi gom đủ 1 frame
static bool PushByte(uint8_t b) {
    if (ctx.rx.idx == 0 && b != RELAY_FRAME_HEADER) return false; // bỏ byte rác
    ctx.rx.buf[ctx.rx.idx++] = b;
    return ctx.rx.idx == sizeof(ctx.rx.buf);
}

// Frame sai: bỏ byte đầu, tìm lại Header 0xAA trong phần còn lại
static void ResyncBuffer() {
    uint8_t i = 1;
    while (i < ctx.rx.idx && ctx.rx.buf[i] != RELAY_FRAME_HEADER) i++;
    ctx.rx.idx -= i;
    memmove(ctx.rx.buf, ctx.rx.buf + i, ctx.rx.idx);
}

static void ProcessFrame() {
    const RelayFrame_t* f = (const RelayFrame_t*)ctx.rx.buf;
    if (!IsFrameValid(f)) {
        Serial.println("[UART] Loi Checksum!");
        ResyncBuffer();
        return;
    }
    DispatchFrame(f);
    ctx.rx.idx = 0;
}

static void HandleIncomingBytes() {
    while (Serial2.available() > 0) {
        if (PushByte(Serial2.read())) ProcessFrame();
    }
}

// ===================== Gửi dữ liệu =====================

static void RotateMask() {
    ctx.tx.mask = (uint8_t)(ctx.tx.mask << 1);
    if (ctx.tx.mask == 0) ctx.tx.mask = 0x01;
}

static void SendNextMask() {
    UartRelay_SendCommand(RELAY_CMD_SET, ctx.tx.mask);
    RotateMask();
}

// ===================== API công khai =====================

void UartRelay_Init() {
    Serial2.begin(ctx.cfg.baud, SERIAL_8N1, ctx.cfg.rxPin, ctx.cfg.txPin);
}

void UartRelay_SendCommand(uint8_t cmd, uint8_t mask) {
    RelayFrame_t f = { RELAY_FRAME_HEADER, cmd, mask, 0 };
    f.checksum = CalculateChecksum(&f);
    Serial2.write((const uint8_t*)&f, sizeof(f));
}

void UartRelay_Task() {
    HandleIncomingBytes();
    if (SoftTimer_Expired(&ctx.tx.timer)) SendNextMask();
}