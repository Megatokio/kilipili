// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "MadFrame.h"
#include "MadStream.h"
#include "MadSynth.h"
#include "common/standard_types.h"


namespace kilipili::Audio
{

class Mp3Decoder
{
public:
	Mp3Decoder() noexcept		   = default;
	virtual ~Mp3Decoder() noexcept = default;

	Error play(int stream_options = 0) noexcept;

	/*	callback: read more data from file
		return: actual number of bytes read. return 0 = eof.
		throw on hw error
	*/
	virtual uint input(uchar* data, uint count) = 0;

	/*	callback: apply filter (optional)
	*/
	virtual void filter(const MadStream*, MadFrame*);

	/*	callback: send data to audio output
	*/
	virtual void output(const MadHeader*, const MadPcmBuffer*) = 0;
};

} // namespace kilipili::Audio
