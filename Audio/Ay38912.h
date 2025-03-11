// Copyright (c) 1995 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "AudioController.h"
#include "AudioSample.h"
#include "AudioSource.h"
#include "audio_options.h"
#include "common/basic_math.h"
#include "common/cdefs.h"
#include "glue.h"
#include <cstdio>


/*	_______________________________________________________________________________________
	ay-3-8912 sound chip emulation

	template classes for output into a mono or stereo buffer.
	there are 3 different classes for different applications:

	AY38912<NCH>: the basic class.
		Registers can be set any time and take effect the next time
		the AudioController calls getAudio(), which is ~500 times a second.
		NCH: number of channels is normally set to audio_hw_num_channels.

	AY38912_player<NCH,QSZ>: adds a queue for register set updates.
		Register setd for each frame are stored into a queue for getAudio().		
		NCH: number of channels is normally set to audio_hw_num_channels.
		QSZ: a queue size of 4 register sets is the default and best in 90% of all use cases.
		The register sets are written into the AY at a fixed interval, normally the source's frame rate.
		The clock, framerate and stereo mix for each song can be set synchronously.

	AY38912_sync<NCH,QSZ>: adds a queue for timed register updates.
		All register updates are stored with their exact AY clock cycle into a queue for getAudio().		
		NCH: Number of channels is normally set to audio_hw_num_channels.
		QSZ: A queue size of 64 covers 90% of all use cases. a larger queue size is required to
			 play back sampled sound. an emulator may use 256 or even higher, just to be safe.
		The worst case scenario depends on the AudioController's callback frequency which depends on
		the audio hw_sample_frequency and the AUDIO_DMA_BUFFER_SIZE and audio max_latency setting,
		and the speed with which the emulated system can write new values into the AY chip.
*/

namespace kio::Audio
{

// actual HW sample frequency used by AudioController:
extern float hw_sample_frequency;

constexpr uint8 ayRegisterNumBits[] = {8, 4, 8, 4, 8, 4, 5, 8, 5, 5, 5, 8, 8, 4, 8, 8};

constexpr uint8 ayRegisterBitMasks[] = {0xff, 0x0f, 0xff, 0x0f, 0xff, 0x0f, 0x1f, 0xff,
										0x1f, 0x1f, 0x1f, 0xff, 0xff, 0x0f, 0xff, 0xff};

constexpr uint8 ayRegisterResetValues[] = {0xff, 0x0f, 0xff, 0x0f, 0xff, 0x0f, 0x1f, 0x3f,
										   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff};

enum AyStereoMix { Mono, ABCstereo, ACBstereo };


// ******************************************************************************************
// ******************************************************************************************
// *****																				*****
// *****				  unbuffered ay-3-8912 sound chip emulation  					*****
// *****																				*****
// ******************************************************************************************
// ******************************************************************************************

template<uint num_channels = audio_hw_num_channels>
class Ay38912 : public AudioSource<num_channels>
{
public:
	Id("Ay38912");

	Ay38912(float ay_clock_frequency, AyStereoMix = Mono, float volume = 0.5) noexcept;

	void setStereoMix(AyStereoMix) noexcept;
	void setVolume(float volume) noexcept;

	// change the AY clock:
	// should only be called on core0 (same core as the audio interrupt or dispatcher)
	// the actually set clock will be a tiny amount higher than the requested one, never lower.
	void  setClock(float ay_clock_frequency) noexcept;
	float getActualClock() const noexcept { return hw_sample_frequency * ccx_per_sample / (1 << ccx_fract_bits); }

	void reset() noexcept;
	void setRegister(uint r, uint8 n) noexcept;
	void setRegisters(const uint8 regs[14]) noexcept;

	// convenience:
	// these also set both bytes at the same time.
	void setChannelAPeriod(uint16) noexcept;
	void setChannelBPeriod(uint16) noexcept;
	void setChannelCPeriod(uint16) noexcept;
	void setEnvelopePeriod(uint16) noexcept;

	// AudioSource interface:
	virtual void setSampleRate(float frequency) noexcept override;
	virtual uint getAudio(AudioSample<num_channels>* buffer, uint num_samples) noexcept override;

protected:
	// the emulation uses internally 24.8 fixed point values for the AY38912 clock frequency
	// to allow integer calculations when resampling the output to the hw_frequency with little error.
	static constexpr int ccx_fract_bits = 8;
	using CCx							= CC; // 24.8 fixed point

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
		bool  hold;		 // !repeat ramping
		bool  toggle;	 // toggle ramping direction
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

	AyStereoMix stereo_mix;

	float ay_clock;		  // ay clock frequency
	float volume;		  // volume of all channels combined: 0 .. 1.0
	int	  ccx_per_sample; // frequency / hw_frequency
	uint8 ay_reg[16];	  // registers
	int	  log_vol[16];	  // logarithmic volume table prescaled for faster resampling

	AudioSample<num_channels>*	   output_buffer  = nullptr;
	AudioSample<num_channels, int> current_value  = 0; // current output of chip
	AudioSample<num_channels, int> current_sample = 0; // sample under construction

