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


spin_lock_t*		displaylist_spinlock = nullptr;
static volatile int hot_row				 = 0; // set by renderScanline(): the currently displayed screen row
bool				hotlist_overflow	 = false;


// =======================================================================
// Sprite:

template<ZPlane WZ, Animation ANIM, Softening SOFT>
bool Sprite<WZ, ANIM, SOFT>::is_hot() const noexcept
{
	return hot_row >= y && hot_row < y + height();
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprite<WZ, ANIM, SOFT>::wait_while_hot() const noexcept
{
	while (is_hot()) sleep_us(500);
}


// =======================================================================
// Sprites:

template<ZPlane WZ, Animation ANIM, Softening SOFT>
Sprites<WZ, ANIM, SOFT>::~Sprites() noexcept
{
	assert(displaylist == nullptr); // else teardown not called => plane still in planes[] ?
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::Sprites::setup(coord __unused width)
{
	// called by VideoController before first vblank()
	// we don't clear the displaylist and keep all sprites if there are already any

	if (!displaylist_spinlock) displaylist_spinlock = spin_lock_init(uint(spin_lock_claim_unused(true)));
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::Sprites::teardown() noexcept
{
	// called by VideoController

	stackinfo();
	assert(get_core_num() == 1);

	clear_displaylist();
	assert(displaylist == nullptr);
}


template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::Sprites::_unlink(Sprite* s) noexcept
{
	assert(is_in_displaylist(s));
	assert(is_spin_locked(displaylist_spinlock));

	if unlikely (next_sprite == s) next_sprite = s->next;

	if (s->prev) s->prev->next = s->next;
	else displaylist = s->next;
	s->prev = nullptr;

	if (s->next) s->next->prev = s->prev;
	// s->next = nullptr;	  don't clear s->next: vblank() may need it!
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::Sprites::_link_after(Sprite* s, Sprite* other) noexcept
{
	assert(!is_in_displaylist(s));
	assert(other && is_in_displaylist(other));
	assert(is_spin_locked(displaylist_spinlock));

	s->prev = other;
	s->next = other->next;

	if (s->next) s->next->prev = s;
	other->next = s;
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::Sprites::_link_before(Sprite* s, Sprite* other) noexcept
{
	assert(!is_in_displaylist(s));
	assert(other && is_in_displaylist(other));
	assert(is_spin_locked(displaylist_spinlock));

	s->next = other;
	s->prev = other->prev;

	other->prev = s;
	if (s->prev) s->prev->next = s;
	else displaylist = s;
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::Sprites::_link(Sprite* s) noexcept
{
	stackinfo();
	assert(!is_in_displaylist(s));
	assert(is_spin_locked(displaylist_spinlock));

	Sprite* other = displaylist;
	int		y	  = s->y;

	if (other && y > other->y)
	{
		Sprite* next;
		while ((next = other->next) && y > next->y) { other = next; }
		_link_after(s, other);
	}
	else
	{
		s->next		= displaylist;
		s->prev		= nullptr;
		displaylist = s;
	}
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::Sprites::_move(Sprite* s, coord x, coord y) noexcept
{
	stackinfo();
	assert(is_spin_locked(displaylist_spinlock));

	s->x = x;
	s->y = y;

	Sprite* other;
	if ((other = s->prev) && y < other->y)
	{
		_unlink(s);
		Sprite* prev = other->prev;
		do other = prev;
		while ((prev = other->prev) && y < prev->y);
		_link_before(s, other);
	}
	else if ((other = s->next) && y > other->y)
	{
		_unlink(s);
		Sprite* next = other->next;
		do other = next;
		while ((next = other->next) && y > next->y);
		_link_after(s, other);
	}
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
Sprite<WZ, ANIM, SOFT>* Sprites<WZ, ANIM, SOFT>::Sprites::add(Shape shape, int x, int y, uint8 z) throws
{
	stackinfo();

	Sprite* s = new Sprite(shape, x, y, z);
	Lock	_;
	_link(s);
	return s;
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::Sprites::remove(Sprite* s) noexcept
{
	stackinfo();
	assert(is_in_displaylist(s));

	{
		Lock _;
		_unlink(s);
	}
	delete s;
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::move(Sprite* s, coord x, coord y) noexcept
{
	stackinfo();
	assert(is_in_displaylist(s));

	Lock _;
	_move(s, x, y);
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::move(Sprite* s, const Point& p) noexcept
{
	stackinfo();
	assert(is_in_displaylist(s));

	Lock _;
	_move(s, p.x, p.y);
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::Sprites::clear_displaylist() noexcept
{
	stackinfo();

	while (displaylist)
	{
		Sprite* s;
		{
			Lock _;
			s = displaylist;
			if (s) _unlink(s);
		}
		delete s;
	}
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::vblank() noexcept
{
	// called by VideoController before first renderScanline().
	// called by VideoController at start of each frame.
	// variants: with and without animation.

	stackinfo();
	assert(get_core_num() == 1);

	num_hot		= 0;
	hot_row		= -9999;
	next_sprite = displaylist;

	if constexpr (ANIM == Animated)
	{
		// in a RC the other thread may have just unlinked the sprite.
		// remove(): the sprite will be deleted and subsequently overwritten.
		//			 but sprite.next was not nulled and can be used if we act fast!
		// move():   we will miss this animation. depending on whether the sprite is moved up or down
		//			 animations for other sprites will be done twice or missed as well.

		for (Sprite* sprite = displaylist; sprite; sprite = sprite->next)
		{
			if (--sprite->countdown > 0) continue;

			Lock _;

			if (is_in_displaylist(sprite))
			{
				Shape& s		  = sprite->shape;
				int	   x		  = sprite->x + s.hot_x();
				int	   y		  = sprite->y + s.hot_y();
				s				  = s.next_frame();
				sprite->countdown = s.duration() - 1;
				_move(sprite, x - s.hot_x(), y - s.hot_y());
			}
		}
	}
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::add_to_hotlist(Shape shape, int x, int dy, uint8 z) noexcept
{
	// add shape to the hotlist.
	// variants: with or without Z
	//			 with or without animation

	if unlikely (num_hot == max_hot)
	{
		hotlist_overflow = true;
		return;
	}

	shape.skip_preamble();

	for (; dy < 0; dy++) // skip over dy rows if dy < 0:
	{
		shape.skip_row(x);
		if unlikely (shape.is_end()) return;
	}

	uint idx = num_hot++;

	if constexpr (WZ == HasZ)
	{
		while (idx && z < hotlist[idx - 1].z)
		{
			hotlist[idx] = hotlist[idx - 1];
			idx--;
		}
	}

	assert(shape.is_pfx());
	hotlist[idx].x		= x;
	hotlist[idx].pixels = shape.pixels;
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::renderScanline(int hot_row, uint32* scanline) noexcept
{
	// render all sprites into the scanline
	// variants: with or without Z --> add_to_hotlist()
	// variants: with or without animation --> add_to_hotlist()
	// variants: normal or softened --> hot_shape.render_one_row()
	// flag:     ghostly --> hot_shape.render_one_row()

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
		if (s->x < screen_width() && s->x + s->width() > 0) { add_to_hotlist(s->shape, s->x, hot_row - s->y, s->z); }
		next_sprite = s = s->next;
	}

	// render shapes into framebuffer and
	// advance shapes to next row and remove finished shapes

	for (uint i = num_hot; i;)
	{
		HotShape& hot_shape = hotlist[--i];
		bool	  finished	= hot_shape.render_one_row(hot_shape.x, reinterpret_cast<Color*>(scanline), s->ghostly);
		if unlikely (finished) hot_shape = hotlist[--num_hot];
	}
}

template class Sprites<NoZ, NotAnimated, NotSoftened>;
template class Sprites<NoZ, NotAnimated, Softened>;
template class Sprites<NoZ, Animated, NotSoftened>;
template class Sprites<NoZ, Animated, Softened>;
template class Sprites<HasZ, NotAnimated, NotSoftened>;
template class Sprites<HasZ, NotAnimated, Softened>;
template class Sprites<HasZ, Animated, NotSoftened>;
template class Sprites<HasZ, Animated, Softened>;

} // namespace kio::Video


/*



































*/
