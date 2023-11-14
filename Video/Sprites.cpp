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
static volatile int hot_row				 = 0;	  // the currently displayed screen row
bool				hotlist_overflow	 = false; // set by add_to_hotlist()


// =======================================================================
// Sprite:

template<ZPlane WZ, Softening SOFT>
bool Sprite<WZ, NotAnimated, SOFT>::is_hot() const noexcept
{
	return hot_row >= y && hot_row < y + height();
}

template<ZPlane WZ, Softening SOFT>
void Sprite<WZ, NotAnimated, SOFT>::wait_while_hot() const noexcept
{
	while (is_hot()) sleep_us(500);
}


// =======================================================================
// Sprites:

template<ZPlane WZ, Animation ANIM, Softening SOFT>
Sprites<WZ, ANIM, SOFT>::~Sprites() noexcept
{
	assert(displaylist == nullptr); // else teardown not called => plane still in planes[] ?
	assert(hotlist == nullptr);
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::setup(coord __unused width)
{
	// called by VideoController before first vblank()
	// we don't clear the displaylist and keep all sprites if there are already any

	if (!displaylist_spinlock) displaylist_spinlock = spin_lock_init(uint(spin_lock_claim_unused(true)));

	const uint cnt = 20;
	hotlist		   = new HotShape[cnt];
	max_hot		   = cnt;
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::clear_displaylist(bool delete_sprites) noexcept
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
		if (delete_sprites) delete s;
	}

	Lock _;
	next_sprite = nullptr;
	num_hot		= 0;
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::teardown() noexcept
{
	// called by VideoController

	stackinfo();
	assert(get_core_num() == 1);

	clear_displaylist();

	max_hot = num_hot = 0;
	delete[] hotlist;
	hotlist = nullptr;
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
Sprite<WZ, ANIM, SOFT>* Sprites<WZ, ANIM, SOFT>::add(Sprite* sprite) throws
{
	stackinfo();

	Lock _;
	_link(sprite);
	return sprite;
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
Sprite<WZ, ANIM, SOFT>* Sprites<WZ, ANIM, SOFT>::add(Sprite* sprite, int x, int y, uint8 z) throws
{
	stackinfo();

	sprite->x = x - sprite->hot_x();
	sprite->y = y - sprite->hot_y();
	sprite->z = z;

	return add(sprite);
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
Sprite<WZ, ANIM, SOFT>* Sprites<WZ, ANIM, SOFT>::remove(Sprite* sprite) noexcept
{
	stackinfo();
	assert(is_in_displaylist(sprite));

	Lock _;
	_unlink(sprite);
	return sprite;
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::move(Sprite* s, coord x, coord y) noexcept
{
	stackinfo();
	assert(is_in_displaylist(s));

	Lock _;
	_move(s, x - s->hot_x(), y - s->hot_y());
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void Sprites<WZ, ANIM, SOFT>::move(Sprite* s, const Point& p) noexcept
{
	stackinfo();
	assert(is_in_displaylist(s));

	Lock _;
	_move(s, p.x - s->hot_x(), p.y - s->hot_y());
}


// -------------------------------
// in RAM:

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void RAM Sprites<WZ, ANIM, SOFT>::_unlink(Sprite* s) noexcept
{
	// used in renderScanline()

	assert(is_in_displaylist(s));
	assert(is_spin_locked(displaylist_spinlock));

	if unlikely (next_sprite == s) next_sprite = static_cast<Sprite*>(s->next);

	if (s->prev) s->prev->next = s->next;
	else displaylist = static_cast<Sprite*>(s->next);
	s->prev = nullptr;

	if (s->next) s->next->prev = s->prev;
	// s->next = nullptr;	  don't clear s->next: vblank() may need it!
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void __always_inline Sprites<WZ, ANIM, SOFT>::_link_after(Sprite* s, Sprite* other) noexcept
{
	// used in renderScanline()

	assert(!is_in_displaylist(s));
	assert(other && is_in_displaylist(other));
	assert(is_spin_locked(displaylist_spinlock));

	s->prev = other;
	s->next = other->next;

	if (s->next) s->next->prev = s;
	other->next = s;
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void __always_inline Sprites<WZ, ANIM, SOFT>::_link_before(Sprite* s, Sprite* other) noexcept
{
	// used in renderScanline()

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
void Sprites<WZ, ANIM, SOFT>::_link(Sprite* s) noexcept
{
	stackinfo();
	assert(!is_in_displaylist(s));
	assert(is_spin_locked(displaylist_spinlock));

	Sprite* other = displaylist;
	int		y	  = s->y;

	if (other && y > other->y)
	{
		Sprite* next;
		while ((next = static_cast<Sprite*>(other->next)) && y > next->y) { other = next; }
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
void RAM Sprites<WZ, ANIM, SOFT>::_move(Sprite* s, coord x, coord y) noexcept
{
	// used in renderScanline()

	stackinfo();
	assert(is_spin_locked(displaylist_spinlock));

	s->x = x;
	s->y = y;

	Video::Sprite<WZ, NotAnimated, SOFT>* other;

	if ((other = s->prev) && y < other->y)
	{
		_unlink(s);
		auto* prev = other->prev;
		do other = prev;
		while ((prev = other->prev) && y < prev->y);
		_link_before(s, static_cast<Sprite*>(other));
	}
	else if ((other = s->next) && y > other->y)
	{
		_unlink(s);
		auto* next = other->next;
		do other = next;
		while ((next = other->next) && y > next->y);
		_link_after(s, static_cast<Sprite*>(other));
	}
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void RAM Sprites<WZ, ANIM, SOFT>::add_to_hotlist(Shape shape, int x, int dy, uint z) noexcept
{
	// used in renderScanline()

	if unlikely (num_hot == max_hot)
	{
		hotlist_overflow = true;
		return;
	}

	shape.skip_preamble();

	for (; dy < 0; dy++) // skip over dy rows if dy < 0
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
void RAM Sprites<WZ, ANIM, SOFT>::renderScanline(int hot_row, uint32* scanline) noexcept
{
	stackinfo();
	assert(get_core_num() == 1);

	Video::hot_row = hot_row;

	// add sprites coming into range of scanline:
	// adds sprites which start in the current row
	// adds sprites which started in a previous row and advances shape properly,
	//   e.g. after a missed scanline or for sprites starting above screen

	Sprite* s = next_sprite;
	while (s && s->y <= hot_row)
	{
		if (s->x < screen_width() && s->x + s->width() > 0) { add_to_hotlist(s->shape, s->x, s->y - hot_row, s->z); }
		next_sprite = s = static_cast<Sprite*>(s->next);
	}

	// render shapes into framebuffer and
	// advance shapes to next row and remove finished shapes

	for (uint i = num_hot; i;)
	{
		HotShape& hot_shape = hotlist[--i];
		bool	  finished	= hot_shape.render_row(hot_shape.x, reinterpret_cast<Color*>(scanline), s->ghostly);
		if unlikely (finished) hot_shape = hotlist[--num_hot];
	}
}

template<ZPlane WZ, Animation ANIM, Softening SOFT>
void RAM Sprites<WZ, ANIM, SOFT>::vblank() noexcept
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

		for (Sprite* s = displaylist; s; s = static_cast<Sprite*>(s->next))
		{
			if (--s->countdown > 0) continue;

			Lock _;

			if (is_in_displaylist(s))
			{
				int dx = s->hot_x();
				int dy = s->hot_y();

				if (++s->frame_idx >= s->animated_shape.num_frames) s->frame_idx = 0;
				s->countdown = s->animated_shape.durations[s->frame_idx];
				s->shape	 = s->animated_shape.frames[s->frame_idx];

				dx -= s->hot_x();
				dy -= s->hot_y();

				s->x += dx;
				if (dy) _move(s, s->x, s->y + dy);
			}
		}
	}
}


// the linker will know what we need:
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
