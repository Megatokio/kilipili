// Copyright (c) 2012 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Canvas.h"
#include "Font.h"
#include "Printer.h"
#undef CHAR_WIDTH

namespace kilipili::Graphics
{

template<>
class Printer<Canvas> final : public Printer<>
{
public:
	static constexpr int CHAR_WIDTH = 8;

	Printer(Canvas*, const Font* font1 = &latin1_12x8, const Font* font2 = &graphics_12x8) noexcept;

	void reset(bool cls = false) noexcept override;
	void printChar(char c, int count = 1) noexcept override; // no ctl
	void print(cstr text) noexcept override;				 // supports \n and \t
	void clearRect(int row, int col, int rows, int cols) noexcept override;
	void copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept override;
	cstr identify() override;
	void setAttributes(uint add, uint remove = 0xff) noexcept override;


	RCPtr<Canvas>	  pixmap;
	RCPtr<const Font> font1; // default: latin-1
	RCPtr<const Font> font2; // default: graphics font

	const ColorMode	 colormode;
	const AttrHeight attrheight;
	const ColorDepth colordepth;	 // 0 .. 4  log2 of bits per color in attributes[]
	const AttrMode	 attrmode;		 // 0 .. 2  log2 of bits per color in pixmap[]
	const AttrWidth	 attrwidth;		 // 0 .. 3  log2 of width of tiles
	const uint8		 bits_per_color; // bits per color in pixmap[] or attributes[]
	const uint8		 bits_per_pixel; // bpp in pixmap[]
	char			 _padding = 0;

	uint bgcolor; // paper color
	uint fgcolor; // text color
	uint fg_ink;  // for Pixmap_wAttr
	uint bg_ink;  // for Pixmap_wAttr

	uint default_bgcolor = 0x0000ffff; // white paper (note: ic1 has inverted default colormap)
	uint default_fgcolor = 0;		   // black ink   (      => black paper & green ink)

	// print attributes:
	enum Attributes : uint8 {
		normal		  = 0,
		bold		  = 1 << 0,
		underline	  = 1 << 1,
		inverted	  = 1 << 2,
		italic		  = 1 << 3,
		transparent	  = 1 << 4,
		double_width  = 1 << 5,
		double_height = 1 << 6,
		graphics	  = 1 << 7
	};

	// cursor blob:
	uint32 cursorXorColor; // value used to xor the colors

private:
	using CharMatrix = uint8[24];
	void show_cursor(bool f) noexcept override;
	void printCharMatrix(CharMatrix, int count = 1) noexcept;
	void readBmp(CharMatrix, bool use_fgcolor) noexcept;
	void writeBmp(CharMatrix, uint8 attr) noexcept;
	void getCharMatrix(CharMatrix, char c) noexcept;
	void applyAttributes(CharMatrix) noexcept;
};


// _____________________________________________________________________
// deduction guides:

Printer(Canvas*) -> Printer<Canvas>;
Printer(RCPtr<Canvas>) -> Printer<Canvas>;


} // namespace kilipili::Graphics


/*

















*/
