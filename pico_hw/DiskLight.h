// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once

// you may define this symbol to get a disk activity light:
extern __attribute__((__weak__)) void set_disk_light(bool onoff);

struct DiskLight
{
	inline DiskLight() noexcept { set_disk_light ? set_disk_light(1) : void(0); }
	inline ~DiskLight() noexcept { set_disk_light ? set_disk_light(0) : void(0); }
};
