#include "max30102.h"

//uch_spo2_table is approximated as  -45.060*ratioAverage* ratioAverage + 30.354 *ratioAverage + 94.845;
const uint8_t uch_spo2_table[184] = {95, 95, 95, 96, 96, 96, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98, 99, 99, 99, 99, 
              99, 99, 99, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 
              100, 100, 100, 100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 98, 98, 98, 97, 97, 
              97, 97, 96, 96, 96, 96, 95, 95, 95, 94, 94, 94, 93, 93, 93, 92, 92, 92, 91, 91, 
              90, 90, 89, 89, 89, 88, 88, 87, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81, 
              80, 80, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68, 67, 
              66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 
              49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 31, 30, 29, 
              28, 27, 26, 25, 23, 22, 21, 20, 19, 17, 16, 15, 14, 12, 11, 10, 9, 7, 6, 5, 
              3, 2, 1};
static int32_t an_ir[MAX30102_BUFF_SZ];  // ir
static int32_t an_red[MAX30102_BUFF_SZ]; // red

void max30102_init() {
  uint8_t fifo_cfg[2] = {0x08, 0x50};
  i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, fifo_cfg, 2, false); // 4 sample average, enable FIFO rollover

  uint8_t led_mode[2] = {0x09, 0x03};
  i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, led_mode, 2, false); // Red and IR enabled

  uint8_t spo2_cfg[2] = {0x0A, 0x2F};
  i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, spo2_cfg, 2, false); // 4096 ADC range, 400 samples/sec, 411 us pulse width

  uint8_t red_pa[2] = {0x0C, 0x1F};
  i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, red_pa, 2, false); // Set Red LED to 10mA

  uint8_t ir_pa[2] = {0x0D, 0x1F};
  i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, ir_pa, 2, false); // Set IR LED to 10mA
  
  uint8_t multi_led[2] = {0x11, 0x21};
  i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, multi_led, 2, false); // Set to slot 1 to red, slot 2 to IR

  uint8_t clr_wrt_ptr[2] = {0x04, 0x00};
  i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, clr_wrt_ptr, 2, false); // Clear the FIFO write pointer

  uint8_t clr_overflow[2] = {0x05, 0x00};
  i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, clr_overflow, 2, false); // Clear the FIFO overflow counter

  uint8_t clr_rd_ptr[2] = {0x06, 0x00};
  i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, clr_rd_ptr, 2, false); // Clear the FIFO read pointer
}

void max30102_off() {
  uint8_t led_mode[2] = {0x09, 0x80};
  i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, led_mode, 2, false); // Disable all LEDs
}

void max30102_on() {
  uint8_t led_mode[2] = {0x09, 0x03};
  i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, led_mode, 2, false); // Enable Red and IR LEDs
}

uint16_t max30102_hr_check(
  sense_struct *sense
) {
  
  // Get read pointer
  uint8_t rd_ptr_reg = 0x06;
  uint8_t rd_ptr;
  i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, &rd_ptr_reg, 1, true);
  i2c_read_blocking(I2C_CHAN, MAX30102_I2C_ADDR, &rd_ptr, 1, false);

  // Get write pointer
  uint8_t wr_ptr_reg = 0x04;
  uint8_t wr_ptr;
  i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, &wr_ptr_reg, 1, true);
  i2c_read_blocking(I2C_CHAN, MAX30102_I2C_ADDR, &wr_ptr, 1, false);

  int num_samples = 0;
  int active_leds = 2; // Red and IR

  //Do we have new data?
  if (rd_ptr != wr_ptr)
  {
    //Calculate the number of readings we need to get from sensor
    num_samples = wr_ptr - rd_ptr;
    if (num_samples < 0) num_samples += 32; //Wrap condition
    int bytes_to_read = num_samples * active_leds * 3;
    uint8_t fifo_data_reg = 0x07;
    i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, &fifo_data_reg, 1, true);
    while (bytes_to_read > 0)
    {
      int toGet = bytes_to_read;
      if (toGet > MAX30102_I2C_BUFF_LEN)
        toGet = MAX30102_I2C_BUFF_LEN - (MAX30102_I2C_BUFF_LEN % (active_leds * 3));
      bytes_to_read -= toGet;
      while (toGet > 0) {
        sense->head++; //Advance the head of the storage struct
        sense->head %= MAX30102_HR_STORE_SZ; //Wrap condition
        uint8_t temp[6];
        uint32_t temp_long;
        i2c_read_blocking(I2C_CHAN, MAX30102_I2C_ADDR, temp, 6, false);
        uint8_t red_temp[4] = {temp[2], temp[1], temp[0], 0x00};
        uint8_t ir_temp[4] = {temp[5], temp[4], temp[3], 0x00};
        memcpy(&temp_long, red_temp, sizeof(temp_long));
        temp_long &= 0x3FFFF; //Zero out all but 18 bits
        sense->red[sense->head] = temp_long;
        memcpy(&temp_long, ir_temp, sizeof(temp_long));
        temp_long &= 0x3FFFF; //Zero out all but 18 bits
        sense->ir[sense->head] = temp_long;
        toGet -= active_leds * 3;
      }
    }
  }
  return num_samples;
}

