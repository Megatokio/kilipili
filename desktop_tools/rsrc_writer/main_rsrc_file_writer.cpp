

#include "Array.h"
#include "Audio/Ay38912.h"
#include "CompressedRsrcFileWriter.h"
#include "RsrcFileWriter.h"
#include "YMFileConverter.h"
#include "cdefs.h"
#include "cstrings.h"
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
static uint		   wsize	  = 12;
static uint		   lsize	  = 6;
static bool		   write_wav  = false;
static bool		   write_ym	  = false;
static bool		   write_ymrf = false;
static bool		   write_hdr  = false;	 // will be enabled if no other is set
static cstr		   indir	  = nullptr; // must be set
static cstr		   outdir	  = nullptr; // will be set to indir if not set
static bool		   verbose	  = false;
static bool		   recursive  = true;
static Array<cstr> rsrc_files;

using namespace Audio;

static uint32 getFileSize(FILE* fp)
{
	long fSize;
	long oPos;
	oPos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	fSize = ftell(fp);
	fseek(fp, oPos, SEEK_SET);
	assert_eq(fSize, uint32(fSize));
	return uint32(fSize);
}

void convert(cstr indir, cstr outdir, cstr infile)
{
	cstr ext	  = extension_from_path(infile);
	cstr basename = substr(infile, ext); // includes partial path

	if (lceq(ext, ".ym"))
	{
		if (verbose) printf("converting %s\n", infile);

		YMFileConverter converter(YMFileConverter::NOTHING);
		converter.importFile(catstr(indir, infile), verbose);
		if (write_wav)
		{
			uint32 size = converter.exportWavFile(catstr(outdir, basename, ".wav"));
			if (verbose) printf(".wav size: %d\n", size);
		}
		if constexpr (false) // for reference
		{
			converter.exportYMMusicWavFile(catstr(indir, infile), catstr(outdir, basename, ".ymm.wav"));
		}
		if (write_ym)
		{
			uint32 size = converter.exportYMFile(catstr(outdir, basename, ".unc.ym"));
			if (verbose) printf(".ym   size: %d\n", size);
		}
		if (write_ymrf)
		{
			uint32 size = converter.exportYMRegisterFile(catstr(outdir, basename, ".ymrf"));
			if (verbose) printf(".ymrf size: %d\n", size);
		}
		if (write_hdr) // store YM file as YMRF (YM register file) into resource
		{
			cstr   include_fname = catstr(basename, ".rsrc");	  // file for #include
			cstr   dest_fname	 = catstr(outdir, include_fname); // file written to
			cstr   rsrc_fname	 = catstr(basename, ".ymrf");	  // fname inside rsrc filesystem
			uint32 size			 = converter.exportRsrcFile(dest_fname, rsrc_fname);
			if (verbose) printf(".rsrc csize: %d \n", size);
			rsrc_files.append(include_fname);
		}
	}
	//else if (lceq(ext, ".gif"))
	//{
	//	// copy verbatim or recompress raw image with heatshrink?
	//}
	else
	{
		if (verbose) printf("storing %s\n", infile);

		FILE* file = fopen(catstr(indir, infile), "rb");
		if (!file) throw "Unable to open source file";
		uint32				  fsize = getFileSize(file);
		std::unique_ptr<char> data {new char[fsize]};
		if (fread(data.get(), 1, fsize, file) != fsize) throw "Unable to read file";
		fclose(file);
		file = nullptr;

		cstr include_fname = catstr(infile, ".rsrc");		// file for #include
		cstr dest_fname	   = catstr(outdir, include_fname); // file written to
		cstr rsrc_fname	   = infile;						// fname inside rsrc filesystem

		CompressedRsrcFileWriter cwriter(dest_fname, rsrc_fname, uint8(wsize), uint8(lsize));
		cwriter.store(data.get(), fsize);
		cwriter.close();
		uint32 csize = cwriter.csize;

		if (verbose) printf("  fsize: %d \n", fsize);
		if (verbose) printf("  csize: %d \n", csize);

		if (csize + 4 > fsize)
		{
			if (verbose) printf("compression increased size --> stored uncompressed\n");
			RsrcFileWriter writer(dest_fname, rsrc_fname);
			writer.store(data.get(), fsize);
			writer.close();
			assert(writer.datasize == fsize);
		}

		rsrc_files.append(include_fname);
	}
}

void convert_dir(cstr indir, cstr outdir, cstr subdir)
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

		try
		{
			convert(indir, outdir, catstr(subdir, de->d_name));
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
	closedir(dir);
}
} // namespace kio

/*
	-W12 -L6 [-WAV -YM -HDR] -v indir [outdir]

	reads all known indir/files and converts and stores them in outdir/files
	-W12 set window size for heatshrink compressor.     default=12
	-L6  set look-ahead size for heatshrink compressor. default=6
	-WAV convert audio files to WAV
	-YM  convert YM audio files to uncompressed interleaved YM files
	-HDR store the output files as resource (default)
		 also create combining header file
*/
int main(int argc, cstr argv[])
{
	using namespace kio;

	try
	{
		int i = 1;
		for (;; i++)
		{
			if (i == argc) { throw("input directory missing"); }
			cstr s = argv[i];
			if (s[0] != '-') break;
			if (eq(s, "-WAV")) write_wav = true;		// YM file -> WAV file
			else if (eq(s, "-YM")) write_ym = true;		// YM file -> uncompressed YM file
			else if (eq(s, "-YMRF")) write_ymrf = true; // YM file -> YM register file
			else if (eq(s, "-HDR")) write_hdr = true;	// misc. -> resource file
			else if (eq(s, "-v")) verbose = true;
			else if (s[1] == 'W')
			{
				wsize = 0;
				for (uint i = 2; is_decimal_digit(s[i]); i++) wsize = wsize * 10 + dec_digit_value(s[i]);
			}
			else if (s[1] == 'L')
			{
				lsize = 0;
				for (uint i = 2; is_decimal_digit(s[i]); i++) lsize = lsize * 10 + dec_digit_value(s[i]);
			}
			else { throw usingstr("unrecognized option %s", s); }
		}
		indir = argv[i++];
		if (i < argc && argv[i][0] != '-') outdir = argv[i++];
		if (i < argc) { throw("options after directory paths?"); }
		if (wsize < 6 || wsize > 14) throw "wsize oorange";
		if (lsize < 4 || lsize > 8) throw "lsize oorange";
		if (lsize + 2 > wsize) throw "lsize too large or wsize too small";
		if (!write_wav && !write_ym) write_hdr = true;
		if (!outdir) outdir = indir;
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
