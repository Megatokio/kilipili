#pragma once


// ==== CONFIG ====================================================

// pull in board description file for targeted board:
#ifndef MAKE_TOOLS
  #include <pico/config.h>
#elif defined PICO_BOARD
  #include PICO_BOARD
#endif


// convert vgaboard-style #defines
#ifndef VIDEO_COLOR_PIN_BASE

  #ifndef PICO_SCANVIDEO_COLOR_PIN_COUNT
	#define PICO_SCANVIDEO_COLOR_PIN_COUNT 16
  #endif
  #ifndef PICO_SCANVIDEO_PIXEL_RSHIFT
	#define PICO_SCANVIDEO_PIXEL_RSHIFT 0u
  #endif
  #ifndef PICO_SCANVIDEO_PIXEL_GSHIFT
	#define PICO_SCANVIDEO_PIXEL_GSHIFT 6u
  #endif
  #ifndef PICO_SCANVIDEO_PIXEL_BSHIFT
	#define PICO_SCANVIDEO_PIXEL_BSHIFT 11u
  #endif
  #ifndef PICO_SCANVIDEO_PIXEL_RCOUNT
	#define PICO_SCANVIDEO_PIXEL_RCOUNT 5
  #endif
  #ifndef PICO_SCANVIDEO_PIXEL_GCOUNT
	#define PICO_SCANVIDEO_PIXEL_GCOUNT 5
  #endif
  #ifndef PICO_SCANVIDEO_PIXEL_BCOUNT
	#define PICO_SCANVIDEO_PIXEL_BCOUNT 5
  #endif

  #define VIDEO_COLOR_PIN_COUNT PICO_SCANVIDEO_COLOR_PIN_COUNT
  #define VIDEO_PIXEL_RSHIFT	PICO_SCANVIDEO_PIXEL_RSHIFT
  #define VIDEO_PIXEL_GSHIFT	PICO_SCANVIDEO_PIXEL_GSHIFT
  #define VIDEO_PIXEL_BSHIFT	PICO_SCANVIDEO_PIXEL_BSHIFT
  #define VIDEO_PIXEL_RCOUNT	PICO_SCANVIDEO_PIXEL_RCOUNT
  #define VIDEO_PIXEL_GCOUNT	PICO_SCANVIDEO_PIXEL_GCOUNT
  #define VIDEO_PIXEL_BCOUNT	PICO_SCANVIDEO_PIXEL_BCOUNT
#endif

#ifndef VIDEO_PIXEL_ISHIFT
  #define VIDEO_PIXEL_ISHIFT 0
  #define VIDEO_PIXEL_ICOUNT 0
#endif

#ifndef VIDEO_PIXEL_RSHIFT
  #define VIDEO_PIXEL_RSHIFT 0
  #define VIDEO_PIXEL_GSHIFT 0
  #define VIDEO_PIXEL_BSHIFT 0
  #define VIDEO_PIXEL_RCOUNT 0
  #define VIDEO_PIXEL_GCOUNT 0
  #define VIDEO_PIXEL_BCOUNT 0
#endif


// check minimal plausibility:
static_assert(VIDEO_PIXEL_RSHIFT + VIDEO_PIXEL_RCOUNT <= VIDEO_COLOR_PIN_COUNT);
static_assert(VIDEO_PIXEL_GSHIFT + VIDEO_PIXEL_GCOUNT <= VIDEO_COLOR_PIN_COUNT);
static_assert(VIDEO_PIXEL_BSHIFT + VIDEO_PIXEL_BCOUNT <= VIDEO_COLOR_PIN_COUNT);
static_assert(VIDEO_PIXEL_ISHIFT + VIDEO_PIXEL_ICOUNT <= VIDEO_COLOR_PIN_COUNT);
