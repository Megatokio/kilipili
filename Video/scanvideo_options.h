#pragma once


// ==== CONFIG ====================================================

// pull in board description file and defaults for vgaboard:
#include "Color.h"

//#ifndef PICO_SCANVIDEO_ALPHA_PIN
//#define PICO_SCANVIDEO_ALPHA_PIN 5u
//#endif
//
//#ifndef PICO_SCANVIDEO_PIXEL_RSHIFT
//#define PICO_SCANVIDEO_PIXEL_RSHIFT 0u
//#endif
//
//#ifndef PICO_SCANVIDEO_PIXEL_GSHIFT
//#define PICO_SCANVIDEO_PIXEL_GSHIFT 6u
//#endif
//
//#ifndef PICO_SCANVIDEO_PIXEL_BSHIFT
//#define PICO_SCANVIDEO_PIXEL_BSHIFT 11u
//#endif
//
//#ifndef PICO_SCANVIDEO_PIXEL_RCOUNT
//#define PICO_SCANVIDEO_PIXEL_RCOUNT 5
//#endif
//
//#ifndef PICO_SCANVIDEO_PIXEL_GCOUNT
//#define PICO_SCANVIDEO_PIXEL_GCOUNT 5
//#endif
//
//#ifndef PICO_SCANVIDEO_PIXEL_BCOUNT
//#define PICO_SCANVIDEO_PIXEL_BCOUNT 5
//#endif

#ifndef PICO_SCANVIDEO_COLOR_PIN_COUNT
  #define PICO_SCANVIDEO_COLOR_PIN_COUNT 16
#endif

#ifndef PICO_SCANVIDEO_COLOR_PIN_BASE // base gpio for color outputs
  #define PICO_SCANVIDEO_COLOR_PIN_BASE 0
#endif

#ifndef PICO_SCANVIDEO_SYNC_PIN_BASE // base gpio for sync pins
  #define PICO_SCANVIDEO_SYNC_PIN_BASE (PICO_SCANVIDEO_COLOR_PIN_BASE + PICO_SCANVIDEO_COLOR_PIN_COUNT)
#endif

#ifndef PICO_SCANVIDEO_ENABLE_CLOCK_PIN
  #define PICO_SCANVIDEO_ENABLE_CLOCK_PIN 0
#endif

#ifndef PICO_SCANVIDEO_CLOCK_POLARITY
  #define PICO_SCANVIDEO_CLOCK_POLARITY 0
#endif

#ifndef PICO_SCANVIDEO_ENABLE_DEN_PIN
  #define PICO_SCANVIDEO_ENABLE_DEN_PIN 0
#endif

#ifndef PICO_SCANVIDEO_DEN_POLARITY
  #define PICO_SCANVIDEO_DEN_POLARITY 0
#endif

#ifndef PICO_SCANVIDEO_SCANLINE_BUFFER_COUNT
  #define PICO_SCANVIDEO_SCANLINE_BUFFER_COUNT 8
#endif


// ==== CODE OPTIONS =================================================

#ifndef PICO_SCANVIDEO_ENABLE_VIDEO_RECOVERY
  #define PICO_SCANVIDEO_ENABLE_VIDEO_RECOVERY 0
#endif


// ==== CONFIG DMA =================================================

#ifndef PICO_SCANVIDEO_FIXED_FRAGMENT_DMA
  #define PICO_SCANVIDEO_FIXED_FRAGMENT_DMA 0
#endif


// ==== HARDWARE ASSIGNMENT =================================================

#define video_pio pio0 // there's no choice about this!

#define PICO_SCANVIDEO_SCANLINE_SM1 0u
#define PICO_SCANVIDEO_TIMING_SM	1u

#define PICO_SCANVIDEO_TIMING_DMA_CHANNEL	   0u // for timing SM
#define PICO_SCANVIDEO_SCANLINE_DMA_CHANNEL	   1u // for plane1
#define PICO_SCANVIDEO_SCANLINE_DMA_CB_CHANNEL 2u // if plane1 fragment dma
