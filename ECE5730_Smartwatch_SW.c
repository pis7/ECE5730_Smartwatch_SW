/**
 * Parker Schless (pis7), Katarina Duric (kd374), George Maidhof (gpm58)
 *
 * Smartwatch Code
 *
 */

#include "common.h"

// Include custom libraries
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
// #include "pt_cornell_rp2040_v1_3.h"
#include "font_ssh1106.h"
#include "ssh1106.h"
#include "i3g4250d.h"

// Include custom images
#include "dat/battery_img.h"
#include "dat/phone_img.h"
#include "dat/heartrate_img.h"
#include "dat/activity_img.h"
#include "dat/weather_img.h"

// Wifi library
#include "wifi_udp.h"

// Heart rate library
#include "heart_sensor_bpm.h" 

// Some macros for max/min/abs
#define min(a, b) ((a < b) ? a : b)
#define max(a, b) ((a < b) ? b : a)
#define abs(a) ((a > 0) ? a : -a)

// GPIO Config
#define CYCLE_BUTTON 14
#define SELECT_BUTTON 15

// ADC Config
#define ADC_CHAN 0
#define ADC_PIN 26

// Battery values
#define ADC_MAX 2800
#define ADC_MIN 2050
#define BATT_MIN 0
#define BATT_MAX 100

// Direct Digital Synthesis (DDS) parameters for DAC
#define two32 4294967296.0 // 2^32 (a constant)
#define Fs 50000
#define DELAY 20 // 1/Fs (in microseconds)
#define SOUND_DURATION 25000
#define ALARM_NUM 0
#define ALARM_IRQ TIMER_IRQ_0
#define DAC_config_chan_A 0b0001000000000000
#define sine_table_size 256
volatile unsigned int STATE_0 = 0;
volatile unsigned int count_0 = 0;
volatile unsigned int phase_accum_main_0;
volatile unsigned int phase_incr_main_0 = (400.0 * two32) / Fs;
volatile unsigned int sound_curr_freq = 0;
float current_amplitude_0 = 1.0;
float sin_table[sine_table_size];
int DAC_output_0;
uint16_t DAC_data_0;
int twinkle_twinkle[] = {261, 261, 392, 392, 440, 440, 392, 349, 349, 329, 329, 293, 293, 261};
int tt_idx = 0;

// Heart rate variables
#define RATE_SIZE 4
float rates[RATE_SIZE];
uint8_t rateSpot = 0;
uint32_t lastBeat = 0;

int16_t IR_AC_Max = 20;
int16_t IR_AC_Min = -20;

int16_t IR_AC_Signal_Current = 0;
int16_t IR_AC_Signal_Previous;
int16_t IR_AC_Signal_min = 0;
int16_t IR_AC_Signal_max = 0;
int16_t IR_Average_Estimated;

int16_t positiveEdge = 0;
int16_t negativeEdge = 0;
int32_t ir_avg_reg = 0;

int16_t cbuf[32];
uint8_t offset = 0;
static const uint16_t FIRCoeffs[12] = {172, 321, 579, 927, 1360, 1858, 2390, 2916, 3391, 3768, 4012, 4096};

typedef enum
{
  MM_TIME,
  MM_PHONE,
  MM_WEATHER,
  MM_HEART_RATE,
  MM_ACTIVITY,
  MM_BATTERY
} main_menu_state_t;
main_menu_state_t main_menu_state = MM_TIME;

typedef enum
{
  DB_NOT_PRESSED,
  DB_MAYBE_PRESSED,
  DB_PRESSED,
  DB_MAYBE_NOT_PRESSED
} debounce_state_t;
debounce_state_t select_button_state = DB_NOT_PRESSED;
debounce_state_t cycle_button_state = DB_NOT_PRESSED;
int in_sub_menu = 0;

