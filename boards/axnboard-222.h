// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef AXNBOARD_222
#define AXNBOARD_222

// Serial
#define PICO_DEFAULT_UART		 0
#define PICO_DEFAULT_UART_TX_PIN 12
#define PICO_DEFAULT_UART_RX_PIN 13

#define PICO_SECOND_UART		1
#define PICO_SECOND_UART_TX_PIN 20
#define PICO_SECOND_UART_RX_PIN 21

// Video
#define VIDEO_COLOR_PIN_BASE  2
#define VIDEO_COLOR_PIN_COUNT 6
#define VIDEO_SYNC_PIN_BASE	  8

#define VIDEO_PIXEL_RSHIFT 0
#define VIDEO_PIXEL_GSHIFT 2
#define VIDEO_PIXEL_BSHIFT 4
#define VIDEO_PIXEL_RCOUNT 2
#define VIDEO_PIXEL_GCOUNT 2
#define VIDEO_PIXEL_BCOUNT 2

// Audio
#define PICO_AUDIO_SIGMA_DELTA
#define PICO_AUDIO_LEFT_PIN	 26
#define PICO_AUDIO_RIGHT_PIN 27

// SDCard
#define PICO_DEFAULT_SPI		 0
#define PICO_DEFAULT_SPI_RX_PIN	 16
#define PICO_DEFAULT_SPI_CSN_PIN 17
#define PICO_DEFAULT_SPI_SCK_PIN 18
#define PICO_DEFAULT_SPI_TX_PIN	 19

// I2C
#define PICO_DEFAULT_I2C		 1
#define PICO_DEFAULT_I2C_SDA_PIN 0
#define PICO_DEFAULT_I2C_SCL_PIN 1

// pull in Pico defaults
#include "boards/pico.h"

#endif
