// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "scanvideo_options.h"
#include "standard_types.h"

namespace kio::Video
{

union ScanlineID
{
	uint32 full_id;
	struct
	{
		uint16 scanline, frame;
	}; // little endian

	ScanlineID() = default;
	ScanlineID(uint32 n) noexcept : full_id(n) {}
	ScanlineID(uint16 a, uint16 b) noexcept : scanline(a), frame(b) {}

	bool		operator>(ScanlineID b) const { return full_id > b.full_id; }
	bool		operator>=(ScanlineID b) const { return full_id >= b.full_id; }
	bool		operator<(ScanlineID b) const { return full_id < b.full_id; }
	bool		operator<=(ScanlineID b) const { return full_id <= b.full_id; }
	bool		operator!=(ScanlineID b) const { return full_id != b.full_id; }
	bool		operator==(ScanlineID b) const { return full_id == b.full_id; }
	ScanlineID	operator+(uint n) const { return ScanlineID(full_id + n); }
	ScanlineID& operator+=(uint n)
	{
		full_id += n;
		return *this;
	}
};

struct Scanline
{
	ScanlineID id = 0;
#if PICO_SCANVIDEO_FIXED_FRAGMENT_DMA
	uint16 fragment_words = 0;
#else
	static uint16 fragment_words; // nowhere declared, only for dead code
#endif

	uint32* data = nullptr;
	uint16	used = 0;
	uint16	max	 = 0;
};

} // namespace kio::Video