	CCx ccx_at_sos {0}; // cc at start of sample
	CCx ccx_now {0};	// current cc

	int	 output_of(Channel&) noexcept;
	void run_up_to_cycle(CCx ccx) noexcept;
};


extern template class Ay38912<1>;
extern template class Ay38912<2>;


// ******************************************************************************************
// ******************************************************************************************
// *****																				*****
// *****					buffered ay-3-8912 sound chip emulation 					*****
// *****					for use by AY music file players 							*****
// *****																				*****
// ******************************************************************************************
// ******************************************************************************************


template<uint num_channels = audio_hw_num_channels, uint queue_size = 4>
class Ay38912_player : public Ay38912<num_channels>
{
public:
	static_assert((queue_size & (queue_size - 1)) == 0);
	using super = Ay38912<num_channels>;

	Ay38912_player(float ay_clock, AyStereoMix = Mono, int frames_per_second = 50, float volume = 0.5) noexcept;

	// new settings take effect immediately and also apply to not yet played queued commands!
	void setStereoMix(AyStereoMix mix) noexcept { super::setStereoMix(mix); }
	void setVolume(float volume) noexcept { super::setVolume(volume); }
	void setClock(float frequency) noexcept
	{
		super::setClock(frequency);
		setFps(fps);
	}
	void setFps(int fps) noexcept;

	void reset(float ay_clock, AyStereoMix = Mono, int frames_per_second = 50) noexcept;
	void reset() noexcept { setRegisters(ayRegisterResetValues); }
	void setRegisters(const uint8 registers[14]) noexcept;

	uint free() const noexcept { return queue.free(); }
	uint avail() const noexcept { return queue.avail(); }

protected:
	uint getAudio(AudioSample<num_channels>* buffer, uint num_frames) noexcept override;
	void setSampleRate(float frequency) noexcept override;

	using CCx = typename super::CCx;

	struct Queue
	{
		enum Cmd { SetRegisters, Reset };
		union Frame
		{
			struct
			{
				float		clock;
				AyStereoMix mix;
				int			fps;
				uint16		_padding;
				uint16		cmd; // at position of port A/B registers 14/15
			};
			uint8 registers[16];
		};
		Frame buffer[queue_size];
		uint8 ri = 0, wi = 0;

		Frame& operator[](uint i) noexcept { return buffer[i & (queue_size - 1)]; }
		uint   avail() const noexcept { return uint8(wi - ri); }
		uint   free() const noexcept { return queue_size - avail(); }
	} queue;

	uint16 fps;				   // frames per second
	int32  ccx_per_frame;	   // ay clock cycles per frame (24.8 fixed point)
	CCx	   ccx_next_frame {0}; // ay clock cycle when the next register frame is to be written
};

template<uint NCH, uint QSZ>
Ay38912_player<NCH, QSZ>::Ay38912_player(float frequency, AyStereoMix mix, int fps, float volume) noexcept :
	super(frequency, mix, volume)
{
	setFps(fps);
}

template<uint NCH, uint QSZ>
void Ay38912_player<NCH, QSZ>::setFps(int fps) noexcept
{
	this->fps	  = fps;
	ccx_per_frame = super::ay_clock * (1 << super::ccx_fract_bits) / fps;
}

template<uint NCH, uint QSZ>
void Ay38912_player<NCH, QSZ>::setSampleRate(float new_hw_sample_frequency) noexcept
{
	// callback from AudioController
	// *not* the AY clock!
	super::setSampleRate(new_hw_sample_frequency);
	setFps(fps);
}

template<uint NCH, uint QSZ>
void Ay38912_player<NCH, QSZ>::reset(float clock, AyStereoMix mix, int fps) noexcept
{
	while (!free()) __wfe();
	auto& zdata = queue[queue.wi];
	zdata.clock = clock;
	zdata.mix	= mix;
	zdata.fps	= fps;
	zdata.cmd	= Queue::Reset;
	__dmb();
	queue.wi++;
}

template<uint NCH, uint QSZ>
void Ay38912_player<NCH, QSZ>::setRegisters(const uint8 regs[14]) noexcept
{
	while (!free()) __wfe();
	auto& zdata = queue[queue.wi];
	memcpy(zdata.registers, regs, 14);
	zdata.cmd = Queue::SetRegisters;
	__dmb();
	queue.wi++;
}

template<uint NCH, uint QSZ>
uint Ay38912_player<NCH, QSZ>::getAudio(AudioSample<NCH>* buffer, uint num_frames) noexcept
{
	// callback from AudioController
	super::output_buffer	= buffer;
	auto* output_buffer_end = buffer + num_frames;
	CCx	  ccx_buffer_end	= super::ccx_now + int(num_frames) * super::ccx_per_sample;
	super::ccx_at_sos		= super::ccx_now;
	super::current_sample	= 0;

	assert(ccx_next_frame >= super::ccx_now);

	while (ccx_next_frame < ccx_buffer_end)
	{
		super::run_up_to_cycle(ccx_next_frame);
		ccx_next_frame += ccx_per_frame;

		if (avail())
		{
			auto& qdata = queue[queue.ri];

			if (qdata.cmd == Queue::Reset)
			{
				super::setClock(qdata.clock);
				super::setStereoMix(qdata.mix);
				setFps(qdata.fps);
				super::reset();

				super::current_sample = 0;
				super::ccx_at_sos	  = super::ccx_now;
				ccx_buffer_end = super::ccx_now + (output_buffer_end - super::output_buffer) * super::ccx_per_sample;
			}
			else super::setRegisters(qdata.registers);

			__dmb();
			queue.ri++;
			__sev();
		}
	}

	super::run_up_to_cycle(ccx_buffer_end);
	return num_frames;
}


// ******************************************************************************************
// ******************************************************************************************
// *****																				*****
// *****				  synchronized ay-3-8912 sound chip emulation 					*****
// *****																				*****
// ******************************************************************************************
// ******************************************************************************************


template<uint num_channels = audio_hw_num_channels, uint queue_size = 64>
class Ay38912_sync : public Ay38912<num_channels>
{
public:
	static_assert((queue_size & (queue_size - 1)) == 0);
	using super = Ay38912<num_channels>;

