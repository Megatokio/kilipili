// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FrameBuffer.h"
#include "composable_scanline.h"

namespace kio::Video
{

void framebuffer_setup_helper(coord width, VideoQueue& vq)
{
	for (uint i = 0; i < vq.SIZE; i++) // initialize buffer contents to save time in render loop
	{
		auto& plane_data = vq.buckets[i];

		int size = (width + 4) / 2; // +4 = CMD, count, black, EOL;  /2 = uint16 -> uint32
		assert_ge(plane_data.max, size);
		plane_data.used = uint16(size);

		uint16* p = uint16ptr(plane_data.data);
		p[0]	  = CMD::RAW_RUN; // cmd
		//p[1]     = p[2];				// 1st pixel
		//p[2]     = uint16(width-3+1);	// count-3 +1 for final black pixel
		//p[3++]   = pixels
		p[width + 2] = 0;		 // final black pixel (actually transparent)
		p[width + 3] = CMD::EOL; // end of line; total count of uint16 values must be even!
	}
}

} // namespace kio::Video
