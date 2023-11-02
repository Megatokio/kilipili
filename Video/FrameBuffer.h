// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Graphics/ColorMap.h"
#include "Graphics/Pixmap.h"
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
	using Pixmap   = Graphics::Pixmap<CM>;
	using ColorMap = Graphics::ColorMap<get_colordepth(CM)>;

	const Pixmap&	   pixmap;
	const Color* const colormap;
	uint8*			   pixels; // next position
	int				   row;	   // expected next row

	FrameBuffer(const Pixmap& px, const ColorMap colormap) noexcept : pixmap(px), colormap(colormap) {}

	virtual ~FrameBuffer() noexcept override = default;

	virtual void setup(coord /*width*/) throws override
	{
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
			pixels += pixmap.row_offset;
		}

		uint width = uint(pixmap.width);
		scanlineRenderFunction<CM>(plane_data, width, pixels);

		pixels += pixmap.row_offset;
	}

	virtual void RAM vblank() noexcept override final
	{
		pixels = pixmap.pixmap;
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

	const Pixmap&	   pixmap;
	const Color* const colormap;
	uint8*			   pixels; // next position
	int				   row;	   // expected next row
	uint8*			   attributes;
	int				   arow;

	FrameBuffer(const Pixmap& px, const ColorMap cm) noexcept : pixmap(px), colormap(cm) {}

	virtual ~FrameBuffer() noexcept override = default;

	virtual void setup(coord /*width*/) throws override
	{
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
			pixels += pixmap.row_offset;
			if (unlikely(++arow == pixmap.attrheight))
			{
				arow = 0;
				attributes += pixmap.attributes.row_offset;
			}
		}

		uint width = uint(pixmap.width);
		scanlineRenderFunction<CM>(plane_data, width, pixels, attributes);

		pixels += pixmap.row_offset;
		if (unlikely(++arow == pixmap.attrheight))
		{
			arow = 0;
			attributes += pixmap.attributes.row_offset;
		}
	}

	virtual void RAM vblank() noexcept override final
	{
		pixels	   = pixmap.pixmap;
		attributes = pixmap.attributes.pixmap;
		row		   = 0;
		arow	   = 0;
	}
};

} // namespace kio::Video
