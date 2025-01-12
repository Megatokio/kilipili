

#include "Audio/Ay38912.h"
#include "Devices/HeatShrinkEncoder.h"
#include "Devices/LzhDecoder.h"
#include "Devices/StdFile.h"
#include "ImageFileWriter.h"
#include "RgbImageCompressor.h"
#include "RsrcFileEncoder.h"
#include "YMFileConverter.h"
#include "YMMFileConverter.h"
#include "common/Array.h"
#include "common/cdefs.h"
#include "common/standard_types.h"
#include "exportStSoundWavFile.h"
#include <dirent.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG 1
#include "extern/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "extern/stb/stb_image_write.h"

namespace kio::Audio
{
float hw_sample_frequency; // normally in AudioController.cpp
}

namespace kio
{
void panic(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	exit(2);
}

static bool		   write_rsrc = false;	 // will be enabled if no other is set
static cstr		   indir	  = nullptr; // must be set
static cstr		   outdir	  = nullptr; // will be set to indir if not set
static bool		   verbose	  = false;
static bool		   recursive  = true;
static Array<cstr> rsrc_files;

using namespace Audio;
using namespace Devices;

enum FType : uint8 { UNSET, COPY, STSOUND_WAV, WAV, YMM, IMG, SKIP, HAM_IMG };
struct Info
{
	cstr	   pattern = nullptr;
	FType	   format  = UNSET;
	uint8	   w = 0, l = 0;									   // compression
	DitherMode dithermode				  = DitherMode::Diffusion; // ham
	bool	   noalpha				  : 1 = false;				   // img
	bool	   hwcolor				  : 1 = false;				   // img
	bool	   also_create_ref_image  : 1 = false;				   // ham
	bool	   also_create_diff_image : 1 = false;				   // ham
	bool	   also_write_stats_file  : 1 = false;				   // ham
	bool	   enriched_filenames	  : 1 = false;				   // ham
	bool	   _padding2			  : 2;
	char	   _padding[3];

