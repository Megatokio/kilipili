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
class Sprite;


template<ZPlane WZ, Softening SOFT>
class Sprite<WZ, NotAnimated, SOFT>
{
protected:
	Sprite* next = nullptr; // linking in the internal displaylist
	Sprite* prev = nullptr;

public:
	NO_COPY_MOVE(Sprite);
	friend Sprites<WZ, NotAnimated, SOFT>;
	friend Sprites<WZ, Animated, SOFT>;
	using Shape = Video::Shape<SOFT>;

	uint8 width() const noexcept { return shape.preamble().width; }
	uint8 height() const noexcept { return shape.preamble().height; }
	int8  hot_x() const noexcept { return shape.hot_x(); }
	int8  hot_y() const noexcept { return shape.hot_y(); }
	Dist  hotspot() const noexcept { return shape.hotspot(); }
	coord xpos() const noexcept { return pos.x + hot_x(); }
	coord ypos() const noexcept { return pos.y + hot_y(); }
	Point position() const noexcept { return pos + shape.hotspot(); }

	bool is_hot() const noexcept;
	void wait_while_hot() const noexcept;

	Shape shape; // the compressed image of the sprite.
	Point pos;	 // position of top left corner, already adjusted by hot_x.

	uint16 z;				// if HasZ
	bool   ghostly = false; // translucent
	char   _padding;

	Sprite(Shape s, const Point& p, uint16 z = 0) noexcept : shape(s), pos(p - s.hotspot()), z(z) {}
	~Sprite() noexcept = default;
};


// ==============================================================================


template<ZPlane WZ, Softening SOFT>
class Sprite<WZ, Animated, SOFT> : public Sprite<WZ, NotAnimated, SOFT>
{
public:
	using AnimatedShape = Video::AnimatedShape<SOFT>;
	using super			= Sprite<WZ, NotAnimated, SOFT>;

	AnimatedShape animated_shape; // collection of frames of this sprite.
	int16		  countdown;
	uint16		  frame_idx;
};


// ==============================================================================


template<ZPlane WZ, Softening SOFT>
struct HotShape : public Shape<SOFT>
{
	int x; // accumulated x position for current row
};

template<Softening SOFT>
struct HotShape<HasZ, SOFT> : public Shape<SOFT>
{
	int	 x;
	uint z;
};


/*	==============================================================================
	class Sprites is a VideoPlane which can be added to the VideoController
	to display sprites.
*/
template<ZPlane WZ, Animation ANIM, Softening SOFT>
class Sprites;


template<ZPlane WZ, Animation ANIM, Softening SOFT>
class Sprites : public VideoPlane
{
public:
	using Sprite   = Video::Sprite<WZ, ANIM, SOFT>;
	using Shape	   = Video::Shape<SOFT>;
	using HotShape = Video::HotShape<WZ, SOFT>;

	Sprites() noexcept = default;
	virtual ~Sprites() noexcept override;

	virtual void setup(coord width) override;
	virtual void teardown() noexcept override;
	virtual void vblank() noexcept override;
	virtual void renderScanline(int row, uint32* scanline) noexcept override;

	Sprite* add(Sprite* s) throws;
	Sprite* add(Sprite* s, const Point& p, uint8 z = 0) throws;
	Sprite* remove(Sprite* sprite) noexcept;
	void	moveTo(Sprite*, const Point& p) noexcept;

	bool __always_inline is_in_displaylist(Sprite* s) const noexcept { return s->prev || displaylist == s; }
	void				 clear_displaylist(bool delete_sprites = false) noexcept;

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

	void add_to_hotlist(Shape, int x, int dy, uint z) noexcept;
};


// ============================================================================

extern template class Sprites<NoZ, NotAnimated, NotSoftened>;
extern template class Sprites<NoZ, NotAnimated, Softened>;
extern template class Sprites<HasZ, NotAnimated, NotSoftened>;
extern template class Sprites<HasZ, NotAnimated, Softened>;
extern template class Sprites<NoZ, Animated, NotSoftened>;
extern template class Sprites<NoZ, Animated, Softened>;
extern template class Sprites<HasZ, Animated, NotSoftened>;
extern template class Sprites<HasZ, Animated, Softened>;

} // namespace kio::Video

/*


































*/
