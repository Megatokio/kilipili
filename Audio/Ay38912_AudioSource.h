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
	template class Ay38912_AudioSource implements a AY-3-8912 sound chip.
	The class queues the write commands so that writing and reading can be done asynchronously.
	If you use port A & B, then write commands with a callback function are available.
	The AudioSource should be added to the AudioController.

	num_channels: 1=mono, 2=stereo. should match audio_hw_num_channels.
	queue_size:   recommended value depends on use case.

	Use Cases:
	 1: Play instantly
	 2: Buffer a lot
	 3. The AY chip is programmed by another real-time process

	1: Play instantly

		You want a a tune to play immediately when you set a register.
		 -> Set the maximum latency to a low value (the default value of 20ms is good) and set register values
			with setRegister(r,n). This will use the current CC for the time stamp which very fast will be in
			the past. The backend detects and compensates for this.
			If you play no note for a long time (~20 minutes) then the internal CC may roll over and the next
			note played will be seemingly far in the future. This is caught by setting a low latency.

	2: Buffer a lot

		You want to buffer many register updates ahead of time, e.g. to play back an audio file with infrequent
		updates or possible high latency when reading from the file.
		 -> Use a large queue_size and disable the latency test by setting max_latency to 0.
			Before each file resetTimebase() and start with CC=0. This eliminates problems after playing
			no sound (~20 minutes) for a long while.
			Use setRegister(CC,r,n) to set the registers, increment the CC according to the time stamps.

	3. The AY chip is programmed by another real-time process

		You want to emulate an ancient home computer or something like this.
		 -> Use a large queue size (~512) and set the maximum latency to a low value (~20ms = time for video frame).
			Use setRegister(CC,r,n) to set the registers, with CC calculated from the emulated system's time stamp.
			Use shiftTimebase() if needed, e.g. if the emulation restarts the time stamp once per video frame.
		Note: most of the time a queue_size of 16 will do it, but the possible upper limit is much higher:
			a typical home computer could write up to ~200000 registers per second. it might actually at some time
			write ~20000 samples/sec for sampled sound, which is 400 samples/frame. Therefore the recommended
			queue_size is 512. This can be reduced if the emulator is run in smaller chunks than 1/frame or you may
			simply rely on the Ay38912_AudioSource to block on a full queue until the backend read the next command.
		Note: due to rounding the actual clock frequency of the emulation is not exactly the requested frequency.
			This leads to either the emulation slowly running ahead or falling behind the other real-time process.
			Running ahead is rarely a problem: once in a while the emulation receives a command which seems to be
			in the past and compensates for that. Falling behind will be caught once it reaches max_latency.
			If you don't like the emulation to permanently lag max_latency behind then use nextHigherClock()
			for the AY clock. You may still be ever so slightly slower in some cases, e.g. because the
			hardware sample frequency is rounded to start with or after changing the hardware sample frequency.
			You can use getActualClock() to get the exact rounded value used for the AY clock at any time.
*/
template<uint num_channels, uint queue_size = 256>
class Ay38912_AudioSource : public AudioSource<num_channels>
{
	static_assert(queue_size >= 16 && queue_size <= 1024 * 4);
	using Ay38912 = Audio::Ay38912<num_channels>;

public:
	using StereoMix		= typename Ay38912::StereoMix;
	using CC			= circular_int;
	using WritePortProc = const std::function<void(CC, bool, uint8)>;
	using ReadPortProc	= const std::function<uint8(CC, bool)>;

	/*	constructor:
		ay_frequency: the ay chip clock
		              sample_frequency is set to hw_sample_frequency
		StereoMix:    how the ay chip outputs were connected to one or two audio outputs
		volume:       initial volume. range 0.0 … 1.0.
		max_latency:  the maximum latency for play back. this adds to the AudioController's latency. 0=disabled.
	*/
	Ay38912_AudioSource(float ay_frequency, StereoMix = Ay38912::mono, float volume = 0.5, uint max_latency_ms = 20);

	// calculate clock which will result in a clock not slower than the requested one:
	static float nextHigherClock(float clock) noexcept { return Ay38912::nextHigherClock(clock); }

	void  setMaxLatency(uint msec) noexcept;
	void  setVolume(float v) noexcept { ay.setVolume(v); }
	float getClock() const noexcept { return ay.getClock(); }			  // get programmed clock
	float getActualClock() const noexcept { return ay.getActualClock(); } // get actual clock

	void  setRegNr(uint n) noexcept { ay_reg_nr = n & 0x0f; }
	uint8 getRegister(uint n) const noexcept { return ay_reg[n & 0x0f]; }
	uint  getRegNr() const noexcept { return ay_reg_nr; }

	CC	 clockCycle() const noexcept { return cc_in; }	// get current clock cycle
	uint free() const noexcept { return queue.free(); } // get number of unblocked writes

	// the following functions are intended to be called at runtime to store commands:
	// callbacks are called immediately, reads are from the shadow registers, writes are queued.
	// CC is the ever incrementing clock cycle. it may happily wrap around or you call shiftTimebase() as needed.
	void reset() noexcept { reset(cc_in); } // reset the AY chip
	void reset(CC) noexcept;
	void reset(CC, WritePortProc&);

	void finish() noexcept { finish(cc_in); } // remove self from AudioController when finished
	void finish(CC) noexcept;				  // play up to CC, then remove self from AudioController

