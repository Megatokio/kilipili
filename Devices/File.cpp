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

} // namespace kio::Devices
