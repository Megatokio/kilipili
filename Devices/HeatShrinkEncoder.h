// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "HeatShrinkDecoder.h"

// Use indexing for compression (much faster but requires much more space.)
#define HEATSHRINK_USE_INDEX 1


namespace kio::Devices
{

/*	____________________________________________________________________________________
	class HeatShrinkEncoder is a wrapper around another File.
	This allows easy compression of all files written, because class HeatShrinkEncoder implements
	the File interface just like any other file does. The data source just must provide
	an initialization with an open File instead of opening it itself.
	class HeatShrinkEncoder does not support setFPos()!

	HeatShrink encoded file header:
	all fields are little endian
		uint32	magic
		uint32  usize | 0x80000000
		uint24	csize
		uint8	wbits << 4 + lbits
		uint8	cdata[csize]
*/
class HeatShrinkEncoder : public File
{
	NO_COPY_MOVE(HeatShrinkEncoder);

public:
	static constexpr uint32 magic = HeatShrinkDecoder::magic;

	/*	Create a compressed file wrapper around another file.
		write_magic = true:  start file with the file magic
		write_magic = false: don't write the file magic.
		Then 8 bytes are reserved for usize, csize and cflags which are written when the file is closed.
		These 12 (or 8) bytes are not included in getSize() and getFpos().
	*/
	HeatShrinkEncoder(FilePtr file, uint8 windowbits = 12, uint8 lookaheadbits = 6, bool write_magic = true);

	/*	`close()` the encoder but do not actively close the target file.
	*/
	virtual ~HeatShrinkEncoder() noexcept override;

	/*	Write remaining data to the target file and dispose of all buffers.
		Write usize, csize and cflags.
		close the target file.
	*/
	virtual void close() override;

	/*	Write remaining data to the target file and dispose of all buffers.
		Write usize, csize and cflags.
		setFpos() to end of data.
		Do not actively close the target file.
	*/
	void finish();

	/*	File / SerialDevice interface:
	*/
	virtual SIZE write(const void* data, SIZE, bool partial = false) override;
	virtual ADDR getSize() const noexcept override { return usize; }
	virtual ADDR getFpos() const noexcept override { return usize; }
	virtual void setFpos(ADDR) override { throw "set fpos not supported"; }

	FilePtr file;
	uint32	cdata = 0; // start of compressed data
	uint32	csize = 0; // compressed size (size of cdata[])
	uint32	usize = 0; // uncompressed size (uncompressed file size)

private:
	void flush();
	void encoder_free();
	void encoder_reset();

	// write data into the encoder.
	// return the actual number of bytes that count be written.
	uint encoder_write(const void* data, uint data_size);

	// read output from the encoder.
	// return the actual number of bytes that count be read.
	uint encoder_read(uint8* buffer, uint buffer_size);

	// Notify the encoder that the input stream is finished.
	// If the return value is HSER_FINISH_MORE, there is still more output, so
	// call heatshrink_encoder_poll and repeat.
	typedef enum {
		HSER_FINISH_DONE, /* encoding is complete */
		HSER_FINISH_MORE, /* more output remaining; use poll */
	} HSE_finish_res;
	HSE_finish_res encoder_finish();


	uint16 input_size; // bytes in input buffer
	uint16 match_scan_index;
	uint16 match_length;
	uint16 match_pos;
	uint16 outgoing_bits; // enqueued outgoing bits
	uint8  outgoing_bits_count;
	bool   is_finishing;
	uint8  state;		  // current state machine node
	uint8  current_byte;  // current byte of output
	uint8  bit_index;	  // current bit index
	uint8  windowbits;	  // 2^n size of window
	uint8  lookaheadbits; // 2^n size of lookahead

#if HEATSHRINK_USE_INDEX
	struct hs_index
	{
		uint16 size;
		int16  index[];
	};
	hs_index* search_index = nullptr;
#endif

	// input buffer and / sliding window for expansion
	uint8* buffer = nullptr;


	enum HSE_state {
		HSES_NOT_FULL_ENOUGH, /* input buffer not full enough */
		HSES_FILLED,		  /* buffer is full */
		HSES_SEARCH,		  /* searching for patterns */
		HSES_YIELD_TAG_BIT,	  /* yield tag bit */
		HSES_YIELD_LITERAL,	  /* emit literal byte */
		HSES_YIELD_BR_INDEX,  /* yielding backref index */
		HSES_YIELD_BR_LENGTH, /* yielding backref length */
		HSES_SAVE_BACKLOG,	  /* copying buffer to backlog */
		HSES_FLUSH_BITS,	  /* flush bit buffer */
		HSES_DONE,			  /* done */
	};

	struct output_info
	{
		uint8* buf;			/* output buffer */
		uint   buf_size;	/* buffer size */
		uint*  output_size; /* bytes pushed to buffer, so far */
		int	   can_take_byte() { return *output_size < buf_size; }
	};

	uint16 get_input_offset();
	uint16 get_input_buffer_size();
	uint16 get_lookahead_size();
	void   add_tag_bit(output_info* oi, uint8 tag);
	void   save_backlog();
	/* Push COUNT (max 8) bits to the output buffer, which has room. */
	void	  push_bits(uint8 count, uint8 bits, output_info* oi);
	uint8	  push_outgoing_bits(output_info* oi);
	void	  push_literal_byte(output_info* oi);
	uint16	  find_longest_match(uint16 start, uint16 end, const uint16 maxlen, uint16* match_length);
	void	  do_indexing();
	HSE_state st_flush_bit_buffer(output_info* oi);
	HSE_state st_save_backlog();
	HSE_state st_yield_br_length(output_info* oi);
	HSE_state st_yield_br_index(output_info* oi);
	HSE_state st_yield_literal(output_info* oi);
	HSE_state st_yield_tag_bit(output_info* oi);
	HSE_state st_step_search();
};


} // namespace kio::Devices


/*

































*/
