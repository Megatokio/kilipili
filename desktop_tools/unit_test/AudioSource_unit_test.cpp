// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Audio/AudioSource.h"
#include "cstrings.h"
#include "doctest.h"
#include <cmath>
#include <cstdio>


namespace kio::Audio
{
float hw_sample_frequency = 44100; // normally in AudioController.cpp

template<typename SAMPLE>
doctest::String toString(AudioSample<1, SAMPLE> s)
{
	return usingstr("<%i>", s.m);
}

template<typename SAMPLE>
doctest::String toString(AudioSample<2, SAMPLE> s)
{
	return usingstr("<%i,%i>", s.l, s.r);
}
} // namespace kio::Audio


namespace kio::Test
{

using namespace kio;
using namespace kio::Audio;


TEST_CASE("Audio::AudioSource ctor, instantiation")
{
	// ctor:
	RCPtr<SampleRateAdapter<1>> sra1 = new SampleRateAdapter<1>(new NoAudioSource<1>(), 123e6f);
	RCPtr<SampleRateAdapter<2>> sra2 = new SampleRateAdapter<2>(new NoAudioSource<2>(), 123e6f, 44100);
	CHECK_EQ(1, 1); // count the test

	// deduction guide:
	RCPtr<SampleRateAdapter<1>> sra11 = new SampleRateAdapter(new NoAudioSource<1>(), 123e6f, hw_sample_frequency);
	RCPtr<SampleRateAdapter<2>> sra21 = new SampleRateAdapter(new NoAudioSource<2>(), 123e6f);
	CHECK_EQ(1, 1); // count the test

	// ctor:
	RCPtr<MonoSource>	o1 = new MakeMonoAdapter(new NoAudioSource<2>);
	RCPtr<StereoSource> o2 = new MakeStereoAdapter(new NoAudioSource<1>);
	CHECK_EQ(1, 1); // count the test

	o1 = new MakeMonoAdapter(sra2);
	o2 = new MakeStereoAdapter(sra1);
	CHECK_EQ(1, 1); // count the test

	// ctor:
	RCPtr<RCObject> o3 = new JoinStereoAdapter(sra1, o1);
	CHECK_EQ(1, 1); // count the test

	// ctor:
	o1 = new SetVolumeAdapter<1>(o1, 0.5f);
	o2 = new SetVolumeAdapter<2>(o2, 0.5f);
	CHECK_EQ(1, 1); // count the test

	// deduction guide:
	o1 = new SetVolumeAdapter(o1, 0.5f);
	o2 = new SetVolumeAdapter(o2, 0.5f);
	CHECK_EQ(1, 1); // count the test

	// ctor:
	o1 = new NoAudioSource<1>();
	o2 = new NoAudioSource<2>();

	// ctor:
	o1 = new SquareWaveSource<1>(440, 0.1f);
	o2 = new SquareWaveSource<2>(440, 0.1f);
	o1 = new SquareWaveSource<1>(440, 44100, 0.1f);
	o2 = new SquareWaveSource<2>(440, 22050, 0.1f);
}

template<uint num_channels>
struct NumberProvider : public AudioSource<num_channels>
{
	AudioSample<num_channels> s;
	NumberProvider(AudioSample<num_channels> start) : s(start) {}
	virtual uint getAudio(AudioSample<num_channels>* buffer, uint num_frames) noexcept override
	{
		for (uint i = 0; i < num_frames; i++)
		{
			buffer[i] = s;
			s		  = AudioSample<num_channels, int>(s) + AudioSample<num_channels, int>(1);
		}
		return num_frames;
	}
};

TEST_CASE("Audio::MakeStereoAdapter")
{
	Sample					 v	= -99;
	RCPtr<NumberProvider<1>> m1 = new NumberProvider<1>(v);
	MakeStereoAdapter		 sa(m1);

	constexpr uint busize = 128 + 10;
	StereoSample   bu[busize];
	for (uint sz = 1; sz <= busize; sz++)
	{
		uint n = sa.getAudio(bu, sz);
		CHECK_EQ(n, sz);
		for (uint i = 0; i < sz; i++)
		{
			CHECK_EQ(bu[i], StereoSample(v, v));
			v++;
		}
	}
}

TEST_CASE("Audio::MakeMonoAdapter")
{
	Sample					 v1			  = -99;
	Sample					 v2			  = -9999;
	RCPtr<NumberProvider<2>> stereosource = new NumberProvider<2>(StereoSample(v1, v2));
	MakeMonoAdapter			 ma(stereosource);

	constexpr uint busize = 128 + 10;
	MonoSample	   bu[busize];
	for (uint sz = 1; sz <= busize; sz++)
	{
		uint n = ma.getAudio(bu, sz);
		CHECK_EQ(n, sz);
		for (uint i = 0; i < sz; i++)
		{
			CHECK_EQ(bu[i], MonoSample((v1 + v2) >> 1));
			v1++;
			v2++;
		}
	}
}

TEST_CASE("Audio::JoinStereoAdapter")
{
	Sample					 v1 = -8888;
	Sample					 v2 = 9999;
	RCPtr<NumberProvider<1>> m1 = new NumberProvider<1>(v1);
	RCPtr<NumberProvider<1>> m2 = new NumberProvider<1>(v2);
	JoinStereoAdapter		 sa(m1, m2);

	constexpr uint busize = 128 + 10;
	StereoSample   bu[busize];
	for (uint sz = 1; sz <= busize; sz++)
	{
		uint n = sa.getAudio(bu, sz);
		CHECK_EQ(n, sz);
		for (uint i = 0; i < sz; i++)
		{
			CHECK_EQ(bu[i], StereoSample(v1, v2));
			v1++;
			v2++;
		}
	}
}

TEST_CASE_TEMPLATE("Audio::SetVolumeAdapter", AudioSample, MonoSample, StereoSample)
{
	constexpr uint				ch	   = NELEM(AudioSample::channels);
	Audio::AudioSample<ch, int> v	   = AudioSample(-10, -99);
	Audio::AudioSample<ch, int> d	   = AudioSample(1, 1);
	RCPtr<NumberProvider<ch>>	source = new NumberProvider<ch>(v);
	RCPtr<SetVolumeAdapter<ch>> sva	   = new SetVolumeAdapter<ch>(source, 0);

	constexpr uint busize = 128 + 10;
	AudioSample	   bu[busize];

	for (float volume = -0.6f; volume <= +0.4001f; volume += 1.f)
	{
		sva->setVolume(volume);
		int sample_volume = int(volume * 0x8000);

		for (uint sz = 1; sz <= busize; sz++)
		{
			uint n = sva->getAudio(bu, sz);
			CHECK_EQ(n, sz);
			for (uint i = 0; i < sz; i++)
			{
				CHECK_EQ(bu[i], AudioSample((v * sample_volume) >> 15));
				v += d;
			}
		}
	}
}

TEST_CASE_TEMPLATE("Audio::NoAudioSource", AudioSample, MonoSample, StereoSample)
{
	constexpr uint	  ch = NELEM(AudioSample::channels);
	NoAudioSource<ch> nas;

	constexpr uint busize = 128 + 10;
	AudioSample	   bu[busize];

	for (uint sz = 1; sz < busize; sz++)
	{
		memset(bu, 66, sizeof(bu));
		uint n = nas.getAudio(bu, sz);
		CHECK_EQ(n, sz);
		for (uint i = 0; i < sz; i++) { CHECK_EQ(bu[i], AudioSample(0)); }
		CHECK_EQ(bu[n], AudioSample(Sample(66 * 257)));
	}
}

TEST_CASE_TEMPLATE("Audio::SquareWaveSource", AudioSample, MonoSample, StereoSample)
{
	constexpr uint ch = NELEM(AudioSample::channels);

	{ // generates s.th. like a wave with the expected volume and ~frequency?
		hw_sample_frequency = 44100;
		float  volume		= 0.3f;
		Sample intvolume	= Sample(0x8000 * volume);
		Sample hi			= +intvolume;
		Sample lo			= -intvolume;

		SquareWaveSource<ch> sws(1000, volume);
		AudioSample			 bu[140];

		uint n = sws.getAudio(bu, 140);
		CHECK_EQ(n, 140);
		CHECK_EQ(bu[0], AudioSample(hi));
		CHECK_EQ(bu[11], AudioSample(hi));
		CHECK_EQ(bu[33], AudioSample(lo));
		CHECK_EQ(bu[55], AudioSample(hi));
		CHECK_EQ(bu[77], AudioSample(lo));
		CHECK_EQ(bu[99], AudioSample(hi));
		CHECK_EQ(bu[121], AudioSample(lo));
	}
	{ // test setVolume()
		hw_sample_frequency = 44100;
		float  volume		= 0.3f;
		Sample intvolume	= Sample(0x8000 * volume);
		Sample hi			= +intvolume;
		Sample lo			= -intvolume;

		SquareWaveSource<ch> sws(1000, 0.f);
		sws.setVolume(0.3f);
		AudioSample bu[140];

		uint n = sws.getAudio(bu, 140);
		CHECK_EQ(n, 140);
		CHECK_EQ(bu[0], AudioSample(hi));
		CHECK_EQ(bu[11], AudioSample(hi));
		CHECK_EQ(bu[33], AudioSample(lo));
		CHECK_EQ(bu[55], AudioSample(hi));
		CHECK_EQ(bu[77], AudioSample(lo));
	}
	{ // test setFrequency()
		float  volume	 = 0.3f;
		Sample intvolume = Sample(0x8000 * volume);
		Sample hi		 = +intvolume;
		Sample lo		 = -intvolume;

		SquareWaveSource<ch> sws(10, volume, 20000);
		sws.setFrequency(1000);
		AudioSample bu[140];

		uint n = sws.getAudio(bu, 140);
		CHECK_EQ(n, 140);
		CHECK_EQ(bu[0], AudioSample(hi));
		CHECK_EQ(bu[9], AudioSample(hi));
		CHECK_EQ(bu[10], AudioSample(lo));
		CHECK_EQ(bu[19], AudioSample(lo));
		CHECK_EQ(bu[20], AudioSample(hi));
		CHECK_EQ(bu[29], AudioSample(hi));
	}
	{ // test setSampleRate()
		float  volume	 = 0.3f;
		Sample intvolume = Sample(0x8000 * volume);
		Sample hi		 = +intvolume;
		Sample lo		 = -intvolume;

		SquareWaveSource<ch> sws(1000, volume, 40000);
		sws.setSampleRate(20000);
		AudioSample bu[140];

		uint n = sws.getAudio(bu, 140);
		CHECK_EQ(n, 140);
		CHECK_EQ(bu[0], AudioSample(hi));
		CHECK_EQ(bu[9], AudioSample(hi));
		CHECK_EQ(bu[10], AudioSample(lo));
		CHECK_EQ(bu[19], AudioSample(lo));
		CHECK_EQ(bu[20], AudioSample(hi));
		CHECK_EQ(bu[29], AudioSample(hi));
	}
	{ // check overall frequency
		SquareWaveSource<ch> sws(1000, 0.1f, 44100);
		AudioSample			 bu[44100 + 1];

		uint n = sws.getAudio(bu, NELEM(bu));
		CHECK_EQ(n, NELEM(bu));

		uint		f = 0;
		AudioSample s = bu[0];

		for (uint i = 0; i < NELEM(bu); i++)
		{
			if (bu[i] == s) continue;
			f++;
			s = bu[i];
		}

		CHECK_EQ(f, 1000 * 2);
	}
	{ // check overall frequency
		SquareWaveSource<ch> sws(1000, 0.1f, 44100);

		AudioSample s;
		uint		n = sws.getAudio(&s, 1);
		CHECK_EQ(n, 1);

		uint f = 0;

		for (uint i = 0; i < 100; i++)
		{
			AudioSample bu[441];
			n = sws.getAudio(bu, NELEM(bu));
			CHECK_EQ(n, NELEM(bu));

			for (uint i = 0; i < NELEM(bu); i++)
			{
				if (bu[i] == s) continue;
				f++;
				s = bu[i];
			}
		}

		CHECK_EQ(f, 1000 * 2);
	}
}


TEST_CASE_TEMPLATE("Audio::SineWaveSource", AudioSample, MonoSample, StereoSample)
{
	constexpr uint ch = NELEM(AudioSample::channels);

	float freqs[]  = {10, 100, 1000, 10000};
	float sfreqs[] = {44100, 20000, 48000, 16665};
	float vols[]   = {1.0f, 0.5f, 0.1f};

	for (uint fi = 0; fi < NELEM(freqs); fi++)
		for (uint si = 0; si < NELEM(sfreqs); si++)
			for (uint vi = 0; vi < NELEM(vols); vi++)
			{
				float f	  = freqs[fi];
				float sf  = sfreqs[si];
				float vol = vols[vi];
				if (f > sf / 2) continue;

				RCPtr<SineWaveSource<ch>> sws = new SineWaveSource<ch>(f, vol, sf);

				AudioSample bu[97];
				uint		n = sws->getAudio(bu, NELEM(bu));
				CHECK_EQ(n, NELEM(bu));

				double p = 0;
				double d = 2 * 3.1415926535898 * double(f) / double(sf);
				for (uint i = 0; i < NELEM(bu); i++)
				{
					if (ch == 2) CHECK_EQ(bu[i].left(), bu[i].right());
					int	 ref = int(sin(p) * 0x8000);
					bool pos = ref >= 0;
					bool neg = ref <= 0;
					// normally, due to interpolation, the error is towards 0:
					CHECK_GE(bu[i].left(), int(float(ref - 48 * pos - 8 * neg) * vol));
					CHECK_LE(bu[i].left(), int(float(ref + 40 * neg + 4 * pos) * vol));
					p += d;
				}
			}
}

TEST_CASE("Audio::SampleRateAdapter<1>")
{
	// check that resampling works:
	// create sine wave and convert it up and down.
	// due to linear interpolation the error is normally towards 0.
	// the tests have been tailored to just pass with the currently selected frequencies.
	// the error increases with increasing frequency / decreasing sample_frequency.

	float qfreqs[] = {44100, 22050, 48000};
	float zfreqs[] = {44100, 24000, 56785, 19997, 23055, 16666};
	float freq	   = 500;

	for (uint fi = 0; fi < NELEM(qfreqs); fi++)
	{
		for (uint si = 0; si < NELEM(zfreqs); si++)
		{
			float qf = qfreqs[fi];
			float zf = zfreqs[si];

			RCPtr<SineWaveSource<1>>	sws = new SineWaveSource<1>(freq, 1.0f, qf);
			RCPtr<SampleRateAdapter<1>> sra = new SampleRateAdapter<1>(sws, qf, zf);

			MonoSample bu[64 * 5 - 1];
			uint	   n = sra->getAudio(bu, NELEM(bu));
			CHECK_EQ(n, NELEM(bu));

			//::printf("--------------------------\n");
			double p = 0;
			double d = 2 * 3.1415926535898 * double(freq) / double(zf);
			for (uint i = 0; i < NELEM(bu); i++)
			{
				int ref = int(sin(p) * 0x8000);

				//::printf("%6i ; %6i\n", ref, bu[i].left());
				bool pos = ref >= 0;
				bool neg = ref <= 0;
				// normally, due to interpolation, the error is towards 0:
				CHECK_GE(bu[i].left(), ref - 117 * pos - 25 * neg);
				CHECK_LE(bu[i].left(), ref + 117 * neg + 23 * pos);

				p += d;
			}
		}
	}
}

TEST_CASE("Audio::SampleRateAdapter<1>")
{
	// check that resampling works:
	// create sine wave and convert it up and down and back to original sample rate.
	// the tests have been tailored to just pass with the currently selected frequencies.
	// the error increases with increasing frequency / decreasing sample_frequency.

	float qfreqs[] = {44100, 22050, 48000};
	float zfreqs[] = {44100, 24000, 56785, 19997, 23055, 16666};
	float freq	   = 500;

	for (uint fi = 0; fi < NELEM(qfreqs); fi++)
	{
		for (uint si = 0; si < NELEM(zfreqs); si++)
		{
			float qf = qfreqs[fi];
			float zf = zfreqs[si];

			RCPtr<SineWaveSource<1>>	sws1 = new SineWaveSource<1>(freq, 1.0f, qf);
			RCPtr<SineWaveSource<1>>	sws2 = new SineWaveSource<1>(freq, 1.0f, qf);
			RCPtr<SampleRateAdapter<1>> sra1 = new SampleRateAdapter<1>(sws1, qf, zf);
			RCPtr<SampleRateAdapter<1>> sra2 = new SampleRateAdapter<1>(sra1, zf, qf);

			MonoSample bu1[64 * 5 - 1];
			MonoSample bu2[64 * 5 - 1];
			uint	   n1 = sws2->getAudio(bu1, NELEM(bu1));
			CHECK_EQ(n1, NELEM(bu1));
			uint n2 = sra2->getAudio(bu2, NELEM(bu2));
			CHECK_EQ(n2, NELEM(bu2));

			//::printf("--------------------------\n");
			for (uint i = 0; i < NELEM(bu1); i++)
			{
				double val = bu1[i].mono();
				double ref = bu2[i].mono();
				if (abs(ref) <= 10) continue;
				if (abs(val) <= 10) continue;

				double r = val / ref;
				if (r > 1.022 || r < 0.98) { ::printf("%f / %f = %f\n", val, ref, r); }
				CHECK_LE(r, 1.022);
				CHECK_GE(r, 0.98);
			}
		}
	}
}

TEST_CASE("Audio::SampleRateAdapter<2>")
{
	// check that resampling stereo channel works:
	// check buffer stitching

	float freq1 = 510;
	float freq2 = 490;
	float qf	= 44100;
	float zfs[] = {24000, 56783};

	for (uint i = 0; i < NELEM(zfs); i++)
	{
		float zf = zfs[i];

		RCPtr<SineWaveSource<1>>	sws1 = new SineWaveSource<1>(freq1, 1.0f, qf);
		RCPtr<SineWaveSource<1>>	sws2 = new SineWaveSource<1>(freq2, 1.0f, qf);
		RCPtr<JoinStereoAdapter>	jsa	 = new JoinStereoAdapter(sws1, sws2);
		RCPtr<SampleRateAdapter<2>> sra	 = new SampleRateAdapter<2>(jsa, qf, zf);

		const uint	 busize = 64 * 5 + 3;
		StereoSample bu[busize];
		for (uint n, i = 0; i < busize; i += n)
		{
			n = sra->getAudio(bu + i, max((busize - i) / 2, 1u));
			CHECK_NE(n, 0);
		}

		double p1 = 0;
		double d1 = 2 * 3.1415926535898 * double(freq1) / double(zf);
		double p2 = 0;
		double d2 = 2 * 3.1415926535898 * double(freq2) / double(zf);

		for (uint i = 0; i < NELEM(bu); i++)
		{
			int ref1 = int(sin(p1) * 0x8000);
			int ref2 = int(sin(p2) * 0x8000);

			bool pos1 = ref1 >= 0;
			bool neg1 = ref1 <= 0;
			bool pos2 = ref2 >= 0;
			bool neg2 = ref2 <= 0;
			// normally, due to interpolation, the error is towards 0:
			CHECK_GE(bu[i].left(), ref1 - 60 * pos1 - 13 * neg1);
			CHECK_LE(bu[i].left(), ref1 + 59 * neg1 + 8 * pos1);
			CHECK_GE(bu[i].right(), ref2 - 57 * pos2 - 2 * neg2);
			CHECK_LE(bu[i].right(), ref2 + 57 * neg2 + 8 * pos2);

			p1 += d1;
			p2 += d2;
		}
	}
}

TEST_CASE("Audio::HF_DC_Filter")
{
	// minimum test: instantiate
	HF_DC_Filter<1> f1(new SineWaveSource<1>(100,1));
	HF_DC_Filter<2> f2(new SineWaveSource<2>(100,1));
}

} // namespace kio::Test


/*







































*/
