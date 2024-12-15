// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Devices/StdFile.h"
#include "standard_types.h"
#include <cstdio>

namespace kio::Audio
{

/*	class YMFileConverter provides some conversion functions for YM music files.
	this file is currently only intended for desktop use, as it buffers whole files.
*/
class YMFileConverter
{
public:
	using FilePtr = Devices::FilePtr;

	enum FType { UNSET = 0, YM2, YM3, YM3b, YM5, YM6 };
	enum Attributes { NotInterleaved = 1 };

	YMFileConverter() noexcept {}
	~YMFileConverter();

	uint32 importFile(FilePtr, cstr fname, bool verbose = false);
	uint32 exportYMMFile(FilePtr);
	uint32 exportWavFile(FilePtr, float sample_rate = 44100);

	uint32 importFile(cstr fpath, bool verbose = false);
	uint32 exportYMMFile(cstr fpath, uint8 wbits, uint8 lbits);
	uint32 exportWavFile(cstr fpath, float sample_rate = 44100);
	uint32 exportRsrcFile(cstr fpath, cstr rsrc_fpath, uint8 wbits, uint8 lbits);

	FType  file_type  = UNSET;
	uint   frame_size = 0;
	uint32 num_frames = 0;
	uint32 attributes = 0;
	uint32 ay_clock	  = 0;
	uint16 drums	  = 0;
	uint16 frame_rate = 0;
	uint32 loop_frame = 0;
	uint32 _padding	  = 0;
	cstr   title	  = nullptr;
	cstr   author	  = nullptr;
	cstr   comment	  = nullptr;

	uint8* register_data = nullptr;

private:
	uint32 write_raw_audio_file(FilePtr, float sample_rate);
	void   interleave_registers();
};

} // namespace kio::Audio
