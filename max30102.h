#ifndef HEART_SENSOR_BPM_H
#define HEART_SENSOR_BPM_H

#include "common.h"

#define MAX30102_I2C_ADDR     0x57
#define MAX30102_I2C_BUFF_LEN 32
#define MAX30102_HR_STORE_SZ  4
#define MAX30102_SAMP_FREQ    25
#define MAX30102_BUFF_SZ      (MAX30102_SAMP_FREQ * 4) 
#define MAX30102_MA4_SZ       4

typedef struct 
{
  uint32_t red[MAX30102_HR_STORE_SZ];
  uint32_t ir [MAX30102_HR_STORE_SZ];
  uint8_t  head;
  uint8_t  tail;
} sense_struct;

typedef struct {
  int16_t hr_ir_ac_max;
  int16_t hr_ir_ac_min;
  int16_t hr_ir_ac_sig_curr;
  int16_t hr_ir_ac_sig_prev;
  int16_t hr_ir_ac_sig_max;
  int16_t hr_ir_ac_sig_min;
  int16_t hr_ir_avg_est;
  int16_t hr_pos_edge;
  int16_t hr_neg_edge;
  int32_t hr_ir_avg_reg;
  int16_t hr_cbuf[32];
  uint8_t hr_offset;
} hr_signal_t;

void max30102_init();
void max30102_off();
void max30102_on();
uint16_t max30102_hr_check(sense_struct* sense);
bool max30102_hr_safe_check(uint8_t timeout, sense_struct *sense);
uint8_t max30102_avail(sense_struct *sense);
void max30102_next_sample(sense_struct *sense);
uint32_t max30102_get_red(sense_struct *sense);
uint32_t max30102_get_ir(sense_struct *sense);
void max30102_read_fifo(uint32_t *red, uint32_t *irValue);
bool max30102_hr_check_for_beat(int32_t sample, hr_signal_t* hr_signal, const uint16_t fir_coeffs[12]);
void max30102_read_spo2(uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, int32_t *pn_spo2, int8_t *pch_spo2_valid);
void max30102_spo2_find_peaks(int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, int32_t n_size, int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num);
void max30102_spo2_peaks_above_min_height(int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, int32_t n_size, int32_t n_min_height);
void max30102_spo2_remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_min_distance);
void max30102_spo2_sort_ascend(int32_t  *pn_x, int32_t n_size);
void max30102_sort_indices_descend(int32_t  *pn_x, int32_t *pn_indx, int32_t n_size);

#endif
