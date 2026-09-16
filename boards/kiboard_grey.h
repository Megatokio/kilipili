// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


// for testing only: grey image only emitted on the green channel


#ifndef KIBOARD
#define KIBOARD

// Serial
#define PICO_DEFAULT_UART		 1
#define PICO_DEFAULT_UART_TX_PIN 20
#define PICO_DEFAULT_UART_RX_PIN 21

// Video
#define VIDEO_COLOR_PIN_BASE  6
#define VIDEO_COLOR_PIN_COUNT 4
#define VIDEO_SYNC_PIN_BASE	  14

#define VIDEO_PIXEL_ISHIFT 0
#define VIDEO_PIXEL_ICOUNT VIDEO_COLOR_PIN_COUNT

// Audio
#define AUDIO_SIGMA_DELTA
#define AUDIO_LEFT_PIN	28
#define AUDIO_RIGHT_PIN 26

// SDCard
#define SDCARD_SPI		   0
#define SDCARD_SPI_RX_PIN  16
#define SDCARD_SPI_CS_PIN  17
#define SDCARD_SPI_CLK_PIN 18
#define SDCARD_SPI_TX_PIN  19
#define SDCARD_SPI_CLOCK   20000000

// I2C
#define PICO_DEFAULT_I2C		 1
#define PICO_DEFAULT_I2C_SDA_PIN 22
#define PICO_DEFAULT_I2C_SCL_PIN 27

#define PICO_SECOND_I2C			0
#define PICO_SECOND_I2C_SDA_PIN 0
#define PICO_SECOND_I2C_SCL_PIN 1

// pull in Pico defaults
#include "boards/pico.h"

#endif