bool max30102_hr_safe_check(
  uint8_t       timeout, 
  sense_struct* sense
) {
  uint32_t start_time = to_ms_since_boot(get_absolute_time());
  
  while(1) {
    if (to_ms_since_boot(get_absolute_time()) - start_time > timeout) return false;
    if (max30102_hr_check(sense)) return true;
    sleep_ms(1);
  }
}

uint32_t max30102_get_red(
  sense_struct* sense
) {
  if (max30102_hr_safe_check(250, sense)) return (sense->red[sense->head]);
  else return 0;
}

uint32_t max30102_get_ir(
  sense_struct* sense
) {
  if(max30102_hr_safe_check(250, sense)) return (sense->ir[sense->head]);
  else return 0;
}

uint8_t max30102_avail(
  sense_struct* sense
) {
  int8_t num_samples = sense->head - sense->tail;
  if (num_samples < 0) num_samples += MAX30102_HR_STORE_SZ; //Wrap condition
  return (uint8_t)num_samples;
}

void max30102_next_sample(
  sense_struct* sense
) {
  if (max30102_avail(sense)) {
    sense->tail++; //Advance the tail of the storage struct
    sense->tail %= MAX30102_HR_STORE_SZ; //Wrap condition
  }
}

void max30102_read_fifo(
  uint32_t* red, 
  uint32_t* irValue
) {
  uint8_t fifo_data_reg = 0x07;
  i2c_write_blocking(I2C_CHAN, MAX30102_I2C_ADDR, &fifo_data_reg, 1, true);
  uint8_t temp[6];
  uint32_t temp_long;
  i2c_read_blocking(I2C_CHAN, MAX30102_I2C_ADDR, temp, 6, false);
  uint8_t red_temp[4] = {temp[2], temp[1], temp[0], 0x00};
  uint8_t ir_temp[4] = {temp[5], temp[4], temp[3], 0x00};
  memcpy(&temp_long, red_temp, sizeof(temp_long));
  temp_long &= 0x3FFFF; //Zero out all but 18 bits
  *red = temp_long;
  memcpy(&temp_long, ir_temp, sizeof(temp_long));
  temp_long &= 0x3FFFF; //Zero out all but 18 bits
  *irValue = temp_long;
}

//  Average DC Estimator
int16_t max30102_hr_avg_dc_estimator(
  int32_t* p, 
  uint16_t x
) {
  *p += ((((long) x << 15) - *p) >> 4);
  return (*p >> 15);
}

// Low Pass FIR Filter
int16_t max30102_hr_low_pass_fir_filter(
  int16_t         din, 
  int16_t*        cbuf, 
  uint8_t*        offset, 
  const uint16_t* FIRCoeffs
) {  
  cbuf[*offset] = din;
  int32_t z = (long)(FIRCoeffs[11]) * (long)(cbuf[(*offset - 11) & 0x1F]);
  for (uint8_t i = 0; i < 11; i++)
    z += (long)(FIRCoeffs[i]) * (long)(cbuf[(*offset - i) & 0x1F] + cbuf[(*offset - 22 + i) & 0x1F]);
  *offset = *offset + 1;
  *offset = *offset % 32;
  return(z >> 15);
}

