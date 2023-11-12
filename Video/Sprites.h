// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Shape.h"
#include "VideoPlane.h"
#include "no_copy_move.h"
#include <pico/sync.h>

namespace kio::Video
{
using coord = Graphics::coord;
using Point = Graphics::Point;

extern spin_lock_t* displaylist_spinlock;
extern bool			hotlist_overflow; // for test by the application

enum ZPlane : bool { NoZ, HasZ };

template<ZPlane, Animation, Softening>
class Sprites;


/*
	Sprites are ghostly images which hover above a regular video image.

	Displaying sprites is quite cpu intensive.  
	Eventually the most popular use of a sprite is the display of a mouse pointer.  
	Sprites don't manage the lifetime of the shape => they don't delete it in ~Sprite().  
*/
template<ZPlane WZ, Animation ANIM, Softening SOFT>
class Sprite
{
	Sprite* next = nullptr; // linking in the internal displaylist
	Sprite* prev = nullptr;

public:
	NO_COPY_MOVE(Sprite);
	friend Sprites<WZ, ANIM, SOFT>;
	using Shape = Video::Shape<ANIM, SOFT>;

	uint8 width() const noexcept { return shape.preamble().width; }
	uint8 height() const noexcept { return shape.preamble().height; }
	coord xpos() const noexcept { return x + shape.hot_x(); }
	coord ypos() const noexcept { return y + shape.hot_y(); }

	bool is_hot() const noexcept;
	void wait_while_hot() const noexcept;

	Shape shape; // the compressed image of the sprite.
	coord x = 0; // position of top left corner, already adjusted by hot_x.
	coord y = 0; // position of top left corner, already adjusted by hot_y.

	uint8 z;			   // if HasZ
	uint8 countdown;	   // if Animated
	bool  ghostly = false; // blend with 50& opacity
	char  _padding;

private:
	Sprite(Shape s, int x, int y, uint8 z) noexcept : shape(s), x(x), y(y), z(z) {}
	~Sprite() noexcept = default;
};


template<ZPlane WZ, Animation ANIM, Softening SOFT>
struct HotShape : public Shape<ANIM, SOFT>
{
	int x; // accumulated x position of current row
};
template<Animation ANIM, Softening SOFT>
struct HotShape<HasZ, ANIM, SOFT> : public HotShape<NoZ, ANIM, SOFT>
{
	uint z;
};


/*	==============================================================================
	class Sprites is a VideoPlane which can be added to the VideoController
	to display sprites.
*/
template<ZPlane WZ, Animation ANIM, Softening SOFT>
class Sprites : public VideoPlane
{
public:
	using Sprite   = Video::Sprite<WZ, ANIM, SOFT>;
	using Shape	   = Video::Shape<ANIM, SOFT>;
	using HotShape = Video::HotShape<WZ, ANIM, SOFT>;

	Sprites() noexcept = default;
	virtual ~Sprites() noexcept override;

	virtual void setup(coord width) override;
	virtual void teardown() noexcept override;
	virtual void vblank() noexcept override;
	virtual void renderScanline(int row, uint32* scanline) noexcept override;

	Sprite* add(Shape s, int x, int y, uint8 z = 0) throws;
	void	remove(Sprite* sprite) noexcept;
	void	move(Sprite*, coord x, coord y) noexcept;
	void	move(Sprite*, const Point& p) noexcept;

	bool is_in_displaylist(Sprite* s) const noexcept { return s->prev || displaylist == s; }
	void clear_displaylist() noexcept;

private:
	void _unlink(Sprite*) noexcept;
	void _link_after(Sprite*, Sprite* other) noexcept;
	void _link_before(Sprite*, Sprite* other) noexcept;
	void _link(Sprite*) noexcept;
	void _move(Sprite*, coord new_x, coord new_y) noexcept;

	struct Lock
	{
		uint32 status_register;
		Lock() noexcept { status_register = spin_lock_blocking(displaylist_spinlock); }
		~Lock() noexcept { spin_unlock(displaylist_spinlock, status_register); }
	};

	Sprite* displaylist			 = nullptr;
	Sprite* volatile next_sprite = nullptr;
	HotShape* hotlist			 = nullptr;
	uint	  max_hot			 = 0;
	uint	  num_hot			 = 0;

	void add_to_hotlist(Shape, int x, int dy, uint8 z) noexcept;
};


// ============================================================================

extern template class Sprites<NoZ, NotAnimated, NotSoftened>;
extern template class Sprites<NoZ, NotAnimated, Softened>;
extern template class Sprites<NoZ, Animated, NotSoftened>;
extern template class Sprites<NoZ, Animated, Softened>;
extern template class Sprites<HasZ, NotAnimated, NotSoftened>;
extern template class Sprites<HasZ, NotAnimated, Softened>;
extern template class Sprites<HasZ, Animated, NotSoftened>;
extern template class Sprites<HasZ, Animated, Softened>;

} // namespace kio::Video

/*


































*/
