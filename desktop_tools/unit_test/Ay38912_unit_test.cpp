// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#include "Audio/Ay38912.h"
#include "Xoshiro256.h"
//#include "YMFileConverter.h"
#include "cdefs.h"
//#include "cstrings.h"
#include "doctest.h"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <zlib.h>

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
			CHECK_EQ(bu[i], bu2[i]);
		}
	}
}

TEST_CASE("Audio::Ay38912 setRegister(CC)")
{
	using CC		 = Ay38912<1>::CC;
	uint8 rv[]		 = {0, 1, 2, 3, 4, 5, 6, 0xff, 8, 9, 10, 11, 12, 13, 14, 15};
	uint8 ports[2]	 = {0, 0};
	CC	  portccs[2] = {CC(0), CC(0)};

	hw_sample_frequency = 40000;
	Ay38912<1> ay(1000000);
	MonoSample bu[100];
	ay.audioBufferStart(bu, 100);
	for (uint8 r = 0; r <= 15; r++)
	{
		ay.setRegister(CC(r * 10), r, rv[r], [&](CC cc, bool i, uint8 v) { ports[i] = v, portccs[i] = cc; });
	}
	ay.audioBufferEnd();
	CHECK_EQ(ports[0], 14);
	CHECK_EQ(ports[1], 15);
	CHECK_EQ(portccs[0].value, 140);
	CHECK_EQ(portccs[1].value, 150);
}

TEST_CASE("Audio::Ay38912 writeRegister(CC)")
{
	using CC		 = Ay38912<1>::CC;
	uint8 rv[]		 = {10, 11, 12, 13, 14, 15, 16, 0xff, 18, 19, 110, 111, 112, 113, 114, 115};
	uint8 ports[2]	 = {0, 0};
	CC	  portccs[2] = {CC(0), CC(0)};

	hw_sample_frequency = 40000;
	Ay38912<1> ay(1000000);
	MonoSample bu[100];
	ay.audioBufferStart(bu, 100);
	for (uint8 r = 0; r <= 15; r++)
	{
		ay.setRegNr(r);
		ay.writeRegister(CC(r * 10), rv[r], [&](CC cc, bool i, uint8 v) { ports[i] = v, portccs[i] = cc; });
	}
	ay.audioBufferEnd();
	CHECK_EQ(ports[0], 114);
	CHECK_EQ(ports[1], 115);
	CHECK_EQ(portccs[0].value, 140);
	CHECK_EQ(portccs[1].value, 150);

	rv[7] = 0x3f; // set ports to input

	ay.audioBufferStart(bu, 100);
	for (uint8 r = 0; r <= 15; r++)
	{
		ay.setRegNr(r);
		ay.writeRegister(CC(r * 9), rv[r], [&](CC cc, bool i, uint8 v) { ports[i] = v, portccs[i] = cc; });
	}
	ay.audioBufferEnd();
	CHECK_EQ(ports[0], 0xff); // callback called when ports set to input
	CHECK_EQ(ports[1], 0xff);
	CHECK_EQ(portccs[0].value, 7 * 9); // when reg 7 is changed
	CHECK_EQ(portccs[1].value, 7 * 9);
}

TEST_CASE("Audio::Ay38912 readRegister(CC)")
{
	using CC		 = Ay38912<1>::CC;
	uint8 rv[]		 = {20, 21, 22, 23, 24, 25, 26, 0xff, 28, 29, 210, 211, 212, 213, 214, 215};
	uint8 ports[2]	 = {0, 0};
	CC	  portccs[2] = {CC(0), CC(0)};

	hw_sample_frequency = 40000;
	Ay38912<1> ay(1000000);
	MonoSample bu[100];
	ay.audioBufferStart(bu, 100);

	for (uint8 r = 0; r <= 15; r++) { ay.setRegister(r, rv[r]); }

	for (uint8 r = 0; r <= 15; r++)
	{
		ay.setRegNr(r);
		uint8 n = ay.readRegister(CC(r * 10), [&](CC cc, bool i) {
			portccs[i] = cc;
			return 50 + i;
		});
		if (r >= 14)
		{
			ports[r - 14] = n; //
		}
	}
	ay.audioBufferEnd();
	CHECK_EQ(ports[0], 50 & 214); // ports were set to output
	CHECK_EQ(ports[1], 51 & 215);
	CHECK_EQ(portccs[0].value, 140);
	CHECK_EQ(portccs[1].value, 150);

	ay.setRegister(7, 0x3f); // set ports to input

	for (uint8 r = 0; r <= 15; r++)
	{
		ay.setRegNr(r);
		uint8 n = ay.readRegister(CC(r * 10), [&](CC cc, bool i) {
			portccs[i] = cc;
			return 50 + i;
		});
		if (r >= 14)
		{
			ports[r - 14] = n; //
		}
	}
	ay.audioBufferEnd();
	CHECK_EQ(ports[0], 50);
	CHECK_EQ(ports[1], 51);
	CHECK_EQ(portccs[0].value, 140);
	CHECK_EQ(portccs[1].value, 150);
}

