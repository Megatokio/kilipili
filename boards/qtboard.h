// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#ifndef QTBOARD_H
#define QTBOARD_H

// Serial
#define PICO_DEFAULT_UART		 1
#define PICO_DEFAULT_UART_TX_PIN 20
#define PICO_DEFAULT_UART_RX_PIN 21

// Video
#define VIDEO_COLOR_PIN_BASE  0
#define VIDEO_COLOR_PIN_COUNT 24
#define VIDEO_SYNC_PIN_BASE	  24

// QRgb = #AARRGGBB
// openGL RGBA8888 = bytes R,G,B,A in memory or little endian #AABBGGRR  and AA = 0xFF !!
#define VIDEO_PIXEL_RSHIFT 16u
#define VIDEO_PIXEL_GSHIFT 8u
#define VIDEO_PIXEL_BSHIFT 0u
#define VIDEO_PIXEL_RCOUNT 8
#define VIDEO_PIXEL_GCOUNT 8
#define VIDEO_PIXEL_BCOUNT 8

// Audio
#define AUDIO_NONE

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

// SDCard

//#define SDCARD_SPI
//#define SDCARD_SPI_RX_PIN  16
//#define SDCARD_SPI_CS_PIN  17
//#define SDCARD_SPI_CLK_PIN 18
//#define SDCARD_SPI_TX_PIN  19
//#define SDCARD_SPI_CLOCK   20000000
//#define SDCARD_SPI_CS2_PIN  9

// I2C
//#define PICO_DEFAULT_I2C		 1
//#define PICO_DEFAULT_I2C_SDA_PIN 22
//#define PICO_DEFAULT_I2C_SCL_PIN 27
//
//#define PICO_SECOND_I2C			0
//#define PICO_SECOND_I2C_SDA_PIN 0
//#define PICO_SECOND_I2C_SCL_PIN 1

// pull in Pico defaults
//#include "boards/pico.h"

#endif