static void alarm_irq(void)
{
  hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
  timer_hw->alarm[ALARM_NUM] = timer_hw->timerawl + DELAY;
  if (STATE_0 == 0)
  {
    if (main_menu_state == MM_PHONE && in_sub_menu)
    {
      sound_curr_freq = twinkle_twinkle[tt_idx];
      phase_incr_main_0 = (sound_curr_freq * two32) / Fs;
      phase_accum_main_0 += phase_incr_main_0;
      DAC_output_0 = (int)(current_amplitude_0 *
                           sin_table[phase_accum_main_0 >> 24]) +
                     2048;
      DAC_data_0 = (DAC_config_chan_A | (DAC_output_0 & 0xffff));
      spi_write16_blocking(SPI_PORT, &DAC_data_0, 1);
      count_0 += 1;
      if (count_0 == SOUND_DURATION)
      {
        STATE_0 = 1;
        count_0 = 0;
        tt_idx = (tt_idx + 1) % (sizeof(twinkle_twinkle) / sizeof(twinkle_twinkle[0]));
      }
    }
  }
  else
  {
    count_0 += 1;
    if (count_0 == SOUND_DURATION)
    {
      phase_accum_main_0 = 0;
      STATE_0 = 0;
      count_0 = 0;
    }
  }
}

void update_menu()
{
  int sel_pressed = gpio_get(SELECT_BUTTON);
  switch (select_button_state)
  {
  case DB_NOT_PRESSED:
    if (sel_pressed)
      select_button_state = DB_MAYBE_PRESSED;
    else
      select_button_state = DB_NOT_PRESSED;
    break;
  case DB_MAYBE_PRESSED:
    if (sel_pressed)
    {
      if (main_menu_state == MM_PHONE || main_menu_state == MM_WEATHER || main_menu_state == MM_ACTIVITY || main_menu_state == MM_HEART_RATE)
      {
        in_sub_menu = 1;
        SSH1106_Clear();
      }
      select_button_state = DB_PRESSED;
    }
    else
      select_button_state = DB_NOT_PRESSED;
    break;
  case DB_PRESSED:
    if (sel_pressed)
      select_button_state = DB_PRESSED;
    else
      select_button_state = DB_MAYBE_NOT_PRESSED;
    break;
  case DB_MAYBE_NOT_PRESSED:
    if (sel_pressed)
      select_button_state = DB_PRESSED;
    else
      select_button_state = DB_NOT_PRESSED;
    break;
  default:
    select_button_state = DB_NOT_PRESSED;
    break;
  }

  int cyc_pressed = gpio_get(CYCLE_BUTTON);
  switch (cycle_button_state)
  {
  case DB_NOT_PRESSED:
    if (cyc_pressed)
      cycle_button_state = DB_MAYBE_PRESSED;
    else
      cycle_button_state = DB_NOT_PRESSED;
    break;
  case DB_MAYBE_PRESSED:
    if (cyc_pressed)
    {
      if (in_sub_menu)
        in_sub_menu = 0;
      else
        main_menu_state = (main_menu_state == MM_BATTERY) ? MM_TIME : (main_menu_state + 1);
      SSH1106_Clear();
      cycle_button_state = DB_PRESSED;
    }
    else
      cycle_button_state = DB_NOT_PRESSED;
    break;
  case DB_PRESSED:
    if (cyc_pressed)
      cycle_button_state = DB_PRESSED;
    else
      cycle_button_state = DB_MAYBE_NOT_PRESSED;
    break;
  case DB_MAYBE_NOT_PRESSED:
    if (cyc_pressed)
      cycle_button_state = DB_PRESSED;
    else
      cycle_button_state = DB_NOT_PRESSED;
    break;
  default:
    cycle_button_state = DB_NOT_PRESSED;
    break;
  }
}

int map_batt(int input, int input_min, int input_max, int output_min, int output_max)
{
  int output = (int)((float)(input - input_min) / (float)(input_max - input_min) * (float)(output_max - output_min) + (float)output_min);
  output = ((output + 5) / 10) * 10;
  return output;
}

