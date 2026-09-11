// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Mp3Decoder.h"
#include "common/Trace.h"
#include "common/cdefs.h"
#include "common/standard_types.h"
//#include "global.h"
#include "common/RCPtr.h"
#include <cstdio>
#include <cstring>
#include <new>

namespace kilipili::Audio
{

void Mp3Player::filter(const MadStream*, MadFrame*) {}

Error Mp3Player::play(int stream_options) noexcept
{
	RCPtr<MadSynth> synth(new MadSynth);	// too big: > 13 kB
	RCPtr<MadFrame> frame(new MadFrame);	// too big: > 9 kB
	MadStream		stream(stream_options); // ~ 100 byte
	constexpr uint	buffer_size = 2000;
	uint			buffer_count {0};

	struct Buffer
	{
		uchar data[buffer_size]; // input stream buffer
		int	  rc = 0;
	};

	RCPtr<Buffer> buffer(new Buffer);

	//debugstr("mp3_input: first call\n");
	buffer_count = input(buffer->data, buffer_size);
	//debugstr("mp3_input: read %u bytes\n", buffer_count);
	assert(buffer_count != 0);
	MadStream_buffer(&stream, buffer->data, buffer_count);

	for (;;)
	{
		for (;;)
		{
			if (debug && mad_header_decode(&frame->header, &stream) == -1)
			{
				if (stream.error == MAD_ERROR_BUFLEN) break;
				cstr msg = MadStream_errorstr(&stream);
				debugstr("mp3_header_decode: %s\n", msg);
				if (!MAD_RECOVERABLE(stream.error)) return msg;
				else continue; // goto skip;
			}

			if (MadFrame_decode(frame, &stream) == -1)
			{
				if (stream.error == MAD_ERROR_BUFLEN) break;
				cstr msg = MadStream_errorstr(&stream);
				debugstr("mp3_frame_decode: %s\n", msg);
				if (!MAD_RECOVERABLE(stream.error)) return msg;
				else continue; // goto skip;
			}

			filter(&stream, frame);
			synth->synthesize_pcm(frame);
			output(&frame->header, &synth->pcm);
		}

		//skip:
		assert(stream.error == MAD_ERROR_BUFLEN);
		int nprocessed = stream.next_frame - buffer->data; // num bytes decoder processed for current (last) frame
		int nremaining = buffer_count - nprocessed;		   // num bytes remaining unprocessed in buffer
		assert(nprocessed >= 0 && nprocessed <= buffer_count);
		assert(nremaining >= 0 && nremaining <= buffer_count);
		if unlikely (nremaining == buffer_size) return "mp3_input: buffer too small";
		memmove(buffer->data, stream.next_frame, nremaining); // move remaining data to start of buffer

		uint n		 = input(buffer->data + nremaining, buffer_size - nremaining);
		buffer_count = nremaining + n;

		if (n)
		{
			//debugstr("mp3_input: read %u bytes\n", n);
			MadStream_buffer(&stream, buffer->data, nremaining + n);
		}
		else // n=0 -> eof
		{
			debugstr("mp3_input: %u bytes not processed at eof\n", nremaining);
			return nullptr;
		}
	}
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
	case MAD_ERROR_BADCRC: MadFrame_mute(frame); return MAD_FLOW_IGNORE;
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
//	MadStream_options(this->stream, options);
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

	MadStream_options(stream, this->options);

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
			// 		cstr msg = MadStream_errorstr(stream);
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

			if (MadFrame_decode(frame, stream) == -1)
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

			MadSynth_frame(synth, frame);

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
