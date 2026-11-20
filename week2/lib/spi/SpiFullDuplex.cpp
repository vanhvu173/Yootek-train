#include "SpiFullDuplex.h"
#include "PinConfig.h"
#include "SoftTimer.h"
#include <SPI.h>

// ===================== Kiểu dữ liệu nội bộ =====================

// Cấu hình VSPI
typedef struct {
    uint8_t  sckPin;
    uint8_t  misoPin;
    uint8_t  mosiPin;
    uint8_t  csPin;
    uint32_t clockHz;
    uint8_t  adcPin;
} SpiCfg_t;

// Trạng thái trung gian của mỗi phiên trao đổi
typedef struct {
    SoftTimer_t timer;
    uint8_t     counter;
    SpiPacket_t tx;
    SpiPacket_t rx;
} SpiState_t;

typedef struct {
    SpiCfg_t   cfg;
    SpiState_t st;
} SpiCtx_t;

static SPIClass vspi(VSPI);

static SpiCtx_t ctx = {
    { SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CS_PIN, SPI_CLOCK_HZ, SPI_ADC_PIN },
    { { SPI_PERIOD_MS, 0 }, 0, { 0, 0, 0 }, { 0, 0, 0 } }
};

// ===================== Chân CS (điều khiển bằng tay) =====================

static void CsInit() {
    pinMode(ctx.cfg.csPin, OUTPUT);
    digitalWrite(ctx.cfg.csPin, HIGH);
}

static void CsSelect(bool selected) {
    digitalWrite(ctx.cfg.csPin, selected ? LOW : HIGH);
}

// ===================== Xử lý gói tin =====================

static void BuildTxPacket() {
    ctx.st.tx.packetId    = ctx.st.counter++;
    ctx.st.tx.sensorValue = (uint16_t)analogRead(ctx.cfg.adcPin);
    ctx.st.tx.statusFlag  = SPI_STATUS_OK;
}

static void LogExchange() {
    Serial.printf("[SPI] Gui ID: %d | Nhan ID: %d, Val: %d\n",
                  ctx.st.tx.packetId, ctx.st.rx.packetId, ctx.st.rx.sensorValue);
}

// ===================== API công khai =====================

void SpiFullDuplex_Init() {
    // Truyền SS = -1 để SPI không chiếm chân CS, ta tự điều khiển bằng CsSelect()
    vspi.begin(ctx.cfg.sckPin, ctx.cfg.misoPin, ctx.cfg.mosiPin, -1);
    CsInit(); // phải gọi SAU begin()
}

void SpiFullDuplex_Transfer(const SpiPacket_t* txData, SpiPacket_t* rxData) {
    vspi.beginTransaction(SPISettings(ctx.cfg.clockHz, MSBFIRST, SPI_MODE0));
    CsSelect(true);
    // Đẩy MOSI và nhận MISO đồng thời trong cùng 1 phiên
    vspi.transferBytes((const uint8_t*)txData, (uint8_t*)rxData, sizeof(SpiPacket_t));
    CsSelect(false);
    vspi.endTransaction();
}

void SpiFullDuplex_Task() {
    if (!SoftTimer_Expired(&ctx.st.timer)) return;
    BuildTxPacket();
    SpiFullDuplex_Transfer(&ctx.st.tx, &ctx.st.rx);
    LogExchange();
}