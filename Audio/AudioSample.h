// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"

namespace kio::Audio
{

using Sample = int16;

template<typename Z, typename Q>
inline constexpr Z convert_sample(const Q) noexcept;


// #################################################################
// template for audio samples (frames)
// with number of channels and sample type:

template<uint NUM_CHANNELS, typename SAMPLE = Sample>
struct AudioSample;

using NoAudioSample = AudioSample<0, Sample>;
using MonoSample	= AudioSample<1, Sample>;
using StereoSample	= AudioSample<2, Sample>;


// #################################################################
// audio sample with 0 channels:
// provided for templates etc. if audio_hw = NONE or BUZZER

template<typename Sample>
struct AudioSample<0, Sample>
{
	static Sample* channels; // dummy, for syntax only

	constexpr Sample left() const noexcept { return 0; }
	constexpr Sample right() const noexcept { return 0; }
	constexpr Sample mono() const noexcept { return 0; }

	AudioSample() {}
	constexpr AudioSample(Sample) noexcept {}
	constexpr AudioSample(Sample, Sample) noexcept {}
	constexpr AudioSample(const AudioSample<1, Sample>&) noexcept {}
	constexpr AudioSample(const AudioSample<2, Sample>&) noexcept {}

	constexpr operator AudioSample<1, Sample>() const noexcept { return 0; }
	constexpr operator AudioSample<2, Sample>() const noexcept { return 0; }
};


// #################################################################
// audio sample with 1 channel:

template<typename Sample>
struct AudioSample<1, Sample>
{
	union
	{
		Sample channels[1];
		Sample m;
	};

	constexpr Sample left() const noexcept { return m; }
	constexpr Sample right() const noexcept { return m; }
	constexpr Sample mono() const noexcept { return m; }

	AudioSample() {}
	constexpr AudioSample(Sample m) noexcept : m(m) {}
	constexpr AudioSample(Sample l, Sample r) noexcept : m(Sample((l + r) >> 1)) {}

	Sample&		operator[](uint i) noexcept { return channels[i]; }
	bool		operator==(const AudioSample& q) const noexcept { return m == q.m; }
	AudioSample operator*(float f) const noexcept { return AudioSample(Sample(m * f)); }
	AudioSample operator+(const AudioSample& q) const noexcept { return AudioSample(m + q.m); }
};


// #################################################################
// audio sample with 2 channels:

template<typename Sample>
struct AudioSample<2, Sample>
{
	union
	{
		Sample channels[2];
		struct
		{
			Sample l, r;
		};
	};

	constexpr Sample left() const noexcept { return l; }
	constexpr Sample right() const noexcept { return r; }
	constexpr Sample mono() const noexcept { return (l + r) >> 1; }

	AudioSample() {}
	constexpr AudioSample(Sample l, Sample r) noexcept : l(l), r(r) {}
	constexpr AudioSample(Sample m) noexcept : l(m), r(m) {}
	constexpr AudioSample(const AudioSample<1, Sample>& q) noexcept : AudioSample(q.m) {}

	constexpr operator AudioSample<1, Sample>() const noexcept { return mono(); }

	Sample&		operator[](uint i) noexcept { return channels[i]; }
	bool		operator==(const AudioSample& q) const noexcept { return l == q.l && r == q.r; }
	AudioSample operator*(float f) const noexcept { return AudioSample(Sample(l * f), Sample(r * f)); }
	AudioSample operator+(const AudioSample& q) const noexcept { return AudioSample(l + q.l, r + q.r); }
};


// #################################################################
// int32 audio sample with 1 channel:
// this is used in the SampleRateAdapter.

template<>
struct AudioSample<1, int>
{
	union
	{
		int channels[1];
		int m;
	};

	constexpr int left() const noexcept { return m; }
	constexpr int right() const noexcept { return m; }
	constexpr int mono() const noexcept { return m; }

	AudioSample() {}
	constexpr AudioSample(int m) noexcept : m(m) {}
	constexpr AudioSample(int l, int r) noexcept : m(Sample((l + r) >> 1)) {}
	constexpr AudioSample(const MonoSample& q) noexcept : m(q.m) {}

