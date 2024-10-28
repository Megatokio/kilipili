// Copyright (c) 1995 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "AudioSample.h"
#include "common/basic_math.h"
#include <functional>

namespace kio::Audio
{

// actual HW sample frequency used by AudioController:
extern float hw_sample_frequency;


constexpr uint8 ayRegisterBitMasks[] = {0xff, 0x0f, 0xff, 0x0f, 0xff, 0x0f, 0x1f, 0xff,
										0x1f, 0x1f, 0x1f, 0xff, 0xff, 0x0f, 0xff, 0xff};

constexpr uint8 ayRegisterResetValues[] = {0xff, 0x0f, 0xff, 0x0f, 0xff, 0x0f, 0x1f, 0x3f,
										   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff};


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
	using CC			= circular_int;
	using WritePortProc = const std::function<void(CC, bool, uint8)>;
	using ReadPortProc	= const std::function<uint8(CC, bool)>;

	enum StereoMix { mono, abc_stereo, acb_stereo };

	// create instance for hw_sample_frequency:
	Ay38912(float frequency, StereoMix = mono, float volume = 0.5) noexcept;

	// calculate clock which will result in a clock not slower than the requested one:
	static float nextHigherClock(float clock, float sample_clock = hw_sample_frequency) noexcept;

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
	float getClock() const noexcept { return clock_frequency; }
	float getActualClock() const noexcept { return sample_frequency * ccx_per_sample / (1 << ccx_fract_bits); }
	void  setStereoMix(StereoMix) noexcept;

	// adjust the clock cycle counter to match what your application did:
	void shiftTimebase(int delta_cc) noexcept; // subtract delta_cc from the current clock cycle
	void resetTimebase() noexcept;			   // reset the clock cycle to 0

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
	CC audioBufferStart(AudioSample<num_channels>* buffer, uint num_samples) noexcept;

	// finish audio output into the audio buffer.
	void audioBufferEnd() noexcept;

private:
	// the emulation uses internally 24.8 fixed point values for the SP0256 clock frequency
	// to allow integer calculations when resampling the output to the hw_frequency with little error.
	static constexpr int ccx_fract_bits = 8;
	using CCx							= circular_int; // 24.8 fixed point

	struct Noise
	{
		static constexpr int predivider = 16 << ccx_fract_bits;

		int	  reload;	// reload value
		CCx	  when;		// next tick to toggle output
		int32 shiftreg; // random number shift register; output = bit 0

		void setPeriod(uint8);
		void trigger();
		void fastForward(CCx now);
		void reset(CCx now);
	};

	struct Envelope
	{
		static constexpr int predivider = 16 << ccx_fract_bits;

		int	  reload;	 // reload value
		CCx	  when;		 // next tick to inc/dec volume
		bool  repeat;	 // repeat ramping
		bool  invert;	 // toggle ramping direction
		uint8 index;	 // current output state: volume 0 ... 15
		int8  direction; // ramping up/down/off

		void setShape(CCx, uint8);
		void setPeriod(uint16);
		void trigger();
		void reset(CCx now);
		void fastForward(CCx now);
	};

	struct Channel
	{
		static constexpr int predivider = 8 << ccx_fract_bits; // 16 but toggle every 1/2 period

		int	  reload;		// reload value
		CCx	  when;			// next tick to toggle state
		bool  sound_enable; // from mixer control register
		bool  sound_in;		// from sound generator
		bool  noise_enable; // from mixer control register
		uint8 volume;		// volume of this channel

		void setVolume(uint8);
		void setPeriod(uint16);
		void trigger();
		void reset(CCx now);
		void fastForward(CCx now);
	};

	Channel	 channel_A, channel_B, channel_C;
	Noise	 noise;
	Envelope envelope;

	StereoMix stereo_mix;

	uint  ay_reg_nr = 0; // selected register
	uint8 ay_reg[16];	 // registers

	float volume;	   // volume of all channels combined: 0 .. 1.0
	int	  log_vol[16]; // logarithmic volume table prescaled for faster resampling

	float clock_frequency;	// ay_clock
	float sample_frequency; // normally hw_sample_frequency
	int	  ccx_per_sample;	// frequency / hw_frequency

	AudioSample<num_channels>*	   output_buffer  = nullptr;
	AudioSample<num_channels, int> current_value  = 0; // current output of chip
	AudioSample<num_channels, int> current_sample = 0; // sample under construction

	CCx	  ccx_at_sos;		  // cc at start of sample
	CCx	  ccx_now;			  // current cc
	int64 ccx_buffer_end = 0; // cc at end of buffer = buffer_size * cc_per_sample;

	int	 output_of(Channel&) noexcept;
	void run_up_to_cycle(CCx ccx) noexcept;
	void shift_timebase(int delta_ccx) noexcept;
};


extern template class Ay38912<1>;
extern template class Ay38912<2>;


} // namespace kio::Audio


/*



















































































*/
