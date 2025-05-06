#ifndef I3G4250D_H
#define I3G4250D_H

#include "common.h"

#define I3G4250D_CTRL_REG1 0x20
#define I3G4250D_TEMP_REG 0x26
#define I3G4250D_OUT_X_L_REG 0x28
#define I3G4250D_OUT_X_H_REG 0x29
#define I3G4250D_OUT_Y_L_REG 0x2A
#define I3G4250D_OUT_Y_H_REG 0x2B
#define I3G4250D_OUT_Z_L_REG 0x2C
#define I3G4250D_OUT_Z_H_REG 0x2D

#define RATE_CONV 490.0/65536.0

void init_i3g4250();
int read_temp();
int read_gyro_x();
int read_gyro_y();
int read_gyro_z();
float get_z_rateDPS();
void update_step(float* prev_z, int* step_count);
int check_screen();

#endif
