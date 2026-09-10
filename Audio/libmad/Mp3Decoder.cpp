// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Mp3Decoder.h"
#include "common/Trace.h"
#include "common/cdefs.h"
#include "common/standard_types.h"
//#include "global.h"
#include <cstdio>
#include <cstring>
#include <new>

namespace kilipili::Audio
{

using MadHeader	   = mad_header;
using MadPcmBuffer = mad_pcm;
using MadStream	   = mad_stream;
using MadFrame	   = mad_frame;
using MadSynth	   = mad_synth;


void Mp3Player::filter(const MadStream*, MadFrame*) {}

Error Mp3Player::play(int stream_options) noexcept
{
	MadSynth	   synth;  // too big: > 13 kB
	MadFrame	   frame;  // too big: > 9 kB
	MadStream	   stream; // ~ 100 byte
	constexpr uint buffer_size = 2000;
	uint		   buffer_count {0};

	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);
	mad_stream_options(&stream, stream_options);


	struct Buffer
	{
		uchar data[buffer_size]; // input stream buffer
	};

	Buffer buffer;

	//debugstr("mp3_input: first call\n");
	buffer_count = input(buffer.data, buffer_size);
	//debugstr("mp3_input: read %u bytes\n", buffer_count);
	assert(buffer_count != 0);
	mad_stream_buffer(&stream, buffer.data, buffer_count);

	for (;;)
	{
		for (;;)
		{
			if (debug && mad_header_decode(&frame.header, &stream) == -1)
			{
				if (stream.error == MAD_ERROR_BUFLEN) break;
				cstr msg = mad_stream_errorstr(&stream);
				debugstr("mp3_header_decode: %s\n", msg);
				if (!MAD_RECOVERABLE(stream.error)) return msg;
				else continue; // goto skip;
			}

			if (mad_frame_decode(&frame, &stream) == -1)
			{
				if (stream.error == MAD_ERROR_BUFLEN) break;
				cstr msg = mad_stream_errorstr(&stream);
				debugstr("mp3_frame_decode: %s\n", msg);
				if (!MAD_RECOVERABLE(stream.error)) return msg;
				else continue; // goto skip;
			}

			filter(&stream, &frame);
			mad_synth_frame(&synth, &frame);
			output(&frame.header, &synth.pcm);
		}

		//skip:
		assert(stream.error == MAD_ERROR_BUFLEN);
		int nread = stream.next_frame - buffer.data; // num bytes decoder processed for current (last) frame
		int nrem  = buffer_count - nread;			 // num bytes remaining in buffer
		//debugstr("mp3_input: last frame size = %i bytes\n", nread);
		if unlikely (nread == 0) return "mp3_input: buffer too small";
		assert(nread >= 0 && nread <= buffer_count);
		assert(nrem >= 0 && nrem <= buffer_count);
		memmove(buffer.data, stream.next_frame, nrem); // move remaining data to start of buffer

		uint n		 = input(buffer.data + nrem, buffer_size - nrem);
		buffer_count = nrem + n;

		if (n)
		{
			//debugstr("mp3_input: read %u bytes\n", n);
			mad_stream_buffer(&stream, buffer.data, nrem + n);
		}
		else // n=0 -> eof
		{
			debugstr("mp3_input: %u bytes not processed at eof\n", nrem);
			return nullptr;
		}
	}

	mad_stream_finish(&stream);
	mad_frame_finish(&frame);
	mad_synth_finish(&synth);
}


// --------------------------------------------------------
#if 0

Mp3Decoder::Mp3Decoder() noexcept {}
Mp3Decoder::~Mp3Decoder() noexcept {}


//	Mp3Player::FlowCtl Mp3Player::header(const MadHeader*)
//	{
//		f_header = 0; // don't call again: subclass didn't implement header()
//		return CONTINUE;
//	}
//void Mp3Player::filter(const MadStream*, MadFrame*)
//{
//	f_filter = 0; // don't call again: subclass didn't implement filter()
//}

