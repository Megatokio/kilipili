// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VgaMode.h"
#include "VgaTiming.h"
#include <hardware/dma.h>

namespace kio::Video
{

struct TimingSM
{
	alignas(16) uint32 prog_active[4];
	alignas(16) uint32 prog_vblank[4];
	alignas(16) uint32 prog_vpulse[4];

	struct
	{
		uint32* program;
		uint32	count;
	} program[4];

	enum State {
		generate_v_active,
		generate_v_frontporch,
		generate_v_pulse,
		generate_v_backporch,
	};
	uint state = generate_v_pulse;

	uint timing_scanline;

	uint8 video_htiming_load_offset;

	Error setup(const VgaMode*, const VgaTiming*);
	void  start(); // start or restart
	void  stop();

	//private:
	void configure_dma_channel();
	void setup_timings(const VgaTiming*);
	void install_pio_program(uint32 pixel_clock_frequency);
	void isr();
};


extern TimingSM timing_sm;

} // namespace kio::Video