bool max30102_hr_check_for_beat(
  int32_t        sample, 
  hr_signal_t*   hr_signal, 
  const uint16_t fir_coeffs[12]
) {
  bool beat_detected = false;

  //  Save current state
  hr_signal->hr_ir_ac_sig_prev = hr_signal->hr_ir_ac_sig_curr;

  //  Process next data sample
  hr_signal->hr_ir_avg_est = max30102_hr_avg_dc_estimator(&(hr_signal->hr_ir_avg_reg), sample);
  hr_signal->hr_ir_ac_sig_curr = max30102_hr_low_pass_fir_filter(
                                                sample - hr_signal->hr_ir_avg_est, 
                                                hr_signal->hr_cbuf, &(hr_signal->hr_offset), fir_coeffs
                                              );

  //  Detect positive zero crossing (rising edge)
  if ((hr_signal->hr_ir_ac_sig_prev < 0) & (hr_signal->hr_ir_ac_sig_curr >= 0)) {
    hr_signal->hr_ir_ac_max = hr_signal->hr_ir_ac_sig_max;
    hr_signal->hr_ir_ac_min = hr_signal->hr_ir_ac_sig_min;
    hr_signal->hr_pos_edge = 1;
    hr_signal->hr_neg_edge = 0;
    hr_signal->hr_ir_ac_sig_max = 0;
    if ((hr_signal->hr_ir_ac_max - hr_signal->hr_ir_ac_min) > 20 & 
        (hr_signal->hr_ir_ac_max - hr_signal->hr_ir_ac_min) < 1000)
      beat_detected = true;
  }

  //  Detect negative zero crossing (falling edge)
  if ((hr_signal->hr_ir_ac_sig_prev > 0) & (hr_signal->hr_ir_ac_sig_curr <= 0))
  {
    hr_signal->hr_pos_edge = 0;
    hr_signal->hr_neg_edge = 1;
    hr_signal->hr_ir_ac_sig_min = 0;
  }

  //  Find Maximum value in positive cycle
  if (hr_signal->hr_pos_edge & 
    (hr_signal->hr_ir_ac_sig_curr > hr_signal->hr_ir_ac_sig_prev))
    hr_signal->hr_ir_ac_sig_max = hr_signal->hr_ir_ac_sig_curr;

  //  Find Minimum value in negative cycle
  if (hr_signal->hr_neg_edge & 
    (hr_signal->hr_ir_ac_sig_curr < hr_signal->hr_ir_ac_sig_prev))
    hr_signal->hr_ir_ac_sig_min = hr_signal->hr_ir_ac_sig_curr;
  
  return beat_detected;
}

