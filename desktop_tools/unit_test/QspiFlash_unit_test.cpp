// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Devices/Flash.h"
#include "Devices/Preferences.h"
#include "Devices/QspiFlashDevice.h"
#include "Mock/MockFlash.h"
#include "common/Array.h"
#include "common/cstrings.h"
#include "doctest.h"

// defined in main_unit_test.cpp:

namespace kio
{
extern doctest::String toString(Array<cstr>);

namespace Test
{
using namespace kio::Devices;

static constexpr int			 ssw   = 8; // or 9
static constexpr int			 sse   = 12;
static constexpr uint32			 wsize = 1 << ssw;
static constexpr uint32			 esize = 1 << sse;
__unused static constexpr uint32 wmask = wsize - 1;
__unused static constexpr uint32 emask = esize - 1;

static constexpr uint32 flash_size = 2 MB;
static Array<uint8>		flash {flash_size};
static Array<uint8>		flash2 {flash_size};
static Array<cstr>		log;
static bool				error = 0;

static MockFlash mock_flash(flash, log, error);

#define LOG(...) log.append(usingstr(__VA_ARGS__))

static constexpr uint random_data_size = flash_size;
static uint8*		  random_data	   = new uint8[random_data_size];

static void printLog(int line)
{
	printf("@%i flash log:\n", line);
	if (log.count() == 0) printf("    *empty*\n");
	for (uint i = 0; i < log.count(); i++) { printf("    %s\n", log[i]); }
}


class QspiMock : public QspiFlashDevice<ssw>
{
public:
	using super = QspiFlashDevice;

	uint32 flashdisk_base;
	uint32 flashdisk_size;
	uint32 flashdisk_end;
	uint32 _padding;

	QspiMock(uint32 offset, uint32 size, uint8 initial_data = 0xff) : //
		super(offset, size),
		flashdisk_base(offset),
		flashdisk_size(size),
		flashdisk_end(offset + size)
	{
		assert(offset + size <= flash_size);
		assert(offset % esize == 0);
		assert(size % esize == 0);

		log.purge();
		error = 0;

		memset(&flash[0], 0xe5, flash_size);
		if (initial_data) memset(&flash[flashdisk_base], initial_data, flashdisk_size);
		else memcpy(&flash[flashdisk_base], random_data, flashdisk_size);
		memcpy(&flash2[0], &flash[0], flash_size);
	}

	void verify_writing()
	{
		if (memcmp(&flash2[0], &flash[0], flash_size) == 0) return;
		if (!error) LOG("WRITE ERROR");

		bool same = true;
		for (uint i = 0; i < flash_size; i++)
		{
			if ((flash2[i] == flash[i]) == same) continue;
			same = !same;
			if (!same) printf("0x%08x: erste abweichung soll=0x%02x, ist=0x%02x\n", i, flash2[i], flash[i]);
			else printf("0x%08x: letzte abweichung soll=0x%02x, ist=0x%02x\n", i - 1, flash2[i - 1], flash[i - 1]);
		}

		error = 1;
	}

	void verify_reading(ADDR addr, void* data, SIZE size)
	{
		if (memcmp(&flash2[flashdisk_base + addr], data, size) == 0) return;
		if (!error) LOG("READ ERROR");
		error = 1;
	}

	virtual void readData(ADDR addr, void* data, SIZE size) throws override
	{
		assert_le(addr + size, flashdisk_size);
		LOG("readData %u,%u", addr, size);

		super::readData(addr, data, size);
		verify_reading(flashdisk_base + addr, data, size);
	}

	virtual void readSectors(LBA lba, void* data, SIZE count) throws override
	{
		assert_le((lba + count) << ssw, flashdisk_size);
		LOG("readSectors %u,%u", lba, count);

		super::readSectors(lba, data, count);
		verify_reading(flashdisk_base + (lba << ssw), data, count << ssw);
	}

