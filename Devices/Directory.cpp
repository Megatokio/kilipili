// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Directory.h"
#include "cstrings.h"

namespace kio::Devices
{

Directory::Directory(RCPtr<FileSystem> fs, cstr full_path) : fs(std::move(fs)), dirpath(nullptr)
{
	assert(full_path);
	assert(kio::find(full_path, ":/"));
	this->dirpath = newcopy(full_path);
}

} // namespace kio::Devices