	Info(cstr s);
};
static Array<Info> infos;

Info::Info(cstr _s)
{
	str s = replacedstr(_s, '\t', ' ');				 // remove tabs
	if (ptr p = strchr(s, ';')) *p = 0;				 // cut line at comment
	while (*s && *s <= ' ') s++;					 // remove white space at start
	if (*s == 0 || *s == '#') return;				 // empty line or comment
	for (ptr p = strchr(s, 0); *--p <= ' ';) *p = 0; // remove white space at end

	ptr p = strchr(s, ' ');
	if (!p)
	{
		if (eq(s, "rsrc") || eq(s, "hdr"))
		{
			write_rsrc = true;
			return;
		}
		else throw usingstr("no options given for %s", s);
	}

	pattern = substr(s, p);

	for (s = p; s; s = p)
	{
		while (*s == ' ') s++;
		if (*s == 0) break;
		p = strchr(s, ' ');
		if (p) s = substr(s, p);

		if (eq(s, "wav")) format = WAV;
		else if (eq(s, "stsound_wav")) format = STSOUND_WAV;
		else if (eq(s, "ymm")) format = YMM;
		else if (eq(s, "img")) format = IMG;
		else if (eq(s, "ham")) format = HAM_IMG;
		else if (eq(s, "copy")) format = COPY;
		else if (eq(s, "skip")) format = SKIP;
		else if (eq(s, "noalpha")) noalpha = true;
		else if (eq(s, "hwcolor")) hwcolor = true;
		else if (eq(s, "pattern")) dithermode = DitherMode::Pattern;
		else if (eq(s, "none")) dithermode = DitherMode::None;
		else if (eq(s, "diffusion")) dithermode = DitherMode::Diffusion;
		else if (eq(s, "diff_img")) also_create_diff_image = true;
		else if (eq(s, "ref_img")) also_create_ref_image = true;
		else if (eq(s, "stats")) also_write_stats_file = true;
		else if (eq(s, "enriched")) enriched_filenames = true;
		else if (startswith(s, "W"))
		{
			uint n = 0;
			for (uint i = 1; is_decimal_digit(s[i]); i++) n = n * 10 + dec_digit_value(s[i]);
			w = uint8(n);
		}
		else if (startswith(s, "L"))
		{
			uint n = 0;
			for (uint i = 1; is_decimal_digit(s[i]); i++) n = n * 10 + dec_digit_value(s[i]);
			l = uint8(n);
		}
		else throw usingstr("unknown option %s", s);
	}

	if (format == UNSET) throw usingstr("no format option in: %s", _s);

	if (format == YMM)
	{
		if (!w) w = 10;
		if (w < 8 || w > 14) throw "size W oorange";
	}
	else if (w && l)
	{
		if (w < 6 || w > 14) throw "size W oorange";
		if (l < 4 || l > 10) throw "size L oorange";
		if (l + 2 > w) throw "L too large or W too small";
	}
}

static void copy_as_StSound_wav(cstr indir, cstr outdir, cstr infile)
{
	if (write_rsrc && verbose) puts("  skipped\n"); // don't copy wav into rsrc
	if (write_rsrc) return;

	cstr ext	  = extension_from_path(infile); // points to '.' in infile
	cstr basename = substr(infile, ext);
	exportStSoundWavFile(catstr(indir, infile), catstr(outdir, basename, ".wav"));
}

static void copy_as_wav(cstr indir, cstr outdir, cstr infile)
{
	if (write_rsrc && verbose) puts("  skipped\n"); // don't copy wav into rsrc
	if (write_rsrc) return;

	YMFileConverter converter(catstr(indir, infile), verbose);
	cstr			ext	 = extension_from_path(infile); // points to '.' in infile
	FilePtr			file = new StdFile(catstr(outdir, substr(infile, ext), ".wav"), WRITE | TRUNCATE);
	uint32			size = converter.exportWavFile(file);
	if (verbose) printf("  .wav file size = %u\n", size);
}

static void copy_as_ymm(cstr indir, cstr outdir, cstr infile, const Info& info)
{
	// convert YM file to YMM file:

	YMMFileConverter converter;
	cstr			 ext	  = extension_from_path(infile); // points to '.' in infile
	cstr			 basename = substr(infile, ext);
	uint32			 zsize	  = 0;

	if (write_rsrc)
	{
		cstr include_fname = catstr(infile, ".rsrc");		// file for #include
		cstr hdr_fpath	   = catstr(outdir, include_fname); // file written to
		cstr rsrc_fpath	   = catstr(basename, ".ymm");		// fname inside rsrc filesystem

		FilePtr hfile = new StdFile(hdr_fpath, WRITE | TRUNCATE); // the header file
		FilePtr rfile = new RsrcFileEncoder(hfile, rsrc_fpath);	  // rsrc file encoder
		zsize		  = converter.convertFile(catstr(indir, infile), rfile, verbose, info.w);

		rsrc_files.append(include_fname);
	}
	else
	{
		FilePtr rfile = new StdFile(catstr(outdir, basename, ".ymm"), WRITE | TRUNCATE);
		zsize		  = converter.convertFile(catstr(indir, infile), rfile, verbose, info.w);
	}

	uint32 qsize = converter.csize;
	if (verbose) printf("  .ym input file size = %u\n", qsize);
	if (verbose) printf("  .ymm output file size = %u\n", zsize);
}

static void copy_as_img(cstr indir, cstr outdir, cstr infile, const Info& info)
{
	// convert image to img file:
	// resource is always compressed

	ImageFileWriter converter(info.hwcolor, !info.noalpha);
	converter.importFile(catstr(indir, infile));

	if (verbose)
	{
		printf("  size = %i*%i\n", converter.image_width, converter.image_height);
		printf("  colors = %s\n", tostr(converter.colormodel));
		if (converter.has_cmap) printf("  cmap size = %u\n", converter.cmap.count());
		if (converter.has_transparency) printf("  has transparency\n");
	}

	cstr ext	  = extension_from_path(infile); // points to '.' in infile
	cstr basename = substr(infile, ext);

	if (write_rsrc)
	{
		cstr   include_fname = catstr(infile, ".rsrc");		  // file for #include
		cstr   dest_fname	 = catstr(outdir, include_fname); // file written to
		cstr   rsrc_fname	 = catstr(basename, ".img");	  // fname inside rsrc filesystem
		uint32 size			 = converter.exportRsrcFile(dest_fname, rsrc_fname, info.w, info.l);
		if (verbose) printf("  .img file size = %u\n", size);
		rsrc_files.append(include_fname);
	}
	else
	{
		uint32 size = converter.exportImgFile(catstr(outdir, basename, ".img"), info.w, info.l);
		if (verbose) printf("  .img file size = %u\n", size);
	}
}

static void copy_as_ham_image(cstr indir, cstr outdir, cstr infile, const Info& info)
{
	RgbImageCompressor encoder;
	encoder.write_diff_image = info.also_create_diff_image;
	encoder.write_ref_image	 = info.also_create_ref_image;
	encoder.write_stats_file = info.also_write_stats_file;

	if (verbose) {}
	if (write_rsrc)
	{
		cstr	ext			  = extension_from_path(infile); // points to '.' in infile
		cstr	basename	  = substr(infile, ext);
		cstr	include_fname = catstr(infile, ".rsrc");	   // file for #include
		cstr	dest_fname	  = catstr(outdir, include_fname); // file written to
		cstr	rsrc_fname	  = catstr(basename, ".ham");	   // fname inside rsrc filesystem
		FilePtr outfile		  = new StdFile(dest_fname, FileOpenMode::WRITE);
		outfile				  = new RsrcFileEncoder(outfile, rsrc_fname, info.w == 0);
		if (info.w) outfile = new HeatShrinkEncoder(outfile, info.w, info.l, false);
		uint32 size = encoder.encodeImage(catstr(indir, infile), outfile, verbose, info.dithermode);
		if (verbose) printf("  .img file size = %u\n", size);
		rsrc_files.append(include_fname);
	}
	else
	{
		encoder.encodeImage(indir, outdir, infile, verbose, info.dithermode);
		if (verbose) printf("\n");
	}
}

static void copy_as_is(cstr indir, cstr outdir, cstr infile, const Info& info)
{
	// copy "as is", but:
	// -> normal or resource file
	// -> with or without decompression/recompression
	// -> no compression if file size increased

	FilePtr file  = new StdFile(catstr(indir, infile));
	uint32	csize = uint32(file->getSize());

	bool compressed = false;
	if (info.w && info.l)
	{
		// if compression parameters are set then decompress source file, if compressed:
		if ((compressed = isLzhEncoded(file))) file = new LzhDecoder(file);
		else if ((compressed = isHeatShrinkEncoded(file))) file = new HeatShrinkDecoder(file);
	}

	uint32				  fsize = uint32(file->getSize());
	std::unique_ptr<char> data {new char[fsize]};
	file->read(data.get(), fsize);

	if (verbose && !compressed) printf("  file size = %u\n", fsize);
	if (verbose && compressed) printf("  compressed file size = %u\n", csize);
	if (verbose && compressed) printf("  uncompressed file size = %u\n", fsize);

	if (!write_rsrc) // write plain file:
	{
		if (info.w && info.l) // compress
		{
			file						   = new StdFile(catstr(outdir, infile), WRITE);
			RCPtr<HeatShrinkEncoder> cfile = new HeatShrinkEncoder(file, info.w, info.l);
			cfile->write(data.get(), fsize);
			cfile->close();
			assert(fsize == cfile->usize);
			if (verbose) printf("  compressed size = %u\n", cfile->csize + 12);
			if (cfile->csize + 12 < fsize) return;
			if (verbose) puts("*** compression increased size -> store uncompressed\n");
		}

		file = new StdFile(catstr(outdir, infile), WRITE);
		file->write(data.get(), fsize);
		file->close();
	}
	else // write resource file:
	{
		cstr include_fname = catstr(infile, ".rsrc");		// file for #include
		cstr dest_fpath	   = catstr(outdir, include_fname); // file written to
		cstr rsrc_fname	   = infile;						// fname inside rsrc filesystem

		if (info.w && info.l) // compressed resource file
		{
			file						   = new StdFile(dest_fpath, WRITE);
			file						   = new RsrcFileEncoder(file, rsrc_fname, false);
			RCPtr<HeatShrinkEncoder> cfile = new HeatShrinkEncoder(file, info.w, info.l, false);
			cfile->write(data.get(), fsize);
			cfile->close();
			assert(fsize == cfile->usize);
			if (verbose) printf("  compressed size = %u\n", cfile->csize + 4);

			if (cfile->csize + 4 < fsize)
			{
				rsrc_files.append(include_fname);
				return;
			}

			if (verbose) puts("*** compression increased size -> storing uncompressed\n");
		}

		// uncompressed resource file

		file = new StdFile(dest_fpath, WRITE);
		file = new RsrcFileEncoder(file, rsrc_fname, false);
		file->write_LE(fsize);
		file->write(data.get(), fsize);
		file->close();
		rsrc_files.append(include_fname);
	}
}

static void convert_file(cstr indir, cstr outdir, cstr infile)
{
	try
	{
		if (verbose) printf("\nprocessing %s\n", infile);

		for (uint i = 0; i < infos.count(); i++)
		{
			Info& info = infos[i];
			if (!fnmatch(info.pattern, infile, true)) continue;

			switch (int(info.format))
			{
			case COPY: copy_as_is(indir, outdir, infile, info); return;
			case WAV: copy_as_wav(indir, outdir, infile); return;
			case STSOUND_WAV: copy_as_StSound_wav(indir, outdir, infile); return;
			case YMM: copy_as_ymm(indir, outdir, infile, info); return;
			case IMG: copy_as_img(indir, outdir, infile, info); return;
			case HAM_IMG: copy_as_ham_image(indir, outdir, infile, info); return;
			case SKIP: return;
			default: IERR();
			}
		}
		if (verbose) printf("*** didn't match any pattern\n");
	}
	catch (Error e)
	{
		fprintf(stderr, "*** %s\n", e);
	}
	catch (std::exception& e)
	{
		fprintf(stderr, "*** %s\n", e.what());
	}
	catch (...)
	{
		fprintf(stderr, "*** unknown exception\n");
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


int main(int argc, cstr argv[])
{
	// 1 argument: job file
	// 2++ arguments: indir, outdir, format and options

	using namespace kio;

	if (argc >= 2 && eq(argv[1], "-v"))
	{
		argv += 1;
		argc -= 1;
		verbose = true;
	}

	try
	{
		if (argc == 1)
		{
			puts(
				"1 argument = job_file\n"
				"2++ arguments = indir outdir format options\n"
				"formats: wav ym ymm img as_is\n"
				"options: Wx Lx noalpha hwcolor (x=number)\n");
			return 0;
		}
		else if (argc == 2)
		{
			char  bu[256];
			FILE* file = fopen(argv[1], "r");
			if (!file) throw "file not found";
			cstr basepath = directory_from_path(argv[1]);
			indir		  = fgets(bu, 256, file);
			if (!indir) throw "no indir";
			if (ptr p = strchr(bu, ';')) *p = 0;
			indir  = catstr(basepath, croppedstr(bu));
			outdir = fgets(bu, 256, file);
			if (!outdir) throw "no outdir";
			if (ptr p = strchr(bu, ';')) *p = 0;
			outdir = catstr(basepath, croppedstr(bu));

			while (str s = fgets(bu, 256, file))
			{
				Info info(s);
				if (info.format != UNSET) infos.append(info);
			}
		}
		else
		{
			indir  = argv[1];
			outdir = argv[2];

			cstr s = "*";
			for (int i = 3; i < argc; i++)
			{
				cstr arg = argv[i];
				arg += *arg == '-';
				if (eq(arg, "v")) verbose = true;
				else s = catstr(s, " ", arg);
			}
			Info info(s);
			if (info.format == UNSET) throw "no format option given";
			infos.append(info);
		}

		if (!endswith(indir, "/")) indir = catstr(indir, "/");
		if (!endswith(outdir, "/")) outdir = catstr(outdir, "/");

		convert_dir(indir, outdir, "");

		if (write_rsrc)
		{
			FilePtr file = new StdFile(catstr(outdir, "rsrc.cpp"), WRITE);
			assert(file);
			file->puts("extern const unsigned char resource_file_data[];\n");
			file->puts("constexpr unsigned char resource_file_data[]={\n");
			for (uint i = 0; i < rsrc_files.count(); i++)
			{
				file->printf("#include \"%s\"\n", rsrc_files[i]); //
			}
			file->puts("0};\n");
			file->close();
		}
	}
	catch (cstr e)
	{
		fprintf(stderr, "error: %s\n", e);
		return 1;
	}
	catch (...)
	{
		fprintf(stderr, "unknown exception\n");
		return 1;
	}

	if (int cnt = RgbImageCompressor::total_num_images)
	{
		printf("\nGRANDE TOTAL RgbImageCompressor SUMMARY:\n");
		bool f = RgbImageCompressor::deviation_linear();
		printf("  deviation handling = %s\n", f ? "linear" : "quadratic");
		if (f) printf("  - max. deviation   = %i\n", RgbImageCompressor::deviation_max());
		if (f) printf("  - factor above max = %i\n", RgbImageCompressor::deviation_factor());
		//printf("  high deviation boost = %s\n", RgbImageCompressor::high_deviation_other_boost() ? "enabled" : "disabled");
		printf("  num images: %i\n", cnt);
		printf("  average num_abs_codes: %2.2f\n", double(RgbImageCompressor::total_num_abs_codes) / cnt);
		printf("  average num_rel_codes: %2.2f\n", double(RgbImageCompressor::total_num_rel_codes) / cnt);
		printf("  total deviation: %.0f\n", RgbImageCompressor::total_total_deviation);
		printf("  average deviation: %f\n", RgbImageCompressor::total_average_deviation / cnt);
		printf("Deviation Map:\n");
		for (uint i = 0; i < NELEM(RgbImageCompressor::total_deviations); i++)
		{
			uint n = RgbImageCompressor::total_deviations[i];
			if (n) printf("%4u: %u\n", i, n);
		}
		printf("\n");
	}

	puts("all done.\n");
	return 0;
}


/*









































*/
