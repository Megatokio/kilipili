// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Graphics/geometry.h"
#include "VBlankAction.h"
#include "VideoQueue.h"
#include <new>

namespace kio::Video
{
using coord = Graphics::coord;
using Size	= Graphics::Size;

/*
	Class VideoPlane is the base class for all VideoPlanes which can be added to the Scanvideo engine.
	The primary method is renderScanline() which is called to create the pixel data for one scanline.
	Method vblank() is called at the start of each frame.
*/
class VideoPlane : public VBlankAction
{
protected:
	VideoPlane() noexcept {}

public:
	virtual ~VideoPlane() noexcept override = default;

	/*
		Allocate buffers and initialize buffer with static data, if any, to speed up renderer.
		The default implementation allocates buffers large enough if all pixels are set with CMD::RAW_RUN
	*/
	virtual void setup(uint plane, coord width, VideoQueue& vq)
	{
		uint size = uint(width + 4) / 2; // +4 = CMD::RAW_RUN, count, black, CMD::EOL

		for (uint i = 0; i < vq.SIZE; i++)
		{
			auto& plane_data = vq.buckets[i].data[plane];
			plane_data.data	 = new (std::nothrow) uint32[size];
			if (plane_data.data == nullptr) throw OUT_OF_MEMORY;
			plane_data.used = 0;
			plane_data.max	= uint16(size);
		}
	}

	/*
		Deallocate data buffers.
		The default implementation deallocates the buffers.
	*/
	virtual void teardown(uint plane, VideoQueue& vq) noexcept
	{
		for (uint i = 0; i < vq.SIZE; i++)
		{
			auto& plane_data = vq.buckets[i].data[plane];
			delete[] plane_data.data;
			plane_data.data = nullptr;
			plane_data.used = 0;
			plane_data.max	= 0;
		}
	}

	/*
		reset internal counters and addresses for next frame.
		note: this function must go into RAM else no video during flash lock-out!
		this function is always called before a new frame, even before the first one!
	*/
	virtual void vblank() noexcept override = 0;

	/*
		render one scanline into the buffer.
		note: this function must go into RAM else no video during flash lock-out!
		@param row    the current row, starting at 0
		@param buffer destination for the pixel data
		@return       number of uint32 words actually written
	*/
	virtual uint renderScanline(int row, uint32* buffer) noexcept = 0;
};

} // namespace kio::Video
