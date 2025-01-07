#include "Devices/StdFile.h"
#include "Graphics/Pixmap.h"
#include "Graphics/gif/GifDecoder.h"
#include "common/Array.h"
#include "common/cdefs.h"
#include "common/standard_types.h"
#include <dirent.h>

namespace kio
{
void panic(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	exit(2);
}

//static bool  	   write_rsrc = false;	 // will be enabled if no other is set
static cstr indir	  = nullptr; // must be set
static cstr outdir	  = nullptr; // will be set to indir if not set
static bool verbose	  = false;
static bool recursive = true;

using namespace Devices;
using namespace Graphics;

static int count_1color_run(uint8* scanline, int a, int e)
{
	e = min(e, a + 64 + 1 + 255); // max. run length

	uint8 color = scanline[a];
	int	  i		= a + 1;
	while (i < e && scanline[i] == color) { i++; }
	return i - a;
}

static int count_2color_run(uint8* scanline, int a, int e)
{
	e = min(e, a + 64 + 4 + 255); // max. run length

	uint8 colors[2];
	int	  nc = 0;
	int	  i	 = a;

	for (; i < e; i++)
	{
		int ci = 0;
		while (ci < nc && scanline[i] != colors[ci]) ci++;
		if (ci < nc) continue;
		if (nc == 2) break;
		colors[nc++] = scanline[i];
	}

	return i - a;
}

static int count_4color_run(uint8* scanline, int a, int e)
{
	e = min(e, a + 64 + 6 + 255); // max. run length

	uint8 colors[4];
	int	  nc = 0;
	int	  i	 = a;

	for (; i < e; i++)
	{
		int ci = 0;
		while (ci < nc && scanline[i] != colors[ci]) ci++;
		if (ci < nc) continue;
		if (nc == 4) break;
		colors[nc++] = scanline[i];
	}

	return i - a;
}

static int convert_scanline_stage3(uint8* scanline, int a, int e)
{
	/* stage 3:	Iterate over all pixels:
		 For each position: 
		 	count the length of a 1-, 2- and 4-color run.
		 	If none of them yields a saving 
		 	=> 	add pixel to current uncompressed run.
		 	else:
		 	if only one 
		 	=> 	store this
		 	else:
		 	determine method which saved the most a) absolute and b) relative
		 	if there is a clear candidate 
		 	=> 	store this
		 	else:
		 	((in case of 2 candidates))
		 	one compresses better (relative) but is shorter and therefore saves less (absolute).
		 	Determine whether carving the shorter 'A' from the longer 'B' in total saves space:
		 	Compare the saving of B with saving of A plus saving of the remainder of B.
		 	Does this save space?
		 	=>  store shorter
		 	else:
		 	=>	store longer
	*/

	uint8 uncompressed_bytes[340]; // ~ 64 + 256 max.
	uint  num_uncompressed_bytes = 0;
	int	  num_bytes				 = 0;

	for (int i = a; i < e; i++)
	{
		struct Data
		{
			int nc = 0; // number of colors: 1, 2 or 4
			int sz = 0; // uncompressed size
			int cs = 0; // compressed size
			int as = 0; // absolute saving (may be negative!)
			int rs = 0; // relative saving

			bool operator>=(const Data& b) { return as >= b.as && rs >= b.rs; }
			bool operator<=(const Data& b) { return as <= b.as && rs <= b.rs; }
		};

		Data data[3];
		int	 nc = 0; // number of candidates

		int sz = count_1color_run(scanline, i, e);
		int cs = 2 + (sz >= 64 + 1);
		int as = sz - cs;
		int rs = as * 1000 / sz;
		if (as > 0) data[nc++] = Data {1, sz, cs, as, rs};

		sz = count_2color_run(scanline, i, e);
		cs = 3 + (sz >= 64 + 4) + ((sz + 7) / 8);
		as = sz - cs;
		rs = as * 1000 / sz;
		if (as > 0) data[nc++] = Data {2, sz, cs, as, rs};

		sz = count_4color_run(scanline, i, e);
		cs = 5 + (sz >= 64 + 6) + ((sz + 3) / 4);
		as = sz - cs;
		rs = as * 1000 / sz;
		if (as > 0) data[nc++] = Data {4, sz, cs, as, rs};

		if (nc == 0) // no savings here:
		{
			uncompressed_bytes[num_uncompressed_bytes++] = scanline[i];
			if (num_uncompressed_bytes < 64 + 255) continue;
			num_bytes += num_uncompressed_bytes + 1;
			printf("%i+1 ", num_uncompressed_bytes); //TODO
			num_uncompressed_bytes = 0;
			continue;
		}

		if (num_uncompressed_bytes != 0) // flush uc bytes:
		{
			num_bytes += num_uncompressed_bytes + 1;
			printf("%i+1 ", num_uncompressed_bytes); //TODO
			num_uncompressed_bytes = 0;
		}

		if (nc == 3)
		{
			if (data[2] <= data[1]) { nc--; }
			else if (data[2] <= data[0]) { nc--; }
			else if (data[2] >= data[1]) { data[1] = data[--nc]; }
			else if (data[1] <= data[0]) { data[1] = data[--nc]; }
			else if (data[2] >= data[0]) { data[0] = data[--nc]; }
			else if (data[1] >= data[0]) { data[0] = data[--nc]; }
			else TODO(); // can't remove any
		}

		if (nc == 2)
		{
			if (data[1] <= data[0]) { nc--; }
			else if (data[0] <= data[1]) { data[0] = data[--nc]; }
			else // there is no clear candidate:
			{
				if (data[0].nc > data[1].nc) std::swap(data[0], data[1]);
				assert(data[0].sz < data[1].sz);
				assert(data[0].as < data[1].as);
				assert(data[0].rs > data[1].rs);
				// Does carving the shorter 'A' from the longer 'B' in total save space?
				// Compare the saving of B with saving of A plus saving of the remainder of B.
				sz = data[1].sz - data[0].sz;								 // uncompressed size of remainder
				cs = data[1].nc == 2 ? 3 + (sz >= 64 + 4) + ((sz + 7) / 8) : // 2-color run
									   5 + (sz >= 64 + 6) + ((sz + 3) / 4);				 // 4-color run
				as = max(sz - cs, 0);										 // saving of remainder

				if (data[0].as + as > data[1].as) { nc--; }
				else { data[0] = data[--nc]; }
			}
		}

		assert(nc == 1); // one candidate:
		sz = data[0].sz;
		as = data[0].as;
		if (data[0].nc == 1) printf("S%i-%i ", sz, as);
		else if (data[0].nc == 2) printf("D%i-%i ", sz, as);
		else printf("Q%i-%i ", sz, as);
		i += sz - 1;
		num_bytes += sz - as;
	}

	if (num_uncompressed_bytes != 0) // flush uc bytes:
	{
		num_bytes += num_uncompressed_bytes + 1;
		printf("%i+1 ", num_uncompressed_bytes); //TODO
		num_uncompressed_bytes = 0;
	}

	return num_bytes;
}

static int convert_scanline_stage2(uint8* scanline, int a, int e)
{
	// stage 2:	search for long 2-color runs
	//	  which could be otherwise hidden in a 4-color run.
	//	  This adds the 2-color run command plus another 4-color run thereafter.
	//	split 4-color run:
	//	  +0.5 average loss for byte-alignment = 1/2 byte
	//	  + 4  bytes 1-color run command + at least 1 pattern byte  (??need to add 1 for min pattern??)
	//	  + 5  bytes 4-color run command
	//	  - (0.5+5+4)/4 saved bits in the pattern
	//	  = 7.125 bytes added
	//	=> the 2-color run must encode at least 8 bytes.
	//	-> split at color run, encode left side, store color run and proceed with remainder.

	static constexpr int min_length = 8;

	int num_bytes = 0;

	for (int i = a; i <= e - min_length; i++)
	{
		int sz = count_2color_run(scanline, i, e);
		if (sz < min_length) continue;
		// split:
		if (i > a) num_bytes += convert_scanline_stage2(scanline, a, i); // left side
		int cs = 3 + (sz >= 64 + 4) + ((sz + 7) / 8);
		num_bytes += cs;
		printf("*D%i-%i ", sz, sz - cs); // middle TODO
		a = i + sz;						 // right side
		i = a - 1;
	}

	if (a < e) num_bytes += convert_scanline_stage3(scanline, a, e);
	return num_bytes;
}

static int convert_scanline(uint8* scanline, int a, int e)
{
	// stage 1: search for long 1-color runs
	//    which could be otherwise hidden in a 2- or 4-color run.
	//    This adds the 1-color run command plus the right-side 2- or 4-color run.
	//	if it splits a 4-color run:
	//	  +0.5 average loss for byte-alignment = 1/2 byte
	//	  + 2  bytes 1-color run command
	//	  + 5  bytes 4-color run command
	//	  - (0.5+5+2)/4 saved bits in the pattern
	//	  = 5.5 bytes added
	//	if it splits a 2-color run:
	//	  +0.5 average loss for byte-alignment = 1/2 byte
	//	  + 2  bytes 1-color run command
	//	  + 3  bytes 2-color run command
	//	  - (0.5+3+2)/8 saved bits in the pattern
	//	  = 4.8 bytes added
	//  => the 1-color run must encode at least 6 bytes.
	//  -> split at the color run, encode left side, store color run and proceed with remainder.

	static constexpr int min_length = 6;

	int num_bytes = 0;

	for (int i = a; i <= e - min_length; i++)
	{
		int sz = count_1color_run(scanline, i, e);
		if (sz < min_length) continue;
		// split:
		if (i > a) num_bytes += convert_scanline(scanline, a, i); // left side
		int cs = 2;
		num_bytes += cs;
		printf("*S:%i-%i ", sz, sz - cs); // middle TODO
		a = i + sz;						  // right side
		i = a - 1;
	}

	if (a < e) num_bytes += convert_scanline_stage2(scanline, a, e);
	return num_bytes;
}

static void convert_image(RCPtr<Pixmap_i8>& canvas, Color* cmap, uint num_colors)
{
	printf("w*h = %i*%i\n", canvas->width, canvas->height);
	printf("num colors = %u\n", num_colors);

	uint8* pixmap = canvas->pixmap;
	for (int y = 0; y < canvas->height; y++)
	{
		//for (int x = 0; x < canvas->width; x++)
		//{
		//	if (x % 64 == 0) puts("");
		//	printf("%02x ", pixmap[x]);
		//}
		//puts("");

		int cs = convert_scanline(pixmap, 0, canvas->width);
		int as = canvas->width - cs;
		printf("\ntotal: %i -> %i (%i %%)\n", canvas->width, cs, as * 100 / canvas->width);
		pixmap += canvas->row_offset;
	}
}

static void convert_file(cstr indir, __unused cstr outdir, cstr infile)
{
	try
	{
		printf("\nprocessing %s\n", infile);
		FilePtr					 file = new StdFile(catstr(indir, infile));
		GifDecoder				 gif(file);
		std::unique_ptr<Color[]> cmap {new Color[256]};
		if (!gif.isa_gif_file) throw "not a gif file";
		RCPtr<Pixmap_i8> canvas = new Pixmap_i8(gif.image_width, gif.image_height);
		gif.decode_image(canvas, cmap.get());
		convert_image(canvas, cmap.get(), 1 << gif.global_cmap_bits);
	}
	catch (Error e)
	{
		printf("*** %s\n", e);
	}
	catch (std::exception& e)
	{
		printf("*** %s\n", e.what());
	}
	catch (...)
	{
		printf("*** unknown exception\n");
	}
}

static void convert_dir(cstr indir, cstr outdir, cstr subdir)
{
	if (!endswith(indir, "/")) indir = catstr(indir, "/");
	if (!endswith(outdir, "/")) outdir = catstr(outdir, "/");
	if (ne(subdir, "") && !endswith(subdir, "/")) subdir = catstr(subdir, "/");

	DIR* dir = opendir(catstr(indir, subdir));
	if (!dir) throw strerror(errno);

	while (dirent* de = readdir(dir))
	{
		if (startswith(de->d_name, ".")) continue; // hidden file or folder
		if (de->d_type == DT_DIR && recursive) convert_dir(indir, outdir, catstr(subdir, de->d_name, "/"));
		if (de->d_type != DT_REG) continue; // not a file

		convert_file(indir, outdir, catstr(subdir, de->d_name));
	}
	closedir(dir);
}
} // namespace kio


int main(int argc, cstr* argv)
{
	// 2++ arguments: indir, outdir, options

	using namespace kio;
	argc -= 1, argv += 1; // prog path

	try
	{
		while (argc >= 1 && argv[0][0] == '-')
		{
			if (eq(argv[0], "-v"))
			{
				argc -= 1;
				argv += 1;
				verbose = true;
			}
			else throw "unknown option";
		}

		if (argc < 1 || argc > 2) throw "arguments: [options] indir [outdir]";

		indir  = argv[0];
		outdir = argc > 1 ? argv[1] : indir;

		convert_dir(indir, outdir, "");
	}
	catch (cstr e)
	{
		printf("error: %s\n", e);
		return 1;
	}
	puts("all done.\n");
	return 0;
}
