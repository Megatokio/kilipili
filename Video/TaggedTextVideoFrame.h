// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Graphics/Color.h"
#include "VideoPlane.h"
#include "common/no_copy_move.h"
#include "common/standard_types.h"

namespace kilipili::Video
{

/*	class TaggedCharMap provides a char array
	intended for use as a text based frame buffer, dialog box or GUI menu.

	The array contains one byte per char.
	The class provides 2x4 colors colors.
	Characters may have any code except 0x00 .. 0x0F (0x1F if underline is used).

	Codes 0x00 .. 0x1F print a space and change colors and attributes:
	0x00 + 1:	set background to color[0..1]
	0x00 + 2,4:	set foreground to color[0..3]
	0x00 + 8:	enable bold characters
	0x00 + 16	enable underline (optional).

	0x00 and 0x03 would make text invisible (bg color = fg color).
	they bump the bgcolor to 2 or 3 instead.
	0x00 is used as line end / string delimiter and cannot be used within the text,
	but it can be used as initial attribute: 0x20 + 0x00.

	Switching an attribute ON takes place after the space.
	Switching an attribute OFF takes place before the space.
	(this is only makes a difference for bgcolor and underline.)

	all lines start with an initial attribute and end with char 0x00.
	upper bits in the initial attribute are ignored.

	The charmap can be collapsed:

	collapsed=false: all lines are cols+2 bytes long.
	collapsed=true: lines are truncated after char 0x00
		and immediately start with the next line.
		end of text must be marked with 2x char 0x00.
		collapsed charmaps may be const (in flash).

	first_glyph can be set to 0x10 or 0x20:
	first_glyph=0x10: underline is not available,
		but char 0x10 to 0x1F can be displayed: the attn/warning/error signs.
	first_glyph=0x20: underline is available.
*/

class TaggedTextVideoFrame : public VideoPlane
{
public:
	TaggedTextVideoFrame(int rows, int cols);
	TaggedTextVideoFrame(int rows, int cols, const char* collapsed_text, bool make_copy = false);
	~TaggedTextVideoFrame() override;
	NO_COPY_MOVE(TaggedTextVideoFrame);

	void vblank() noexcept override;
	void renderScanline(int row, int width, uint32* buffer) noexcept override;

	static constexpr char make_attr(int bg, int fg, bool b = 0, bool u = 0) noexcept
	{
		return char((bg & 1) << 0 | (fg & 3) << 1 | b << 3 | u << 4);
	}

	static constexpr int   default_attr = 0x02; // white paper, black text, normal
	static constexpr uchar first_glyph	= 0x10; // 0x20 if attr underline is used

	uint8 drop_shadow  = 0;		// 0 or 1 .. 4
	bool  is_collapsed = false; // lines truncated at char0
	bool  is_const	   = false; // charmap is const (in flash) & we don't own it!
	char  _padding;

	uint16 rows, cols; // excl. attr0 and char0

	char* charmap = nullptr; // const if in flash

	Graphics::Color bgcolors[4] {Graphics::white, Graphics::black, Graphics::light_yellow, Graphics::light_grey};
	Graphics::Color fgcolors[4] {Graphics::white, Graphics::black, Graphics::yellow, Graphics::red};

	void collapse(bool = true); // collapse or expand existing charmap
	void expand() { collapse(false); }
	void clear() noexcept;
	void clearRect(int row, int col, int rows, int cols) noexcept;
	void copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept;
	void scrollScreen(int dy = 1) noexcept; // +1 = scroll up & clear line at bottom of screen
	void clearLine(int row) noexcept;
};

} // namespace kilipili::Video


/*





















*/
