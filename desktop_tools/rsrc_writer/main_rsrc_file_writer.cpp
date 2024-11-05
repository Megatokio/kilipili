

#include "Array.h"
#include "Audio/Ay38912.h"
#include "CompressedRsrcFileWriter.h"
#include "ImageFileWriter.h"
#include "RsrcFileWriter.h"
#include "YMFileConverter.h"
#include "cdefs.h"
#include "standard_types.h"
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <dirent.h>

namespace kio::Audio
{
float hw_sample_frequency; // normally in AudioController.cpp
}

void panic(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	exit(2);
}

namespace kio
{
static bool		   write_hdr = false;	// will be enabled if no other is set
static cstr		   indir	 = nullptr; // must be set
static cstr		   outdir	 = nullptr; // will be set to indir if not set
static bool		   verbose	 = false;
static bool		   recursive = true;
static Array<cstr> rsrc_files;

using namespace Audio;

enum FType : uint8 { UNSET, AS_IS, YMM_WAV, WAV, YM, YMRF, IMG, SKIP };
struct Info
{
	cstr  pattern = nullptr;
	FType format  = UNSET;
	uint8 w = 12, l = 6;
	bool  noalpha = false, hwcolor = false, _padding[3];

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
			write_hdr = true;
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
		else if (eq(s, "ymm_wav")) format = YMM_WAV;
		else if (eq(s, "ym")) format = YM;
		else if (eq(s, "ymrf")) format = YMRF;
		else if (eq(s, "img")) format = IMG;
		else if (eq(s, "*") || eq(s, "as_is")) format = AS_IS;
		else if (eq(s, "skip")) format = SKIP;
		else if (eq(s, "noalpha")) noalpha = true;
		else if (eq(s, "hwcolor")) hwcolor = true;
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

