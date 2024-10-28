// Copyright (c) 1995 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Ay38912.h"
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
	repeat	  = false;
	invert	  = false;
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
	invert	  = c & 2;
	repeat	  = !(c & 1);

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
			if (repeat)
			{
				if (invert)
				{
					index	  = ~index;
					direction = -direction;
				}
			}
			else
			{
				direction = 0;
				if (!invert) index = ~index;
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
		if (repeat)
		{
			if (invert && (index & 0x10))
			{
				index	  = ~index;
				direction = -direction;
			}
		}
		else
		{
			direction = 0;
			if (!invert) index = ~index;
		}
		index &= 0x0F;
	}
}


/* ===============================================================
			ay sound chip
=============================================================== */

template<uint nc>
Ay38912<nc>::Ay38912(float ay_frequency, StereoMix mix, float volume) noexcept :
	stereo_mix(nc == 2 ? mix : mono),
	clock_frequency(ay_frequency),
	sample_frequency(hw_sample_frequency)
{
	setVolume(volume);
	reset();
}

template<uint nc>
float Ay38912<nc>::nextHigherClock(float f, float sample_frequency) noexcept
{
	return sample_frequency * ceilf(f * (1 << ccx_fract_bits) / sample_frequency) / (1 << ccx_fract_bits);
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

	ccx_per_sample = int(clock_frequency * (1 << ccx_fract_bits) / sample_frequency + 0.5f);

	v = v * float(0x7fffffff);	   // wg. scale float -> int32
	v = v / 3;					   // wg. 3 channels added
	v = v / float(ccx_per_sample); // wg. accumulation in resampling

	float base = -v;

	assert_ge(int(base) * 3 * ccx_per_sample, -0x7fffffff);
	assert_le(int(base) * 3 * ccx_per_sample, +0x7fffffff);
	assert_ge(int(base + 2 * v) * 3 * ccx_per_sample, -0x7fffffff);
	assert_le(int(base + 2 * v) * 3 * ccx_per_sample, +0x7fffffff);

	log_vol[0] = int(base);
	for (int i = 15; i; i--)
	{
		log_vol[i] = int(base + 2 * v);
		v *= 0.75f;
	}
}

template<uint nc>
void Ay38912<nc>::setClock(float new_frequency) noexcept
{
	assert(output_buffer == nullptr);
	clock_frequency = new_frequency;
	setVolume(volume);
}

template<uint nc>
void Ay38912<nc>::setSampleRate(float hw_sample_frequency) noexcept
{
	sample_frequency = hw_sample_frequency;
	setVolume(volume);
}

template<uint nc>
void Ay38912<nc>::setStereoMix(StereoMix mix) noexcept
{
	if (nc == 2) stereo_mix = mix; // else mono must be mono
}

template<uint nc>
void Ay38912<nc>::setRegister(uint regnr, uint8 newvalue) noexcept
{
	regnr &= 0x0F;
	newvalue &= ayRegisterBitMasks[regnr];

	//	if (AYreg[regnr]==newvalue && regnr!=13) break;		---> eel_demo !!!
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

	ay_reg[regnr] = newvalue;
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
	ay_reg_nr = 0;
}

template<uint nc>
void Ay38912<nc>::reset(CC cc) noexcept
{
	run_up_to_cycle(cc << ccx_fract_bits);
	reset();
}

template<uint nc>
void Ay38912<nc>::reset(CC cc, WritePortProc& callback)
{
	setRegister(cc, 7, ayRegisterResetValues[7], callback);
	reset();
}

template<uint nc>
void Ay38912<nc>::setRegister(CC cc, uint r, uint8 n) noexcept
{
	run_up_to_cycle(cc << ccx_fract_bits);
	setRegister(r, n);
}

template<uint nc>
void Ay38912<nc>::setRegister(CC cc, uint r, uint8 value, WritePortProc& callback)
{
	run_up_to_cycle(cc << ccx_fract_bits);

	if (r == 7)
	{
		uint8 t = value ^ ay_reg[7];
		if (t & 0x40 && ay_reg[14] != 0xff) callback(cc, 0, value & 0x40 ? ay_reg[14] : 0xff);
		if (t & 0x80 && ay_reg[15] != 0xff) callback(cc, 1, value & 0x80 ? ay_reg[15] : 0xff);
	}
	else if (r >= 14)
	{
		if (ay_reg[r] != value && ay_reg[7] & (1 << (r & 7))) callback(cc, r & 1, value);
	}

	setRegister(r, value);
}

template<uint nc>
uint8 Ay38912<nc>::readRegister(CC cc, ReadPortProc& callback)
{
	uint r = ay_reg_nr;
	return (r < 14) ? ay_reg[r] : (ay_reg[7] & (1 << (r & 7)) ? ay_reg[r] : 0xff) & callback(cc, r & 1);
}

template<uint nc>
void Ay38912<nc>::shift_timebase(int delta_ccx) noexcept
{
	// subtract delta_ccx from the current clock cycle

	channel_A.when -= delta_ccx;
	channel_B.when -= delta_ccx;
	channel_C.when -= delta_ccx;
	noise.when -= delta_ccx;
	envelope.when -= delta_ccx;
	ccx_now -= delta_ccx;
	ccx_at_sos -= delta_ccx;
	ccx_buffer_end -= delta_ccx;
}

template<uint nc>
void Ay38912<nc>::shiftTimebase(int delta_cc) noexcept
{
	shift_timebase(delta_cc << ccx_fract_bits); //
}

template<uint nc>
void Ay38912<nc>::resetTimebase() noexcept
{
	shift_timebase(ccx_now.value); //
}


template<uint nc>
auto Ay38912<nc>::audioBufferStart(AudioSample<nc>* buffer, uint num_samples) noexcept -> CC
{
	output_buffer = buffer;
	ccx_buffer_end += int(num_samples) * ccx_per_sample;
	return CC(int(ccx_buffer_end >> ccx_fract_bits));
}

template<uint nc>
void Ay38912<nc>::audioBufferEnd() noexcept
{
	run_up_to_cycle(CCx(int(ccx_buffer_end)));
	output_buffer = nullptr;
}

template<uint nc>
void Ay38912<nc>::run_up_to_cycle(CCx ccx_end) noexcept
{
	if (ccx_end > CCx(int(ccx_buffer_end))) ccx_end = CCx(int(ccx_buffer_end));

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
			if ((channel_A.volume | channel_B.volume | channel_C.volume) & 0x10)
			{
				who		 = 2;
				ccx_when = envelope.when;
			}
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
				case mono: // mono: ZX 128k, +2, +3, +3A, TS2068, TC2068
					current_value = a + b + c;
					break;
				case abc_stereo: // western Europe
					current_value.channels[0] = 2 * a + b;
					current_value.channels[1] = b + 2 * c;
					break;
				case acb_stereo: // eastern Europe, Didaktik Melodik
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
