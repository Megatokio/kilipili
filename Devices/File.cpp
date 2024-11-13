// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "File.h"

namespace kio::Devices
{

uint32 File::ioctl(IoCtl cmd, void*, void*)
{
	// default implementation

	switch (cmd.cmd)
	{
	case IoCtl::CTRL_SYNC: return 0;
	default: throw INVALID_ARGUMENT;
	}
}

int File::getc(__unused uint timeout_us)
{
	SIZE count = read(&last_char, 1, true);
	if (count) return uchar(last_char);
	if (eof_pending()) throw END_OF_FILE;
	else set_eof_pending();
	return -1;
}

char File::getc()
{
	read(&last_char, 1, false);
	return last_char;
}

} // namespace kio::Devices
