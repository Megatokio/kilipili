// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Mp3Player.h"
#include "common/Dispatcher.h"
#include "common/Logger.h"
#include "common/RCPtr.h"
#include "common/cdefs.h"
#include "common/standard_types.h"
#include "common/trace.h"
#include "devices/File.h"
#include <cstdio>
#include <cstring>

namespace kilipili
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
	setAudioSampleFrequency(AUDIO_DEFAULT_SAMPLE_FREQUENCY);
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

static uint32 decode_synchsafeint(cuptr data)
{
	return (((data[0]) & 0x7Fu) << 21) | //
		   (((data[1]) & 0x7Fu) << 14) | //
		   (((data[2]) & 0x7Fu) << 7) |	 //
		   ((data[3]) & 0x7F);
}

static uint32 peek_little_endian32(cuptr data)
{
	return data[0] + 0x100 * (data[1] + 0x100 * (data[2] + 0x100 * data[3]));
}

static void dump_memory(cuptr p, int sz)
{
	for (int i = 0; i < sz; i += 32)
	{
		printf("  ");
		int n = min(32, sz - i);
		for (int j = i; j < i + n; j++) printf("%02x ", p[j]);
		for (int j = i + n; j < i + 32; j++) printf("   ");
		for (int j = i; j < i + n; j++) printf("%c", p[j] >= 32 && p[j] < 127 ? p[j] : '_');
		printf("\n");
		stdio_flush();
	}
}

void Mp3Player::skip_id3v2()
{
	// ID3v2 header is at least 10 bytes and starts with "ID3"

	uint8 data[10];
	input_file->read(data, 10);
	uint32 tag_size = 0;
	if (memcmp(data, "ID3", 3) == 0)
	{
		// flags (byte 5): bit 4 (0x10) indicates a 10-byte footer is present
		bool has_footer = data[5] & 0x10;
		tag_size		= 10 + decode_synchsafeint(data + 6) + (has_footer ? 10 : 0);
		debugstr("ID3v2 tag at start of file (%u bytes)\n", tag_size);
	}
	input_file->setFpos(tag_size);
}

static bool is_info_frame(MadStream* stream, MadHeader* header)
{
	// needs either the complete (very short) frame or at least 40 bytes of the frame

	cuptr frame_start = stream->this_frame;
	off_t frame_len	  = stream->next_frame - stream->this_frame;

	// Determine side-information offset after the 4-byte MPEG header
	off_t side_info_offset = 4;
	if (header->flags & MAD_FLAG_LSF_EXT)
		side_info_offset += (header->mode == MAD_MODE_SINGLE_CHANNEL) ? 9 : 17;	  // MPEG-2 / 2.5
	else side_info_offset += (header->mode == MAD_MODE_SINGLE_CHANNEL) ? 17 : 32; // MPEG-1

	// Check for "Xing" or "Info"
	if (frame_len >= side_info_offset + 4)
	{
		cuptr id = frame_start + side_info_offset;
		if (memcmp(id, "Xing", 4) == 0 || memcmp(id, "Info", 4) == 0)
		{
			debugstr("%.4s frame (%i bytes)\n", cptr(id), int(frame_len));
			return true;
		}
	}

	// Check for "VBRI" (always at fixed offset 36 from frame start)
	if (frame_len >= 40 && memcmp(frame_start + 36, "VBRI", 4) == 0)
	{
		debugstr("VBRI frame (%i bytes)\n", int(frame_len));
		return true;
	}

	return false;
}