	Ay38912_sync(float ay_clock_frequency, AyStereoMix = Mono, float volume = 0.5) noexcept;

	// new settings take effect immediately and also apply to not yet played queued commands!
	void setStereoMix(AyStereoMix mix) noexcept { super::setStereoMix(mix); }
	void setVolume(float volume) noexcept { super::setVolume(volume); }
	void setClock(float frequency) noexcept { super::setClock(frequency); }

	// set register after delay_ay_cc:
	void reset(int delay_ay_cc) noexcept { setRegister(delay_ay_cc, 14, 0); }
	void setRegister(int delay_ay_cc, uint r, uint8 n) noexcept;
	void addDelay(int delay_ay_cc) noexcept { setRegister(delay_ay_cc, 15, 0); }

	uint free() const noexcept { return queue.free(); }
	uint avail() const noexcept { return queue.avail(); }

private:
	uint getAudio(AudioSample<num_channels>* buffer, uint num_frames) noexcept override;
	//void setSampleRate(float frequency) noexcept override;

	static constexpr int ccx_fract_bits = super::ccx_fract_bits;
	using super::run_up_to_cycle;
	using CCx = typename super::CCx;
	using super::ccx_now;
	using super::ccx_per_sample;
	using super::output_buffer;

	struct Queue
	{
		struct Data
		{
			uint16 delay_cc;
			uint8  reg;
			uint8  value;
		} buffer[queue_size];
		uint16 ri = 0, wi = 0;

		Data& operator[](uint i) noexcept { return buffer[i & (queue_size - 1)]; }
		uint  avail() const noexcept { return uint16(wi - ri); }
		uint  free() const noexcept { return queue_size - avail(); }
	};

	Queue queue;
	CCx	  ccx {0}; // current (last) clock cycle
};


//
// ####################### Implementations ###############################
//

template<uint NCH, uint QSZ>
Ay38912_sync<NCH, QSZ>::Ay38912_sync(float frequency, AyStereoMix mix, float volume) noexcept : //
	super(frequency, mix, volume)
{}

template<uint NCH, uint QSZ>
void Ay38912_sync<NCH, QSZ>::setRegister(int delay_cc, uint r, uint8 n) noexcept
{
	assert(delay_cc >= 0);
	//assert(r < 14); 14=reset, 15=nop

	if (!queue.free())
	{
		debugstr("!ay:free\n");
		while (!queue.free()) __wfe();
	}

	auto& qd	= queue[queue.wi];
	qd.delay_cc = min(delay_cc, 0xffff);
	qd.reg		= r;
	qd.value	= n;
	__dmb();
	queue.wi = queue.wi + 1;
}

template<uint NCH, uint QSZ>
uint Ay38912_sync<NCH, QSZ>::getAudio(AudioSample<NCH>* buffer, uint num_frames) noexcept
{
	output_buffer		 = buffer;
	CCx ccx_buffer_start = ccx_now;
	CCx ccx_buffer_end	 = ccx_buffer_start + int(num_frames) * ccx_per_sample;

	// prevent ccx wrapping into the future if no registers are set for a long time:
	ccx = max(ccx, ccx_buffer_start - (0xffff << ccx_fract_bits));

	while (avail())
	{
		auto& qd	= queue[queue.ri];
		int	  delay = qd.delay_cc << ccx_fract_bits;

		if (ccx + delay > ccx_buffer_end) break; // not yet
		ccx = ccx + delay;
		if (ccx > ccx_buffer_start) run_up_to_cycle(ccx);
		else ccx = ccx_buffer_start; // adjust if we are ahead of time

		if (qd.reg < 14) super::setRegister(qd.reg, qd.value);
		else if (qd.reg == 14) super::reset();

		__dmb();
		queue.ri += 1;
	}

	run_up_to_cycle(ccx_buffer_end);
	__sev();
	return num_frames;
}


} // namespace kio::Audio


/*



















































































*/
