// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "YMMusicPlayer.h"
#include "Devices/Directory.h"
#include "Devices/FileSystem.h"
#include "common/cdefs.h"
#include "cstrings.h"
#include "utilities/Logger.h"
#include <hardware/sync.h>


namespace kio::Audio
{

YMMusicPlayer::YMMusicPlayer() : ay(2000000, AyPlayer::mono, 0.2f) {}

YMMusicPlayer::~YMMusicPlayer()
{
	delete[] next_file;
	delete[] next_dir;
}

void YMMusicPlayer::setSampleRate(float new_sample_frequency) noexcept { ay.setSampleRate(new_sample_frequency); }

uint YMMusicPlayer::getAudio(AudioSample<hw_num_channels>* buffer, uint num_frames) noexcept
{
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
			uint last_register = regs.registers[13] == 0xff ? 12 : 13;
			for (uint i = 1; i <= last_register; i++) ay.setRegister(i, regs.registers[i]);
			cc_next += cc_per_frame;
			__dmb();
			queue.ri++;
		}
		ay.audioBufferEnd();
		return num_frames;
	}
	else
	{
		is_live = false;
		return 0;
	}
}

struct YMMHeader
{
	char   magic[4];
	uint8  format_variant;
	uint8  flags;
	uint8  frame_rate;
	uint8  registers_per_frame;
	uint32 num_frames;
	uint32 loop_frame;
	uint32 ay_clock;
};

int YMMusicPlayer::run()
{
	if (queue.free() == 0) return 0;

	if (ymmusic_file)
	{
		// we are playing :-)

		if (paused) return 0; // we are not..

		QueueData& d = queue[queue.wi];
		ymmusic_file->read(d.registers, registers_per_frame);
		d.what = 0;
		__dmb();
		queue.wi++;

		if unlikely (!is_live)
		{
			is_live = true;
			//AudioController::getRef().addAudioSource(new HF_DC_Filter<hw_num_channels>(this));
			AudioController::getRef().addAudioSource(this);
		}

		if (++frames_played < num_frames) return 0;

		if (repeat_file && next_file == nullptr && next_dir == nullptr)
		{
			uint32 offset = (frames_played - loop_frame) * registers_per_frame;
			frames_played = loop_frame;
			ymmusic_file->setFpos(ymmusic_file->getFpos() - offset);
		}
		else
		{
			ymmusic_file = nullptr; // close file
		}
	}
	else if (next_file)
	{
		// we are not playing
		// but there's a music file to play:

		cstr fname = dupstr(next_file);
		delete[] next_file;
		next_file	 = nullptr; // if open() fails we don't want to come here again
		ymmusic_file = Devices::openFile(fname);

		YMMHeader header;
		ymmusic_file->read(&header, sizeof(header));
		if (memcmp(header.magic, "ymrf", 4)) throw "not a ymrf music file";
		if (header.format_variant != 0) throw "unsupported ymrf variant";
		if (header.flags != 0) throw "unsupported ymrf flags";
		frame_rate			= int8(header.frame_rate);
		registers_per_frame = header.registers_per_frame;
		num_frames			= header.num_frames;
		loop_frame			= header.loop_frame;
		ay_clock			= float(header.ay_clock);
		stereo_mix			= AyPlayer::mono; //TODO

		if (registers_per_frame < 14 || registers_per_frame > 16) throw "illegal registers_per_frame";
		if (frame_rate < 25 || frame_rate > 100) throw "illegal frame_rate";
		if (num_frames <= loop_frame) throw "illegal num_frames";
		if (ay_clock < 990000 || ay_clock > 4100000) throw "illegal ay_clock";

		cstr title	 = ymmusic_file->gets(); // TODO line ends
		cstr author	 = ymmusic_file->gets(); // ""
		cstr comment = ymmusic_file->gets(); // ""
		logline("title:   %s", title);
		logline("author:  %s", author);
		logline("comment: %s", comment);

		frames_played = 0;

		QueueData& d = queue[queue.wi];
		memcpy(d.registers, ayRegisterResetValues, 14); // reset values
		d.what = 1;										// reset the AY chip
		__dmb();
		queue.wi++;
	}
	else if (ymmusic_dir)
	{
		// we are not playing and there is no file requested to play
		// but we are playing from a directory:

		Devices::FileInfo finfo = ymmusic_dir->next("*.ymrf");
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

	return 0;
}

void YMMusicPlayer::play(cstr fpath, bool loop)
{
	delete[] next_file;
	next_file	= newcopy(fpath);
	repeat_file = loop;
}

void YMMusicPlayer::playDirectory(cstr dpath, bool loop)
{
	delete[] next_dir;
	next_dir   = newcopy(dpath);
	repeat_dir = loop;
}

void YMMusicPlayer::skip()
{
	ymmusic_file = nullptr; //
}

void YMMusicPlayer::stop()
{
	ymmusic_file = nullptr;
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

void YMMusicPlayer::setVolume(float v) { ay.setVolume(v); }

} // namespace kio::Audio

/*





























*/
