#ifndef UART_COMM_H
#define UART_COMM_H

#include <Arduino.h>

void uart_comm_init(long baud, int rxPin, int txPin);
void uart_comm_send(const String &cmd);

#endif