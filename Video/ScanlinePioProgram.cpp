// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "ScanlinePioProgram.h"
#include "Color.h"
#include "composable_scanline.h"
#include "scanvideo_options.h"


namespace kipili::Video
{

using namespace Graphics;

#define PICO_SCANVIDEO_PLANE1_FRAGMENT_DMA \
  (PICO_SCANVIDEO_PLANE1_VARIABLE_FRAGMENT_DMA || PICO_SCANVIDEO_PLANE1_FIXED_FRAGMENT_DMA)
#define PICO_SCANVIDEO_PLANE2_FRAGMENT_DMA                                                      \
  ((PICO_SCANVIDEO_PLANE2_VARIABLE_FRAGMENT_DMA || PICO_SCANVIDEO_PLANE2_FIXED_FRAGMENT_DMA) && \
   PICO_SCANVIDEO_PLANE_COUNT >= 2)
#define PICO_SCANVIDEO_PLANE3_FRAGMENT_DMA                                                      \
  ((PICO_SCANVIDEO_PLANE3_VARIABLE_FRAGMENT_DMA || PICO_SCANVIDEO_PLANE3_FIXED_FRAGMENT_DMA) && \
   PICO_SCANVIDEO_PLANE_COUNT >= 3)
static_assert(!PICO_SCANVIDEO_PLANE3_FRAGMENT_DMA, "not implemented");

#ifndef PICO_SCANVIDEO_MISSING_SCANLINE_COLOR
  #define PICO_SCANVIDEO_MISSING_SCANLINE_COLOR bright_red
#endif


// ###########################################################
//				video_24mhz_composable
// ###########################################################

// the arrays here, though mostly const, are deliberately not flagged as const
// in order to allocate them in ram.
// this allows the video output to resume even if flash is inaccessible, e.g. while flashing.


static uint32 missing_scanline_data[] = {
	COMPOSABLE_COLOR_RUN | uint32(PICO_SCANVIDEO_MISSING_SCANLINE_COLOR << 16u),
	/*width-3*/ 0u | (COMPOSABLE_RAW_1P << 16u), black | (COMPOSABLE_EOL_ALIGN << 16u)};

#if 1 && PICO_SCANVIDEO_PLANE_COUNT >= 2
static uint32 missing_scanline_data_overlay[] = {
	// blank line
	(COMPOSABLE_EOL_SKIP_ALIGN) | (COMPOSABLE_EOL_ALIGN << 16)};
#endif

#if PICO_SCANVIDEO_PLANE1_VARIABLE_FRAGMENT_DMA || PICO_SCANVIDEO_PLANE2_VARIABLE_FRAGMENT_DMA || \
	PICO_SCANVIDEO_PLANE3_VARIABLE_FRAGMENT_DMA
static uint32 variable_fragment_missing_scanline_data_chain[] = {
	count_of(missing_scanline_data),
	reinterpret_cast<uint32>(missing_scanline_data),
	0,
	0,
};
#endif

#if PICO_SCANVIDEO_PLANE1_FIXED_FRAGMENT_DMA || PICO_SCANVIDEO_PLANE2_FIXED_FRAGMENT_DMA || \
	PICO_SCANVIDEO_PLANE3_FIXED_FRAGMENT_DMA
static uint32 fixed_fragment_missing_scanline_data_chain[] = {
	reinterpret_cast<uint32>(missing_scanline_data),
	0,
};
#endif

static Scanline video_24mhz_composable_missing_scanline
{
#if PICO_SCANVIDEO_PLANE1_FIXED_FRAGMENT_DMA || PICO_SCANVIDEO_PLANE2_FIXED_FRAGMENT_DMA || \
	PICO_SCANVIDEO_PLANE3_FIXED_FRAGMENT_DMA
	.fragment_words = 0,
#endif

	.data = {
#if PICO_SCANVIDEO_PLANE1_VARIABLE_FRAGMENT_DMA
		variable_fragment_missing_scanline_data_chain,
#elif PICO_SCANVIDEO_PLANE1_FIXED_FRAGMENT_DMA
		fixed_fragment_missing_scanline_data_chain,
#else
		{.data = missing_scanline_data, .used = count_of(missing_scanline_data)},
#endif

#if PICO_SCANVIDEO_PLANE_COUNT >= 2
  #if PICO_SCANVIDEO_PLANE2_VARIABLE_FRAGMENT_DMA
		variable_fragment_missing_scanline_data_chain,
  #elif PICO_SCANVIDEO_PLANE2_VARIABLE_FRAGMENT_DMA
		fixed_fragment_missing_scanline_data_chain,
  #else
		{.data = missing_scanline_data_overlay, .used = count_of(missing_scanline_data_overlay)},
  #endif
#endif

#if PICO_SCANVIDEO_PLANE_COUNT >= 3
  #if PICO_SCANVIDEO_PLANE3_VARIABLE_FRAGMENT_DMA
		variable_fragment_missing_scanline_data_chain,
  #elif PICO_SCANVIDEO_PLANE3_VARIABLE_FRAGMENT_DMA
		fixed_fragment_missing_scanline_data_chain,
  #else
		{.data = missing_scanline_data_overlay, .used = count_of(missing_scanline_data_overlay)},
  #endif
#endif
	}
};


#define video_24mhz_composable_program __CONCAT(video_24mhz_composable_prefix, _program)
//#define video_24mhz_composable_wrap_target __CONCAT(video_24mhz_composable_prefix, _wrap_target)
//#define video_24mhz_composable_wrap __CONCAT(video_24mhz_composable_prefix, _wrap)


void ScanlinePioProgram::adapt_for_mode(const VgaMode* mode, uint16* modifiable_instructions) const
{
	uint delay0 = 2 * mode->xscale - 2;
	uint delay1 = delay0 + 1;
	assert(delay0 <= 31);
	assert(delay1 <= 31);

	reinterpret_cast<uint16*>(missing_scanline_data)[2] = mode->width / 2 - 3;

	modifiable_instructions[video_24mhz_composable_program_extern(delay_a_1)] |= delay1 << 8u;
	modifiable_instructions[video_24mhz_composable_program_extern(delay_b_1)] |= delay1 << 8u;
	modifiable_instructions[video_24mhz_composable_program_extern(delay_c_0)] |= delay0 << 8u;
	modifiable_instructions[video_24mhz_composable_program_extern(delay_d_0)] |= delay0 << 8u;
	modifiable_instructions[video_24mhz_composable_program_extern(delay_e_0)] |= delay0 << 8u;
	modifiable_instructions[video_24mhz_composable_program_extern(delay_f_1)] |= delay1 << 8u;

#if !PICO_SCANVIDEO_USE_RAW1P_2CYCLE
	modifiable_instructions[video_24mhz_composable_program_extern(delay_g_0)] |= delay0 << 8u;
#else
	uint delay_half = mode->xscale - 2;
	modifiable_instructions[video_24mhz_composable_program_extern(delay_g_0)] |= delay_half << 8u;
#endif

	modifiable_instructions[video_24mhz_composable_program_extern(delay_h_0)] |= delay0 << 8u;
}

pio_sm_config ScanlinePioProgram::configure_pio(pio_hw_t* pio, uint sm, uint offset) const
{
	pio_sm_config config = video_24mhz_composable_default_program_get_default_config(offset);

	pio_sm_set_consecutive_pindirs(pio, sm, PICO_SCANVIDEO_COLOR_PIN_BASE, PICO_SCANVIDEO_COLOR_PIN_COUNT, true);
	sm_config_set_out_pins(&config, PICO_SCANVIDEO_COLOR_PIN_BASE, PICO_SCANVIDEO_COLOR_PIN_COUNT);
	sm_config_set_out_shift(&config, true, true, 32); // autopull
	sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);

	bool overlay = sm != PICO_SCANVIDEO_SCANLINE_SM1;
	if (overlay) sm_config_set_out_special(&config, 1, 1, PICO_SCANVIDEO_ALPHA_PIN);
	else sm_config_set_out_special(&config, 1, 0, 0);

	return config;
}

const ScanlinePioProgram video_24mhz_composable {
	video_24mhz_composable_program,
	video_24mhz_composable_program_extern(entry_point),
	&video_24mhz_composable_missing_scanline,
};


} // namespace kipili::Video