	void setRegister(uint r, uint8 n) noexcept { setRegister(cc_in, r, n); }
	void setRegister(CC, uint r, uint8 n) noexcept;
	void setRegister(CC, uint reg, uint8 n, WritePortProc&);

	void writeRegister(CC cc, uint8 n) noexcept { setRegister(cc, ay_reg_nr, n); }
	void writeRegister(CC cc, uint8 n, WritePortProc& cb) { setRegister(cc, ay_reg_nr, n, cb); }

	uint8 readRegister(CC) noexcept { return ay_reg[ay_reg_nr]; }
	uint8 readRegister(CC, ReadPortProc&);

	// adjust the clock cycle counter to match what your application did:
	void shiftTimebase(int delta_cc) noexcept; // subtract delta_cc from the clock cycle counter
	void resetTimebase() noexcept;			   // reset the clock cycle counter to 0

	// callback for the audio backend:
	// always returns num_frames unless this AudioSource can be removed.
	virtual uint getAudio(AudioSample<num_channels>* buffer, uint num_frames) noexcept override;

private:
	void setClock(float f) noexcept { ay.setClock(f); }					   // must be called synchronized
	void setSampleRate(float f) noexcept override { ay.setSampleRate(f); } // AudioController can call base class

	// queue data:
	enum What : uint8 { SET_REG = 0, NOP = 16, RESET, RESET_TIMEBASE, FINISH };
	struct RegInfo
	{
		What   what;
		uint8  value;
		uint16 delta_cc; // to prev. cmd
	};

	Queue<RegInfo, queue_size> queue;
	int						   cc_written = 0; // to calculate current latency
	int						   cc_read	  = 0; // ""

	Ay38912 ay;

	uint8 ay_reg[16];		 // shadow registers
	uint  ay_reg_nr = 0;	 // selected register
	int	  max_latency_cc;	 // 0=disabled, else CC
	CC	  cc_in {0};		 // clock cycle of last queue.put()
	CC	  cc_pos {0};		 // clock cycle of last queue.get()
	CC	  cc_buffer_end {0}; // clock cycle of end of buffer at last call to getAudio()

	void queue_put(What, uint8, CC) noexcept;						   // helper
	int	 cc_in_queue() const noexcept { return cc_written - cc_read; } // helper
};


// ===================================================================================
// implementations:


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
void Ay38912_AudioSource<nc, qs>::queue_put(What what, uint8 value, CC cc) noexcept // helper
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
	queue.put(RegInfo {what, value, uint16(cc - cc_in)});
	cc_written += cc - cc_in;
	cc_in = cc;
}

template<uint nc, uint qs>
void Ay38912_AudioSource<nc, qs>::setRegister(CC cc, uint reg, uint8 value) noexcept
{
	reg &= 0x0f;
	value &= ayRegisterBitMasks[reg];
	ay_reg[reg] = value;
	if (reg < 14) queue_put(What(reg), value, cc);
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
		queue_put(What(reg), value, cc);
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
void Ay38912_AudioSource<nc, qs>::shiftTimebase(int delta_cc) noexcept
{
	assert(delta_cc >= 0);
	cc_in -= delta_cc;
}

template<uint nc, uint qs>
void Ay38912_AudioSource<nc, qs>::resetTimebase() noexcept
{
	cc_in = CC(0);
	queue_put(RESET_TIMEBASE, 0, cc_in);
}

template<uint nc, uint qs>
void Ay38912_AudioSource<nc, qs>::finish(CC cc) noexcept
{
	queue_put(FINISH, 0, cc);
}

template<uint nc, uint qs>
uint Ay38912_AudioSource<nc, qs>::getAudio(AudioSample<nc>* buffer, uint num_frames) noexcept
{
	// start output
	CC cc_start	  = cc_buffer_end;
	cc_buffer_end = ay.audioBufferStart(buffer, num_frames);

	// check whether we are ahead or lag behind:
	// note: if the app produces commands in chunks larger than max_latency,
	//       then we can be ahead and lag behind at the same time!
	while (queue.avail())
	{
		auto& r	 = queue.peek();
		int	  cc = r.delta_cc;

		if (r.what == NOP)
		{
			cc += r.value << 16;
			cc_read += cc;
			cc_pos += cc;
			queue.drop();
			continue;
		}
		else if (r.what == RESET_TIMEBASE)
		{
			cc_pos = cc_start; //
		}
		else if (cc_pos + cc < cc_start)
		{
			// cmd is back in time => we are too fast:
			cc_pos = cc_start - cc;
		}
		else if (max_latency_cc != 0 && cc_pos + cc_in_queue() > cc_start + max_latency_cc)
		{
			// we lag more than max_latency behind:
			// after this adjustment some cmds will be back in time!
			cc_pos = cc_start + max_latency_cc - cc_in_queue();
		}
		break;
	}

	// process the commands:
	while (queue.avail())
	{
		auto& r	 = queue.peek();
		CC	  cc = cc_pos + r.delta_cc;
		if (cc > cc_buffer_end) break;

		if (r.what < 16) { ay.setRegister(cc, r.what, r.value); }
		else if (r.what == NOP) { cc += r.value << 16; }
		else if (r.what == RESET) { ay.reset(cc); }
		else if (r.what == RESET_TIMEBASE) {}
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


/*



























*/
