// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Sprites.h"
#include "StackInfo.h"
#include "VideoBackend.h"
#include "kilipili_cdefs.h"
#include <pico/platform.h>
#include <pico/stdlib.h>
#include <pico/sync.h>

namespace kio::Video
{

using namespace Graphics;

// all hot video code should go into ram to allow video while flashing.
// also, there should be no const data accessed in hot video code for the same reason.
// the most timecritical things should go into core1 stack page because it is not contended.

#define WRAP(X)	 #X
#define XWRAP(X) WRAP(X)
#define XRAM	 __attribute__((section(".scratch_x.spr" XWRAP(__LINE__))))		// the 4k page with the core1 stack
#define RAM		 __attribute__((section(".time_critical.spr" XWRAP(__LINE__)))) // general ram


static spin_lock_t* displaylist_spinlock = nullptr;

struct Locker
{
	static uint32 state; // status register
	Locker() noexcept { state = spin_lock_blocking(displaylist_spinlock); }
	~Locker() noexcept { spin_unlock(displaylist_spinlock, state); }
};

#define lock_display_list() Locker _locklist


static volatile int hot_row = 0; // set by renderScanline(): the currently displayed screen row

Sprite* Sprites::displaylist = nullptr;


struct HotShape : public Shape<NotAnimated>
{
	int x; // effective x position: x = sprite.x + row.xoffs, adjusted by all previous dx
	int z; // z value
};

static HotShape hot_shapes[MAX_HOT_SPRITES]; // sorted by z
static uint		num_hot_shapes		= 0;
static Sprite* volatile next_sprite = nullptr; // in displaylist. access & modify only when locked


// =============================================================
//		HotList and DisplayList::scanlineRenderer()
// =============================================================


void RAM check_hotlist() noexcept
{
	assert(num_hot_shapes <= int(count_of(hot_shapes)));

	for (uint i = 0; i < num_hot_shapes; i++)
	{
		const HotShape& shape = hot_shapes[i];
		assert(shape.is_pfx());
		assert(i == 0 || hot_shapes[i - 1].z <= shape.z);
		const Color* pixels = shape.pixels + sizeof(Shape<NotAnimated>::PFX) / sizeof(Color);
		assert(shape.width() == 0 || pixels[0].is_opaque());
		assert(shape.width() == 0 || pixels[shape.width() - 1].is_opaque());
	}
}

#pragma GCC push_options
#pragma GCC optimize("O3")

void RAM Sprite::add_to_hotlist(int dy) const noexcept
{
	// insert sprite into sorted hotlist.
	//
	// dy: lines to skip before inserting
	//	   normally 0
	//	   may be > 0 if a scanline was missed
	//	   may be > 0 if sprite starts above screen
	//
	// handles hotlist overflow => sprite not added, will not be visible in this frame
	// handles sprites starting above screen or starting in missed scanline (dy > 0)
	// handles empty sprites with height = 0 (first line = stopper) => not added
	// handles sprites ending above screen => not added

	stackinfo();
	assert(get_core_num() == 1);

	Shape shape = this->shape;

	if (num_hot_shapes < count_of(hot_shapes) && !shape.is_end())
	{
		// calc x position xa:
		int xa = x + shape.dx();

		// skip over dy rows if dy != 0:
		if unlikely (dy)
		{
			assert(dy > 0);
			do {
				shape.next_row();
				if unlikely (shape.is_end()) return;
				xa += shape.dx();
			}
			while (--dy);
		}

		// find position in sorted hotlist and make room:
		uint i = num_hot_shapes++;
		while (i && z < hot_shapes[i - 1].z)
		{
			hot_shapes[i] = hot_shapes[i - 1];
			i--;
		}

		// store it:
		hot_shapes[i].pixels = shape.pixels;
		hot_shapes[i].x		 = xa;
		hot_shapes[i].z		 = z;
	}
}

void XRAM Sprites::renderScanline(int hot_row, uint32* scanline) noexcept
{
	stackinfo();
	assert(get_core_num() == 1);

	kio::Video::hot_row = hot_row;

	// add sprites coming into range of scanline:
	// adds sprites which start in the current row
	// adds sprites which started in a previous row and advances shape properly,
	//   e.g. after a missed scanline or for sprites starting above screen

	Sprite* s = next_sprite;
	while (s && s->y <= hot_row)
	{
		if (s->x < screen_width() && s->x + s->width() > 0) { s->add_to_hotlist(hot_row - s->y); }
		next_sprite = s = s->next;
	}

#ifdef DEBUG
	check_hotlist();
#endif

	// render shapes into framebuffer and
	// advance shapes to next row and remove finished shapes

	HotShape* z = &hot_shapes[0];
	HotShape* q = z;
	HotShape* e = z + num_hot_shapes;

	for (; q < e; q++)
	{
		int x = q->x + q->dx();
		z->x  = x;
	a:
		int count = q->width();
		q->skip_pfx();
		const Color* pixels = q->pixels;
		q->pixels += count;

		{
			int a = x;
			int e = x + count;
			// adjust l+r and go:
			if (a < 0)
			{
				pixels -= a;
				a = 0;
			}
			if (e > screen_width()) e = screen_width();
			while (a < e) scanline[a++] = *pixels++;
		}

		if (q->is_pfx())
		{
			z->z	  = q->z;
			z->pixels = q->pixels;
			z++;
		}
		else if (q->is_skip())
		{
			q->skip_cmd();
			assert(q->is_pfx());
			x += count + q->dx();
			goto a;
		}
		// else is_end: don't advance z and continue with next hot_shape
	}

	num_hot_shapes = uint(z - &hot_shapes[0]);
}

#pragma GCC pop_options


// =============================================================
//		class Sprite
// =============================================================


bool Sprite::is_hot() const noexcept
{
	// test whether sprite currently in hotlist.
	// the result may be outdated on return!
	//
	// to add some time `is_hot()` reports `hot` 2 rows early.

	int row = hot_row;
	return num_hot_shapes != 0 && row >= y - 2 && row < y + height();
}

void Sprite::wait_while_hot() const noexcept
{
	for (uint i = 0; i < 100000 / 60 && num_hot_shapes != 0; i++)
	{
		int row = hot_row;
		if (row < y || row >= y + height()) break;
		sleep_us(10);
	}
}

void Sprite::_unlink() noexcept
{
	assert(is_in_displaylist());
	assert(is_spin_locked(displaylist_spinlock));

	if (prev) prev->next = next;
	else Sprites::displaylist = next;
	__compiler_memory_barrier();
	if (next) next->prev = prev;
	__compiler_memory_barrier();
	prev = nullptr;

	if unlikely (next_sprite == this)
	{
		if (hot_row == 9999)
		{
			// next_sprite.vsync() is in progress.
			//   core0: wait until finished. then proceed
			//   core1: we are called from vsync(). the sprite is deleting itself. go on!

			if (get_core_num() == 0)
				while (next_sprite == this) { sleep_us(1); }
		}
		else
		{
			// we are in active display area and this is the next sprite that will go into context
			// possibly it is right now read by the scanline_renderer.
			// wer are _not_ in the scanline_renderer itself: it doesn't call unlink()
			// and we are _not_ in vsync(): hot_row != 9999
			// => we are in some ordinary program, probably on core 0 but possibly on core1 too.
			// if .y == hot_row then wait for the scanline_renderer to advance
			// else set next_sprite to .next

			while (hot_row == y && next_sprite == this) {}
			uint32_t save = save_and_disable_interrupts();
			if (next_sprite == this) next_sprite = next;
			restore_interrupts(save);
		}
	}

	// don't clear .next: vsync() will need it!
	// next = nullptr;
}

void Sprite::_link_after(Sprite* other) noexcept
{
	assert(!is_in_displaylist());
	assert(other && other->is_in_displaylist());
	assert(is_spin_locked(displaylist_spinlock));

	prev = other;
	next = other->next;
	__compiler_memory_barrier();
	if (next) next->prev = this;
	__compiler_memory_barrier();
	other->next = this;
}

void Sprite::_link_before(Sprite* other) noexcept
{
	assert(!is_in_displaylist());
	assert(other && other->is_in_displaylist());
	assert(is_spin_locked(displaylist_spinlock));

	next = other;
	prev = other->prev;
	__compiler_memory_barrier();
	other->prev = this;
	__compiler_memory_barrier();
	if (prev) prev->next = this;
	else Sprites::displaylist = this;
}

void Sprite::_link() noexcept
{
	stackinfo();
	assert(!is_in_displaylist());
	assert(is_spin_locked(displaylist_spinlock));

	last_frame = current_frame;

	if (Sprite* p = Sprites::displaylist)
	{
		if (y > p->y)
		{
			while (p->next && y > p->next->y) { p = p->next; }
			_link_after(p);
		}
		else // before first:
		{
			_link_before(p);
		}
	}
	else { Sprites::displaylist = this; }
}

void Sprite::_move(coord new_x, coord new_y) noexcept
{
	stackinfo();
	assert(is_spin_locked(displaylist_spinlock));

	this->x = new_x - hot_x();
	this->y = new_y -= hot_y();

	Sprite* p;
	if ((p = prev) && new_y < p->y)
	{
		_unlink();
		while (p->prev && new_y < p->prev->y) { p = p->prev; }
		_link_before(p);
	}
	else if ((p = next) && new_y > p->y)
	{
		_unlink();
		while (p->next && new_y > p->next->y) { p = p->next; }
		_link_after(p);
	}
}

void Sprite::show() noexcept
{
	if (!is_in_displaylist())
	{
		stackinfo();
		lock_display_list();
		_link();
	}
}

void Sprite::hide(bool wait) noexcept
{
	if (is_in_displaylist())
	{
		stackinfo();
		if (wait) wait_while_hot();
		lock_display_list();
		_unlink();
	}
}

Sprite::~Sprite() noexcept { hide(); }

void Sprite::move(coord new_x, coord new_y) noexcept
{
	stackinfo();
	lock_display_list();
	_move(new_x, new_y);
}

void Sprite::move(const Point& p) noexcept
{
	stackinfo();
	lock_display_list();
	_move(p.x, p.y);
}

void Sprite::modify(const Shape& s, coord new_x, coord new_y, bool wait) noexcept
{
	stackinfo();
	if (wait) wait_while_hot(); // => caller can delete old shape safely
	lock_display_list();
	shape = s;
	_move(new_x, new_y);
}

void Sprite::modify(const Shape& s, const Point& p, bool wait) noexcept
{
	stackinfo();
	if (wait) wait_while_hot(); // => caller can delete old shape safely
	lock_display_list();
	shape = s;
	_move(p.x, p.y);
}

void Sprite::replace(const Shape& s, bool wait) noexcept
{
	stackinfo();
	if (wait) wait_while_hot(); // => caller can delete old shape safely
	lock_display_list();
	shape = s;
	_move(xpos(), ypos());
}


// =============================================================
//		class Sprites
// =============================================================

void Sprites::clear_displaylist() noexcept // static
{
	stackinfo();

	{
		lock_display_list();
		Sprite* s2	= displaylist;
		displaylist = nullptr;

		while (Sprite* s = s2)
		{
			s2		= s->next;
			s->next = nullptr;
			s->prev = nullptr;
		}
	}

	for (uint i = 0; i < 1000000 / 60; i++)
	{
		next_sprite = nullptr;
		if (num_hot_shapes == 0) break;

		sleep_us(1);
	}
}

void Sprites::setup(coord width)
{
	// called by VideoController before first vblank()
	//
	// note: we don't clear the displaylist.
	// this is not required because the displaylist is initially empty.
	// This way, after a video mode change, all the previous sprites, e.g. the mouse pointer,
	// which were not actively hidden by the program, will immediately show up again.

	stackinfo();
	assert(get_core_num() == 1);

	if (!displaylist_spinlock) displaylist_spinlock = spin_lock_init(uint(spin_lock_claim_unused(true)));
}

void Sprites::teardown() noexcept
{
	// called by VideoController

	stackinfo();
	assert(get_core_num() == 1);
	clear_displaylist();
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

	stackinfo();
	assert(get_core_num() == 1);

	// clear list early because sprite.hide(true) may wait for a sprite extending below screen
	num_hot_shapes = 0;

	// use hotlist.next_sprite because this is taken care for in sprite.unlink().
	// we can use it because while we are in vblank() on core1 no scanlines can be generated.

	Sprite* volatile& nextsprite = next_sprite;

	hot_row	   = 9999;
	nextsprite = displaylist;
	while (Sprite* sprite = nextsprite)
	{
		// if sprite was not yet called:
		// may happen if a sprite reordered itself or was reordered from core0
		if (sprite->last_frame != current_frame)
		{
			sprite->last_frame = current_frame;
			sprite->vblank();
		}

		nextsprite = sprite->next;
	}

	// reset hotlist.next_sprite for next frame:
	hot_row	   = -9999;
	nextsprite = displaylist;
}


} // namespace kio::Video


/*



































*/
