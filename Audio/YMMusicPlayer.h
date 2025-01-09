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
	using FilePtr	   = Devices::FilePtr;
	using DirectoryPtr = Devices::DirectoryPtr;
	using AyPlayer	   = Ay38912<hw_num_channels>;

	YMMusicPlayer();
	~YMMusicPlayer() override;

	int	 run() noexcept;
	uint getAudio(AudioSample<hw_num_channels>* buffer, uint num_frames) noexcept override;
	void setSampleRate(float /*new_sample_frequency*/) noexcept override;

	void play(cstr fpath);
	void playDirectory(cstr dpath);
	void play(cstr fpath, bool loop);
	void playDirectory(cstr dpath, bool loop);

	void stop();
	void stopAfterSong();
	void pause(bool p = true) { paused = p; }
	void resume() { paused = false; } // after pause or stop
	void skip();					  // resume next song if playing from dir
	void setVolume(float);

	AyPlayer ay;

	// while stopped the current file & dir can be remembered:
	cstr next_file = nullptr;
	cstr next_dir  = nullptr;

	// the current file and directory (if any) while playing:
	//FilePtr	 ymmusic_file; --> bitstream.infile
	DirectoryPtr ymmusic_dir;

	bool is_live	 = false; // is connected to audio controller
	bool paused		 = false;
	bool repeat_file = false;
	bool repeat_dir	 = false;

	// data from current file:
	uint8  buffer_bits = 0;
	int8   frame_rate  = 50;
	uint8  registers_per_frame;
	uint8  stereo_mix = AyPlayer::mono;
	uint32 num_frames;
	uint32 loop_frame;
	float  ay_clock;
	uint32 bitstream_start;

	int32  cc_per_frame;	  // calc. from ay_clock and frame_rate
	CC	   cc_next {0};		  // cc for next register update
	uint32 frames_played = 0; // frame counter

	struct QueueData
	{
		uint8 registers[16];
		uint8 what; // 0=registers, 1=reset
	};
	struct Queue
	{
		static constexpr uint SZ = 4;

		QueueData  buffer[SZ];
		uint8	   ri = 0, wi = 0;
		QueueData& operator[](uint i) noexcept { return buffer[i & (SZ - 1)]; }
		uint	   avail() noexcept { return uint8(wi - ri); }
		uint	   free() noexcept { return SZ - avail(); }
	};

	Queue queue;

	// decoder data:
	struct BitStream
	{
		FilePtr infile;
		uint	accu = 0; // accumulator
		uint	bits = 0; // remaining num bits in accu

		uint read_bits(uint nbits);
		uint read_number();
		void reset() { accu = bits = 0; }
	};
	struct RleCode
	{
		uint8 value;
		uint8 count;
	};
	struct BackrefBuffer
	{
		RleCode* data		   = nullptr; // this could be a uint16 offset into allocated_buffer
		uint16	 mask		   = 0;		  // data.size - 1
		uint16	 index		   = 0;
		uint8	 bits		   = 0; // data.size = 1 << bits
		uint8	 aybits		   = 0;
		uint8	 regvalue	   = 0;
		uint8	 regcount	   = 0;
		uint16	 backrefoffset = 0;
		uint16	 backrefcount  = 0;

		BackrefBuffer() = default;
		BackrefBuffer(RleCode* p, uint8 bits, uint8 aybits);
		uint8 next_value(BitStream& instream);
	};

	BitStream	  bitstream;
	RleCode*	  allocated_buffer = nullptr;
	BackrefBuffer backref_buffers[16];

private:
	void read_frame(QueueData&);
};


} // namespace kio::Audio


/*








































*/
