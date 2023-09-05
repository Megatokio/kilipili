// Copyright (c) 2022 - 2022 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "cdefs.h"
#include <hardware/pio.h>
#include "VgaMode.h"
#include "Scanline.h"


namespace kipili::Video
{

struct ScanlinePioProgram
{
	pio_program_t	program;
	uint16			wait_index;
	Scanline*		missing_scanline;

	virtual pio_sm_config configure_pio(pio_hw_t* pio, uint sm, uint offset) const;
	virtual void adapt_for_mode(const VgaMode*, uint16 modifiable_instructions[]) const; // modifiable_instructions is of size program->length

	virtual ~ScanlinePioProgram() = default;

	ScanlinePioProgram(pio_program_t p,uint16 e, Scanline* s) noexcept : program(p), wait_index(e), missing_scanline(s) {}
};

extern const ScanlinePioProgram video_24mhz_composable;

} // namespace








