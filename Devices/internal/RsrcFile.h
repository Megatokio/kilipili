// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "File.h"


namespace kio::Devices
{

/*
	class RsrcFile reads from data in Flash or Rom.
	compressed files are wrapped in a HeatShrinkDecoder by the RsrcFS,
	so you always get the uncompressed data.
*/
class RsrcFile : public File
{
public:
	~RsrcFile() noexcept override = default;

	virtual ADDR getSize() const noexcept override { return fsize; }
	virtual ADDR getFpos() const noexcept override { return fpos; }
	virtual void setFpos(ADDR) override;
	virtual SIZE read(void* data, SIZE, bool partial = false) override;
	virtual void close() override {}

private:
	const uint8* data  = nullptr;
	uint32		 fsize = 0;
	uint32		 fpos  = 0;

	RsrcFile(const uchar* data, uint32 size);
	friend class RsrcFS;
};

} // namespace kio::Devices
