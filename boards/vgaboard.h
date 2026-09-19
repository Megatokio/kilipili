// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#ifndef VGABOARD_H
#define VGABOARD_H

// For board detection
// we cannot have enough of them:
#define RASPBERRYPI_VGABOARD
#define PICO_VGA_BOARD

// Serial
// UART TX/RX are shared with SDCard DAT1/2
// pull jumpers off header to disconnect DAT1/2 and access UART pins
#define PICO_DEFAULT_UART		 1
#define PICO_DEFAULT_UART_TX_PIN 20
#define PICO_DEFAULT_UART_RX_PIN 21

// Video
#define VIDEO_COLOR_PIN_BASE  0
#define VIDEO_COLOR_PIN_COUNT 16
#define VIDEO_SYNC_PIN_BASE	  16

#define VIDEO_PIXEL_RSHIFT 0u
#define VIDEO_PIXEL_GSHIFT 6u
#define VIDEO_PIXEL_BSHIFT 11u
#define VIDEO_PIXEL_RCOUNT 5
#define VIDEO_PIXEL_GCOUNT 5
#define VIDEO_PIXEL_BCOUNT 5

// No Audio:
// use vgaboard-pwm or vgaboard-i2s for sound!

// SDCard
// the spi bus is connected to unusual pins and will use pio mode.
#define SDCARD_SPI
#define SDCARD_SPI_RX_PIN  19 // PICO_SD_DAT0_PIN
#define SDCARD_SPI_CS_PIN  22 // PICO_SD_DAT3_PIN
#define SDCARD_SPI_CLK_PIN 5  // PICO_SD_CLK_PIN
#define SDCARD_SPI_TX_PIN  18 // PICO_SD_CMD_PIN
#define SDCARD_SPI_CLOCK   15000000

// SDC	mSD	 sdcard	  spi	pico
// 8:	8	 Dat1	  -		20
// 7:	7	 Dat0	  DO	19
// 6:	6	 Vss
// 5:	5	 Clk	  CLK	5
// 4:	4	 Vcc
// 3:	-	 Vss
// 2:	3	 Cmd	  DI	18
// 1:	2	 Dat3/CD  CS	22
// 9:	1	 Dat2	  -		21

// buttons are shared with VGA colour LSBs -- if using VGA, you can float
// the pin on VSYNC assertion and sample on VSYNC deassertion
#define VGABOARD_BUTTON_A_PIN 0
#define VGABOARD_BUTTON_B_PIN 6
#define VGABOARD_BUTTON_C_PIN 11

// pull in Pico defaults
#include "boards/pico.h"

#endif