	constexpr operator MonoSample() const noexcept { return MonoSample(Sample(m)); }

	int& operator[](uint i) noexcept { return channels[i]; }

	// for int32 samples we do maths without overflow checking:
	void		operator+=(const AudioSample& q) noexcept { m += q.m; }
	AudioSample operator*(int a) const noexcept { return AudioSample(m * a); }
	AudioSample operator+(const AudioSample& q) const noexcept { return AudioSample(m + q.m); }
	AudioSample operator>>(int n) const noexcept { return AudioSample(m >> n); }
	bool		operator==(const AudioSample& q) const noexcept { return m == q.m; }
};


// #################################################################
// int32 audio sample with 2 channels:
// this is used in the SampleRateAdapter.

template<>
struct AudioSample<2, int>
{
	union
	{
		int channels[2];
		struct
		{
			int l, r;
		};
	};

	constexpr int left() const noexcept { return l; }
	constexpr int right() const noexcept { return r; }
	constexpr int mono() const noexcept { return (l + r) >> 1; }

	AudioSample() {}
	constexpr AudioSample(int l, int r) noexcept : l(l), r(r) {}
	constexpr AudioSample(int m) noexcept : l(m), r(m) {}
	constexpr AudioSample(const AudioSample<1, int>& m) noexcept : AudioSample(m.mono()) {}
	constexpr AudioSample(const StereoSample& q) noexcept : l(q.l), r(q.r) {}

	constexpr operator AudioSample<1, int>() const noexcept { return mono(); }
	constexpr operator StereoSample() const noexcept { return StereoSample(Sample(l), Sample(r)); }

	int& operator[](uint i) noexcept { return channels[i]; }

	// for int32 samples we do maths without overflow checking:
	void		operator+=(const AudioSample& q) noexcept { l += q.l, r += q.r; }
	AudioSample operator*(int a) const noexcept { return AudioSample(l * a, r * a); }
	AudioSample operator+(const AudioSample& q) const noexcept { return AudioSample(l + q.l, r + q.r); }
	AudioSample operator>>(int n) const noexcept { return AudioSample(l >> n, r >> n); }
	bool		operator==(const AudioSample& q) const noexcept { return l == q.l && r == q.r; }
};


// #################################################################
// Helper:
// convert one Sample

// clang-format off
template<> inline constexpr int8   convert_sample(const int8 q) noexcept { return q; };
template<> inline constexpr uint8  convert_sample(const int8 q) noexcept { return uint8(q ^ 0x80); };
template<> inline constexpr int16  convert_sample(const int8 q) noexcept { return int16(q << 8); };
template<> inline constexpr uint16 convert_sample(const int8 q) noexcept { return uint16((q ^ 0x80) << 8); };
template<> inline constexpr int8   convert_sample(const uint8 q) noexcept { return int8(q ^ 0x80); };
template<> inline constexpr uint8  convert_sample(const uint8 q) noexcept { return q; };
template<> inline constexpr int16  convert_sample(const uint8 q) noexcept { return uint8((q ^ 0x80) << 8); };
template<> inline constexpr uint16 convert_sample(const uint8 q) noexcept { return uint16(q << 8); };
template<> inline constexpr int8   convert_sample(const int16 q) noexcept { return int8(q >> 8); };
template<> inline constexpr uint8  convert_sample(const int16 q) noexcept { return uint8((q >> 8) ^ 0x80); };
template<> inline constexpr uint16 convert_sample(const int16 q) noexcept { return uint16(q ^ 0x8000); };
template<> inline constexpr int16  convert_sample(const int16 q) noexcept { return q; };
template<> inline constexpr int8   convert_sample(const uint16 q) noexcept { return int8((q >> 8) ^ 0x80); };
template<> inline constexpr uint8  convert_sample(const uint16 q) noexcept { return uint8(q >> 8); };
template<> inline constexpr int16  convert_sample(const uint16 q) noexcept { return int16(q ^ 0x8000); };
template<> inline constexpr uint16 convert_sample(const uint16 q) noexcept { return q; };
// clang-format on


} // namespace kio::Audio


/*




























*/
