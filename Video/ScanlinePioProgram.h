// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Scanline.h"
#include "VgaMode.h"
#include <hardware/pio.h>


namespace kio::Video
{

struct ScanlinePioProgram
{
	pio_program_t program;
	uint16		  wait_index;
	Scanline*	  missing_scanline;

	virtual pio_sm_config configure_pio(pio_hw_t* pio, uint sm, uint offset) const;
	virtual ~ScanlinePioProgram() = default;

	ScanlinePioProgram(pio_program_t p, uint16 e, Scanline* s) noexcept : program(p), wait_index(e), missing_scanline(s)
	{}
};

extern const ScanlinePioProgram video_24mhz_composable;

} // namespace kio::Video
