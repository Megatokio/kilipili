// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "RCPtr.h"
#include "geometry.h"

namespace kio::Video
{

/*
	Class VideoPlane is the base class for all VideoPlanes which can be added to the Scanvideo engine.
	The primary method is renderScanline() which is called to create the pixel data for one scanline.
	Method vblank() is called at the start of each frame.
*/
class VideoPlane : public RCObject
{
protected:
	VideoPlane() noexcept {}

public:
	virtual ~VideoPlane() noexcept override = default;

	/*
		Initialize this VideoPlane after construction.
		The VideoPlane can be constructed on core 0, but setup() is always called on core 1.
	*/
	virtual void setup(coord width) = 0;

	/*
		Tear down this VideoPlane before destruction.
		The VideoPlane can be deleted on core 0, but teardown() is always called on core 1.
	*/
	virtual void teardown() noexcept = 0;

	/*
		reset internal counters and addresses for next frame.
		note: this function must go into RAM else no video during flash lock-out!
		this function is always called before a new frame, even before the first one!
	*/
	virtual void vblank() noexcept = 0;

	/*
		render one scanline into the buffer.
		note: this function must go into RAM else no video during flash lock-out!
		@param row    the current row, starting at 0
		@param buffer destination for the pixel data
	*/
	virtual void renderScanline(int row, uint32* buffer) noexcept = 0;
};

using VideoPlanePtr = RCPtr<VideoPlane>;


} // namespace kio::Video