// By detecting  peaks of PPG cycle and corresponding AC/DC of red/infra-red signal, the an_ratio for the SPO2 is computed.
// Since this algorithm is aiming for Arm M0/M3. formaula for SPO2 did not achieve the accuracy due to register overflow.
// Thus, accurate SPO2 is precalculated and save longo uch_spo2_table[] per each an_ratio.
void max30102_read_spo2(
  uint32_t* pun_ir_buffer, 
  int32_t   n_ir_buffer_length, 
  uint32_t* pun_red_buffer, 
  int32_t*  pn_spo2, 
  int8_t*   pch_spo2_valid
) {
  uint32_t un_ir_mean;
  int32_t k, n_i_ratio_count;
  int32_t i, n_exact_ir_valley_locs_count, n_middle_idx;
  int32_t n_th1, n_npks;   
  int32_t an_ir_valley_locs[15];
  int32_t n_peak_interval_sum;
  
  int32_t n_y_ac, n_x_ac;
  int32_t n_spo2_calc; 
  int32_t n_y_dc_max, n_x_dc_max; 
  int32_t n_y_dc_max_idx = 0;
  int32_t n_x_dc_max_idx = 0; 
  int32_t an_ratio[5], n_ratio_average; 
  int32_t n_nume, n_denom;

  // calculates DC mean and subtract DC from ir
  un_ir_mean = 0; 
  for (k = 0; k < n_ir_buffer_length; k++) un_ir_mean += pun_ir_buffer[k];
  un_ir_mean = un_ir_mean/n_ir_buffer_length;
    
  // remove DC and invert signal so that we can use peak detector as valley detector
  for (k = 0; k < n_ir_buffer_length; k++)  
    an_ir[k] = -1*(pun_ir_buffer[k] - un_ir_mean); 
    
  // 4 pt Moving Average
  for(k = 0; k < MAX30102_BUFF_SZ-MAX30102_MA4_SZ; k++)
    an_ir[k] = (an_ir[k] + an_ir[k+1] + an_ir[k+2] + an_ir[k+3])/(int)4;        

  // calculate threshold  
  n_th1 = 0; 
  for (k = 0; k < MAX30102_BUFF_SZ; k++)
    n_th1 += an_ir[k];
  n_th1=  n_th1/MAX30102_BUFF_SZ;
  if(n_th1 < 30) n_th1 = 30; // min allowed
  if(n_th1 > 60) n_th1 = 60; // max allowed

  for (k = 0; k < 15; k++) an_ir_valley_locs[k] = 0;

  // since we flipped signal, we use peak detector as valley detector
  max30102_spo2_find_peaks(an_ir_valley_locs, &n_npks, an_ir, MAX30102_BUFF_SZ, n_th1, 4, 15);// peak_height, peak_distance, max_num_peaks 
  n_peak_interval_sum = 0;
  if (n_npks >= 2) {
    for (k = 1; k < n_npks; k++) n_peak_interval_sum += (an_ir_valley_locs[k] - an_ir_valley_locs[k -1]);
    n_peak_interval_sum = n_peak_interval_sum/(n_npks-1);
  }

  // load raw value again for SPO2 calculation : RED(=y) and IR(=X)
  for (k = 0; k < n_ir_buffer_length; k++)  {
      an_ir[k] = pun_ir_buffer[k]; 
      an_red[k] = pun_red_buffer[k]; 
  }

  // find precise min near an_ir_valley_locs
  n_exact_ir_valley_locs_count = n_npks; 
  
  // using exact_ir_valley_locs , find ir-red DC andir-red AC for SPO2 calibration an_ratio
  // finding AC/DC maximum of raw

  n_ratio_average = 0; 
  n_i_ratio_count = 0; 
  for(k = 0; k < 5; k++) an_ratio[k] = 0;
  for (k = 0; k< n_exact_ir_valley_locs_count; k++) {
    if (an_ir_valley_locs[k] > MAX30102_BUFF_SZ ) {
      *pn_spo2 = -999; // do not use SPO2 since valley loc is out of range
      *pch_spo2_valid  = 0; 
      return;
    }
  }
  // find max between two valley locations 
  // and use an_ratio betwen AC compoent of Ir & Red and DC compoent of Ir & Red for SPO2 
  for (k = 0; k < n_exact_ir_valley_locs_count-1; k++) {
    n_y_dc_max = -16777216; 
    n_x_dc_max = -16777216; 
    if (an_ir_valley_locs[k+1]-an_ir_valley_locs[k] >3) {
        for (i = an_ir_valley_locs[k]; i < an_ir_valley_locs[k+1]; i++) {
          if (an_ir[i] > n_x_dc_max) {n_x_dc_max = an_ir[i]; n_x_dc_max_idx = i;}
          if (an_red[i] > n_y_dc_max) {n_y_dc_max = an_red[i]; n_y_dc_max_idx = i;}
      }
      n_y_ac  = (an_red[an_ir_valley_locs[k+1]] - an_red[an_ir_valley_locs[k] ] )*(n_y_dc_max_idx -an_ir_valley_locs[k]); // red
      n_y_ac  = an_red[an_ir_valley_locs[k]] + n_y_ac/ (an_ir_valley_locs[k+1] - an_ir_valley_locs[k]) ; 
      n_y_ac  = an_red[n_y_dc_max_idx] - n_y_ac; // subracting linear DC compoenents from raw 
      n_x_ac  = (an_ir[an_ir_valley_locs[k+1]] - an_ir[an_ir_valley_locs[k] ] )*(n_x_dc_max_idx -an_ir_valley_locs[k]); // ir
      n_x_ac  = an_ir[an_ir_valley_locs[k]] + n_x_ac/ (an_ir_valley_locs[k+1] - an_ir_valley_locs[k]); 
      n_x_ac  = an_ir[n_y_dc_max_idx] - n_x_ac; // subracting linear DC compoenents from raw 
      n_nume  = (n_y_ac *n_x_dc_max)>>7; // prepare X100 to preserve floating value
      n_denom = (n_x_ac *n_y_dc_max)>>7;
      if (n_denom > 0 && n_i_ratio_count < 5 && n_nume != 0) {

        // formula is ( n_y_ac *n_x_dc_max) / ( n_x_ac *n_y_dc_max)
        an_ratio[n_i_ratio_count]= (n_nume*100)/n_denom; 
        n_i_ratio_count++;
      }
    }
  }

  // choose median value since PPG signal may varies from beat to beat
  max30102_spo2_sort_ascend(an_ratio, n_i_ratio_count);
  n_middle_idx = n_i_ratio_count/2;

  if (n_middle_idx > 1)
    n_ratio_average =( an_ratio[n_middle_idx-1] +an_ratio[n_middle_idx])/2; // use median
  else
    n_ratio_average = an_ratio[n_middle_idx ];

  if( n_ratio_average > 2 && n_ratio_average < 184) {
    n_spo2_calc = uch_spo2_table[n_ratio_average];
    *pn_spo2 = n_spo2_calc;
    *pch_spo2_valid  = 1;
  }

  // do not use SPO2 since signal an_ratio is out of range
  else {
    *pn_spo2 = -999;
    *pch_spo2_valid  = 0; 
  }
}