TEST_CASE("Audio::Ay38912 audioBufferStart, audioBufferEnd")
{
	hw_sample_frequency = 50000;
	Ay38912<1> ay2(1000345);
	Ay38912<1> ay1(1000345);
	MonoSample bu1[89];
	MonoSample bu2[83];

	uint8 r0[] = {23, 0, 24, 0, 25, 0, 13, 0x00, 13, 14, 16, 37, 0, 0b1110};
	for (uint r = 0; r < NELEM(r0); r++) ay1.setRegister(r, r0[r]);
	for (uint r = 0; r < NELEM(r0); r++) ay2.setRegister(r, r0[r]);

	uint i1 = NELEM(bu1);
	uint i2 = NELEM(bu2);
	for (uint i = 0; i < 100; i++)
	{
		if (i1 == NELEM(bu1))
		{
			ay1.audioBufferStart(bu1, NELEM(bu1));
			ay1.audioBufferEnd();
			i1 = 0;
		}
		if (i2 == NELEM(bu2))
		{
			ay2.audioBufferStart(bu2, NELEM(bu2));
			ay2.audioBufferEnd();
			i2 = 0;
		}

		//for (uint i = i1, j = i2; i < NELEM(bu1) && j < NELEM(bu2);)
		//{
		//	cstr s = bu1[i] == bu2[j] ? "" : "different!";
		//	printf("sample %6i - %6i %s\n", bu1[i++].m, bu2[j++].m, s);
		//}
		while (i1 < NELEM(bu1) && i2 < NELEM(bu2)) CHECK_EQ(bu1[i1++], bu2[i2++]);
	}
}

