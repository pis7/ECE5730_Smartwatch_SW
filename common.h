#ifndef COMMON_H
#define COMMON_H

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

// SPI configurations
#define PIN_SCK  2
#define PIN_MOSI 3
#define PIN_MISO 4
#define PIN_CS   5
#define SPI_PORT spi0

// I2C configurations and addresses
#define I2C_CHAN i2c0
#define SDA_PIN 0
#define SCL_PIN 1
#define I2C_BAUD_RATE 400000 // 400kHz
#define I3G4250D_ADDR 0x68
#define SSH1106_I2C_ADDR 0x3C

#endif
