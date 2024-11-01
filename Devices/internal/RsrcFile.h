// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "File.h"
#include "extern/heatshrink/heatshrink_decoder.h"


namespace kio::Devices
{

/*
	class RomFile reads from data in Rom (Flash).
*/
class RsrcFile : public File
{
public:
	~RsrcFile() noexcept override = default;

	virtual ADDR getSize() const noexcept override { return fsize; }
	virtual ADDR getFpos() const noexcept override { return fpos; }
	virtual void setFpos(ADDR) override;
	virtual SIZE read(void* data, SIZE, bool partial = false) override;
	virtual void close(bool) override {}

private:
	const uint8* data  = nullptr;
	uint32		 fsize = 0;
	uint32		 fpos  = 0;

	RsrcFile(const uchar* data, uint32 size);
	friend class RsrcFS;
};

class CompressedRomFile : public File
{
public:
	~CompressedRomFile() noexcept override { close(); }

	virtual ADDR getSize() const noexcept override { return usize; }
	virtual ADDR getFpos() const noexcept override { return fpos; }
	virtual void setFpos(ADDR) override;
	virtual void close(bool force = true) override;
	virtual SIZE read(void* data, SIZE, bool partial = false) override;

private:
	const uint8*		cdata	= nullptr;
	heatshrink_decoder* decoder = nullptr;

	uint32 usize = 0;
	uint32 csize = 0;
	uint32 fpos	 = 0;
	uint32 cpos	 = 0;

	CompressedRomFile(const uchar* cdata, uint32 usize, uint32 csize, uint8 parameters);
	friend class RsrcFS;
};


} // namespace kio::Devices
