// Copyright (c) 2007 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Pixelmap.h"
#include "Colormap.h"

namespace kio::Graphics
{

void Pixelmap::init(const Pixelmap& q) throws // copy ctor
{
	data = pixels = new uchar[q.width() * q.height()];
	box			  = q.box;
	dy			  = width();

	for (int y = box.top(); y < box.bottom(); y++) { memcpy(getPixelRow(y), q.getPixelRow(y), dy /*w*/); }
}

int Pixelmap::getMaxColorIndex() const
{
	// Find highest color index in image:
	// returns max_index = 0 for empty Pixmaps

	uchar max = 0;			 // maximum collector
	cuptr p	  = getPixels(); // running pointer

	for (int j = height(); --j >= 0; p += dy)
		for (int i = width(); --i >= 0;)
		{
			if (p[i] > max) max = p[i];
		}

	return max;
}

int Pixelmap::countUsedColors(int* max_color_index) const
{
	// Count used colors (color indexes) in image:
	// if argument max_color_index != NULL, then also store the max. color index found in the image
	// returns 1 used color and sets max_index = 0 for empty Pixmaps

	uchar bits[256];
	memset(bits, 0, 256);  // flags for used colors
	cuptr p = getPixels(); // running pointer

	for (int j = height(); --j >= 0; p += dy)
		for (int i = width(); --i >= 0;)
		{
			bits[p[i]] = 1; // flag all used colors
		}

	int i = 256;
	while (i && bits[--i] == 0) {}
	if (max_color_index) *max_color_index = i; // store max. color index

	int n = 1;
	while (i) { n += bits[--i]; } // count used colors
	return n;
}

void Pixelmap::reduceColors(Colormap& cmap)
{
	// Renumber colors in image to reduce color table size:
	// overwrites this.data[] and cmap[] with new matching indexes/colors.
	// clears transp_color if no transparent pixels are found.

	//xlogline( "old colors: %i", int(cmap.usedColors()) );
	//if( XXLOG && cmap.hasTranspColor() ) logline( "old transp: %i",int(cmap.transpColor()) );
	//xlogline( "cmap: %s",cmap.cmapStr() );

	Colormap my_cmap; // new cmap
	uchar	 my_conv[256];
	memset(my_conv, 0xff, 256); // old color index -> new color index

	uptr p	= getPixels(); // running pointer
	int	 w	= width();
	int	 h	= height();
	int	 dy = rowOffset();
	assert(dy >= w);
	int n = 0; // new cmap size

	for (int y = 0; y < h; y++, p += dy)
		for (int x = 0; x < w; x++)
		{
			int c = p[x];		// c = old color index
			int k = my_conv[c]; // k = new color index
			if (k >= n)
			{
				assert(c < cmap.usedColors());
				my_conv[c] = k = n++;
				my_cmap.addColor(cmap[c]);

				//xlogline( "  pixel[x=%i,y=%i] = %i -> %i, rgb=(%hhu,%hhu,%hhu)",
				//		x1()+x, y1()+y, c, k, cmap[c][0], cmap[c][1], cmap[c][2] );
			}
			p[x] = k;
		}

	assert(n <= cmap.usedColors());
	assert(n == my_cmap.usedColors());

	if (cmap.hasTranspColor())
	{
		int my_transp = my_conv[cmap.transpColor()];
		if (my_transp < n)
		{
			my_cmap.setTranspColor(my_transp);
			//debugstr("new transp: %i\n", int(my_cmap.transpColor()));
		}
		//else { debugstr("no transparent pixels!\n"); }
	}

	cmap = my_cmap;
	//	xlogline( "new colors: %i", int(cmap.usedColors()) );
	//	xlogline( "cmap: %s",cmap.cmapStr() );
}

void Pixelmap::setToDiff(const Pixelmap& neu, int transp_color)
{
	// Set this Pixmap to the difference which converts this old Pixmap into the new Pixmap.
	// -	the Pixmap size is shrinked to the bounding box of all changed pixels
	// -	unchanged pixels are set to the transparent color, except if transp_color<0 ((use Colormap::unset))
	// intended for gif animation

	box.intersect_with(neu.box);

	int h = height();
	int w = width();
	if (w == 0 || h == 0) return;
	int x1 = this->x1();
	int y1 = this->y1();

	uptr  zp = getPixelPtr(x1, y1); // -> top left pixel in box
	cuptr np = neu.getPixelPtr(x1, y1);

	while (h > 0) // skip identical top rows:
	{
		int x;
		for (x = 0; x < w; x++)
		{
			if (zp[x] != np[x]) break;
		}
		if (x < w) break;
		else
		{
			h--;
			y1++;
			zp += dy;
			np += neu.dy;
		}
	}

	zp += (h - 1) * dy; // -> bottom left pixel in box
	np += (h - 1) * neu.dy;

	while (h > 0) // skip identical bottom rows:
	{
		int x;
		for (x = 0; x < w; x++)
		{
			if (zp[x] != np[x]) break;
		}
		if (x < w) break;
		else
		{
			h--;
			zp -= dy;
			np -= neu.dy;
		}
	}

	zp -= (h - 1) * dy; // -> top left pixel in box
	np -= (h - 1) * neu.dy;

	while (w > 0) // skip identical left columns:
	{
		int y;
		for (y = 0; y < h; y++)
		{
			if (zp[y * dy] != np[y * neu.dy]) break;
		}
		if (y < h) break;
		else
		{
			w--;
			x1++;
			np++;
			zp++;
		}
	}

	zp += w - 1;
	np += w - 1;

	while (w > 0) // skip identical right columns:
	{
		int y;
		for (y = 0; y < h; y++)
		{
			if (zp[y * dy] != np[y * neu.dy]) break;
		}
		if (y < h) break;
		else
		{
			w--;
			zp--;
			np--;
		}
	}

	zp -= w - 1;
	np -= w - 1;

	setFrame(x1, y1, w, h);

	// my bounding box x,y,w,h is the bounding box for all changes.
	// replace unchanged pixels with transparent color:

	if (transp_color >= 0)
		for (int y = 0; y < h; y++, zp += dy, np += neu.dy)
			for (int x = 0; x < w; x++) zp[x] = zp[x] == np[x] ? transp_color : np[x];
	else
		for (int y = 0; y < h; y++, zp += dy, np += neu.dy)
			for (int x = 0; x < w; x++) zp[x] = np[x];
}


/*	Reduce this Pixmap to the difference which converts the old Pixmap into this Pixmap.
	-	the Pixmap size is shrinked to the bounding box of all changed pixels
	-	unchanged pixels are set to the transparent color, except if transp_color<0 ((use Colormap::unset))
	intended for gif animation
*/
void Pixelmap::reduceToDiff(const Pixelmap& old, int transp_color)
{
	box.intersect_with(old.box);

	int h = height();
	int w = width();
	if (w == 0 || h == 0) return;
	int x1 = this->x1();
	int y1 = this->y1();

	uptr  zp = getPixelPtr(x1, y1); // -> top left pixel in box
	cuptr np = old.getPixelPtr(x1, y1);

	while (h > 0) // skip identical top rows:
	{
		int x;
		for (x = 0; x < w; x++)
		{
			if (zp[x] != np[x]) break;
		}
		if (x < w) break;
		else
		{
			h--;
			y1++;
			zp += dy;
			np += old.dy;
		}
	}

	zp += (h - 1) * dy; // -> bottom left pixel in box
	np += (h - 1) * old.dy;

	while (h > 0) // skip identical bottom rows:
	{
		int x;
		for (x = 0; x < w; x++)
		{
			if (zp[x] != np[x]) break;
		}
		if (x < w) break;
		else
		{
			h--;
			zp -= dy;
			np -= old.dy;
		}
	}

	zp -= (h - 1) * dy; // -> top left pixel in box
	np -= (h - 1) * old.dy;

	while (w > 0) // skip identical left columns:
	{
		int y;
		for (y = 0; y < h; y++)
		{
			if (zp[y * dy] != np[y * old.dy]) break;
		}
		if (y < h) break;
		else
		{
			w--;
			x1++;
			np++;
			zp++;
		}
	}

	zp += w - 1;
	np += w - 1;

	while (w > 0) // skip identical right columns:
	{
		int y;
		for (y = 0; y < h; y++)
		{
			if (zp[y * dy] != np[y * old.dy]) break;
		}
		if (y < h) break;
		else
		{
			w--;
			zp--;
			np--;
		}
	}

	zp -= w - 1;
	np -= w - 1;

	setFrame(x1, y1, w, h);

	// my bounding box x,y,w,h is the bounding box for all changes.
	// replace unchanged pixels with transparent color:

	if (transp_color >= 0)
		for (int y = 0; y < h; y++, zp += dy, np += old.dy)
			for (int x = 0; x < w; x++)
				if (zp[x] == np[x]) zp[x] = transp_color;
}


void Pixelmap::clear(int color)
{
	uptr p = getPixels(); // p-> first == top/left pixel
	int	 w = width();
	int	 h = height();
	uptr e = p + h * dy;

	while (p < e)
	{
		memset(p, color, w);
		p += dy;
	}
}

} // namespace kio::Graphics


/*




































*/