mad_flow Mp3Decoder::handle_error(MadStream* stream, struct MadFrame* frame)
{
	// default error action:

	switch (stream->error)
	{
	case MAD_ERROR_BADCRC: mad_frame_mute(frame); return MAD_FLOW_IGNORE;
	default: return MAD_FLOW_CONTINUE;
	}
}
mad_flow Mp3Decoder::handle_header(const MadHeader*)
{
	f_header = 0; // don't call again: subclass didn't implement handle_header()
	return MAD_FLOW_CONTINUE;
}
mad_flow Mp3Decoder::filter(const MadStream*, MadFrame*)
{
	f_filter = 0; // don't call again: subclass didn't implement filter()
	return MAD_FLOW_CONTINUE;
}

//void Mp3Decoder::set_options(int options) noexcept //
//{
//	mad_stream_options(this->stream, options);
//}

int Mp3Decoder::run(int options)
{
	trace(__func__);

	this->options = options;

	struct Sync
	{
		mutable int rc = 0; // for RCPtr
		MadStream	stream;
		MadFrame	frame;
		MadSynth	synth;
	};

	RCPtr<Sync> sync {new (std::nothrow) Sync};
	if (sync == nullptr) return -1;

	MadStream* stream = &sync->stream;
	MadFrame*  frame  = &sync->frame;
	MadSynth*  synth  = &sync->synth;
	int		   result = 0;

	mad_stream_options(stream, this->options);

	do {
		switch (input(stream))
		{
		case MAD_FLOW_STOP: goto done;
		case MAD_FLOW_BREAK: goto fail;
		case MAD_FLOW_IGNORE: continue;
		case MAD_FLOW_CONTINUE: break;
		default: break;
		}

		while (1)
		{
			// if (f_header)
			// {
			// 	if (frame->header.decode( stream) == -1)
			// 	{
			// 		cstr msg = mad_stream_errorstr(stream);
			// 		debugstr("mp3_header_decode: %s\n", msg);
			// 		if (!MAD_RECOVERABLE(stream->error)) return msg;
			// 		else goto skip;
			// 	}

			// 	FlowCtl flow = header(&frame->header);
			// 	if (flow == SKIP) goto skip;
			// 	if (flow == STOP) return nullptr;
			// }

			if (f_header)
			{
				if (frame->header.decode(stream) == -1)
				{
					if (!MAD_RECOVERABLE(stream->error)) break;

					switch (handle_error(stream, frame))
					{
					case MAD_FLOW_STOP: goto done;
					case MAD_FLOW_BREAK: goto fail;
					case MAD_FLOW_IGNORE:
					case MAD_FLOW_CONTINUE:
					default: continue;
					}
				}

				switch (handle_header(&frame->header))
				{
				case MAD_FLOW_STOP: goto done;
				case MAD_FLOW_BREAK: goto fail;
				case MAD_FLOW_IGNORE: continue;
				case MAD_FLOW_CONTINUE: break;
				}
			}

			if (mad_frame_decode(frame, stream) == -1)
			{
				if (!MAD_RECOVERABLE(stream->error)) break;

				switch (handle_error(stream, frame))
				{
				case MAD_FLOW_STOP: goto done;
				case MAD_FLOW_BREAK: goto fail;
				case MAD_FLOW_IGNORE: break;
				case MAD_FLOW_CONTINUE: continue;
				}
			}

			if (f_filter)
			{
				switch (filter(stream, frame))
				{
				case MAD_FLOW_STOP: goto done;
				case MAD_FLOW_BREAK: goto fail;
				case MAD_FLOW_IGNORE: continue;
				case MAD_FLOW_CONTINUE: break;
				}
			}

			mad_synth_frame(synth, frame);

			switch (output(&frame->header, &synth->pcm))
			{
			case MAD_FLOW_STOP: goto done;
			case MAD_FLOW_BREAK: goto fail;
			case MAD_FLOW_IGNORE:
			case MAD_FLOW_CONTINUE: break;
			}
		}
	}
	while (stream->error == MAD_ERROR_BUFLEN);

fail:
	result = -1;

done:
	return result;
}
#endif

} // namespace kilipili::Audio


/*
































*/