// Find at most MAX_NUM peaks above MIN_HEIGHT separated by at least MIN_DISTANCE
void max30102_spo2_find_peaks(
  int32_t* pn_locs, 
  int32_t* n_npks,
  int32_t* pn_x, 
  int32_t  n_size, 
  int32_t  n_min_height, 
  int32_t  n_min_distance, 
  int32_t  n_max_num 
) {
  max30102_spo2_peaks_above_min_height( pn_locs, n_npks, pn_x, n_size, n_min_height );
  max30102_spo2_remove_close_peaks( pn_locs, n_npks, pn_x, n_min_distance );

  // limit to max number of peaks
  *n_npks = *n_npks < n_max_num ? *n_npks : n_max_num;
}

// Find all peaks above MIN_HEIGHT
void max30102_spo2_peaks_above_min_height(
  int32_t* pn_locs, 
  int32_t* n_npks,
  int32_t* pn_x, 
  int32_t  n_size, 
  int32_t  n_min_height
) {
  int32_t i = 1, n_width;
  *n_npks = 0;
  
  while (i < n_size-1) {

    // find left edge of potential peaks
    if (pn_x[i] > n_min_height && pn_x[i] > pn_x[i-1]) {
      n_width = 1;

      // find flat peaks
      while (i+n_width < n_size && pn_x[i] == pn_x[i+n_width])
        n_width++;

      // find right edge of peaks
      if (pn_x[i] > pn_x[i+n_width] && (*n_npks) < 15 ) {
        pn_locs[(*n_npks)++] = i;    

        // for flat peaks, peak location is left edge
        i += n_width+1;
      }
      else
        i += n_width;
    }
    else
      i++;
  }
}

// Remove peaks separated by less than MIN_DISTANCE
void max30102_spo2_remove_close_peaks(
  int32_t* pn_locs, 
  int32_t* pn_npks, 
  int32_t* pn_x, 
  int32_t  n_min_distance
) {
  int32_t i, j, n_old_npks, n_dist;
    
  // Order peaks from large to small
  max30102_sort_indices_descend( pn_x, pn_locs, *pn_npks );

  for ( i = -1; i < *pn_npks; i++ ) {
    n_old_npks = *pn_npks;
    *pn_npks = i+1;
    for ( j = i+1; j < n_old_npks; j++ ) {

      // lag-zero peak of autocorr is at index -1
      n_dist =  pn_locs[j] - ( i == -1 ? -1 : pn_locs[i] );
      if ( n_dist > n_min_distance || n_dist < -n_min_distance )
        pn_locs[(*pn_npks)++] = pn_locs[j];
    }
  }

  // Resort indices int32_to ascending order
  max30102_spo2_sort_ascend( pn_locs, *pn_npks );
}

// Sort array in ascending order (insertion sort algorithm)
void max30102_spo2_sort_ascend(
  int32_t *pn_x, 
  int32_t  n_size
) {
  int32_t i, j, n_temp;
  for (i = 1; i < n_size; i++) {
    n_temp = pn_x[i];
    for (j = i; j > 0 && n_temp < pn_x[j-1]; j--)
        pn_x[j] = pn_x[j-1];
    pn_x[j] = n_temp;
  }
}

// Sort indices according to descending order (insertion sort algorithm)
void max30102_sort_indices_descend(
  int32_t* pn_x, 
  int32_t* pn_indx, 
  int32_t  n_size
) {
  int32_t i, j, n_temp;
  for (i = 1; i < n_size; i++) {
    n_temp = pn_indx[i];
    for (j = i; j > 0 && pn_x[n_temp] > pn_x[pn_indx[j-1]]; j--)
      pn_indx[j] = pn_indx[j-1];
    pn_indx[j] = n_temp;
  }
}