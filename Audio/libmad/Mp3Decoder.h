// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
//#include "common/RCPtr.h"
#include "MadFrame.h"
#include "MadStream.h"
#include "MadSynth.h"
#include "common/standard_types.h"


namespace kilipili::Audio
{


class Mp3Player
{
public:
	//	enum FlowCtl { CONTINUE = 0, SKIP = 1, STOP = 2 };

	Mp3Player() noexcept		  = default;
	virtual ~Mp3Player() noexcept = default;

	Error play(int stream_options = 0) noexcept;

	/*	callback: read more data from file
		return: actual number of bytes read. return 0 = eof.
		throw on hw error
	*/
	virtual uint input(uchar* data, uint count) = 0;

	//	/*	optional callback: determine whether this block should be played.
	//		return: CONTINUE = play it (normal), SKIP = skip block, STOP = return from play()
	//	*/
	//	virtual FlowCtl header(const MadHeader*);

	virtual void filter(const MadStream*, MadFrame*); // optional

	/*	callback: send data to audio output
	*/
	virtual void output(const MadHeader*, const MadPcmBuffer*) = 0;

	//bool f_header {1};
	//bool f_filter {1};
};

//	enum mad_flow {
//		MAD_FLOW_CONTINUE = 0x0000, /* continue normally */
//		MAD_FLOW_STOP	  = 0x0010, /* stop decoding normally */
//		MAD_FLOW_BREAK	  = 0x0011, /* stop decoding and signal an error */
//		MAD_FLOW_IGNORE	  = 0x0020, /* ignore (mute) the current frame */
//	};
//
//	class Mp3Decoder
//	{
//	public:
//		Mp3Decoder() noexcept;
//		virtual ~Mp3Decoder() noexcept;
//
//		void set_options(int options) noexcept;
//		int	 run(int options = 0);
//
//		virtual mad_flow input(MadStream*) = 0;						  // input more data to MadStream
//		virtual mad_flow handle_header(const MadHeader*);			  // optional
//		virtual mad_flow filter(const MadStream*, MadFrame*);		  // optional
//		virtual mad_flow output(const MadHeader*, MadPcmBuffer*) = 0; // output samples from MadPcmBuffer
//		virtual mad_flow handle_error(MadStream*, MadFrame*);		  // decode header or decode frame failed
//
//		int	 options {0};
//		bool f_header {1};
//		bool f_filter {1};
//	};


} // namespace kilipili::Audio
