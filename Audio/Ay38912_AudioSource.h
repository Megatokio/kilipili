// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "AudioSource.h"
#include "Ay38912.h"
#include "common/Queue.h"
#include <string.h>


namespace kio::Audio
{

/*	_______________________________________________________________________________________
	template class AudioSource implements a AY-3-8912 sound chip.
	The class queues the write commands so that writing and reading can be done asynchronously.
	If you use port A & B, then write commands with a callback function are available.

	num_channels: 1=mono, 2=stereo. should match audio_hw_num_channels.
	queue_size:   choose wisely...
		if all you want is play back YM or similar audio files, then as little as 16 will do:
		these files write 14 registers every video frame.
		if your app is an emulator of some ancient computing device, then most of the time 16 will do as well,
		but the possible upper limit is much higher: a typical home computer could write up to ~200000 registers per second.
		it might actually at some time write ~20000/sec for sampled sound.
		a typical emulator runs the emulation for one frame and may do this very fast, then wait until the end of the frame
		while the commands are slowly consumed by the audio backend. this would require a buffer of ~20000/50 = 4000 samples.
		if you can't avoid it and have enough memory available: use queue_size = 4096.
		you may revise your program to run in smaller chunks
		or simply rely on the Ay38912_AudioSource to block until the audio backend consumed the next command.
*/
template<uint num_channels, uint queue_size = 256>
class Ay38912_AudioSource : public AudioSource<num_channels>
{
	static_assert(queue_size >= 16 && queue_size <= 1024 * 4);
	using Ay38912 = Audio::Ay38912<num_channels>;

public:
	using StereoMix		= typename Ay38912::StereoMix;
	using CC			= int;
	using CC256			= int; // 24.8 fixed point
	using WritePortProc = const std::function<void(CC, bool, uint8)>;
	using ReadPortProc	= const std::function<uint8(CC, bool)>;

	/*	constructor:
		ay_frequency: frequency of the ay chip clock
		              sample frequency is set to hw_sample_frequency
		StereoMix:    how the ay chip outputs were connected to one or two audio outputs
		volume:       initial volume. set to 0.0 .. 1.0.
		max_latency:  sets the max. latency for play back. this adds to the audio backend's latency.
			= 0:	  disable latency control if you need no synchronization with another realtime instance,
					  e.g. for simply play back audio files.
			= 20:	  recommended for emulators, because at 50Hz this is the time for 1 video frame.
					  max_latency should be at least as long as the period of the timer which runs the emulation.
	*/
	Ay38912_AudioSource(float ay_frequency, StereoMix = Ay38912::mono, float volume = 0.5, uint max_latency_ms = 20);

	// the following functions are intended for initial setup of the AY chip state.
	// setRegister() and reset() are synchronized (via queue), the others need no synchronization.
	void  setMaxLatency(uint msec) noexcept;
	void  setVolume(float v) noexcept { ay.setVolume(v); }
	void  setRegister(uint r, uint8 n) noexcept { setRegister(cc_in, r, n); }
	void  setRegNr(uint n) noexcept { ay_reg_nr = n & 0x0f; }
	uint8 getRegister(uint n) const noexcept { return ay_reg[n & 0x0f]; }
	uint  getRegNr() const noexcept { return ay_reg_nr; }
	void  reset() noexcept { reset(cc_in); }

	// the following functions are intended to be called at runtime to store commands:
	// callbacks are called immediately, reads are from the shadow registers, writes are queued.
	// CC is the ever incrementing clock cycle. it may happily wrap around or you call shiftTimebase() as needed.
	void reset(CC) noexcept;
	void reset(CC, WritePortProc&);
	void finish(CC) noexcept; // play up to CC and then self-destruct by removing this instance from the backend.

	void setRegister(CC, uint r, uint8 value) noexcept;
	void setRegister(CC, uint reg, uint8 n, WritePortProc&);

	void writeRegister(CC cc, uint8 n) noexcept { setRegister(cc, ay_reg_nr, n); }
	void writeRegister(CC cc, uint8 n, WritePortProc& cb) { setRegister(cc, ay_reg_nr, n, cb); }

	uint8 readRegister(CC) noexcept { return ay_reg[ay_reg_nr]; }
	uint8 readRegister(CC, ReadPortProc&);

	// adjust the clock cycle counter to match what your application did:
	void shiftTimebase(int delta_cc);			 // subtract delta_cc from the clock cycle counter
	void resetTimebase() noexcept { cc_in = 0; } // reset the clock cycle counter to 0

