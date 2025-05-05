#include "heart_sensor_bpm.h"

void MAX3010x_Init() {
    uint8_t fifo_cfg[2] = {0x08, 0x90}; // 4 sample average, enable FIFO rollover;
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, fifo_cfg, 2, false);

    uint8_t led_mode[2] = {0x09, 0x02}; // Red only
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, led_mode, 2, false); // Red only

    uint8_t spo2_cfg[2] = {0x0A, 0x00}; // 2048 ADC rangve, 50Hz sample rate, 69us pulse width
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, spo2_cfg, 2, false); // 2048 ADC range, 50Hz sample rate, 69us pulse width

    uint8_t red_pa[2] = {0x0C, 0x0A};
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, red_pa, 2, false); // Set Red LED to 10mA

    uint8_t ir_pa[2] = {0x0D, 0x00};
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, ir_pa, 2, false); // Set IR LED to 10mA
    
    uint8_t multi_led[2] = {0x11, 0x01}; // Set to 1 LED mode (Red only)
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, multi_led, 2, false); // Set to 1 LED mode (Red only)

    uint8_t clr_wrt_ptr[2] = {0x04, 0x00};
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, clr_wrt_ptr, 2, false); // Clear the FIFO write pointer

    uint8_t clr_overflow[2] = {0x05, 0x00};
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, clr_overflow, 2, false); // Clear the FIFO overflow counter

    uint8_t clr_rd_ptr[2] = {0x06, 0x00};
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, clr_rd_ptr, 2, false); // Clear the FIFO read pointer
}



int MAX3010x_ReadTemp() {
  // uint8_t cfg_data[2] = {0x21, 0x01};
  // i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, cfg_data, 2, false); // Set to 1Hz
  uint8_t reg = 0x07;  
  uint8_t data;
  i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, &reg, 1, true);
  i2c_read_blocking(I2C_CHAN, MAX30105_I2C_ADDR, &data, 1, false);
  return data;
}

void MAX3010x_ReadFIFO(uint32_t *red, uint32_t *irValue) {
    uint8_t reg = 0x07;
    uint8_t data[6];
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_CHAN, MAX30105_I2C_ADDR, data, 6, false);

    *red = ((uint32_t)(data[0]) << 16) | ((uint32_t)(data[1]) << 8) | data[2];
    *irValue  = ((uint32_t)(data[3]) << 16) | ((uint32_t)(data[4]) << 8) | data[5];
}

bool checkForBeat(int32_t sample, int16_t IR_AC_Max, int16_t IR_AC_Min, 
    int16_t IR_AC_Signal_Current, int16_t* IR_AC_Signal_Previous,
    int16_t IR_AC_Signal_min, int16_t IR_AC_Signal_max, int16_t IR_Average_Estimated,
    int16_t positiveEdge, int16_t negativeEdge, int32_t ir_avg_reg,
    int16_t cbuf[32], uint8_t offset, const uint16_t FIRCoeffs[12])
{
  bool beatDetected = false;

  //  Save current state
  *IR_AC_Signal_Previous = IR_AC_Signal_Current;
  
  //This is good to view for debugging
  //Serial.print("Signal_Current: ");
  //Serial.println(IR_AC_Signal_Current);

  //  Process next data sample
  IR_Average_Estimated = averageDCEstimator(&ir_avg_reg, sample);
  IR_AC_Signal_Current = lowPassFIRFilter(sample - IR_Average_Estimated, cbuf, offset, FIRCoeffs);

  //  Detect positive zero crossing (rising edge)
  if ((*IR_AC_Signal_Previous < 0) & (IR_AC_Signal_Current >= 0))
  {
  
    IR_AC_Max = IR_AC_Signal_max; //Adjust our AC max and min
    IR_AC_Min = IR_AC_Signal_min;

    positiveEdge = 1;
    negativeEdge = 0;
    IR_AC_Signal_max = 0;

    //if ((IR_AC_Max - IR_AC_Min) > 100 & (IR_AC_Max - IR_AC_Min) < 1000)
    if ((IR_AC_Max - IR_AC_Min) > 20 & (IR_AC_Max - IR_AC_Min) < 1000)
    {
      //Heart beat!!!
      beatDetected = true;
    }
  }

  //  Detect negative zero crossing (falling edge)
  if ((*IR_AC_Signal_Previous > 0) & (IR_AC_Signal_Current <= 0))
  {
    positiveEdge = 0;
    negativeEdge = 1;
    IR_AC_Signal_min = 0;
  }

  //  Find Maximum value in positive cycle
  if (positiveEdge & (IR_AC_Signal_Current > *IR_AC_Signal_Previous))
  {
    IR_AC_Signal_max = IR_AC_Signal_Current;
  }

  //  Find Minimum value in negative cycle
  if (negativeEdge & (IR_AC_Signal_Current < *IR_AC_Signal_Previous))
  {
    IR_AC_Signal_min = IR_AC_Signal_Current;
  }
  
  return(beatDetected);
}

//  Average DC Estimator
int16_t averageDCEstimator(int32_t *p, uint16_t x)
{
  *p += ((((long) x << 15) - *p) >> 4);
  return (*p >> 15);
}

//  Low Pass FIR Filter
int16_t lowPassFIRFilter(int16_t din, int16_t* cbuf, uint8_t offset, const uint16_t* FIRCoeffs)
{  
  cbuf[offset] = din;

  int32_t z = mul16(FIRCoeffs[11], cbuf[(offset - 11) & 0x1F]);
  
  for (uint8_t i = 0 ; i < 11 ; i++)
  {
    z += mul16(FIRCoeffs[i], cbuf[(offset - i) & 0x1F] + cbuf[(offset - 22 + i) & 0x1F]);
  }

  offset++;
  offset %= 32; //Wrap condition

  return(z >> 15);
}

//  Integer multiplier
int32_t mul16(int16_t x, int16_t y)
{
  return((long)x * (long)y);
}