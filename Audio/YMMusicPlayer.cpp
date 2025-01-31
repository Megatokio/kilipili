// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "YMMusicPlayer.h"
#include "Devices/Directory.h"
#include "Devices/FileSystem.h"
#include "Trace.h"
#include "common/cdefs.h"
#include "cstrings.h"
#include "utilities/Logger.h"
#include <hardware/sync.h>

namespace kio::Audio
{


uint YMMusicPlayer::BitStream::read_bits(uint nbits)
{
	// the bits are added at lsb, so they come out at msb:

	while (bits < nbits)
	{
		assert(bits <= 24);
		accu = (accu << 8) + infile->read<uint8>();
		bits += 8;
	}
	bits -= nbits;
	uint rval = accu >> bits;
	accu -= rval << bits;
	return rval;
}

uint YMMusicPlayer::BitStream::read_number()
{
	assert(bits < 8);
	assert((accu >> bits) == 0); // accu must be clean outside valid bits

	// pull bits until the msb of the number is in the accu:
	while (accu == 0)
	{
		assert(bits <= 24);
		accu = infile->read<uint8>();
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
	return read_bits(nbits);
}

YMMusicPlayer::BackrefBuffer::BackrefBuffer(RleCode* p, uint8 bits, uint8 aybits) :
	data(p),
	mask(uint16((1 << bits) - 1)),
	bits(bits),
	aybits(aybits)
{}

uint8 YMMusicPlayer::BackrefBuffer::next_value(BitStream& instream)
{
	if (regcount--) { return regvalue; }

	if (backrefcount == 0)
	{
		bool is_backref = instream.read_bits(1);
		int	 value		= int(instream.read_bits(is_backref ? bits : aybits));
		int	 count		= int(instream.read_number());

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
static_assert(sizeof(YMMusicPlayer) <= 624);


YMMusicPlayer::YMMusicPlayer() : ay(2000000, AyPlayer::mono, 0.2f) {}

YMMusicPlayer::~YMMusicPlayer()
{
	delete[] next_file;
	delete[] next_dir;
	delete[] allocated_buffer;
}

void YMMusicPlayer::setSampleRate(float new_sample_frequency) noexcept
{
	// callback by the AudioController

	trace(__func__);

	ay.setSampleRate(new_sample_frequency);
}

uint YMMusicPlayer::getAudio(AudioSample<hw_num_channels>* buffer, uint num_frames) noexcept
{
	// callback by the AudioController

	trace(__func__);

	if (queue.avail())
	{
		QueueData& regs = queue[queue.ri];
		assert(regs.what <= 1);

		if unlikely (regs.what == 1)
		{
			ay.setStereoMix(AyPlayer::StereoMix(stereo_mix));
			ay.setClock(ay_clock);
			assert(ay_clock == ay.getClock());
			cc_per_frame = int32(ay_clock + frame_rate / 2) / frame_rate;
		}

		CC cc_buffer_end = ay.audioBufferStart(buffer, num_frames);
		if (cc_next < cc_buffer_end)
		{
			ay.setRegister(cc_next, 0, regs.registers[0]);
			uint last_register = regs.registers[13] == 0x0f ? 12 : 13;
			for (uint i = 1; i <= last_register; i++) ay.setRegister(i, regs.registers[i]);
			cc_next += cc_per_frame;
			__dmb();
			queue.ri++;
		}
		ay.audioBufferEnd();
		return num_frames;
	}
	else if (bitstream.infile)
	{
		ay.audioBufferStart(buffer, num_frames);
		ay.audioBufferEnd();
		return num_frames;
	}
	else
	{
		is_live = false;
		return 0; // remove me
	}
}

void YMMusicPlayer::read_frame(QueueData& d)
{
	for (int r = 0; r < registers_per_frame; r++)
	{
		d.registers[r] = backref_buffers[r].next_value(bitstream); //
	}

	d.what = 0;
}

int YMMusicPlayer::run() noexcept
{
	trace("YMMusicPlayer::run");

	if (queue.free() == 0) return 10 * 1000;

	try
	{
		if (bitstream.infile)
		{
			// we are playing :-)

			if (paused) return 100 * 1000; // we are not.. TODO: silence AY

			read_frame(queue[queue.wi]); // throws at eof
			__dmb();
			queue.wi++;

			if unlikely (!is_live)
			{
				is_live = true;
				//AudioController::getRef().addAudioSource(new HF_DC_Filter<hw_num_channels>(this));
				AudioController::getRef().addAudioSource(this);
			}

			if (++frames_played < num_frames) return 10 * 1000;

			if (repeat_file && next_file == nullptr && next_dir == nullptr)
			{
				bitstream.infile->setFpos(bitstream_start);
				bitstream.reset();
				QueueData dummy;
				for (uint frame = 0; frame < loop_frame; frame++) { read_frame(dummy); }
				frames_played = loop_frame;
			}
			else
			{
				bitstream.infile = nullptr; // close file
			}
		}
		else if (next_file)
		{
			// we are not playing
			// but there's a music file to play:

			logline("now playing: %s", next_file);

			cstr fname = dupstr(next_file);
			delete[] next_file;
			next_file			 = nullptr; // if open() fails we don't want to come here again
			FilePtr ymmusic_file = Devices::openFile(fname);

			uint32 magic		= ymmusic_file->read<uint32>();
			uint8  variant		= ymmusic_file->read<uint8>();
			uint8  buffer_bits	= ymmusic_file->read<uint8>();
			frame_rate			= ymmusic_file->read<int8>();
			registers_per_frame = ymmusic_file->read<uint8>();
			num_frames			= ymmusic_file->read_LE<uint32>();
			loop_frame			= ymmusic_file->read_LE<uint32>();
			ay_clock			= float(ymmusic_file->read_LE<uint32>());
			stereo_mix			= AyPlayer::mono; //TODO

			if (memcmp(&magic, "ymm!", 4)) throw "not a .ymm music file";
			if (variant != 2) throw "unknown .ymm variant";
			if (buffer_bits < 8 || buffer_bits > 14) throw "illegal window bits";
			if (frame_rate < 25 || frame_rate > 100) throw "illegal frame rate";
			if (registers_per_frame != 16) throw "illegal registers per frame";
			if (num_frames <= loop_frame) throw "illegal num_frames";
			if (ay_clock < 990000 || ay_clock > 4100000) throw "illegal ay_clock";

			cstr title	 = ymmusic_file->gets(0x0001);
			cstr author	 = ymmusic_file->gets(0x0001);
			cstr comment = ymmusic_file->gets(0x0001);
			logline("title:   %s", title);
			logline("author:  %s", author);
			logline("comment: %s", comment);

			uint32 rbusz	= ymmusic_file->read_LE<uint32>();
			bitstream_start = ymmusic_file->getFpos();

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

			// start playing by sending the reset register values:
			frames_played	 = 0;
			bitstream.infile = ymmusic_file;
			bitstream.reset();

			QueueData& d = queue[queue.wi];
			memcpy(d.registers, ayRegisterResetValues, 16); // reset values
			d.what = 1;										// reset the AY chip
			__dmb();
			queue.wi++;
		}
		else if (ymmusic_dir)
		{
			// we are not playing and there is no file requested to play
			// but we are playing from a directory:

			Devices::FileInfo finfo = ymmusic_dir->next("*.ymm");
			if (finfo)
			{
				next_file = newcopy(catstr(ymmusic_dir->getPath(), "/", finfo.fname)); //
			}
			else if (repeat_dir && next_dir == nullptr)
			{
				ymmusic_dir->rewind(); //
			}
			else
			{
				ymmusic_dir = nullptr; // close dir
			}
		}
		else if (next_dir)
		{
			// we are not playing and there is no file requeste to play
			// and we are not playing from a directory
			// but there is a request for a directory to play:

			cstr dpath = dupstr(next_dir);
			delete[] next_dir;
			next_dir	= nullptr; // we don't want to come back to here if open() fails
			ymmusic_dir = Devices::openDir(dpath);
			ymmusic_dir->rewind();
		}
		else return 100 * 1000;
	}
	catch (Error e)
	{
		logline("YMMusicPlayer: %s", e);
		if (bitstream.infile)
		{
			bitstream.infile = nullptr; // close file
			assert(queue.free());
			QueueData& qd = queue[queue.wi];
			memcpy(qd.registers, ayRegisterResetValues, sizeof(qd.registers));
			qd.what = 0;
			__dmb();
			queue.wi++;
		}
	}
	catch (...)
	{
		logline("YMMusicPlayer: unknown exception");
	}

	return 10 * 1000;
}

void YMMusicPlayer::play(cstr fpath)
{
	delete[] next_file;
	next_file = newcopy(Devices::makeAbsolutePath(fpath));
	debugstr("play file: %s\n", next_file);
}

void YMMusicPlayer::playDirectory(cstr dpath)
{
	delete[] next_dir;
	next_dir = newcopy(Devices::makeAbsolutePath(dpath));
	debugstr("play dir: %s\n", next_dir);
}

void YMMusicPlayer::play(cstr fpath, bool loop)
{
	play(fpath);
	repeat_file = loop;
}

void YMMusicPlayer::playDirectory(cstr dpath, bool loop)
{
	playDirectory(dpath);
	repeat_dir = loop;
}

void YMMusicPlayer::skip()
{
	bitstream.infile = nullptr; //
}

void YMMusicPlayer::stop()
{
	skip();
	stopAfterSong();
}

void YMMusicPlayer::stopAfterSong()
{
	ymmusic_dir = nullptr;
	delete[] next_dir;
	delete[] next_file;
	next_dir	= nullptr;
	next_file	= nullptr;
	repeat_file = false;
	paused		= false;
}

void YMMusicPlayer::setVolume(float v)
{
	debugstr("YMMusicPlayer::setVolume %f\n", double(v));
	ay.setVolume(v);
}

} // namespace kio::Audio

/*





























*/
