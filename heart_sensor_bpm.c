#include <stdint.h>
#include <stdio.h>
#include "MAX30105_registers.h"
#include "heartRate.h" 

#define RATE_SIZE 4
float rates[RATE_SIZE];
uint8_t rateSpot = 0;
uint32_t lastBeat = 0;

extern void i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value);
extern void i2c_read_multi(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);
extern void delay_ms(uint32_t ms);
extern uint32_t millis();  
void MAX3010x_Init() {
    // Reset the sensor
    i2c_write_reg(MAX30105_I2C_ADDR, REG_MODE_CONFIG, MODE_RESET);
    delay_ms(100);

    // Enable interrupts
    i2c_write_reg(MAX30105_I2C_ADDR, REG_INTERRUPT_ENABLE_1, 0xC0);  // A_FULL + PPG Ready

    // Clear FIFO
    i2c_write_reg(MAX30105_I2C_ADDR, REG_FIFO_WR_PTR, 0x00);
    i2c_write_reg(MAX30105_I2C_ADDR, REG_FIFO_RD_PTR, 0x00);
    i2c_write_reg(MAX30105_I2C_ADDR, REG_FIFO_OERVFLOW_COUNTER, 0x00);

    uint8_t spo2_config = SPO2_SR_100 | SPO2_ADC_RANGE_4096 | SPO2_PW_411;
    i2c_write_reg(MAX30105_I2C_ADDR, REG_SPO2_CONFIG, spo2_config);

    // Set LED pulse amplitudes
    i2c_write_reg(MAX30105_I2C_ADDR, REG_RED, 0x24);  // Red 
    i2c_write_reg(MAX30105_I2C_ADDR, REG_IR, 0x24);  // IR

    // Set mode to SpO2 (uses Red + IR)
    i2c_write_reg(MAX30105_I2C_ADDR, REG_MODE_CONFIG, MODE_SPO2);
}

void MAX3010x_ReadFIFO(uint32_t *red, uint32_t *irValue) {
    uint8_t data[6];
    i2c_read_multi(MAX30105_I2C_ADDR, REG_FIFO_DATA, data, 6);

    *red = ((uint32_t)(data[0]) << 16) | ((uint32_t)(data[1]) << 8) | data[2];
    *irValue  = ((uint32_t)(data[3]) << 16) | ((uint32_t)(data[4]) << 8) | data[5];
}

int main() {
    MAX3010x_Init(); // Set a timer!!

    while (1) {
        uint32_t red, irValue;
        MAX3010x_ReadFIFO(&red, &irValue);

        if (checkForBeat(irValue)) {
            uint32_t now = millis();
            uint32_t delta = now - lastBeat;
            lastBeat = now;

            float BeatsPerMinute = 60000.0f / delta;
            rates[rateSpot++] = beatsPerMinute;
            if (rateSpot >= RATE_SIZE) rateSpot = 0;

            float beatAvg = 0;
            for (int i = 0; i < RATE_SIZE; i++) 
                beatAvg += rates[i];
            beatAvg /= RATE_SIZE;

            printf("Beat detected! IR = %lu, BPM = %.2f\n", irValue, beatAvg);
        }

        delay_ms(10);
    }
}