TEST_CASE("Audio::Ay38912 channel A, B, C")
{
	hw_sample_frequency = 50000;
	{
		using CC = Ay38912<1>::CC;
		Ay38912<1> ay(1000000, Ay38912<1>::mono, 1.0f);
		MonoSample bu[100];
		int		   cc_per_sample = 1000000 / 50000;
		CC		   cc0(0);

		// at start channels are off => follow volume & volume=0
		ay.audioBufferStart(bu, 100);
		ay.setRegister(cc0 + 1 * cc_per_sample, 8, 15);	 // A full on
		ay.setRegister(cc0 + 2 * cc_per_sample, 8, 0);	 // A off
		ay.setRegister(cc0 + 3 * cc_per_sample, 9, 15);	 // B full on
		ay.setRegister(cc0 + 4 * cc_per_sample, 9, 0);	 // B off
		ay.setRegister(cc0 + 5 * cc_per_sample, 10, 15); // C full on
		ay.setRegister(cc0 + 6 * cc_per_sample, 10, 0);	 // C off
		ay.setRegister(cc0 + 7 * cc_per_sample, 8, 15);	 // A full on
		ay.setRegister(cc0 + 8 * cc_per_sample, 9, 15);	 // AB full on
		ay.setRegister(cc0 + 9 * cc_per_sample, 10, 15); // ABC full on
		ay.audioBufferEnd();

		int s0 = -32768;
		int s1 = s0 + 0xffff * 1 / 3;
		int s2 = s0 + 0xffff * 2 / 3;
		int s3 = s0 + 0xffff * 3 / 3;
		CHECK_EQ(bu[0].m, s0); // all off
		CHECK_EQ(bu[1].m, s1); // A full on
		CHECK_EQ(bu[2].m, s0); // A off
		CHECK_EQ(bu[3].m, s1); // B full on
		CHECK_EQ(bu[4].m, s0); // B off
		CHECK_EQ(bu[5].m, s1); // C full on
		CHECK_EQ(bu[6].m, s0); // C off
		CHECK_EQ(bu[7].m, s1); // A full on
		CHECK_EQ(bu[8].m, s2); // AB full on
		CHECK_EQ(bu[9].m, s3); // ABC full on
	}

	{
		using CC = Ay38912<2>::CC;
		Ay38912<2>	 ay(1000000, Ay38912<2>::mono, 1.0f);
		StereoSample bu[100];
		int			 cc_per_sample = 1000000 / 50000;
		CC			 cc0(0);

		// at start channels are off => follow volume & volume=0
		ay.audioBufferStart(bu, 100);
		ay.setRegister(cc0 + 1 * cc_per_sample, 8, 15);	 // A full on
		ay.setRegister(cc0 + 2 * cc_per_sample, 8, 0);	 // A off
		ay.setRegister(cc0 + 3 * cc_per_sample, 9, 15);	 // B full on
		ay.setRegister(cc0 + 4 * cc_per_sample, 9, 0);	 // B off
		ay.setRegister(cc0 + 5 * cc_per_sample, 10, 15); // C full on
		ay.setRegister(cc0 + 6 * cc_per_sample, 10, 0);	 // C off
		ay.setRegister(cc0 + 7 * cc_per_sample, 8, 15);	 // A full on
		ay.setRegister(cc0 + 8 * cc_per_sample, 9, 15);	 // AB full on
		ay.setRegister(cc0 + 9 * cc_per_sample, 10, 15); // ABC full on
		ay.audioBufferEnd();

		int s0 = -32768;
		int s1 = s0 + 0xffff * 1 / 3;
		int s2 = s0 + 0xffff * 2 / 3;
		int s3 = s0 + 0xffff * 3 / 3;
		CHECK_EQ(bu[0].l, s0); // all off
		CHECK_EQ(bu[1].l, s1); // A full on
		CHECK_EQ(bu[2].l, s0); // A off
		CHECK_EQ(bu[3].l, s1); // B full on
		CHECK_EQ(bu[4].l, s0); // B off
		CHECK_EQ(bu[5].l, s1); // C full on
		CHECK_EQ(bu[6].l, s0); // C off
		CHECK_EQ(bu[7].l, s1); // A full on
		CHECK_EQ(bu[8].l, s2); // AB full on
		CHECK_EQ(bu[9].l, s3); // ABC full on
		CHECK_EQ(bu[0].r, s0); // all off
		CHECK_EQ(bu[1].r, s1); // A full on
		CHECK_EQ(bu[2].r, s0); // A off
		CHECK_EQ(bu[3].r, s1); // B full on
		CHECK_EQ(bu[4].r, s0); // B off
		CHECK_EQ(bu[5].r, s1); // C full on
		CHECK_EQ(bu[6].r, s0); // C off
		CHECK_EQ(bu[7].r, s1); // A full on
		CHECK_EQ(bu[8].r, s2); // AB full on
		CHECK_EQ(bu[9].r, s3); // ABC full on
	}

	{
		using CC = Ay38912<2>::CC;
		Ay38912<2>	 ay(1000000, Ay38912<2>::abc_stereo, 1.0f);
		StereoSample bu[100];
		int			 cc_per_sample = 1000000 / 50000;
		CC			 cc0(0);

		// at start channels are off => follow volume & volume=0
		ay.audioBufferStart(bu, 100);
		ay.setRegister(cc0 + 1 * cc_per_sample, 8, 15);	 // A full on
		ay.setRegister(cc0 + 2 * cc_per_sample, 8, 0);	 // A off
		ay.setRegister(cc0 + 3 * cc_per_sample, 9, 15);	 // B full on
		ay.setRegister(cc0 + 4 * cc_per_sample, 9, 0);	 // B off
		ay.setRegister(cc0 + 5 * cc_per_sample, 10, 15); // C full on
		ay.setRegister(cc0 + 6 * cc_per_sample, 10, 0);	 // C off
		ay.setRegister(cc0 + 7 * cc_per_sample, 8, 15);	 // A full on
		ay.setRegister(cc0 + 8 * cc_per_sample, 9, 15);	 // AB full on
		ay.setRegister(cc0 + 9 * cc_per_sample, 10, 15); // ABC full on
		ay.audioBufferEnd();

		int s0 = -32768;
		int s1 = s0 + 0xffff * 1 / 3;
		int s2 = s0 + 0xffff * 2 / 3;
		int s3 = s0 + 0xffff * 3 / 3;

		CHECK_EQ(bu[0].l, s0); // all off
		CHECK_EQ(bu[1].l, s2); // A full on
		CHECK_EQ(bu[2].l, s0); // A off
		CHECK_EQ(bu[3].l, s1); // B full on
		CHECK_EQ(bu[4].l, s0); // B off
		CHECK_EQ(bu[5].l, s0); // C full on
		CHECK_EQ(bu[6].l, s0); // C off
		CHECK_EQ(bu[7].l, s2); // A full on
		CHECK_EQ(bu[8].l, s3); // AB full on
		CHECK_EQ(bu[9].l, s3); // ABC full on

		CHECK_EQ(bu[0].r, s0); // all off
		CHECK_EQ(bu[1].r, s0); // A full on
		CHECK_EQ(bu[2].r, s0); // A off
		CHECK_EQ(bu[3].r, s1); // B full on
		CHECK_EQ(bu[4].r, s0); // B off
		CHECK_EQ(bu[5].r, s2); // C full on
		CHECK_EQ(bu[6].r, s0); // C off
		CHECK_EQ(bu[7].r, s0); // A full on
		CHECK_EQ(bu[8].r, s1); // AB full on
		CHECK_EQ(bu[9].r, s3); // ABC full on
	}

	{
		using CC = Ay38912<2>::CC;
		Ay38912<2>	 ay(1000000, Ay38912<2>::acb_stereo, 1.0f);
		StereoSample bu[100];
		int			 cc_per_sample = 1000000 / 50000;
		CC			 cc0(0);

		// at start channels are off => follow volume & volume=0
		ay.audioBufferStart(bu, 100);
		ay.setRegister(cc0 + 1 * cc_per_sample, 8, 15);	 // A full on
		ay.setRegister(cc0 + 2 * cc_per_sample, 8, 0);	 // A off
		ay.setRegister(cc0 + 3 * cc_per_sample, 9, 15);	 // B full on
		ay.setRegister(cc0 + 4 * cc_per_sample, 9, 0);	 // B off
		ay.setRegister(cc0 + 5 * cc_per_sample, 10, 15); // C full on
		ay.setRegister(cc0 + 6 * cc_per_sample, 10, 0);	 // C off
		ay.setRegister(cc0 + 7 * cc_per_sample, 8, 15);	 // A full on
		ay.setRegister(cc0 + 8 * cc_per_sample, 9, 15);	 // AB full on
		ay.setRegister(cc0 + 9 * cc_per_sample, 10, 15); // ABC full on
		ay.audioBufferEnd();

		int s0 = -32768;
		int s1 = s0 + 0xffff * 1 / 3;
		int s2 = s0 + 0xffff * 2 / 3;
		int s3 = s0 + 0xffff * 3 / 3;

		CHECK_EQ(bu[0].l, s0); // all off
		CHECK_EQ(bu[1].l, s2); // A full on
		CHECK_EQ(bu[2].l, s0); // A off
		CHECK_EQ(bu[3].l, s0); // B full on
		CHECK_EQ(bu[4].l, s0); // B off
		CHECK_EQ(bu[5].l, s1); // C full on
		CHECK_EQ(bu[6].l, s0); // C off
		CHECK_EQ(bu[7].l, s2); // A full on
		CHECK_EQ(bu[8].l, s2); // AB full on
		CHECK_EQ(bu[9].l, s3); // ABC full on

		CHECK_EQ(bu[0].r, s0); // all off
		CHECK_EQ(bu[1].r, s0); // A full on
		CHECK_EQ(bu[2].r, s0); // A off
		CHECK_EQ(bu[3].r, s2); // B full on
		CHECK_EQ(bu[4].r, s0); // B off
		CHECK_EQ(bu[5].r, s1); // C full on
		CHECK_EQ(bu[6].r, s0); // C off
		CHECK_EQ(bu[7].r, s0); // A full on
		CHECK_EQ(bu[8].r, s2); // AB full on
		CHECK_EQ(bu[9].r, s3); // ABC full on
	}

	{
		constexpr int cc_per_sample = 8; // integer divisions ahead!
		hw_sample_frequency			= 100000;
		constexpr float ay_f		= 100000 * cc_per_sample;
		Ay38912<1>		ay(ay_f, Ay38912<1>::mono, 1.0f);
		MonoSample		bu[10000];

		int period = 100;

		ay.setRegister(7, 0x3f - 1);			// enable A
		ay.setRegister(0, uint8(period % 256)); // A fine
		ay.setRegister(1, uint8(period / 256)); // A coarse
		ay.setRegister(8, 15);					// A volume

		// sound starts with delay because prev. period (0x0fff*16cc) must first expire
		ay.audioBufferStart(bu, 10000);
		ay.audioBufferEnd();

		// now check square wave:
		ay.audioBufferStart(bu, 10000);
		ay.audioBufferEnd();

		MonoSample s = bu[0];
		int		   i = 0;
		while (i < 10000 && bu[i] == s) i++;
		REQUIRE(i < 10000);

		int n = 0;
		int e = i;
		for (int j = i; j < 10000; j++)
		{
			if (bu[j] == s) continue;
			s = bu[j];
			n++;
			e = j;
		}

		int cc_per_phase = period * 8 / cc_per_sample;
		CHECK_EQ((n - 1) * cc_per_phase, (e - i));

		// ---

		period = 300;

		ay.setRegister(7, 0x3f - 2);			// enable B
		ay.setRegister(2, uint8(period % 256)); // B fine
		ay.setRegister(3, uint8(period / 256)); // B coarse
		ay.setRegister(9, 15);					// B volume

		ay.audioBufferStart(bu, 10000);
		ay.audioBufferEnd();

		s = bu[0];
		i = 0;
		while (i < 10000 && bu[i] == s) i++;
		REQUIRE(i < 10000);

		n = 0;
		e = i;
		for (int j = i; j < 10000; j++)
		{
			if (bu[j] == s) continue;
			s = bu[j];
			n++;
			e = j;
		}

		cc_per_phase = period * 8 / cc_per_sample;
		CHECK_EQ((n - 1) * cc_per_phase, (e - i));

		// ---

		period = 333;

		ay.setRegister(7, 0x3f - 4);			// enable C
		ay.setRegister(4, uint8(period % 256)); // C fine
		ay.setRegister(5, uint8(period / 256)); // C coarse
		ay.setRegister(10, 15);					// C volume

		ay.audioBufferStart(bu, 10000);
		ay.audioBufferEnd();

		s = bu[0];
		i = 0;
		while (i < 10000 && bu[i] == s) i++;
		REQUIRE(i < 10000);

		n = 0;
		e = i;
		for (int j = i; j < 10000; j++)
		{
			if (bu[j] == s) continue;
			s = bu[j];
			n++;
			e = j;
		}

		cc_per_phase = period * 8 / cc_per_sample;
		CHECK_EQ((n - 1) * cc_per_phase, (e - i));
	}
}


