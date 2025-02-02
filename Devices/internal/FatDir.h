// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Directory.h"
#include "ff15/source/ff.h"

namespace kio ::Devices
{

class FatFS;

class FatDir final : public Directory
{
public:
	virtual ~FatDir() noexcept override;

	virtual FileInfo next(cstr pattern = nullptr) throws override;
	virtual void	 rewind() throws override;

private:
	DIR fatdir;

	FatDir(RCPtr<FileSystem>, cstr path) throws;
	friend class FatFS;
};

} // namespace kio::Devices
