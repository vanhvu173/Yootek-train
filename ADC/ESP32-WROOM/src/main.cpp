#include <Arduino.h>
#include "adc_sensor.h"
#include "uart_comm.h"

const int ADC_PIN = 34;       // Chân ADC1 đọc cảm biến
const int BUTTON_PIN = 0;     // Nút nhấn BOOT tích hợp trên bo WROOM
const int RX2_PIN = 16;       // RX2 mặc định
const int TX2_PIN = 17;       // TX2 mặc định

bool lastRelay1State = false;
int lastBtnState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long lastReadMillis = 0;
int currentMode = 0;

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    adc_sensor_init(ADC_PIN);
    uart_comm_init(115200, RX2_PIN, TX2_PIN);

}

void loop() {
    // 1. Đọc ADC mỗi 250ms và gửi lệnh đóng/ngắt Relay 1 theo ngưỡng 1.65V
    if (millis() - lastReadMillis >= 250) {
        lastReadMillis = millis();
        int raw = adc_sensor_read_raw();
        float volt = adc_sensor_read_voltage();
        
        Serial.printf("ADC Raw: %4d | Voltage: %.2f V\n", raw, volt);

        if (volt >= 1.65f && !lastRelay1State) {
            lastRelay1State = true;
            uart_comm_send("R1_ON");
            Serial.println("-> Vuot nguong 1.65V: Gui lenh R1_ON");
        } else if (volt < 1.65f && lastRelay1State) {
            lastRelay1State = false;
            uart_comm_send("R1_OFF");
            Serial.println("-> Duoi nguong 1.65V: Gui lenh R1_OFF");
        }
    }

    // 2. Đọc nút nhấn BOOT (GPIO 0) có chống rung để chuyển hiệu ứng trên bo Relay
    int reading = digitalRead(BUTTON_PIN);
    if (reading == LOW && lastBtnState == HIGH && (millis() - lastDebounceTime > 50)) {
        lastDebounceTime = millis();
        currentMode = (currentMode + 1) % 3;
        
        if (currentMode == 0) {
            uart_comm_send("ALL_OFF");
            Serial.println("-> Nhan nut: Gui ALL_OFF");
        } else if (currentMode == 1) {
            uart_comm_send("ALL_ON");
            Serial.println("-> Nhan nut: Gui ALL_ON");
        } else {
            uart_comm_send("CHASE");
            Serial.println("-> Nhan nut: Gui CHASE (Chay duoi)");
        }
    }
    lastBtnState = reading;
}