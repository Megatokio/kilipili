// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "basic_math.h"
#include "cdefs.h"
#include <utility>


namespace kio
{

using coord = int;

template<typename T>
inline void swap(T& a, T& b)
{
	std::swap(a, b);
}
template<typename T>
inline void sort(T& a, T& b)
{
	if (a > b) swap(a, b);
}


struct Point
{
	coord x, y;

	Point() noexcept = default;
	constexpr Point(coord x, coord y) noexcept : x(x), y(y) {}
	Point& operator+=(const struct Dist&) noexcept;
	Point& operator-=(const struct Dist&) noexcept;
};

struct Dist
{
	coord dx, dy;

	Dist() noexcept = default;
	constexpr Dist(coord w, coord h) noexcept : dx(w), dy(h) {}
};

inline Point  operator+(const Point& p, const Dist& d) noexcept { return Point(p.x + d.dx, p.y + d.dy); }
inline Point  operator-(const Point& p, const Dist& d) noexcept { return Point(p.x - d.dx, p.y - d.dy); }
inline Dist	  operator-(const Point& a, const Point& b) noexcept { return Dist(a.x - b.x, a.y - b.y); }
inline Dist	  operator-(const Dist& a, const Dist& b) noexcept { return Dist(a.dx - b.dx, a.dy - b.dy); }
inline Dist	  operator+(const Dist& a, const Dist& b) noexcept { return Dist(a.dx + b.dx, a.dy + b.dy); }
inline Point& Point::operator+=(const struct Dist& d) noexcept
{
	x += d.dx;
	y += d.dy;
	return *this;
}
inline Point& Point::operator-=(const struct Dist& d) noexcept
{
	x -= d.dx;
	y -= d.dy;
	return *this;
}

struct Size
{
	coord width, height;

	Size() noexcept = default;
	constexpr Size(coord w, coord h) noexcept : width(w), height(h) {}
	Size(const Dist& d) noexcept : width(d.dx), height(d.dy) {}
		 operator Dist() const noexcept { return Dist(width, height); }
	bool operator==(const Size& other) const noexcept { return width == other.width && height == other.height; }
};

inline Point operator+(const Point& p, const Size& d) noexcept { return Point(p.x + d.width, p.y + d.height); }

struct Rect
{
	Point p1;												// top-left
	Point p2;												// bottom-right
	Point p3() const noexcept { return Point(p1.x, p2.y); } // bottom-left
	Point p4() const noexcept { return Point(p2.x, p1.y); } // top-right

	Rect() noexcept = default;
	Rect(coord x, coord y, coord w, coord h) noexcept : p1(x, y), p2(x + w, y + h) { normalize(); }
	Rect(const Point& p, const Size& d) noexcept : p1(p), p2(p1 + d) { normalize(); }
	Rect(const Point& a, const Point& b) noexcept : p1(a), p2(b) { normalize(); }

	void normalize() noexcept
	{
		sort(p1.x, p2.x);
		sort(p1.y, p2.y);
	}
	bool is_normalized() const noexcept { return p1.x <= p2.x && p1.y <= p2.y; }
	bool is_empty() const noexcept { return p1.x == p2.x || p1.y == p2.y; }

	coord left() const noexcept { return p1.x; }
	coord right() const noexcept { return p2.x; }
	coord top() const noexcept { return p1.y; }
	coord bottom() const noexcept { return p2.y; }
	coord width() const noexcept { return p2.x - p1.x; }
	coord height() const noexcept { return p2.y - p1.y; }
	Size  size() const noexcept { return p2 - p1; }

	Point top_left() const noexcept { return p1; }
	Point bottom_right() const noexcept { return p2; }
	Point bottom_left() const noexcept { return p3(); }
	Point top_right() const noexcept { return p4(); }

	Rect& unite_with(const Rect& b) noexcept
	{
		assert(is_normalized());
		assert(b.is_normalized());

		p1.x = min(p1.x, b.p1.x);
		p1.y = min(p1.y, b.p1.y);
		p2.x = max(p2.x, b.p2.x);
		p2.y = max(p2.y, b.p2.y);
		return *this;
	}

	bool intersects_with(const Rect& q) const noexcept
	{
		// note: returns true if either of both is empty and inside (not only on boundary) of the other

		return !(
			p1.x >= q.p2.x || p2.x <= q.p1.x || p1.y >= q.p2.y || p2.y <= q.p1.y
			//  || is_empty() || q.is_empty()
		);
	}

	Rect& intersect_with(const Rect& b) noexcept
	{
		assert(is_normalized());
		assert(b.is_normalized());

		p1.x = max(p1.x, b.p1.x);
		p1.y = max(p1.y, b.p1.y);
		p2.x = min(p2.x, b.p2.x);
		p2.y = min(p2.y, b.p2.y);
		if (!is_normalized()) p2 = p1; // => empty
		return *this;
	}

	friend Rect united(Rect a, const Rect& b) noexcept { return a.unite_with(b); }
	friend Rect intersected(Rect a, const Rect& b) noexcept { return a.intersect_with(b); }
};


} // namespace kio
