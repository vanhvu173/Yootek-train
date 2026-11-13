#ifndef ADC_SENSOR_H
#define ADC_SENSOR_H

#include <Arduino.h>

void adc_sensor_init(uint8_t pin);
int adc_sensor_read_raw();
float adc_sensor_read_voltage();

#endif