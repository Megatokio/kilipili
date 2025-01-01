// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include <Devices/File.h>
#include <common/Array.h>
#include <cstdio>


namespace kio::Audio
{

/*	class YMMFileConverter converts .ym music file to .ymm format.

	.ym files contain register data for the AY-3-8912 sound chip.
	The .ym format is only suitable for playback if you can load the entire decompressed file into ram
	because the data is stored per register and not per frame as it would be needed.
	In addition they are normally Lzh compressed and the decoder requires a big chunk of memory too.
	
	.ymm files are designed for playback in memory constrained environments.
	If compressed with a window size of winbits=14 <=> buffer size = 32 kByte then they are often better
	compressed than .ym files and you only need this large buffer hanging around but not the whole file.
	But they also compress well with much smaller buffer size: The recommended buffer size is
	actually only between winbits=10 <=> 2 kBytes and winbits=12 <=> 8 kBytes.
	This frees almost all ram for video display. (the main use case of lib kilipili)
	The allowed window size is between winbits=8 and winbits=14.

	class YMMFileConverter is part of the RsrcFileWriter.
	The relevant entries in a command file for the RsrcFileWriter are the conversion type 'ymm'
	and the window size, e.g.:
	*.ym   ymm   W12

	The decoder is kilipili/Audio/YMMusicPlayer.cpp

	How it works:
	The data for each register is RLE compressed
	and this is further LZ compressed with windowsize = total window/16 or winbits-4
	and this is encoded into a bitstream.
	So there are actually 16 compressed streams which each use 1/16 of the total buffer size.

	Then the RLE data is also LZ compressed with windowsize/2 and windowsize*2.
	Then the window sizes are traded between the registers to give registers which benefit the most
	from a larger buffer a larger buffer and give registers which suffer the least a smaller buffer
	or even no backref buffer at all.

	Then the 16 choosen bitstreams are combined into a single bitstream, which is quite tricky.
	Finally the final bitstream is decoded and compared against the original data.
	So the encoder is it's own unit test.
	Also this decoder is a good reference for the actual .ymm file decoder.

	The .ym and the .ymm file contains a 'loop position'.
	In order to reach this position the bitstream must be rewound to the start
	and the frames of the intro must be skipped until the loop frame is reached.
*/
class YMMFileConverter
{
public:
	using File		   = Devices::File;
	using FilePtr	   = Devices::FilePtr;
	using SerialDevice = Devices::SerialDevice;

	enum FType { UNSET = 0, YM2, YM3, YM3b, YM5, YM6 };
	enum Attributes { NotInterleaved = 1 };

	YMMFileConverter() = default;
	~YMMFileConverter() { delete[] register_data; }

	uint32 convertFile(cstr infile, File* outfile, int verbose = false, uint winbits = 10);

	uint32 csize; // the compressed input file size (if it was compressed)
	uint32 usize; // the uncompressed input file size

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
	void import_ym_file(File* file, SerialDevice* log, cstr fname);
	void export_ymm_file(File* file, SerialDevice* log, uint winbits);
	void deinterleave_registers();

	Array<uint8> extract_register_stream(uint reg, uint8 mask = 0xff);
	void		 decode_ymm(uint32 rbusz, struct BitArray& instream, uint winbits);
};


} // namespace kio::Audio
