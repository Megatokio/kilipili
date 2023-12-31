// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_PICOMITE_H
#define _BOARDS_PICOMITE_H

// For board detection
#define PICOMITE

// Serial
#define PICO_DEFAULT_UART 0
#define PICO_DEFAULT_UART_TX_PIN 0
#define PICO_DEFAULT_UART_RX_PIN 1

// Video
#define VIDEO_COLOR_PIN_BASE  18
#define VIDEO_COLOR_PIN_COUNT 4
#define VIDEO_SYNC_PIN_BASE   16

#define VIDEO_PIXEL_RSHIFT 3
#define VIDEO_PIXEL_GSHIFT 1
#define VIDEO_PIXEL_BSHIFT 0
#define VIDEO_PIXEL_RCOUNT 1
#define VIDEO_PIXEL_GCOUNT 2
#define VIDEO_PIXEL_BCOUNT 1

// Audio
#define PICO_AUDIO_LEFT_CHANNEL  26 // actually no dedicated pins for audio or a beep
#define PICO_AUDIO_RIGHT_CHANNEL 27

// SDCard
#define PICO_DEFAULT_SPI 1
#define PICO_DEFAULT_SPI_SCK_PIN 10
#define PICO_DEFAULT_SPI_TX_PIN  11
#define PICO_DEFAULT_SPI_RX_PIN  12
#define PICO_DEFAULT_SPI_CSN_PIN 13

// I2C
#define PICO_DEFAULT_I2C 1
#define PICO_DEFAULT_I2C_SDA_PIN 4
#define PICO_DEFAULT_I2C_SCL_PIN 5

#define PICO_SECOND_I2C 0
#define PICO_SECOND_I2C_SDA_PIN 6
#define PICO_SECOND_I2C_SCL_PIN 7

// PS2 keyboards
#define PICO_PS2_CLK  8
#define PICO_PS2_DATA 9

// pull in Pico defaults
#include "boards/pico.h"


#endif
