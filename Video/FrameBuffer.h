// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "ColorMap.h"
#include "Pixmap.h"
#include "ScanlineRenderFu.h"
#include "VideoPlane.h"


#define WRAP(X)	 #X
#define XWRAP(X) WRAP(X)
#define XRAM	 __attribute__((section(".scratch_x." XWRAP(__LINE__))))	 // the 4k page with the core1 stack
#define RAM		 __attribute__((section(".time_critical." XWRAP(__LINE__)))) // general ram


namespace kio::Video
{

/*
	Template class FrameBuffer renders a whole scanline from a screen_width Pixmap.
*/
template<ColorMode CM, typename = void>
class FrameBuffer;


/*
	Partial specialization for direct color modes (color modes without attributes):
*/
template<ColorMode CM>
class FrameBuffer<CM, std::enable_if_t<is_direct_color(CM)>> final : public VideoPlane
{
public:
	Id("FrameBuffer");

	using Pixmap   = Graphics::Pixmap<CM>;
	using ColorMap = Graphics::ColorMap<get_colordepth(CM)>;
	using Canvas   = Graphics::Canvas;

	RCPtr<Pixmap>	   pixmap;
	const Color* const colormap;
	uint8*			   pixels; // next position
	int				   row;	   // expected next row
	uint			   width;

	FrameBuffer(Pixmap* px, const ColorMap cm) noexcept : FrameBuffer(RCPtr<Pixmap>(px), cm) {}
	FrameBuffer(RCPtr<Pixmap> px, const ColorMap colormap) noexcept : pixmap(px), colormap(colormap)
	{
		assert(is_true_color(CM) || colormap != nullptr);
	}
	FrameBuffer(RCPtr<Canvas> px, const ColorMap colormap) noexcept :
		pixmap(static_cast<Pixmap*>(px.ptr())),
		colormap(colormap)
	{
		assert(px->colormode == CM);
		assert(is_true_color(CM) || colormap != nullptr);
	}

	virtual ~FrameBuffer() noexcept override = default;

	virtual void setup(coord width) throws override
	{
		this->width = width;
		setupScanlineRenderer<CM>(colormap); // setup render function
		FrameBuffer::vblank();				 // reset state variables
	}

	virtual void teardown() noexcept override
	{
		teardownScanlineRenderer<CM>(); //
	}

	virtual void RAM renderScanline(int row, uint32* plane_data) noexcept override final
	{
		while (unlikely(++this->row <= row)) // increment row and check whether we missed some rows
		{
			pixels += pixmap->row_offset;
		}

		scanlineRenderFunction<CM>(plane_data, width, pixels);

		pixels += pixmap->row_offset;
	}

	virtual void RAM vblank() noexcept override final
	{
		pixels = pixmap->pixmap;
		row	   = 0;
	}
};


/*
	Partial specialization for color modes with color attributes.
	Somehow i can't avoid code duplication here.
	Partial specialization is meant to drive people crazy.
*/
template<ColorMode CM>
class FrameBuffer<CM, std::enable_if_t<is_attribute_mode(CM)>> final : public VideoPlane
{
public:
	using Pixmap   = Graphics::Pixmap<CM>;
	using ColorMap = Graphics::ColorMap<get_colordepth(CM)>;
	using Canvas   = Graphics::Canvas;

	RCPtr<Pixmap>	   pixmap;
	const Color* const colormap;
	uint8*			   pixels; // next position
	int				   row;	   // expected next row
	uint8*			   attributes;
	int				   arow;
	uint			   width;
	int				   row_offset;
	int				   attrheight;

	FrameBuffer(Pixmap* px, const ColorMap cm) noexcept : FrameBuffer(RCPtr<Pixmap>(px), cm) {}
	FrameBuffer(RCPtr<Pixmap> px, const ColorMap cm) noexcept :
		pixmap(px),
		colormap(cm),
		row_offset(px->row_offset),
		attrheight(px->attrheight)
	{
		assert(is_true_color(CM) || colormap != nullptr);
	}
	FrameBuffer(RCPtr<Canvas> px, const ColorMap colormap) noexcept :
		pixmap(static_cast<Pixmap*>(px.ptr())),
		colormap(colormap),
		row_offset(pixmap->row_offset),
		attrheight(px->attrheight)
	{
		assert(px->colormode == CM);
		assert(is_true_color(CM) || colormap != nullptr);
	}

	virtual ~FrameBuffer() noexcept override = default;

	virtual void setup(coord width) throws override
	{
		this->width = width;
		setupScanlineRenderer<CM>(colormap); // setup render function
		FrameBuffer::vblank();				 // reset state variables
	}

	virtual void teardown() noexcept override
	{
		teardownScanlineRenderer<CM>(); //
	}

	virtual void XRAM renderScanline(int row, uint32* plane_data) noexcept override final
	{
		while (unlikely(++this->row <= row)) // increment row and check whether we missed some rows
		{
			pixels += pixmap->row_offset;
			if unlikely (++arow == pixmap->attrheight)
			{
				arow = 0;
				attributes += pixmap->attributes.row_offset;
			}
		}

		scanlineRenderFunction<CM>(plane_data, width, pixels, attributes);

		pixels += row_offset;
		if unlikely (++arow == attrheight)
		{
			arow = 0;
			attributes += pixmap->attributes.row_offset;
		}
	}

	virtual void RAM vblank() noexcept override final
	{
		pixels	   = pixmap->pixmap;
		attributes = pixmap->attributes.pixmap;
		row		   = 0;
		arow	   = 0;
	}
};

// deduction guides:

template<ColorMode CM>
FrameBuffer(Graphics::Pixmap<CM>*, const Graphics::ColorMap<get_colordepth(CM)>) -> FrameBuffer<CM>;

template<ColorMode CM>
FrameBuffer(RCPtr<Graphics::Pixmap<CM>>, const Graphics::ColorMap<get_colordepth(CM)>) -> FrameBuffer<CM>;

} // namespace kio::Video
