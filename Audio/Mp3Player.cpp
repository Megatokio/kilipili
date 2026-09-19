// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Mp3Player.h"
#include "Devices/File.h"
#include "common/Dispatcher.h"
#include "common/Logger.h"
#include "common/RCPtr.h"
#include "common/cdefs.h"
#include "common/standard_types.h"
#include "common/trace.h"
#include <cstdio>
#include <cstring>
#include <new>

namespace kilipili::Audio
{

static int mp3_callback(void* data) noexcept
{
	Mp3Player* mp3_player = reinterpret_cast<Mp3Player*>(data);
	return mp3_player->run();
}

Mp3Player::Mp3Player() : AudioPlayer("*.mp3")
{
	Dispatcher::addWithDelay(mp3_callback, this, 100 * 1000); //
}

Mp3Player::~Mp3Player() noexcept
{
	Dispatcher::removeHandler(mp3_callback, this);
	if (output_adapter) output_adapter->eof = true;
	Audio::setSampleFrequency(AUDIO_DEFAULT_SAMPLE_FREQUENCY);
}

void Mp3Player::setVolume(float v) //
{
	debugstr("Mp3Player::setVolume %f  (TODO)\n", double(v));
}

static inline Sample mp3_scale(mad_fixed_t sample) // scale mp3 output to int16
{
	static_assert(sizeof(mad_fixed_t) == sizeof(int32));
	static_assert(sizeof(Sample) == sizeof(int16));

	if ((0)) sample += (1 << (MAD_F_FRACBITS - 16)); // round
	sample = sample >> (MAD_F_FRACBITS + 1 - 16);	 // quantize

	return sample == Sample(sample) ? Sample(sample) : sample < 0 ? -0x8000 : +0x7fff;
}


#define YIELD(N)     \
	state = N;       \
	return 1 * 1000; \
	case N:

#define SM_START() \
	switch (state) \
	{              \
	default:       \
	case 0:

#define SM_END() }


int Mp3Player::run() noexcept
{
	trace("Mp3Player::run");

	try
	{
		if (input_file)
		{
			if (paused) return 100 * 1000;

			SM_START()

			if (!output_adapter)
			{
				static_assert(sizeof(MadSynth) >= sizeof(MadFrame));
				static_assert(sizeof(MadFrame) >= sizeof(AudioAdapter));
				static_assert(sizeof(AudioAdapter) >= sizeof(Buffer));
				static_assert(sizeof(Buffer) >= sizeof(MadStream));

				synth					   = new MadSynth;
				frame					   = new MadFrame;
				RCPtr<AudioAdapter> output = new AudioAdapter;
				buffer					   = new Buffer;
				stream					   = new MadStream();
				output_adapter			   = output;
				addAudioSource(std::move(output));
			}
			else
			{
				assert(synth && frame && buffer && stream);
				buffer->reset();
				synth->reset();
				frame->reset();
				stream->reset();
			}

			YIELD(5);
			buffer->count = uint16(input_file->read(buffer->data, buffer->size, true));
			stream->set_buffer_pointers(buffer->data, buffer->count);

			// decode, synthesize and play audio until not enough bytes in input buffer[]:
			for (;;)
			{
				YIELD(1)
				if (frame->header.decode(stream) == -1)
				{
					if (stream->error == MAD_ERROR_BUFLEN) break; // need more input data
					if ((0)) debugstr("mp3_header_decode: %s\n", stream->errorstr());
					if (is_recoverable(stream->error)) continue; // return 1 * 1000; // cover art?
					else throw stream->errorstr();
				}

				if (frame->decode(stream) == -1)
				{
					if (stream->error == MAD_ERROR_BUFLEN) break;			// need more input data
					debugstr("mp3_frame_decode: %s\n", stream->errorstr()); // almost always header: cover art?
					if (is_recoverable(stream->error)) continue;			// return 1 * 1000;
					else throw stream->errorstr();
				}

				YIELD(2)
				header(&frame->header);
				synth->synthesize_pcm(frame);
				filter(stream, frame);

				nsamples_done = 0;

				YIELD(3)
				MadPcmBuffer& pcm = synth->pcm;

				if (pcm.samplerate != sample_rate) // must be start of file
				{
					if (output_adapter->queue.avail()) return 5 * 1000; // back to YIELD(3)
					debugstr("pcm.f = %u Hz\n", pcm.samplerate);
					Audio::setSampleFrequency(float(sample_rate = pcm.samplerate));
				}

				uint			   nchannels = pcm.channels;
				uint			   nsamples	 = pcm.length;
				const mad_fixed_t* left_ch	 = pcm.samples[0];
				const mad_fixed_t* right_ch	 = pcm.samples[1];

				for (uint i = nsamples_done; i < nsamples; i++)
				{
					if unlikely (!output_adapter->queue.free())
					{
						nsamples_done = uint16(i);
						return 5 * 1000;
					}
					if (nchannels == 1) output_adapter->queue.put(HwAudioSample(mp3_scale(left_ch[i])));
					else output_adapter->queue.put(HwAudioSample(mp3_scale(left_ch[i]), mp3_scale(right_ch[i])));
				}
			}

			YIELD(4)
			assert(stream->error == MAD_ERROR_BUFLEN);
			size_t nprocessed = size_t(stream->next_frame - buffer->data); // num bytes processed for current/last frame
			size_t nremaining = buffer->count - nprocessed;				   // num bytes remaining unprocessed in buffer
			assert(nprocessed <= buffer->count);
			assert(nremaining <= buffer->count);
			if unlikely (nremaining == buffer->size) throw "mp3_input: buffer too small";
			memmove(buffer->data, stream->next_frame, nremaining); // move remaining data to start of buffer

			uint n		  = input_file->read(buffer->data + nremaining, buffer->size - nremaining, true);
			buffer->count = uint16(nremaining + n);

			if (n)
			{
				stream->set_buffer_pointers(buffer->data, nremaining + n); //
				state = 1;
			}
			else // n=0 => end of file => next file or quit
			{
				debugstr("mp3_input: %u bytes not processed at eof\n", nremaining);

				state = 0;
				if (repeat_file && !next_file && !next_dir) input_file->setFpos(0u);
				else input_file = nullptr;
			}
			SM_END()
		}
		else if (nextInputFile())
		{
			// we are not playing
			// but there's a music file to play:
			assert(input_file);
			state = 0;
		}
		else
		{
			// we are not playing
			// there is nothing to play:
			assert(!input_file);
			assert(!current_dir);

			if (output_adapter)
			{
				output_adapter->eof = true;
				output_adapter		= nullptr;
				synth				= nullptr;
				frame				= nullptr;
				buffer				= nullptr;
				stream				= nullptr;
			}
			return 100 * 1000;
		}
	}
	catch (Error e)
	{
		logline("Mp3Player: %s", e);
		input_file = nullptr;
		return 100 * 1000;
	}
	catch (...)
	{
		logline("Mp3Player: unknown exception");
		input_file = nullptr;
		return 100 * 1000;
	}

	return 5 * 1000;
}


} // namespace kilipili::Audio


/*
































*/
