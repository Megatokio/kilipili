// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Graphics/Canvas.h"
#include "Graphics/Mock/TextVDU.h"
#include "Video/Color.h"
#include "common/Queue.h"
#include "common/RCPtr.h"
#include "common/geometry.h"
#include <stdarg.h>

/**	ANSI / ECMA-48 wannabe-compliant terminal.
	reads input via USB::getKeyEvent() and USB::getMouseEvent().
	writes output via TextVDU to the Pixmap which was provided in the constructor.

	to create a serial terminal copy stdin to putc() and getc() to stdout.

	class AnsiTerm does not
	 - create a VideoPlane from the Pixmap or start the VideoController
	 - change the USB::HidKeyTable used in USB::getKeyEvent()

	After construction you can
	 - change the 'default_' values and reset(hard)
	 - set import_char() and export_char() ¹

	¹) If you plan to use a 8-bit character set which is not latin-8, then you must:
	 - modify the default font in Graphics/rsrc/
	 - provide a national USB::HidKeyTable (keyboard map) which maps your keyboard to this font
	 - If you communicate with an external device, this must either use the same 8-bit font,
	   or you must use utf-8 encoding for communication and provide import_char() and export_char()
	   to convert your 8-bit characters from and to Unicode for utf-8 encoded transmission.
*/

constexpr char CTRL_ALT_DEL_PRESSED[] = "ctrl-alt-del pressed";


namespace kio::Graphics
{

class AnsiTerm final : public RCObject
{
public:
	using Color		= Video::Color;
	using CanvasPtr = Graphics::CanvasPtr;
#ifdef UNIT_TEST
	using TextVDU = Graphics::Mock::TextVDU;
#else
	using TextVDU = Graphics::TextVDU;
#endif

	using ImportChar	   = char(uint) noexcept;
	using ExportChar	   = uint(char) noexcept;
	using RunStateMachines = void() noexcept;

	AnsiTerm(CanvasPtr, Color*);

	void reset(bool hard = false) noexcept;
	int	 getc();					  // non blocking
	uint read(char* text, uint size); // non blocking
	void putc(char);
	uint puts(cstr);
	uint printf(cstr, va_list) __printflike(2, 0);
	uint printf(cstr, ...) __printflike(2, 3);
	uint write(const char* text, uint size);
	void flush_out() noexcept { wstate = NothingPending; }
	void flush_in() noexcept { inputbuffer.flush(); }

	// convert between local charset and utf8:
	ImportChar* import_char;
	ExportChar* export_char;

	CanvasPtr	   full_pixmap;
	Color*		   colormap;
	RCPtr<TextVDU> display;

	// settings:
	bool default_auto_wrap		  : 1 = false; // CSI ? 7 h
	bool default_application_mode : 1 = false; // CSI ? 1 h  or  ESC =
	bool default_utf8_mode		  : 1 = false; // ESC % G
	bool default_c1_codes_8bit	  : 1 = false; // ESC SPC G
	bool default_newline_mode	  : 1 = false; // LFNL: CSI 20 h
	bool default_local_echo		  : 1 = false; // SRM:	CSI 12 l
	bool default_sgr_cumulative	  : 1 = false; // GRCM:	CSI 21 h
	bool _padding2				  : 1 = false;

#ifdef RELEASE
	static constexpr bool debug		= false;
	bool				  _padding4 = false;
#else
	bool debug	  = true; // log unknown/bogus control codes
#endif
	bool dump_control_chars	 = false;
	bool detect_ctrl_alt_del = false; // true => throw CTRL_ALT_DEL_PRESSED

	// SETTINGS & state:
	bool  utf8_mode			  = false;
	bool  c1_codes_8bit		  = false;
	bool  application_mode	  = false;
	bool  local_echo		  = false;
	bool  newline_mode		  = false;
	bool  sgr_cumulative	  = false;
	bool  auto_wrap			  = false;
	bool  lr_ever_set_by_csis = false;
	uchar htabs[160 / 8];

	Rect mouse_rect				   = {0, 0, 0, 0};
	bool mouse_enabled		   : 1 = false;
	bool mouse_enabled_once	   : 1 = false;
	bool mouse_report_pixels   : 1 = false; // else characters
	bool mouse_report_btn_down : 1 = false;
	bool mouse_report_btn_up   : 1 = false;
	bool mouse_enable_rect	   : 1 = false;
	bool _padding3			   : 2 = 0;

	// state saved with pushCursor():
	bool  insert_mode		 = false; // CSI 4 h
	bool  cursor_visible	 = true;  // CSI ? 25 h
	bool  lr_margins_enabled = false; // CSI ? 69 h
	bool  tb_margins_enabled = false; // CSI ? 6 h
	bool  lr_set_by_csir	 = false; //
	uint8 top_margin		 = 0;	  // CSI <top> ; <bottom> r
	uint8 bottom_margin		 = 0;	  //
	uint8 left_margin		 = 0;	  // CSI <left> ; <right> s
	uint8 right_margin		 = 0;	  //

private:
	struct SavedState
	{
		RCPtr<TextVDU> display;

		bool  insert_mode		 : 1;
		bool  cursor_visible	 : 1;
		bool  lr_margins_enabled : 1;
		bool  tb_margins_enabled : 1;
		bool  lr_set_by_csir	 : 1;
		bool  _padding1			 : 3;
		uint8 top_margin;
		uint8 bottom_margin;
		uint8 left_margin;
		uint8 right_margin;
		int8  _padding[3];
	};

	// cursor stack
	uint16	   sp = 0;
	SavedState stack[2];

	// buffer for control code and arguments sent to the terminal with putc():
	uchar wbu[40];
	uint  wcnt = 0;

	enum WState {
		NothingPending,
		EscPending,
		EscArgsPending,
		CsiArgsPending,
		SkipUntilST,
		Utf8ArgsPending,
	};
	WState wstate = NothingPending;

	// buffer for chars from keyboard and terminal responses for reading with getc():
	Queue<char, 32, uint16> inputbuffer;

	void apply_margins() noexcept;
	void handle_tab(uint n = 1);
	void handle_backTab(uint n);
	void handle_sendDA();
	void handle_Utf8ArgsPending(char c);
	void handle_C0(char c);	 // 0x00_0x1f
	void handle_C1(char c);	 // 0x80_0x9f
	void handle_ESC(char c); // ESC ansi
	void handle_EscArgsPending(char c);
	void handle_CsiArgsPending(char c);		   // ESC [
	void handle_SGR(uint argc, uint16 args[]); // ESC [ ... m
	void push_cursor() noexcept;
	void pop_cursor() noexcept;
	void log_rbu();
	int	 next_tab(int col) noexcept;
	int	 prev_tab(int col) noexcept;
	void print_char(char);
	void put_csi_response(cstr fmt, ...) __printflike(2, 3);
};

} // namespace kio::Graphics
