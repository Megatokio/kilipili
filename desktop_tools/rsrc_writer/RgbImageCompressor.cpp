// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "RgbImageCompressor.h"
#include "Devices/StdFile.h"
#include "common/Xoshiro128.h"
#include "common/basic_math.h"
#include "common/cstrings.h"

#define STBI_FAILURE_USERMSG 1
#include "extern/stb/stb_image.h"
#include "extern/stb/stb_image_write.h"

#ifndef RSRC_MAX_IMG_WIDTH
  #define RSRC_MAX_IMG_WIDTH 1024
#endif
#ifndef RSRC_MAX_IMG_HEIGHT
  #define RSRC_MAX_IMG_HEIGHT RSRC_MAX_IMG_WIDTH
#endif


namespace kio
{
using namespace Graphics;
using namespace Devices;
using namespace std;


// easy access:
static constexpr int rbits = Color::rbits;
static constexpr int gbits = Color::gbits;
static constexpr int bbits = Color::bbits;

static constexpr int rshift = 8 - rbits;
static constexpr int gshift = 8 - gbits;
static constexpr int bshift = 8 - bbits;

static constexpr int rmask = (1 << rbits) - 1;
static constexpr int gmask = (1 << gbits) - 1;
static constexpr int bmask = (1 << bbits) - 1;

// components are weighted rgb = 453 (as in Color::distance())
static constexpr int average_weight = 4;
static constexpr int rweight		= (average_weight + 0) << (gbits - rbits);
static constexpr int gweight		= (average_weight + 1);
static constexpr int bweight		= (average_weight - 1) << (gbits - bbits);


// number of initial code points in the rel_codes map:
// ~100 .. ~200
// we currently end up with ~190 (RGB555) or more than 200 (RGB444) but less may be better (TODO)
static constexpr int initial_num_rel_codes = 128;

// boost deviation for large deviations:
// quadratic: devi = devi * devi
// linear:    devi = devi_max + (devi - devi_max) * devi_fak
// test result: linear with below parameters better than quadratic
static constexpr bool devi_linear	 = true;
static constexpr bool devi_quadratic = !devi_linear;

// linear: limit from where to boost:
// average_weight << (gbits - 2)   =>  ~deviation of one component by 1/4 of max. value
// test result: RGB555: average_weight*5.0, RGB444: average_weight*2.0
static constexpr int devi_max = average_weight * (rbits == 5 ? 5 : rbits == 4 ? 2 : 1);

// linear:  boost factor above devi_max:
// 1 or 2 .. ~80
// test result: 10 ~ best, 6 and 14 are only slightly worse
static constexpr int devi_fak = 10;

// in encode(): mark usage for the other rel or abs code too if deviation is very high:
// test result: this should generally be enabled
static constexpr bool high_deviation_other_boost = true;

// the table for codepoints for relative colors:
// table size must be larger than max. offset intended to use
// because we want to be able to mitigate x+1, x+2 and evtl. x+3 to x if this fits better.
// -7 … +7 => 2*7+1 = 15 => 15*15*15 = 3375 codepoints.
// -8 … +8 => 2*8+1 = 17 => 17*17*17 = 4913 codepoints.
// -9 … +9 => 2*9+1 = 19 => 19*19*19 = 6859 codepoints.
// test result: RGB555: 6<7<8 but the improvement is little
//              RGB444: 6<7>8 the worsening of 8 is quite noticable
static constexpr int rel_max = 7;
static constexpr int rel_dim = 2 * rel_max + 1;

// encodeImage(): number of consecutive worse runs before we bail out and take the best:
// beyond 2 there is not much improvement
// test result: any better result aka `total_deviation` comes at cost of more high deviations! TODO
constexpr int max_bad_runs = 2;


//////////////////////////////////////////////////////////////////////////////////////////////////


int	   RgbImageCompressor::total_num_images		   = 0;
int	   RgbImageCompressor::total_num_abs_codes	   = 0; // sum of all!
int	   RgbImageCompressor::total_num_rel_codes	   = 0; // sum of all!
double RgbImageCompressor::total_total_deviation   = 0; // sum of all!
double RgbImageCompressor::total_average_deviation = 0; // sum of all!
uint32 RgbImageCompressor::total_deviations[64] {0};	// sum of all!

struct AbsCode;
struct RelCode;
struct AbsCodes;
struct RelCodes;
struct RgbImage;
struct EncodedImage;


static constexpr char rgbstr[] = {'r', 'g', 'b', '0' + rbits, '0' + gbits, '0' + bbits, 0};

constexpr uint16 plain_deviation(int dr, int dg, int db)
{
	// returns `deviation * average_weight`

	return uint16((abs(dr) * rweight) + (abs(dg) * gweight) + (abs(db) * bweight));
}

constexpr uint weighted_deviation(uint d)
{
	if constexpr (devi_quadratic) return min(d * d, 0xfffeU);
	else if constexpr (devi_fak == 1) return d;
	else if (d <= devi_max) return d;
	else return devi_max + (d - devi_max) * devi_fak; // this fits in int16 up to devi_fak ~ 88
}

constexpr double unweighted_deviation(double weighted_deviation)
{
	if constexpr (devi_quadratic) return pow(weighted_deviation, 0.5) / average_weight;
	else if constexpr (devi_fak == 1) return weighted_deviation / average_weight;

	// this fu is only used on the average, so we should never see xlarge values:
	if constexpr (1) return weighted_deviation / average_weight;

	if (weighted_deviation <= devi_max) return weighted_deviation / average_weight;
	return devi_max + (weighted_deviation - devi_max) / devi_fak / average_weight;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


// A 3-dimensional map of absolute colors
//
template<int rdim, int gdim, int bdim>
struct RgbCube
{
	float values[rdim][gdim][bdim];
	int	  rc = 0;

	RgbCube() { memset(values, 0, sizeof(values)); }

	void   blur(int steps);
	float& center() { return values[rdim / 2][gdim / 2][bdim / 2]; }
	bool   find_maximum(int& r, int& g, int& b, int padding);

	template<int rdim2, int gdim2, int bdim2>
	void punch(int r_center, int g_center, int b_center, const RgbCube<rdim2, gdim2, bdim2>&);
};


// A Hole can be multiplied into a DeviationMap to remove deviation after adding a new color.
//
static constexpr int padding  = rbits - 2;		 // 3x3x3 -> 1,      4x4x4 -> 2,      5x5x5 -> 3
static constexpr int diameter = padding * 2 + 1; // 3x3x3 -> 1+1+1,  4x4x4 -> 2+1+2,  5x5x5 -> 3+1+3
struct Hole : public RgbCube<diameter, diameter, diameter>
{
	Hole();
};
static const Hole hole; // we always use the same map


// a DeviationMap covers all possible colors plus some padding, enough for bluring
// and to punch holes for new colors without border test.
//
using DeviationMap = RgbCube<(1 << rbits) + 2 * padding, (1 << gbits) + 2 * padding, (1 << bbits) + 2 * padding>;


// An AbsCode stores r, g and b component in their component size inside Color.
// They are stored in AbsCodes.
//
struct alignas(int) AbsCode
{
	uint8 r, g, b;
	uint8 code;

	constexpr int  distance(int r, int g, int b) const;
	constexpr bool operator==(const AbsCode& o) const;
	void		   operator+=(const struct RelCode& delta);

	constexpr operator Color() const { return Color(Color::mkred(r) | Color::mkgreen(g) | Color::mkblue(b)); }
};


// A RelCode stores r, g and b differences to 'the previous pixel'.
// They are stored in RelCodes.
//
struct alignas(int) RelCode
{
	int8  dr, dg, db;
	uint8 code = 0;

	int	 distance(int dr, int dg, int db) const;
	bool operator==(const RelCode& o) const { return dr == o.dr && dg == o.dg && db == o.db; }
		 operator Color() const;
};


// struct 'AbsCodes' stores the AbsCodes currently used to encode an image.
// It provides to add and remove colors from this table.
// It provides a map to quickly find a code for a given rgb value along with it's deviation from the desired value.
// It provides functions to add and remove colors to minimize the deviation.
//
struct AbsCodes
{
	struct ColorInfo
	{
		uint16	deviation;
		uint16	usage;
		AbsCode abs_code;
		void	add_usage() { usage += usage < 0xffff; }
		void	add_usage(int n) { usage += min(n, 0xffff - usage); }
		uint	deviation_x_usage() const { return deviation * usage; }
				operator bool() const { return deviation != 0xffffu; } // true = is_valid
	};
	struct CodeInfo
	{
		AbsCode abs_code;
		uint	usage;
	};

	bool code_map_changed;
	char _padding[3];
	int	 num_codes = 0;

	CodeInfo			codes[256];
	ColorInfo			colors[1 << rbits][1 << gbits][1 << bbits];
	RCPtr<DeviationMap> deviation_map;

	AbsCodes(RgbImage*, RelCodes*);
	AbsCodes(const AbsCodes&);
	uint	   find_lowest_code_usage();
	void	   remove_codes_below_limit(uint min_usage_count);
	void	   add_codes(int count);
	void	   remove_codes(int count);
	void	   remove_code(int);
	ColorInfo& get(int r, int g, int b);
	void	   calculate_code_usages();
	void	   print_color_map(File*);
	void	   fix_colors();
	void	   clear_code_usages();
};


// struct 'RelCodes' stores the RelCodes currently used to encode an image.
// It provides to add and remove colors from this table.
//
struct RelCodes
{
	struct ColorInfo
	{
		uint16	deviation;
		uint16	usage;
		RelCode rel_code;
		void	add_usage() { usage += usage != 0xffffu; }
		void	add_usage(int n) { usage += min(n, 0xffff - usage); }
				operator bool() const { return deviation != 0xffffu; } // true = is_valid
	};
	struct CodeInfo
	{
		RelCode rel_code;
		uint	usage;
	};

	bool code_map_changed;
	char _padding[3];
	int	 first_code = 256;

	CodeInfo  codes[256];
	ColorInfo colors[rel_dim][rel_dim][rel_dim];

	RelCodes(RgbImage*);
	RelCodes(const RelCodes&);
	void	   remove_codes(int count);
	void	   remove_code(int);
	void	   add_codes(int count);
	uint	   find_lowest_code_usage();
	void	   remove_codes_below_limit(uint min_usage_count);
	ColorInfo& get(uint* deviation_out, int dr, int dg, int db);
	void	   calculate_code_usages();
	void	   print_color_map(File*);
	void	   clear_code_usages();
	void	   fix_colors();

private:
	//ColorInfo scratch; // for get()

	void fix_map_entry(ColorInfo&, int dr, int dg, int db);
};

struct RgbImage
{
	NO_COPY_MOVE(RgbImage);

	static constexpr int num_channels = 3;

	int	   width, height;
	uint8* data = nullptr;
	uint   num_colors;
	bool   is_rgb888 = false;
	char   _padding[7];
	uint   rc = 0;

	RgbImage(cstr fpath, DitherMode); // read original image from file
	RgbImage(struct EncodedImage*);	  // decode the encoded image
	~RgbImage() { delete[] data; }

	void write_to_file(cstr fpath);
	void make_diff_image(RgbImage*);
	void print_statistics(File*);
	void to_native_depth();
	void calc_deviation_map(RgbImage* other, uint map[256]);
	void print_deviation_map(File*, uint map[256]);

private:
	void count_colors();
	void reduce_color_depth(DitherMode);
	void to_rgb888();
};

struct EncodedImage
{
	NO_COPY_MOVE(EncodedImage);

	RCPtr<RgbImage> rgb_image;
	RelCodes		rel_codes;
	AbsCodes		abs_codes;
	int				width, height;
	uint8*			data = nullptr;
	Color			ctab[256];
	uint32			total_weighted_deviation = 0;
	uint			rc						 = 0;

	EncodedImage(RgbImage*);
	EncodedImage(EncodedImage*, int min_gap);
	~EncodedImage() { delete[] data; }

	void   write_to_file(File*);
	void   write_to_file(cstr fpath);
	void   print_statistics(File*);
	double unweighted_average_deviation() const;
	double unweighted_total_deviation() const;

private:
	void encode();
};


////////////////////////////////////////////////////////////
////					RgbCube 						////
////////////////////////////////////////////////////////////


template<int rdim, int gdim, int bdim>
void RgbCube<rdim, gdim, bdim>::blur(int n)
{
	// blur n times
	// each pixel affects all pixels in range [-n .. +n]
	// the border pixels are assumed to be padding and receive bleeding only.

	while (--n >= 0)
	{
		// 3x3x3 blur map:
		//  2 3 2    3 5 3    2 3 2
		//  3 5 3    5 8 5    3 5 3
		//  2 3 2    3 5 3    2 3 2
		// sum = 1 * a + 6 * b + 12 * c + 8 * d = 90

		constexpr float a = 8;
		constexpr float b = 5;
		constexpr float c = 3;
		constexpr float d = 2;

		RgbCube z;

		for (int i = 1; i < rdim - 1; i++)
			for (int j = 1; j < gdim - 1; j++)
				for (int k = 1; k < bdim - 1; k++)
				{
					float v = 0;
					v += values[i - 1][j - 1][k - 1] * d;
					v += values[i - 1][j - 1][k + 0] * c;
					v += values[i - 1][j - 1][k + 1] * d;
					v += values[i - 1][j + 0][k - 1] * c;
					v += values[i - 1][j + 0][k + 0] * b;
					v += values[i - 1][j + 0][k + 1] * c;
					v += values[i - 1][j + 1][k - 1] * d;
					v += values[i - 1][j + 1][k + 0] * c;
					v += values[i - 1][j + 1][k + 1] * d;
					v += values[i + 0][j - 1][k - 1] * c;
					v += values[i + 0][j - 1][k + 0] * b;
					v += values[i + 0][j - 1][k + 1] * c;
					v += values[i + 0][j + 0][k - 1] * b;
					v += values[i + 0][j + 0][k + 0] * a;
					v += values[i + 0][j + 0][k + 1] * b;
					v += values[i + 0][j + 1][k - 1] * c;
					v += values[i + 0][j + 1][k + 0] * b;
					v += values[i + 0][j + 1][k + 1] * c;
					v += values[i + 1][j - 1][k - 1] * d;
					v += values[i + 1][j - 1][k + 0] * c;
					v += values[i + 1][j - 1][k + 1] * d;
					v += values[i + 1][j + 0][k - 1] * c;
					v += values[i + 1][j + 0][k + 0] * b;
					v += values[i + 1][j + 0][k + 1] * c;
					v += values[i + 1][j + 1][k - 1] * d;
					v += values[i + 1][j + 1][k + 0] * c;
					v += values[i + 1][j + 1][k + 1] * d;
					z.values[i][j][k] = v;
				}

		*this = z;
	}
}

template<int rdim, int gdim, int bdim>
bool RgbCube<rdim, gdim, bdim>::find_maximum(int& r, int& g, int& b, int padding)
{
	r = g = b	  = 0;
	float maximum = 0;
	for (int ri = padding; ri < rdim - padding; ri++)
		for (int gi = padding; gi < gdim - padding; gi++)
			for (int bi = padding; bi < bdim - padding; bi++)
				if unlikely (values[ri][gi][bi] > maximum) { maximum = values[r = ri][g = gi][b = bi]; }
	return r != 0; // 0=nothing left => invalid return, 1=valid return
}

template<int rdim, int gdim, int bdim>
template<int rdim2, int gdim2, int bdim2>
void RgbCube<rdim, gdim, bdim>::punch(int r0, int g0, int b0, const RgbCube<rdim2, gdim2, bdim2>& hole)
{
	// punch a hole for a new color at rgb into a deviation map
	// remove deviation roughly as provided by the new color

	r0 -= rdim2 / 2;
	g0 -= gdim2 / 2;
	b0 -= bdim2 / 2;

	assert(r0 >= 0 && r0 + rdim2 <= rdim);
	assert(g0 >= 0 && g0 + gdim2 <= gdim);
	assert(b0 >= 0 && b0 + bdim2 <= bdim);

	for (int r = 0; r < rdim2; r++)
		for (int g = 0; g < gdim2; g++)
			for (int b = 0; b < bdim2; b++) { values[r0 + r][g0 + g][b0 + b] *= hole.values[r][g][b]; }
}

Hole::Hole()
{
	center() = 1;				  // where the color is
	blur(padding);				  // blur it
	float  f   = 1.0f / center(); // the center must be reduced to zero
	float* map = &values[0][0][0];
	for (int i = 0; i < diameter * diameter * diameter; i++) { map[i] = 1.0f - map[i] * f; }
}


////////////////////////////////////////////////////////////
////					AbsCode							////
////////////////////////////////////////////////////////////


constexpr int AbsCode::distance(int r, int g, int b) const
{
	return abs(this->r - r) * rweight + abs(this->g - g) * gweight + abs(this->b - b) * bweight;
}

constexpr bool AbsCode::operator==(const AbsCode& o) const
{
	return r == o.r && g == o.g && b == o.b; //
}

void AbsCode::operator+=(const RelCode& o)
{
	r += o.dr;
	g += o.dg;
	b += o.db;

	// component must not overflow:
	assert(uint(r) <= rmask && uint(g) <= gmask && uint(b) <= bmask);
}


////////////////////////////////////////////////////////////
////					RelCode							////
////////////////////////////////////////////////////////////

int RelCode::distance(int _dr, int _dg, int _db) const
{
	// this{dr,dg,db} = destination
	// {_dr,_dg,_db}  = source value
	// the destination must be same or closer to {0,0,0} than the source

	// note:
	//  if (dr > 0 && _dr < dr) return 999;
	//  if (dr < 0 && _dr > dr) return 999;
	// <=>
	//  if (dr > 0 && _dr-dr < 0) return 999;
	//  if (dr < 0 && _dr-dr > 0) return 999;
	// <=>
	//  if (dr * (_dr-dr) < 0) return 999;

	if (dr * (_dr -= dr) < 0) return 0xfffe;
	if (dg * (_dg -= dg) < 0) return 0xfffe;
	if (db * (_db -= db) < 0) return 0xfffe;
	return abs(_dr) * rweight + abs(_dg) * gweight + abs(_db) * bweight;
}

RelCode::operator Color() const
{
	// create a pseudo color which, when added to another color, results in a color with this difference.
	// note: negative delta causes overflow into the following component.
	// therefore we calculate the raw difference between two example colors with the difference dr,dg,db.

	int r0 = dr >= 0 ? 0 : rmask;
	int g0 = dg >= 0 ? 0 : gmask;
	int b0 = db >= 0 ? 0 : bmask;
	int r1 = r0 + dr;
	int g1 = g0 + dg;
	int b1 = b0 + db;

	Color c0 {Color::mkred(r0) + Color::mkgreen(g0) + Color::mkblue(b0)};
	Color c1 {Color::mkred(r1) + Color::mkgreen(g1) + Color::mkblue(b1)};

	return Color {c1.raw - c0.raw};
}


////////////////////////////////////////////////////////////
////					RelCodes						////
////////////////////////////////////////////////////////////


inline RelCodes::ColorInfo& RelCodes::get(uint* deviation_out, int dr, int dg, int db)
{
	int ri = rel_max + dr;
	int gi = rel_max + dg;
	int bi = rel_max + db;

	int deviation = 0;

	if unlikely (uint(ri) >= rel_dim)
	{
		if (ri < 0) { deviation -= ri * rweight, ri = 0; }
		else { deviation += (ri - (rel_dim - 1)) * rweight, ri = rel_dim - 1; }
	}
	if unlikely (uint(gi) >= rel_dim)
	{
		if (gi < 0) { deviation -= gi * gweight, gi = 0; }
		else { deviation += (gi - (rel_dim - 1)) * gweight, gi = rel_dim - 1; }
	}
	if unlikely (uint(bi) >= rel_dim)
	{
		if (bi < 0) { deviation -= bi * bweight, bi = 0; }
		else { deviation += (bi - (rel_dim - 1)) * bweight, bi = rel_dim - 1; }
	}

	ColorInfo& info = colors[ri][gi][bi];
	*deviation_out	= uint(info.deviation + deviation);
	return info;
}

RelCodes::RelCodes(RgbImage* image)
{
	// prepare rel_codes[] for first run

	image->to_native_depth();

	// clear all rel_color usage counts:
	memset(colors, 0, sizeof(colors));

	// count rel_color usage:
	AbsCode current {0, 0, 0, 0};
	uint8*	q = image->data + image->num_channels;
	for (int y = 0; y < image->height; y++, q += image->num_channels)
		for (int x = 1; x < image->width; x++, q += image->num_channels)
		{
			uint8 r_neu = q[0], g_neu = q[1], b_neu = q[2];

			int ri = rel_max + r_neu - current.r;
			int gi = rel_max + g_neu - current.g;
			int bi = rel_max + b_neu - current.b;
			if (uint(ri) < rel_dim && uint(gi) < rel_dim && uint(bi) < rel_dim) { colors[ri][gi][bi].usage += 1; }
			current = {r_neu, g_neu, b_neu, 0};
		}

	code_map_changed = true; // fix the color map
	first_code		 = 256;
	add_codes(initial_num_rel_codes);
}

RelCodes::RelCodes(const RelCodes& q) : //
	code_map_changed(q.code_map_changed),
	first_code(q.first_code)
{
	memcpy(codes, q.codes, sizeof(codes));
	memcpy(colors, q.colors, sizeof(colors));
}

void RelCodes::clear_code_usages()
{
	ColorInfo* infos = &colors[0][0][0];
	for (int i = 0; i < rel_dim * rel_dim * rel_dim; i++) { infos[i].usage = 0; }
}

inline void RelCodes::fix_map_entry(ColorInfo& info, int dr, int dg, int db)
{
	int best_code = 255;
	int deviation = 0xffffu;
	for (int i = first_code; i < 256; i++)
	{
		int d = codes[i].rel_code.distance(dr, dg, db);
		if (d < deviation) deviation = d, best_code = i;
	}
	info.deviation = uint16(deviation);
	info.rel_code  = codes[best_code].rel_code;
}

void RelCodes::fix_colors()
{
	//if (code_map_changed == false) return;
	code_map_changed = false;

	for (int ri = 0; ri < rel_dim; ri++)
		for (int gi = 0; gi < rel_dim; gi++)
			for (int bi = 0; bi < rel_dim; bi++)
			{
				fix_map_entry(colors[ri][gi][bi], ri - rel_max, gi - rel_max, bi - rel_max);
			}
}

void RelCodes::add_codes(int count)
{
	assert(count < first_code);

	fix_colors();

	while (--count >= 0)
	{
		int best_usage = 0;
		int best_ri = 0, best_gi = 0, best_bi = 0;

		for (int ri = 0; ri < rel_dim; ri++)
			for (int gi = 0; gi < rel_dim; gi++)
				for (int bi = 0; bi < rel_dim; bi++)
				{
					ColorInfo& info = colors[ri][gi][bi];
					if (info.usage <= best_usage) continue; // not better
					if (info.deviation == 0) continue;		// already in codes[]
					best_usage = info.usage, best_ri = ri, best_gi = gi, best_bi = bi;
				}

		if (best_usage == 0) break; // all codes covered

		uint8 code			 = uint8(--first_code);
		codes[code].rel_code = {int8(best_ri - rel_max), int8(best_gi - rel_max), int8(best_bi - rel_max), code};
		colors[best_ri][best_gi][best_bi].deviation = 0;
		code_map_changed							= true;
	}
}

void RelCodes::calculate_code_usages()
{
	fix_colors();
	for (int i = first_code; i < 256; i++) { codes[i].usage = 0; }
	ColorInfo* infos = &colors[0][0][0];
	for (int i = 0; i < rel_dim * rel_dim * rel_dim; i++)
	{
		ColorInfo& info = infos[i];
		if (info.usage) codes[info.rel_code.code].usage += info.usage;
	}
}

void RelCodes::remove_code(int i)
{
	assert(i >= first_code);

	codes[i]			   = codes[first_code++];
	codes[i].rel_code.code = uint8(i);
	code_map_changed	   = true;
}

uint RelCodes::find_lowest_code_usage()
{
	assert(first_code <= 255);

	calculate_code_usages();

	uint min_usage = codes[255].usage;
	for (int i = first_code; i < 255; i++) { min_usage = min(min_usage, codes[i].usage); }
	return min_usage;
}

void RelCodes::remove_codes_below_limit(uint min_usage_count)
{
	calculate_code_usages();

	for (int i = first_code; i < 256; i++)
	{
		if (codes[i].usage < min_usage_count) remove_code(i);
	}
}

void RelCodes::remove_codes(int count)
{
	if (first_code + count > 256)
	{
		printf("RelCodes: removing all codes\n");
		count = 256 - first_code;
	}

	calculate_code_usages();

	while (--count >= 0)
	{
		int	 min_code  = 255;
		uint min_usage = codes[255].usage;

		for (int i = 255; --i >= first_code;)
		{
			if (codes[i].usage < min_usage) min_usage = codes[i].usage, min_code = i;
		}

		remove_code(min_code);
	}
}

void RelCodes::print_color_map(File* file)
{
	file->printf("\nREL CODES:\n");

	for (int ri = 0; ri < rel_dim; ri++)
	{
		file->printf(
			"dr=%2i:  db: %i  %i  %i  %+i  %+i  %+i  %+i ...\n", ri - rel_max, //
			1 - rel_max, 2 - rel_max, 3 - rel_max, 4 - rel_max, 5 - rel_max, 6 - rel_max, 7 - rel_max);
		for (int gi = 0; gi < rel_dim; gi++)
		{
			file->printf("dg=%+i: ", gi - rel_max);
			for (int bi = 0; bi < rel_dim; bi++)
			{
				bool in_map = colors[ri][gi][bi].deviation == 0;
				uint n		= colors[ri][gi][bi].usage;
				if (in_map)
				{
					if (n >= 100) file->printf("[XX]");
					else if (n != 0) file->printf("[%2u]", n);
					else file->printf("[--]");
				}
				else
				{
					if (n >= 100) file->printf(" XX ");
					else if (n != 0) file->printf(" %2u ", n);
					else file->printf(" -- ");
				}
			}
			file->printf("\n");
		}
		file->printf("\n");
	}
}


////////////////////////////////////////////////////////////
////					AbsCodes						////
////////////////////////////////////////////////////////////


AbsCodes::AbsCodes(RgbImage* image, RelCodes* rel_codes)
{
	// prepare abs_codes[] for first run
	// mark all unused colors as invalid
	// mark all used colors as valid with max. deviation => rel_colors will be used where possible

	image->to_native_depth();

	// mark all colors invalid:
	memset(colors, 0xff, sizeof(colors));

	// mark used colors valid:
	uint8* q = image->data;
	for (int i = 0; i < image->width * image->height; i++, q += image->num_channels)
	{
		colors[q[0]][q[1]][q[2]].deviation = 0xfffd;
	}

	code_map_changed = false; // don't fix the color map
	num_codes		 = 0;
	add_codes(rel_codes->first_code);
}

AbsCodes::AbsCodes(const AbsCodes& q) :
	code_map_changed(q.code_map_changed),
	num_codes(q.num_codes),
	deviation_map(nullptr)
{
	memcpy(codes, q.codes, sizeof(codes));
	memcpy(colors, q.colors, sizeof(colors));
}

void AbsCodes::clear_code_usages()
{
	ColorInfo* infos = &colors[0][0][0];
	for (int i = 0; i < 1 << (rbits + gbits + bbits); i++) { infos[i].usage = 0; }
}

void AbsCodes::fix_colors()
{
	if (code_map_changed == false) return;
	code_map_changed = false;

	for (int r = 0; r < 1 << rbits; r++)
		for (int g = 0; g < 1 << gbits; g++)
			for (int b = 0; b < 1 << bbits; b++)
				if (ColorInfo& info = colors[r][g][b])
				{
					int best_code = 0;
					int deviation = 0xffff;
					for (int i = 0; i < num_codes; i++)
					{
						int d = codes[i].abs_code.distance(r, g, b);
						if (d < deviation) deviation = d, best_code = i;
					}
					info.deviation = uint16(deviation);
					info.abs_code  = codes[best_code].abs_code;
				}
}

inline AbsCodes::ColorInfo& AbsCodes::get(int r, int g, int b)
{
	return colors[r][g][b]; //
}

void AbsCodes::add_codes(int count)
{
	// a quick method is to just find the colors which are used most often.
	// this will not find clusters of colors with low usage per color,
	// though putting a code in their middle would benefit a lot.
	// this requires bluring the map, which is quite costly:
	// - create 3D usage map (using usage*deviation)
	// - blur the map
	// - for all available colors subtract blured holes from map
	// - repeat for the requested number of colors:
	//   - in the resulting map find the highest point and add a code for it
	//   - subtract blured hole from map

	assert(count <= 256 - num_codes);

	if (!deviation_map)
	{
		deviation_map = new DeviationMap;

		for (int r = 0; r < 1 << rbits; r++)
			for (int g = 0; g < 1 << gbits; g++)
				for (int b = 0; b < 1 << bbits; b++)
					if (uint d = colors[r][g][b].deviation_x_usage())
						deviation_map->values[r + padding][g + padding][b + padding] = float(d);

		deviation_map->blur(padding);

		for (int i = 0; i < num_codes; i++)
		{
			AbsCode& code = codes[i].abs_code;
			deviation_map->punch(code.r + padding, code.g + padding, code.b + padding, hole);
		}
	}

	for (int i = 0; i < count; i++)
	{
		int	 r, g, b;
		bool valid = deviation_map->find_maximum(r, g, b, padding);
		if unlikely (!valid) break;
		assert(uint(r - padding) < 1 << rbits && uint(g - padding) < 1 << gbits && uint(b - padding) < 1 << bbits);
		codes[num_codes].abs_code = AbsCode {
			.r = uint8(r - padding), .g = uint8(g - padding), .b = uint8(b - padding), .code = uint8(num_codes)};
		num_codes++;
		code_map_changed = true;
		deviation_map->punch(r, g, b, hole);
	}
}

void AbsCodes::calculate_code_usages()
{
	fix_colors();
	for (int i = 0; i < num_codes; i++) { codes[i].usage = 0; }
	ColorInfo* infos = &colors[0][0][0];
	for (int i = 0; i < 1 << (rbits + gbits + bbits); i++)
	{
		ColorInfo& info = infos[i];
		if (info.usage) codes[info.abs_code.code].usage += info.usage;
	}
}

uint AbsCodes::find_lowest_code_usage()
{
	calculate_code_usages();

	uint min_usage = 999999;
	for (int i = 0; i < num_codes; i++) { min_usage = min(min_usage, codes[i].usage); }
	return min_usage;
}

void AbsCodes::remove_code(int i)
{
	assert(i < num_codes);

	codes[i]			   = codes[--num_codes];
	codes[i].abs_code.code = uint8(i);
	code_map_changed	   = true;
	deviation_map		   = nullptr;
}

void AbsCodes::remove_codes(int count)
{
	if (count >= num_codes)
	{
		printf("AbsCodes: removing all codes\n");
		count = num_codes;
	}

	calculate_code_usages();

	while (--count >= 0)
	{
		uint min_usage = codes[0].usage;
		int	 min_code  = 0;

		for (int i = 1; i < num_codes; i++)
		{
			if (codes[i].usage < min_usage) min_usage = codes[i].usage, min_code = i;
		}
		remove_code(min_code);
	}
}

void AbsCodes::remove_codes_below_limit(uint min_usage_count)
{
	calculate_code_usages();

	for (int i = num_codes; --i >= 0;)
	{
		if (codes[i].usage >= min_usage_count) continue;
		codes[i]			   = codes[--num_codes];
		codes[i].abs_code.code = uint8(i);
	}
	code_map_changed = true;
	deviation_map	 = nullptr;
}

void AbsCodes::print_color_map(File* file)
{
	if constexpr (rbits + gbits + bbits <= 12)
	{
		file->printf("\nABS CODES:\n");

		calculate_code_usages();
		for (int r = 0; r < 1 << rbits; r++)
		{
			file->printf("r=%2i:  b=0 b=1 b=2 b=3 b=4 b=5 b=6 ...\n", r);
			for (int g = 0; g < 1 << gbits; g++)
			{
				file->printf("g=%2i: ", g);
				for (int b = 0; b < 1 << bbits; b++)
				{
					bool in_map	  = colors[r][g][b].deviation == 0;
					bool in_image = colors[r][g][b];
					uint n		  = colors[r][g][b].usage;
					if (in_map)
					{
						if (n >= 100) file->printf("{XX}");
						else if (n != 0) file->printf("{%2u}", n);
						else if (in_image) file->printf("{ 0}");
						else file->printf("{--}");
					}
					else
					{
						if (n >= 100) file->printf(" XX ");
						else if (n != 0) file->printf(" %2u ", n);
						else if (in_image) file->printf("  0 ");
						else file->printf(" -- ");
					}
				}
				file->printf("\n");
			}
			file->printf("\n");
		}
	}
}


////////////////////////////////////////////////////////////
////				  EncodedImage						////
////////////////////////////////////////////////////////////


EncodedImage::EncodedImage(RgbImage* image) : //
	rgb_image(image),
	rel_codes(image),
	abs_codes(image, &rel_codes)
{
	encode();
}

EncodedImage::EncodedImage(EncodedImage* old, int min_gap) :
	rgb_image(old->rgb_image),
	rel_codes(old->rel_codes),
	abs_codes(old->abs_codes)
{
	uint min_usage = max(abs_codes.find_lowest_code_usage(), rel_codes.find_lowest_code_usage());
	abs_codes.remove_codes_below_limit(min_usage + 1);
	rel_codes.remove_codes_below_limit(min_usage + 1);

	while (rel_codes.first_code - abs_codes.num_codes < min_gap)
	{
		uint min_abs_usage = abs_codes.find_lowest_code_usage();
		uint min_rel_usage = rel_codes.find_lowest_code_usage();

		if (min_abs_usage <= min_rel_usage) abs_codes.remove_codes_below_limit(min_abs_usage + 1);
		if (min_rel_usage <= min_abs_usage) rel_codes.remove_codes_below_limit(min_rel_usage + 1);
	}

	rel_codes.add_codes((rel_codes.first_code - abs_codes.num_codes) / 2);
	abs_codes.add_codes(rel_codes.first_code - abs_codes.num_codes);

	encode();
}

void EncodedImage::encode()
{
	// encode image using this->abs_codes and this->rel_codes

	width  = rgb_image->width;
	height = rgb_image->height;
	assert(!data);
	data = new uint8[uint(width * height)];

	rgb_image->to_native_depth();
	abs_codes.clear_code_usages();
	abs_codes.fix_colors();
	abs_codes.deviation_map = nullptr;

	rel_codes.clear_code_usages();
	rel_codes.fix_colors();

	assert(abs_codes.num_codes <= rel_codes.first_code);
	total_weighted_deviation = 0;

	for (int i = 0; i < 256; i++)
	{
		ctab[i] = i < abs_codes.num_codes ? Color(abs_codes.codes[i].abs_code) : Color(rel_codes.codes[i].rel_code);
	}

	AbsCode first {.r = 0, .g = 0, .b = 0, .code = 0}; // color of first pixel of previous scanline
	uint8*	q = rgb_image->data;
	uint8*	z = data;

	for (int y = 0; y < height; y++)
	{
		AbsCode current = first;

		for (int x = 0; x < width; x++, z++, q += rgb_image->num_channels)
		{
			// find best abs or rel code to update current to next pixel.
			// update current by applying the code.
			// increment the usage count of the desired code in the used abs_code or rel_code table.

			uint8 new_r = q[0], new_g = q[1], new_b = q[2]; // color of next pixel

			auto& abs_info = abs_codes.get(new_r, new_g, new_b);
			uint  rel_deviation;
			auto& rel_info = rel_codes.get(&rel_deviation, new_r - current.r, new_g - current.g, new_b - current.b);

			if (rel_deviation <= abs_info.deviation)
			{
				total_weighted_deviation += weighted_deviation(rel_deviation);
				current += rel_info.rel_code;
				assert(rel_info.rel_code.code >= rel_codes.first_code);
				assert(rel_info.rel_code == rel_codes.codes[rel_info.rel_code.code].rel_code);
				*z = rel_info.rel_code.code;
				rel_info.add_usage();
				if constexpr (high_deviation_other_boost)
					if (rel_deviation > devi_max) abs_info.add_usage();
			}
			else
			{
				total_weighted_deviation += weighted_deviation(abs_info.deviation);
				current = abs_info.abs_code;
				assert(abs_info.abs_code.code < abs_codes.num_codes);
				assert(abs_info.abs_code == abs_codes.codes[abs_info.abs_code.code].abs_code);
				*z = abs_info.abs_code.code;
				abs_info.add_usage();
				if constexpr (high_deviation_other_boost)
					if (abs_info.deviation > devi_max) rel_info.add_usage();
			}

			if unlikely (x == 0) { first = current; }
		}
	}
}

void EncodedImage::write_to_file(File* file)
{
	static constexpr uint32 magic = 3109478632u;

	file->write_LE(magic);
	file->write("rgb", 4);
	file->write_LE<uint16>(uint16(width));
	file->write_LE<uint16>(uint16(height));
	file->write<uint8>(Color::rbits);
	file->write<uint8>(Color::gbits);
	file->write<uint8>(Color::bbits);
	file->write<uint8>(Color::ibits);
	file->write<uint8>(Color::rshift);
	file->write<uint8>(Color::gshift);
	file->write<uint8>(Color::bshift);
	file->write<uint8>(Color::ishift);
	file->write_LE<uint16>(uint16(abs_codes.num_codes));
	file->write_LE<uint16>(uint16(256 - rel_codes.first_code));
	for (int i = 0; i < 256; i++) file->write_LE(ctab[i]);
	file->write(data, uint(width * height));
}

void EncodedImage::write_to_file(cstr fpath)
{
	StdFile file {fpath, Devices::WRITE};
	write_to_file(&file);
	file.close();
}

double EncodedImage::unweighted_average_deviation() const
{
	return unweighted_deviation(double(total_weighted_deviation) / (width * height));
}
double EncodedImage::unweighted_total_deviation() const { return unweighted_average_deviation() * (width * height); }

void EncodedImage::print_statistics(File* file)
{
	int num_rel_codes = 256 - rel_codes.first_code;
	int num_abs_codes = abs_codes.num_codes;
	file->printf("num codes = %i (abs) + %i (rel) = %i\n", num_abs_codes, num_rel_codes, num_abs_codes + num_rel_codes);
	file->printf("total deviation = %.0f\n", unweighted_total_deviation());
	file->printf("average deviation = %f (%s)\n", unweighted_average_deviation(), rgbstr);
}


////////////////////////////////////////////////////////////
////					RgbImage						////
////////////////////////////////////////////////////////////


RgbImage::RgbImage(cstr fpath, DitherMode dithermode)
{
	// read original image from file

	int num_ch;
	data = stbi_load(fpath, &width, &height, &num_ch, 0);
	if (!data) throw stbi_failure_reason();
	if (num_ch < 3) throw "not an RGB image";
	if (width > RSRC_MAX_IMG_WIDTH || height > RSRC_MAX_IMG_HEIGHT) throw "image too big";

	static_assert(num_channels == 3);
	if (num_ch > num_channels)
	{
		uint8* data2 = new uint8[uint(width * height * 3)];
		uint8 *q = data, *z = data2;
		for (int i = 0; i < width * height; i++, q += num_ch, z += 3) { z[0] = q[0], z[1] = q[1], z[2] = q[2]; }
		delete[] data;
		data = data2;
	}

	is_rgb888 = true;
	reduce_color_depth(dithermode);
	count_colors();
}

RgbImage::RgbImage(EncodedImage* encoded_image)
{
	// decode image

	width	  = encoded_image->width;
	height	  = encoded_image->height;
	data	  = new uint8[uint(width * height * num_channels)];
	is_rgb888 = false;

	uint8* q = encoded_image->data;
	uint8* z = data;

	int			 num_abs_codes = encoded_image->abs_codes.num_codes;
	const Color* ctab		   = encoded_image->ctab;

	Color first = Color(0);
	for (int y = 0; y < height; y++)
	{
		Color current = first;
		for (int x = 0; x < width; x++)
		{
			uint8 i = *q++;
			int	  c = ctab[i].raw;
			current = i < num_abs_codes ? c : current.raw + c;
			*z++	= current.red();
			*z++	= current.green();
			*z++	= current.blue();
			if (x == 0) first = current;
		}
	}

	count_colors();
}

void RgbImage::reduce_color_depth(DitherMode dithermode)
{
	// 0=none, 1=pattern, 2=noise
	// reduce color depth of source image to the range of hardware colors.
	// to simulate more bits we can add noise or a fixed 2x2 pattern.

	assert(is_rgb888);

	if (dithermode == DitherMode::Pattern)
	{
		uint8* p = data;
		for (int y = 0; y < height; y++)
		{
			// row 2N:   2,1
			// row 2N+1: 0,3
			uint pattern = y & 1 ? 0 : 2;

			for (int x = 0; x < width; x++, p += num_channels)
			{
				if (pattern)
				{
					p[0] = uint8(min(p[0] + (pattern << (rshift - 2)), 255u));
					p[1] = uint8(min(p[1] + (pattern << (gshift - 2)), 255u));
					p[2] = uint8(min(p[2] + (pattern << (bshift - 2)), 255u));
				}
				pattern = 3 - pattern; // 0->3->0 or 2->1->2
			}
		}
	}
	else if (dithermode == DitherMode::Diffusion)
	{
		const uint32 seed = uint32(width * height) + *reinterpret_cast<uint32*>(data);
		Xoshiro128	 rng(seed);

		uint8* p = data;
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++, p += num_channels)
			{
				if (x + 1 < width)
				{
					uint rnd = rng.random(0x100u);

					p[3] = uint8(min(p[3] + ((uint8(p[0] << rbits) * 3u + rnd) >> (rbits + 2)), 255u));
					p[4] = uint8(min(p[4] + ((uint8(p[1] << gbits) * 3u + rnd) >> (gbits + 2)), 255u));
					p[5] = uint8(min(p[5] + ((uint8(p[2] << bbits) * 3u + rnd) >> (bbits + 2)), 255u));
				}
			}
		}
	}

