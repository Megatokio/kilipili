// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once

// pull in board description file for targeted board:
#ifndef MAKE_TOOLS
  #include <pico/config.h>
#else
  #define PICO_AUDIO_PWM
  #define PICO_AUDIO_LEFT_PIN  1
  #define PICO_AUDIO_RIGHT_PIN 2
#endif


#if defined PICO_AUDIO_BUZZER_PIN
  #define PICO_AUDIO_BUZZER
  #define audio_buzzer_pin PICO_AUDIO_BUZZER_PIN
#elif defined PICO_AUDIO_PWM_L_PIN && defined PICO_AUDIO_PWM_R_PIN
  #define PICO_AUDIO_PWM
  #define audio_left_pin  PICO_AUDIO_PWM_L_PIN
  #define audio_right_pin PICO_AUDIO_PWM_R_PIN
#elif defined PICO_AUDIO_I2S_DATA_PIN && defined PICO_AUDIO_I2S_CLOCK_PIN_BASE
  #define PICO_AUDIO_I2S
  #define audio_i2s_data_pin	   PICO_AUDIO_I2S_DATA_PIN
  #define audio_i2s_clock_pin_base PICO_AUDIO_I2S_CLOCK_PIN_BASE
#elif defined PICO_AUDIO_BUZZER
  #error "pin for buzzer not defined"
#elif defined PICO_AUDIO_I2S
  #error "i2s pins not defined"
#elif defined PICO_AUDIO_PWM
#elif defined PICO_AUDIO_SIGMA_DELTA
#else
  #define PICO_AUDIO_NONE
#endif

#if defined PICO_AUDIO_LEFT_PIN != defined PICO_AUDIO_RIGHT_PIN
  #error "left or right audio pin not defined"
#endif
#if defined PICO_AUDIO_LEFT_PIN && defined PICO_AUDIO_MONO_PIN
  #error "either mono or left and right audio pin must be defined"
#endif

#ifdef PICO_AUDIO_LEFT_PIN
  #define audio_left_pin PICO_AUDIO_LEFT_PIN
#endif
#ifdef PICO_AUDIO_RIGHT_PIN
  #define audio_right_pin PICO_AUDIO_RIGHT_PIN
#endif
#ifdef PICO_AUDIO_MONO_PIN
  #define audio_left_pin PICO_AUDIO_MONO_PIN
#endif


#ifdef PICO_AUDIO_NONE
  #define audio_hw				NONE
  #define audio_hw_num_channels 0
  #define audio_hw_sample_size	0
#elif defined PICO_AUDIO_BUZZER
  #define audio_hw				BUZZER
  #define audio_hw_num_channels 0
  #define audio_hw_sample_size	0
#elif defined PICO_AUDIO_I2S
  #define audio_hw				I2S
  #define audio_hw_num_channels 2
  #define audio_hw_sample_size	2 // sizeof(int16)
#elif defined PICO_AUDIO_PWM
  #define audio_hw				PWM
  #define audio_hw_num_channels 1
  #define audio_hw_sample_size	4 // sizeof(uint32)
#elif defined PICO_AUDIO_SIGMA_DELTA
  #define audio_hw				SIGMA_DELTA
  #define audio_hw_num_channels 1
  #define audio_hw_sample_size	1 // sizeof(int8)
#else
  #error "PICO_AUDIO_XXX not defined"
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
#ifndef AUDIO_DMA_BUFFER_NUM_FRAMES
  #define AUDIO_DMA_BUFFER_NUM_FRAMES 256 // ~ 5ms
#endif


/*

































*/
