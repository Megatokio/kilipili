// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

// Copyright (c) 2013-2015, Scott Vokes <vokes.s@gmail.com>
// All rights reserved.
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

#include "HeatShrinkEncoder.h"
#include "cdefs.h"
#include <cstring>

static constexpr uint8 MIN_WINDOW_BITS	  = 5;	// orig: 4
static constexpr uint8 MAX_WINDOW_BITS	  = 14; // orig: 15, but does not work
static constexpr uint8 MIN_LOOKAHEAD_BITS = 4;	// orig: 3


namespace kio::Devices
{

HeatShrinkEncoder::HeatShrinkEncoder(FilePtr file, uint8 windowbits, uint8 lookaheadbits, bool write_magic) :
	File(Flags::writable),
	file(file),
	windowbits(windowbits),
	lookaheadbits(lookaheadbits)
{
	assert(file != nullptr);

	if ((windowbits < MIN_WINDOW_BITS) || (windowbits > MAX_WINDOW_BITS) || //
		(lookaheadbits < MIN_LOOKAHEAD_BITS) || (lookaheadbits >= windowbits))
		throw "illegal compression parameters";

	// Note: 2 * the window size is used because the buffer needs to fit
	// (1 << windowbits) bytes for the current input, and an additional
	// (1 << windowbits) bytes for the previous buffer of input, which
	// will be scanned for useful backreferences.

	size_t buf_sz = 2u << windowbits;
	buffer		  = reinterpret_cast<uint8*>(malloc(buf_sz));
	if (!buffer) throw OUT_OF_MEMORY;

#if HEATSHRINK_USE_INDEX
	size_t index_sz = buf_sz * sizeof(uint16);
	search_index	= reinterpret_cast<hs_index*>(malloc(index_sz + sizeof(hs_index)));
	if (!search_index)
	{
		free(buffer);
		buffer = nullptr;
		throw OUT_OF_MEMORY;
	}
	search_index->size = uint16(index_sz);
#endif

	if (write_magic) file->write_LE(magic);
	file->write("AAAAAAAA", 8); // reserve space for usize, csize, cflags
	cdata = uint32(file->getFpos());

	encoder_reset();
}

HeatShrinkEncoder::~HeatShrinkEncoder() noexcept
{
	// flush all data to the target file.
	// don't close the target file.
	// if close() wasn't called then the target file will be closed in it's d'tor too.

	try
	{
		finish();
	}
	catch (...)
	{
		encoder_free();
		debugstr("close() failed\n");
	}
}

void HeatShrinkEncoder::finish()
{
	// flush all data to the target file.
	// write usize & csize.
	// release resources.
	// don't close the target file.

	if (buffer)
	{
		encoder_finish();
		flush();
		encoder_free();

		if (file)
		{
			assert(file->getFpos() == cdata + csize);
			file->setFpos(cdata - 8);
			file->write_LE(usize | 0x80000000);
			file->write_LE(csize | uint32(windowbits << 28) | uint32(lookaheadbits << 24));
			file->setFpos(cdata + csize);
		}
	}
}

void HeatShrinkEncoder::close()
{
	// flush all data to the target file.
	// close the target file.

	finish();
	if (file) file->close();
	file = nullptr;
}

void HeatShrinkEncoder::flush()
{
	uint8 buffer[100];
	while (uint cnt = encoder_read(buffer, sizeof(buffer)))
	{
		csize += cnt;
		file->write(buffer, cnt);
	}
}

SIZE HeatShrinkEncoder::write(const void* _data, SIZE size, bool)
{
	const uint8* data	   = reinterpret_cast<const uint8*>(_data);
	SIZE		 remaining = size;

	for (;;)
	{
		SIZE cnt = encoder_write(data, remaining);

		usize += cnt;
		data += cnt;
		remaining -= cnt;
		if (remaining == 0) return size;

		flush();
	}
}

void HeatShrinkEncoder::encoder_free()
{
#if HEATSHRINK_USE_INDEX
	free(search_index);
	search_index = nullptr;
#endif
	free(buffer);
	buffer = nullptr;
}

void HeatShrinkEncoder::encoder_reset()
{
	size_t buf_sz = 2u << this->windowbits;
	memset(this->buffer, 0, buf_sz);
	this->input_size		  = 0;
	this->state				  = HSES_NOT_FULL_ENOUGH;
	this->match_scan_index	  = 0;
	this->is_finishing		  = false;
	this->bit_index			  = 0x80;
	this->current_byte		  = 0x00;
	this->match_length		  = 0;
	this->outgoing_bits		  = 0x0000;
	this->outgoing_bits_count = 0;
}

#define LITERAL_MARKER	0x01
#define BACKREF_MARKER	0x00
#define MATCH_NOT_FOUND uint16(-1)

uint HeatShrinkEncoder::encoder_write(const void* data, uint data_size)
{
	assert(data != nullptr);
	assert(buffer != nullptr);
	assert(!is_finishing);

	if (this->state != HSES_NOT_FULL_ENOUGH) return 0;

	uint write_offset = get_input_offset() + this->input_size;
	uint ibs		  = get_input_buffer_size();
	uint rem		  = ibs - this->input_size;
	uint cp_sz		  = rem < data_size ? rem : data_size;

	memcpy(this->buffer + write_offset, data, cp_sz);
	this->input_size += cp_sz;
	if (cp_sz == rem) this->state = HSES_FILLED;
	return cp_sz;
}

uint HeatShrinkEncoder::encoder_read(uint8* out_buf, uint out_buf_size)
{
	assert(out_buf != nullptr);
	assert(buffer != nullptr);

	uint output_size = 0;

	output_info oi;
	oi.buf		   = out_buf;
	oi.buf_size	   = out_buf_size;
	oi.output_size = &output_size;

	while (true)
	{
		uint8 in_state = this->state;
		switch (in_state)
		{
		case HSES_NOT_FULL_ENOUGH: return output_size;
		case HSES_FILLED:
			do_indexing();
			this->state = HSES_SEARCH;
			break;
		case HSES_SEARCH: this->state = st_step_search(); break;
		case HSES_YIELD_TAG_BIT: this->state = st_yield_tag_bit(&oi); break;
		case HSES_YIELD_LITERAL: this->state = st_yield_literal(&oi); break;
		case HSES_YIELD_BR_INDEX: this->state = st_yield_br_index(&oi); break;
		case HSES_YIELD_BR_LENGTH: this->state = st_yield_br_length(&oi); break;
		case HSES_SAVE_BACKLOG: this->state = st_save_backlog(); break;
		case HSES_FLUSH_BITS: this->state = st_flush_bit_buffer(&oi); return output_size;
		case HSES_DONE: return output_size;
		default: IERR();
		}

		if (this->state == in_state)
		{
			// Check if output buffer is exhausted.
			if (output_size == out_buf_size) return output_size;
		}
	}
}

auto HeatShrinkEncoder::encoder_finish() -> HSE_finish_res
{
	this->is_finishing = true;
	if (this->state == HSES_NOT_FULL_ENOUGH) this->state = HSES_FILLED;
	return this->state == HSES_DONE ? HSER_FINISH_DONE : HSER_FINISH_MORE;
}

auto HeatShrinkEncoder::st_step_search() -> HSE_state
{
	uint16 window_length = get_input_buffer_size();
	uint16 lookahead_sz	 = get_lookahead_size();
	uint16 msi			 = this->match_scan_index;

	if (msi > this->input_size - (is_finishing ? 1 : lookahead_sz))
	{
		// Current search buffer is exhausted, copy it into the
		// backlog and await more input.
		return is_finishing ? HSES_FLUSH_BITS : HSES_SAVE_BACKLOG;
	}

	uint16 input_offset = get_input_offset();
	uint16 end			= input_offset + msi;
	uint16 start		= end - window_length;

	uint16 max_possible = lookahead_sz;
	if (this->input_size - msi < lookahead_sz) { max_possible = this->input_size - msi; }

	uint16 match_length = 0;
	uint16 match_pos	= find_longest_match(start, end, max_possible, &match_length);

	if (match_pos == MATCH_NOT_FOUND)
	{
		this->match_scan_index++;
		this->match_length = 0;
		return HSES_YIELD_TAG_BIT;
	}
	else
	{
		this->match_pos	   = match_pos;
		this->match_length = match_length;
		assert_le(match_pos, 1 << this->windowbits /*window_length*/);
		return HSES_YIELD_TAG_BIT;
	}
}

auto HeatShrinkEncoder::st_yield_tag_bit(output_info* oi) -> HSE_state
{
	if (oi->can_take_byte())
	{
		if (this->match_length == 0)
		{
			add_tag_bit(oi, LITERAL_MARKER);
			return HSES_YIELD_LITERAL;
		}
		else
		{
			add_tag_bit(oi, BACKREF_MARKER);
			this->outgoing_bits		  = this->match_pos - 1;
			this->outgoing_bits_count = this->windowbits;
			return HSES_YIELD_BR_INDEX;
		}
	}
	else return HSES_YIELD_TAG_BIT; // output is full, continue
}

auto HeatShrinkEncoder::st_yield_literal(output_info* oi) -> HSE_state
{
	if (oi->can_take_byte())
	{
		push_literal_byte(oi);
		return HSES_SEARCH;
	}
	else return HSES_YIELD_LITERAL;
}

auto HeatShrinkEncoder::st_yield_br_index(output_info* oi) -> HSE_state
{
	if (oi->can_take_byte())
	{
		if (push_outgoing_bits(oi) > 0) return HSES_YIELD_BR_INDEX; // continue
		else
		{
			this->outgoing_bits		  = this->match_length - 1;
			this->outgoing_bits_count = this->lookaheadbits;
			return HSES_YIELD_BR_LENGTH; // done
		}
	}
	else return HSES_YIELD_BR_INDEX; // continue
}

auto HeatShrinkEncoder::st_yield_br_length(output_info* oi) -> HSE_state
{
	if (oi->can_take_byte())
	{
		if (push_outgoing_bits(oi) > 0) { return HSES_YIELD_BR_LENGTH; }
		else
		{
			this->match_scan_index += this->match_length;
			this->match_length = 0;
			return HSES_SEARCH;
		}
	}
	else { return HSES_YIELD_BR_LENGTH; }
}

auto HeatShrinkEncoder::st_save_backlog() -> HSE_state
{
	save_backlog();
	return HSES_NOT_FULL_ENOUGH;
}

auto HeatShrinkEncoder::st_flush_bit_buffer(output_info* oi) -> HSE_state
{
	if (this->bit_index == 0x80) { return HSES_DONE; }
	else if (oi->can_take_byte())
	{
		oi->buf[(*oi->output_size)++] = this->current_byte;
		return HSES_DONE;
	}
	else { return HSES_FLUSH_BITS; }
}

void HeatShrinkEncoder::add_tag_bit(output_info* oi, uint8 tag) { push_bits(1, tag, oi); }

uint16 HeatShrinkEncoder::get_input_offset() { return get_input_buffer_size(); }

uint16 HeatShrinkEncoder::get_input_buffer_size() { return (1 << this->windowbits); }

uint16 HeatShrinkEncoder::get_lookahead_size() { return (1 << this->lookaheadbits); }

void HeatShrinkEncoder::do_indexing()
{
#if HEATSHRINK_USE_INDEX
	/* Build an index array I that contains flattened linked lists
     * for the previous instances of every byte in the buffer.
     * 
     * For example, if buf[200] == 'x', then index[200] will either
     * be an offset i such that buf[i] == 'x', or a negative offset
     * to indicate end-of-list. This significantly speeds up matching,
     * while only using sizeof(uint16)*sizeof(buffer) bytes of RAM.
     *
     * Future optimization options:
     * 1. Since any negative value represents end-of-list, the other
     *    15 bits could be used to improve the index dynamically.
     *    
     * 2. Likewise, the last lookahead_sz bytes of the index will
     *    not be usable, so temporary data could be stored there to
     *    dynamically improve the index.
     * */
	struct hs_index* hsi = this->search_index;
	int16_t			 last[256];
	memset(last, 0xFF, sizeof(last));

	uint8* const   data	 = this->buffer;
	int16_t* const index = hsi->index;

	const uint16 input_offset = get_input_offset();
	const uint16 end		  = input_offset + this->input_size;

	for (uint16 i = 0; i < end; i++)
	{
		uint8	v  = data[i];
		int16_t lv = last[v];
		index[i]   = lv;
		last[v]	   = i;
	}
#else
	(void)this;
#endif
}

uint16 HeatShrinkEncoder::find_longest_match(uint16 start, uint16 end, const uint16 maxlen, uint16* match_length)
{
	// Return the longest match for the bytes at buf[end:end+maxlen] between
	// buf[start] and buf[end-1]. If no match is found, return -1.

	uint8* buf = this->buffer;

	uint16 match_maxlen = 0;
	uint16 match_index	= MATCH_NOT_FOUND;

	uint16		 len		 = 0;
	uint8* const needlepoint = &buf[end];

#if HEATSHRINK_USE_INDEX
	struct hs_index* hsi = this->search_index;
	int16			 pos = hsi->index[end];

	while (pos - int16(start) >= 0)
	{
		uint8* const pospoint = &buf[pos];
		len					  = 0;

		// Only check matches that will potentially beat the current maxlen.
		// This is redundant with the index if match_maxlen is 0, but the
		// added branch overhead to check if it == 0 seems to be worse.
		if (pospoint[match_maxlen] != needlepoint[match_maxlen])
		{
			pos = hsi->index[pos];
			continue;
		}

		for (len = 1; len < maxlen; len++)
		{
			if (pospoint[len] != needlepoint[len]) break;
		}

		if (len > match_maxlen)
		{
			match_maxlen = len;
			match_index	 = pos;
			if (len == maxlen) { break; } /* won't find better */
		}
		pos = hsi->index[pos];
	}
#else
	for (int16_t pos = end - 1; pos - (int16_t)start >= 0; pos--)
	{
		uint8* const pospoint = &buf[pos];
		if ((pospoint[match_maxlen] == needlepoint[match_maxlen]) && (*pospoint == *needlepoint))
		{
			for (len = 1; len < maxlen; len++)
			{
				if (0)
				{
					LOG("  --> cmp buf[%d] == 0x%02x against %02x (start %u)\n", pos + len, pospoint[len],
						needlepoint[len], start);
				}
				if (pospoint[len] != needlepoint[len]) { break; }
			}
			if (len > match_maxlen)
			{
				match_maxlen = len;
				match_index	 = pos;
				if (len == maxlen) { break; } /* don't keep searching */
			}
		}
	}
#endif

	const size_t break_even_point = (1 + this->windowbits + this->lookaheadbits);

	// Instead of comparing break_even_point against 8*match_maxlen,
	// compare match_maxlen against break_even_point/8 to avoid
	// overflow. Since MIN_WINDOW_BITS and MIN_LOOKAHEAD_BITS are 4 and
	// 3, respectively, break_even_point/8 will always be at least 1.
	if (match_maxlen > (break_even_point / 8))
	{
		*match_length = match_maxlen;
		return end - match_index;
	}
	return MATCH_NOT_FOUND;
}

uint8 HeatShrinkEncoder::push_outgoing_bits(output_info* oi)
{
	uint8 count = 0;
	uint8 bits	= 0;
	if (this->outgoing_bits_count > 8)
	{
		count = 8;
		bits  = this->outgoing_bits >> (this->outgoing_bits_count - 8);
	}
	else
	{
		count = this->outgoing_bits_count;
		bits  = this->outgoing_bits;
	}

	if (count > 0)
	{
		push_bits(count, bits, oi);
		this->outgoing_bits_count -= count;
	}
	return count;
}

void HeatShrinkEncoder::push_bits(uint8 count, uint8 bits, output_info* oi)
{
	// Push COUNT (max 8) bits to the output buffer, which has room.
	// Bytes are set from the lowest bits, up.

	assert(count <= 8);

	// If adding a whole byte and at the start of a new output byte,
	// just push it through whole and skip the bit IO loop.
	if (count == 8 && this->bit_index == 0x80) { oi->buf[(*oi->output_size)++] = bits; }
	else
	{
		for (int i = count - 1; i >= 0; i--)
		{
			bool bit = bits & (1 << i);
			if (bit) { this->current_byte |= this->bit_index; }
			this->bit_index >>= 1;
			if (this->bit_index == 0x00)
			{
				this->bit_index				  = 0x80;
				oi->buf[(*oi->output_size)++] = this->current_byte;
				this->current_byte			  = 0x00;
			}
		}
	}
}

void HeatShrinkEncoder::push_literal_byte(output_info* oi)
{
	uint16 processed_offset = this->match_scan_index - 1;
	uint16 input_offset		= get_input_offset() + processed_offset;
	uint8  c				= this->buffer[input_offset];
	push_bits(8, c, oi);
}

void HeatShrinkEncoder::save_backlog()
{
	size_t input_buf_sz = get_input_buffer_size();

	uint16 msi = this->match_scan_index;

	/* Copy processed data to beginning of buffer, so it can be
     * used for future matches. Don't bother checking whether the
     * input is less than the maximum size, because if it isn't,
     * we're done anyway. */
	uint16 rem		= input_buf_sz - msi; // unprocessed bytes
	uint16 shift_sz = input_buf_sz + rem;

	memmove(&this->buffer[0], &this->buffer[input_buf_sz - rem], shift_sz);

	this->match_scan_index = 0;
	this->input_size -= input_buf_sz - rem;
}


} // namespace kio::Devices


/*






























*/