	to_native_depth(); // cut off the low bits
}

void RgbImage::to_rgb888()
{
	if (is_rgb888) return;
	is_rgb888 = true;

	uint8* p = data;
	for (int i = 0; i < width * height; i++, p += num_channels)
	{
		p[0] <<= (8 - rbits);
		p[1] <<= (8 - gbits);
		p[2] <<= (8 - bbits);
	}
}

void RgbImage::to_native_depth()
{
	if (!is_rgb888) return;
	is_rgb888 = false;

	uint8* p = data;
	for (int i = 0; i < width * height; i++, p += num_channels)
	{
		p[0] >>= (8 - rbits);
		p[1] >>= (8 - gbits);
		p[2] >>= (8 - bbits);
	}
}

void RgbImage::write_to_file(cstr fpath)
{
	// write image as PNG file

	to_rgb888();
	int result = stbi_write_png(fpath, width, height, num_channels, data, width * num_channels);
	assert(result != 0);
}

void RgbImage::make_diff_image(RgbImage* other_image)
{
	to_rgb888();
	other_image->to_rgb888();

	uint8* q = other_image->data;
	uint8* z = data;
	for (int i = 0; i < width * height; i++, q += num_channels, z += num_channels)
	{
		z[0] = uint8(min(abs(z[0] - q[0]) * 8, 255));
		z[1] = uint8(min(abs(z[1] - q[1]) * 8, 255));
		z[2] = uint8(min(abs(z[2] - q[2]) * 8, 255));
	}
}

