// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "MultiSpritesPlane.h"
#include "Trace.h"
#include "VideoBackend.h"
#include "cdefs.h"
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


bool hotlist_overflow = false; // set by add_to_hotlist()


template<typename Sprite, ZPlane WZ>
void MultiSpritesPlane<Sprite, WZ>::setup()
{
	// called by VideoController before first vblank()
	// we don't clear the displaylist and keep all sprites if there are already any

	if (!sprites_spinlock) sprites_spinlock = spin_lock_init(uint(spin_lock_claim_unused(true)));

	const uint cnt = 20;
	hotlist		   = new HotShape[cnt];
	max_hot		   = cnt;
}

template<typename Sprite, ZPlane WZ>
void MultiSpritesPlane<Sprite, WZ>::clear_displaylist(bool delete_sprites) noexcept
{
	trace(__func__);

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

template<typename Sprite, ZPlane WZ>
void MultiSpritesPlane<Sprite, WZ>::teardown() noexcept
{
	// called by VideoController

	trace(__func__);
	assert(get_core_num() == 1);

	clear_displaylist();

	max_hot = num_hot = 0;
	delete[] hotlist;
	hotlist = nullptr;
}

template<typename Sprite, ZPlane WZ>
Sprite* MultiSpritesPlane<Sprite, WZ>::add(Sprite* sprite) throws
{
	trace(__func__);

	Lock _;
	_link(sprite);
	return sprite;
}

template<typename Sprite, ZPlane WZ>
Sprite* MultiSpritesPlane<Sprite, WZ>::remove(Sprite* sprite) noexcept
{
	trace(__func__);
	assert(is_in_displaylist(sprite));

	Lock _;
	_unlink(sprite);
	return sprite;
}

template<typename Sprite, ZPlane WZ>
void MultiSpritesPlane<Sprite, WZ>::moveTo(Sprite* s, const Point& p) noexcept
{
	trace(__func__);
	assert(is_in_displaylist(s));

	Lock _;
	bool f = p.y != s->get_ypos();
	s->set_position(p);
	if (f) _move(s);
}

template<typename Sprite, ZPlane WZ>
void MultiSpritesPlane<Sprite, WZ>::replace(Sprite* s, const Shape& new_shape) noexcept
{
	if (s->replace(new_shape)) _move(s);
}


// -------------------------------
// in RAM:

template<typename Sprite, ZPlane WZ>
void RAM MultiSpritesPlane<Sprite, WZ>::_unlink(Sprite* s) noexcept
{
	// used in renderScanline()

	assert(is_in_displaylist(s));
	assert(is_spin_locked(sprites_spinlock));

	if unlikely (next_sprite == s) next_sprite = static_cast<Sprite*>(s->next);

	if (s->prev) s->prev->next = s->next;
	else displaylist = static_cast<Sprite*>(s->next);
	s->prev = nullptr;

	if (s->next) s->next->prev = s->prev;
	// s->next = nullptr;	  don't clear s->next: vblank() may need it!
}

template<typename Sprite, ZPlane WZ>
void __always_inline MultiSpritesPlane<Sprite, WZ>::_link_after(Sprite* s, Sprite* other) noexcept
{
	// used in renderScanline()

	assert(!is_in_displaylist(s));
	assert(other && is_in_displaylist(other));
	assert(is_spin_locked(sprites_spinlock));

	s->prev = other;
	s->next = other->next;

	if (s->next) s->next->prev = s;
	other->next = s;
}

template<typename Sprite, ZPlane WZ>
void __always_inline MultiSpritesPlane<Sprite, WZ>::_link_before(Sprite* s, Sprite* other) noexcept
{
	// used in renderScanline()

	assert(!is_in_displaylist(s));
	assert(other && is_in_displaylist(other));
	assert(is_spin_locked(sprites_spinlock));

	s->next = other;
	s->prev = other->prev;

	other->prev = s;
	if (s->prev) s->prev->next = s;
	else displaylist = s;
}

template<typename Sprite, ZPlane WZ>
void MultiSpritesPlane<Sprite, WZ>::_link(Sprite* s) noexcept
{
	trace(__func__);
	assert(!is_in_displaylist(s));
	assert(is_spin_locked(sprites_spinlock));

	Sprite* other = displaylist;
	int		y	  = s->pos.y;

	if (other && y > other->pos.y)
	{
		Sprite* next;
		while ((next = static_cast<Sprite*>(other->next)) && y > next->pos.y) { other = next; }
		_link_after(s, other);
	}
	else
	{
		s->next = displaylist;
		s->prev = nullptr;
		if (s->next) s->next->prev = s;
		displaylist = s;
	}
}

template<typename Sprite, ZPlane WZ>
void RAM MultiSpritesPlane<Sprite, WZ>::_move(Sprite* s) noexcept
{
	trace(__func__);
	assert(is_spin_locked(sprites_spinlock));

	int		y = s->pos.y;
	Sprite* other;

	if ((other = static_cast<Sprite*>(s->prev)) && y < other->pos.y)
	{
		_unlink(s);
		Sprite* prev = static_cast<Sprite*>(other->prev);
		do other = prev;
		while ((prev = static_cast<Sprite*>(other->prev)) && y < prev->pos.y);
		_link_before(s, other);
	}
	else if ((other = static_cast<Sprite*>(s->next)) && y > other->pos.y)
	{
		_unlink(s);
		Sprite* next = static_cast<Sprite*>(other->next);
		do other = next;
		while ((next = static_cast<Sprite*>(other->next)) && y > next->pos.y);
		_link_after(s, other);
	}
}

template<typename Sprite, ZPlane WZ>
void RAM MultiSpritesPlane<Sprite, WZ>::add_to_hotlist(const Sprite* sprite) noexcept
{
	if unlikely (num_hot == max_hot)
	{
		hotlist_overflow = true;
		return;
	}

	uint idx = num_hot++;

	if constexpr (WZ == HasZ)
	{
		uint z = sprite->z;
		while (idx && z > hotlist[idx - 1].z)
		{
			hotlist[idx] = hotlist[idx - 1];
			idx--;
		}
	}

	HotShape& hot_shape = hotlist[idx];
	sprite->start(hot_shape);
	if constexpr (WZ) hot_shape.z = sprite->z;

	if unlikely (sprite->pos.y < hot_row)
	{
		for (int dy = sprite->pos.y - hot_row; dy < 0; dy++)
		{
			bool finished = hot_shape.skip_row();
			if unlikely (finished) return; // TODO: remove from hotlist
		}
	}
}

template<typename Sprite, ZPlane WZ>
void RAM MultiSpritesPlane<Sprite, WZ>::renderScanline(int hot_row, uint32* scanline) noexcept
{
	trace(__func__);
	assert(get_core_num() == 1);

	Video::hot_row = hot_row;

	// add sprites coming into range of scanline:
	// adds sprites which start in the current row
	// adds sprites which started in a previous row and advances shape properly,
	//   e.g. after a missed scanline or for sprites starting above screen

	Sprite* s = next_sprite;
	while (s && s->pos.y <= hot_row)
	{
		if (s->pos.x < screen_width() && s->pos.x + s->width() > 0) { add_to_hotlist(s); }
		next_sprite = s = static_cast<Sprite*>(s->next);
	}

	// render shapes into framebuffer and
	// advance shapes to next row and remove finished shapes
	for (uint i = num_hot; i;)
	{
		HotShape& hot_shape = hotlist[--i];
		bool	  finished	= hot_shape.render_row(reinterpret_cast<Color*>(scanline));
		if unlikely (finished)
		{
			if (WZ == NoZ) //
			{
				hot_shape = hotlist[--num_hot];
			}
			else // WithZ
			{
				for (uint j = i + 1; j < num_hot; j++) hotlist[j - 1] = hotlist[j];
				num_hot -= 1;
			}
		}
	}
}

template<typename Sprite, ZPlane WZ>
void RAM MultiSpritesPlane<Sprite, WZ>::vblank() noexcept
{
	// called by VideoController before first renderScanline().
	// called by VideoController at start of each frame.
	// variants: with and without animation.

	trace(__func__);
	assert(get_core_num() == 1);

	num_hot		= 0;
	hot_row		= -9999;
	next_sprite = displaylist;

	if constexpr (Sprite::is_animated)
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
				if (s->next_frame()) _move(s);
			}
		}
	}
}


// the linker will know what we need:
template class MultiSpritesPlane<Sprite<Shape>, NoZ>;
template class MultiSpritesPlane<Sprite<Shape>, HasZ>;
template class MultiSpritesPlane<Sprite<SoftenedShape>, NoZ>;
template class MultiSpritesPlane<Sprite<SoftenedShape>, HasZ>;
template class MultiSpritesPlane<AnimatedSprite<Shape>, NoZ>;
template class MultiSpritesPlane<AnimatedSprite<Shape>, HasZ>;
template class MultiSpritesPlane<AnimatedSprite<SoftenedShape>, NoZ>;
template class MultiSpritesPlane<AnimatedSprite<SoftenedShape>, HasZ>;

} // namespace kio::Video


/*



































*/
