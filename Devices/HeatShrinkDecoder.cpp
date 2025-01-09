// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause
//
// Copyright (c) 2013-2015, Scott Vokes <vokes.s@gmail.com>
// All rights reserved.
// "https://github.com/atomicobject/heatshrink"
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


#include "HeatShrinkDecoder.h"
#include "cdefs.h"
#include <string.h>

#define MIN_WINDOW_BITS 4
#define MAX_WINDOW_BITS 14

#define MIN_LOOKAHEAD_BITS 3


namespace kio::Devices
{

bool isHeatShrinkEncoded(File* file)
{
	if (file == nullptr) return false;
	ADDR fpos  = file->getFpos();
	ADDR fsize = file->getSize();
	if (fpos + 12 < fsize) return false;
	uint32 magic = file->read_LE<uint32>();
	uint32 usize = file->read_LE<uint32>();
	uint32 csize = file->read_LE<uint32>();
	file->setFpos(fpos);
	(void)usize;
	if ((csize >> 24) == 0) return false;
	if (fpos + 12 + csize > fsize) return false;
	if (magic != HeatShrinkDecoder::magic) return false;
	return true;
}

HeatShrinkDecoder::HeatShrinkDecoder(FilePtr file, bool read_magic) : //
	File(Flags::readable),
	file(file)
{
	assert(file != nullptr);

	if (read_magic && file->read_LE<uint32>() != magic) throw "not a HeatShrink encoded file";
	usize = file->read_LE<uint32>();
	csize = file->read_LE<uint32>();
	init();
}

HeatShrinkDecoder::HeatShrinkDecoder(FilePtr file, uint32 usize, uint32 csize) :
	File(Flags::readable),
	file(file),
	csize(csize),
	usize(usize)
{
	assert(file != nullptr);
	init();
}

void HeatShrinkDecoder::init()
{
	uint8 wbits = csize >> 28;
	uint8 lbits = (csize >> 24) & 0x0f;
	csize		= csize & 0x00ffffff;

	if (lbits < 4 || lbits > 12) throw "illegal compression parameters";
	if (wbits <= lbits || wbits > 14) throw "illegal compression parameters";

	decoder_alloc(100, wbits, lbits);

	cdata = uint32(file->getFpos());
}

HeatShrinkDecoder::~HeatShrinkDecoder() noexcept
{
	// does not actively close the target file.
	// if close() wasn't called then the target file will be closed in it's d'tor too.

	decoder_free();
}

void HeatShrinkDecoder::close()
{
	decoder_free();
	if (file) file->close();
	file = nullptr;
}

SIZE HeatShrinkDecoder::read(void* _data, SIZE size, bool partial)
{
	if unlikely (size > usize - upos)
	{
		size = usize - upos;
		if (!partial) throw END_OF_FILE;
		if (eof_pending()) throw END_OF_FILE;
		if (size == 0) set_eof_pending();
	}
	uint8* data		 = reinterpret_cast<uint8*>(_data);
	uint32 remaining = size;

	for (;;)
	{
		size_t cnt	  = 0;
		int	   result = decoder_poll(data, remaining, &cnt);
		assert(result >= 0);
		upos += cnt;
		data += cnt;
		remaining -= cnt;
		if (remaining == 0) return size;

		uint8  buffer[100];
		uint32 avail = file->read(buffer, std::min(csize - cpos, uint32(sizeof(buffer))));
		if unlikely (avail == 0) throw "data corrupted"; // cpos == csize
		result = decoder_sink(buffer, avail, &cnt);
		assert(result >= 0);
		assert(cnt == avail);
		cpos += avail;
	}
}

void HeatShrinkDecoder::setFpos(ADDR new_upos)
{
	clear_eof_pending();

	if unlikely (new_upos >= usize)
	{
		upos = usize;
		return;
	}

	if (uint32(new_upos) < upos)
	{
		decoder_reset();
		file->setFpos(cdata);
		upos = 0;
		cpos = 0;
	}

	while (upos < uint32(new_upos))
	{
		uint8 buffer[100];
		read(buffer, std::min(uint32(new_upos) - upos, uint32(sizeof(buffer))));
	}
}


#define NO_BITS uint16(-1)

void HeatShrinkDecoder::decoder_alloc(uint16 input_buffer_size, uint8 window_sz2, uint8 lookahead_sz2)
{
	assert(buffers == nullptr);

	if ((window_sz2 < MIN_WINDOW_BITS) || (window_sz2 > MAX_WINDOW_BITS) || (input_buffer_size == 0) ||
		(lookahead_sz2 < MIN_LOOKAHEAD_BITS) || (lookahead_sz2 >= window_sz2))
		throw "HeatShrinkDecoder: illegal parameters";

	buffers = new uint8[(1 << window_sz2) + input_buffer_size];

	this->input_buffer_size = input_buffer_size;
	this->window_sz2		= window_sz2;
	this->lookahead_sz2		= lookahead_sz2;

	decoder_reset();
}

void HeatShrinkDecoder::decoder_free() noexcept
{
	delete[] buffers;
	buffers = nullptr;
}

void HeatShrinkDecoder::decoder_reset()
{
	size_t buf_sz	= 1 << this->window_sz2;
	size_t input_sz = this->input_buffer_size;
	memset(this->buffers, 0, buf_sz + input_sz);
	this->state		   = HSDS_TAG_BIT;
	this->input_size   = 0;
	this->input_index  = 0;
	this->bit_index	   = 0x00;
	this->current_byte = 0x00;
	this->output_count = 0;
	this->output_index = 0;
	this->head_index   = 0;
}

/* Copy SIZE bytes into the decoder's input buffer, if it will fit. */
auto HeatShrinkDecoder::decoder_sink(const uint8* in_buf, size_t size, size_t* input_size) -> HSD_sink_res
{
	if ((in_buf == NULL) || (input_size == NULL)) { return HSDR_SINK_ERROR_NULL; }

	size_t rem = this->input_buffer_size - this->input_size;
	if (rem == 0)
	{
		*input_size = 0;
		return HSDR_SINK_FULL;
	}

	size = rem < size ? rem : size;

	// copy into input buffer (at head of buffers)
	memcpy(&this->buffers[this->input_size], in_buf, size);
	this->input_size += size;
	*input_size = size;
	return HSDR_SINK_OK;
}


/*****************
 * Decompression *
 *****************/

auto HeatShrinkDecoder::decoder_poll(uint8* out_buf, size_t out_buf_size, size_t* output_size) -> HSD_poll_res
{
	if ((out_buf == NULL) || (output_size == NULL)) { return HSDR_POLL_ERROR_NULL; }
	*output_size = 0;

	output_info oi;
	oi.buf		   = out_buf;
	oi.buf_size	   = out_buf_size;
	oi.output_size = output_size;

	while (1)
	{
		uint8 in_state = this->state;
		switch (in_state)
		{
		case HSDS_TAG_BIT: this->state = st_tag_bit(); break;
		case HSDS_YIELD_LITERAL: this->state = st_yield_literal(&oi); break;
		case HSDS_BACKREF_INDEX_MSB: this->state = st_backref_index_msb(); break;
		case HSDS_BACKREF_INDEX_LSB: this->state = st_backref_index_lsb(); break;
		case HSDS_BACKREF_COUNT_MSB: this->state = st_backref_count_msb(); break;
		case HSDS_BACKREF_COUNT_LSB: this->state = st_backref_count_lsb(); break;
		case HSDS_YIELD_BACKREF: this->state = st_yield_backref(&oi); break;
		default: return HSDR_POLL_ERROR_UNKNOWN;
		}

		// If the current state cannot advance, check if input or output buffer are exhausted.
		if (this->state == in_state)
		{
			if (*output_size == out_buf_size) { return HSDR_POLL_MORE; }
			return HSDR_POLL_EMPTY;
		}
	}
}

auto HeatShrinkDecoder::st_tag_bit() -> HSD_state
{
	uint32 bits = get_bits(1); // get tag bit
	if (bits == NO_BITS) { return HSDS_TAG_BIT; }
	else if (bits) { return HSDS_YIELD_LITERAL; }
	else if (this->window_sz2 > 8) { return HSDS_BACKREF_INDEX_MSB; }
	else
	{
		this->output_index = 0;
		return HSDS_BACKREF_INDEX_LSB;
	}
}

void HeatShrinkDecoder::push_byte(output_info* oi, uint8 byte)
{
	oi->buf[(*oi->output_size)++] = byte;
	(void)this;
}

auto HeatShrinkDecoder::st_yield_literal(output_info* oi) -> HSD_state
{
	/* Emit a repeated section from the window buffer, and add it (again)
     * to the window buffer. (Note that the repetition can include
     * itself.)*/
	if (*oi->output_size < oi->buf_size)
	{
		uint16 byte = get_bits(8);
		if (byte == NO_BITS) { return HSDS_YIELD_LITERAL; } /* out of input */
		uint8* buf					   = &this->buffers[this->input_buffer_size];
		uint   mask					   = (1 << this->window_sz2) - 1;
		uint8  c					   = byte & 0xFF;
		buf[this->head_index++ & mask] = c;
		push_byte(oi, c);
		return HSDS_TAG_BIT;
	}
	else { return HSDS_YIELD_LITERAL; }
}

auto HeatShrinkDecoder::st_backref_index_msb() -> HSD_state
{
	uint8 bit_ct = this->window_sz2;
	assert(bit_ct > 8);
	uint16 bits = get_bits(bit_ct - 8);
	if (bits == NO_BITS) { return HSDS_BACKREF_INDEX_MSB; }
	this->output_index = bits << 8;
	return HSDS_BACKREF_INDEX_LSB;
}

auto HeatShrinkDecoder::st_backref_index_lsb() -> HSD_state
{
	uint8  bit_ct = this->window_sz2;
	uint16 bits	  = get_bits(bit_ct < 8 ? bit_ct : 8);
	if (bits == NO_BITS) { return HSDS_BACKREF_INDEX_LSB; }
	this->output_index |= bits;
	this->output_index++;
	uint8 br_bit_ct	   = this->lookahead_sz2;
	this->output_count = 0;
	return (br_bit_ct > 8) ? HSDS_BACKREF_COUNT_MSB : HSDS_BACKREF_COUNT_LSB;
}

auto HeatShrinkDecoder::st_backref_count_msb() -> HSD_state
{
	uint8 br_bit_ct = this->lookahead_sz2;
	assert(br_bit_ct > 8);
	uint16 bits = get_bits(br_bit_ct - 8);
	if (bits == NO_BITS) { return HSDS_BACKREF_COUNT_MSB; }
	this->output_count = bits << 8;
	return HSDS_BACKREF_COUNT_LSB;
}

auto HeatShrinkDecoder::st_backref_count_lsb() -> HSD_state
{
	uint8  br_bit_ct = this->lookahead_sz2;
	uint16 bits		 = get_bits(br_bit_ct < 8 ? br_bit_ct : 8);
	if (bits == NO_BITS) { return HSDS_BACKREF_COUNT_LSB; }
	this->output_count |= bits;
	this->output_count++;
	return HSDS_YIELD_BACKREF;
}

auto HeatShrinkDecoder::st_yield_backref(output_info* oi) -> HSD_state
{
	size_t count = oi->buf_size - *oi->output_size;
	if (count > 0)
	{
		size_t i = 0;
		if (this->output_count < count) count = this->output_count;
		uint8* buf		  = &this->buffers[this->input_buffer_size];
		uint16 mask		  = (1 << this->window_sz2) - 1;
		uint16 neg_offset = this->output_index;
		assert(count <= size_t(1 << this->lookahead_sz2));
		assert(neg_offset <= mask + 1);

		for (i = 0; i < count; i++)
		{
			uint8 c = buf[(this->head_index - neg_offset) & mask];
			push_byte(oi, c);
			buf[this->head_index & mask] = c;
			this->head_index++;
		}
		this->output_count -= count;
		if (this->output_count == 0) { return HSDS_TAG_BIT; }
	}
	return HSDS_YIELD_BACKREF;
}

uint16 HeatShrinkDecoder::get_bits(uint8 count)
{
	// Get the next COUNT bits from the input buffer, saving incremental progress.
	// Returns NO_BITS on end of input, or if more than 15 bits are requested.

	uint16 accumulator = 0;
	int	   i		   = 0;
	if (count > 15) { return NO_BITS; }

	// If we aren't able to get COUNT bits, suspend immediately, because we
	// don't track how many bits of COUNT we've accumulated before suspend.
	if (this->input_size == 0)
		if (this->bit_index < (1 << (count - 1))) { return NO_BITS; }

	for (i = 0; i < count; i++)
	{
		if (this->bit_index == 0x00)
		{
			if (this->input_size == 0) { return NO_BITS; }
			this->current_byte = this->buffers[this->input_index++];
			if (this->input_index == this->input_size)
			{
				this->input_index = 0; // input is exhausted
				this->input_size  = 0;
			}
			this->bit_index = 0x80;
		}
		accumulator <<= 1;
		if (this->current_byte & this->bit_index) accumulator |= 0x01;
		this->bit_index >>= 1;
	}

	return accumulator;
}

auto HeatShrinkDecoder::decoder_finish() -> HSD_finish_res
{
	switch (this->state)
	{
	case HSDS_TAG_BIT: return this->input_size == 0 ? HSDR_FINISH_DONE : HSDR_FINISH_MORE;

	// If we want to finish with no input, but are in these states, it's
	// because the 0-bit padding to the last byte looks like a backref
	// marker bit followed by all 0s for index and count bits.
	case HSDS_BACKREF_INDEX_LSB:
	case HSDS_BACKREF_INDEX_MSB:
	case HSDS_BACKREF_COUNT_LSB:
	case HSDS_BACKREF_COUNT_MSB: return this->input_size == 0 ? HSDR_FINISH_DONE : HSDR_FINISH_MORE;

	// If the output stream is padded with 0xFFs (possibly due to being in
	// flash memory), also explicitly check the input size rather than
	// uselessly returning MORE but yielding 0 bytes when polling.
	case HSDS_YIELD_LITERAL: return this->input_size == 0 ? HSDR_FINISH_DONE : HSDR_FINISH_MORE;

	default: return HSDR_FINISH_MORE;
	}
}


} // namespace kio::Devices


/*
































*/