#define HT_GET_KEY(x)  ((x) >> 12)
#define HT_GET_CODE(x) ((x)&0x0FFF)
#define HT_PUT_KEY(x)  ((x) << 12)
#define HT_PUT_CODE(x) ((x)&0x0FFF)

struct lzh_compressor
{
	static constexpr int cmap_bits		= 8;
	static constexpr int FIRST_CODE		= 4097;
	static constexpr int HT_SIZE		= 8192;	  /* 13 bit hash table size */
	static constexpr int HT_KEY_MASK	= 0x1FFF; /* 13 bit key mask */
	static constexpr int FLUSH_OUTPUT	= 4096;	  /* Impossible code = flush */
	static constexpr int IMAGE_COMPLETE = 1;	  /* finished reading or writing */
	static constexpr int IMAGE_SAVING	= 0;	  /* file_state = processing */
	static constexpr int LZ_MAX_CODE	= 4095;	  /* Largest 12 bit code */

	int	   current_code		 = FIRST_CODE;
	int	   bufsize			 = 0;
	int	   file_state		 = IMAGE_SAVING;
	int	   shift_state		 = 0;
	uint32 shift_data		 = 0;
	int	   running_bits		 = cmap_bits + 1;
	int	   max_code_plus_one = 1 << running_bits;
	int	   clear_code		 = 1 << cmap_bits;
	int	   eof_code			 = clear_code + 1;
	int	   running_code		 = eof_code + 1;
	int	   depth			 = cmap_bits;

	uint32 hash_table[HT_SIZE];
	uint8  buf[256];

	uint filesize = 0;

