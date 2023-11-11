// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "VideoBackend.h"
#include "kilipili_cdefs.h"

namespace kio::Video
{

// A Shape is merely a string of true color pixels
// of which some are interpreted as commands to define how they are placed.

// Softening is done by down scaling the sprite horizontally 2:1.
// => there is no difference in the stored data.
// displaying a softened shape as a normal one will display it in double width.
// displaying a normal shape as a softened one will display it in half width.
// the constructors are different thou.

// Animated Shapes are just the frames stringed together with some added data to the FramePrefix.


enum Animation : bool { NotAnimated = 0, Animated = 1 };
enum Softening : bool { NotSoftened = 0, Softened = 1 };


template<Animation>
struct FramePrefix
{
	uint8 width;
	uint8 height;
	int8  hot_x;
	int8  hot_y;

	static constexpr uint8 duration	  = 0x7f;
	static constexpr int16 next_frame = 0;
	static constexpr uint8 magic	  = 0x7e;
};

template<>
struct FramePrefix<Animated> : public FramePrefix<NotAnimated>
{
	// the additional fields must come first:
	int16 next_frame; // offset to next frame data measured in pixels
	uint8 duration;	  // frames
	uint8 magic;	  // for asserts

	uint8 width;
	uint8 height;
	int8  hot_x;
	int8  hot_y;
};


template<Animation ANIM, Softening SOFT>
struct Shape
{
	const Color* pixels;
	Shape(const Color* c = nullptr) noexcept : pixels(c) {}

	using Preamble = FramePrefix<ANIM>;

	// frame = sequence of rows.
	// each frame starts with a preamble to define the total size and the hot spot for this frame.
	// Each row starts with a HDR and then that number of colors follow.
	// After that there is the HDR of the next row.
	// If the next HDR is a CMD, then handle this CMD as part of the current line:
	// 	 END:  shape is finished, remove it from hotlist.
	// 	 SKIP: resume one more HDR at the current position: used to insert transparent space.

	struct PFX // raw pixels prefix
	{
		int8  dx;	 // initial offset
		uint8 width; // count of pixels that follow
	};

	enum CMD : uint16 { END = 0x0080, GAP = 0x0180 }; // little endian

	bool is_cmd() const noexcept { return reinterpret_cast<const PFX*>(pixels)->dx == -128; }
	bool is_pfx() const noexcept { return reinterpret_cast<const PFX*>(pixels)->dx != -128; }
	bool is_end() const noexcept
	{
		return *reinterpret_cast<const CMD*>(pixels) == END;
	} // TODO alignment if Color=uint8
	bool is_skip() const noexcept { return *reinterpret_cast<const CMD*>(pixels) == GAP; }

	const CMD&		cmd() const noexcept { return *reinterpret_cast<const CMD*>(pixels); }
	const PFX&		pfx() const noexcept { return *reinterpret_cast<const PFX*>(pixels); }
	const Preamble& preamble() const noexcept
	{
		assert(reinterpret_cast<const Preamble*>(pixels)->magic == 0x7e); // 90% if animated, 0% if not
		return *reinterpret_cast<const Preamble*>(pixels);
	}

	Shape					 next_frame() noexcept { return Shape(pixels + preamble().next_frame); }
	uint8					 duration() const noexcept { return preamble().duration; }
	Shape<NotAnimated, SOFT> get_static_part() noexcept; // get current frame as not-animated Shape

	int8  dx() const noexcept { return pfx().dx; }
	uint8 width() const noexcept { return pfx().width; }
	int8  hot_x() const noexcept { return preamble().hot_x; }
	int8  hot_y() const noexcept { return preamble().hot_y; }
	void  skip_cmd() noexcept { pixels += sizeof(CMD) / sizeof(*pixels); }
	void  skip_pfx() noexcept { pixels += sizeof(PFX) / sizeof(*pixels); }
	void  skip_preamble() noexcept { pixels += sizeof(Preamble) / sizeof(*pixels); }
	void  next_row() noexcept;
	bool  render_one_row(int& x, Color* scanline, bool blend) noexcept;
};


//
// ****************************** Implementations ************************************
//

template<Animation ANIM, Softening SOFT>
Shape<NotAnimated, SOFT> Shape<ANIM, SOFT>::get_static_part() noexcept
{
	// skip over .next_frame and .duration:
	const Color* p = pixels + (sizeof(FramePrefix<ANIM>) - sizeof(FramePrefix<NotAnimated>)) / sizeof(Color);
	return Shape<NotAnimated, SOFT>(p);
}

template<Animation ANIM, Softening SOFT>
inline void __attribute__((section(".scratch_x.next_row"))) Shape<ANIM, SOFT>::next_row() noexcept
{
	while (1)
	{
		assert(is_pfx());
		pixels += sizeof(PFX) / sizeof(*pixels) + width();
		if (!is_skip()) break;
		skip_cmd();
	}
}

template<Animation ANIM, Softening SOFT>
inline bool __attribute__((section(".scratch_x.render_one_row")))
Shape<ANIM, SOFT>::render_one_row(int& x, Color* scanline, bool blend) noexcept
{
	// TODO: ghostly

	for (;;)
	{
		assert(is_pfx());

		x += pfx().dx;
		int count = pfx().width;
		skip_pfx();
		const Color* q = pixels;
		pixels		   = q + count;

		int a = x;
		int e = a + count;
		if (a < 0)
		{
			q -= a;
			a = 0;
		}
		if (e > screen_width()) e = screen_width();
		if (!blend)
			while (a < e) scanline[a++] = *q++;
		else
			while (a < e) scanline[a++].blend_with(*q++);

		if (is_pfx()) return false;	 // this is the next line
		if (!is_skip()) return true; // end of shape

		// skip gap and draw more pixels
		skip_cmd();
		x += count;
	}
}


} // namespace kio::Video


/*




































*/
