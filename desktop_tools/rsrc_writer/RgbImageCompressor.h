// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "File.h"
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

	Some images have visual artifacts. These are images with highlight colors with low pixel count or images
	with a lot of high-contrast contens. Though after some improvements this no longer a pressing issue.
	Remaining ideas for improvement not yet tested:
	- hard code and protect codes for:
	  - rel_codes: the central 3x3x3 = 27 codes, though this is hardly needed
	  - abs_codes: center colors of a 3x3x3 sliced color cube, though really unused colors have to be removed at some point.
	- define a lower limit for the number of abs_colors:
	  The evaluation function currently only assesses the total deviation.
	  More rel_codes typically lead to lower total deviation but at the cost of some more pixels with high deviation.
*/

namespace kio
{

using Color = Graphics::Color;
using File	= Devices::File;

enum class DitherMode : uint8 { None, Pattern, Diffusion };


class RgbImageCompressor
{
public:
	RgbImageCompressor();
	~RgbImageCompressor();

	void encodeImage(cstr indir, cstr outdir, cstr infile, int verbose = false, DitherMode = DitherMode::Pattern);
	uint encodeImage(cstr infile, File* outfile, int verbose = false, DitherMode = DitherMode::Pattern);

	bool write_diff_image = false;
	bool write_ref_image  = false;
	bool write_stats_file = false;

	RCPtr<struct RgbImage>	   image;		  // image dithered and reduced to native color depth
	RCPtr<struct EncodedImage> encoded_image; // current / last / best encoded image

	// statistics after encoding:
	int	   image_width();
	int	   image_height();
	uint   num_colors();
	int	   num_abs_codes();
	int	   num_rel_codes();
	double total_deviation();
	double average_deviation();

	static bool deviation_linear();
	static bool deviation_quadratic();
	static int	deviation_max();
	static int	deviation_factor();
	static bool high_deviation_other_boost();

	static int	  total_num_images;
	static int	  total_num_abs_codes;	   // sum of all!
	static int	  total_num_rel_codes;	   // sum of all!
	static double total_total_deviation;   // sum of all!
	static double total_average_deviation; // sum of all!
	static uint32 total_deviations[64];	   // sum of all!  only if write_diff_image=true!

private:
	int encode_image();
};


} // namespace kio


inline cstr tostr(kio::DitherMode dithermode)
{
	static cstr name[] = {"none", "pattern", "diffusion"};
	return name[int(dithermode)];
}
