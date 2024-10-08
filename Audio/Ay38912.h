// Copyright (c) 1995 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "AudioSample.h"
#include <functional>

namespace kio::Audio
{

// actual HW sample frequency used by AudioController:
extern float hw_sample_frequency;


/*	_______________________________________________________________________________________
	ay-3-8912 sound chip emulation
	template class for output into a mono or stereo buffer.

	this class does not support synchronization of read & write access.
	you may want to use the Ay38912_AudioSource which uses this class.

	use:
	- create instance
	- setup registers etc. 
	- while more commands to go:
		- audioBufferStart()
		- while timestamp of command < buffer end time
			- writeRegister(command)
		- audioBufferEnd()
		- play buffer
*/
template<uint num_channels>
class Ay38912
{
public:
	using CC			= int;
	using CC256			= int; // 24.8 fixed point
	using WritePortProc = const std::function<void(CC, bool, uint8)>;
	using ReadPortProc	= const std::function<uint8(CC, bool)>;

	enum StereoMix { mono, abc_stereo, acb_stereo };

	// create instance for hw_sample_frequency:
	Ay38912(float frequency, StereoMix = mono, float volume = 0.5) noexcept;

	// setup/save/restore:
	// these should be called outside audioBufferStart() .. end()
	void  setVolume(float volume) noexcept;
	void  setClock(float frequency) noexcept;
	void  setSampleRate(float frequency) noexcept;
	void  setRegister(uint r, uint8 n) noexcept;
	void  setRegNr(uint n) noexcept { ay_reg_nr = n & 0x0f; }
	uint8 getRegister(uint n) const noexcept { return ay_reg[n & 0x0f]; }
	uint  getRegNr() const noexcept { return ay_reg_nr; }
	void  reset() noexcept;
	float getClock() const noexcept { return frequency; }

	// runtime:
	// these functions must only be called after audioBufferStart() until audioBufferEnd()!
	// the passed CC should not exceed the duration of the audio buffer!
	void reset(CC, WritePortProc&); // reset at clock cycle cc
	void reset(CC) noexcept;		// reset at clock cycle cc

	void setRegister(CC, uint r, uint8 n) noexcept;
	void setRegister(CC, uint r, uint8 n, WritePortProc&);

	void writeRegister(CC cc, uint8 n, WritePortProc& cb) { setRegister(cc, ay_reg_nr, n, cb); }
	void writeRegister(CC cc, uint8 n) noexcept { setRegister(cc, ay_reg_nr, n); }

	uint8 readRegister(CC, ReadPortProc&);						  // read from selected register at cc
	uint8 readRegister(CC) noexcept { return ay_reg[ay_reg_nr]; } // read from selected register at cc


	// start next audio buffer:
	// returns the clock cycle at the end of the buffer as a 24.8 fixed point number.
	// use this to increment your ay clock cycle in sync with the audio output.
	CC256 audioBufferStart(AudioSample<num_channels>* buffer, uint num_samples) noexcept;

	// finish audio output into the audio buffer.
	void audioBufferEnd() noexcept;

private:
	// the emulation uses internally a clock frequency 256 times of the actual clock frequency.
	// this may also be viewed as using 24.8 fixed point numbers.
	// this is to allow integer calculations when resampling the output to the hw_sample_frequency.
	static constexpr int oversampling = 256;

	struct Noise
	{
		static constexpr CC predivider = 16 * oversampling;

		CC	  reload;	// reload value
		CC	  when;		// next tick to toggle output
		int32 shiftreg; // random number shift register; output = bit 0

		void setPeriod(uint8);
		void trigger();
	};

	struct Envelope
	{
		static constexpr CC predivider = 16 * oversampling;

		CC	  reload;	 // reload value
		CC	  when;		 // next tick to inc/dec volume
		bool  repeat;	 // repeat ramping
		bool  invert;	 // toggle ramping direction
		uint8 index;	 // current output state: volume 0 ... 15
		int8  direction; // ramping up/down/off

		void setShape(CC, uint8);
		void setPeriod(uint16);
		void trigger();
	};

	struct Channel
	{
		static constexpr CC predivider = 8 * oversampling; // 16 but toggle every 1/2 period

		CC	  reload;		// reload value
		CC	  when;			// next tick to toggle state
		bool  sound_enable; // from mixer control register
		bool  sound_in;		// from sound generator
		bool  noise_enable; // from mixer control register
		uint8 volume;		// volume of this channel if not use envelope

		void setVolume(uint8);
		void setPeriod(uint16);
		void trigger();
	};

	Channel	 channel_A, channel_B, channel_C;
	Noise	 noise;
	Envelope envelope;

	StereoMix stereo_mix;

	uint  ay_reg_nr = 0; // selected register
	uint8 ay_reg[16];	 // registers

	float volume;	   // volume of all channels combined: 0 .. 1.0
	int	  log_vol[16]; // logarithmic volume table prescaled for faster resampling

	float frequency;	 // ay_clock
	float hw_frequency;	 // normally hw_sample_frequency
	CC256 cc_per_sample; // frequency * oversampling / hw_frequency

	AudioSample<num_channels>*	   output_buffer  = nullptr;
	AudioSample<num_channels, int> current_value  = 0; // current output of chip
	AudioSample<num_channels, int> current_sample = 0; // sample under construction

	CC256 cc_at_sos = 0; // cc at start of sample
	CC256 cc_now	= 0; // current cc
	CC256 cc_end	= 0; // cc at end of buffer = buffer_size * cc_per_sample;

	int	 output_of(Channel&) noexcept;
	void run_up_to_cycle(CC256 cc) noexcept;
};


extern template class Ay38912<1>;
extern template class Ay38912<2>;


} // namespace kio::Audio

/*



















































































*/
