#include "I2cMasterCom.h"
#include "PinConfig.h"
#include "SoftTimer.h"
#include <Wire.h>


// Cấu hình I2C Master
typedef struct {
    uint8_t  sdaPin;
    uint8_t  sclPin;
    uint32_t freqHz;
    uint8_t  slaveAddr;
} I2cMasterCfg_t;

typedef struct {
    I2cMasterCfg_t cfg;
    SoftTimer_t    timer;
} I2cMasterCtx_t;

static I2cMasterCtx_t ctx = {
    { I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ, I2C_SLAVE_ADDR },
    { I2C_PERIOD_MS, 0 }
};


static I2cPayload_t BuildTxPayload() {
    I2cPayload_t p = { I2C_CMD_SET_TEMP, I2C_TX_TEMP_C, I2C_ERR_NONE };
    return p;
}

static void LogWrite(bool ok) {
    Serial.println(ok ? "[I2C] Gui du lieu toi Slave thanh cong"
                      : "[I2C] Slave khong phan hoi (NACK)");
}

static void LogRead(bool ok, const I2cPayload_t* p) {
    if (!ok) {
        Serial.println("[I2C] Doc tu Slave that bai");
        return;
    }
    Serial.printf("[I2C] Doc tu Slave -> Temp: %.2f | Err: %d\n",
                  p->temperature, p->errorCode);
}



void I2cMaster_Init() {
    // Ép kiểu rõ ràng để tránh nhập nhằng overload Wire.begin()
    Wire.begin((int)ctx.cfg.sdaPin, (int)ctx.cfg.sclPin, (uint32_t)ctx.cfg.freqHz);
}

bool I2cMaster_WriteData(const I2cPayload_t* data) {
    Wire.beginTransmission(ctx.cfg.slaveAddr);
    Wire.write((const uint8_t*)data, sizeof(I2cPayload_t));
    return Wire.endTransmission() == 0; // true nếu nhận được ACK
}

bool I2cMaster_ReadData(I2cPayload_t* data) {
    uint8_t n = Wire.requestFrom((uint8_t)ctx.cfg.slaveAddr, (uint8_t)sizeof(I2cPayload_t));
    if (n != sizeof(I2cPayload_t)) return false;
    return Wire.readBytes((uint8_t*)data, sizeof(I2cPayload_t)) == sizeof(I2cPayload_t);
}

void I2cMaster_Task() {
    if (!SoftTimer_Expired(&ctx.timer)) return;

    I2cPayload_t tx = BuildTxPayload();
    I2cPayload_t rx = { 0, 0, 0 };

    LogWrite(I2cMaster_WriteData(&tx));

    bool readOk = I2cMaster_ReadData(&rx);
    LogRead(readOk, &rx);
}