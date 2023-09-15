// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "cdefs.h"


namespace kipili::Video
{

/*
	Class VBlankAction provides a method to be called during vblank by the Scanvideo engine on core1.
	VBlankActions are added with Scanvideo::addVBlankCallback() which allows ordering of these callbacks.
*/
class VBlankAction
{
protected:
	VBlankAction() noexcept = default;

public:
	virtual ~VBlankAction() noexcept = default;

	/*
		Callback from the Scanvideo engine during vblank on core1.
		note: these functions must go into RAM else no video during flash lock-out!
	*/
	virtual void vblank() noexcept = 0;
};

} // namespace