bool Mp3Player::is_id3v1_or_ape()
{
	// return true if an id3v1 or ape frame was skipped

	// we have at least 8 bytes (MAD_BUFFER_GUARD) bytes available
	// because otherwise header.decode() bailed out with MAD_ERROR_BUFLEN first:
	uint remaining = uint(stream->bufend - stream->this_frame);
	assert(remaining >= 8);

	// test for ID3v1 tag:
	// always 128 bytes long and always at end of file
	if (memcmp(stream->this_frame, "TAG", 3) == 0)
	{
		remaining += input_file->getSize() - input_file->getFpos();
		if (remaining == 128) debugstr("ID3v1 tag at end of file (128 bytes)\n");
		else if (remaining < 128) debugstr("ID3v1 tag truncated (%u bytes)\n", remaining);
		else debugstr("ID3v1 tag before eof (128 + %u bytes)\n", remaining - 128u);
		input_file->setFpos(input_file->getSize());
		stream->next_frame = stream->bufend;
		return true; // skipped
	}

	// test for APE tag:
	// the APE tag is either first or last frame.
	// if last it comes right before an eventual final ID3v1 tag:
	if (memcmp(stream->this_frame, "APETAGEX", 8) == 0)
	{
		if (!first_frame)
		{
			debugstr("APE tag in last frame: skipping rest of file\n"); // incl. an evtl. ID3v1 tag
			input_file->setFpos(input_file->getSize());
			stream->next_frame = stream->bufend;
			return true; // skipped
		}

		// this probably never happens because for the first frame the input buffer should be full:
		if (remaining < 16)
		{
			stream->next_frame = stream->this_frame; // -> come back here
			stream->error	   = MAD_ERROR_BUFLEN;
			return false; // not skipped
		}

		cuptr data	   = stream->this_frame;
		uint  ape_size = 32 + peek_little_endian32(data + 12);

		debugstr("APE tag in first frame (%u bytes)\n", ape_size);

		if (remaining >= ape_size)
		{
			stream->next_frame = stream->this_frame + ape_size;
			return true; // skipped
		}
		else // this may happen if s.o. puts an image into the APE frame:
		{
			stream->next_frame = stream->bufend;
			input_file->setFpos(input_file->getFpos() + ape_size - remaining);
			return true; // skipped
		}
	}

	// a real glitch in the file:

	if (remaining < 128)
	{
		stream->next_frame = stream->this_frame; // -> come back here
		stream->error	   = MAD_ERROR_BUFLEN;
		return false; // not skipped
	}

	debugstr("mp3_header_decode: %s\n", stream->errorstr());
	if (debug) dump_memory(stream->this_frame, 128);

	// skip over possible burst of 0xff to prevent a burst of memory dumps
	// and reduce the chance for 'successfully' decoding a broken frame:
	cuptr p = stream->this_frame;
	cuptr e = stream->bufend;
	while (p < e && *p == 0xff) p++;
	stream->next_frame = max(stream->next_frame, p - 1);

	return false; // not skipped
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

			// skip ID3v2 tag at start of file:
			skip_id3v2();
			// prepare to skip Info, Xing or VBRI in first frame:
			first_frame = true;

			buffer->count = uint16(input_file->read(buffer->data, buffer->size, true));
			stream->set_buffer_pointers(buffer->data, buffer->count);

			// decode, synthesize and play audio until not enough bytes in input buffer[]:
			for (;;)
			{
				YIELD(1)
				if (frame->header.decode(stream) == -1)
				{
					if (stream->error == MAD_ERROR_BUFLEN) break; // need more input data
					if (!is_recoverable(stream->error)) throw stream->errorstr();
					if (is_id3v1_or_ape()) continue;			  // resolved: skipped
					if (stream->error == MAD_ERROR_BUFLEN) break; // need more input data
					continue;									  // not resolved: seek sync
				}

				// skip first valid audio frame if it is in fact a Xing, VBRI or Info frame:
				if (first_frame && is_info_frame(stream, &frame->header)) continue;
				first_frame = false;

				if (frame->decode(stream) == -1)
				{
					if (stream->error == MAD_ERROR_BUFLEN) break; // safety, never happens
					if (!is_recoverable(stream->error)) throw stream->errorstr();
					debugstr("mp3_frame_decode: %s\n", stream->errorstr());
					continue;
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
					setAudioSampleFrequency(float(sample_rate = pcm.samplerate));
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
			uint nprocessed = uint(stream->next_frame - buffer->data); // num bytes processed for current/last frame
			uint nremaining = buffer->count - nprocessed;			   // num bytes remaining unprocessed in buffer
			assert(nprocessed <= buffer->count);
			assert(nremaining <= buffer->count);
			if unlikely (nremaining == buffer->size) throw "mp3_input: buffer too small";
			memmove(buffer->data, stream->next_frame, nremaining); // move remaining data to start of buffer

			uint n		  = input_file->read(buffer->data + nremaining, buffer->size - nremaining, true);
			buffer->count = uint16(nremaining + n);

			if (n)
			{
				stream->set_buffer_pointers(buffer->data, nremaining + n);
				state = 1;
			}
			else // n=0 => end of file => next file or quit
			{
				if (debug && nremaining)
				{
					bool id3v1 = nremaining >= 128 && memcmp(buffer->data + nremaining - 128, "TAG", 3) == 0;
					if (id3v1) nremaining -= 128;
					debugstr("mp3_input: %u bytes not processed at eof\n", nremaining);
					dump_memory(buffer->data, int(nremaining));
					if (id3v1) debugstr("ID3v1 tag at end of file\n");
				}

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


} // namespace kilipili


/*
































*/
