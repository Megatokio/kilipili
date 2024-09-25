// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "AudioSample.h"
#include "basic_math.h"
#include "common/RCPtr.h"
#include "common/no_copy_move.h"
#include <string.h>

namespace kio::Audio
{

// actual HW sample frequency used by AudioController:
extern float hw_sample_frequency;


/* _______________________________________________________________________________________
   interface for audio source:
   the AudioController uses AudioSource<hw_num_channels> as it's audio sources.
*/
template<uint num_channels>
class AudioSource : public RCObject
{
	NO_COPY_MOVE(AudioSource);

public:
	virtual ~AudioSource() override = default;

	/* callback from the AudioController:
	   store audio into buffer.
	   return number of samples written: must be same as requested except if AudioSource can be removed.
	*/
	virtual uint getAudio(AudioSample<num_channels>* buffer, uint num_frames) noexcept = 0;

	/* callback from the AudioController:
	   hw_sample_frequency was changed by the application.
	   called synchronized with getAudio().
	*/
	virtual void setSampleRate(float /*new_sample_frequency*/) noexcept {}

protected:
	AudioSource() noexcept {}
};

using MonoSource   = AudioSource<1>;
using StereoSource = AudioSource<2>;


/* _______________________________________________________________________________________
   convert stereo to mono or mono to stereo:
*/
template<uint source_channels, uint dest_channels>
class NumChannelsAdapter : public AudioSource<dest_channels>
{
	RCPtr<AudioSource<source_channels>> audio_source;

public:
	NumChannelsAdapter(RCPtr<AudioSource<source_channels>> source) noexcept : audio_source(std::move(source)) {}

	virtual uint getAudio(AudioSample<dest_channels>* dest, uint num_frames) noexcept override
	{
		AudioSample<source_channels> source[64];
		uint						 remaining = num_frames;
		while (remaining)
		{
			uint count = audio_source->getAudio(source, min(remaining, NELEM(source)));
			if (count == 0) break;
			for (uint i = 0; i < count; i++) dest[i] = source[i];
			dest += count;
			remaining -= count;
		}
		return num_frames - remaining;
	}

	virtual void setSampleRate(float new_sample_frequency) noexcept override
	{
		audio_source->setSampleRate(new_sample_frequency);
	}
};

using MakeMonoAdapter	= NumChannelsAdapter<2, 1>;
using MakeStereoAdapter = NumChannelsAdapter<1, 2>;


/* _______________________________________________________________________________________
   join two mono sources to stereo source:
*/

class JoinStereoAdapter : public StereoSource
{
	RCPtr<MonoSource> left_source;
	RCPtr<MonoSource> right_source;

public:
	JoinStereoAdapter(RCPtr<MonoSource> left, RCPtr<MonoSource> right) noexcept :
		left_source(std::move(left)),
		right_source(std::move(right))
	{}

	virtual uint getAudio(StereoSample* dest, uint num_frames) noexcept override
	{
		MonoSample source[64];
		uint	   remaining = num_frames;
		while (remaining)
		{
			uint cnt = left_source->getAudio(source, min(remaining, NELEM(source)));
			for (uint i = 0; i < cnt; i++) dest[i].l = source[i].m;
			cnt = right_source->getAudio(source, cnt);
			for (uint i = 0; i < cnt; i++) dest[i].r = source[i].m;
			if (cnt == 0) break;
			dest += cnt;
			remaining -= cnt;
		}
		return num_frames - remaining;
	}

	virtual void setSampleRate(float new_sample_frequency) noexcept override
	{
		left_source->setSampleRate(new_sample_frequency);
		right_source->setSampleRate(new_sample_frequency);
	}
};


/* _______________________________________________________________________________________
   add volume control to a source:
*/
template<uint num_channels>
class SetVolumeAdapter : public AudioSource<num_channels>
{
	RCPtr<AudioSource<num_channels>> source;
	Sample							 volume;

public:
	SetVolumeAdapter(RCPtr<AudioSource<num_channels>> source, float volume) noexcept :
		source(std::move(source)),
		volume(Sample(minmax(-0x8000, int(volume * 0x8000), 0x7fff)))
	{}

	virtual uint getAudio(AudioSample<num_channels>* dest, uint num_frames) noexcept override
	{
		uint count = source->getAudio(dest, num_frames);

		Sample* z = &dest[0][0];
		for (uint i = 0; i < count * num_channels; i++) { z[i] = (z[i] * volume) >> 15; }
		return count;
	}

