// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "ColorMap.h"
#include "Pixmap.h"
#include "ScanlineRenderFu.h"
#include "VideoPlane.h"


namespace kio::Video
{

/*	_____________________________________________________________________________________
	Base class for FrameBuffers with direct color modes (color modes without attributes):
*/
class FrameBufferBase : public VideoPlane
{
protected:
	using scanlineRenderFu = void(uint32* dest, uint screen_width_pixels, const uint8* video_pixels);

	FrameBufferBase(uint8* pixmap, uint row_offset, scanlineRenderFu* fu) noexcept :
		VideoPlane(&_vblank, &_render),
		pixmap(pixmap),
		row_offset(row_offset),
		render_fu(fu),
		pixels(pixmap)
	{}

private:
	Id("FrameBuffer");
	uint8*			  pixmap;
	uint			  row_offset;
	scanlineRenderFu* render_fu;
	uint8*			  pixels; // next position

	static void _render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void _vblank(VideoPlane*) noexcept;
};


/*	_____________________________________________________________________________________
	Base class for FrameBuffers with color modes with color attributes.
*/
class FrameBufferBase_wAttr : public VideoPlane
{
protected:
	using scanlineRenderFu = void(uint32* dest, uint screen_width_pixels, const uint8* pixels, const uint8* attributes);

	FrameBufferBase_wAttr(
		const uint8* pixmap, uint row_offset,			  //
		const uint8* attr, uint arow_offset, int aheight, //
		scanlineRenderFu* fu) noexcept					  //
		:
		VideoPlane(&_vblank, &_render),
		pixmap(pixmap),
		pixels(pixmap),
		row_offset(row_offset),
		render_fu(fu),
		attrmap(attr),
		attributes(attr),
		arow_offset(arow_offset),
		arow(0),
		attrheight(aheight)
	{}

private:
	Id("FrameBuffer");
	const uint8*	  pixmap;
	const uint8*	  pixels; // next position
	uint			  row_offset;
	scanlineRenderFu* render_fu;

	const uint8* attrmap;
	const uint8* attributes; // next position
	uint		 arow_offset;
	int			 arow;
	int			 attrheight;

	static void _render(VideoPlane*, int row, int width, uint32* scanline) noexcept;
	static void _vblank(VideoPlane*) noexcept;
};


/*	_____________________________________________________________________________________
	Template class FrameBuffer renders whole scanlines from a vga_mode.width Pixmap.
*/
template<ColorMode CM, typename = void>
class FrameBuffer;


/*	_____________________________________________________________________________________
	Partial specialization for direct color modes (color modes without attributes):
*/
template<ColorMode CM>
class FrameBuffer<CM, std::enable_if_t<is_direct_color(CM)>> final : public FrameBufferBase
{
public:
	using Pixmap   = Graphics::Pixmap<CM>;
	using ColorMap = Graphics::ColorMap<get_colordepth(CM)>;
	using Canvas   = Graphics::Canvas;

	RCPtr<const Pixmap>	  pixmap;
	RCPtr<const ColorMap> colormap;

	FrameBuffer(const Pixmap* px, const ColorMap* cm = nullptr) noexcept :
		FrameBufferBase(pixmap->pixmap, pixmap->row_offset, &scanlineRenderFunction<CM>),
		pixmap(px),
		colormap(cm ? cm : &Graphics::system_colormap)
	{
		assert(px->colormode == CM);
	}
	FrameBuffer(const Canvas* px, const ColorMap* cmap = nullptr) noexcept :
		FrameBuffer(static_cast<const Pixmap*>(px), cmap)
	{}

	virtual void setup() throws override { setupScanlineRenderer<CM>(colormap->colors); }
	virtual void teardown() noexcept override { teardownScanlineRenderer<CM>(); }
};


/*	_____________________________________________________________________________________
	Partial specialization for color modes with color attributes.
*/
template<ColorMode CM>
class FrameBuffer<CM, std::enable_if_t<is_attribute_mode(CM)>> final : public FrameBufferBase_wAttr
{
public:
	using Pixmap   = Graphics::Pixmap<CM>;
	using ColorMap = Graphics::ColorMap<get_colordepth(CM)>;
	using Canvas   = Graphics::Canvas;

	RCPtr<const Pixmap>	  pixmap;
	RCPtr<const ColorMap> colormap;

	FrameBuffer(const Pixmap* px, const ColorMap* cm = nullptr) noexcept :
		FrameBufferBase_wAttr(
			px->pixmap, px->row_offset,										  //
			px->attributes.pixmap, px->attributes.row_offset, px->attrheight, //
			&scanlineRenderFunction<CM>),
		pixmap(px),
		colormap(cm ? cm : &Graphics::system_colormap)
	{
		assert(px->colormode == CM);
	}
	FrameBuffer(const Canvas* px, const ColorMap* cm = nullptr) noexcept :
		FrameBuffer(static_cast<const Pixmap*>(px), cm)
	{}

	virtual void setup() throws override { setupScanlineRenderer<CM>(colormap->colors); }
	virtual void teardown() noexcept override { teardownScanlineRenderer<CM>(); }
};


//	_____________________________________________________________________________________
//	deduction guides:

template<ColorMode CM>
FrameBuffer(Graphics::Pixmap<CM>*, const Graphics::ColorMap<get_colordepth(CM)>*) -> FrameBuffer<CM>;

template<ColorMode CM>
FrameBuffer(RCPtr<Graphics::Pixmap<CM>>, const Graphics::ColorMap<get_colordepth(CM)>*) -> FrameBuffer<CM>;

} // namespace kio::Video


/*







































*/
