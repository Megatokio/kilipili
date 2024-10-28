// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "File.h"

/*
	template class RamFile provides files in ram.
	provides the File and the SerialDevice interface.
	default sector size is 1<<9 = 512 bytes.
*/

namespace kio::Devices
{

template<uint ss = 9>
class RamFile final : public File
{
	NO_COPY_MOVE(RamFile);

	static constexpr uint sector_size = 1 << ss;
	static constexpr uint sector_mask = sector_size - 1;

	struct Sector
	{
		Sector* next = nullptr;
		char	data[sector_size];
	};

	Sector	sector0;
	Sector* sector = &sector0; // current sector
	uint	sn	   = 0;		   // sector number of current sector
	uint	fpos   = 0;
	uint	fsize  = 0;

public:
	RamFile() : File(readwrite) {}

	virtual ~RamFile() override
	{
		while (Sector* s = sector0.next)
		{
			sector0.next = s->next;
			delete s;
		}
	}

	Sector* get_sector(uint n)
	{
		if unlikely (n < sn)
		{
			sn	   = 0;
			sector = &sector0;
		}

		while (sn < n)
		{
			if unlikely (sector->next == nullptr) sector->next = new Sector;
			sector = sector->next;
			sn++;
		}

		return sector;
	}

	virtual SIZE read(void* _data, SIZE size, bool partial = false) override
	{
		if (size > fsize - fpos)
		{
			size = fsize - fpos;
			if (!partial) throw END_OF_FILE;
		}

		ptr	 data = reinterpret_cast<ptr>(_data);
		uint rem  = size;
		while (rem)
		{
			uint offs = fpos & sector_mask;
			uint cnt  = std::min(rem, sector_size - offs);
			memcpy(data, get_sector(fpos >> ss)->data + offs, cnt);
			data += cnt;
			fpos += cnt;
			rem -= cnt;
		}

		return size;
	}

	virtual SIZE write(const void* _data, SIZE size, bool = false) override
	{
		cptr data = reinterpret_cast<cptr>(_data);
		uint rem  = size;
		while (rem)
		{
			uint offs = fpos & sector_mask;
			uint cnt  = std::min(rem, sector_size - offs);
			memcpy(get_sector(fpos >> ss)->data + offs, data, cnt);
			data += cnt;
			fpos += cnt;
			fsize = std::max(fsize, fpos);
			rem -= cnt;
		}

		return size;
	}

	virtual ADDR getSize() const noexcept override { return fsize; }
	virtual ADDR getFpos() const noexcept override { return fpos; }
	virtual void setFpos(ADDR a) override { fpos = std::min(uint(a), fsize); }
	virtual void close(bool = true) override {}
	virtual void truncate() override
	{
		fsize = fpos;
		while (Sector* s = sector->next)
		{
			sector->next = s->next;
			delete s;
		}
	}
};

} // namespace kio::Devices

/*
































*/