	virtual void setSampleRate(float new_sample_frequency) noexcept override
	{
		source->setSampleRate(new_sample_frequency);
	}

	void setVolume(float v) noexcept { volume = Sample(minmax(-0x8000, int(v * 0x8000), 0x7fff)); }
};

SetVolumeAdapter(RCPtr<AudioSource<1>>, float)->SetVolumeAdapter<1>;
SetVolumeAdapter(RCPtr<AudioSource<2>>, float)->SetVolumeAdapter<2>;


/* _______________________________________________________________________________________
   resample source to target sample rate:
*/
template<uint nc>
class SampleRateAdapter : public AudioSource<nc>
{
	RCPtr<AudioSource<nc>> audio_source;
	float				   source_frequency;
	float				   dest_frequency;

	float			source_samples_per_dest_samples;
	float			position_in_source = -1;
	AudioSample<nc> last_source_sample = 0;
	AudioSample<nc> last_second_sample = 0;

public:
	SampleRateAdapter(RCPtr<AudioSource<nc>> source, float source_freq, float dest_freq = hw_sample_frequency) :
		audio_source(std::move(source)),
		source_frequency(source_freq),
		dest_frequency(dest_freq),
		source_samples_per_dest_samples(source_freq / dest_freq)
	{}

	virtual void setSampleRate(float new_sample_frequency) noexcept override
	{
		dest_frequency					= new_sample_frequency;
		source_samples_per_dest_samples = source_frequency / dest_frequency;
	}

	void setSourceSampleRate(float new_source_frequency) noexcept
	{
		source_frequency				= new_source_frequency;
		source_samples_per_dest_samples = source_frequency / dest_frequency;
	}

	virtual uint getAudio(AudioSample<nc>* dest, uint num_frames) noexcept override
	{
		AudioSample<nc> source[64] = {};
		uint			zi = 0, qi = 0, cnt = 0;

		while (zi < num_frames)
		{
			while (position_in_source < 0)
			{
				if unlikely (qi >= cnt)
				{
					cnt = uint((num_frames - zi) * source_samples_per_dest_samples - position_in_source + 0.99f);
					assert_ne(cnt, 0);
					cnt = audio_source->getAudio(source, min(cnt, NELEM(source)));
					if (cnt == 0) return zi;
					qi = 0u;
				}

				position_in_source += 1;
				last_source_sample = last_second_sample;
				last_second_sample = source[qi++];
			}

			dest[zi++] = last_source_sample * position_in_source + last_second_sample * (1 - position_in_source);
			position_in_source -= source_samples_per_dest_samples;
		}

		while (qi < cnt)
		{
			position_in_source += 1;
			last_source_sample = last_second_sample;
			last_second_sample = source[qi++];
		}

		assert_lt(position_in_source, 1);
		return num_frames;
	}
};

SampleRateAdapter(RCPtr<AudioSource<1>>, float, float)->SampleRateAdapter<1>;
SampleRateAdapter(RCPtr<AudioSource<2>>, float, float)->SampleRateAdapter<2>;
SampleRateAdapter(RCPtr<AudioSource<1>>, float)->SampleRateAdapter<1>;
SampleRateAdapter(RCPtr<AudioSource<2>>, float)->SampleRateAdapter<2>;


/* _______________________________________________________________________________________
   dummy source which provides silence:
*/
template<uint num_channels>
class NoAudioSource : public AudioSource<num_channels>
{
public:
	virtual uint getAudio(AudioSample<num_channels>* dest, uint num_frames) noexcept override
	{
		memset(dest, 0, num_frames * sizeof(*dest));
		return num_frames;
	}
};


/* _______________________________________________________________________________________
   source which provides a square wave:
*/
template<uint num_channels>
class SquareWaveSource : public AudioSource<num_channels>
{
public:
	float  frequency;
	float  sample_frequency;
	float  samples_per_phase;
	float  position_in_phase = 0;
	Sample sample, _padding;

	SquareWaveSource(float frequency, float volume, float sample_frequency = hw_sample_frequency) :
		frequency(frequency),
		sample_frequency(sample_frequency),
		samples_per_phase(sample_frequency / frequency * 0.5f),
		sample(Sample(minmax(-0x7fff, int(volume * 0x8000), 0x7fff)))
	{}