void RgbImage::count_colors()
{
	bool map[1 << rbits][1 << gbits][1 << bbits];
	memset(map, 0, sizeof(map));
	to_native_depth();

	uint8* p = data;
	for (int i = 0; i < width * height; i++, p += num_channels)
	{
		uint8 r		 = p[0];
		uint8 g		 = p[1];
		uint8 b		 = p[2];
		map[r][g][b] = true;
	}

	uint count = 0;
	for (int r = 0; r < 1 << rbits; r++)
		for (int g = 0; g < 1 << gbits; g++)
			for (int b = 0; b < 1 << bbits; b++) { count += map[r][g][b]; }

	num_colors = count;
}

void RgbImage::calc_deviation_map(RgbImage* other, uint map[256])
{
	to_native_depth();
	other->to_native_depth();

	memset(map, 0, 256 * sizeof(*map));
	uint8* p = data;
	uint8* q = other->data;

	for (int i = 0; i < width * height; i++, p += num_channels, q += num_channels)
	{
		int r1 = p[0];
		int g1 = p[1];
		int b1 = p[2];
		int r2 = q[0];
		int g2 = q[1];
		int b2 = q[2];

		int d = (abs(r1 - r2) * rweight + abs(g1 - g2) * gweight + abs(b1 - b2) * bweight + average_weight - 1) /
				average_weight;
		assert(d < 256);
		map[d] += 1;
	}
}

