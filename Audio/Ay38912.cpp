// Copyright (c) 1995 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

/*	AY-3-8912 sound chip emulation
	------------------------------

	24.Mar.1995 kio	First revision on MacOS
	05.Jun.1995 kio	Combined ear and mic sound output
	29.Nov.1995 kio	AY-3-8912 emulation
	05.Jan.1999 kio	port to Linux
	09.jan.1999 kio	ay emu fixed, perfect sound sampling
	31.dec.1999 kio	started port back to MacOS
	13.jul.2000 kio implemented signals
	06.oct.2024 kio port for microcontroller
*/

#include "Ay38912.h"
#include "common/cdefs.h"
#include <cstring>


namespace kio::Audio
{

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

	if (ch.sound_enable && !ch.sound_in) return 0;
	if (ch.noise_enable && !(noise.shiftreg & 1)) return 0;
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
	shiftreg		  = (oldshiftreg >> 1) + ((oldshiftreg << 16) ^ (oldshiftreg << 14));
	when += reload;
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
void Ay38912<nc>::Envelope::setPeriod(uint16 n)
{
	reload = (n ? n : 1) * predivider;
}

template<uint nc>
void Ay38912<nc>::Envelope::setShape(CC now, uint8 c)
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
	else when = 0x7fffffff;
}


/* ===============================================================
			ay sound chip
=============================================================== */

template<uint nc>
Ay38912<nc>::Ay38912(float ay_frequency, StereoMix mix, float volume) noexcept :
	stereo_mix(nc == 2 ? mix : mono),
	frequency(ay_frequency),
	hw_frequency(hw_sample_frequency)
{
	setVolume(volume);
	reset();
}


template<uint nc>
void Ay38912<nc>::setVolume(float v) noexcept
{
	// set volume and compute logarithmic volume table.
	// precompensate for all the multiplications and divisions etc.

	// the Ay chip had a stepping of ~3.5 dB

	if (v < -1) v = -1;
	if (v > +1) v = +1;
	volume = v;

	cc_per_sample = int(frequency * oversampling / hw_frequency + 0.5f);

	v = v * 0xfffe;				  // wg. max. value ~ 1.0
	v = v * 0x10000;			  // wg. >> 16
	v = v / 3;					  // wg. 3 channels added
	v = v / float(cc_per_sample); // wg. accumulation in resampling

	float base = -v / 2;

	assert(int(base) * 3 * cc_per_sample >= -0x7ffffff);
	assert(int(base) * 3 * cc_per_sample <= +0x7ffffff);
	assert(int(base + v) * 3 * cc_per_sample >= -0x7fffffff);
	assert(int(base + v) * 3 * cc_per_sample <= +0x7fffffff);

	log_vol[0] = int(base);
	for (int i = 15; i; i--)
	{
		log_vol[i] = int(base + v);
		v *= 0.8f;
	}
}

template<uint nc>
void Ay38912<nc>::setClock(float new_frequency) noexcept
{
	frequency = new_frequency;
	setVolume(volume);
}

template<uint nc>
void Ay38912<nc>::setSampleRate(float hw_sample_frequency) noexcept
{
	hw_frequency = hw_sample_frequency;
	setVolume(volume);
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
	case 13: envelope.setShape(cc_now, newvalue); break;
	default: break;
	}

	ay_reg[regnr] = newvalue;
}

template<uint nc>
void Ay38912<nc>::reset() noexcept
{
	for (uint r = 0; r < 16; r++) { setRegister(r, ayRegisterResetValues[r]); }
	ay_reg_nr = 0;
}

template<uint nc>
void Ay38912<nc>::reset(int cc) noexcept
{
	run_up_to_cycle(cc << 8);
	reset();
}

template<uint nc>
void Ay38912<nc>::reset(int cc, WritePortProc& callback)
{
	ay_reg_nr = 7;
	writeRegister(cc, ayRegisterResetValues[7], callback);
	reset();
}

template<uint nc>
void Ay38912<nc>::setRegister(CC cc, uint r, uint8 n) noexcept
{
	run_up_to_cycle(cc << 8);
	setRegister(r, n);
}

