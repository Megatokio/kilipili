// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Array.h"
#include "standard_types.h"

namespace kio
{

class MockFlash
{
public:
	MockFlash(Array<uint8>& data, Array<cstr>& log, bool& error);
	~MockFlash();

	//uint8& operator[](uint idx) { return data[idx]; }
	//uint32 count() const { return data.count(); }

	Array<uint8>& data;
	Array<cstr>&  log;
	bool&		  error;
};

} // namespace kio
