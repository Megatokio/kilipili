// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "HamImageInfo.h"
#include "Devices/File.h"
#include "little_big_endian.h"
#include <string.h>

namespace kio::Video
{
using namespace Graphics;

Color HamImageInfo::convert_abs_color(int qcolor)
{
	// masks for color component values:
	const int rmask = (1 << rbits) - 1;
	const int gmask = (1 << gbits) - 1;
	const int bmask = (1 << bbits) - 1;
	const int imask = (1 << ibits) - 1;

	int r = ((qcolor >> rshift) & rmask) << (Color::rshift + Color::rbits - rbits);
	int g = ((qcolor >> gshift) & gmask) << (Color::gshift + Color::gbits - gbits);
	int b = ((qcolor >> bshift) & bmask) << (Color::bshift + Color::bbits - bbits);
	int i = ((qcolor >> ishift) & imask) << (Color::ishift + Color::ibits - ibits);

	return r | g | b | i;
}

Color HamImageInfo::convert_rel_color(int qcolor)
{
	// besides that we must shuffle around the components and possibly add a low bit here or there,
	// we must handle the overflow from one component into another.
	// the qcolor we get has overflow from one component to the one above compensated.
	// in order to undo this, we need to know whether a component is intended to be added or subtracted
	// from the previous pixel's component.
	// technically this cannot be decided, we need to see some of it's actual use cases in the image.
	// but if we assume that the rel_map[] in the encoder only allows for rel_max = ±(comp_max-1)/2,
	// then it's the pos. or neg. offset closer to +0; e.g. in a 3-bit component 3 = +3 but 5 = -3.
	// though some (exotic) color models, let's say rgbi3331, can not possibly comply with this requirement.

	// masks for the msbit in each qcolor component:
	int rmsb = (1 << rbits >> 1) << rshift;
	int gmsb = (1 << gbits >> 1) << gshift;
	int bmsb = (1 << bbits >> 1) << bshift;
	int imsb = (1 << ibits >> 1) << ishift;

	// a qcolor which should work with all possible offsets
	// if the above assumption is met:
	int mid_grey = rmsb | gmsb | bmsb | imsb;

	Color c1 = convert_abs_color(mid_grey);
	Color c2 = convert_abs_color(mid_grey + qcolor);
	return Color(c2.raw - c1.raw);
}

HamImageInfo::HamImageInfo(File* file)
{
	// read file up to pixel data

	static constexpr uint32 rgb8_magic = 3109478632u;

	static_assert(little_endian);
	file->read(this, 4 + 4 + 2 + 2 + 8 * 1 + 2 + 2);

	if (magic != rgb8_magic) throw "no rgb8 file";
	if (memcmp(rgbstr, "rgb", 4) != 0) throw "rgb8 file corrupted";
	if (num_abs_codes + num_rel_codes > 256) throw "rgb8 file corrupted";

	int rmask = ((1 << rbits) - 1) << rshift;
	int gmask = ((1 << gbits) - 1) << gshift;
	int bmask = ((1 << bbits) - 1) << bshift;
	int imask = ((1 << ibits) - 1) << ishift;

	if (rmask == Color::rmask && gmask == Color::gmask && bmask == Color::bmask && imask == Color::imask)
	{
		// this is our own Color model:

		is_native_color = true;
		static_assert(little_endian);
		file->read(cmap, sizeof(cmap));
		return;
	}

	if ((rmask & gmask) || ((rmask | gmask) & bmask) || ((rmask | gmask | bmask) & imask)) throw "rgb8 file corrupted";
	if (rbits + rshift > 16 || gbits + gshift > 16 || bbits + bshift > 16 || ibits + ishift > 16)
		throw "rgb8 file corrupted";

	// due to the way rel_colors are added we cannot ignore lost low bits.
	// the image still can be displayed if it is decoded into a true color pixmap, but not in real-time.
	if (ibits && Color::rbits != rbits) throw "rgb8 image has incompatible color model";
	if (Color::ibits != ibits || Color::rbits < rbits || Color::gbits < gbits || Color::bbits < bbits)
		throw "rgb8 image has incompatible color model";

	// each color component is not larger than our Color component, so we can convert the cmap[]:
	// read cmap and rearrange the color components, possibly padding low bits:

	bool f2 = (rmask | gmask | bmask | imask) > 0xff; // true if 2 bytes per color

	uint					 sizeof_cmap = (1 + f2) * 256;
	std::unique_ptr<uint8[]> qmap {new uint8[sizeof_cmap]};
	file->read(qmap.get(), sizeof_cmap);

	uint8*	q  = qmap.get();
	uint16* q2 = reinterpret_cast<uint16*>(q);
	for (uint i = 0; i < num_abs_codes; i++) cmap[i] = convert_abs_color(f2 ? q2[i] : q[i]);
	for (uint i = 256 - num_rel_codes; i < 256; i++) cmap[i] = convert_rel_color(f2 ? q2[i] : q[i]);
}


} // namespace kio::Video
