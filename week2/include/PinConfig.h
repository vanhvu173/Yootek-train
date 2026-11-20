#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// --- UART2 (Giao tiếp Relay-X8) ---
#define UART_RELAY_RX       16
#define UART_RELAY_TX       17
#define UART_RELAY_BAUD     115200

// --- VSPI (Giao tiếp Song công Full-Duplex) ---
#define SPI_SCK_PIN         18
#define SPI_MISO_PIN        19
#define SPI_MOSI_PIN        23
#define SPI_CS_PIN          5
#define SPI_CLOCK_HZ        1000000 // 1 MHz

// --- I2C0 (Master giao tiếp ESP32 Slave) ---
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22
#define I2C_FREQ_HZ         100000  // 100 kHz
#define I2C_SLAVE_ADDR      0x08

#endif