void RgbImage::print_deviation_map(File* file, uint map[256])
{
	file->printf("\ncolor deviation map:\n");
	for (uint i = 0; i < 256; i++)
	{
		if (map[i]) file->printf("%4u: %u\n", i, map[i]);
	}
	file->printf("\n");
}

void RgbImage::print_statistics(File* file)
{
	file->printf("image size: %i x %i\n", width, height);
	file->printf("total colors: %u (individual colors after color depth reduction)\n", num_colors);
}


////////////////////////////////////////////////////////////
////				RgbImageCompressor					////
////////////////////////////////////////////////////////////


RgbImageCompressor::RgbImageCompressor() {}
RgbImageCompressor::~RgbImageCompressor() {}

int RgbImageCompressor::encode_image()
{
	RCPtr<EncodedImage> next_image = new EncodedImage(image);
	encoded_image				   = next_image;
	int rounds					   = 1;
	int min_code_gap			   = 32; // codes to delete and add again
	int num_bad_runs			   = 0;	 // runs without improvement

	while (num_bad_runs < max_bad_runs && rounds < 99)
	{
		rounds += 1;
		next_image	 = new EncodedImage(next_image, min_code_gap);
		min_code_gap = max(min_code_gap * 3 / 4, 2);
		if (next_image->total_weighted_deviation < encoded_image->total_weighted_deviation)
		{
			encoded_image = next_image;
			num_bad_runs  = 0;
		}
		else num_bad_runs += 1;
	}

	total_num_images += 1;
	total_num_abs_codes += num_abs_codes();
	total_num_rel_codes += num_rel_codes();
	total_total_deviation += total_deviation();
	total_average_deviation += average_deviation();

	return rounds - num_bad_runs; // the round with the final result
}

