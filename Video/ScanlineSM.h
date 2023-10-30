// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VgaMode.h"
#include "utilities.h"

namespace kio::Video
{

class ScanlineSM
{
public:
	bool in_vblank;
	int	 current_frame_start = 0; // rolling index of start of currently displayed frame
	int	 current_scanline	 = 0; // rolling index of currently displayed scanline

	ScanlineSM() = default;

	void setup(const VgaMode* mode) throws;
	void teardown() noexcept;
	void start(); // start or restart
	void stop();

	void waitForVBlank() const noexcept
	{
		while (!in_vblank) { wfe(); }
	}
	void waitForScanline(int scanline) const noexcept
	{
		while (current_scanline - scanline < 0) { wfe(); }
	}

private:
	static void isr_pio0_irq0() noexcept;
	void		prepare_active_scanline() noexcept;
	void		prepare_vblank_scanline() noexcept;
	void		abort_all_scanline_sms() noexcept;
};


extern ScanlineSM scanline_sm;

} // namespace kio::Video
