// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
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
	enum FType { UNSET = 0, YM2, YM3, YM3b, YM5, YM6 };
	enum Attributes { NotInterleaved = 1 };

	YMFileConverter() noexcept {}
	~YMFileConverter() { dispose_all(); }

	uint32		importFile(cstr infile, bool verbose = false);
	uint32		exportYMRegisterFile(cstr outfile);
	uint32		exportWavFile(cstr outfile, float sample_rate = 44100);
	uint32		exportYMFile(cstr outfile);
	uint32		exportRsrcFile(cstr hdrfilepath, cstr outfile, uint8 wsize = 12, uint8 lsize = 6);
	static void exportYMMusicWavFile(cstr infile, cstr outfile);

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
	FILE*  file			 = nullptr;

private:
	uint32 write_raw_audio_file(FILE*, float sample_rate);
	void   unpack_lzh_data(uint8*& io_data, uint32& io_size);
	void   dispose_all();
	void   interleave_registers();
	void   write_ym5ym6_header();
	uint   store_ym5ym6_header(uint8* bu, uint bu_size);
};

} // namespace kio::Audio
