#ifndef HEART_SENSOR_BPM_H
#define HEART_SENSOR_BPM_H

#include "common.h"

#define MAX30105_I2C_ADDR 0x57

void MAX3010x_Init();
int MAX3010x_ReadTemp();
void MAX3010x_ReadFIFO(uint32_t *red, uint32_t *irValue);
bool checkForBeat(int32_t sample, int16_t IR_AC_Max, int16_t IR_AC_Min, 
    int16_t IR_AC_Signal_Current, int16_t* IR_AC_Signal_Previous,
    int16_t IR_AC_Signal_min, int16_t IR_AC_Signal_max, int16_t IR_Average_Estimated,
    int16_t positiveEdge, int16_t negativeEdge, int32_t ir_avg_reg,
    int16_t cbuf[32], uint8_t offset, const uint16_t FIRCoeffs[12]);
int16_t averageDCEstimator(int32_t *p, uint16_t x);
int16_t lowPassFIRFilter(int16_t din, int16_t* cbuf, uint8_t offset, const uint16_t* FIRCoeffs);
int32_t mul16(int16_t x, int16_t y);

#endif
