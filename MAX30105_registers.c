#ifndef MAX30105_REGISTERS_H
#define MAX30105_REGISTERS_H

// I2C address for MAX30105/MAX30102 (default)
#define MAX30105_I2C_ADDR       0x57

// Interrupt Registers
// #define REG_INTERRUPT_STATUS_1          0x00
// #define REG_INTERRUPT_STATUS_2          0x01
#define REG_INTERRUPT_ENABLE_1          0x02
// #define REG_INTERRUPT_ENABLE_2          0x03

// FIFO Registers
#define REG_FIFO_WR_PTR            0x04
#define REG_FIFO_OVERFLOW_COUNTER  0x05
#define REG_FIFO_RD_PTR            0x06
#define REG_FIFO_DATA              0x07

// Configuration Registers
#define REG_MODE_CONFIG            0x09
#define REG_SPO2_CONFIG            0x0A
#define REG_RED                    0x0C  // Red LED
#define REG_IR                     0x0D  // IR LED
// #define REG_GREEN                  0x0E  // Green LED (only on MAX30105)

#define REG_MULTI_LED_CTRL1        0x11
#define REG_MULTI_LED_CTRL2        0x12

// Temp Registers
#define REG_TEMP_INT               0x1F
#define REG_TEMP_FRAC              0x20
#define REG_TEMP_CONFIG            0x21

// Part ID
#define REG_PART_ID                0xFF

// MODE_CONFIG (0x09) bits
#define MODE_SHUTDOWN              (1 << 7)
#define MODE_RESET                 (1 << 6)
#define MODE_REDONLY               0x02
#define MODE_SPO2                  0x03
#define MODE_MULTILED              0x07

// SPO2_CONFIG (0x0A) bits
// #define SPO2_ADC_RANGE_2048          (0 << 5)
#define SPO2_ADC_RANGE_4096          (1 << 5)
// #define SPO2_ADC_RANGE_8192          (2 << 5)
// #define SPO2_ADC_RANGE_16384         (3 << 5)

// #define SPO2_SR_50                 (0 << 2)
#define SPO2_SR_100                (1 << 2)
// #define SPO2_SR_200                (2 << 2)
// #define SPO2_SR_400                (3 << 2)
// #define SPO2_SR_800                (4 << 2)
// #define SPO2_SR_1000               (5 << 2)
// #define SPO2_SR_1600               (6 << 2)
// #define SPO2_SR_3200               (7 << 2)

// #define SPO2_PW_69                 0
// #define SPO2_PW_118                1
// #define SPO2_PW_215                2
#define SPO2_PW_411                3

#endif // MAX30105_REGISTERS_H
