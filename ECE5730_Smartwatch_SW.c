/**
 * Parker Schless (pis7), Katarina Duric (kd374), George Maidhof (gpm58)
 * 
 * Smartwatch Code
 *
 */

// Include standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

// Include PICO libraries
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/time.h"

// Include hardware libraries
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

// Include custom libraries
#include "pt_cornell_rp2040_v1_3.h"
#include "font_ssh1106.h"
#include "ssh1106.h"

// Include custom images
#include "dat/battery_img.h"
#include "dat/phone_img.h"
#include "dat/heartrate_img.h"

// Some macros for max/min/abs
#define min(a,b) ((a<b) ? a:b)
#define max(a,b) ((a<b) ? b:a)
#define abs(a) ((a>0) ? a:-a)

// I2C Config
#define I2C_CHAN i2c0
#define I2C_BAUD_RATE 400000 // 400kHz
#define SDA_PIN 0
#define SCL_PIN 1

// SPI Config
#define PIN_MISO 4
#define PIN_CS   5
#define PIN_SCK  6
#define PIN_MOSI 7
#define SPI_PORT spi0

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

typedef enum {
  MM_TIME,
  MM_PHONE,
  MM_HEART_RATE,
  MM_BATTERY
} main_menu_state_t;
main_menu_state_t main_menu_state = MM_TIME;

typedef enum {
  DB_NOT_PRESSED,
  DB_MAYBE_PRESSED,
  DB_PRESSED,
  DB_MAYBE_NOT_PRESSED
} debounce_state_t;
debounce_state_t cycle_button_state = DB_NOT_PRESSED;
debounce_state_t select_button_state = DB_NOT_PRESSED;

void update_main_menu() {
  int pressed = gpio_get(CYCLE_BUTTON);
  switch (cycle_button_state) {
    case DB_NOT_PRESSED:
      if (pressed) cycle_button_state = DB_MAYBE_PRESSED;
      else cycle_button_state = DB_NOT_PRESSED;
      break;
    case DB_MAYBE_PRESSED:
      if (pressed) {
        main_menu_state = (main_menu_state == MM_BATTERY) ? MM_TIME : (main_menu_state + 1);
        SSH1106_Clear();
        cycle_button_state = DB_PRESSED;
      }
      else cycle_button_state = DB_NOT_PRESSED;
      break;
    case DB_PRESSED:
      if (pressed) cycle_button_state = DB_PRESSED;
      else cycle_button_state = DB_MAYBE_NOT_PRESSED;
      break;
    case DB_MAYBE_NOT_PRESSED:
      if (pressed) cycle_button_state = DB_PRESSED;
      else cycle_button_state = DB_NOT_PRESSED;
      break;
    default:
      cycle_button_state = DB_NOT_PRESSED;
      break;
  }
}

int map_batt(int input, int input_min, int input_max, int output_min, int output_max) {
  int output = (int)((float)(input - input_min) / (float)(input_max - input_min) * (float)(output_max - output_min) + (float)output_min);
  output = ((output + 5) / 10) * 10;
  return output;
}

int main() {

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

  char battery_str[20];

  while (true) {

    update_main_menu();
    switch (main_menu_state) {
      case MM_TIME:
        sprintf(prev_time_str, "%s", time_str);
        uptime_ms = time_us_64() / 1000;
        hours = (uptime_ms / (1000 * 60 * 60)) % 24;
        minutes = (uptime_ms / (1000 * 60)) % 60;
        seconds = (uptime_ms / 1000) % 60;
        sprintf(time_str, "%02u:%02u:%02u", hours, minutes, seconds);
        if (seconds != prev_seconds) {
          prev_seconds = seconds;
          SSH1106_GotoXY(25, 25);
          SSH1106_Puts(prev_time_str, &Font_11x18, 0);
          SSH1106_GotoXY(25, 25);
          SSH1106_Puts(time_str, &Font_11x18, 1);
          SSH1106_UpdateScreen();
        }
        break;
      case MM_PHONE:
        for (int i = 0; i < sizeof(phone_img)/sizeof(phone_img[0]); i++)
          SSH1106_DrawPixel(phone_img[i][0], phone_img[i][1], 1);
        SSH1106_UpdateScreen();
        break;
      case MM_HEART_RATE:
        for (int i = 0; i < sizeof(heartrate_img)/sizeof(heartrate_img[0]); i++)
          SSH1106_DrawPixel(heartrate_img[i][0], heartrate_img[i][1], 1);
        SSH1106_UpdateScreen();
      break;
      case MM_BATTERY:
        for (int i = 0; i < sizeof(battery_img)/sizeof(battery_img[0]); i++)
          SSH1106_DrawPixel(battery_img[i][0], battery_img[i][1], 1);
        sprintf(battery_str, "%02d%%", map_batt(adc_read(), ADC_MIN, ADC_MAX, BATT_MIN, BATT_MAX));
        SSH1106_GotoXY(50, 40);
        SSH1106_Puts(battery_str, &Font_11x18, 1);
        SSH1106_UpdateScreen();
        break;
    }
  }
}