uint RgbImageCompressor::encodeImage(cstr infile, File* outfile, int verbose, DitherMode dithermode)
{
	image = new RgbImage(infile, dithermode);
	encode_image();
	encoded_image->write_to_file(outfile);
	if (verbose)
	{
		StdFile console {stdout, Flags::writable};
		image->print_statistics(&console);
		encoded_image->print_statistics(&console);
	}
	return outfile->getSize();
}

void RgbImageCompressor::encodeImage(cstr indir, cstr outdir, cstr infile, int verbose, DitherMode dithermode)
{
	if (!endswith(indir, "/")) indir = catstr(indir, "/");
	if (!endswith(outdir, "/")) outdir = catstr(outdir, "/");

	cstr basename = catstr(outdir, directory_and_basename_from_path(infile), "-", tostr(dithermode));

	FilePtr console = new StdFile(stdout, Flags::writable);
	FilePtr statsfile;
	if (write_stats_file) statsfile = new StdFile(catstr(basename, "-", rgbstr, ".txt"), Devices::WRITE);

	printf("File: %s\n", filenamefrompath(infile));
	if (statsfile)
	{
		statsfile->printf("File: %s\n", filenamefrompath(infile));
		bool f = deviation_linear();
		statsfile->printf("  deviation handling = %s\n", f ? "linear" : "quadratic");
		if (f) statsfile->printf("  - max. deviation   = %i\n", deviation_max());
		if (f) statsfile->printf("  - factor above max = %i\n", deviation_factor());
		statsfile->printf("  high deviation boost = %s\n", high_deviation_other_boost() ? "enabled" : "disabled");
	}

	image = new RgbImage(catstr(indir, infile), dithermode);
	if (verbose) image->print_statistics(console);
	if (statsfile) image->print_statistics(statsfile);
	if (write_ref_image) image->write_to_file(catstr(basename, "-", rgbstr, "-in.png"));

	int rounds = encode_image();
	if (verbose) console->printf("final image in round: %i\n", rounds);
	if (statsfile) statsfile->printf("final image in round: %i\n", rounds);

	if (verbose) encoded_image->print_statistics(console);
	if (statsfile) encoded_image->print_statistics(statsfile);
	encoded_image->write_to_file(catstr(basename, ".", rgbstr));

	if (write_diff_image)
	{
		RgbImage decoded_image(encoded_image);
		decoded_image.write_to_file(catstr(basename, "-", rgbstr, "-out.png"));

		uint deviation_map[256];
		decoded_image.calc_deviation_map(image, deviation_map);
		for (uint i = 0; i < NELEM(total_deviations); i++) total_deviations[i] += deviation_map[i];

		if (verbose >= 2) decoded_image.print_deviation_map(console, deviation_map);
		if (statsfile) decoded_image.print_deviation_map(statsfile, deviation_map);
		decoded_image.make_diff_image(image);
		decoded_image.write_to_file(catstr(basename, "-", rgbstr, "-diff.png"));
	}

	if (verbose >= 3) encoded_image->abs_codes.print_color_map(console);
	if (verbose >= 3) encoded_image->rel_codes.print_color_map(console);
	if (statsfile) encoded_image->abs_codes.print_color_map(statsfile);
	if (statsfile) encoded_image->rel_codes.print_color_map(statsfile);
}

int	   RgbImageCompressor::image_width() { return image->width; }
int	   RgbImageCompressor::image_height() { return image->height; }
uint   RgbImageCompressor::num_colors() { return image->num_colors; }
int	   RgbImageCompressor::num_abs_codes() { return encoded_image->abs_codes.num_codes; }
int	   RgbImageCompressor::num_rel_codes() { return 256 - encoded_image->rel_codes.first_code; }
double RgbImageCompressor::total_deviation() { return encoded_image->unweighted_total_deviation(); }
double RgbImageCompressor::average_deviation() { return encoded_image->unweighted_average_deviation(); }
bool   RgbImageCompressor::deviation_linear() { return devi_linear; }
bool   RgbImageCompressor::deviation_quadratic() { return devi_quadratic; }
int	   RgbImageCompressor::deviation_max() { return devi_max / average_weight; }
int	   RgbImageCompressor::deviation_factor() { return devi_fak; }
bool   RgbImageCompressor::high_deviation_other_boost() { return kio::high_deviation_other_boost; }

} // namespace kio


/*




















































































*/
