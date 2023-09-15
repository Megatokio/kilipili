// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Sprites.h"
#include "Graphics/Pixmap.h"
#include "StackInfo.h"
#include "composable_scanline.h"
#include <hardware/sync.h>
#include <pico/stdlib.h>
#include <pico/sync.h>


namespace kipili::Video
{

using namespace Graphics;

// all hot video code should go into ram to allow video while flashing.
// also, there should be no const data accessed in hot video code for the same reason.
// the most timecritical things should go into core1 stack page because it is not contended.

#define WRAP(X)	 #X
#define XWRAP(X) WRAP(X)
#define XRAM	 __attribute__((section(".scratch_x.spr" XWRAP(__LINE__))))		// the 4k page with the core1 stack
#define RAM		 __attribute__((section(".time_critical.spr" XWRAP(__LINE__)))) // general ram


static spin_lock_t* spinlock = nullptr;

struct Locker
{
	uint32 state; // status register
	Locker() noexcept { state = spin_lock_blocking(spinlock); }
	~Locker() noexcept { spin_unlock(spinlock, state); }
};

#define lock_list() Locker _locklist


// =============================================================
//		struct Shape
// =============================================================

Shape::Row Shape::row_with_stopper {.dx = Shape::Row::dxStopper, .width = 0};

Shape::Row* Shape::Row::construct(int width, int height, const Color* pixels) noexcept
{
	// copy and 'compress' data from bitmap
	// kind of placement new
	//
	// this Shape must be large enough to hold the resulting data!
	// you may call calc_size() for this

	assert(width >= 0);
	assert(height >= 0);
	assert(pixels != nullptr);

	// copy & 'compress' pixels:
	Row* p	 = this; // running pointer
	int	 dx0 = 0;	 // x offset of last row (absolute dx)

	for (int y = 0; y < height; y++, pixels += width, p = p->next())
	{
		int dx, sz;
		tie(dx, sz) = calc_sz(width, pixels);
		if (sz)
		{
			dx -= dx0;
			dx0 += dx;
		}
		else dx = 0;

		assert(dx == int8(dx) && dx != dxStopper);
		assert(sz == uint8(sz));
		assert(uint(dx0 + sz) <= uint(width));

		p->dx	 = int8(dx);
		p->width = uint8(sz);
		memcpy(p->pixels, pixels + dx0, uint(sz) * sizeof(uint16));
	}

	p->dx = dxStopper;
	assert((uint16ptr(p) - uint16ptr(this) + 1) == calc_size(width, height, pixels - (width * height)).first);
	return this;
}

Shape::Row* Shape::Row::newShape(int width, int height, const Color* pixels) // throws oomem
{
	// allocate and initialize a new Shape

	assert(width >= 0);
	assert(height >= 0);
	assert(pixels != nullptr);

	int count;
	tie(count, height) = calc_size(width, height, pixels);				  // calc size and adjust height for empty lines
	Row* shape		   = reinterpret_cast<Row*>(new uint16[uint(count)]); // allocate: may throw.
	return shape->construct(width, height, pixels);
}

Shape::Row* Shape::Row::newShape(const Graphics::Pixmap_rgb& pixmap) // throws oomem
{
	assert(pixmap.row_offset == pixmap.width * int(sizeof(Color)));

	return newShape(pixmap.width, pixmap.height, reinterpret_cast<const Color*>(pixmap.pixmap));
}

Shape::Shape(const Graphics::Pixmap_rgb& pixmap, int16 hot_x, int16 hot_y) :
	width(int16(pixmap.width)),
	height(int16(pixmap.height)),
	hot_x(hot_x),
	hot_y(hot_y),
	rows(Shape::Row::newShape(pixmap)) // throws
{}

constexpr Color bitmap_test1[] = {
#define _ transparent,
#define b Color::fromRGB8(0, 0, 0),
#define F Color::fromRGB8(0xEE, 0xEE, 0xFF),

	_ _ F _ _ F _ _ _ _ _ _ _ F F _

#undef _
#undef b
#undef F
};

static_assert(Shape::Row::calc_size(4, 4, bitmap_test1).second == 4);
static_assert(Shape::Row::calc_size(4, 4, bitmap_test1).first == 9);


// =============================================================
//		HotList and DisplayList::scanlineRenderer()
// =============================================================

#pragma GCC push_options
#pragma GCC optimize("O3")

static inline XRAM uint16* store_transp(uint16* dp, int cnt)
{
	if (cnt >= 3)
	{
		*dp++ = CMD::COLOR_RUN;
		*dp++ = transparent;
		*dp++ = uint16(cnt - 3);
	}
	else if (cnt == 2)
	{
		*dp++ = CMD::RAW_2P;
		*dp++ = transparent;
		*dp++ = transparent;
	}
	else if (cnt == 1)
	{
		*dp++ = CMD::RAW_1P;
		*dp++ = transparent;
	}
	return dp;
}

static inline XRAM uint16* store_pixels(uint16* dp, const Color* pixels, int cnt)
{
	if (cnt >= 3)
	{
		*dp++ = CMD::RAW_RUN;
		*dp++ = *pixels;
		*dp++ = uint16(cnt - 3);
		memcpy(dp, pixels + 1, uint(cnt - 1) * sizeof(Color));
		dp += cnt - 1;
	}
	else if (cnt == 2)
	{
		*dp++ = CMD::RAW_2P;
		*dp++ = *pixels++;
		*dp++ = *pixels++;
	}
	else if (cnt == 1)
	{
		*dp++ = CMD::RAW_1P;
		*dp++ = *pixels++;
	}
	return dp;
}

static inline XRAM uint16* store_eol(uint16* dp)
{
	*dp++ = CMD::RAW_1P;
	*dp++ = transparent;

	if (uint(dp) & 2) { *dp++ = CMD::EOL; }
	else
	{
		*dp++ = CMD::EOL_SKIP;
		*dp++ = 0;
	}
	return dp;
}

struct HotData
{
	const Shape::Row* rows;
	int				  xa; // effective x position: xa = sprite.x + row.xoffs
	int				  xe; // xe = xa + number of pixels in current row
	uint			  z;  // z value
};

struct HotList
{
	HotData shapes[MAX_HOT_SPRITES + 1]; // sorted by xa; '+1' for stopper
	uint	count = 0;
	int		screen_width;
	Sprite* next_sprite = nullptr; // in displaylist
	int		row;

