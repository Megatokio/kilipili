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
		VideoPlane(&vblank, &render),
		pixmap(px),
		row_offset(pixmap->row_offset),
		pixels(pixmap->pixmap)
	{}
	FrameBuffer(const Canvas* px, const ColorMap* = nullptr) noexcept : FrameBuffer(static_cast<const Pixmap*>(px))
	{
		assert(px->colormode == CM);
	}

	static void render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void vblank(VideoPlane*) noexcept;
};


/*	_____________________________________________________________________________________
	Explicit specialization for 1-bit indexed color mode:
*/
template<>
class FrameBuffer<Graphics::colormode_i1> final : public VideoPlane
{
public:
	static constexpr Graphics::ColorMode CM = Graphics::colormode_i1;
	using Pixmap							= Graphics::Pixmap<CM>;
	using ColorMap							= Graphics::ColorMap<get_colordepth(CM)>;
	using Canvas							= Graphics::Canvas;

	Id("FrameBuffer");
	RCPtr<const Pixmap>	  pixmap;
	RCPtr<const ColorMap> colormap;
	ScanlineRenderer_i1	  scanline_renderer;
	int					  row_offset;
	uint8*				  pixels;

	FrameBuffer(const Pixmap* px, const ColorMap* cm = nullptr) noexcept :
		VideoPlane(&vblank, &render),
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

	static void render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void vblank(VideoPlane*) noexcept;
};


/*	_____________________________________________________________________________________
	Explicit specialization for 2-bit indexed color mode:
*/
template<>
class FrameBuffer<Graphics::colormode_i2> final : public VideoPlane
{
public:
	static constexpr Graphics::ColorMode CM = Graphics::colormode_i2;
	using Pixmap							= Graphics::Pixmap<CM>;
	using ColorMap							= Graphics::ColorMap<get_colordepth(CM)>;
	using Canvas							= Graphics::Canvas;

	Id("FrameBuffer");
	RCPtr<const Pixmap>	  pixmap;
	RCPtr<const ColorMap> colormap;
	ScanlineRenderer_i2	  scanline_renderer;
	int					  row_offset;
	uint8*				  pixels;

	FrameBuffer(const Pixmap* px, const ColorMap* cm = nullptr) noexcept :
		VideoPlane(&vblank, &render),
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

	static void render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void vblank(VideoPlane*) noexcept;
};


/*	_____________________________________________________________________________________
	Explicit specialization for 4-bit indexed color mode:
*/
template<>
class FrameBuffer<Graphics::colormode_i4> final : public VideoPlane
{
public:
	static constexpr Graphics::ColorMode CM = Graphics::colormode_i4;
	using Pixmap							= Graphics::Pixmap<CM>;
	using ColorMap							= Graphics::ColorMap<get_colordepth(CM)>;
	using Canvas							= Graphics::Canvas;

	Id("FrameBuffer");
	RCPtr<const Pixmap>	  pixmap;
	RCPtr<const ColorMap> colormap;
	ScanlineRenderer_i4	  scanline_renderer;
	int					  row_offset;
	uint8*				  pixels;

	FrameBuffer(const Pixmap* px, const ColorMap* cm = nullptr) noexcept :
		VideoPlane(&vblank, &render),
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

	static void render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void vblank(VideoPlane*) noexcept;
};


/*	_____________________________________________________________________________________
	Explicit specialization for 8-bit indexed color mode:
*/
template<>
class FrameBuffer<Graphics::colormode_i8> final : public VideoPlane
{
public:
	static constexpr Graphics::ColorMode CM = Graphics::colormode_i8;
	using Pixmap							= Graphics::Pixmap<CM>;
	using ColorMap							= Graphics::ColorMap<get_colordepth(CM)>;
	using Canvas							= Graphics::Canvas;

	Id("FrameBuffer");
	RCPtr<const Pixmap>	  pixmap;
	RCPtr<const ColorMap> colormap;
	ScanlineRenderer_i8	  scanline_renderer;
	int					  row_offset;
	uint8*				  pixels;

	FrameBuffer(const Pixmap* px, const ColorMap* cm = nullptr) noexcept :
		VideoPlane(&vblank, &render),
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
	Id("FrameBuffer");
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
template<ColorMode CM>
class FrameBuffer<CM, std::enable_if_t<is_attribute_mode(CM)>> final : public FrameBufferBase_wAttr
{
public:
	using Pixmap   = Graphics::Pixmap<CM>;
	using ColorMap = Graphics::ColorMap<get_colordepth(CM)>;
	using Canvas   = Graphics::Canvas;

	RCPtr<const Pixmap> pixmap;

	FrameBuffer(const Pixmap* px, const ColorMap* = nullptr) noexcept :
		FrameBufferBase_wAttr(
			px->pixmap, px->row_offset, px->attributes.pixmap, px->attributes.row_offset, px->attrheight,
			&ScanlineRenderer<CM>),
		pixmap(px)
	{}

	FrameBuffer(const Canvas* px, const ColorMap* = nullptr) noexcept : FrameBuffer(static_cast<const Pixmap*>(px))
	{
		assert(px->colormode == CM);
	}
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

template<ColorMode CM>
FrameBuffer(Graphics::Pixmap<CM>*) -> FrameBuffer<CM>;

template<ColorMode CM>
FrameBuffer(RCPtr<Graphics::Pixmap<CM>>) -> FrameBuffer<CM>;

} // namespace kio::Video


/*







































*/