	// callback for the audio backend:
	// always returns num_frames unless this AudioSource can be removed.
	virtual uint getAudio(AudioSample<num_channels>* buffer, uint num_frames) noexcept override;

private:
	void setClock(float f) noexcept { ay.setClock(f); }					   // must be called synchronized
	void setSampleRate(float f) noexcept override { ay.setSampleRate(f); } // AudioController can call base class

	// queue data:
	enum WHAT : uint8 { SET_REG = 0, NOP = 16, RESET, FINISH };
	struct RegInfo
	{
		WHAT   what;
		uint8  value;
		uint16 delta_cc; // to prev.
	};

	Queue<RegInfo, queue_size> queue;
	int						   cc_written = 0; // to calculate current latency
	int						   cc_read	  = 0; // ""

	Ay38912 ay;

	uint8 ay_reg[16];	  // shadow registers
	uint  ay_reg_nr = 0;  // selected register
	CC	  max_latency_cc; // 0=disabled, else CC
	CC	  cc_in	  = 0;	  // clock cycle of last queue.put()
	CC	  cc_pos  = 0;	  // clock cycle of last queue.get()
	CC256 cc_base = 0;	  // exact clock cycle of end of last call to getAudio() (a 24.8 fixed point value)

	void queue_put(WHAT, uint8, CC) noexcept;						   // helper
	int	 cc_in_queue() const noexcept { return cc_written - cc_read; } // helper
};


// ===================================================================================
// implementations:


constexpr uint8 ayRegisterBitMasks[16] = {
	0xff, 0x0f,		  // channel A period
	0xff, 0x0f,		  // channel B period
	0xff, 0x0f,		  // channel C period
	0x1f,			  // noise period
	0xff,			  // mixer control
	0x1f, 0x1f, 0x1f, // channel A,B,C volume
	0xff, 0xff, 0x0f, // envelope period and shape
	0xff, 0xff		  // i/o ports
};

constexpr uint8 ayRegisterResetValues[] = {
	0xff, 0x0f, 0xff, 0x0f, 0xff, 0x0f, // channel A,B,C period
	0x1f,								// noise period
	0x3f,								// mixer control
	0x00, 0x00, 0x00,					// channel A,B,C volume
	0x00, 0x00, 0x00, // envelope period and shape	note: some sq-tracker demos assume registers=0 after reset
	0xff, 0xff		  // i/o ports
};

template<uint nc, uint qs>
Ay38912_AudioSource<nc, qs>::Ay38912_AudioSource(float frequency, StereoMix mix, float volume, uint max_latency_ms) :
	ay(frequency, mix, volume)
{
	setMaxLatency(max_latency_ms);
	memcpy(ay_reg, ayRegisterResetValues, sizeof(ay_reg));
}

template<uint nc, uint qs>
void Ay38912_AudioSource<nc, qs>::setMaxLatency(uint msec) noexcept
{
	if (msec) limit(5u, msec, 1000u);
	max_latency_cc = msec == 0 ? 0 : int(ay.getClock()) * int(msec) / 1000;
}

template<uint nc, uint qs>
void Ay38912_AudioSource<nc, qs>::queue_put(WHAT what, uint8 reg, CC cc) noexcept // helper
{
	assert(cc >= cc_in);

	while (unlikely(cc - cc_in > 0xffff))
	{
		int d = min(cc - cc_in - 0xffff, 0x00ffffff);
		while (queue.free() == 0) { __dmb(); }
		queue.put(RegInfo {NOP, uint8(d >> 16), uint16(d)});
		cc_in += d;
		cc_written += d;
	}

	while (unlikely(queue.free() == 0)) { __dmb(); }
	queue.put(RegInfo {what, reg, uint16(cc - cc_in)});
	cc_in = cc;
	cc_written += cc - cc_in;
}

template<uint nc, uint qs>
void Ay38912_AudioSource<nc, qs>::setRegister(CC cc, uint reg, uint8 value) noexcept
{
	reg &= 0x0f;
	value &= ayRegisterBitMasks[reg];
	ay_reg[reg] = value;
	if (reg < 14) queue_put(WHAT(reg), value, cc);
}

template<uint nc, uint qs>
void Ay38912_AudioSource<nc, qs>::setRegister(CC cc, uint reg, uint8 value, WritePortProc& callback)
{
	reg &= 0x0f;
	value &= ayRegisterBitMasks[reg];

	if (reg < 14)
	{
		if (reg == 7)
		{
			uint8 t = value ^ ay_reg[7];
			if (t & 0x40 && ay_reg[14] != 0xff) callback(cc, 0, value & 0x40 ? ay_reg[14] : 0xff);
			if (t & 0x80 && ay_reg[15] != 0xff) callback(cc, 1, value & 0x80 ? ay_reg[15] : 0xff);
		}
		queue_put(WHAT(reg), value, cc);
	}
	else
	{
		if (ay_reg[reg] != value && ay_reg[7] & (1 << (reg & 7))) callback(cc, reg & 1, value);
	}

	ay_reg[reg] = value;
}

template<uint nc, uint qs>
void Ay38912_AudioSource<nc, qs>::reset(CC cc) noexcept
{
	ay_reg_nr = 0;
	memcpy(ay_reg, ayRegisterResetValues, sizeof(ay_reg));
	queue_put(RESET, 0, cc);
}

template<uint nc, uint qs>
void Ay38912_AudioSource<nc, qs>::reset(CC cc, WritePortProc& callback)
{
	setRegister(cc, 7, ayRegisterResetValues[7], callback);
	reset(cc);
}

template<uint nc, uint qs>
uint8 Ay38912_AudioSource<nc, qs>::readRegister(CC cc, ReadPortProc& callback)
{
	uint r = ay_reg_nr;
	return (r < 14) ? ay_reg[r] : (ay_reg[7] & (1 << (r & 7)) ? ay_reg[r] : 0xff) & callback(cc, r & 1);
}

template<uint nc, uint qs>
void Ay38912_AudioSource<nc, qs>::shiftTimebase(int delta_cc)
{
	assert(delta_cc >= 0);
	cc_in -= delta_cc;
}

template<uint nc, uint qs>
void Ay38912_AudioSource<nc, qs>::finish(CC cc) noexcept
{
	queue_put(FINISH, 0, cc);
}

template<uint nc, uint qs>
uint Ay38912_AudioSource<nc, qs>::getAudio(AudioSample<nc>* buffer, uint num_frames) noexcept
{
	// shift time base to avoid overflow:
	cc_base -= cc_pos << 8;
	cc_pos = 0;

	// start output
	CC cc_start = (cc_base + 0xff) >> 8;
	cc_base += ay.audioBufferStart(buffer, num_frames);
	CC cc_end = cc_base >> 8;

	// ever so slowly creep ahead of realtime so that we don't unneccessarily lag behind
	// => occasionally we are ahead and adjust
	// => possibly we never lag too much behind
	cc_base += 2; // add 2/256 ay_clocks per sample buffer -> ~2 ay_clocks per second

	// check whether we are ahead or lag behind:
	// note: if the app produces commands in chunks larger than max_latency,
	//       then we can be ahead and lag behind at the same time!
	while (queue.avail())
	{
		auto& r	 = queue.peek();
		CC	  cc = r.delta_cc;

		if (r.what == NOP)
		{
			cc += r.value << 16;
			cc_read += cc;
			cc_pos += cc;
			queue.drop();
			continue;
		}
		else if (cc_pos + cc < cc_start)
		{
			// cmd is back in time => we are too fast:
			cc_pos = cc_start - cc;
		}
		else if (max_latency_cc != 0 && cc_pos + cc_in_queue() > cc_start + max_latency_cc)
		{
			// we lag more than max_latency behind:
			// after adjustment some cmds will be back in time!
			cc_pos = cc_start + max_latency_cc - cc_in_queue();
		}
		break;
	}

	// process the commands:
	while (queue.avail())
	{
		auto& r	 = queue.peek();
		CC	  cc = cc_pos + r.delta_cc;
		if (cc > cc_end) break;

		if (r.what < 16) { ay.setRegister(cc - cc_start, r.what, r.value); }
		else if (r.what == NOP) { cc += r.value << 16; }
		else if (r.what == RESET) { ay.reset(cc - cc_start); }
		else //if (r.what == FINISH)
		{
			assert(r.what == FINISH);
			if (cc > cc_start) break; // some audio in buffer => return full buffer, don't skip command
			else return 0;			  // no audio in buffer => remove us from the AudioController
		}
		cc_read += cc - cc_pos;
		cc_pos = cc;
		queue.drop();
	}

	// finalize output
	ay.audioBufferEnd();
	return num_frames;
}

} // namespace kio::Audio
