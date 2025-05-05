#include "heart_sensor_bpm.h"

void MAX3010x_Init() {
    uint8_t fifo_cfg[2] = {0x08, 0x50};
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, fifo_cfg, 2, false); // 4 sample average, enable FIFO rollover

    uint8_t led_mode[2] = {0x09, 0x03};
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, led_mode, 2, false); // Red and IR enabled

    uint8_t spo2_cfg[2] = {0x0A, 0x2F};
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, spo2_cfg, 2, false); // 4096 ADC range, 400 samples/sec, 411 us pulse width

    uint8_t red_pa[2] = {0x0C, 0x0A};
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, red_pa, 2, false); // Set Red LED to 10mA

    uint8_t ir_pa[2] = {0x0D, 0x1F};
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, ir_pa, 2, false); // Set IR LED to 10mA
    
    uint8_t multi_led[2] = {0x11, 0x21};
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, multi_led, 2, false); // Set to slot 1 to red, slot 2 to IR

    uint8_t clr_wrt_ptr[2] = {0x04, 0x00};
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, clr_wrt_ptr, 2, false); // Clear the FIFO write pointer

    uint8_t clr_overflow[2] = {0x05, 0x00};
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, clr_overflow, 2, false); // Clear the FIFO overflow counter

    uint8_t clr_rd_ptr[2] = {0x06, 0x00};
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, clr_rd_ptr, 2, false); // Clear the FIFO read pointer
}

void hr_off() {
  uint8_t led_mode[2] = {0x09, 0x80};
  i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, led_mode, 2, false); // Disable all LEDs
}

void hr_on() {
  uint8_t led_mode[2] = {0x09, 0x03};
  i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, led_mode, 2, false); // Enable Red and IR LEDs
}

uint16_t check_hr(sense_struct *sense) {
  
  // Get read pointer
  uint8_t rd_ptr_reg = 0x06;
  uint8_t rd_ptr;
  i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, &rd_ptr_reg, 1, true);
  i2c_read_blocking(I2C_CHAN, MAX30105_I2C_ADDR, &rd_ptr, 1, false);

  // Get write pointer
  uint8_t wr_ptr_reg = 0x04;
  uint8_t wr_ptr;
  i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, &wr_ptr_reg, 1, true);
  i2c_read_blocking(I2C_CHAN, MAX30105_I2C_ADDR, &wr_ptr, 1, false);

  int numberOfSamples = 0;
  int activeLEDs = 2; // Red and IR

  //Do we have new data?
  if (rd_ptr != wr_ptr)
  {
    //Calculate the number of readings we need to get from sensor
    numberOfSamples = wr_ptr - rd_ptr;
    if (numberOfSamples < 0) numberOfSamples += 32; //Wrap condition
    int bytesLeftToRead = numberOfSamples * activeLEDs * 3;
    uint8_t fifo_data_reg = 0x07;
    i2c_write_blocking(I2C_CHAN, MAX30105_I2C_ADDR, &fifo_data_reg, 1, true);
    while (bytesLeftToRead > 0)
    {
      int toGet = bytesLeftToRead;
      if (toGet > I2C_BUFFER_LENGTH)
        toGet = I2C_BUFFER_LENGTH - (I2C_BUFFER_LENGTH % (activeLEDs * 3));
      bytesLeftToRead -= toGet;
      while (toGet > 0) {
        sense->head++; //Advance the head of the storage struct
        sense->head %= HR_STORAGE_SIZE; //Wrap condition
        uint8_t temp[6];
        uint32_t tempLong;
        i2c_read_blocking(I2C_CHAN, MAX30105_I2C_ADDR, temp, 6, false);
        uint8_t red_temp[4] = {temp[2], temp[1], temp[0], 0x00};
        uint8_t ir_temp[4] = {temp[5], temp[4], temp[3], 0x00};
        memcpy(&tempLong, red_temp, sizeof(tempLong));
        tempLong &= 0x3FFFF; //Zero out all but 18 bits
        sense->red[sense->head] = tempLong;
        memcpy(&tempLong, ir_temp, sizeof(tempLong));
        tempLong &= 0x3FFFF; //Zero out all but 18 bits
        sense->IR[sense->head] = tempLong;
        toGet -= activeLEDs * 3;
      }
    }
  }
  return (numberOfSamples);
}