	void check() const noexcept;

	void initialize() noexcept;
	void add(const Sprite*, int dy) noexcept; // insert sorted
	void remove(uint i) noexcept;
	void sort() noexcept; // sort by xa

	void add_sprites(int row) noexcept;
	void next_row() noexcept;

	static uint16* render_scanline(uint16* dp, const HotData* rows);
};

static HotList hotlist;

static struct
{
	Shape::Row row1 {.dx = 0, .width = 1}; // .dx must be 0, .width must be (at least) 1 pixel wide
	Color	   d1[1] {black};			   // must not be transparent
	Shape::Row row2 {.dx = Shape::Row::dxStopper, .width = 0};
} stopper_rows;


void RAM HotList::check() const noexcept
{
	assert(count >= 1);
	assert(count <= int(count_of(shapes)));

	for (uint i = 0; i < count; i++)
	{
		const HotData& shape = shapes[i];
		assert(!shape.rows->is_at_end());
		assert(i == 0 || shapes[i - 1].xa <= shape.xa);
		assert(shape.xe - shape.xa == shape.rows->width);
		assert(shape.rows->width == 0 || shape.rows->pixels[0].is_opaque());
		assert(shape.rows->width == 0 || shape.rows->pixels[shape.rows->width - 1].is_opaque());
		assert(shape.z < Sprite::zStopper || shape.xa == screen_width);
		assert(shape.z < Sprite::zStopper || shape.rows == &stopper_rows.row1);
	}
}

void HotList::initialize() noexcept
{
	HotData& stopper = shapes[0];
	stopper.rows	 = &stopper_rows.row1;
	stopper.xa		 = screen_width;
	stopper.xe		 = screen_width + 1; // must match rows.row1.width
	stopper.z		 = Sprite::zStopper;

	count = 1;
}

inline XRAM uint16* HotList::render_scanline(uint16* dp, const HotData* row0)
{
	/* copy pixels from all rows[] into scanline at dp++
	   rows[] must be sorted by x position
	   all z values must be < StopperZ
	   last row must be a stopper with xa == screen_width and z = StopperZ
	   return new dp behind stored data

	   handles num_rows = 0
	   handles rows starting left of screen (x < 0)
	   handles rows ending left of screen (x + width <= 0)
	   handles rows ending right of screen (x+width > screen_width)
	   handles rows starting right of screen (x >= screen_width)
	   handles rows with width = 0

	   handles overlapping rows
	   if perfect transparence then handle transparency in overlapping sprites
	   else lower plane will show through transparent pixels
	   even if there is another sprite below which should show through instead

	   !! too many sprites on the same line may result in cpu overload
	   !! if sprites cover allmost all pixels of a scanline then data may overwrite buffer end
	*/

	debuginfo();

	if constexpr (SPRITES_USE_PERFECT_TRANSPARENCY)
	{
		// running x position:
		int x = 0;

		// find first sprite in screen:
		while (row0->xe <= 0 || row0->xe == row0->xa) row0++;

	loop_with_gap:

		assert(x < hotlist.screen_width);

		// bail out if no sprite in screen:
		if (row0->xa >= hotlist.screen_width) return store_eol(dp);

		// store transparency up to new x position:
		if (row0->xa > x)
		{
			dp = store_transp(dp, row0->xa - x);
			x  = row0->xa;
		}

		// start of stored pixels:
		uint16* dp0 = dp;
		dp += 2; // +2 for CMD and count

	loop_no_gap:

		assert(x < hotlist.screen_width);
		assert(row0->xa <= x);
		assert(row0->xe > x); // && row0->xe != row0->xa

		// find row which provides the first pixel:
		const HotData* row = nullptr;

		for (const HotData* r = row0; r->xa <= x; r++)
		{
			if (r->xe <= x) continue;
			if (r->rows->pixels[x - r->xa].is_transparent()) continue;
			if (row == nullptr || r->z > row->z) row = r;
		}

		if (row != nullptr) // `row` starts with opaque pixel (most common case)
		{
			assert(row->rows->pixels[x - row->xa].is_opaque());

			// find end of copy range:
			// this is the end of this `row` or when overlapped by opaque pixel in row with higher z:

			int x_end = row->xe; // current x_end which we eventually have to trim down

			for (const HotData* r = row0; r->xa < x_end; r++)
			{
				if (r->xe > x && r->z > row->z) // above current `row`:
				{
					if (r->xa >= x) // first pixel of any row must be opaque
					{
						if (r->xe == r->xa) continue; // except if width==0

						x_end = r->xa; // any further row starts at or after r->xa
						break;		   // => we are done
					}

					// row `r` is above `row` and it started before current `x`
					// but did not obscure current position
					// => row `r` must actually start with a transparent pixel at `x`.

					//assert (r->xe > r->xa);
					assert(r->xa < x);
					assert(r->xe > x + 1);

					const Color* px = r->rows->pixels - r->xa; // -> pixel at x=0 (outside pixels[])
					assert(px[x].is_transparent());

					int e = x;
					while (e < x_end && px[e].is_transparent()) { e++; }
					x_end = e;
				}
			}

			assert(x_end > x);
			assert(x_end <= hotlist.screen_width);

			// copy the pixels:
			const Color* px = row->rows->pixels - row->xa;
			while (x < x_end && px[x].is_opaque()) { *dp++ = px[x++]; }
		}
		else // row starts with transparent pixel at `x`
		{
			row = row0;
			assert(row->rows->pixels[x - row->xa].is_transparent());

			*dp++ = transparent;
			x++;
			//if (x < hotlist.screen_width) { goto loop_no_gap; }
		}

		assert(x <= hotlist.screen_width);

		// skip over done rows:
		while (row0->xe <= x || row0->xe == row0->xa) { row0++; }

		// more in current scrum?
		if (row0->xa <= x && x < hotlist.screen_width) { goto loop_no_gap; }

		// now there is a gap or we are at end of rows:
		// store appropriate CMD before pixels in output buffer:
		int cnt = dp - dp0 - 2;
		if (cnt >= 3)
		{
			dp0[0] = CMD::RAW_RUN;
			dp0[1] = dp0[2];
			dp0[2] = uint16(cnt - 3);
		}
		else if (cnt == 2)
		{
			dp -= 1;
			dp0[0] = CMD::RAW_2P;
			dp0[1] = dp0[2];
			dp0[2] = *dp;
		}
		else // if (cnt == 1)
		{
			assert(cnt == 1);
			dp -= 1;
			dp0[0] = CMD::RAW_1P;
			dp0[1] = *dp;
		}

		if (x < hotlist.screen_width && row0->xa < hotlist.screen_width)
		{
			assert(row0->xa > x);
			goto loop_with_gap;
		}

		assert(x <= hotlist.screen_width);

		return store_eol(dp);
	}
	else // no perfect transparency:
	{
		int x = 0; // running x position

		for (;;)
		{
			// discard rows left of current x position:
			while (row0->xe <= x || row0->xe == row0->xa)
			{
				assert(row0->z < Sprite::zStopper);
				row0++;
			}

			assert(row0->xe > x);

			// store transparency up to new x position:
			if (row0->xa >= x)
			{
				if (row0->z >= Sprite::zStopper) break;
				dp = store_transp(dp, row0->xa - x);
				x  = row0->xa;
			}

			assert(row0->xa <= x);
			assert(row0->xe > x);
			assert(row0->xe - row0->xa <= 255);

			// find row with highest z at current x position:
			// row2 will point to next row starting after current x position
			const HotData* row	= row0;
			const HotData* row2 = row;
			while ((++row2)->xa <= x)
			{
				if (row2->xe > x)		  // still (some pixels) ahead?
					if (row2->z > row->z) // test whether this row has higher z
					{
						row = row2;
						assert(row->xa <= x);
						assert(row->xe > x);
						assert(row->xe - row->xa <= 255);
					}
			}


			// store transparency up to new x position:
			if (row->xa >= x)
			{
				if (row->z >= Sprite::zStopper) break;
				dp = store_transp(dp, row->xa - x);
				x  = row->xa;
			}

			assert(row->xa <= x);
			assert(row->xe > x); // if xa==xe then this fails
			assert(row->xe - row->xa <= 255);
			assert(row2->xa > x);

			// how many pixels to copy?
			int e = row->xe; // e-x pixels to copy
			assert(e - x > 0);
			assert(e - x <= 255);

			// truncate w if supsequently overlapped by row with higher z
			// ultimately truncated by the stopper at screen end:
			while (row2->xa < e)
			{
				if (row2->z > row->z)
				{
					if (row2->xa > x) { e = row2->xa; }
					else if (row2->xe > x)
					{
						row = row2;
						e	= min(e, row->xe);
					}
				}
				row2++;
			}

			// store pixels:
			if (e - x < 0) { printf("x=%i,e=%i,xa=%i,xe=%i\n", x, e, row->xa, row->xe); }
			assert(e - x > 0);
			assert(e - x <= 255);
			dp = store_pixels(dp, row->rows->pixels + (x - row->xa), e - x);
			x  = e;
		}

		return store_eol(dp);
	}
}

inline RAM void HotList::add(const Sprite* sprite, int dy) noexcept
{
	/* insert sprite into sorted hotlist.

	   dy:  lines to skip before inserting
			normally 0
			may be > 0 if a scanline was missed
			may be > 0 if sprite starts above screen

	   handles hotlist overflow => sprite not added, will be not visible in this frame
	   handles sprites starting above screen or starting in missed scanline (dy > 0)
	   handles empty sprites with height = 0 (first line = stopper) => not added
	   handles sprites ending above screen => not added
	   handles sprites ending left of screen => not added

	   does not handle sprites starting right of screen
	*/

	debuginfo();
	assert(is_spin_locked(spinlock));

	const Shape::Row* shape = sprite->shape.rows;

	//uint32 addr = reinterpret_cast<uint32>(shape);
	//if (unlikely(addr < 0x20000000 || addr > 0x20042000 || addr&1))
	//	panic("@shape = 0x%08x\n",addr);

	if (count < count_of(shapes) && !shape->is_at_end())
	{
		// calc x position xa:
		int xa = sprite->x + shape->dx;

		// skip over dy rows if dy != 0:
		if (unlikely(dy))
		{
			assert(dy > 0);
			do {
				shape = shape->next();
				if (unlikely(shape->is_at_end())) return;
				xa += shape->dx;
			}
			while (--dy);
		}

		// find position in sorted hotlist and make room:
		uint i = count++;
		while (i && xa < shapes[i - 1].xa)
		{
			shapes[i] = shapes[i - 1];
			i--;
		}

		// store it:
		shapes[i].rows = shape;
		shapes[i].xa   = xa;
		shapes[i].xe   = xa + shape->width;
		shapes[i].z	   = sprite->z;
	}
}

inline RAM void HotList::add_sprites(int row) noexcept
{
	// add sprites coming into range of scanline:
	// adds sprites which start in the current row
	// adds sprites which started in a previous row and advances shape properly,
	//   e.g. after a missed scanline or for sprites starting above screen

	debuginfo();

	Sprite* s = next_sprite;

	if (s && s->y <= row)
	{
		lock_list();
		s = next_sprite;

		while (s && s->y <= row)
		{
			if (s->x < screen_width && s->x + s->width() > 0) { add(s, row - s->y); }
			s = s->next;
		}

		next_sprite = s;
	}
}

inline RAM void HotList::remove(uint i) noexcept
{
	count -= 1;
	while (i < count)
	{
		shapes[i] = shapes[i + 1];
		i++;
	}
}

inline RAM void HotList::sort() noexcept
{
	debuginfo();

	// sort by xa

	for (int i = int(count) - 2; i >= 0; i--)
	{
		int xa = shapes[i].xa;
		if (xa <= shapes[i + 1].xa) continue;

		// i and i+1 are wrong sorted:

		auto tmp = shapes[i];

		int j = i;
		do {
			shapes[j] = shapes[j + 1];
			j++;
		}
		while (j + 1 < int(count) && xa > shapes[j + 1].xa);

		shapes[j] = tmp;
	}
}

inline RAM void HotList::next_row() noexcept
{
	debuginfo();

	// advance shapes to next row and remove shapes which finished:

	HotData*	   z = &shapes[0];
	const HotData* q = z;
	const HotData* e = z + count;

	for (; q < e; q++)
	{
		const Shape::Row* row = q->z != Sprite::zStopper ? q->rows->next() // advance shape to next row
														   :
														   q->rows; // the stopper has no real data and is not advanced

		if (!row->is_at_end())
		{
			z->rows = row;
			z->xa	= q->xa + row->dx;
			z->xe	= z->xa + row->width;
			z->z	= q->z;
			z++;
		}
	}

	count = uint(z - &shapes[0]);
}

RAM uint Sprites::renderScanline(int row, uint32* data_out) noexcept
{
	debuginfo();
	uint16* buffer = reinterpret_cast<uint16*>(data_out);
	hotlist.row	   = row;

	hotlist.next_row();		  // advance to next row and remove finished shapes
	hotlist.sort();			  // sort sorted list for new x positions of sprite rows
	hotlist.add_sprites(row); // add sprites coming into range of scanline
	//if (hotlist.count == 1) return buffer;// quick exit if nothing to do BUMMER need CMD::EOL because SM will start on IRPT
	hotlist.check();
	uint16* dp = HotList::render_scanline(buffer, hotlist.shapes); // render pixels into buffer

	assert((uint(buffer) & 3) == 0);
	return uint(dp - buffer) / 2; // return pointer behind data written
}

#pragma GCC pop_options


// =============================================================
//		class Sprite
// =============================================================

Sprite* Sprites::displaylist   = nullptr;
int16	Sprites::current_frame = 0;

bool Sprite::is_hot() const noexcept
{
	// test whether sprite currently in hotlist.
	// the result may be outdated on return!
	//
	// to add some time `is_hot()` reports `hot` 2 rows early.

	return hotlist.count > 1 && hotlist.row >= y - 2 && hotlist.row < y + height();
}

void Sprite::wait() const noexcept
{
	for (uint i = 0; i < 100000 / 60 && hotlist.count > 1 && hotlist.row >= y && hotlist.row < y + height(); i++)
	{
		sleep_us(10);
	}
}

void Sprite::unlink() noexcept
{
	assert(is_in_displaylist());
	assert(is_spin_locked(spinlock));

	if (unlikely(hotlist.next_sprite == this)) hotlist.next_sprite = next;

	if (prev) prev->next = next;
	else Sprites::displaylist = next;
	if (next) next->prev = prev;

	next = prev = nullptr;
}

void Sprite::link_after(Sprite* other) noexcept
{
	assert(!is_in_displaylist());
	assert(other && other->is_in_displaylist());
	assert(is_spin_locked(spinlock));

	prev = other;
	next = other->next;
	if (next) next->prev = this;
	other->next = this;
}

void Sprite::link_before(Sprite* other) noexcept
{
	assert(!is_in_displaylist());
	assert(other && other->is_in_displaylist());
	assert(is_spin_locked(spinlock));

	next = other;
	prev = other->prev;
	if (prev) prev->next = this;
	else Sprites::displaylist = this;
	other->prev = this;
}

void Sprite::link() noexcept
{
	debuginfo();
	assert(!is_in_displaylist());
	assert(is_spin_locked(spinlock));

	if (Sprite* p = Sprites::displaylist)
	{
		if (y > p->y)
		{
			while (p->next && y > p->next->y) { p = p->next; }
			link_after(p);
		}
		else // before first:
		{
			link_before(p);
		}
	}
	else { Sprites::displaylist = this; }
}

void Sprite::show() noexcept
{
	lock_list();
	if (!is_in_displaylist()) link();
}

void Sprite::hide(bool wait) noexcept
{
	{
		lock_list();
		if (is_in_displaylist()) unlink();
	}

	if (wait) this->wait();
}

Sprite::~Sprite() noexcept { hide(); }

void Sprite::move_p0(coord new_x, coord new_y) noexcept
{
	debuginfo();
	assert(is_spin_locked(spinlock));

	x = new_x;
	y = new_y;

	Sprite* p;
	if ((p = prev) && new_y < p->y)
	{
		unlink();
		while (p->prev && new_y < p->prev->y) { p = p->prev; }
		link_before(p);
	}
	else if ((p = next) && new_y > p->y)
	{
		unlink();
		while (p->next && new_y > p->next->y) { p = p->next; }
		link_after(p);
	}
}

void Sprite::move(coord new_x, coord new_y) noexcept
{
	debuginfo();
	lock_list();
	move_p0(new_x - shape.hot_x, new_y - shape.hot_y);
}

void Sprite::move(const Point& p) noexcept
{
	debuginfo();
	lock_list();
	move_p0(p.x - shape.hot_x, p.y - shape.hot_y);
}

void Sprite::replace(const Shape& s, bool wait) noexcept
{
	debuginfo();

	{
		coord dx = shape.hot_x - s.hot_x;
		coord dy = shape.hot_y - s.hot_y;

		lock_list();
		shape = s;
		move_p0(x + dx, y + dy);
	}

	if (wait) this->wait();
}

void Sprite::modify(const Shape& s, coord new_x, coord new_y, bool wait) noexcept
{
	debuginfo();

	{
		lock_list();
		shape = s;
		move_p0(new_x - s.hot_x, new_y - s.hot_y);
	}

	if (wait) this->wait();
}

void Sprite::modify(const Shape& s, const Point& p, bool wait) noexcept
{
	debuginfo();

	{
		lock_list();
		shape = s;
		move_p0(p.x - s.hot_x, p.y - s.hot_y);
	}

	if (wait) this->wait();
}


// =============================================================
//		class Sprites
// =============================================================

bool Sprites::hotlist_is_empty() noexcept // static
{
	return hotlist.count <= 1;
}

void Sprites::clear_displaylist() noexcept // static
{
	debuginfo();

	while (Sprite* s = displaylist) s->unlink();
	for (uint i = 0; i < 100000 / 60 && hotlist.count > 1; i++) sleep_us(10);
}

void Sprites::setup(uint plane, coord width, VideoQueue& vq)
{
	// called by VideoController before first vblank()
	//
	// note: we don't clear the displaylist.
	// this is not required because the displaylist is initially empty.
	// This way, after a video mode change, all the previous sprites, e.g. the mouse pointer,
	// which were not actively hidden by the program, will immediately show up again.
	//
	// TODO: currently we use the default buffersize for buffer allocation.
	// this is a safe bet in allmost all cases and typically way too much.

	debuginfo();
	assert(get_core_num() == 1);
	assert(displaylist == nullptr);

	hotlist.screen_width = width;
	if (!spinlock) spinlock = spin_lock_init(uint(spin_lock_claim_unused(true)));

	VideoPlane::setup(plane, width, vq); // use default buffer size for now
}

void Sprites::teardown(uint plane, VideoQueue& vq) noexcept
{
	// called by VideoController
	//
	// note: we don't clear the displaylist. this is not required
	// because all sprites unlink themselves when deleted.
	// This way sprites, e.g. the mouse pointer, will immediately show up again
	// when video is restarted.

	debuginfo();
	assert(get_core_num() == 1);

	VideoPlane::teardown(plane, vq); // delete[] buffers
}

void RAM Sprites::vblank() noexcept
{
	// called by VideoController at start of each frame.
	// calls sprite.vblank() for all sprites in the displaylist.
	//
	// basically all sprite.vblank() are guaranteed to be called
	// except if a sprite is moved from below the current sprite to above the current sprite
	// either by the vblank() of the current sprite itself or concurrently by the other core.
	// then that sprite's vblank() will be missed for this frame.
	//
	// sprite.vblank() and concurrently other core may:
	//  - modify itself (shape, position)
	//  - remove itself from displaylist
	//  - add new sprites to display list
	//  - remove others from displaylist
	//  - modify (shape, position) other sprites with the caveat mentioned above!
	//
	// TODO: lockout when flash not accessible
	//		 by putting this into flash and testing in Scanvideo for vblank in flash?

	debuginfo();
	assert(get_core_num() == 1);

	// clear list early because sprite.hide(true) may wait for a sprite extending below screen
	hotlist.initialize();

	// used to tell which sprites were already called (in case of reordering):
	current_frame += 1;

	// use hotlist.next_sprite because this is taken care for in sprite.unlink().
	// we can use it because while we are in vblank() on core1 no scanlines can be generated.

	Sprite*& next_sprite = hotlist.next_sprite;
	for (next_sprite = displaylist; next_sprite;)
	{
		Sprite* sprite = next_sprite;

		// get `next` early in case sprite removes or reorders itself in display list:
		next_sprite = sprite->next;

		// if it was not yet called (this is expected,
		// but it may be a sprite which reordered itself or was added concurrently)
		if (sprite->last_frame != current_frame)
		{
			sprite->last_frame = current_frame;
			sprite->vblank();
		}
	}

	// reset hotlist.next_sprite for next frame:
	hotlist.next_sprite = displaylist;
}

} // namespace kipili::Video
