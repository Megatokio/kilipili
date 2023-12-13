#pragma once
// Copyright (c) 1995 - 2022 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "cdefs.h"
#include "standard_types.h"
#include "tempmem.h"
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <type_traits>


namespace kio
{

template<typename T>
class Array;

inline constexpr bool is_space(char c) noexcept;
inline constexpr bool is_letter(char c) noexcept;
inline constexpr bool is_control(char c) noexcept;
inline constexpr bool is_printable(char c) noexcept;
inline constexpr bool is_ascii(char c) noexcept;
inline constexpr bool is_utf8_fup(char c) noexcept; // prefer utf8::is_fup(c) if included
inline constexpr bool is_uppercase(char c) noexcept;
inline constexpr bool is_lowercase(char c) noexcept;
inline constexpr char to_upper(char c) noexcept;
inline constexpr char to_lower(char c) noexcept;

inline constexpr bool is_bin_digit(char c) noexcept;
inline constexpr bool is_oct_digit(char c) noexcept;
inline constexpr bool is_decimal_digit(int c) noexcept;
inline constexpr bool is_hex_digit(char c) noexcept;

inline constexpr bool no_bin_digit(char c) noexcept;
inline constexpr bool no_oct_digit(char c) noexcept;
inline constexpr bool no_dec_digit(char c) noexcept;
inline constexpr bool no_hex_digit(char c) noexcept;

inline constexpr uint digit_val(char c) noexcept __attribute__((deprecated));	// --> dec_digit_value()
inline constexpr uint digit_value(char c) noexcept __attribute__((deprecated)); // --> hex_digit_value()

inline constexpr uint dec_digit_value(char c) noexcept; // char -> digit value: non-digits ≥ 10
inline constexpr uint hex_digit_value(char c) noexcept; // non-digits ≥ 36
inline constexpr char hexchar(int n) noexcept;			// masked legal


// ---- queries ----
extern uint strlen(cstr s) noexcept;
extern bool lt(cstr, cstr) noexcept;
extern bool gt(cstr, cstr) noexcept;
extern bool lcgt(cstr, cstr) noexcept;
extern bool eq(cstr, cstr) noexcept;
extern bool ne(cstr, cstr) noexcept;
inline bool le(cstr a, cstr b) noexcept { return !gt(a, b); }
inline bool ge(cstr a, cstr b) noexcept { return !lt(a, b); }
extern bool lceq(cstr s, cstr t) noexcept;
extern bool fnmatch(cstr pattern, cstr path, bool casefold) noexcept;

extern cptr find(cstr target, cstr search) noexcept;
inline ptr	find(str target, cstr search) noexcept;
inline ptr	find(str target, char c) noexcept { return strchr(target, c); }
inline cptr find(cstr target, char c) noexcept { return strchr(target, c); }
extern cptr lcfind(cstr target, char c) noexcept;
extern cptr rfind(cstr target, cstr search) noexcept;
inline ptr	rfind(str target, cstr search) noexcept;
extern cptr rfind(cstr start, cstr end, char c) noexcept;
inline cptr rfind(cstr target, char c) noexcept;
extern bool startswith(cstr, cstr) noexcept;
extern bool endswith(cstr, cstr) noexcept;
inline bool contains(cstr z, cstr s) noexcept { return find(z, s); }
extern bool isupperstr(cstr) noexcept;
extern bool islowerstr(cstr) noexcept;


// ----	allocate with new[] ----
extern str newstr(uint n); // tempmem.h: allocate memory with new[]
extern str newcopy(cstr);  // tempmem.h: allocate memory with new[] and copy text


// ---- allocate in TempMemPool ----
extern str	tempstr(uint n);
inline str	tempstr(int size);
inline str	tempstr(ulong size);
inline str	tempstr(long size);
extern str	spacestr(int n, char c = ' ');
extern cstr spaces(uint n);
extern str	whitestr(cstr, char c = ' ');

extern str	substr(cptr a, cptr e);
inline str	substr(cuptr a, cuptr e) { return substr(cptr(a), cptr(e)); } // convenience method
extern str	mulstr(cstr, uint n) throws;								  // std::length_error
extern str	catstr(cstr, cstr);
extern str	catstr(cstr, cstr, cstr, cstr = nullptr, cstr = nullptr, cstr = nullptr);
extern str	catstr(cstr, cstr, cstr, cstr, cstr, cstr, cstr, cstr = nullptr, cstr = nullptr, cstr = nullptr);
extern str	midstr(cstr, int a, int n);
extern str	midstr(cstr, int a);
extern str	leftstr(cstr, int n);
extern str	rightstr(cstr, int n);
inline char lastchar(cstr s) noexcept;

inline void toupper(str s) noexcept;
inline void tolower(str s) noexcept;
extern str	upperstr(cstr);
extern str	lowerstr(cstr);
extern str	replacedstr(cstr, char oldchar, char newchar);
extern cstr replacedstr(cstr, cstr oldtext, cstr newtext);
extern str	quotedstr(cstr);
extern str	unquotedstr(cstr); // sets errno
extern str	escapedstr(cstr);
extern str	unescapedstr(cstr); // sets errno
extern str	tohtmlstr(cstr);
extern cstr fromhtmlstr(cstr); // may return original string
extern str	toutf8str(cstr);
extern str	fromutf8str(cstr); // ucs1, sets errno
extern str	unhexstr(cstr);	   // may return nullptr
extern str	base64str(cstr);
extern str	unbase64str(cstr);		   // may return nullptr
extern cstr croppedstr(cstr);		   // may return (substring of) original string
extern cstr detabstr(cstr, uint tabs); // may return original string

extern str usingstr(cstr fmt, va_list) __printflike(1, 0);
extern str usingstr(cstr fmt, ...) __printflike(1, 2);

extern str binstr(uint32 n, cstr b0 = "00000000", cstr b1 = "11111111");
extern str binstr(uint64 n, cstr b0 = "00000000", cstr b1 = "11111111");
extern str hexstr(uint32 n, uint len);
extern str hexstr(uint64 n, uint len);

// this is a PITA:
#define str_if_Tle4 \
  typename std::enable_if<(std::is_integral<T>::value || std::is_enum<T>::value) && sizeof(T) <= 4, str>::type
#define str_if_Tgt4 \
  typename std::enable_if<(std::is_integral<T>::value || std::is_enum<T>::value) && 4 < sizeof(T), str>::type

template<typename T>
inline str_if_Tle4 binstr(T n, cstr b0 = "00000000", cstr b1 = "11111111");
template<typename T>
inline str_if_Tgt4 binstr(T n, cstr b0 = "00000000", cstr b1 = "11111111");

template<typename T>
inline str_if_Tle4 hexstr(T n, uint len);
template<typename T>
inline str_if_Tgt4 hexstr(T n, uint len);

extern str hexstr(cptr, uint len);
inline str hexstr(cstr s); // must not contain nullbyte

//template<class T> str hexstr (T* p, uint cnt) throws AMBIGUITY: reinterpret vs. static cast!
//template<class T> str hexstr (T n, uint len)  throws AMBIGUITY: reinterpret vs. static cast!

template<class T>
inline str numstr(T n);

static constexpr char str36[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

extern str numstr(uint32 n, uint base, cstr digits = str36);
extern str numstr(uint64 n, uint base, cstr digits = str36);
template<typename T>
inline str_if_Tle4 numstr(T n, uint base, cstr digits = str36);
template<typename T>
inline str_if_Tgt4 numstr(T n, uint base, cstr digits = str36);

extern str charstr(char);
extern str charstr(char, char);
extern str charstr(char, char, char);
extern str charstr(char, char, char, char);
extern str charstr(char, char, char, char, char);

extern str	  datestr(time_t secs); // returned string is in local time
inline str	  datestr(double secs) { return datestr(time_t(secs)); }
extern str	  timestr(time_t secs); // returned string is in local time
inline str	  timestr(double secs) { return timestr(time_t(secs)); }
inline str	  timestr(float secs) { return timestr(time_t(secs)); }
extern str	  datetimestr(time_t secs); // returned string is in local time
inline str	  datetimestr(double secs) { return datetimestr(time_t(secs)); }
extern time_t dateval(cstr localtimestr) noexcept;

extern str durationstr(time_t secs);
extern str durationstr(double secs);
inline str durationstr(float secs) { return durationstr(double(secs)); }
template<typename T>
inline str durationstr(T secs);

// NOTE: _split() reuses the source buffer and overwrites line delimiters with 0, evtl. overwriting char at ptr e!
extern void _split(Array<str>& z, ptr a, ptr e) throws;			// split at line breaks
extern void _split(Array<str>& z, ptr a, ptr e, char c) throws; // split at char
inline void _split(Array<cstr>& z, ptr a, ptr e) throws;
inline void _split(Array<cstr>& z, ptr a, ptr e, char c) throws;

extern void split(Array<str>& z, cptr a, cptr e) throws;		 // split at line breaks
extern void split(Array<str>& z, cptr a, cptr e, char c) throws; // split at char
extern void split(Array<str>& z, cstr s) throws;				 // split c-string at line breaks
extern void split(Array<str>& z, cstr s, char c) throws;		 // split c-string at char

inline void split(Array<cstr>& z, cptr a, cptr e) throws;
inline void split(Array<cstr>& z, cptr a, cptr e, char c) throws;
inline void split(Array<cstr>& z, cstr s) throws;
inline void split(Array<cstr>& z, cstr s, char c) throws;

using ::strcat;
using ::strcpy;
uint strcpy(ptr z, cptr q, uint buffersize) noexcept;
uint strcat(ptr z, cptr q, uint buffersize) noexcept;

str join(const Array<cstr>& q) throws;
str join(const Array<cstr>& q, char, bool final = false) throws;
str join(const Array<cstr>& q, cstr, bool final = false) throws;

cstr filename_from_path(cstr path) noexcept;	   // "…/name.ext" --> "name.ext"	"…/" -> ""
cstr extension_from_path(cstr path) noexcept;	   // "….ext"		--> ".ext"		"…"  -> ""
cstr basename_from_path(cstr path);				   // "…/name.ext"	--> "name"
cstr directory_from_path(cstr path);			   // "path/…"		--> "path/"		"…"	 -> "./"
cstr parent_directory_from_path(cstr path);		   // "path/name/" -> "path/";  "path/name" -> "path/"; "…" -> "./"
cstr last_component_from_path(cstr path) noexcept; // "…/name.ext"	--> "name.ext"	"…/dir/" -> "dir/"

} // namespace kio


inline cstr tostr(bool f) noexcept { return f ? "true" : "false"; }
inline str	tostr(float n) { return kio::usingstr("%.10g", double(n)); }
inline str	tostr(double n) { return kio::usingstr("%.14g", n); }
inline str	tostr(long double n) { return kio::usingstr("%.22Lg", n); }
inline str	tostr(int n) { return kio::usingstr("%i", n); }
inline str	tostr(unsigned int n) { return kio::usingstr("%u", n); }
inline str	tostr(long n) { return kio::usingstr("%li", n); }
inline str	tostr(unsigned long n) { return kio::usingstr("%lu", n); }
inline str	tostr(long long n) { return kio::usingstr("%lli", n); }
inline str	tostr(unsigned long long n) { return kio::usingstr("%llu", n); }
inline cstr tostr(cstr s) { return s ? kio::quotedstr(s) : "nullptr"; }


//
//
// **************** Implementations *************************
//
//


namespace kio
{
inline constexpr bool is_space(char c) noexcept { return uchar(c) <= ' ' && c != 0; }
inline constexpr bool is_letter(char c) noexcept { return uchar((c | 0x20) - 'a') <= 'z' - 'a'; }
inline constexpr bool is_control(char c) noexcept { return uchar(c) < 0x20 || uchar(c) == 0x7f; }
inline constexpr bool is_printable(char c) noexcept { return (c & 0x7f) >= 0x20 && uchar(c) != 0x7f; }
inline constexpr bool is_ascii(char c) noexcept { return uchar(c) <= 0x7F; }
inline constexpr bool is_utf8_fup(char c) noexcept
{
	return schar(c) < schar(0xc0);
} // prefer utf8::is_fup(c) if included
inline constexpr bool is_uppercase(char c) noexcept { return uchar(c - 'A') <= 'Z' - 'A'; }
inline constexpr bool is_lowercase(char c) noexcept { return uchar(c - 'a') <= 'z' - 'a'; }
inline constexpr char to_upper(char c) noexcept { return uchar(c - 'a') <= 'z' - 'a' ? c & ~0x20 : c; }
inline constexpr char to_lower(char c) noexcept { return uchar(c - 'A') <= 'Z' - 'A' ? c | 0x20 : c; }

inline constexpr bool is_bin_digit(char c) noexcept { return uchar(c - '0') <= '1' - '0'; } // { return (c|1)=='1'; }
inline constexpr bool is_oct_digit(char c) noexcept { return uchar(c - '0') <= '7' - '0'; } // { return (c|7)=='7'; }
inline constexpr bool is_decimal_digit(int c) noexcept { return uchar(c - '0') <= '9' - '0'; }
inline constexpr bool is_hex_digit(char c) noexcept
{
	return uchar(c - '0') <= '9' - '0' || uchar((c | 0x20) - 'a') <= 'f' - 'a';
}

inline constexpr bool no_bin_digit(char c) noexcept { return uchar(c - '0') > '1' - '0'; } // { return (c|1)=='1'; }
inline constexpr bool no_oct_digit(char c) noexcept { return uchar(c - '0') > '7' - '0'; } // { return (c|7)=='7'; }
inline constexpr bool no_dec_digit(char c) noexcept { return uchar(c - '0') > '9' - '0'; }
inline constexpr bool no_hex_digit(char c) noexcept
{
	return uchar(c - '0') > '9' - '0' && uchar((c | 0x20) - 'a') > 'f' - 'a';
}

inline constexpr uint digit_val(char c) noexcept __attribute__((deprecated));	// --> dec_digit_value()
inline constexpr uint digit_value(char c) noexcept __attribute__((deprecated)); // --> hex_digit_value()

inline constexpr uint dec_digit_value(char c) noexcept
{
	return uchar(c - '0');
} // char -> digit value: non-digits ≥ 10
inline constexpr uint hex_digit_value(char c) noexcept
{
	return c <= '9' ? uchar(c - '0') : uchar((c | 0x20) - 'a') + 10;
} // non-digits ≥ 36
inline constexpr char hexchar(int n) noexcept
{
	return char(((n & 15) >= 10 ? 'A' - 10 : '0') + (n & 15));
} // masked legal


inline ptr	find(str target, cstr search) noexcept { return ptr(find(cstr(target), search)); }
inline ptr	rfind(str target, cstr search) noexcept { return ptr(rfind(cstr(target), search)); }
inline cptr rfind(cstr target, char c) noexcept { return target ? rfind(target, strchr(target, 0), c) : target; }

inline str tempstr(int size)
{
	assert(size >= 0);
	return tempstr(uint(size));
}
inline str tempstr(ulong size)
{
	assert(size == uint(size));
	return tempstr(uint(size));
}
inline str tempstr(long size)
{
	assert(size >= 0);
	return tempstr(ulong(size));
}

inline char lastchar(cstr s) noexcept { return s && *s ? s[::strlen(s) - 1] : 0; }

inline void toupper(str s) noexcept
{
	if (s)
		for (; *s; s++) *s = to_upper(*s);
}
inline void tolower(str s) noexcept
{
	if (s)
		for (; *s; s++) *s = to_lower(*s);
}

template<typename T>
inline str_if_Tle4 hexstr(T n, uint len)
{
	return hexstr(uint32(n), len);
}

template<typename T>
inline str_if_Tgt4 hexstr(T n, uint len)
{
	return hexstr(uint64(n), len);
}

template<typename T>
inline str_if_Tle4 binstr(T n, cstr b0, cstr b1)
{
	return binstr(uint32(n), b0, b1);
}
template<typename T>
inline str_if_Tgt4 binstr(T n, cstr b0, cstr b1)
{
	return binstr(uint64(n), b0, b1);
}

inline str hexstr(cstr s) { return hexstr(s, strlen(s)); }

template<class T>
inline str numstr(T n) throws
{
	return tostr(n);
}

template<typename T>
inline str_if_Tle4 numstr(T n, uint base, cstr digits)
{
	return numstr(uint32(n), base, digits);
}
template<typename T>
inline str_if_Tgt4 numstr(T n, uint base, cstr digits)
{
	return numstr(uint64(n), base, digits);
}

template<typename T>
inline str durationstr(T secs)
{
	return durationstr(time_t(secs));
}

inline void _split(Array<cstr>& z, ptr a, ptr e) throws { _split(reinterpret_cast<Array<str>&>(z), a, e); }
inline void _split(Array<cstr>& z, ptr a, ptr e, char c) throws { _split(reinterpret_cast<Array<str>&>(z), a, e, c); }

inline void split(Array<cstr>& z, cptr a, cptr e) throws { split(reinterpret_cast<Array<str>&>(z), a, e); }
inline void split(Array<cstr>& z, cptr a, cptr e, char c) throws { split(reinterpret_cast<Array<str>&>(z), a, e, c); }
inline void split(Array<cstr>& z, cstr s) throws { split(reinterpret_cast<Array<str>&>(z), s); }
inline void split(Array<cstr>& z, cstr s, char c) throws { split(reinterpret_cast<Array<str>&>(z), s, c); }

} // namespace kio

/*



























*/
