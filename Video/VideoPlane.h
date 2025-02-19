// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "RCPtr.h"
#include "standard_types.h"

#define XRAM __attribute__((section(".scratch_x.VP" __XSTRING(__LINE__))))	   // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.VP" __XSTRING(__LINE__)))) // general ram


namespace kio::Video
{

/*
	Class VideoPlane is the base class for all VideoPlanes which can be added to the VideoController.
	The primary method is renderScanline() which is called to create the pixel data for one scanline.
	Method vblank() is called at the start of each frame.

	NOTE: during flash lockout (when writing to the internal flash) the virtual `vblank()` and
	`renderScanline()` are not called.

	If a subclass want's to render it's content during flash lockout then 
	if must provide the static functions `vblank_fu` and `render_fu`.
	Note that templates probably don't work! The C++ compiler will put some code in rom anyway!
	This is probably only needed for full screen VideoPlanes.
	Take a look at classes FrameBuffer (expert) and UniColorBackdrop (easy).

	For most other VideoPlanes it may be acceptable that they are not visible during flash lockout.
	Possibly you don't care anyway because you rarely write to flash, if ever.
	In this case `renderScanline()` cannot rely on `vblank()` to reset counters and pointers.
	Mixing a static and a virtual function is possible, if it helps.
*/
class VideoPlane : public RCObject
{
public:
	Id("VideoPlane");

	virtual ~VideoPlane() noexcept override = default;

	/*
		reset internal counters and addresses for next frame.
		called at the start of each frame; except during flash lockout!
	*/
	virtual void vblank() noexcept {}

	/*
		render one scanline into the buffer.
		called for each scanline; except during flash lockout!
		this function should go into RAM!
		@param row    the current row, starting at 0
		@param width  number of pixels to draw
		@param buffer destination for the pixel data
	*/
	virtual void renderScanline(int __unused row, int __unused width, uint32* __unused buffer) noexcept {}

	using VblankFu = void(VideoPlane*) noexcept;
	using RenderFu = void(VideoPlane*, int row, int width, uint32* buffer) noexcept;

	VblankFu* vblank_fu = nullptr;
	RenderFu* render_fu = nullptr;

protected:
	// default vblank and render functions:
	// these call the virtual vblank() and render() functions unless the flash is locked out.
	static void do_vblank(VideoPlane*) noexcept;
	static void do_render(VideoPlane*, int row, int width, uint32* buffer) noexcept;

	VideoPlane() noexcept : vblank_fu(&do_vblank), render_fu(&do_render) {}
	VideoPlane(VblankFu* a, RenderFu* b) noexcept : vblank_fu(a), render_fu(b) {}
};

using VideoPlanePtr = RCPtr<VideoPlane>;


} // namespace kio::Video

#undef RAM
#undef XRAM
