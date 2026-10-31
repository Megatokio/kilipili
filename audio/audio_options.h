// Copyright (c) 2024 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once

// pull in board description file for targeted board:
#ifndef MAKE_TOOLS
  #include <pico.h>
#elif defined PICO_BOARD_H
  #define pico_board_cmake_set(...)
  #define pico_board_cmake_set_default(...)
  #include PICO_BOARD_H
#endif


// outdated names:
#if defined PICO_AUDIO_BUZZER || defined PICO_AUDIO_NONE || defined PICO_AUDIO_I2S || defined PICO_AUDIO_PWM || \
	defined PICO_AUDIO_SIGMA_DELTA || defined PICO_AUDIO_BUZZER_PIN || defined PICO_AUDIO_LEFT_PIN ||           \
	defined PICO_AUDIO_RIGHT_PIN || defined PICO_AUDIO_MONO_PIN
  #error "remove PICO_ from settings in board.h"
#endif


// allow vgaboard names:
#if defined PICO_AUDIO_I2S_DATA_PIN && !defined AUDIO_PWM
  #define AUDIO_I2S
  #define AUDIO_I2S_DATA_PIN PICO_AUDIO_I2S_DATA_PIN
#endif
#if defined PICO_AUDIO_I2S_CLOCK_PIN_BASE && !defined AUDIO_PWM
  #define AUDIO_I2S
  #define AUDIO_I2S_CLOCK_PIN_BASE PICO_AUDIO_I2S_CLOCK_PIN_BASE
#endif
#if defined PICO_AUDIO_PWM_L_PIN && !defined AUDIO_I2S
  #define AUDIO_PWM
  #define AUDIO_LEFT_PIN PICO_AUDIO_PWM_L_PIN
#endif
#if defined PICO_AUDIO_PWM_R_PIN && !defined AUDIO_I2S
  #define AUDIO_PWM
  #define AUDIO_RIGHT_PIN PICO_AUDIO_PWM_R_PIN
#endif


#if defined AUDIO_BUZZER_PIN
  #define AUDIO_BUZZER
  #define audio_buzzer_pin AUDIO_BUZZER_PIN

#elif defined AUDIO_I2S_DATA_PIN && defined AUDIO_I2S_CLOCK_PIN_BASE
  #define AUDIO_I2S
  #define audio_i2s_data_pin	   AUDIO_I2S_DATA_PIN
  #define audio_i2s_clock_pin_base AUDIO_I2S_CLOCK_PIN_BASE

#elif defined AUDIO_LEFT_PIN && AUDIO_RIGHT_PIN
  #define audio_left_pin  AUDIO_LEFT_PIN
  #define audio_right_pin AUDIO_RIGHT_PIN

#elif defined AUDIO_MONO_PIN
  #define audio_left_pin AUDIO_MONO_PIN

#elif defined AUDIO_BUZZER
  #error "definition of AUDIO_BUZZER_PIN is missing"

#elif defined AUDIO_I2S || defined AUDIO_I2S_DATA_PIN || defined AUDIO_I2S_CLOCK_PIN_BASE
  #error "error in definition of I2S audio pins"

#elif defined AUDIO_PWM || AUDIO_SIGMA_DELTA || defined AUDIO_LEFT_PIN || AUDIO_RIGHT_PIN
  #error "error in definition of audio pins"

#else
  #define AUDIO_NONE
#endif

#if defined audio_left_pin && !defined AUDIO_PWM && !defined AUDIO_SIGMA_DELTA
  #error "definition of AUDIO_PWM or AUDIO_SIGMA_DELTA is missing"
#endif

#if defined AUDIO_NONE + defined AUDIO_BUZZER + defined AUDIO_PWM + defined AUDIO_I2S + defined AUDIO_SIGMA_DELTA > 1
  #error "multiple audio settings detected"
#endif


#ifdef AUDIO_NONE
  #define audio_hw				NONE
  #define audio_hw_num_channels 0
  #define audio_hw_sample_size	0
#elif defined AUDIO_BUZZER
  #define audio_hw				BUZZER
  #define audio_hw_num_channels 0
  #define audio_hw_sample_size	0
#elif defined AUDIO_I2S
  #define audio_hw				I2S
  #define audio_hw_num_channels 2
  #define audio_hw_sample_size	2 // sizeof(int16)
#elif defined AUDIO_PWM
  #define audio_hw				PWM
  #define audio_hw_num_channels 1
  #define audio_hw_sample_size	4 // sizeof(uint32)
#elif defined AUDIO_SIGMA_DELTA
  #define audio_hw				SIGMA_DELTA
  #define audio_hw_num_channels 1
  #define audio_hw_sample_size	1 // sizeof(int8)
#else
  #error "AUDIO_XXX not defined"
#endif

#ifdef audio_right_pin
  #undef audio_hw_num_channels
  #define audio_hw_num_channels 2
#endif

#ifndef audio_buzzer_pin
  #define audio_buzzer_pin 0 // dummy
#endif

#ifndef audio_i2s_data_pin
  #define audio_i2s_data_pin	   0 // dummy
  #define audio_i2s_clock_pin_base 0 // dummy
#endif

#ifndef audio_right_pin
  #define audio_right_pin 0 // dummy
#endif

#ifndef audio_left_pin
  #define audio_left_pin 0 // dummy
#endif

#ifndef AUDIO_DEFAULT_SAMPLE_FREQUENCY
  #define AUDIO_DEFAULT_SAMPLE_FREQUENCY 44100 // exact if possible, coarsely approximated otherwise
#endif

// dma buffer size in number of audio frames (Sample[num_channels])
#ifndef AUDIO_DMA_BUFFER_SIZE
  #define AUDIO_DMA_BUFFER_SIZE 256 // ~ 5ms
#endif


/*

































*/
