// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#include "Ay38912.h"
#include "cdefs.h"
#include "doctest.h"
#include <cmath>
#include <cstdio>

namespace kio::Audio
{
extern float hw_sample_frequency; // normally in AudioController.cpp
}


namespace kio::Test
{

using namespace kio;
using namespace kio::Audio;


TEST_CASE("Audio::Ay38912 ctor")
{
	Ay38912<1> ay1(1000000, Ay38912<1>::abc_stereo, 1.0f);
	Ay38912<2> ay2(1000000, Ay38912<2>::acb_stereo);
	CHECK(1); // count test

	MonoSample bu[102];
	bu[0] = bu[101] = 12345; //guards
	ay1.audioBufferStart(bu + 1, 100);
	ay1.audioBufferEnd();
	CHECK_EQ(bu[0].m, 12345); //check guards
	CHECK_EQ(bu[101].m, 12345);

	// check we got silence:
	for (uint i = 1; i < 101; i++) CHECK_EQ(bu[1].m, bu[i].m);
}

TEST_CASE("Audio::Ay38912 nextHigherClock")
{
	for (float f = 999000; f < 1005000; f += 47)
	{
		//printf("clock %i -> %i\n", int(f), int(Ay38912<1>::nextHigherClock(f)));
		CHECK_GE(Ay38912<1>::nextHigherClock(f), f);
		CHECK_LE(Ay38912<1>::nextHigherClock(f), f * 1.001f);
		CHECK_EQ(Ay38912<1>::nextHigherClock(f), Ay38912<1>::nextHigherClock(Ay38912<1>::nextHigherClock(f)));
	}
}

TEST_CASE("Audio::Ay38912 setVolume") // * doctest::skip())
{
	// due to integer-based resampling the volume granularity may be reduced.
	// check that it is at least 100 discrete values.

	hw_sample_frequency = 40000;
	// 2 MHz @ 20 kHz = 100:1
	// 1 MHz @ 60 kHz = 16*1

	for (int16 f = 16; f <= 64; f *= 4)
	{
		float ay_f = hw_sample_frequency * f;

		Ay38912<1> ay(ay_f);
		MonoSample bu[203];
		bu[0]	= 47; //guards
		bu[202] = 111;
		ay.audioBufferStart(bu + 1, 201);
		for (int16 i = -100; i <= 100; i++) // 201 values from -1.0 to 1.0
		{
			ay.setVolume(0.01f * i);
			ay.setRegister(Ay38912<1>::CC(f * (i + 101)), 7, 0xff);
		}
		ay.audioBufferEnd();
		CHECK_EQ(bu[0].m, 47); //check guards
		CHECK_EQ(bu[202].m, 111);

		//printf("-----------------\n");
		for (int i = 1 + 0; i < 1 + 200; i++)
		{
			//printf("sample = %i\n", bu[i].m);
			CHECK_GT(bu[i].m, bu[i + 1].m); //
		}
		//printf("sample = %i\n", bu[201].m);

		CHECK_EQ(bu[1], 32767);
		CHECK_EQ(bu[101], 0);
		CHECK_EQ(bu[201], -32768);
	}

	for (int16 f = 16; f <= 64; f *= 4)
	{
		float ay_f = hw_sample_frequency * f;

		Ay38912<1> ay(ay_f);
		ay.setRegister(8, 15);
		ay.setRegister(9, 15);
		ay.setRegister(10, 15);
		MonoSample bu[203];
		bu[0]	= 47; //guards
		bu[202] = 111;
		ay.audioBufferStart(bu + 1, 201);
		for (int16 i = -100; i <= 100; i++) // 201 values from -1.0 to 1.0
		{
			ay.setVolume(0.01f * i);
			ay.setRegister(Ay38912<1>::CC(f * (i + 101)), 7, 0xff);
		}
		ay.audioBufferEnd();
		CHECK_EQ(bu[0].m, 47); //check guards
		CHECK_EQ(bu[202].m, 111);

		//printf("-----------------\n");
		for (int i = 1 + 0; i < 1 + 200; i++)
		{
			//printf("sample = %i\n", bu[i].m);
			CHECK_LT(bu[i].m, bu[i + 1].m); //
		}
		//printf("sample = %i\n", bu[201].m);

		CHECK_EQ(bu[201], 32767);
		CHECK_EQ(bu[101], 0);
		CHECK_EQ(bu[1], -32768);
	}
}

static int ccxPerSample(float ay_f, float sample_f)
{
	// assuming Ay38912 uses fixed float 24.8 internally

	return int(ay_f / sample_f * 256 + 0.5f);
}

static float calcActualClock(float f, float sf)
{
	// assuming Ay38912 uses fixed float 24.8 internally

	float ccx_per_sample = floorf(f / sf * 256 + 0.5f);
	return sf * ccx_per_sample / 256;
}

TEST_CASE("Audio::Ay38912 setClock, getClock, getActualClock")
{
	hw_sample_frequency = 40000;
	{
		Ay38912<1> ay(1000000);
		CHECK_EQ(ay.getClock(), 1000000);
		CHECK_EQ(ay.getActualClock(), 1000000);

		ay.setClock(1000010);
		CHECK_EQ(ay.getClock(), 1000010);
		CHECK_EQ(ay.getActualClock(), 1000000);

		ay.setClock(999990);
		CHECK_EQ(ay.getClock(), 999990);
		CHECK_EQ(ay.getActualClock(), 1000000);
	}
	hw_sample_frequency = 40010;
	{
		Ay38912<1> ay(1000000);
		CHECK_EQ(ay.getClock(), 1000000);
		CHECK_EQ(ay.getActualClock(), ay.nextHigherClock(999900));
		CHECK_EQ(ay.getActualClock(), calcActualClock(1000000, 40010)); // comparing floats, but same formula

		ay.setClock(1000066);
		CHECK_EQ(ay.getClock(), 1000066);
		CHECK_EQ(ay.getActualClock(), calcActualClock(1000066, 40010));

		ay.setClock(999966);
		CHECK_EQ(ay.getClock(), 999966);
		CHECK_EQ(ay.getActualClock(), calcActualClock(999966, 40010));
	}
	hw_sample_frequency = 39990;
	{
		Ay38912<1> ay(1000000);
		CHECK_EQ(ay.getClock(), 1000000);
		float ac = ay.nextHigherClock(1000000);
		CHECK_EQ(ay.getActualClock(), ac);
		CHECK_EQ(calcActualClock(1000000, 39990), ac);
	}
}

TEST_CASE("Audio::Ay38912 setSampleRate")
{
	hw_sample_frequency = 40000;
	Ay38912<1> ay(1000000);
	CHECK_EQ(ay.getClock(), 1000000);
	CHECK_EQ(ay.getActualClock(), 1000000);

	ay.setSampleRate(39000);
	CHECK_EQ(ay.getClock(), 1000000);
	CHECK_EQ(ay.getActualClock(), calcActualClock(1000000, 39000));

	ay.setSampleRate(44100);
	CHECK_EQ(ay.getClock(), 1000000);
	CHECK_EQ(ay.getActualClock(), calcActualClock(1000000, 44100));
}

TEST_CASE("Audio::Ay38912 setRegister, getRegister")
{
	Ay38912<1> ay(1000000);
	for (uint8 r = 0; r <= 15; r++) { CHECK_EQ(ay.getRegister(r), ayRegisterResetValues[r]); }
	for (uint8 r = 0; r <= 15; r++) { ay.setRegister(r, r + 1); }
	for (uint8 r = 0; r <= 15; r++) { CHECK_EQ(ay.getRegister(r), r + 1); }
	for (uint8 r = 0; r <= 15; r++) { ay.setRegister(r, 0xff); }
	for (uint8 r = 0; r <= 15; r++) { CHECK_EQ(ay.getRegister(r), ayRegisterBitMasks[r]); }
}

TEST_CASE("Audio::Ay38912 setRegNr, getRegNr")
{
	using CC = Ay38912<1>::CC;
	Ay38912<1> ay(1000000);
	ay.setRegNr(3);
	CHECK_EQ(ay.getRegNr(), 3);
	ay.setRegNr(15);
	CHECK_EQ(ay.getRegNr(), 15);
	ay.setRegNr(16);
	CHECK_EQ(ay.getRegNr(), 0);

	MonoSample bu[100] = {77};
	ay.audioBufferStart(bu, 100);

	for (uint8 r = 0; r <= 15; r++)
	{
		ay.setRegNr(r);
		ay.writeRegister(CC(0), r + 1);
	}
	for (uint8 r = 0; r <= 15; r++)
	{
		ay.setRegNr(r);
		CHECK_EQ(ay.readRegister(CC(0)), r + 1);
	}
	for (uint8 r = 0; r <= 15; r++)
	{
		ay.setRegNr(r);
		ay.writeRegister(CC(0), 0xf0 + r);
	}
	for (uint8 r = 0; r <= 15; r++)
	{
		ay.setRegNr(r);
		CHECK_EQ(ay.readRegister(CC(0)), (0xf0 + r) & ayRegisterBitMasks[r]);
	}

	CHECK_EQ(bu[0], 77); // never written anything to buffer
}

TEST_CASE("Audio::Ay38912 reset")
{
	using CC = Ay38912<1>::CC;
	{
		Ay38912<1> ay(1000000);
		for (uint8 r = 0; r <= 15; r++) { ay.setRegister(r, 0); }
		ay.setRegNr(7);
		CHECK_EQ(ay.getRegNr(), 7);
		ay.reset();
		CHECK_EQ(ay.getRegNr(), 0);
		for (uint8 r = 0; r <= 15; r++) { CHECK_EQ(ay.getRegister(r), ayRegisterResetValues[r]); }
	}
	{
		hw_sample_frequency = 31245;
		Ay38912<1> ay(1000000, Ay38912<1>::mono, 0.5f);

		// 10 samples with volABC=0
		// 10 samples with volABC=15
		// 10 samples with volABC=0

		MonoSample bu[30];
		ay.audioBufferStart(bu, 30);
		int ccx_per_sample = ccxPerSample(1000000, hw_sample_frequency);
		CC	a			   = CC(0);
		CC	b			   = a + ccx_per_sample * 10 / 256;
		CC	c			   = a + ccx_per_sample * 20 / 256;
		ay.setRegister(b, 8, 0x0f);
		ay.setRegister(b, 9, 0x0f);
		ay.setRegister(b, 10, 0x0f);
		ay.reset(c);
		ay.audioBufferEnd();

		for (int i = 0; i < 30; i++)
		{
			//printf("sample = %i\n", bu[i].m);
			if (i == 9 || i == 19) continue;
			CHECK_EQ(bu[i].m, i < 10 ? -16384 : i < 20 ? +16383 : -16384);
		}
	}
	{
		hw_sample_frequency = 47112;
		Ay38912<1> ay(1000000, Ay38912<1>::mono, 0.5f);

		// 10 samples with volABC=0
		// 10 samples with volABC=15
		// 10 samples with volABC=0

		uint8 port[2] = {77, 77};
		ay.setRegister(7, 0xFF); // port A & B output
		ay.setRegister(14, 55);
		ay.setRegister(15, 66);

		MonoSample bu[30];
		ay.audioBufferStart(bu, 30);
		int ccx_per_sample = ccxPerSample(1000000, hw_sample_frequency);
		CC	a			   = CC(0);
		CC	b			   = a + ccx_per_sample * 10 / 256;
		CC	c			   = a + ccx_per_sample * 20 / 256;
		ay.setRegister(b, 8, 0x0f);
		ay.setRegister(b, 9, 0x0f);
		ay.setRegister(b, 10, 0x0f);
		ay.reset(c, [&](CC, bool p, uint8 n) { port[p] = n; });
		ay.audioBufferEnd();

		CHECK_EQ(port[0], 0xff);
		CHECK_EQ(port[1], 0xff);

		for (int i = 0; i < 30; i++)
		{
			//printf("sample = %i\n", bu[i].m);
			if (i == 9 || i == 19) continue;
			CHECK_EQ(bu[i].m, i < 10 ? -16384 : i < 20 ? +16383 : -16384);
		}
	}
}

TEST_CASE("Audio::Ay38912 shiftTimebase, resetTimebase")
{
	using CC			= Ay38912<1>::CC;
	hw_sample_frequency = 25000;
	Ay38912<1> ay(1000000, Ay38912<1>::mono, 1.0f);
	MonoSample bu[20];
	int		   cc_per_sample = 1000000 / 25000;
	int		   cc_per_buffer = cc_per_sample * 20;
	CC		   cc0			 = CC(0);

	Sample lo = -0x8000;
	Sample hi = lo + 0xffff / 3;

	for (int i = 0; i < 20; i++)
	{
		ay.audioBufferStart(bu, 20);
		if (i < 10) ay.setRegister(cc0 + cc_per_sample * i, 8, 0x0f);
		ay.shiftTimebase(cc_per_buffer - 17);
		cc0 -= cc_per_buffer - 17;
		if (i >= 10) ay.setRegister(cc0 + cc_per_sample * i, 8, 0x0f);
		ay.setRegister(cc0 + cc_per_buffer, 8, 0);
		ay.audioBufferEnd();
		cc0 += cc_per_buffer;

		//printf("-----------------\n");
		for (int j = 0; j < 20; j++)
		{
			//printf("sample = %i\n", bu[j].m);
			CHECK_EQ(bu[j].m, j < i ? lo : hi);
		}
	}

	for (int i = 0; i < 20; i++)
	{
		ay.audioBufferStart(bu, 20);
		if (i < 10)
		{
			cc0 += cc_per_sample * i;
			ay.setRegister(cc0, 8, 0x0f);
		}

		ay.resetTimebase();
		cc0 = CC(0);

		if (i >= 10)
		{
			cc0 += cc_per_sample * i;
			ay.setRegister(cc0, 8, 0x0f);
		}

		ay.setRegister(cc0 + cc_per_buffer, 8, 0);
		ay.audioBufferEnd();
		cc0 += cc_per_sample * (20 - i);

		//printf("-----------------\n");
		for (int j = 0; j < 20; j++)
		{
			//printf("sample = %i\n", bu[j].m);
			CHECK_EQ(bu[j].m, j < i ? lo : hi);
		}
	}

	hw_sample_frequency = 22053;
	ay.resetTimebase();
	ay.setSampleRate(hw_sample_frequency);
	ay.reset();
	Ay38912<1> ay2(1000000, Ay38912<1>::mono, 1.0f);
	MonoSample bu2[20];
	uint8	   r[] = {233, 0, 217, 0, 254, 0, 21, 0, 15, 15, 15};
	for (uint i = 0; i < NELEM(r); i++) ay.setRegister(i, r[i]);
	for (uint i = 0; i < NELEM(r); i++) ay2.setRegister(i, r[i]);

	for (int i = 0; i < 100; i++)
	{
		ay.shiftTimebase(6777);
		CC end = ay.audioBufferStart(bu, 20);
		ay.setRegister(end - 999, 14, 0xff);
		ay.audioBufferEnd();

		end = ay2.audioBufferStart(bu2, 20);
		ay.setRegister(end - 888, 14, 0xff);
		//ay2.shiftTimebase(5999);
		ay2.resetTimebase();
		ay2.audioBufferEnd();

		//printf("-----------------\n");
		for (int i = 0; i < 20; i++)
		{
			//printf("sample = %6i - %6i\n", bu[i].m, bu2[i].m);
			CHECK_EQ(bu[i], bu2[i]); //
		}
	}
}

TEST_CASE("Audio::Ay38912 setRegister(CC)")
{
	//
}

TEST_CASE("Audio::Ay38912 writeRegister(CC)")
{
	//
}

TEST_CASE("Audio::Ay38912 readRegister(CC)")
{
	//
}

TEST_CASE("Audio::Ay38912 audioBufferStart, audioBufferEnd")
{
	//
}

TEST_CASE("Audio::Ay38912 channel A, B, C")
{
	//
}

TEST_CASE("Audio::Ay38912 noise")
{
	//
}

TEST_CASE("Audio::Ay38912 envelope")
{
	//
}

} // namespace kio::Test

/*











































*/
