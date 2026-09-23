// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef KIBOARD_H
#define KIBOARD_H

// Serial
#define PICO_DEFAULT_UART		 1
#define PICO_DEFAULT_UART_TX_PIN 20
#define PICO_DEFAULT_UART_RX_PIN 21

// Video
#define VIDEO_COLOR_PIN_BASE  2
#define VIDEO_COLOR_PIN_COUNT 12
#define VIDEO_SYNC_PIN_BASE	  14

#define VIDEO_PIXEL_RSHIFT 0u
#define VIDEO_PIXEL_GSHIFT 4u
#define VIDEO_PIXEL_BSHIFT 8u
#define VIDEO_PIXEL_RCOUNT 4
#define VIDEO_PIXEL_GCOUNT 4
#define VIDEO_PIXEL_BCOUNT 4

// Audio

// #define AUDIO_NONE

// #define AUDIO_BUZZER
// #define AUDIO_BUZZER_PIN

// #define AUDIO_I2S
// #define AUDIO_I2S_DATA_PIN
// #define AUDIO_I2S_CLOCK_PIN_BASE

// #define AUDIO_PWM
// #define AUDIO_MONO_PIN 28
// #define AUDIO_LEFT_PIN  28
// #define AUDIO_RIGHT_PIN 26

// #define AUDIO_SIGMA_DELTA
// #define AUDIO_MONO_PIN 28
// #define AUDIO_LEFT_PIN  28
// #define AUDIO_RIGHT_PIN 26

#define AUDIO_SIGMA_DELTA
#define AUDIO_LEFT_PIN	28
#define AUDIO_RIGHT_PIN 26

// SDCard

// If you have one or more sdcard connectors on your board
// #define SDCARD_SPI and the RX, CLK, TX and CS pins.
// If you have two card connectors also define CS2 pin.
// 2 or more connectors must be connected to the same SPI bus.

#define SDCARD_SPI
#define SDCARD_SPI_RX_PIN  16
#define SDCARD_SPI_CS_PIN  17
#define SDCARD_SPI_CLK_PIN 18
#define SDCARD_SPI_TX_PIN  19
#define SDCARD_SPI_CLOCK   20000000
//#define SDCARD_SPI_CS2_PIN  9

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