int main()
{

  // Initialize standard I/O
  stdio_init_all();

  // Initialize I2C
  i2c_init(I2C_CHAN, I2C_BAUD_RATE);
  gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);

  // Initialize SPI
  spi_init(SPI_PORT, 20000000); // 20MHz
  spi_set_format(SPI_PORT, 16, 0, 0, 0);
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
  gpio_set_function(PIN_CS, GPIO_FUNC_SPI);

  // Initialize GPIO
  gpio_init(CYCLE_BUTTON);
  gpio_set_dir(CYCLE_BUTTON, GPIO_IN);
  gpio_pull_down(CYCLE_BUTTON);

  gpio_init(SELECT_BUTTON);
  gpio_set_dir(SELECT_BUTTON, GPIO_IN);
  gpio_pull_down(SELECT_BUTTON);

  // Initialize ADC
  adc_gpio_init(ADC_PIN);
  adc_init();
  adc_select_input(ADC_CHAN);

  // Initialize OLED display
  SSH1106_Init();

  // Initialize I3G4250D
  init_i3g4250();

  // Heart rate sensor initialization
  MAX3010x_Init();

  // Time variables
  uint64_t uptime_ms = 0;
  uint32_t hours = 0;
  uint32_t minutes = 0;
  uint32_t seconds = 0;
  uint32_t prev_seconds = 0;
  char prev_time_str[20];
  char time_str[20];
  sprintf(time_str, "%02u:%02u:%02u", hours, minutes, seconds);
  sprintf(prev_time_str, "%s", time_str);

  // Other strings
  char phone_str[20];
  char weather_str[20];
  char battery_str[20];
  char gyro_x_str[20];
  char gyro_y_str[20];
  char gyro_z_str[20];
  char hr_str[20];

  // Build the sine lookup table
  // scaled to produce values between 0 and 4096 (for 12-bit DAC)
  int ii;
  for (ii = 0; ii < sine_table_size; ii++)
  {
    sin_table[ii] = 2047.0 * sin((float)ii * 6.283 / (float)sine_table_size);
  }

  // Enable the interrupt for the alarm (we're using Alarm 0)
  hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
  // Associate an interrupt handler with the ALARM_IRQ
  irq_set_exclusive_handler(ALARM_IRQ, alarm_irq);
  // Enable the alarm interrupt
  irq_set_enabled(ALARM_IRQ, true);
  // Write the lower 32 bits of the target time to the alarm register, arming it.
  timer_hw->alarm[ALARM_NUM] = timer_hw->timerawl + DELAY;

  // Initialize the UDP server
  // wifi_udp_init();
  // start_wifi_threads();

  while (true)
  {

    update_menu();
    switch (main_menu_state)
    {
    case MM_TIME:
      sprintf(prev_time_str, "%s", time_str);
      uptime_ms = time_us_64() / 1000;
      hours = (uptime_ms / (1000 * 60 * 60)) % 24;
      minutes = (uptime_ms / (1000 * 60)) % 60;
      seconds = (uptime_ms / 1000) % 60;
      sprintf(time_str, "%02u:%02u:%02u", hours, minutes, seconds);
      if (seconds != prev_seconds)
      {
        prev_seconds = seconds;
        SSH1106_GotoXY(25, 25);
        SSH1106_Puts(prev_time_str, &Font_11x18, 0);
        SSH1106_GotoXY(25, 25);
        SSH1106_Puts(time_str, &Font_11x18, 1);
        SSH1106_UpdateScreen();
      }
      break;
    case MM_PHONE:
      if (in_sub_menu)
      {
        sprintf(phone_str, "Song!");
        SSH1106_GotoXY(40, 25);
        SSH1106_Puts(phone_str, &Font_11x18, 1);
      }
      else
      {
        for (int i = 0; i < sizeof(phone_img) / sizeof(phone_img[0]); i++)
          SSH1106_DrawPixel(phone_img[i][0], phone_img[i][1], 1);
      }
      SSH1106_UpdateScreen();
      break;
    case MM_WEATHER:
      if (in_sub_menu)
      {
        sprintf(weather_str, "Temp: %dC", read_temp());
        SSH1106_GotoXY(10, 25);
        SSH1106_Puts(weather_str, &Font_11x18, 1);
      }
      else
      {
        for (int i = 0; i < sizeof(weather_img) / sizeof(weather_img[0]); i++)
          SSH1106_DrawPixel(weather_img[i][0], weather_img[i][1], 1);
      }
      SSH1106_UpdateScreen();
      break;
    case MM_HEART_RATE:
      if (in_sub_menu)
      {

        SSH1106_GotoXY(10, 25);
        sprintf(hr_str, "BPM: %d", MAX3010x_ReadTemp());
        SSH1106_Puts(hr_str, &Font_11x18, 1);
        // uint32_t red, irValue;
        // MAX3010x_ReadFIFO(&red, &irValue);

        // if (checkForBeat(irValue, IR_AC_Max, IR_AC_Min,
        //                  IR_AC_Signal_Current, &IR_AC_Signal_Previous,
        //                  IR_AC_Signal_min, IR_AC_Signal_max, IR_Average_Estimated,
        //                  positiveEdge, negativeEdge, ir_avg_reg,
        //                  cbuf, offset, FIRCoeffs)) {
        //     uint32_t now = to_ms_since_boot(get_absolute_time());
        //     uint32_t delta = now - lastBeat;
        //     lastBeat = now;

        //     float beatsPerMinute = 60000.0f / delta;
        //     rates[rateSpot++] = beatsPerMinute;
        //     if (rateSpot >= RATE_SIZE) rateSpot = 0;

        //     float beatAvg = 0;
        //     for (int i = 0; i < RATE_SIZE; i++) 
        //         beatAvg += rates[i];
        //     beatAvg /= RATE_SIZE;

        //     SSH1106_GotoXY(10, 25);
        //     sprintf(hr_str, "BPM: %.2f", beatAvg);
        //     SSH1106_Puts(hr_str, &Font_11x18, 1);

        //     // printf("Beat detected! IR = %lu, BPM = %.2f\n", irValue, beatAvg);
        // }
        // sleep_ms(10);
      } else {
        for (int i = 0; i < sizeof(heartrate_img) / sizeof(heartrate_img[0]); i++)
          SSH1106_DrawPixel(heartrate_img[i][0], heartrate_img[i][1], 1);
      }
      SSH1106_UpdateScreen();
      break;
    case MM_ACTIVITY:
      if (in_sub_menu)
      {
        for (int i = 0; i < 128; i++)
          for (int j = 0; j < 64; j++)
            SSH1106_DrawPixel(i, j, SSH1106_COLOR_BLACK);
        sprintf(gyro_x_str, "X: %d", read_gyro_x());
        sprintf(gyro_y_str, "Y: %d", read_gyro_y());
        sprintf(gyro_z_str, "Z: %d", read_gyro_z());
        SSH1106_GotoXY(10, 6);
        SSH1106_Puts(gyro_x_str, &Font_11x18, 1);
        SSH1106_GotoXY(10, 25);
        SSH1106_Puts(gyro_y_str, &Font_11x18, 1);
        SSH1106_GotoXY(10, 44);
        SSH1106_Puts(gyro_z_str, &Font_11x18, 1);
      }
      else
      {
        for (int i = 0; i < sizeof(activity_img) / sizeof(activity_img[0]); i++)
          SSH1106_DrawPixel(activity_img[i][0], activity_img[i][1], 1);
      }
      SSH1106_UpdateScreen();
      break;
    case MM_BATTERY:
      for (int i = 0; i < sizeof(battery_img) / sizeof(battery_img[0]); i++)
        SSH1106_DrawPixel(battery_img[i][0], battery_img[i][1], 1);
      sprintf(battery_str, "%02d%%", map_batt(adc_read(), ADC_MIN, ADC_MAX, BATT_MIN, BATT_MAX));
      SSH1106_GotoXY(50, 40);
      SSH1106_Puts(battery_str, &Font_11x18, 1);
      SSH1106_UpdateScreen();
      break;
    }
  }
}
