// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "File.h"


namespace kio::Devices
{

extern bool isHeatShrinkEncoded(File*);

/*	_____________________________________________________________________________________
	class HeatShrinkDecoder is a wrapper around another File.
	This allows easy decompression of all files read, because class HeatShrinkDecoder implements
	the File interface just like any other file does. The data receiver just must provide
	an initialization with an open File instead of opening it itself.
	class HeatShrinkDecoder supports setFPos() but it is slow.
*/
class HeatShrinkDecoder : public File
{
	NO_COPY_MOVE(HeatShrinkDecoder);

public:
	static constexpr uint32 magic = 0x5f76d7e1;

	/*	Wrap a regular compressed file and read the compression data from the file header.
	*/
	HeatShrinkDecoder(FilePtr file, bool read_magic = true);

	/*	Wrap another file using the provided usize and csize and start decompression at the current fpos.
		csize must contain the wbits and lbits in the MSB.
	*/
	HeatShrinkDecoder(FilePtr file, uint32 usize, uint32 csize);
	virtual ~HeatShrinkDecoder() noexcept override;

	virtual SIZE read(void* data, SIZE, bool partial = false) override;
	virtual ADDR getSize() const noexcept override { return usize; }
	virtual ADDR getFpos() const noexcept override { return upos; }
	virtual void setFpos(ADDR) override;
	virtual void close() override;

	FilePtr file;
	uint32	cdata = 0; // start of compressed data
	uint32	csize = 0; // compressed size (size of cdata[])
	uint32	usize = 0; // uncompressed size (uncompressed file size)
	uint32	upos  = 0; // position inside uncompressed data
	uint32	cpos  = 0; // position inside compressed data

private:
	void init();

	uint16 input_size;	 /* bytes in input buffer */
	uint16 input_index;	 /* offset to next unprocessed input byte */
	uint16 output_count; /* how many bytes to output */
	uint16 output_index; /* index for bytes to output */
	uint16 head_index;	 /* head of window buffer */
	uint8  state;		 /* current state machine node */
	uint8  current_byte; /* current byte of input */
	uint8  bit_index;	 /* current bit index */

	/* Fields that are only used if dynamically allocated. */
	uint8  window_sz2;		  /* window buffer bits */
	uint8  lookahead_sz2;	  /* lookahead bits */
	uint16 input_buffer_size; /* input buffer size */

	/* Input buffer, then expansion window buffer */
	uint8* buffers = nullptr;

	typedef enum {
		HSDR_SINK_OK,			   /* data sunk, ready to poll */
		HSDR_SINK_FULL,			   /* out of space in internal buffer */
		HSDR_SINK_ERROR_NULL = -1, /* NULL argument */
	} HSD_sink_res;

	typedef enum {
		HSDR_POLL_EMPTY,			  /* input exhausted */
		HSDR_POLL_MORE,				  /* more data remaining, call again w/ fresh output buffer */
		HSDR_POLL_ERROR_NULL	= -1, /* NULL arguments */
		HSDR_POLL_ERROR_UNKNOWN = -2,
	} HSD_poll_res;

	typedef enum {
		HSDR_FINISH_DONE,			 /* output is done */
		HSDR_FINISH_MORE,			 /* more output remains */
		HSDR_FINISH_ERROR_NULL = -1, /* NULL arguments */
	} HSD_finish_res;


	/*	Allocate a decoder with an input buffer of INPUT_BUFFER_SIZE bytes,
		an expansion buffer size of 2^WINDOW_SZ2, and a lookahead
		size of 2^lookahead_sz2. (The window buffer and lookahead sizes
		must match the settings used when the data was compressed.)
	*/
	void decoder_alloc(uint16_t input_buffer_size, uint8_t expansion_buffer_sz2, uint8_t lookahead_sz2);

	void decoder_free() noexcept;
	void decoder_reset();

	/*	Sink at most SIZE bytes from IN_BUF into the decoder. *INPUT_SIZE is set to
		indicate how many bytes were actually sunk (in case a buffer was filled).
	*/
	HSD_sink_res decoder_sink(const uint8_t* in_buf, size_t size, size_t* input_size);

	/*	Poll for output from the decoder, copying at most OUT_BUF_SIZE bytes into
		OUT_BUF (setting *OUTPUT_SIZE to the actual amount copied).
	*/
	HSD_poll_res decoder_poll(uint8_t* out_buf, size_t out_buf_size, size_t* output_size);

	/*	Notify the dencoder that the input stream is finished.
		If the return value is HSDR_FINISH_MORE, there is still more output, so
		call heatshrink_decoder_poll and repeat.
	*/
	HSD_finish_res decoder_finish();


	struct output_info
	{
		uint8*	buf;		 /* output buffer */
		size_t	buf_size;	 /* buffer size */
		size_t* output_size; /* bytes pushed to buffer, so far */
	};

	uint16_t get_bits(uint8_t count);
	void	 push_byte(output_info* oi, uint8_t byte);

	/* States for the polling state machine. */
	enum HSD_state {
		HSDS_TAG_BIT,			/* tag bit */
		HSDS_YIELD_LITERAL,		/* ready to yield literal byte */
		HSDS_BACKREF_INDEX_MSB, /* most significant byte of index */
		HSDS_BACKREF_INDEX_LSB, /* least significant byte of index */
		HSDS_BACKREF_COUNT_MSB, /* most significant byte of count */
		HSDS_BACKREF_COUNT_LSB, /* least significant byte of count */
		HSDS_YIELD_BACKREF,		/* ready to yield back-reference */
	};

	HSD_state st_tag_bit();
	HSD_state st_yield_literal(output_info*);
	HSD_state st_backref_index_msb();
	HSD_state st_backref_index_lsb();
	HSD_state st_backref_count_msb();
	HSD_state st_backref_count_lsb();
	HSD_state st_yield_backref(output_info*);
};


} // namespace kio::Devices