	int	 gif_hash_key(uint32 key);
	int	 lookup_hash_key(uint32 key);
	void clear_hash_table();
	void add_hash_key(uint32 key, int code);
	void write_gif_byte(int ch);
	void write_gif_code(int code);
	void write_data(cuptr pixel, uint length);
	uint compress(cuptr pixel, uint length);
	void init();
	uint finish();
};

int lzh_compressor::gif_hash_key(uint32 key)
{
	return ((key >> 12) ^ key) & HT_KEY_MASK; //
}

void lzh_compressor::clear_hash_table()
{
	for (int i = 0; i < HT_SIZE; i++) { hash_table[i] = 0xFFFFFFFFL; }
}

void lzh_compressor::add_hash_key(uint32 key, int code)
{
	int hkey = gif_hash_key(key);
	while (HT_GET_KEY(hash_table[hkey]) != 0xFFFFFL) { hkey = (hkey + 1) & HT_KEY_MASK; }
	hash_table[hkey] = HT_PUT_KEY(key) | HT_PUT_CODE(code);
}

int lzh_compressor::lookup_hash_key(uint32 key)
{
	int	   hkey = gif_hash_key(key);
	uint32 htkey;

	while ((htkey = HT_GET_KEY(hash_table[hkey])) != 0xFFFFFL)
	{
		if (key == htkey) return HT_GET_CODE(hash_table[hkey]); // exists
		hkey = (hkey + 1) & HT_KEY_MASK;
	}
	return -1; // does not exist
}

void lzh_compressor::write_gif_byte(int ch)
{
	if (file_state == IMAGE_COMPLETE) return;

	assert(bufsize <= 255);

	if (ch == FLUSH_OUTPUT)
	{
		if (bufsize)
		{
			filesize += 1;			   //fd->write_uchar(bufsize);
			filesize += uint(bufsize); //fd->write(ptr(buf), bufsize);
			bufsize = 0;
		}
		/* write an empty block to mark end of data */
		filesize += 1; //fd->write_uchar(0);
		file_state = IMAGE_COMPLETE;
	}
	else
	{
		if (bufsize == 255)
		{
			/* write this buffer to the file */
			filesize += 1;			   //fd->write_uchar(bufsize);
			filesize += uint(bufsize); //fd->write(ptr(buf), bufsize);
			bufsize = 0;
		}
		buf[bufsize++] = uint8(ch);
	}
}

void lzh_compressor::write_gif_code(int code)
{
	// Write a Gif code word to the output file.
	//
	// This function packages code words up into whole bytes
	// before writing them. It uses the encoder to store
	// codes until enough can be packaged into a whole byte.

	if (code == FLUSH_OUTPUT)
	{
		// write all remaining data:
		while (shift_state > 0)
		{
			write_gif_byte(shift_data & 0xff);
			shift_data >>= 8;
			shift_state -= 8;
		}
		shift_state = 0;
		write_gif_byte(FLUSH_OUTPUT);
	}
	else
	{
		shift_data |= uint32(code) << shift_state;
		shift_state += running_bits;

		while (shift_state >= 8)
		{
			// write full bytes:
			write_gif_byte(shift_data & 0xff);
			shift_data >>= 8;
			shift_state -= 8;
		}
	}

	// If code can't fit into running_bits bits, raise its size.
	// Note: codes above 4095 are for signalling.
	if (running_code >= max_code_plus_one && code <= 4095) { max_code_plus_one = 1 << ++running_bits; }
}

void lzh_compressor::write_data(cuptr pixel, uint length)
{
	int	   new_code;
	uint32 new_key;
	uchar  pixval;
	uint   i = 0;

	int current_code = this->current_code == FIRST_CODE ? pixel[i++] : this->current_code;

	while (i < length)
	{
		pixval = pixel[i++]; // Fetch next pixel from stream

		// Form a new unique key to search hash table for the code
		// Combines current_code as prefix string with pixval as postfix char
		new_key = (uint32(current_code) << 8) + pixval;
		if ((new_code = lookup_hash_key(new_key)) >= 0)
		{
			// This key is already there, or the string is old,
			// so simply take new code as current_code
			current_code = new_code;
		}
		else
		{
			// Put it in hash table, output the prefix code,
			// and make current_code equal to pixval
			this->write_gif_code(current_code);
			current_code = pixval;

			// If the hash_table if full, send a clear first
			// then clear the hash table:
			if (this->running_code >= LZ_MAX_CODE)
			{
				this->write_gif_code(this->clear_code);
				this->running_code		= this->eof_code + 1;
				this->running_bits		= this->depth + 1;
				this->max_code_plus_one = 1 << this->running_bits;
				clear_hash_table();
			}
			else
			{
				// Put this unique key with its relative code in hash table
				add_hash_key(new_key, this->running_code++);
			}
		}
	}

	// Preserve the current state of the compressor:
	this->current_code = current_code;
}

void lzh_compressor::init()
{
	depth			  = cmap_bits;
	file_state		  = IMAGE_SAVING;
	bufsize			  = 0;
	buf[0]			  = 0;
	clear_code		  = 1 << cmap_bits;
	eof_code		  = clear_code + 1;
	running_code	  = eof_code + 1;
	running_bits	  = cmap_bits + 1;
	max_code_plus_one = 1 << running_bits;
	current_code	  = FIRST_CODE;
	shift_state		  = 0;
	shift_data		  = 0;

	//fd->write_uchar(uchar(cmap_bits)); // Write the lzw minimum code size
	clear_hash_table();
	write_gif_code(clear_code);
}

