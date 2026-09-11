/*
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: bit.h,v 1.12 2004/01/23 09:41:32 rob Exp $
 *
 *
 * c++ adaption:
 * Copyright (c) 2026 - 2026 kio@little-bat.de
 * GPL-2.0 license
 * https://opensource.org/license/gpl-2.0
 */

#pragma once
#include "common/standard_types.h"

struct mad_bitptr
{
	const uchar* byte;
	ushort		 cache;
	ushort		 left;

	void   init(const uchar*) noexcept;
	void   finish() noexcept {}
	ushort bitsleft() noexcept { return left; }

	// return pointer to next unprocessed byte:
	const uchar* nextbyte() noexcept { return left == CHAR_BIT ? byte : byte + 1; }

	void   skip(uint) noexcept;
	ulong  read(uint) noexcept;
	ushort crc(uint, ushort) noexcept;
};

uint mad_bit_length(const mad_bitptr*, const mad_bitptr*) noexcept;