bool safeCheck(uint8_t timeout, sense_struct *sense) {
  uint32_t markTime = to_ms_since_boot(get_absolute_time());
  
  while(1) {
    if(to_ms_since_boot(get_absolute_time()) - markTime > timeout) return false;
    if(check_hr(sense)) return true;
    sleep_ms(1);
  }
}

uint32_t getIR(sense_struct *sense) {
  if(safeCheck(250, sense)) return (sense->IR[sense->head]);
  else return 0;
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

bool checkForBeat(int32_t sample, int16_t* IR_AC_Max, int16_t* IR_AC_Min, 
    int16_t* IR_AC_Signal_Current, int16_t* IR_AC_Signal_Previous,
    int16_t* IR_AC_Signal_min, int16_t* IR_AC_Signal_max, int16_t* IR_Average_Estimated,
    int16_t* positiveEdge, int16_t* negativeEdge, int32_t* ir_avg_reg,
    int16_t cbuf[32], uint8_t* offset, const uint16_t FIRCoeffs[12])
{
  bool beatDetected = false;

  //  Save current state
  *IR_AC_Signal_Previous = *IR_AC_Signal_Current;

  //  Process next data sample
  *IR_Average_Estimated = averageDCEstimator(ir_avg_reg, sample);
  *IR_AC_Signal_Current = lowPassFIRFilter(sample - *IR_Average_Estimated, cbuf, offset, FIRCoeffs);

  //  Detect positive zero crossing (rising edge)
  if ((*IR_AC_Signal_Previous < 0) & (*IR_AC_Signal_Current >= 0))
  {
  
    *IR_AC_Max = *IR_AC_Signal_max; //Adjust our AC max and min
    *IR_AC_Min = *IR_AC_Signal_min;

    *positiveEdge = 1;
    *negativeEdge = 0;
    *IR_AC_Signal_max = 0;

    //if ((IR_AC_Max - IR_AC_Min) > 100 & (IR_AC_Max - IR_AC_Min) < 1000)
    if ((*IR_AC_Max - *IR_AC_Min) > 20 & (*IR_AC_Max - *IR_AC_Min) < 1000)
    {
      //Heart beat!!!
      beatDetected = true;
    }
  }

  //  Detect negative zero crossing (falling edge)
  if ((*IR_AC_Signal_Previous > 0) & (*IR_AC_Signal_Current <= 0))
  {
    *positiveEdge = 0;
    *negativeEdge = 1;
    *IR_AC_Signal_min = 0;
  }

  //  Find Maximum value in positive cycle
  if (*positiveEdge & (*IR_AC_Signal_Current > *IR_AC_Signal_Previous))
  {
    *IR_AC_Signal_max = *IR_AC_Signal_Current;
  }

  //  Find Minimum value in negative cycle
  if (*negativeEdge & (*IR_AC_Signal_Current < *IR_AC_Signal_Previous))
  {
    *IR_AC_Signal_min = *IR_AC_Signal_Current;
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
int16_t lowPassFIRFilter(int16_t din, int16_t* cbuf, uint8_t* offset, const uint16_t* FIRCoeffs)
{  
  cbuf[*offset] = din;

  int32_t z = mul16(FIRCoeffs[11], cbuf[(*offset - 11) & 0x1F]);
  
  for (uint8_t i = 0 ; i < 11 ; i++)
  {
    z += mul16(FIRCoeffs[i], cbuf[(*offset - i) & 0x1F] + cbuf[(*offset - 22 + i) & 0x1F]);
  }

  *offset = *offset + 1;
  *offset = *offset % 32; //Wrap condition

  return(z >> 15);
}

//  Integer multiplier
int32_t mul16(int16_t x, int16_t y)
{
  return((long)x * (long)y);
}