uint lzh_compressor::finish()
{
	write_gif_code(current_code);
	write_gif_code(eof_code);
	write_gif_code(FLUSH_OUTPUT);
	return filesize;
}

uint lzh_compressor::compress(cuptr pixel, uint length)
{
	init();
	write_data(pixel, length);
	return finish();
}

static inline void			 set_bit_at_index(uchar bits[], uint i) { bits[i / 8] |= (1 << i % 8); }
static inline constexpr bool bit_at_index(uchar bits[], uint i) { return (bits[i / 8] >> (i % 8)) & 1; }

TEST_CASE("Audio::Ay38912 noise")
{
	// is it noise?

	{
		hw_sample_frequency = 50000;
		Ay38912<1> ay(50000 * 16, Ay38912<1>::mono, 1.0f);
		MonoSample bu[256];

		ay.setRegister(7, 0x07); // noise on channel ABC
		ay.setRegister(6, 1);	 //noise period
		ay.setRegister(8, 15);	 //channel A volume
		ay.setRegister(9, 15);	 //channel B volume
		ay.setRegister(10, 15);	 //channel C volume

		lzh_compressor compressor;
		compressor.init();

		uint8 ubu[1024 * 256 / 8]; // for zlib compress

		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();
		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		for (uint i = 0; i < 1024; i++)
		{
			ay.audioBufferStart(bu, NELEM(bu));
			ay.audioBufferEnd();

			uint8 qbu[256 / 8];
			memset(qbu, 0, sizeof(qbu));
			for (uint j = 0; j < 256; j++)
			{
				//printf("%6i\n", bu[j].m);
				if (bu[j].m < 0) set_bit_at_index(qbu, j);
			}
			compressor.write_data(qbu, sizeof(qbu));
			memcpy(ubu + i * 256 / 8, qbu, 256 / 8);
		}
		uint csize = compressor.finish();
		uint usize = 1024 * 256 / 8;
		printf("uncompressed size = %i\n", usize);	 // => 32768
		printf("compressed lzh size = %i\n", csize); // => 45527 ((whow!))
		CHECK_GT(csize * 8, usize * 10);

		uint8 cbu[40000];
		ulong cbusize = sizeof(cbu);
		auto  err	  = compress(cbu, &cbusize, ubu, sizeof(ubu));
		CHECK_EQ(err, 0);
		printf("compressed zlib size = %lu\n", cbusize); // => 32786 ((uncompressibe))
		CHECK_GT(cbusize, usize);

		// fyi: the used shift register rng has a period of 114681 (0x1BFF9) bits
		uint b = (sizeof(ubu) - 0x4000) * 8; // compare against the last 0x4000 bytes
		for (uint a = 0; a < 0x4000 * 8; a++)
		{
			uint i = 0;
			for (i = 0; i < 0x4000 * 8 && bit_at_index(ubu, a + i) == bit_at_index(ubu, b + i); i++) {}
			if (i == 0x4000 * 8) printf("repetition at bit 0x%x vs. 0x%x\n", a, b);
		}

		// for reference: Xoshiro random numbers:
		Xoshiro256 xo(435634);
		for (uint i = 0; i < sizeof(ubu); i++) { ubu[i] = uint8(xo.random(256u)); }
		cbusize = sizeof(cbu);
		err		= compress(cbu, &cbusize, ubu, sizeof(ubu));
		CHECK_EQ(err, 0);
		printf("compressed zlib size = %lu (xoshiro) \n", cbusize); // => 32784 ((uncompressibe))
	}

	// mixed to A,B,C?

	{
		Ay38912<2>	 ay(1000000, Ay38912<2>::abc_stereo);
		StereoSample bu[250];

		ay.setRegister(6, 1);	// noise period
		ay.setRegister(8, 15);	// channel A volume
		ay.setRegister(9, 15);	// channel B volume
		ay.setRegister(10, 15); // channel C volume

		ay.setRegister(7, 0x3f - 8); // noise on channel A
		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		Sample lmin = bu[0].l, lmax = lmin, rmin = bu[0].r, rmax = rmin;
		for (uint i = 0; i < NELEM(bu); i++)
		{
			lmin = min(lmin, bu[i].l);
			rmin = min(rmin, bu[i].r);
			lmax = max(lmax, bu[i].l);
			rmax = max(rmax, bu[i].r);
		}

		int v0 = -0x4000;
		int v1 = v0 + 0x7fff * 1 / 3;
		int v2 = v0 + 0x7fff * 2 / 3;
		int v3 = v0 + 0x7fff * 3 / 3;

		CHECK_EQ(lmin, v1);
		CHECK_EQ(lmax, v3);
		CHECK_EQ(rmin, v3);
		CHECK_EQ(rmax, v3);

		ay.setRegister(7, 0x3f - 0x20); // noise on channel C
		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		lmin = bu[0].l, lmax = lmin, rmin = bu[0].r, rmax = rmin;
		for (uint i = 0; i < NELEM(bu); i++)
		{
			lmin = min(lmin, bu[i].l);
			rmin = min(rmin, bu[i].r);
			lmax = max(lmax, bu[i].l);
			rmax = max(rmax, bu[i].r);
		}

		CHECK_EQ(rmin, v1);
		CHECK_EQ(rmax, v3);
		CHECK_EQ(lmin, v3);
		CHECK_EQ(lmax, v3);

		ay.setRegister(7, 0x3f - 0x10); // noise on channel B

		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		lmin = bu[0].l, lmax = lmin, rmin = bu[0].r, rmax = rmin;
		for (uint i = 0; i < NELEM(bu); i++)
		{
			lmin = min(lmin, bu[i].l);
			rmin = min(rmin, bu[i].r);
			lmax = max(lmax, bu[i].l);
			rmax = max(rmax, bu[i].r);
		}

		CHECK_EQ(lmin, v2 + 1); // +1: there is some rounding
		CHECK_EQ(lmax, v3);
		CHECK_EQ(rmin, v2 + 1);
		CHECK_EQ(rmax, v3);
	}

	// only in high phase?
	{
		hw_sample_frequency = 50000;
		Ay38912<2>	 ay(50000 * 8, Ay38912<2>::acb_stereo);
		StereoSample bu[250];

		ay.setRegister(6, 1);  // noise period = 1*16
		ay.setRegister(8, 15); // channel A volume
		ay.setRegister(9, 15); // channel B volume
		ay.setRegister(10, 0); // channel C volume
		ay.setRegister(0, 8);  // f_A = 8*16
		ay.setRegister(1, 0);
		ay.setRegister(2, 8); // f_B = f_A
		ay.setRegister(3, 0);

		ay.setRegister(7, 0x3f - 8 - 3); // noise on channel A, enable channel A & B

		for (uint i = 0; i < 20; i++) // takes some time before the current tune period expires
		{
			ay.audioBufferStart(bu, NELEM(bu));
			ay.audioBufferEnd();
		}

		int v0 = -0x4000;
		//int v1 = v0 + 0x7fff * 1 / 3;
		//int v2 = v0 + 0x7fff * 2 / 3;
		//int v3 = v0 + 0x7fff * 3 / 3;

		uint hi = 0, lo = 0;
		for (uint i = 0; i < NELEM(bu); i++)
		{
			StereoSample& s = bu[i];
			//printf("%6i, %8i\n", s.l, s.r);
			if (s.r < 0)
			{
				CHECK_EQ(s.l, s.r); // if tune in low phase then no noise added!
			}
			else
			{
				if (s.l != s.r) // if tune in high phase then noise added!
				{
					lo++;
					CHECK_EQ(s.l, v0);
				}
				else hi++;
			}
		}
		//printf("lo=%i + hi=%i = %i (from %li total)\n", lo, hi, lo + hi, NELEM(bu));
		CHECK_GE(lo, 50); // 58
		CHECK_GE(hi, 50); // 70
	}
}

