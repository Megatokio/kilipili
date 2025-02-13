// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "ColorMap.h"
#include "Pixmap.h"
#include "ScanlineRenderer.h"
#include "VideoPlane.h"


namespace kio::Video
{

/*	_____________________________________________________________________________________
	Template class FrameBuffer renders whole Pixmaps.
*/
template<ColorMode CM, typename = void>
class FrameBuffer;

/*	_____________________________________________________________________________________
	Explicit specialization for true color mode without attributes:
*/
template<>
class FrameBuffer<ColorMode::colormode_rgb> final : public VideoPlane
{
public:
	static const ColorMode CM = Graphics::colormode_rgb;
	using Pixmap			  = Graphics::Pixmap<CM>;
	using ColorMap			  = Graphics::ColorMap<Graphics::colordepth_rgb>;
	using Canvas			  = Graphics::Canvas;

	Id("FrameBuffer");
	RCPtr<const Pixmap> pixmap;
	int					row_offset;
	uint8*				pixels; // next position

	FrameBuffer(const Pixmap* px, const ColorMap* = nullptr) noexcept : //
		VideoPlane(&_vblank, &_render),
		pixmap(px),
		row_offset(pixmap->row_offset),
		pixels(pixmap->pixmap)
	{}
	FrameBuffer(const Canvas* px, const ColorMap* = nullptr) noexcept : FrameBuffer(static_cast<const Pixmap*>(px))
	{
		assert(px->colormode == CM);
	}

	static void _render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void _vblank(VideoPlane*) noexcept;
};


/*	_____________________________________________________________________________________
	Partial specialization for color modes with color attributes.
*/
template<ColorMode CM>
class FrameBuffer<CM, std::enable_if_t<is_attribute_mode(CM)>> final : public VideoPlane
{
public:
	using Pixmap   = Graphics::Pixmap<CM>;
	using ColorMap = Graphics::ColorMap<get_colordepth(CM)>;
	using Canvas   = Graphics::Canvas;

	Id("FrameBuffer");
	RCPtr<const Pixmap> pixmap;
	int					row_offset;
	const uint8*		pixels; // next position
	const uint8*		attrmap;
	const uint8*		attributes; // next position
	uint				arow_offset;
	int					attrheight;
	int					arow;

	FrameBuffer(const Pixmap* px, const ColorMap* = nullptr) noexcept :
		VideoPlane(&_vblank, &_render),
		pixmap(px),
		row_offset(pixmap->row_offset),
		pixels(pixmap->pixmap),
		attrmap(px->attributes.pixmap),
		attributes(attrmap),
		arow_offset(px->attributes.row_offset),
		attrheight(px->attrheight),
		arow(attrheight)
	{}

	FrameBuffer(const Canvas* px, const ColorMap* cm = nullptr) noexcept :
		FrameBuffer(static_cast<const Pixmap*>(px), cm)
	{
		assert(px->colormode == CM);
	}

	static void _render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void _vblank(VideoPlane*) noexcept;
};


/*	_____________________________________________________________________________________
	Partial specialization for indexed color modes:
*/
template<ColorMode CM>
class FrameBuffer<CM, std::enable_if_t<is_indexed_color(CM)>> final : public VideoPlane
{
public:
	using Pixmap   = Graphics::Pixmap<CM>;
	using ColorMap = Graphics::ColorMap<get_colordepth(CM)>;
	using Canvas   = Graphics::Canvas;

	Id("FrameBuffer");
	RCPtr<const Pixmap>	  pixmap;
	RCPtr<const ColorMap> colormap;
	ScanlineRenderer<CM>  scanline_renderer;
	int					  row_offset;
	uint8*				  pixels; // next position

	FrameBuffer(const Pixmap* px, const ColorMap* cm = nullptr) noexcept :
		VideoPlane(&_vblank, &_render),
		pixmap(px),
		colormap(cm ? cm : &Graphics::system_colormap),
		scanline_renderer(colormap->colors),
		row_offset(pixmap->row_offset),
		pixels(pixmap->pixmap)
	{}
	FrameBuffer(const Canvas* px, const ColorMap* cmap = nullptr) noexcept :
		FrameBuffer(static_cast<const Pixmap*>(px), cmap)
	{
		assert(px->colormode == CM);
	}

	static void _render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void _vblank(VideoPlane*) noexcept;
};


//	_____________________________________________________________________________________
//  declare implementation in another file:

extern template class FrameBuffer<ColorMode::colormode_i1>;
extern template class FrameBuffer<ColorMode::colormode_i2>;
extern template class FrameBuffer<ColorMode::colormode_i4>;
extern template class FrameBuffer<ColorMode::colormode_i8>;
extern template class FrameBuffer<ColorMode::colormode_rgb>;
extern template class FrameBuffer<ColorMode::colormode_a1w1>;
extern template class FrameBuffer<ColorMode::colormode_a1w2>;
extern template class FrameBuffer<ColorMode::colormode_a1w4>;
extern template class FrameBuffer<ColorMode::colormode_a1w8>;
extern template class FrameBuffer<ColorMode::colormode_a2w1>;
extern template class FrameBuffer<ColorMode::colormode_a2w2>;
extern template class FrameBuffer<ColorMode::colormode_a2w4>;
extern template class FrameBuffer<ColorMode::colormode_a2w8>;


//	_____________________________________________________________________________________
//	deduction guides:

template<ColorMode CM>
FrameBuffer(Graphics::Pixmap<CM>*, const Graphics::ColorMap<get_colordepth(CM)>*) -> FrameBuffer<CM>;

template<ColorMode CM>
FrameBuffer(RCPtr<Graphics::Pixmap<CM>>, const Graphics::ColorMap<get_colordepth(CM)>*) -> FrameBuffer<CM>;

} // namespace kio::Video


/*







































*/
