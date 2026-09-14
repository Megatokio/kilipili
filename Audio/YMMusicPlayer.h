// Copyright (c) 2024 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Audio.h"
#include "AudioPlayer.h"
#include "Ay38912.h"


namespace kilipili::Audio
{

class YMMusicPlayer : public Ay38912_player<hw_num_channels>, public AudioPlayer
{
public:
	using super = Ay38912_player<hw_num_channels>;

	YMMusicPlayer();
	~YMMusicPlayer() override;

	int	 run() noexcept;
	void setVolume(float);

private:
	bool is_live = false; // is connected to audio controller

	// data from current file:
	uint8  buffer_bits = 0;
	uint8  registers_per_frame;
	uint32 num_frames;
	uint32 loop_frame;
	uint32 bitstream_start;

	int32  cc_per_frame;	  // calc. from ay_clock and frame_rate
	CC	   cc_next {0};		  // cc for next register update
	uint32 frames_played = 0; // frame counter

	// BitStream decoder data:
	uint accu = 0; // accumulator
	uint bits = 0; // remaining num bits in accu
	// BitStream decoder:
	uint bs_read_bits(uint nbits);
	uint bs_read_number();
	void bs_reset() { accu = bits = 0; }

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
		uint8 next_value(YMMusicPlayer* bitstream);
	};

	RleCode*	  allocated_buffer = nullptr;
	BackrefBuffer backref_buffers[16];

private:
	void read_frame(uint8 regs[16]);
};


} // namespace kilipili::Audio


/*








































*/