TEST_CASE("Audio::Ay38912 envelope")
{
	// mix to A, B, C

	Sample log_A[16]; // channel A or C
	Sample log_B[16]; // channel B

	{
		using CC			= Ay38912<2>::CC;
		hw_sample_frequency = 50000;
		Ay38912<2>	 ay(50000 * 16, Ay38912<2>::abc_stereo, 1.0f);
		StereoSample bu[250];

		// extract the log_vol table:
		ay.audioBufferStart(bu, NELEM(bu));
		for (uint8 i = 0; i < 16; i++) { ay.setRegister(CC(0) + i * 16, 8, i); }
		ay.setRegister(CC(256), 8, 0);
		for (uint8 i = 0; i < 16; i++) { ay.setRegister(CC(256) + i * 16, 9, i); }
		ay.audioBufferEnd();

		for (uint i = 0; i < 16; i++) { log_A[i] = bu[i].l; }
		for (uint i = 0; i < 16; i++) { log_B[i] = bu[i + 16].l; }

		ay.reset();
		ay.setRegister(11, 1); // shortest envelope period
		ay.setRegister(12, 0);

		ay.setRegister(8, 16);		// channel A volume: use envelope
		ay.setRegister(13, 0b1100); //  ///////

		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		for (uint i = 0; i < NELEM(bu); i++)
		{
			CHECK_EQ(bu[i].r, -0x8000);
			CHECK_EQ(bu[i].l, log_A[i & 15]);
		}

		ay.setRegister(8, 0);		// A off
		ay.setRegister(9, 16);		// channel B volume: use envelope
		ay.setRegister(13, 0b1100); //  ///////

		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		for (uint i = 0; i < NELEM(bu); i++)
		{
			CHECK_EQ(bu[i].r, log_B[i & 15]);
			CHECK_EQ(bu[i].l, log_B[i & 15]);
		}

		ay.setRegister(9, 0);		// B off
		ay.setRegister(10, 16);		// channel C volume: use envelope
		ay.setRegister(13, 0b1100); //  ///////

		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		for (uint i = 0; i < NELEM(bu); i++)
		{
			CHECK_EQ(bu[i].l, -0x8000);
			CHECK_EQ(bu[i].r, log_A[i & 15]);
		}
	}

	// frequency

	{
		hw_sample_frequency = 50000;
		Ay38912<1> ay(50000 * 16, Ay38912<1>::mono, 1.0f);
		MonoSample bu[1024 + 1];

		ay.setRegister(8, 16); // channel A use env
		ay.setRegister(12, 0); //env coarse period
		for (uint i = 1; i < 1024 / 16; i++)
		{
			ay.setRegister(11, uint8(i));
			ay.setRegister(13, 0b1100); //      ///////
			ay.audioBufferStart(bu, NELEM(bu));
			ay.audioBufferEnd();

			CHECK_EQ(bu[0].m, -0x8000);
			CHECK_EQ(bu[i * 16].m, -0x8000);
			CHECK_EQ(bu[i - 1].m, log_B[0]);
			CHECK_EQ(bu[i].m, log_B[1]);
			CHECK_EQ(bu[2 * i - 1].m, log_B[1]);
			CHECK_EQ(bu[2 * i].m, log_B[2]);
			CHECK_EQ(bu[15 * i - 1].m, log_B[14]);
			CHECK_EQ(bu[15 * i].m, log_B[15]);
			CHECK_EQ(bu[16 * i - 1].m, log_B[15]);
			CHECK_EQ(bu[16 * i].m, log_B[0]);
		}

		ay.setRegister(11, 0);
		ay.setRegister(12, 1);
		ay.setRegister(13, 0b1100); //      ///////

		int idx = -1;
		for (uint i0 = 0; i0 < 256 * 16; i0 += 1024)
		{
			ay.audioBufferStart(bu, 1024);
			ay.audioBufferEnd();

			for (uint i = 0; i < 1024; i++)
			{
				if (i % 256 == 0) idx++;
				CHECK_EQ(bu[i].m, log_B[idx]);
			}
		}
	}

	// shape

	{
		hw_sample_frequency = 50000;
		Ay38912<1> ay(50000 * 16, Ay38912<1>::mono, 1.0f);
		MonoSample bu[128];

		ay.setRegister(8, 16); // channel A use env
		ay.setRegister(11, 1); //env fine period
		ay.setRegister(12, 0); //env coarse period

		auto up = [&](uint i0) {
			for (uint i = 0; i < 16; i++)
				if (bu[i0 + i].m != log_B[i]) return false;
			return true;
		};
		auto down = [&](uint i0) {
			for (uint i = 0; i < 16; i++)
				if (bu[i0 + 15 - i].m != log_B[i]) return false;
			return true;
		};
		auto high = [&](uint i0) {
			for (uint i = 0; i < 16; i++)
				if (bu[i0 + i].m != log_B[15]) return false;
			return true;
		};
		auto low = [&](uint i0) {
			for (uint i = 0; i < 16; i++)
				if (bu[i0 + i].m != log_B[0]) return false;
			return true;
		};

		for (uint8 shape = 0; shape < 4; shape++) //   \_______
		{
			ay.setRegister(13, shape);
			ay.audioBufferStart(bu, NELEM(bu));
			ay.audioBufferEnd();

			CHECK(down(0));
			CHECK(low(16));
			CHECK(low(32));
		}

		for (uint8 shape = 4; shape < 8; shape++) //    /______
		{
			ay.setRegister(13, shape);
			ay.audioBufferStart(bu, NELEM(bu));
			ay.audioBufferEnd();

			CHECK(up(0));
			CHECK(low(16));
			CHECK(low(32));
		}

		ay.setRegister(13, 0b1000); //     "\\\\\\\\"
		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		CHECK(down(0));
		CHECK(down(16));
		CHECK(down(32));

		ay.setRegister(13, 0b1001); //     "\___________"
		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		CHECK(down(0));
		CHECK(low(16));
		CHECK(low(32));

		ay.setRegister(13, 0b1010); //     "\/\/\/\/\/"
		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		CHECK(down(0));
		CHECK(up(16));
		CHECK(down(32));
		CHECK(up(48));
		CHECK(down(64));
		CHECK(up(80));

		ay.setRegister(13, 0b1011); //     "\~~~~~~~~~"
		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		CHECK(down(0));
		CHECK(high(16));
		CHECK(high(32));

		ay.setRegister(13, 0b1100); //     "///////"
		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		CHECK(up(0));
		CHECK(up(16));
		CHECK(up(32));

		ay.setRegister(13, 0b1101); //     "/~~~~~~~"
		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		CHECK(up(0));
		CHECK(high(16));
		CHECK(high(32));

		ay.setRegister(13, 0b1110); //     "/\/\/\/\/\"
		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		CHECK(up(0));
		CHECK(down(16));
		CHECK(up(32));
		CHECK(down(48));
		CHECK(up(64));
		CHECK(down(80));

		ay.setRegister(13, 0b1111); //       /_________
		ay.audioBufferStart(bu, NELEM(bu));
		ay.audioBufferEnd();

		CHECK(up(0));
		CHECK(low(16));
		CHECK(low(32));
	}
}

