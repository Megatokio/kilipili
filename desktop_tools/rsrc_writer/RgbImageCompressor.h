// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Graphics/Color.h"
#include "common/RCPtr.h"
#include "common/standard_types.h"
#include <cstdlib>


/*	Compressor for RGB images
	- Slow resource hungry compressor.
	- Extremely fast decompressor with zero ram requirement
	  originally intended for on-the-fly decoding of a frame buffer in a video display engine 60 times a second.
	  On a ARMv4 this takes estimated 14 clock cycles per pixel incl. reading of the source and storing of the pixel.
	- Fixed compression ratio of 1 byte per pixel (like 8 bit indexed color images from gif files.)

	- The resulting images are generally undistinguishable from the color-reduced and dithered originals.
	- For RGB555 they are also so close to the RGB888 original that they are hard to tell apart too.
	  Comparing an image with lower color depth with the RGB888 original directly is not very helpful.

	Limitations:

	- This format is not useful if the hardware Color fits in 1 byte directly:
	  Then encode as a true-color image instead.
	- The format may not be useful for index color images which already use only 1 byte per pixel.
	  But decompression of a gif image uses ~40kB of memory which we might not have at that point
	  or the diffusion of rounding errors from the RGB888 colors in the .img file results in a better image.
	- Not suitable for monochrome images except for what is said above for indexed color images.
	
	- Blending from one image to another is hard 
	  because he color of the first pixel of each scanline is derived from the previous line.
	  Dividing the display vertically as it is read from a file is possible with only little overhead.
	- Fading the image to/from black is hard
	  because the relative colors need the full and unmodified color value of the previous pixel.
	  The decoder couls, given some more clock cycles, shift and mask the pixel before storing
	  which would give ~4 intermediate steps before reaching black.
	  
	Encoding:

	- The RGB888 image is reduced to hardware Color depth
	  using methods of dither to simulate a color depth of 2 more bits.
	- Then the image is encoded using absolute color values and relative color differences.
	- A total of 256 codes are used which are split between the absolute and relative colors.
	- The assignment of code points to either table and the selection of colors and color differences
	  for each table is optimized in multiple optimize-and-encode runs.

	- Adding absolute colors:
	  A usage map for all colors of the hardware Color is created from the actually used colors from the last run.
	  The usage map is blurred and then blurred holes for all available abs_color code are punched.
	  In the remaining usage map the high spots are searched and assigned for additional abs_color codes.
	  *** Blurring is very time consuming.
	- Removing absolute colors:
	  Within each run all used colors are counted.
	  The codes with the lowest count are removed.
	- Adding relative colors:
	  Within each run all used and could-have-been-used color offsets within the maximum allowed range are counted.
	  In this map the most used offsets are searched and added to the rel_color table.
	  *** this could be handled with blurring like for abs_colors too.
	- Removing relative colors:
	  Within each run all used and could-have-been-used color offsets within a maximum allowed range are counted.
	  The codes with the lowest count are removed.

	Remaining problems:

	Some images showe visual artifacts. These are images with highlight colors with low pixel count.
	These colors don't make it into the abs_codes map due to their low usage count.
	If the transition to this color is too steep, then a rel_code can't be used and a poorly matching abs_color must be used instead.
	If the transition from this color to the next pixel remains too steep, then more of this pooly matching color will be used.
	This then becomes noticable.
	Examples are
		Anderswelten-Hundertzwei	the purple foilage
		Temptation in Lab			the yellow lamp

	Possible solutions:

	- Increase the size of the rel_codes table to handle wider offsets (even if these codes are hardly ever included in the rel_codes table)
	- Search for rel_codes outside the rel_codes map for some additional distance and calculate the best code and offset in the table on-the-fly.
	- Error propagation in the encoder:
	  this will work well but
	  this will introduce new abs colors which were formerly not present in the image and the abs_codes handler has an optimization for this.
	- Reserve some absolute colors from start to end with a spacing narrow enough that rel codes will always find them from any color between.
	  These actually don't need to be may, e.g. for RGB444 a basic 8 colors cube may suffice.
	  For RGB555 we probably need 3*3*3 = 27 colors (depends on rel_codes table dimensions)
	  The reserved colors will worsen the overall result.
	- Inspect the resulting image for hot spots of deviation and assign abs_colors to cover these.
	- Inspect the final distribution of deviations:
	  They sould quickly fall from a high peak for zero deviation to very low numbers for high deviation.
	  In case of a missed highligh color i expect a high number of high deviation.
	  If this is detected the encoder should create a abs_colors usage_map with deviation only or exagerated deviation, e.g. usage*deviation^3.
	  In this table search for the hot spot and assign an abs_code for that.
	  Possibly it is best to use usage*deviation^2 or ^3 for 'adding absolute colors' in the first place.
	- Possibly blurring the absolute color usage map 4 times (instead of 3 times) for RGB555 improves the result.
	  For RGB444 the problem is present but to a much smaller extend. e.g. only 'Anderswelten-Hundertzwei' still has some noticable deviation.
*/

namespace kio
{

using Color = Graphics::Color;

enum class DitherMode : uint8 { None, Pattern, Noise, Diffusion };


class RgbImageCompressor
{
public:
	RgbImageCompressor();
	~RgbImageCompressor();

	void encodeImage(
		cstr indir, cstr outdir, cstr infile, int verbose = false, DitherMode = DitherMode::Pattern,
		int max_rounds = 6);

	bool write_diff_image = false;
	bool write_ref_image  = false;
	bool write_stats_file = false;

	RCPtr<struct RgbImage>	   image;		  // image dithered and reduced to native color depth
	RCPtr<struct EncodedImage> encoded_image; // current / last encoded image
	RCPtr<struct AbsCodes>	   abs_codes;	  // current / last abs_codes table
	RCPtr<struct RelCodes>	   rel_codes;	  // current / last rel_codes table
	RCPtr<struct EncodedImage> candidate;	  // best encoded image so far

	// statistics after encoding:
	int	   image_width();
	int	   image_height();
	uint   num_colors();
	int	   num_abs_codes();
	int	   num_rel_codes();
	uint   total_deviation();
	double average_deviation();

private:
	void optimize_codes(int jiggle);
};

} // namespace kio


inline cstr tostr(kio::DitherMode dithermode)
{
	static cstr name[] = {"none", "pattern", "noise", "diffusion"};
	return name[int(dithermode)];
}
