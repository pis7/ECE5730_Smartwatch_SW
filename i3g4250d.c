#include "i3g4250d.h"

void init_i3g4250() {
  uint8_t data[2] = {I3G4250D_CTRL_REG1 | 0x80, 0x0F};
  i2c_write_blocking(I2C_CHAN, I3G4250D_ADDR, data, 2, false);
}

int read_temp() {
  uint8_t temp_reg = I3G4250D_TEMP_REG | 0x80;
  uint8_t data;
  i2c_write_blocking(I2C_CHAN, I3G4250D_ADDR, &temp_reg, 1, true);
  i2c_read_blocking(I2C_CHAN, I3G4250D_ADDR, &data, 1, false);
  return (data * -1) + 25; // Convert to Celsius
}

int read_gyro_x() {
  uint8_t reg = I3G4250D_OUT_X_L_REG | 0x80;
  uint8_t data[2];
  i2c_write_blocking(I2C_CHAN, I3G4250D_ADDR, &reg, 1, true);
  i2c_read_blocking(I2C_CHAN, I3G4250D_ADDR, data, 2, false);
  int16_t tmp_data = (data[1] << 8) | data[0];
  return (int)(tmp_data * RATE_CONV);
}

int read_gyro_y() {
  uint8_t reg = I3G4250D_OUT_Y_L_REG | 0x80;
  uint8_t data[2];
  i2c_write_blocking(I2C_CHAN, I3G4250D_ADDR, &reg, 1, true);
  i2c_read_blocking(I2C_CHAN, I3G4250D_ADDR, data, 2, false);
  int16_t tmp_data = (data[1] << 8) | data[0];
  return (int)(tmp_data * RATE_CONV);
}

int read_gyro_z() {
  uint8_t reg = I3G4250D_OUT_Z_L_REG | 0x80;
  uint8_t data[2];
  i2c_write_blocking(I2C_CHAN, I3G4250D_ADDR, &reg, 1, true);
  i2c_read_blocking(I2C_CHAN, I3G4250D_ADDR, data, 2, false);
  int16_t tmp_data = (data[1] << 8) | data[0];
  return (int)(tmp_data * RATE_CONV);
}