#if 0
TEST_CASE("Audio::Ay38912 play \"Ninja Spirits #5.ym\"")
{
	// this file doesn't use channel A and C for a prolonged time
	// which resulted in .when underflow which resulted in running beyond the output buffer end

	cstr			infile = "test_files/Ninja Spirits  5.ym";
	YMFileConverter converter(converter.WAV);
	converter.convert(infile, "/tmp", true);
}

  #if defined YM_FILE && defined YM_WHAT

TEST_CASE("Audio::Ay38912 create file")
{
	using What				= YMFileConverter::What;
	bool			verbose = true;
	cstr			infile	= YM_FILE;
	YMFileConverter converter(What(0xff)); //(YM_WHAT);
	cstr			outdir = directory_from_path(infile);
	converter.convert(infile, outdir, verbose);
	CHECK(1);
}

  #endif

  #if defined YM_DIR && defined YM_WHAT

TEST_CASE("Audio::Ay38912 create files in dir")
{
	using What				  = YMFileConverter::What;
	bool			recursive = true;
	bool			verbose	  = false;
	YMFileConverter converter(YM_WHAT);
	converter.convertDir(YM_DIR, recursive, verbose);
	if (verbose) printf("-----------------------------\n");
	CHECK(1);
}

  #endif
#endif


} // namespace kio::Test

/*











































*/
