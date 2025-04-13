// Copyright (c) 1995 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Ay38912.h"
#include "common/Trace.h"
#include "common/cdefs.h"
#include <cmath>
#include <cstring>

/*
	AY-3-8912 sound chip emulation
	------------------------------
*/


namespace kio::Audio
{

/* ===============================================================
	the 3 channels:

	the sound output to the amplifier is calculated from the mixer inputs like this:
	output = (!enable_sound | sound_input) & (!enable_noise | noise_input)
=============================================================== */

template<uint nc>
inline int Ay38912<nc>::output_of(Channel& ch) noexcept
{
	// the output is normally at volume level.
	// except if pulled down by sound or noise.

	if (ch.sound_enable && !ch.sound_in) return log_vol[0];
	if (ch.noise_enable && !(noise.shiftreg & 1)) return log_vol[0];
	return log_vol[ch.volume < 0x10 ? ch.volume : envelope.index];
}

template<uint nc>
void Ay38912<nc>::Channel::setVolume(uint8 n)
{
	// set volume and select envelope:
	// in: value written to the channel's volume register
	// each channel has it's own amplifier.
	// changing the volume takes effect immediately
	// bits 0-3 define the channel's volume, which is stepped logarithmically
	// and bit 4 overrides bits 0-3 selecting the envelope generator's volume

	assert(n < 0x20);
	volume = n;
}

template<uint nc>
void Ay38912<nc>::Channel::setPeriod(uint16 n)
{
	// change a channel's pitch.
	// the ay input clock is first divided by 16, then by a programmable counter.
	// programming a pitch of 0 seems to have the same effect as a pitch of 1.
	// each counter has 12 significant bits.

	assert(n < 0x1000);
	reload = (n ? n : 1) * predivider;
}

template<uint nc>
inline void Ay38912<nc>::Channel::trigger()
{
	sound_in = !sound_in;
	when += reload;
}

template<uint nc>
void Ay38912<nc>::Channel::fastForward(CCx now)
{
	assert(now >= when);
	int periods = (now - when + reload - 1) / reload;
	when += periods * reload;
	if (periods & 1) sound_in = !sound_in;
}

template<uint nc>
void Ay38912<nc>::Channel::reset(CCx now)
{
	sound_enable = false;
	sound_in	 = false;
	noise_enable = false;
	volume		 = 0;
	reload		 = 0xfff * predivider;
	when		 = now + reload;
}


/* ===============================================================
	noise generator

	there is 1 noise generator. it's output can be mixed to any channel.
	the ay input clock is first divided by 16, then by a programmable counter.
	this counter has 5 significant bits.
	programming a pitch of 0 seems to have the same effect as a pitch of 1.
	
	The noise generator of the ay chip is a 17-bit shift register.
	The input to the shift register is bit0 xor bit2.
	The output of the noise generator is bit0.
=============================================================== */

template<uint nc>
void Ay38912<nc>::Noise::setPeriod(uint8 n)
{
	assert(n < 0x20);
	reload = (n ? n : 1) * predivider;
}

template<uint nc>
inline void Ay38912<nc>::Noise::trigger()
{
	int32 oldshiftreg = shiftreg;
	shiftreg		  = (oldshiftreg >> 1) + (((oldshiftreg << 16) ^ (oldshiftreg << 14)) & 0x10000);
	when += reload;
}

template<uint nc>
void Ay38912<nc>::Noise::fastForward(CCx now)
{
	assert(now >= when);
	when += (now - when + reload - 1) / reload * reload;
}

template<uint nc>
void Ay38912<nc>::Noise::reset(CCx now)
{
	shiftreg = 0x0001FFFF;
	reload	 = 0x1f * predivider;
	when	 = now + reload;
}


/* ===============================================================
	envelope generator

	there is 1 envelope generator.
	envelope shape is controlled by attack, invert and hold.
	the 'repeat' bit (D3 of reg. 13) is not used and always treated as set:
	therefore %00xx is converted to %1001 and %01xx is converted to %1111
	which give the same results.
	the envelope generator is retriggered when a new envelope is selected.

	D3 = repeat
	D2 = attack
	D1 = toggle
	D0 = hold

	1111, 01xx:	/_____	 repeat &  attack &  toggle &  hold	||  !repeat & attack
	1110:		/\/\/\	 repeat &  attack &  toggle & !hold
	1101:		/~~~~~	 repeat &  attack & !toggle &  hold
	1100:		//////	 repeat &  attack & !toggle & !hold
	1011:		\~~~~~	 repeat & !attack &  toggle &  hold
	1010:		\/\/\/	 repeat & !attack &  toggle & !hold
	1001, 00xx:	\_____	 repeat & !attack & !toggle &  hold	||  !repeat & !attack
	1000:		\\\\\\	 repeat & !attack & !toggle & !hold
=============================================================== */

template<uint nc>
void Ay38912<nc>::Envelope::reset(CCx now)
{
	hold	  = false;
	toggle	  = false;
	index	  = 0;
	direction = 0;
	reload	  = 0xffff * predivider;
	when	  = now + reload;
}

template<uint nc>
void Ay38912<nc>::Envelope::setPeriod(uint16 n)
{
	reload = (n ? n : 1) * predivider;
}

template<uint nc>
void Ay38912<nc>::Envelope::setShape(CCx now, uint8 c)
{
	// select new envelope shape
	// restart the envelope generator

	if (!(c & 8)) c = c & 4 ? 0x0f : 0x09;

	index	  = c & 4 ? 0 : 15;
	direction = c & 4 ? 1 : -1;
	toggle	  = c & 2;
	hold	  = c & 1;

	when = now + reload;
}

template<uint nc>
inline void Ay38912<nc>::Envelope::trigger()
{
	if (direction)
	{
		index += direction;

		if (index & 0xF0) // ramp finished
		{
			if (hold)
			{
				direction = 0;
				if (!toggle) index = ~index;
			}
			else
			{
				if (toggle)
				{
					index	  = ~index;
					direction = -direction;
				}
			}
			index &= 0x0F;
		}
		when += reload;
	}
	else when += 0x3fff0000;
}

template<uint nc>
void Ay38912<nc>::Envelope::fastForward(CCx now)
{
	assert(now >= when);

	if (!direction)
	{
		when += 0x3fff0000;
		return;
	}

	int steps = (now - when + reload - 1) / reload;
	when += steps * reload;

	steps &= 31; // avoid xlarge values: max. pattern is 2 ramps à 16 steps

	index += steps * direction;

	if (index & 0xF0) // ramp finished
	{
		if (hold)
		{
			direction = 0;
			if (!toggle) index = ~index;
		}
		else
		{
			if (toggle && (index & 0x10))
			{
				index	  = ~index;
				direction = -direction;
			}
		}
		index &= 0x0F;
	}
}


/* ===============================================================
			ay sound chip
=============================================================== */

template<uint nc>
Ay38912<nc>::Ay38912(float ay_clock, AyStereoMix mix, float volume) noexcept :
	stereo_mix(nc == 2 ? mix : Mono),
	volume(volume)
{
	setClock(ay_clock);
	reset();
}

template<uint nc>
void Ay38912<nc>::setSampleRate(float new_hw_sample_frequency) noexcept
{
	assert(new_hw_sample_frequency == hw_sample_frequency);
	setClock(ay_clock);
}

template<uint nc>
void Ay38912<nc>::setClock(float new_ay_clock) noexcept
{
	debugstr("Ay: set_clock: %f\n", double(new_ay_clock));
	debugstr("Ay: hw_sample_frequency = %f\n", double(hw_sample_frequency));

	// setClock() must not be interrupted by AudioController::getAudio()
	// because `ccx_per_sample` must not change while getAudio() is running
	// because `ccx_buffer_end` is precalculated and used to terminate the loop.
	//
	// this can only happen if setClock() is called on a different core or from a
	// higher prioritized interrupt which is not normally the case.

	// calculate clock which will result in a clock not slower than the requested one:
	ay_clock	   = new_ay_clock;
	ccx_per_sample = 1 + int(new_ay_clock * (1 << ccx_fract_bits) / hw_sample_frequency);
	debugstr("Ay: ccx_per_sample = %d\n", ccx_per_sample);
	setVolume(volume);
}

template<uint nc>
void Ay38912<nc>::setVolume(float v) noexcept
{
	// set volume and compute logarithmic volume table.
	// precompensate for all the multiplications and divisions etc.

	// the Ay chip had a stepping of ~3.5 dB
	// this results in a factor of ~0.78 for each step
	// we use 0.75 which results in min = 1.33% of max

	if (v < -1) v = -1;
	if (v > +1) v = +1;
	volume = v;

	v = v * float(0x7fffffff);	   // wg. scale float -> int32
	v = v / 3;					   // wg. 3 channels added
	v = v / float(ccx_per_sample); // wg. accumulation in resampling

	float base = -v;

	assert_ge(int(base) * 3.0 * ccx_per_sample, -0x7fffffff);
	assert_le(int(base) * 3.0 * ccx_per_sample, +0x7fffffff);
	assert_ge(int(base + 2 * v) * 3.0 * ccx_per_sample, -0x7fffffff);
	assert_le(int(base + 2 * v) * 3.0 * ccx_per_sample, +0x7fffffff);

	log_vol[0] = int(base);
	for (int i = 15; i; i--)
	{
		log_vol[i] = int(base + 2 * v);
		v *= 0.75f;
	}
}

template<uint nc>
void Ay38912<nc>::setStereoMix(AyStereoMix mix) noexcept
{
	if (nc == 2) stereo_mix = mix; // else mono must be mono
}

template<uint nc>
void Ay38912<nc>::setChannelAPeriod(uint16 n) noexcept
{
	n &= ayRegisterBitMasks[0] + 256 * ayRegisterBitMasks[1];
	ay_reg[0] = uint8(n);
	ay_reg[1] = n >> 8;
	channel_A.setPeriod(n);
}

template<uint nc>
void Ay38912<nc>::setChannelBPeriod(uint16 n) noexcept
{
	n &= ayRegisterBitMasks[2] + 256 * ayRegisterBitMasks[3];
	ay_reg[2] = uint8(n);
	ay_reg[3] = n >> 8;
	channel_B.setPeriod(n);
}

template<uint nc>
void Ay38912<nc>::setChannelCPeriod(uint16 n) noexcept
{
	n &= ayRegisterBitMasks[4] + 256 * ayRegisterBitMasks[5];
	ay_reg[4] = uint8(n);
	ay_reg[5] = n >> 8;
	channel_C.setPeriod(n);
}

template<uint nc>
void Ay38912<nc>::setEnvelopePeriod(uint16 n) noexcept
{
	n &= ayRegisterBitMasks[11] + 256 * ayRegisterBitMasks[12];
	ay_reg[11] = uint8(n);
	ay_reg[12] = n >> 8;
	envelope.setPeriod(n);
}

template<uint nc>
void Ay38912<nc>::setRegisters(const uint8 regs[14]) noexcept
{
	if constexpr (1)
	{
		setChannelAPeriod(regs[0] + 256 * regs[1]);
		setChannelBPeriod(regs[2] + 256 * regs[3]);
		setChannelCPeriod(regs[4] + 256 * regs[5]);
		for (uint r = 6; r <= 10; r++) setRegister(r, regs[r]);
		setEnvelopePeriod(regs[11] + 256 * regs[12]);
		if (regs[13] != 0xff) setRegister(13, regs[13]);
	}
	else
	{
		uint cnt = 13 + (regs[13] != 0xff);
		for (uint r = 0; r < cnt; r++) { setRegister(r, regs[r]); }
	}
}

template<uint nc>
void Ay38912<nc>::setRegister(uint regnr, uint8 newvalue) noexcept
{
	trace(__func__);

	regnr &= 0x0F;
	newvalue &= ayRegisterBitMasks[regnr];
	if (ay_reg[regnr] == newvalue && regnr != 13) return;
	ay_reg[regnr] = newvalue;

	switch (regnr)
	{
	case 0: channel_A.setPeriod(ay_reg[1] * 256 + newvalue); break;
	case 1: channel_A.setPeriod(ay_reg[0] + newvalue * 256); break;
	case 2: channel_B.setPeriod(ay_reg[3] * 256 + newvalue); break;
	case 3: channel_B.setPeriod(ay_reg[2] + newvalue * 256); break;
	case 4: channel_C.setPeriod(ay_reg[5] * 256 + newvalue); break;
	case 5: channel_C.setPeriod(ay_reg[4] + newvalue * 256); break;
	case 6: noise.setPeriod(newvalue); break;
	case 7:
	{
		// channels are enabled if bit = 0:
		uint8 c				   = ~newvalue;
		channel_A.sound_enable = c & 1;
		channel_B.sound_enable = c & 2;
		channel_C.sound_enable = c & 4;
		channel_A.noise_enable = c & 8;
		channel_B.noise_enable = c & 16;
		channel_C.noise_enable = c & 32;
		break;
	}
	case 8: channel_A.setVolume(newvalue); break;
	case 9: channel_B.setVolume(newvalue); break;
	case 10: channel_C.setVolume(newvalue); break;
	case 11: envelope.setPeriod(ay_reg[12] * 256 + newvalue); break;
	case 12: envelope.setPeriod(ay_reg[11] + newvalue * 256); break;
	case 13: envelope.setShape(ccx_now, newvalue); break;
	default: break;
	}
}

template<uint nc>
void Ay38912<nc>::reset() noexcept
{
	channel_A.reset(ccx_now);
	channel_B.reset(ccx_now);
	channel_C.reset(ccx_now);
	noise.reset(ccx_now);
	envelope.reset(ccx_now);

	memcpy(ay_reg, ayRegisterResetValues, sizeof(ay_reg));
}

template<uint nc>
uint Ay38912<nc>::getAudio(AudioSample<nc>* buffer, uint num_samples) noexcept
{
	output_buffer  = buffer;
	ccx_at_sos	   = ccx_now;
	current_sample = 0;
	run_up_to_cycle(ccx_now + int(num_samples) * ccx_per_sample);
	return num_samples;
}

template<uint nc>
void Ay38912<nc>::run_up_to_cycle(const CCx ccx_end) noexcept
{
	trace(__func__);

	if (ccx_end <= ccx_now) return;

	assert(ccx_now >= ccx_at_sos);
	assert(ccx_at_sos < ccx_now + ccx_per_sample);

	while (true)
	{
		// who is next ?
		int who		 = 0; // finish
		CCx ccx_when = ccx_end;

		if (noise.when < ccx_when)
		{
			if (~ay_reg[7] & 0x38) { who = 1, ccx_when = noise.when; }
			else noise.fastForward(ccx_end);
		}
		if (channel_A.when < ccx_when) // channel A
		{
			if (ay_reg[8] && ~ay_reg[7] & 1) { who = 3, ccx_when = channel_A.when; }
			else channel_A.fastForward(ccx_end);
		}
		if (channel_B.when < ccx_when) // channel B
		{
			if (ay_reg[9] && ~ay_reg[7] & 2) { who = 4, ccx_when = channel_B.when; }
			else channel_B.fastForward(ccx_end);
		}
		if (channel_C.when < ccx_when) // channel C
		{
			if (ay_reg[10] && ~ay_reg[7] & 4) { who = 5, ccx_when = channel_C.when; }
			else channel_C.fastForward(ccx_end);
		}
		if (envelope.when < ccx_when)
		{
			if ((channel_A.volume | channel_B.volume | channel_C.volume) & 0x10) { who = 2, ccx_when = envelope.when; }
			else envelope.fastForward(ccx_end);
		}

		if (ccx_when > ccx_now)
		{
			// update current output value:
			int a = output_of(channel_A);
			int b = output_of(channel_B);
			int c = output_of(channel_C);

			if constexpr (nc == 2)
			{
				switch (int(stereo_mix))
				{
				default:
				case Mono: // mono: ZX 128k, +2, +3, +3A, TS2068, TC2068
					current_value = a + b + c;
					break;
				case ABCstereo: // western Europe
					current_value.channels[0] = 2 * a + b;
					current_value.channels[1] = b + 2 * c;
					break;
				case ACBstereo: // eastern Europe, Didaktik Melodik
					current_value.channels[0] = 2 * a + c;
					current_value.channels[1] = c + 2 * b;
					break;
				}
			}
			else
			{
				current_value = a + b + c; // mono
			}

			// emit samples
			if (ccx_when < ccx_at_sos + ccx_per_sample)
			{
				current_sample += current_value * (ccx_when - ccx_now);
				ccx_now = ccx_when;
			}
			else
			{
				*output_buffer++ = (current_sample + current_value * (ccx_at_sos + ccx_per_sample - ccx_now)) >> 16;
				ccx_at_sos += ccx_per_sample;

				while (ccx_at_sos + ccx_per_sample <= ccx_when)
				{
					*output_buffer++ = (current_value * ccx_per_sample) >> 16;
					ccx_at_sos += ccx_per_sample;
				}

				current_sample = current_value * (ccx_when - ccx_at_sos);
				ccx_now		   = ccx_when;
			}
		}

		// handle next:
		switch (who)
		{
		case 1: noise.trigger(); break;
		case 2: envelope.trigger(); break;
		case 3: channel_A.trigger(); break;
		case 4: channel_B.trigger(); break;
		case 5: channel_C.trigger(); break;
		default: return;
		}
	}
}


// instantiate both.
// the linker will know what we need:

template class Ay38912<1>;
template class Ay38912<2>;


} // namespace kio::Audio


/*




































*/
