// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "kilipili_cdefs.h"
#include "standard_types.h"
#include "string.h"
#include <utility>

namespace kio::Video
{

template<typename Shape>
struct Frame
{
	Shape  shape;
	uint16 duration = 0;
};


/* ————————————————————————————————————————————————————————————————————
	struct Frames<> is a vector of Frames.
*/
template<typename Shape>
struct Frames
{
	using Frame = Video::Frame<Shape>;

	Frame* frames	  = nullptr;
	uint   num_frames = 0;

	Frames() noexcept = default;												   // ctor
	Frames(uint num_frames) noexcept;											   // ctor
	Frames(Frame* frames, uint8 num_frames) noexcept;							   // ctor
	Frames(const Shape* shapes, const uint16* durations, uint8 num_frames) throws; // ctor
	Frames(const Shape* shapes, uint16 duration, uint8 num_frames) throws;		   // ctor
	Frames(Frames&& q) noexcept;												   // move
	Frames(const Frames& q) throws;												   // copy
	Frames& operator=(Frames&& q) noexcept;
	Frames& operator=(const Frames& q) throws;
	~Frames() noexcept { dealloc(); }

	const Frame& operator[](uint i) const noexcept { return frames[i]; }

	void replace(const Frames& new_frames) throws;
	void replace(Frames&& new_frames) noexcept;
	void replace(const Frame* new_frames, uint new_num_frames) throws;
	void replace(const Shape* new_shapes, const uint16* new_durations, uint new_num_frames) throws;
	void replace(const Shape* new_shapes, uint16 new_duration, uint new_num_frames) throws;

	void dealloc() noexcept;

private:
	void alloc(uint count) throws;
	void alloc_and_copy(const Shape* shapes, const uint16* durations, uint count) throws;
	void alloc_and_copy(const Shape* shapes, uint16 duration, uint count) throws;
	void alloc_and_copy(const Frame* frames, uint count) throws;
};


//
// ======================== Implementations ===========================
//

template<typename Shape>
Frames<Shape>::Frames(uint num_frames) noexcept // ctor
{
	alloc(num_frames);
	memset(frames, 0, num_frames * sizeof(Frame));
}

template<typename Shape>
Frames<Shape>::Frames(Frame* frames, uint8 num_frames) noexcept // ctor
{
	alloc_and_copy(frames, num_frames);
}

template<typename Shape>
Frames<Shape>::Frames(const Shape* shapes, const uint16* durations, uint8 num_frames) throws // ctor
{
	alloc_and_copy(shapes, durations, num_frames);
}

template<typename Shape>
Frames<Shape>::Frames(const Shape* shapes, uint16 duration, uint8 num_frames) throws // ctor
{
	alloc_and_copy(shapes, duration, num_frames);
}

template<typename Shape>
Frames<Shape>::Frames(Frames&& q) noexcept : frames(q.frames), num_frames(q.num_frames) // move
{
	q.frames	 = nullptr;
	q.num_frames = 0;
}

template<typename Shape>
Frames<Shape>::Frames(const Frames& q) throws // copy
{
	alloc_and_copy(q.frames, q.num_frames);
}

template<typename Shape>
Frames<Shape>& Frames<Shape>::operator=(Frames&& q) noexcept
{
	std::swap(q.num_frames, num_frames);
	std::swap(q.frames, frames);
	return *this;
}

template<typename Shape>
Frames<Shape>& Frames<Shape>::operator=(const Frames& q) throws
{
	if (this != &q) replace(q.frames, q.num_frames);
	return *this;
}

template<typename Shape>
void Frames<Shape>::replace(const Frames& new_frames) throws
{
	replace(new_frames.frames, new_frames.num_frames);
}

template<typename Shape>
void Frames<Shape>::replace(Frames&& new_frames) noexcept
{
	std::swap(new_frames.frames, frames);
	std::swap(new_frames.num_frames, num_frames);
}

template<typename Shape>
void Frames<Shape>::replace(const Frame* new_frames, uint new_num_frames) throws
{
	if (new_num_frames == num_frames)
	{
		for (uint i = 0; i < new_num_frames; i++) frames[i] = new_frames[i];
	}
	else
	{
		dealloc();
		alloc_and_copy(new_frames, new_num_frames);
	}
}

template<typename Shape>
void Frames<Shape>::replace(const Shape* new_shapes, const uint16* new_durations, uint new_num_frames) throws
{
	if (new_num_frames == num_frames)
	{
		for (uint i = 0; i < new_num_frames; i++) frames[i] = Frame(new_shapes[i], new_durations[i]);
	}
	else
	{
		dealloc();
		alloc_and_copy(new_shapes, new_durations, new_num_frames);
	}
}

template<typename Shape>
void Frames<Shape>::replace(const Shape* new_shapes, uint16 new_duration, uint new_num_frames) throws
{
	if (new_num_frames == num_frames)
	{
		for (uint i = 0; i < new_num_frames; i++) frames[i] = Frame(new_shapes[i], new_duration);
	}
	else
	{
		dealloc();
		alloc_and_copy(new_shapes, new_duration, new_num_frames);
	}
}

template<typename Shape>
void Frames<Shape>::dealloc() noexcept
{
	while (num_frames) frames[--num_frames].~Frame();
	delete[] ptr(frames);
	frames = nullptr;
}

template<typename Shape>
void Frames<Shape>::alloc(uint count) throws
{
	static_assert(sizeof(Frame) % sizeof(ptr) == 0);
	frames	   = reinterpret_cast<Frame*>(new char[count * sizeof(Frame)]);
	num_frames = count;
}

template<typename Shape>
void Frames<Shape>::alloc_and_copy(const Shape* shapes, const uint16* durations, uint count) throws
{
	alloc(count);
	for (uint i = 0; i < count; i++) new (&frames[i]) Frame(shapes[i], durations[i]);
}

template<typename Shape>
void Frames<Shape>::alloc_and_copy(const Shape* shapes, uint16 duration, uint count) throws
{
	alloc(count);
	for (uint i = 0; i < count; i++) new (&frames[i]) Frame(shapes[i], duration);
}

template<typename Shape>
void Frames<Shape>::alloc_and_copy(const Frame* frames, uint count) throws
{
	alloc(count);
	for (uint i = 0; i < count; i++) new (&this->frames[i]) Frame(frames[i]);
}


} // namespace kio::Video
