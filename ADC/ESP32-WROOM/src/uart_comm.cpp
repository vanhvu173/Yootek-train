#include "uart_comm.h"

void uart_comm_init(long baud, int rxPin, int txPin) {
    Serial2.begin(baud, SERIAL_8N1, rxPin, txPin);
}

void uart_comm_send(const String &cmd) {
    Serial2.println(cmd);
}