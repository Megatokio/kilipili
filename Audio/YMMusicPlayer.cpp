// Copyright (c) 2024 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "YMMusicPlayer.h"
#include "Devices/File.h"
#include "Dispatcher.h"
#include "common/Logger.h"
#include "common/cdefs.h"
#include "common/trace.h"

namespace kilipili::Audio
{

uint YMMusicPlayer::bs_read_bits(uint nbits)
{
	// the bits are added at lsb, so they come out at msb:

	while (bits < nbits)
	{
		assert(bits <= 24);
		accu = (accu << 8) + input_file->read<uint8>();
		bits += 8;
	}
	bits -= nbits;
	uint rval = accu >> bits;
	accu -= rval << bits;
	return rval;
}

uint YMMusicPlayer::bs_read_number()
{
	assert(bits < 8);
	assert((accu >> bits) == 0); // accu must be clean outside valid bits

	// pull bits until the msb of the number is in the accu:
	while (accu == 0)
	{
		assert(bits <= 24);
		accu = input_file->read<uint8>();
		bits += 8;
	}

	// find the msbit:
	uint msbit = bits - 1;
	while (accu >> msbit == 0) msbit--;
	uint nbits = bits - msbit;

	// remove the preceeding 0-bits:
	bits = msbit + 1;
	assert((accu >> msbit) == 1);

	// read and return the number:
	return bs_read_bits(nbits);
}

YMMusicPlayer::BackrefBuffer::BackrefBuffer(RleCode* p, uint8 bits, uint8 aybits) :
	data(p),
	mask(uint16((1 << bits) - 1)),
	bits(bits),
	aybits(aybits)
{}

uint8 YMMusicPlayer::BackrefBuffer::next_value(YMMusicPlayer* instream)
{
	if (regcount--) { return regvalue; }

	if (backrefcount == 0)
	{
		bool is_backref = instream->bs_read_bits(1);
		int	 value		= int(instream->bs_read_bits(is_backref ? bits : aybits));
		int	 count		= int(instream->bs_read_number());

		if (is_backref) // LZ code:
		{
			assert(value >= 1 && value < 1 << bits);
			assert(count >= 1 && count <= UINT16_MAX);
			backrefoffset = uint16(value);
			backrefcount  = uint16(count);
		}
		else // RLE code:
		{
			assert(value >= 0 && value < 1 << aybits);
			assert(count >= 1 && count <= UINT8_MAX);
			data[index++ & mask] = RleCode {uint8(value), uint8(count)};
			regvalue			 = uint8(value);
			regcount			 = uint8(count - 1);
			return uint8(value);
		}
	}

	backrefcount -= 1;
	RleCode& vc	   = data[(index - backrefoffset) & mask];
	uint8	 count = vc.count;
	uint8	 value = vc.value;

	assert(value >= 0 && value < 1 << aybits);
	assert(count >= 1 && count <= UINT8_MAX);
	data[index++ & mask] = RleCode {value, count};
	regvalue			 = value;
	regcount			 = count - 1;
	return value;
}


// ###################################################################


// size is close to 600 bytes plus 3 memory Ids:
static_assert(sizeof(YMMusicPlayer) <= 608);

static int ymm_callback(void* data) noexcept
{
	YMMusicPlayer* ymm_player = reinterpret_cast<YMMusicPlayer*>(data);
	return ymm_player->run();
}

YMMusicPlayer::YMMusicPlayer() : super(2000000, Mono, 50, 0.2f), AudioPlayer("*.ymm")
{
	Dispatcher::addWithDelay(ymm_callback, this, 100 * 1000);
}

YMMusicPlayer::~YMMusicPlayer()
{
	Dispatcher::removeHandler(ymm_callback, this);
	if (is_live) removeAudioSource(this);
	delete[] allocated_buffer;
}

void YMMusicPlayer::read_frame(uint8 regs[16])
{
	for (int r = 0; r < registers_per_frame; r++)
	{
		regs[r] = backref_buffers[r].next_value(this); //
	}
	if (regs[13] == 0x0f) regs[13] = 0xff;
}

int YMMusicPlayer::run() noexcept
{
	trace("YMMusicPlayer::run");

	if (queue.free() == 0) return 10 * 1000;

	try
	{
		if (input_file)
		{
			// we are playing :-)

			if (paused) return 100 * 1000; // we are not..

			assert(is_live);

			read_frame(queue[queue.wi].registers); // throws at eof
			queue[queue.wi].cmd = Queue::SetRegisters;
			__dmb();
			queue.wi++;

			if (++frames_played < num_frames) return 10 * 1000;

			if (repeat_file && next_file == nullptr && next_dir == nullptr)
			{
				input_file->setFpos(bitstream_start);
				bs_reset();
				uint8 dummy[16];
				for (uint frame = 0; frame < loop_frame; frame++) { read_frame(dummy); }
				frames_played = loop_frame;
			}
			else
			{
				input_file = nullptr; // close file
			}
		}
		else if (nextInputFile())
		{
			// we are not playing
			// but there's a music file to play:

			uint32 magic		   = input_file->read<uint32>();
			uint8  variant		   = input_file->read<uint8>();
			uint8  buffer_bits	   = input_file->read<uint8>();
			uint8  frame_rate	   = input_file->read<uint8>();
			registers_per_frame	   = input_file->read<uint8>();
			num_frames			   = input_file->read_LE<uint32>();
			loop_frame			   = input_file->read_LE<uint32>();
			float		ay_clock   = float(input_file->read_LE<uint32>());
			AyStereoMix stereo_mix = Mono; //TODO

			if (memcmp(&magic, "ymm!", 4)) throw "not a .ymm music file";
			if (variant != 2) throw "unknown .ymm variant";
			if (buffer_bits < 8 || buffer_bits > 14) throw "illegal window bits";
			if (frame_rate < 25 || frame_rate > 100) throw "illegal frame rate";
			if (registers_per_frame != 16) throw "illegal registers per frame";
			if (num_frames <= loop_frame) throw "illegal num_frames";
			if (ay_clock < 990000 || ay_clock > 4100000) throw "illegal ay_clock";

			cstr title	 = input_file->gets(0x0001);
			cstr author	 = input_file->gets(0x0001);
			cstr comment = input_file->gets(0x0001);
			logline("title:   %s", title);
			logline("author:  %s", author);
			logline("comment: %s", comment);

			uint32 rbusz	= input_file->read_LE<uint32>();
			bitstream_start = input_file->getFpos();

			if (this->buffer_bits != buffer_bits)
			{
				delete[] allocated_buffer;
				allocated_buffer  = nullptr;
				this->buffer_bits = 0;
				allocated_buffer  = new RleCode[1 << buffer_bits];
				this->buffer_bits = buffer_bits;
			}

			assert(buffer_bits >= 8 && buffer_bits <= 14);
			assert(registers_per_frame == 16);
			assert(NELEM(backref_buffers) == 16);

			RleCode* p = allocated_buffer;
			for (int r = 0; r < 16; r++)
			{
				if (uint8 sz = (rbusz >> r >> r) & 0x03)
				{
					sz += buffer_bits - 4 - 2;
					backref_buffers[r] = BackrefBuffer {p, sz, ayRegisterNumBits[r]};
					p += 1 << sz;
				}
				else
				{
					static RleCode dead_RleCode;
					backref_buffers[r] = BackrefBuffer {&dead_RleCode, 0, ayRegisterNumBits[r]};
				}
			}
			if (p != allocated_buffer + (1 << buffer_bits)) throw "illegal buffer assignment";

			// start playing by sending the setup command:
			frames_played = 0;
			bs_reset();

			super::reset(ay_clock, stereo_mix, frame_rate);

			if (!is_live) addAudioSource(this);
			is_live = true;
		}
		else
		{
			// we are not playing
			// there is nothing to play:
			assert(!input_file);
			assert(!current_dir);

			if (is_live)
			{
				if (avail() == 0)
				{
					is_live = false;
					removeAudioSource(this);
				}
			}
			else return 100 * 1000;
		}
	}
	catch (Error e)
	{
		logline("YMMusicPlayer: %s", e);
		input_file = nullptr; // close file
	}
	catch (...)
	{
		logline("YMMusicPlayer: unknown exception");
		input_file = nullptr; // close file
	}

	return 10 * 1000;
}

void YMMusicPlayer::setVolume(float v)
{
	debugstr("YMMusicPlayer::setVolume %f\n", double(v));
	super::setVolume(v);
}

} // namespace kilipili::Audio

/*





























*/