template<uint nc>
void Ay38912<nc>::setRegister(CC cc, uint r, uint8 value, WritePortProc& callback)
{
	run_up_to_cycle(cc << 8);

	if (r == 7)
	{
		uint8 t = value ^ ay_reg[7];
		if (t & 0x40 && ay_reg[14] != 0xff) callback(cc_now, 0, value & 0x40 ? ay_reg[14] : 0xff);
		if (t & 0x80 && ay_reg[15] != 0xff) callback(cc_now, 1, value & 0x80 ? ay_reg[15] : 0xff);
	}
	else if (r >= 14)
	{
		if (ay_reg[r] != value && ay_reg[7] & (1 << (r & 7))) callback(cc_now, r & 1, value);
	}

	setRegister(r, value);
}

template<uint nc>
uint8 Ay38912<nc>::readRegister(int cc, ReadPortProc& callback)
{
	uint r = ay_reg_nr;
	return (r < 14) ? ay_reg[r] : (ay_reg[7] & (1 << (r & 7)) ? ay_reg[r] : 0xff) & callback(cc, r & 1);
}

template<uint nc>
int /*CC256*/ Ay38912<nc>::audioBufferStart(AudioSample<nc>* buffer, uint num_samples) noexcept
{
	output_buffer  = buffer;
	current_sample = 0;

	channel_A.when -= cc_now;
	channel_B.when -= cc_now;
	channel_C.when -= cc_now;
	noise.when -= cc_now;
	envelope.when -= cc_now;

	cc_now	  = 0;
	cc_at_sos = 0;
	cc_end	  = 0 + int(num_samples) * cc_per_sample;

	return cc_end;
}

template<uint nc>
void Ay38912<nc>::audioBufferEnd() noexcept
{
	run_up_to_cycle(cc_end);
	assert_eq(cc_now, cc_end);
	output_buffer = nullptr;
}

template<uint nc>
void Ay38912<nc>::run_up_to_cycle(CC256 cc_end) noexcept
{
	// generate sound up to cc_end

	cc_end *= oversampling;
	if unlikely (cc_end > this->cc_end) cc_end = this->cc_end;

	while (true)
	{
		// who is next ?
		int who	 = 0; // finish
		CC	when = cc_end;

		if (noise.when < when)
		{
			if (~ay_reg[7] & 0x38) { who = 1, when = noise.when; }
			else noise.when += 512 * oversampling;
		}
		if (channel_A.when < when) // channel A
		{
			if (ay_reg[8] && ~ay_reg[7] & 1) { who = 3, when = channel_A.when; }
			else channel_A.when += 1024 * oversampling;
		}
		if (channel_B.when < when) // channel B
		{
			if (ay_reg[9] && ~ay_reg[7] & 2) { who = 4, when = channel_B.when; }
			else channel_B.when += 1024 * oversampling;
		}
		if (channel_C.when < when) // channel C
		{
			if (ay_reg[10] && ~ay_reg[7] & 4) { who = 5, when = channel_C.when; }
			else channel_C.when += 1024 * oversampling;
		}
		if (envelope.when < when)
		{
			if ((channel_A.volume | channel_B.volume | channel_C.volume) & 0x10)
			{
				who	 = 2;
				when = envelope.when;
			}
			else if (envelope.direction) envelope.when += 1024 * oversampling;
			else envelope.when = 0x7fffffff;
		}

		// emit samples
		if (when > cc_now)
		{
			if (when < cc_at_sos + cc_per_sample)
			{
				current_sample += current_value * (when - cc_now);
				cc_now = when;
			}
			else
			{
				*output_buffer++ = (current_sample + current_value * (cc_at_sos + cc_per_sample - cc_now)) >> 16;
				cc_at_sos += cc_per_sample;

				while (cc_at_sos + cc_per_sample <= when)
				{
					*output_buffer++ = (current_value * cc_per_sample) >> 16;
					cc_at_sos += cc_per_sample;
				}

				current_sample = current_value * (when - cc_at_sos);
				cc_now		   = when;
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
	}
}


// instantiate both.
// the linker will know what we need:

template class Ay38912<1>;
template class Ay38912<2>;


} // namespace kio::Audio

/*




































*/
