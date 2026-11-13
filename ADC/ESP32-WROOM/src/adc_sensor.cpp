#include "adc_sensor.h"

static uint8_t _adcPin = 34;

void adc_sensor_init(uint8_t pin) {
    _adcPin = pin;
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
}

int adc_sensor_read_raw() {
    long sum = 0;
    for (int i = 0; i < 16; i++) {
        sum += analogRead(_adcPin);
        delayMicroseconds(50);
    }
    return sum / 16;
}

float adc_sensor_read_voltage() {
    long mvSum = 0;
    for (int i = 0; i < 16; i++) {
        mvSum += analogReadMilliVolts(_adcPin);
        delayMicroseconds(50);
    }
    return (mvSum / 16.0f) / 1000.0f;
}