// Copyright (c) 2014 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#pragma once
#include "AudioSample.h"
#include "common/basic_math.h"
#include "common/standard_types.h"

namespace kio::Audio
{

// actual HW sample frequency used by AudioController:
extern float hw_sample_frequency;


/*	_________________________________________________________________________
	SP0256 speech synthesizer chip emulation
	template class for output into a mono or stereo buffer.

	this class does not support synchronization of read & write access.
	you may want to use the SP0256_AudioSource which uses this class.

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
class SP0256
{
public:
	using CC = circular_int;

	// create instance for hw_sample_frequency:
	SP0256(float frequency = 3.12e6f, float volume = 0.5f) noexcept;

	// setup/save/restore:
	// these should be called outside audioBufferStart() .. end()
	void  setVolume(float volume) noexcept;
	void  setClock(float frequency) noexcept;
	void  setSampleRate(float frequency) noexcept;
	void  reset() noexcept;
	bool  acceptsNextCommand() const noexcept { return !command_valid; }
	bool  isSpeaking() const noexcept { return !stand_by; }
	float getClock() const noexcept { return frequency; }
	void  writeCommand(uint sp0256_cmd) noexcept;

	// adjust the clock cycle counter to match what your application did:
	void shiftTimebase(int delta_cc) noexcept; // subtract delta_cc from the current clock cycle
	void resetTimebase() noexcept;			   // reset the clock cycle to 0

	// runtime:
	// these functions must only be called after audioBufferStart() until audioBufferEnd()!
	// the passed CC should not exceed the duration of the audio buffer!
	// the clock cycle can be left ever increasing and happy wrap around or
	// be reset with shiftTimebase() or resetTimebase()
	void reset(CC) noexcept;
	bool acceptsNextCommand(CC) noexcept;
	bool isSpeaking(CC) noexcept;
	void writeCommand(CC, uint sp0256_cmd) noexcept;

	// start next audio buffer:
	// returns the clock cycle at the end of the buffer up to which writeCommand()s can be issued.
	// the increment is not constant but will jitter due to resampling.
	// at 3.12MHz and 44.1kHz and num_samples=64 CC increments by ~4528.
	CC audioBufferStart(AudioSample<num_channels>* buffer, uint num_samples) noexcept;

	// finish audio output into the audio buffer.
	void audioBufferEnd() noexcept;

private:
	// the emulation uses internally 24.8 fixed point values for the SP0256 clock frequency
	// to allow integer calculations when resampling the output to the hw_frequency with little error.
	static constexpr int ccx_fract_bits = 8; // 24.8 fixed point
	using CCx							= circular_int;

	float volume;		 // 0 .. 1.0
	int	  amplification; // volume setting (incl. all other factors)

	float frequency;	  // sp0256_clock
	float hw_frequency;	  // normally hw_sample_frequency
	int	  ccx_per_sample; // frequency / hw_frequency

	AudioSample<num_channels>* output_buffer  = nullptr;
	int						   current_value  = 0; // current output of chip
	int						   current_sample = 0; // sample under construction

	CCx	  ccx_at_sos {0};	  // cc at start of output sample
	CCx	  ccx_now {0};		  // cc at current output position
	int64 ccx_buffer_end {0}; // cc at end of output buffer = buffer_size * cc_per_sample;
	CCx	  ccx_next {0};		  // cc of next SP0256 sample

	int	 next_sample() noexcept;
	void run_up_to_cycle(CCx ccx) noexcept;
	void shift_timebase(int delta_ccx) noexcept;


	// coroutine state machine
	uint sm_state; // for coroutine macros

	// 17 sound and filter registers:
	uint  repeat;		  // 6 bit: ≥ 1
	uint8 pitch;		  // 8 bit: 0 -> white noise
	uint8 amplitude;	  // 8 bit: bit[7…5] = exponent, bit[4…0] = mantissa
	uint8 c[12];		  // 8 bit: filter coefficients b and f
	int8  pitch_incr;	  // Δ update value applied to pitch after each period
	int8  amplitude_incr; // Δ update value applied to amplitude after each period

	int	   _c[12];	  // int10:  filter coefficients b and f (bereits umgerechnet)
	int	   _z[12];	  // feedback values
	uint16 _shiftreg; // noise
	uint   _i;

	// microsequencer registers:
	uint mode; // 2 bit	from SETMODE
	// uint repeat_prefix;// 2 bit	from SETMODE	(already shifted left 4 bits)
	uint page;			  // 4 bit	(already bit-swapped and shifted left 12 bits)
	uint pc;			  // 16 bit
	uint stack;			  // 16 bit single level return "stack"
	uint command;		  // 8 bit current/next command
	uint current_command; // 8 bit current command

	bool stand_by;		// 1 = stand by (utterance completed)
	bool command_valid; // 1 = command valid == !LRQ (load request)

	// speech rom AL2:		((presumably "american language" vs.2))
	uint byte; // current/last byte read from rom, remaining valid bits are right-aligned
	uint bits; // number of valid bits remaining

	void cmdSetPage(uint opcode) noexcept; // or RTS
	void cmdLoadAll() noexcept;
	void cmdLoad2C(uint instr) noexcept;
	void cmdLoad2() noexcept;
	void cmdSetMsb35A(uint instr) noexcept;
	void cmdSetMsb3() noexcept;
	void cmdLoad4() noexcept;
	void cmdSetMsb5() noexcept;
	void cmdSetMsb6() noexcept;
	void cmdJmp(uint opcode) noexcept;
	void cmdSetMode(uint opcode) noexcept;
	void cmdDelta9() noexcept;
	void cmdSetMsbA() noexcept;
	void cmdJsr(uint opcode) noexcept;
	void cmdLoadC() noexcept;
	void cmdDeltaD() noexcept;
	void cmdLoadE() noexcept;
	void cmdPause() noexcept;

	void disassAllophones() noexcept;
	void logSetPage(uint opcode) noexcept; // or RTS
	void logLoadAll() noexcept;
	void logLoad2C(uint instr) noexcept;
	void logLoad2() noexcept;
	void logSetMsb35A(uint instr) noexcept;
	void logSetMsb3() noexcept;
	void logLoad4() noexcept;
	void logSetMsb5() noexcept;
	void logSetMsb6() noexcept;
	void logJmp(uint opcode) noexcept;
	void logSetMode(uint opcode) noexcept;
	void logDelta9() noexcept;
	void logSetMsbA() noexcept;
	void logJsr(uint opcode) noexcept;
	void logLoadC() noexcept;
	void logDeltaD() noexcept;
	void logLoadE() noexcept;
	void logPause() noexcept;

	uint8 next8() noexcept;
	uint8 nextL(uint n) noexcept;
	uint8 nextR(uint n) noexcept;
	int8  nextSL(uint n) noexcept;
	int8  nextSR(uint n) noexcept;
};


extern template class SP0256<1>;
extern template class SP0256<2>;

} // namespace kio::Audio


/*



































*/
