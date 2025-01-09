// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Frames.h"
#include "Sprite.h"
#include "geometry.h"
#include "string.h"

namespace kio::Video
{

/* ————————————————————————————————————————————————————————————
	Animated Sprite:
	An AnimatedSprite is a Sprite with automatic animation.
	VideoPlane SingleSpritePlane<> and MultiSpritePlane<> 
	are based on any class Sprite which may be an AnimatedSprite.
	
	Because template class MultiSpritePlane<> is based on the exact Sprite class 
	a MultiSpritePlane<AnimatedSprite> can only contain AnimatedSprites.
	But a AnimatedSprite can consist of only one Frame, so a 
	MultiSpritePlane<AnimatedSprite> can seemingly mix animated and not-animated Sprites.

	An AnimatedSprite allocates one chunk on the heap, except for AnimatedSprites with only one Frame.
	Replacing the Frames of an AnimatedSprite with new Frames results in deallocation and 
	reallocation of this heap memory, except if old and new number of frames are the same.
*/
template<typename SHAPE>
class AnimatedSprite : public Sprite<SHAPE>
{
public:
	using super	 = Sprite<SHAPE>;
	using Shape	 = SHAPE;
	using Frame	 = Video::Frame<Shape>;
	using Frames = Video::Frames<Shape>;

	static constexpr bool is_animated = true; // helper for templates
	static constexpr bool isa_sprite  = true;
	static_assert(Shape::isa_shape);

	using super::current_frame;
	uint16 countdown = 0xffff;
	Frames frames;

	/*
		create an AnimatedSprite from Frames<>.
		The AnimatedSprite copies the data from the Frames. (does not take ownership of the vectors)
		If frames.num_frames==1 then you should use `AnimatedSprite(Shape,x,y,z)` instead.
	*/
	AnimatedSprite(const Frames& frames, const Point&, uint16 z = 0) throws;
	AnimatedSprite(Frames&& frames, const Point&, uint16 z = 0) throws;
	AnimatedSprite(const Frame* frames, uint8 num_frames, const Point&, uint16 z = 0) throws;
	AnimatedSprite(const Shape* shapes, const uint16* durations, uint8 num_frames, const Point& p, uint16 z = 0) throws;
	AnimatedSprite(const Shape* shapes, uint16 duration, uint8 num_frames, const Point& p, uint16 z = 0) throws;

	/*
		create an AnimatedSprite with only one frame.
		This makes no allocation on the heap and thus can't fail. (doesn't throw)
	*/
	AnimatedSprite(const Shape& shape, const Point& p, uint16 z = 0) noexcept; // ctor

	~AnimatedSprite() noexcept = default;

	/*
		replace the Frames of this Sprite with a new set of Frames.
		The AnimatedSprite copies the data from the vector. (does not take ownership)
		If num_frames==1 then you should use `replace(Shape)` instead.
		Returns true if hot_y changed and the Sprite may need re-linking.
	*/
	bool replace(const Frames& new_frames) throws;
	bool replace(Frames&& new_frames) throws;
	bool replace(const Frame* new_frames, uint new_num_frames) throws;
	bool replace(const Shape* new_shapes, const uint16* new_durations, uint new_num_frames) throws;
	bool replace(const Shape* new_shapes, uint16 new_duration, uint new_num_frames) throws;

	/*
		replace the Frames of this Sprite with a new single Shape and no animation.
		This makes no allocation on the heap and thus can't fail. (doesn't throw)		
		Returns true if hot_y changed and the Sprite may need re-linking.
	*/
	bool replace(const Shape& new_shape) noexcept;

	/*
		advance the animation to the next frame.
		If the Sprite has currently only one frame then nothing happens,
		else the new Shape and it's countdown are loaded.
		Returns true if hot_y changed and the Sprite may need re-linking.
	*/
	bool next_frame() noexcept;
};


//
// ============================ Implementations ================================
//

template<typename Shape>
AnimatedSprite<Shape>::AnimatedSprite(const Frames& frames, const Point& p, uint16 z ) throws : // ctor
	super(frames[0].shape, p, z),
	countdown(frames[0].duration),
	frames(frames)
{}

template<typename Shape>
AnimatedSprite<Shape>::AnimatedSprite(Frames&& frames, const Point&p, uint16 z ) throws : // ctor
	super(frames[0].shape, p, z),
	countdown(frames[0].duration),
	frames(std::move(frames))
{}

template<typename Shape>
AnimatedSprite<Shape>::AnimatedSprite(const Frame* frames, uint8 num_frames, const Point&p, uint16 z ) throws : // ctor
	super(frames[0].shape, p, z),
	countdown(frames[0].duration),
	frames(frames,num_frames)
{}

template<typename Shape>
AnimatedSprite<Shape>::AnimatedSprite(
	const Shape* shapes, const uint16* durations, uint8 num_frames, const Point& p, uint16 z) throws :
	super(shapes[0], p, z),
	countdown(durations[0]),
	frames(shapes, durations, num_frames)
{}

template<typename Shape>
AnimatedSprite<Shape>::AnimatedSprite(
	const Shape* shapes, uint16 duration, uint8 num_frames, const Point& p, uint16 z) throws :
	super(shapes[0], p, z),
	countdown(duration),
	frames(shapes, duration, num_frames)
{}

template<typename Shape>
AnimatedSprite<Shape>::AnimatedSprite(const Shape& shape, const Point& p, uint16 z) noexcept : // ctor
	super(shape, p, z)
{}

template<typename Shape>
bool AnimatedSprite<Shape>::replace(const Frames& new_frames) throws
{
	frames.replace(new_frames);
	current_frame = 0;
	countdown	  = frames[0].duration;
	return super::replace(frames[0].shape);
}

template<typename Shape>
bool AnimatedSprite<Shape>::replace(Frames&& new_frames) throws
{
	frames.replace(std::move(new_frames));
	current_frame = 0;
	countdown	  = frames[0].duration;
	return super::replace(frames[0].shape);
}

template<typename Shape>
bool AnimatedSprite<Shape>::replace(const Frame* new_frames, uint new_num_frames) throws
{
	frames.replace(new_frames, new_num_frames);
	current_frame = 0;
	countdown	  = new_frames[0].duration;
	return super::replace(new_frames[0].shape);
}

template<typename Shape>
bool AnimatedSprite<Shape>::replace(const Shape* new_shapes, const uint16* new_durations, uint new_num_frames) throws
{
	frames.replace(new_shapes, new_durations, new_num_frames);
	current_frame = 0;
	countdown	  = frames[0].duration;
	return super::replace(frames[0].shape);
}

template<typename Shape>
bool AnimatedSprite<Shape>::replace(const Shape* new_shapes, uint16 new_duration, uint new_num_frames) throws
{
	frames.replace(new_shapes, new_duration, new_num_frames);
	current_frame = 0;
	countdown	  = new_duration;
	return super::replace(frames[0].shape);
}

template<typename Shape>
bool AnimatedSprite<Shape>::replace(const Shape& new_shape) noexcept
{
	frames.dealloc();
	return super::replace(new_shape);
}

template<typename Shape>
bool AnimatedSprite<Shape>::next_frame() noexcept
{
	if (frames.num_frames == 0) return false; // no animation
	if (++current_frame >= frames.num_frames) current_frame = 0;
	countdown = frames[current_frame].duration;
	return super::replace(frames[current_frame].shape);
}

} // namespace kio::Video

/*


































*/
