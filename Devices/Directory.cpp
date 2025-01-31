// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Directory.h"
#include "cstrings.h"

namespace kio::Devices
{

static bool is_absolute_path(cstr path) { return path[0] == '/'; }

Directory::Directory(RCPtr<FileSystem> fs, cstr path) : fs(std::move(fs)), path(nullptr)
{
	assert(path);
	assert(is_absolute_path(path));
	this->path = newcopy(path);
}

cstr Directory::makeAbsolutePath(cstr path)
{
	assert(path);
	if (is_absolute_path(path)) return path;
	else return catstr(this->path, "/", path);
}

} // namespace kio::Devices
