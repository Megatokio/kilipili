// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "ColorMap.h"
#include "FrameBuffer.h"
#include "Pixmap.h"
#include "Pixmap_wAttr.h"
#include "internal/ScanlineRenderer.h"


namespace kilipili
{


/*	_____________________________________________________________________________________
	Explicit specialization for true color mode without attributes:
*/
template<>
class FrameBuffer<Pixmap_rgb> final : public VideoPlane
{
public:
	RCPtr<const Pixmap_rgb> pixmap;
	int						row_offset;
	uint8*					pixels; // next position

	FrameBuffer(const Pixmap_rgb* px) noexcept : //
		VideoPlane(&vblank, &render),
		pixmap(px),
		row_offset(pixmap->row_offset),
		pixels(pixmap->pixmap)
	{}
	FrameBuffer(const Canvas* px) noexcept : FrameBuffer(static_cast<const Pixmap_rgb*>(px))
	{
		assert(px->is_colormode_rgb);
	}

	static void render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void vblank(VideoPlane*) noexcept;
};


/*	_____________________________________________________________________________________
	Explicit specialization for 1-bit indexed color mode:
*/
template<>
class FrameBuffer<Pixmap_i1> final : public VideoPlane
{
public:
	RCPtr<const Pixmap_i1>	 pixmap;
	RCPtr<const ColorMap_i1> colormap;
	ScanlineRenderer_i1		 scanline_renderer;
	int						 row_offset;
	uint8*					 pixels;

	FrameBuffer(const Pixmap_i1* px) noexcept :
		VideoPlane(&vblank, &render),
		pixmap(px),
		colormap(px->colormap),
		scanline_renderer(colormap->colors),
		row_offset(px->row_offset),
		pixels(px->pixmap)
	{
		assert(colormap);
	}
	FrameBuffer(const Canvas* px) noexcept : FrameBuffer(static_cast<const Pixmap_i1*>(px))
	{
		assert(px->colormode == colormode_i1);
	}

	static void render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void vblank(VideoPlane*) noexcept;
};


/*	_____________________________________________________________________________________
	Explicit specialization for 2-bit indexed color mode:
*/
template<>
class FrameBuffer<Pixmap_i2> final : public VideoPlane
{
public:
	RCPtr<const Pixmap_i2>	 pixmap;
	RCPtr<const ColorMap_i2> colormap;
	ScanlineRenderer_i2		 scanline_renderer;
	int						 row_offset;
	uint8*					 pixels;

	FrameBuffer(const Pixmap_i2* px) noexcept :
		VideoPlane(&vblank, &render),
		pixmap(px),
		colormap(px->colormap),
		scanline_renderer(colormap->colors),
		row_offset(px->row_offset),
		pixels(px->pixmap)
	{
		assert(colormap);
	}
	FrameBuffer(const Canvas* px) noexcept : FrameBuffer(static_cast<const Pixmap_i2*>(px))
	{
		assert(px->colormode == colormode_i2);
	}

	static void render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void vblank(VideoPlane*) noexcept;
};


/*	_____________________________________________________________________________________
	Explicit specialization for 4-bit indexed color mode:
*/
template<>
class FrameBuffer<Pixmap_i4> final : public VideoPlane
{
public:
	RCPtr<const Pixmap_i4>	 pixmap;
	RCPtr<const ColorMap_i4> colormap;
	ScanlineRenderer_i4		 scanline_renderer;
	int						 row_offset;
	uint8*					 pixels;

	FrameBuffer(const Pixmap_i4* px) noexcept :
		VideoPlane(&vblank, &render),
		pixmap(px),
		colormap(px->colormap),
		scanline_renderer(colormap->colors),
		row_offset(px->row_offset),
		pixels(px->pixmap)
	{
		assert(colormap);
	}
	FrameBuffer(const Canvas* px) noexcept : FrameBuffer(static_cast<const Pixmap_i4*>(px))
	{
		assert(px->colormode == colormode_i4);
	}

	static void render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void vblank(VideoPlane*) noexcept;
};


/*	_____________________________________________________________________________________
	Explicit specialization for 8-bit indexed color mode:
*/
template<>
class FrameBuffer<Pixmap_i8> final : public VideoPlane
{
public:
	RCPtr<const Pixmap_i8>	 pixmap;
	RCPtr<const ColorMap_i8> colormap;
	ScanlineRenderer_i8		 scanline_renderer;
	int						 row_offset;
	uint8*					 pixels;

	FrameBuffer(const Pixmap_i8* px) noexcept :
		VideoPlane(&vblank, &render),
		pixmap(px),
		colormap(px->colormap),
		scanline_renderer(colormap->colors),
		row_offset(px->row_offset),
		pixels(px->pixmap)
	{
		assert(colormap);
	}
	FrameBuffer(const Canvas* px) noexcept : FrameBuffer(static_cast<const Pixmap_i8*>(px))
	{
		assert(px->colormode == colormode_i8);
	}

	static void render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void vblank(VideoPlane*) noexcept;
};


/*	_____________________________________________________________________________________
	Base class for color modes with true color attributes.
*/
class FrameBufferBase_wAttr : public VideoPlane
{
protected:
	using ScanlineRenderFu = void(uint32* dest, uint width, const uint8* pixels, const uint8* attributes) noexcept;

	FrameBufferBase_wAttr(
		const uint8* pixmap, uint row_offset,			  //
		const uint8* attr, uint arow_offset, int aheight, //
		ScanlineRenderFu* fu) noexcept					  //
		:
		VideoPlane(&vblank, &render),
		pixmap(pixmap),
		pixels(pixmap),
		row_offset(row_offset),
		render_fu(fu),
		attrmap(attr),
		attributes(attr),
		arow_offset(arow_offset),
		arow(aheight),
		attrheight(aheight)
	{}

private:
	const uint8*	  pixmap;
	const uint8*	  pixels;
	uint			  row_offset;
	ScanlineRenderFu* render_fu;

	const uint8* attrmap;
	const uint8* attributes;
	uint		 arow_offset;
	int			 arow;
	int			 attrheight;

	static void render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void vblank(VideoPlane*) noexcept;
};


/*	_____________________________________________________________________________________
	Partial specialization for color modes with true color attributes.
*/
template<class Pixmap>
class FrameBuffer<Pixmap, std::enable_if_t<Pixmap::isa_Pixmap_wAttr>> final : public FrameBufferBase_wAttr
{
public:
	RCPtr<const Pixmap> pixmap;

	FrameBuffer(const Pixmap* px) noexcept :
		FrameBufferBase_wAttr(
			px->pixmap, px->row_offset, px->attributes.pixmap, px->attributes.row_offset, px->attrheight,
			&ScanlineRenderer<Pixmap>),
		pixmap(px)
	{}

	FrameBuffer(const Canvas* px) noexcept : FrameBuffer(static_cast<const Pixmap*>(px))
	{
		assert(px->colormode == Pixmap::colormode); // must be same type
	}
};


//	_____________________________________________________________________________________
//  implementation are in the .cpp file:

extern template class FrameBuffer<Pixmap_i1>;
extern template class FrameBuffer<Pixmap_i2>;
extern template class FrameBuffer<Pixmap_i4>;
extern template class FrameBuffer<Pixmap_i8>;
extern template class FrameBuffer<Pixmap_rgb>;


} // namespace kilipili


/*







































*/
