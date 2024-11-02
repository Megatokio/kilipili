// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "AudioController.h"
#include "AudioSource.h"
#include "Ay38912.h"
#include "Devices/File.h"
#include "basic_math.h"

namespace kio::Audio
{

class YMMusicPlayer : public AudioSource<hw_num_channels>
{
public:
	YMMusicPlayer();
	~YMMusicPlayer() override;

	int	 run();
	uint getAudio(AudioSample<hw_num_channels>* buffer, uint num_frames) noexcept override;
	void setSampleRate(float /*new_sample_frequency*/) noexcept override;

	void play(cstr fpath, bool loop = false);
	void playDirectory(cstr dpath, bool loop = false);

	void stop();
	void stopAfterSong();
	void pause(bool p = true) { paused = p; }
	void resume() { paused = false; } // after pause or stop
	void skip();					  // resume next song if playing from dir
	void setVolume(float);

	using AyPlayer = Ay38912<hw_num_channels>;
	AyPlayer ay;

	// while stopped the current file & dir can be remembered:
	cstr next_file = nullptr;
	cstr next_dir  = nullptr;

	// the current file and directory (if any) while playing:
	Devices::FilePtr	  ymmusic_file;
	Devices::DirectoryPtr ymmusic_dir;

	bool is_live	 = false; // is connected to audio controller
	bool paused		 = false;
	bool repeat_file = false;
	bool repeat_dir	 = false;

	// data from current file:
	int8   frame_rate = 50;
	uint8  registers_per_frame;
	uint8  stereo_mix = AyPlayer::mono;
	uint32 num_frames;
	uint32 loop_frame;
	float  ay_clock;

	int32  cc_per_frame;	  // calc. from ay_clock and frame_rate
	CC	   cc_next {0};		  // cc for next register update
	uint32 frames_played = 0; // frame counter

	struct QueueData
	{
		uint8 registers[15];
		uint8 what; // 0=registers, 1=reset
	};
	struct Queue
	{
		static constexpr uint SZ = 8;

		QueueData  buffer[SZ];
		uint8	   ri = 0, wi = 0;
		QueueData& operator[](uint i) noexcept { return buffer[i & (SZ - 1)]; }
		uint	   avail() noexcept { return uint8(wi - ri); }
		uint	   free() noexcept { return SZ - avail(); }
	};

	Queue queue;
};


} // namespace kio::Audio


/*








































*/
