// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Mp3Decoder.h"
#include "common/RCPtr.h"
#include "common/cdefs.h"
#include "common/standard_types.h"
#include "common/trace.h"
#include <cstdio>
#include <cstring>
#include <new>

namespace kilipili::Audio
{

void Mp3Decoder::filter(const MadStream*, MadFrame*) {}

Error Mp3Decoder::play(int stream_options) noexcept
{
	trace(__func__);

	try
	{
		RCPtr<MadSynth> synth(new MadSynth);	// too big: > 13 kB
		RCPtr<MadFrame> frame(new MadFrame);	// too big: > 9 kB
		MadStream		stream(stream_options); // ~ 100 byte
		constexpr uint	buffer_size	 = 2000;
		uint			buffer_count = 0;

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
		stream.set_buffer_pointers(buffer->data, buffer_count);

		for (;;)
		{
			for (;;)
			{
				if (debug && frame->header.decode(&stream) == -1)
				{
					if (stream.error == MAD_ERROR_BUFLEN) break;
					debugstr("mp3_header_decode: %s\n", stream.errorstr());
					if (is_recoverable(stream.error)) continue;
					else return stream.errorstr();
				}

				if (frame->decode(&stream) == -1)
				{
					if (stream.error == MAD_ERROR_BUFLEN) break;
					debugstr("mp3_frame_decode: %s\n", stream.errorstr());
					if (is_recoverable(stream.error)) continue;
					else return stream.errorstr();
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
				stream.set_buffer_pointers(buffer->data, nremaining + n);
			}
			else // n=0 -> eof
			{
				debugstr("mp3_input: %u bytes not processed at eof\n", nremaining);
				return nullptr;
			}
		}
	}
	catch (Error e)
	{
		return e;
	}
}

} // namespace kilipili::Audio


/*
































*/