	void setVolume(float v) noexcept
	{
		if (sample < 0) v = -v;
		sample = Sample(minmax(-0x7fff, int(v * 0x8000), 0x7fff));
	}

	void setFrequency(float f) noexcept
	{
		frequency		  = f;
		samples_per_phase = sample_frequency / f * 0.5f;
	}

	virtual void setSampleRate(float new_sample_frequency) noexcept override
	{
		sample_frequency  = new_sample_frequency;
		samples_per_phase = new_sample_frequency / frequency * 0.5f;
	}

	virtual uint getAudio(AudioSample<num_channels>* dest, uint num_frames) noexcept override
	{
		for (uint i = 0; i < num_frames; i++)
		{
			if (position_in_phase >= samples_per_phase)
			{
				position_in_phase -= samples_per_phase;
				sample = -sample;
			}

			position_in_phase++;
			*dest++ = sample;
		}
		return num_frames;
	}
};


/* _______________________________________________________________________________________
   source which provides a sine wave:
*/
constexpr int		nelem_quarter_sine = 16;
extern const uint16 quarter_sine[nelem_quarter_sine + 1];

template<uint num_channels>
class SineWaveSource : public AudioSource<num_channels>
{
public:
	float sample_frequency;
	float frequency;
	float step_per_sample;

	int position = 0;	  // * 0x10000
	int steps_per_sample; // * 0x10000
	int volume;			  // * 0x8000

	// for low frequency the steps_per_sample can become small => frequency accuracy becomes worse:
	// 4*16 * 0x10000 * 20 / 60000 = 1398 (still pretty good)
	// 4*16 * 0x10000 * 1 / 44100 = 95 (still pretty good)

	// for high frequency the steps per sample become large => entire quarter phases can be skipped:
	// 4*16 * 10000 / 20000 = 2 * 16 (two quarter phases)

	constexpr int calc_steps_per_sample(float f, float sf) noexcept
	{
		return int(0x10000 * nelem_quarter_sine * 4 * f / sf);
	}

	SineWaveSource(float frequency, float volume, float sample_frequency = hw_sample_frequency) :
		sample_frequency(sample_frequency),
		frequency(minmax(1.f, frequency, sample_frequency / 2)),
		steps_per_sample(calc_steps_per_sample(frequency, sample_frequency)),
		volume(minmax(-0x8000, int(volume * 0x8000), 0x8000))
	{}

	void setVolume(float new_volume) noexcept
	{
		volume = minmax(-0x8000, int(new_volume * 0x8000), 0x8000); //
	}

	void setFrequency(float new_frequency) noexcept
	{
		frequency		 = minmax(1.f, new_frequency, sample_frequency / 2);
		steps_per_sample = calc_steps_per_sample(frequency, sample_frequency);
	}

	virtual void setSampleRate(float new_sample_frequency) noexcept override
	{
		sample_frequency = new_sample_frequency;
		steps_per_sample = calc_steps_per_sample(frequency, sample_frequency);
	}

	virtual uint getAudio(AudioSample<num_channels>* z, uint num_frames) noexcept override
	{
		for (uint zi = 0; zi < num_frames; zi++)
		{
			assert(position >= 0 && position <= nelem_quarter_sine * 0x10000);

			int i1 = position >> 16; // left pole for interpolation
			int i2 = i1 + 1;		 // right pole

			uint16 s1 = quarter_sine[i1]; // left value for interpolation
			uint16 s2 = quarter_sine[i2]; // right value

			uint frac	= position & 0xffff; // distance from left pole
			uint sample = s2 * frac + s1 * (0x10000 - frac);

			z[zi] = (uint16(sample >> 16) * volume) >> 16;

			position += steps_per_sample;

			if (uint(position) < uint(nelem_quarter_sine * 0x10000)) continue;

			// advance to next quarter, possibly skipping whole quarter(s):
			for (;;)
			{
				if (position < 0)
				{
					assert(steps_per_sample < 0);
					position		 = -position;
					volume			 = -volume;
					steps_per_sample = -steps_per_sample;
				}
				if (position > nelem_quarter_sine * 0x10000)
				{
					assert(steps_per_sample > 0);
					position		 = 2 * nelem_quarter_sine * 0x10000 - position;
					steps_per_sample = -steps_per_sample;
				}
				else break;
			}
		}
		return num_frames;
	}
};


} // namespace kio::Audio


/*










































*/