	virtual void writeData(ADDR addr, const void* data, SIZE size) throws override
	{
		assert_le(addr + size, flashdisk_size);
		LOG("writeData %u,%u", addr, size);

		if (data) memcpy(&flash2[flashdisk_base + addr], data, size);
		else memset(&flash2[flashdisk_base + addr], 0xff, size);

		super::writeData(addr, data, size);
		verify_writing();
	}

	virtual void writeSectors(LBA lba, const void* data, SIZE count) throws override
	{
		assert_le((lba + count) << ssw, flashdisk_size);
		LOG("writeSectors %u,%u", lba, count);

		uint32 addr = lba << ssw;
		uint   size = count << ssw;
		if (data) memcpy(&flash2[flashdisk_base + addr], data, size);
		else memset(&flash2[flashdisk_base + addr], 0xff, size);

		super::writeSectors(lba, data, count);
		verify_writing();
	}

	//virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) throws override;
};

TEST_CASE("QspiFlash: constructor")
{
	for (uint i = 0; i < random_data_size; i++) random_data[i] = uint8(rand() >> 8);
	Flash::setupMockFlash(&flash[0], flash_size);

	QspiMock q(500 kB, 1 MB);
	q.writeData(0, nullptr, 0);
	CHECK_FALSE(error);
}

TEST_CASE("QspiFlash: erase sectors")
{
	{
		QspiMock q(500 kB, 1 MB, 123);
		CHECK_FALSE(error);
		q.writeData(0, nullptr, 0);
		CHECK_FALSE(error);
		q.writeSectors(0, nullptr, 1);
		CHECK_FALSE(error);
		q.writeSectors((1 MB - esize) / wsize, nullptr, esize / wsize);
		CHECK_FALSE(error);
		q.writeSectors((752 kB) >> ssw, nullptr, 240 kB >> ssw);
		CHECK_FALSE(error);
		printLog(__LINE__);
	}
	{
		QspiMock q(1 MB, 760 kB, 123);
		CHECK_FALSE(error);
		q.writeSectors(0, nullptr, 760 kB >> ssw);
		CHECK_FALSE(error);
		printLog(__LINE__);
	}
	{
		QspiMock q(500 kB, 1 MB, 123);
		CHECK_FALSE(error);
		q.writeData(0, nullptr, 0);
		CHECK_FALSE(error);
		q.writeSectors(0, nullptr, 1);
		CHECK_FALSE(error);
		q.writeSectors(120 kB / wsize - 1, nullptr, 1);
		CHECK_FALSE(error);
		q.writeSectors(121 kB / wsize - 2, nullptr, 2);
		CHECK_FALSE(error);
		q.writeSectors(122 kB / wsize - 1, nullptr, 2);
		CHECK_FALSE(error);
		q.writeSectors(123 kB / wsize, nullptr, 2);
		CHECK_FALSE(error);
		q.writeSectors(124 kB / wsize, nullptr, 15);
		CHECK_FALSE(error);
		q.writeSectors(126 kB / wsize, nullptr, 16);
		CHECK_FALSE(error);
		q.writeSectors(128 kB / wsize, nullptr, 17);
		CHECK_FALSE(error);
		q.writeSectors(130 kB / wsize + 1, nullptr, 9);
		CHECK_FALSE(error);
		q.writeSectors(131 kB / wsize + 1, nullptr, 15);
		CHECK_FALSE(error);
		q.writeSectors(133 kB / wsize + 1, nullptr, 16);
		CHECK_FALSE(error);
		q.writeSectors(135 kB / wsize + 1, nullptr, 17);
		CHECK_FALSE(error);
		q.writeSectors(137 kB / wsize - 1, nullptr, 23);
		CHECK_FALSE(error);
		q.writeSectors(139 kB / wsize - 1, nullptr, 123);
		CHECK_FALSE(error);
		printLog(__LINE__);
	}
}

TEST_CASE("QspiFlash: write sectors")
{
	{
		QspiMock q(600 kB, 1100 kB, 123);
		CHECK_FALSE(error);
		q.writeData(0, random_data, 0);
		CHECK_FALSE(error);

		q.writeSectors(0, random_data, 16);
		CHECK_FALSE(error);

		q.writeSectors(0, random_data + 99, 1);
		CHECK_FALSE(error);

		q.writeSectors((1 MB - esize) / wsize, random_data, esize / wsize);
		CHECK_FALSE(error);
		q.writeSectors((352 kB) >> ssw, random_data, 720 kB >> ssw);
		CHECK_FALSE(error);
		printLog(__LINE__);
	}
	{
		QspiMock q(820 kB, 800 kB, 123);
		CHECK_FALSE(error);
		q.writeSectors(esize >> ssw, random_data, 760 kB >> ssw);
		CHECK_FALSE(error);
		q.writeSectors(0, random_data, 800 kB >> ssw);
		CHECK_FALSE(error);
		printLog(__LINE__);
	}
	{
		QspiMock q(500 kB, 1 MB, 123);
		CHECK_FALSE(error);
		q.writeData(0, random_data, 0);
		CHECK_FALSE(error);
		q.writeSectors(0, random_data, 1);
		CHECK_FALSE(error);
		q.writeSectors(120 kB / wsize - 1, random_data, 1);
		CHECK_FALSE(error);
		q.writeSectors(121 kB / wsize - 2, random_data, 2);
		CHECK_FALSE(error);
		q.writeSectors(122 kB / wsize - 1, random_data, 2);
		CHECK_FALSE(error);
		q.writeSectors(123 kB / wsize, random_data, 2);
		CHECK_FALSE(error);
		q.writeSectors(124 kB / wsize, random_data, 15);
		CHECK_FALSE(error);
		q.writeSectors(126 kB / wsize, random_data, 16);
		CHECK_FALSE(error);
		q.writeSectors(128 kB / wsize, random_data, 17);
		CHECK_FALSE(error);
		q.writeSectors(130 kB / wsize + 1, random_data, 9);
		CHECK_FALSE(error);
		q.writeSectors(131 kB / wsize + 1, random_data, 15);
		CHECK_FALSE(error);
		q.writeSectors(133 kB / wsize + 1, random_data, 16);
		CHECK_FALSE(error);
		q.writeSectors(135 kB / wsize + 1, random_data, 17);
		CHECK_FALSE(error);
		q.writeSectors(137 kB / wsize - 1, random_data, 23);
		CHECK_FALSE(error);
		q.writeSectors(139 kB / wsize - 1, random_data, 123);
		CHECK_FALSE(error);
		printLog(__LINE__);
	}
}

TEST_CASE("QspiFlash: erase range of bytes - aligned to esize")
{
	{
		QspiMock q(500 kB, 1 MB, 123);
		CHECK_FALSE(error);
		q.writeData(0, nullptr, 0);
		CHECK_FALSE(error);
		q.writeData(0, nullptr, esize);
		CHECK_FALSE(error);
		q.writeData((1 MB - esize), nullptr, esize);
		CHECK_FALSE(error);
		q.writeData((752 kB), nullptr, 240 kB);
		CHECK_FALSE(error);
		printLog(__LINE__);
	}
}

TEST_CASE("QspiFlash: erase range of bytes - aligned to wsize")
{
	{
		QspiMock q(500 kB, 1 MB, 123);
		CHECK_FALSE(error);
		q.writeData(0, nullptr, 0);
		CHECK_FALSE(error);
		q.writeData(0, nullptr, wsize);
		CHECK_FALSE(error);
		q.writeData((1 MB - wsize), nullptr, wsize);
		CHECK_FALSE(error);
		q.writeData((751 kB), nullptr, 243 kB);
		CHECK_FALSE(error);
		q.writeData((wsize * 17), nullptr, wsize * 14);
		CHECK_FALSE(error);
		q.writeData((esize * 1 + wsize * 17), nullptr, wsize * 15);
		CHECK_FALSE(error);
		q.writeData((esize * 2 + wsize * 17), nullptr, wsize * 16);
		CHECK_FALSE(error);
		printLog(__LINE__);
	}
	{
		QspiMock q(500 kB, 1 MB, 123);
		CHECK_FALSE(error);
		q.writeData(0, nullptr, 0);
		CHECK_FALSE(error);
		q.writeData(0, nullptr, wsize);
		CHECK_FALSE(error);
		q.writeData(120 kB - 1 * wsize, nullptr, wsize);
		CHECK_FALSE(error);
		q.writeData(121 kB - 2 * wsize, nullptr, wsize * 2);
		CHECK_FALSE(error);
		q.writeData(122 kB - 1 * wsize, nullptr, wsize * 3);
		CHECK_FALSE(error);
		q.writeData(123 kB, nullptr, wsize * 4);
		CHECK_FALSE(error);
		q.writeData(124 kB, nullptr, wsize * 15);
		CHECK_FALSE(error);
		q.writeData(126 kB, nullptr, wsize * 16);
		CHECK_FALSE(error);
		q.writeData(128 kB, nullptr, wsize * 17);
		CHECK_FALSE(error);
		q.writeData(130 kB + wsize, nullptr, wsize * 9);
		CHECK_FALSE(error);
		q.writeData(131 kB + wsize, nullptr, wsize * 15);
		CHECK_FALSE(error);
		q.writeData(133 kB + wsize, nullptr, wsize * 16);
		CHECK_FALSE(error);
		q.writeData(135 kB + wsize, nullptr, wsize * 17);
		CHECK_FALSE(error);
		q.writeData(137 kB - wsize, nullptr, wsize * 23);
		CHECK_FALSE(error);
		q.writeData(139 kB - wsize, nullptr, wsize * 123);
		CHECK_FALSE(error);
		printLog(__LINE__);
	}
}

TEST_CASE("QspiFlash: erase range of bytes - unaligned")
{
	{
		QspiMock q(500 kB, 1 MB, 123);
		CHECK_FALSE(error);
		q.writeData(0, nullptr, 0);
		CHECK_FALSE(error);
		q.writeData(128, nullptr, 47);
		CHECK_FALSE(error);
		q.writeData(0, nullptr, 47);
		CHECK_FALSE(error);
		q.writeData(2 * wsize - 33, nullptr, 32);
		CHECK_FALSE(error);
		q.writeData(3 * wsize - 33, nullptr, 33);
		CHECK_FALSE(error);
		q.writeData(4 * wsize - 33, nullptr, 34);
		CHECK_FALSE(error);
		q.writeData(5 * wsize - 33, nullptr, 34 + wsize);
		CHECK_FALSE(error);
		q.writeData((1 MB - (3 * wsize + 44)), nullptr, (3 * wsize + 44));
		CHECK_FALSE(error);
		q.writeData((751 kB + 751), nullptr, 243 kB + 33);
		CHECK_FALSE(error);
		q.writeData((wsize * 17 - 0), nullptr, wsize * 14 + 2);
		CHECK_FALSE(error);
		q.writeData((wsize * 18 - 1), nullptr, wsize * 14 + 1);
		CHECK_FALSE(error);
		q.writeData((wsize * 19 + 1), nullptr, wsize * 14 + 2);
		CHECK_FALSE(error);
		q.writeData((esize * 1 + wsize * 17 + 12), nullptr, wsize * 15);
		CHECK_FALSE(error);
		q.writeData((esize * 2 + wsize * 17 - 12), nullptr, wsize * 16);
		CHECK_FALSE(error);
		printLog(__LINE__);
	}
}

TEST_CASE("QspiFlash: write range of bytes - aligned to esize")
{
	{
		QspiMock q(500 kB, 1 MB, 123);
		CHECK_FALSE(error);
		q.writeData(0, random_data, 0);
		CHECK_FALSE(error);
		//q.writeData(0, nullptr, esize);
		//CHECK_FALSE(error);
		q.writeData(0, random_data, esize);
		CHECK_FALSE(error);
		//q.writeData((1 MB - esize), random_data, esize);
		//CHECK_FALSE(error);
		//q.writeData((752 kB), random_data, 240 kB);
		//CHECK_FALSE(error);
		printLog(__LINE__);
	}
}

TEST_CASE("QspiFlash: write range of bytes - aligned to wsize")
{
	{
		QspiMock q(500 kB, 1 MB, 123);
		CHECK_FALSE(error);
		q.writeData(0, random_data, 0);
		CHECK_FALSE(error);
		q.writeData(0, random_data, wsize);
		CHECK_FALSE(error);
		q.writeData((1 MB - wsize), random_data, wsize);
		CHECK_FALSE(error);
		q.writeData((751 kB), random_data, 243 kB);
		CHECK_FALSE(error);
		q.writeData((wsize * 17), random_data, wsize * 14);
		CHECK_FALSE(error);
		q.writeData((esize * 1 + wsize * 17), random_data, wsize * 15);
		CHECK_FALSE(error);
		q.writeData((esize * 2 + wsize * 17), random_data, wsize * 16);
		CHECK_FALSE(error);
		printLog(__LINE__);
	}
	{
		QspiMock q(500 kB, 1 MB, 123);
		CHECK_FALSE(error);
		q.writeData(0, random_data, 0);
		CHECK_FALSE(error);
		q.writeData(0, random_data, wsize);
		CHECK_FALSE(error);
		q.writeData(120 kB - 1 * wsize, random_data, wsize);
		CHECK_FALSE(error);
		q.writeData(121 kB - 2 * wsize, random_data, wsize * 2);
		CHECK_FALSE(error);
		q.writeData(122 kB - 1 * wsize, random_data, wsize * 3);
		CHECK_FALSE(error);
		q.writeData(123 kB, random_data, wsize * 4);
		CHECK_FALSE(error);
		q.writeData(124 kB, random_data, wsize * 15);
		CHECK_FALSE(error);
		q.writeData(126 kB, random_data, wsize * 16);
		CHECK_FALSE(error);
		q.writeData(128 kB, random_data, wsize * 17);
		CHECK_FALSE(error);
		q.writeData(130 kB + wsize, random_data, wsize * 9);
		CHECK_FALSE(error);
		q.writeData(131 kB + wsize, random_data, wsize * 15);
		CHECK_FALSE(error);
		q.writeData(133 kB + wsize, random_data, wsize * 16);
		CHECK_FALSE(error);
		q.writeData(135 kB + wsize, random_data, wsize * 17);
		CHECK_FALSE(error);
		q.writeData(137 kB - wsize, random_data, wsize * 23);
		CHECK_FALSE(error);
		q.writeData(139 kB - wsize, random_data, wsize * 123);
		CHECK_FALSE(error);
		printLog(__LINE__);
	}
}

TEST_CASE("QspiFlash: write range of bytes - unaligned")
{
	{
		QspiMock q(500 kB, 1 MB, 123);
		CHECK_FALSE(error);
		q.writeData(0, random_data, 0);
		CHECK_FALSE(error);
		q.writeData(128, random_data, 47);
		CHECK_FALSE(error);
		q.writeData(0, random_data, 47);
		CHECK_FALSE(error);
		q.writeData(2 * wsize - 33, random_data, 32);
		CHECK_FALSE(error);
		q.writeData(3 * wsize - 33, random_data, 33);
		CHECK_FALSE(error);
		q.writeData(4 * wsize - 33, random_data, 34);
		CHECK_FALSE(error);
		q.writeData(5 * wsize - 33, random_data, 34 + wsize);
		CHECK_FALSE(error);
		q.writeData((1 MB - (3 * wsize + 44)), random_data, (3 * wsize + 44));
		CHECK_FALSE(error);
		q.writeData((751 kB + 751), random_data, 243 kB + 33);
		CHECK_FALSE(error);
		q.writeData((wsize * 17 - 0), random_data, wsize * 14 + 2);
		CHECK_FALSE(error);
		q.writeData((wsize * 18 - 1), random_data, wsize * 14 + 1);
		CHECK_FALSE(error);
		q.writeData((wsize * 19 + 1), random_data, wsize * 14 + 2);
		CHECK_FALSE(error);
		q.writeData((esize * 1 + wsize * 17 + 12), random_data, wsize * 15);
		CHECK_FALSE(error);
		q.writeData((esize * 2 + wsize * 17 - 12), random_data, wsize * 16);
		CHECK_FALSE(error);
		printLog(__LINE__);
	}
}

TEST_CASE("QspiFlash: optimizing partially no need writing")
{
	{
		QspiMock q(500 kB, 1 MB, 0);
		CHECK_FALSE(error);
		CHECK_EQ(log.count(), 0);
		q.writeData(0x1234, &random_data[0x1234], 5000);
		CHECK_FALSE(error);
		printLog(__LINE__);
		CHECK_EQ(log.count(), 1);
	}
}

TEST_CASE("QspiFlash: optimizing partially no need erasing") {}

#if !defined FLASH_PREFERENCES
TEST_CASE("Preferences: ctor" * doctest::skip()) {}

#else
static constexpr uint prefs_size = FLASH_PREFERENCES;

TEST_CASE("Preferences: ctor")
{
	Flash::flash_erase(Flash::flash_size() - prefs_size, prefs_size);
	Preferences p;
	CHECK_EQ(p.free(), prefs_size);
}

TEST_CASE("Preferences")
{
	SUBCASE("set and read back")
	{
		log.purge();
		Preferences p;
		for (uint8 tag = 0; tag < 255; tag++) p.write(tag, uint16(tag * 2));
		for (uint8 tag = 0; tag < 255; tag++) { CHECK_EQ(p.read(tag, uint16(999)), 2 * tag); }
		CHECK_EQ(int(prefs_size) - p.free(), 255 * 4);
		p.sync();
		CHECK_EQ(int(prefs_size) - p.free(), 255 * 4);
		for (uint8 tag = 0; tag < 255; tag++) { CHECK_EQ(p.read(tag, uint16(999)), 2 * tag); }
		printLog(__LINE__);
	}
	SUBCASE("erase tag")
	{
		log.purge();
		Preferences p;
		CHECK_EQ(int(prefs_size) - p.free(), 255 * 4);
		for (uint8 tag = 0; tag < 255; tag++) { CHECK_EQ(p.read(tag, uint16(999)), 2 * tag); }
		for (uint tag = 0; tag < 255; tag += 9) p.remove(uint8(tag));
		for (uint8 tag = 0; tag < 255; tag++) { CHECK_EQ(p.read(tag, uint16(999)), tag % 9 == 0 ? 999 : 2 * tag); }
		p.sync();
		for (uint8 tag = 0; tag < 255; tag++) { CHECK_EQ(p.read(tag, uint16(999)), tag % 9 == 0 ? 999 : 2 * tag); }
		for (uint tag = 0; tag < 255; tag += 9) p.write(uint8(tag), uint16(2 * tag));
		for (uint8 tag = 0; tag < 255; tag++) { CHECK_EQ(p.read(tag, uint16(999)), 2 * tag); }
		for (uint8 tag = 0; tag < 255; tag++) p.remove(tag);
		for (uint8 tag = 0; tag < 255; tag++) { CHECK_EQ(p.read(tag, 99999), 99999); }
		for (uint8 tag = 0; tag < 255; tag++) { CHECK_EQ(p.read(tag, uint16(999)), uint16(999)); }
		p.sync();
		printLog(__LINE__);
	}
	SUBCASE("update tag")
	{
		log.purge();
		Preferences p;
		for (uint tag = 0; tag < 255; tag++) p.write(uint8(tag), tag);
		p.write(66, 666);
		p.write(77, 777);
		p.write(88, 888);
		CHECK_EQ(p.read(66, 0), 666);
		CHECK_EQ(p.read(77, 0), 777);
		CHECK_EQ(p.read(88, 0), 888);
		p.sync();
		CHECK_EQ(p.read(66, 0), 666);
		CHECK_EQ(p.read(77, 0), 777);
		CHECK_EQ(p.read(88, 0), 888);
		printLog(__LINE__);
	}
	SUBCASE("set and get cstring")
	{
		cstr a = "ldhvmariuvaoui ozozuvhm oezvrhoeivum,";
		cstr b = "7631245821374";
		cstr c = "";
		cstr d = nullptr;
		log.purge();
		Preferences p;
		p.write(15, a);
		p.write(16, b);
		p.write(17, c);
		p.write(18, d);
		CHECK_EQ(std::string(p.read(15, "xxx")), std::string(a));
		CHECK_EQ(std::string(p.read(16, "xxx")), std::string(b));
		CHECK_EQ(std::string(p.read(17, "xxx")), std::string(c));
		CHECK_EQ(std::string(p.read(18, "xxx")), std::string(""));
		p.sync();
		CHECK_EQ(std::string(p.read(15, "xxx")), std::string(a));
		CHECK_EQ(std::string(p.read(16, "xxx")), std::string(b));
		CHECK_EQ(std::string(p.read(17, "xxx")), std::string(c));
		CHECK_EQ(std::string(p.read(18, "xxx")), std::string(""));
		p.remove(10);
		//p.dump_store();
		printLog(__LINE__);
		printf("preferences.free = %i\n", p.free());
	}
	SUBCASE("compacting")
	{
		log.purge();
		Preferences p;
		uint		max = (prefs_size / 255 - 2) / 3;
		for (uint n = 0; n <= max; n++)
		{
			for (uint8 tag = 0; tag < 255; tag += 1)
			{
				p.write(tag, 123);
				p.write(tag, 123ul);
				p.write(tag, mulstr(numstr(tag), n)); //
			}
			p.sync();
		}

		for (uint8 tag = 0; tag < 255; tag++)
		{
			CHECK_EQ(std::string(p.read(tag, "")), std::string(mulstr(numstr(tag), max)));
		}

		//p.dump_store();
		printLog(__LINE__);
	}

	SUBCASE("removing")
	{
		log.purge();
		Preferences p;
		for (uint8 tag = 0; tag < 255; tag++) p.remove(tag);
		CHECK_EQ(p.free(), prefs_size);
		p.sync();
		CHECK_EQ(p.free(), prefs_size);

		// removed tags should not be included in the compacted store:
		char bu[] = "93742659234hj3q4875h246246246246246246246246246246ertertwezhww4wrw";
		cptr s	  = bu + sizeof(bu) - min(sizeof(bu), (prefs_size / 255 - 2));
		for (uint8 tag = 0; tag < 255; tag++) p.write(tag, s);
		CHECK_GE(p.free(), 0);
		p.sync();
		CHECK_GE(p.free(), 0);
		for (uint8 tag = 0; tag < 255; tag++) p.remove(tag); // this should be too large
		p.sync();											 // this should trigger compact
		CHECK(startswith(log.last(), "erase "));			 // now the store should be empty, no "removed" entries
		printf("--- there should be no store entries listed below this line ---\n");
		p.dump_store();
		printf("--- there should be no store entries listed above this line ---\n");
		printLog(__LINE__);
	}
}

#endif

} // namespace Test

} // namespace kio


/*





























*/
