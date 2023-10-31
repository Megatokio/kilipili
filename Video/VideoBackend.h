// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VgaMode.h"
#include "utilities.h"

namespace kio::Video
{

extern int	current_frame_start; // rolling index of start of currently displayed frame
extern int	current_scanline;	 // rolling index of currently displayed scanline
extern bool in_vblank;


class VideoBackend
{
public:
	static void initialize() noexcept; // panics

	static void setup(const VgaMode*, uint) throws;
	static void teardown() noexcept;
	static void start() noexcept;
	static void stop() noexcept;

private:
	static void isr_pio0_irq0() noexcept;
};


inline void waitForVBlank() noexcept
{
	while (!in_vblank) { wfe(); }
}

inline void waitForScanline(int scanline) noexcept
{
	while (current_scanline - scanline < 0) { wfe(); }
}

} // namespace kio::Video
