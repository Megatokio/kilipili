// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Audio/AudioSample.h"
#include "doctest.h"


namespace kio::Test
{

using namespace kio;
using namespace kio::Audio;


TEST_CASE("Audio::AudioSample ctor, conversion")
{
	Sample s = 123;
	(void)s;
	MonoSample						   s1;
	MonoSample						   s2;
	AudioSample<1, Sample>			   s3;
	AudioSample<1, int>				   s4;
	AudioSample<1, uint8>			   s5;
	StereoSample					   s6;
	StereoSample					   s7;
	AudioSample<2, Sample>			   s9;
	AudioSample<2, int>				   s8;
	AudioSample<2, uint8>			   s10;
	NoAudioSample					   s0;
	AudioSample<2, kio::Audio::Sample> bu[33];
	MonoSample						   m;
	MonoSample						   m2;
	AudioSample<1, kio::Audio::Sample> m3(3);
	AudioSample<1, int>				   m4;
	MonoSample						   m5(Sample(3));
	AudioSample<1, Sample>			   m6(77);

	MonoSample m7(77);
	static_assert(sizeof(m7) == sizeof(Sample));
	MonoSample m9 = 88;
	static_assert(sizeof(m9) == sizeof(Sample));

	s1 = MonoSample();
	s1 = MonoSample(s);
	s1 = MonoSample(s, s);
	s1 = StereoSample();
	s1 = StereoSample(s);
	s1 = StereoSample(s, s);
	s1 = NoAudioSample().mono();
	s1 = NoAudioSample(s).mono();
	s1 = NoAudioSample(s, s).left();
	s0 = s;
	s0 = s1;
	s0 = s6;
	s1 = s0;
	s1 = s6;
	s6 = s0;
	s6 = s1;
	s  = m5.mono();
	s  = s1.m;
	s  = s6.mono();
	s  = s0.left();
	s  = m3.mono();
	s  = m6.right();
	s  = m7.m;

	CHECK_EQ(1, 1); // count the test
}

TEST_CASE("Audio::NoAudioSample")
{
	CHECK_EQ(NoAudioSample().mono(), 0);
	CHECK_EQ(NoAudioSample(1).mono(), 0);
	CHECK_EQ(NoAudioSample(2, 3).mono(), 0);
	CHECK_EQ(NoAudioSample(MonoSample(33)).mono(), 0);
	CHECK_EQ(NoAudioSample(StereoSample(33, 44)).mono(), 0);

	CHECK_EQ(NoAudioSample(5).mono(), 0);
	CHECK_EQ(NoAudioSample(5).left(), 0);
	CHECK_EQ(NoAudioSample(5).right(), 0);

	CHECK_EQ(MonoSample(NoAudioSample()).m, 0);
	CHECK_EQ(StereoSample(NoAudioSample()).l, 0);
	CHECK_EQ(StereoSample(NoAudioSample()).r, 0);

	CHECK_EQ(AudioSample<0, int>(55).left(), 0);
	CHECK_EQ(AudioSample<0, uint8>(55).left(), 0);
}

TEST_CASE("Audio::MonoSample")
{
	//CHECK_EQ(MonoSample().mono(), 0);		uninitialized
	CHECK_EQ(MonoSample(1).mono(), 1);
	CHECK_EQ(MonoSample(2, 4).mono(), 3);
	CHECK_EQ(MonoSample(-4, 4).mono(), 0);
	CHECK_EQ(MonoSample(MonoSample(33)).mono(), 33);
	CHECK_EQ(MonoSample(StereoSample(33, 55)).mono(), 44);
	CHECK_EQ(MonoSample(-4, 4).left(), 0);
	CHECK_EQ(MonoSample(-4, 4).right(), 0);
	CHECK_EQ(MonoSample(66).channels[0], 66);
	CHECK_EQ(MonoSample(77)[0], 77);
	CHECK_EQ(MonoSample(88).m, 88);

	CHECK_EQ(StereoSample(MonoSample(44)).left(), 44);
	CHECK_EQ(MonoSample(StereoSample(44)).right(), 44);
}

TEST_CASE("Audio::StereoSample")
{
	//CHECK_EQ(StereoSample().mono(), 0);		uninitialized
	CHECK_EQ(StereoSample(1).mono(), 1);
	CHECK_EQ(StereoSample(2, 4).mono(), 3);
	CHECK_EQ(StereoSample(-4, 4).mono(), 0);
	CHECK_EQ(StereoSample(MonoSample(33)).mono(), 33);
	CHECK_EQ(StereoSample(StereoSample(33, 55)).mono(), 44);
	CHECK_EQ(StereoSample(-4, 4).left(), -4);
	CHECK_EQ(StereoSample(-4, 4).right(), 4);
	CHECK_EQ(StereoSample(66, 99).channels[0], 66);
	CHECK_EQ(StereoSample(77, 99)[0], 77);
	CHECK_EQ(StereoSample(88, 99).l, 88);
	CHECK_EQ(StereoSample(99, 66).channels[1], 66);
	CHECK_EQ(StereoSample(99, 77)[1], 77);
	CHECK_EQ(StereoSample(99, 88).r, 88);

	CHECK_EQ(StereoSample(MonoSample(44)).left(), 44);
	CHECK_EQ(MonoSample(StereoSample(33, 55)).right(), 44);

	CHECK_EQ(StereoSample(StereoSample(33, 55)).left(), 33);
	CHECK_EQ(StereoSample(StereoSample(33, 55)).right(), 55);
}

TEST_CASE("Audio::MonoSample<int> ctor, mono/stereo conversion")
{
	using MonoSample   = AudioSample<1, int>;
	using StereoSample = AudioSample<2, int>;

	//CHECK_EQ(MonoSample().mono(), 0);		uninitialized
	CHECK_EQ(MonoSample(1).mono(), 1);
	CHECK_EQ(MonoSample(2, 4).mono(), 3);
	CHECK_EQ(MonoSample(-4, 4).mono(), 0);
	CHECK_EQ(MonoSample(MonoSample(33)).mono(), 33);
	CHECK_EQ(MonoSample(StereoSample(33, 55)).mono(), 44);
	CHECK_EQ(MonoSample(-4, 4).left(), 0);
	CHECK_EQ(MonoSample(-4, 4).right(), 0);
	CHECK_EQ(MonoSample(66).channels[0], 66);
	CHECK_EQ(MonoSample(77)[0], 77);
	CHECK_EQ(MonoSample(88).m, 88);

	CHECK_EQ(StereoSample(MonoSample(44)).left(), 44);
	CHECK_EQ(MonoSample(StereoSample(44)).right(), 44);
}

TEST_CASE("Audio::StereoSample<int> ctor, mono/stereo conversion")
{
	using MonoSample   = AudioSample<1, int>;
	using StereoSample = AudioSample<2, int>;

	//CHECK_EQ(StereoSample().mono(), 0);		uninitialized
	CHECK_EQ(StereoSample(1).mono(), 1);
	CHECK_EQ(StereoSample(2, 4).mono(), 3);
	CHECK_EQ(StereoSample(-4, 4).mono(), 0);
	CHECK_EQ(StereoSample(MonoSample(33)).mono(), 33);
	CHECK_EQ(StereoSample(StereoSample(33, 55)).mono(), 44);
	CHECK_EQ(StereoSample(-4, 4).left(), -4);
	CHECK_EQ(StereoSample(-4, 4).right(), 4);
	CHECK_EQ(StereoSample(66, 99).channels[0], 66);
	CHECK_EQ(StereoSample(77, 99)[0], 77);
	CHECK_EQ(StereoSample(88, 99).l, 88);
	CHECK_EQ(StereoSample(99, 66).channels[1], 66);
	CHECK_EQ(StereoSample(99, 77)[1], 77);
	CHECK_EQ(StereoSample(99, 88).r, 88);

	CHECK_EQ(StereoSample(MonoSample(44)).left(), 44);
	CHECK_EQ(MonoSample(StereoSample(33, 55)).right(), 44);

	CHECK_EQ(StereoSample(StereoSample(33, 55)).left(), 33);
	CHECK_EQ(StereoSample(StereoSample(33, 55)).right(), 55);
}

TEST_CASE("Audio::MonoSample<int> arithmetics")
{
	using MonoSample = AudioSample<1, int>;

	CHECK_EQ(MonoSample(300) + MonoSample(400), MonoSample(700));
	CHECK_EQ(MonoSample(30000) + MonoSample(40000), MonoSample(70000));
	CHECK_EQ(MonoSample(-30000) + MonoSample(40000), MonoSample(10000));

	MonoSample z = 500;
	z += 33;
	CHECK_EQ(z, 533);

	CHECK_EQ(MonoSample(300) * 3, MonoSample(900));
	CHECK_EQ(MonoSample(-30000) * 3, MonoSample(-90000));
	CHECK_EQ(MonoSample(30000) * -3, MonoSample(-90000));

	CHECK_EQ(MonoSample(300) >> 3, MonoSample(300 / 8));
	CHECK_EQ(MonoSample(-90000) >> 3, MonoSample(-90000 / 8));
	CHECK_EQ(MonoSample(-900) >> 3, MonoSample((-900 - 7) / 8));

	CHECK_EQ(Audio::MonoSample(MonoSample(777)).m, 777);
	CHECK_EQ(Audio::MonoSample(MonoSample(-77)).m, -77);

	CHECK_EQ(MonoSample(Audio::MonoSample(777)).m, 777);
	CHECK_EQ(MonoSample(Audio::MonoSample(-77)).m, -77);
}

TEST_CASE("Audio::StereoSample<int> arithmetics")
{
	using StereoSample = AudioSample<2, int>;

	CHECK_EQ(StereoSample(300, 400) + StereoSample(500, 600), StereoSample(800, 1000));
	CHECK_EQ(StereoSample(30000, -40000) + StereoSample(40000), StereoSample(70000, 0));
	CHECK_EQ(StereoSample(-30000, 40000) + StereoSample(40000), StereoSample(10000, 80000));

	StereoSample z(500, 600);
	z += StereoSample(5, 6);
	CHECK_EQ(z, StereoSample(505, 606));

	CHECK_EQ(StereoSample(300, 3) * 3, StereoSample(900, 9));
	CHECK_EQ(StereoSample(-30000, 3) * 3, StereoSample(-90000, 9));
	CHECK_EQ(StereoSample(30000, -3) * -3, StereoSample(-90000, 9));

	CHECK_EQ(StereoSample(300, 40) >> 3, StereoSample(300 / 8, 40 / 8));
	CHECK_EQ(StereoSample(-90000, 400) >> 3, StereoSample(-90000 / 8, 400 / 8));
	CHECK_EQ(StereoSample(-900, -4000) >> 3, StereoSample((-900 - 7) / 8, -4000 / 8));

	CHECK_EQ(Audio::StereoSample(StereoSample(777, 8)).l, 777);
	CHECK_EQ(Audio::StereoSample(StereoSample(8, -77)).r, -77);

	CHECK_EQ(StereoSample(Audio::StereoSample(777, 8)).l, 777);
	CHECK_EQ(StereoSample(Audio::StereoSample(8, -77)).r, -77);
}


} // namespace kio::Test


/*







































*/
