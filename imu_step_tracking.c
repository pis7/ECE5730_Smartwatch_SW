#include "i3g4250d.h"
#define I3G4250_ADDR 0x69 

i2c_write_reg(I3G4250_ADDR, 0x20, 0x0F);
i2c_write_reg(I3G4250_ADDR, 0x23, 0x00); 

int16_t read_gyro_z() {
    uint8_t reg = I3G4250D_OUT_X_L_REG | 0x80;
    uint8_t data[2];
    i2c_write_blocking(I2C_CHAN, I3G4250D_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_CHAN, I3G4250D_ADDR, data, 2, false);
    int16_t tmp_data = (data[1] << 8) | data[0];
    return (int)(tmp_data * RATE_CONV);
  }

float get_z_rateDPS() {
    int16_t raw_z = read_gyro_z();
    return raw_z * 0.00875f;  // For 250 dps: 8.75 mdps/LSB
}

float prev_z = 0;
int step_count = 0;

void update_step() {
    float z = get_z_rateDPS();
    if (z > 1.2 && prev_z <= 1.2) {
        step_count++;
        printf("Step %d\n", step_count);
    }

    prev_z = z;
}
