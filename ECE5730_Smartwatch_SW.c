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

// Include hardware libraries
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"

// Include custom libraries
#include "pt_cornell_rp2040_v1_3.h"
#include "font_ssh1106.h"
#include "ssh1106.h"

// Some macros for max/min/abs
#define min(a,b) ((a<b) ? a:b)
#define max(a,b) ((a<b) ? b:a)
#define abs(a) ((a>0) ? a:-a)

// I2C Config
#define I2C_CHAN i2c0
#define I2C_BAUD_RATE 400000 // 400kHz
#define SDA_PIN 12
#define SCL_PIN 13

int main() {

  // Initialize standard I/O
  stdio_init_all();

  // Initialize I2C
  i2c_init(I2C_CHAN, I2C_BAUD_RATE);
  gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);

  // Initialize OLED display
  SSH1106_Init();

  SSH1106_GotoXY (32, 25);
  SSH1106_Puts ("Boats and", &Font_7x10, 1);
  SSH1106_GotoXY (32, 35);
  SSH1106_Puts ("fucking logs", &Font_7x10, 1);
  SSH1106_UpdateScreen();

  sleep_ms(200);

}
