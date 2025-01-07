#include "RgbImageCompressor.h"
#include "common/cdefs.h"
#include "common/cstrings.h"
#include "common/standard_types.h"
#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <exception>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG 1
#include "extern/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "extern/stb/stb_image_write.h"


namespace kio
{
void panic(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	printf("\n");
	exit(2);
}

//static bool  	   write_rsrc = false;	 // will be enabled if no other is set
static cstr indir	  = nullptr; // must be set
static cstr outdir	  = nullptr; // will be set to indir if not set
static bool verbose	  = false;
static bool recursive = true;


static void convert_file(cstr indir, __unused cstr outdir, cstr infile)
{
	try
	{
		RgbImageCompressor encoder;
		encoder.encodeImage(indir, outdir, infile, true, DitherMode::Diffusion);
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
