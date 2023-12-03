// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "SingleSpritePlane.h"
#include <pico/stdlib.h>
#include <pico/sync.h>

namespace kio::Video
{
using namespace Graphics;

struct HotShapeFoo
{
	bool skip_row() noexcept;
	bool render_row(Color* scanline) noexcept;
	bool is_hot() const noexcept { return f; }
	bool is_end() const noexcept;
	void finish() noexcept {}
	bool f;
};

struct ShapeFoo
{
	static constexpr bool isa_shape = true;
	using HotShape					= HotShapeFoo;
};

struct SpriteFoo
{
	using Shape = ShapeFoo;

	static constexpr bool is_animated = false;
	Point				  pos;
	ShapeFoo			  shape;
	bool				  ghostly;
	short				  _padding;

	SpriteFoo(ShapeFoo, const Point&);
	template<typename HotShape>
	void start(HotShape&) const noexcept
	{}
};

void testFoo() { SingleSpritePlane<SpriteFoo> foo {ShapeFoo(), Point {}}; }


} // namespace kio::Video


/*









































*/