	if (w && l)
	{
		if (w < 6 || w > 15) throw "size W oorange";
		if (l < 4 || l > 10) throw "size L oorange";
		if (l + 2 > w) throw "L too large or W too small";
	}
}


static uint32 getFileSize(FILE* fp)
{
	long oPos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	long fSize = ftell(fp);
	fseek(fp, oPos, SEEK_SET);
	assert_eq(fSize, uint32(fSize));
	return uint32(fSize);
}

static void copy_as_ymm_wav(cstr indir, cstr outdir, cstr infile)
{
	if (write_hdr && verbose) puts("  skipped\n"); // don't copy wav into rsrc
	if (write_hdr) return;

	cstr ext	  = extension_from_path(infile); // points to '.' in infile
	cstr basename = substr(infile, ext);
	YMFileConverter::exportYMMusicWavFile(catstr(indir, infile), catstr(outdir, basename, ".ymm.wav"));
}

static void copy_as_wav(cstr indir, cstr outdir, cstr infile)
{
	if (write_hdr && verbose) puts("  skipped\n"); // don't copy wav into rsrc
	if (write_hdr) return;

	YMFileConverter converter;
	converter.importFile(catstr(indir, infile), verbose);
	cstr   ext	= extension_from_path(infile); // points to '.' in infile
	uint32 size = converter.exportWavFile(catstr(outdir, substr(infile, ext), ".wav"));
	if (verbose) printf("  .wav file size = %d\n", size);
}

static void copy_as_ym(cstr indir, cstr outdir, cstr infile)
{
	if (write_hdr && verbose) puts("  skipped\n"); // TODO maybe into rsrc too
	if (write_hdr) return;

	YMFileConverter converter;
	uint32			qsize = converter.importFile(catstr(indir, infile), verbose);
	cstr			ext	  = extension_from_path(infile); // points to '.' in infile
	uint32			zsize = converter.exportYMFile(catstr(outdir, substr(infile, ext), ".unc.ym"));
	if (verbose) printf("  .ym input file size = %d\n", qsize);
	if (verbose) printf("  .ym output file size = %d\n", zsize);
}

static void copy_as_ymrf(cstr indir, cstr outdir, cstr infile, Info& info)
{
	YMFileConverter converter;
	uint32			qsize	 = converter.importFile(catstr(indir, infile), verbose);
	cstr			ext		 = extension_from_path(infile); // points to '.' in infile
	cstr			basename = substr(infile, ext);
	if (write_hdr)
	{
		cstr   include_fname = catstr(infile, ".rsrc");		  // file for #include
		cstr   dest_fname	 = catstr(outdir, include_fname); // file written to
		cstr   rsrc_fname	 = catstr(basename, ".ymrf");	  // fname inside rsrc filesystem
		uint32 zsize		 = converter.exportRsrcFile(dest_fname, rsrc_fname, info.w, info.l);
		if (verbose) printf("  .ym input file size = %d \n", qsize);
		if (verbose) printf("  .ymrf output file size = %d \n", zsize);
		rsrc_files.append(include_fname);
	}
	else
	{
		uint32 zsize = converter.exportYMRegisterFile(catstr(outdir, basename, ".ymrf"));
		if (verbose) printf("  .ym input file size = %d \n", qsize);
		if (verbose) printf("  .ymrf output file size = %d \n", zsize);
	}
}

static void copy_as_img(cstr indir, cstr outdir, cstr infile, Info& info)
{
	ImageFileWriter c(info.hwcolor, !info.noalpha);
	c.importFile(catstr(indir, infile));
	if (verbose)
	{
		printf("  size = %i*%i\n", c.image_width, c.image_height);
		printf("  colors = %s\n", c.colormodel == c.grey ? "grey" : c.colormodel == c.rgb ? "rgb" : "hwcolor");
		if (c.has_cmap) printf("  cmap size = %i\n", c.cmap.count());
		if (c.has_transparency) printf("  has transparency\n");
	}

	cstr ext	  = extension_from_path(infile); // points to '.' in infile
	cstr basename = substr(infile, ext);

	if (write_hdr)
	{
		cstr   include_fname = catstr(infile, ".rsrc");		  // file for #include
		cstr   dest_fname	 = catstr(outdir, include_fname); // file written to
		cstr   rsrc_fname	 = catstr(basename, ".img");	  // fname inside rsrc filesystem
		uint32 size			 = c.exportRsrcFile(dest_fname, rsrc_fname);
		if (verbose) printf("  .rsrc file size = %d\n", size);
		rsrc_files.append(include_fname);
	}
	else
	{
		uint32 size = c.exportImgFile(catstr(outdir, basename, ".img"));
		if (verbose) printf("  .img file size = %d\n", size);
	}
}

static void copy_as_is(cstr indir, cstr outdir, cstr infile, Info& info)
{
	FILE* file = fopen(catstr(indir, infile), "rb");
	if (!file) throw "Unable to open source file";
	uint32				  fsize = getFileSize(file);
	std::unique_ptr<char> data {new char[fsize]};
	if (fread(data.get(), 1, fsize, file) != fsize) throw "Unable to read file";
	fclose(file);
	file = nullptr;

	if (verbose) printf("  file size = %d\n", fsize);

	if (!write_hdr)
	{
		FILE* file = fopen(catstr(outdir, infile), "wb");
		if (!file) throw "Unable to open output file";
		if (fwrite(data.get(), 1, fsize, file) != fsize) throw "Unable to write file";
		fclose(file);
		return;
	}

	cstr include_fname = catstr(infile, ".rsrc");		// file for #include
	cstr dest_fname	   = catstr(outdir, include_fname); // file written to
	cstr rsrc_fname	   = infile;						// fname inside rsrc filesystem

	if (info.w == 0) // write uncompressed rsrc file
	{
		RsrcFileWriter writer(dest_fname, rsrc_fname);
		writer.store(data.get(), fsize);
		writer.close();
		assert(writer.datasize == fsize);
	}
	else
	{
		CompressedRsrcFileWriter cwriter(dest_fname, rsrc_fname, info.w, info.l);
		cwriter.store(data.get(), fsize);
		cwriter.close();
		uint32 csize = cwriter.csize;

		if (verbose) printf("  compressed size = %d\n", csize);

		if (csize + 4 > fsize)
		{
			if (verbose) puts("*** compression increased size -> store uncompressed\n");

			RsrcFileWriter writer(dest_fname, rsrc_fname);
			writer.store(data.get(), fsize);
			writer.close();
			assert(writer.datasize == fsize);
		}
	}

	rsrc_files.append(include_fname);
}

static void convert_file(cstr indir, cstr outdir, cstr infile)
{
	try
	{
		if (verbose) printf("processing %s\n", infile);

		for (uint i = 0; i < infos.count(); i++)
		{
			Info& info = infos[i];
			if (!fnmatch(info.pattern, infile, true)) continue;

			switch (uint8(info.format))
			{
			case AS_IS: copy_as_is(indir, outdir, infile, info); return;
			case WAV: copy_as_wav(indir, outdir, infile); return;
			case YMM_WAV: copy_as_ymm_wav(indir, outdir, infile); return;
			case YM: copy_as_ym(indir, outdir, infile); return;
			case YMRF: copy_as_ymrf(indir, outdir, infile, info); return;
			case IMG: copy_as_img(indir, outdir, infile, info); return;
			case SKIP: return;
			default: IERR();
			}
		}
		if (verbose) printf("*** didn't match any pattern\n");
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


int main(int argc, cstr argv[])
{
	// 1 argument: job file
	// 2++ arguments: indir, outdir, format and options

	using namespace kio;

	try
	{
		if (argc == 3 && eq(argv[2], "-v"))
		{
			argc	= 2;
			verbose = true;
		}

		if (argc == 1)
		{
			puts(
				"1 argument = job_file\n"
				"2++ arguments = indir outdir format options\n"
				"formats: wav ym ymrf img as_is\n"
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

		if (write_hdr)
		{
			FILE* file = nullptr;
			file	   = fopen(catstr(outdir, "rsrc.cpp"), "wb");
			assert(file);
			fputs("extern const unsigned char resource_file_data[];\n", file);
			fputs("constexpr unsigned char resource_file_data[]={\n", file);
			for (uint i = 0; i < rsrc_files.count(); i++) { fprintf(file, "#include \"%s\"\n", rsrc_files[i]); }
			fputs("};\n", file);
			fclose(file);
		}
	}
	catch (cstr e)
	{
		printf("error: %s\n", e);
		return 1;
	}
	puts("all done.\n");
	return 0;
}


/*









































*/
