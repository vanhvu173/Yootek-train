#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// --- UART2 (Giao tiep Relay-X8) ---
#define UART_RELAY_RX           16
#define UART_RELAY_TX           17
#define UART_RELAY_BAUD         115200
#define UART_RELAY_PERIOD_MS    1000    // chu ky gui lenh dich bit relay

// --- VSPI (Giao tiep Song cong Full-Duplex) ---
#define SPI_SCK_PIN             18
#define SPI_MISO_PIN            19
#define SPI_MOSI_PIN            23
#define SPI_CS_PIN              5
#define SPI_CLOCK_HZ            1000000 // 1 MHz
#define SPI_PERIOD_MS           500     // chu ky moi phien trao doi
#define SPI_ADC_PIN             34      // chan ADC lay du lieu gui di
#define SPI_STATUS_OK           0x01

// --- I2C0 (Master giao tiep ESP32 Slave) ---
#define I2C_SDA_PIN             21
#define I2C_SCL_PIN             22
#define I2C_FREQ_HZ             100000  // 100 kHz
#define I2C_SLAVE_ADDR          0x08
#define I2C_PERIOD_MS           2000    // chu ky Master hoi Slave
#define I2C_TX_TEMP_C           28.5f   // nhiet do mau Master gui di

#endif