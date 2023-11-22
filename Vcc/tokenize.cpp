// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Array.h"
#include "Vcc.h"
#include "cstrings.h"
#include "idf_ids.h"
#include <cmath>
#include <limits>


// Veraltet:
//
//	tokenize_line(…) tokenisiert idR. eine Zeile bis Nullbyte oder Newline.
//	tokenize() tokenisiert alle Zeilen bis zum Nullbyte.
//	Der Text muss mit einem Nullbyte abgeschlossen sein. (c-style string)
//	Als Zeilenende wird nur Newline '\n' erkannt.
//	Der Text muss UTF8-kodiert sein oder reines 7-Bit Ascii.
//
//	tokenize() überspringt '//…' Zeilenkommentare und '/*…*/' Blockkommentare.
//	tokenize() tokenisiert u.U. mehr als eine Zeile, wenn ein Blockkommentar oder ein Langstring «…» gefunden wird
//	oder wenn die Zeile mit einem Backslash endet. (Ausnahme: Zeilenkommentar)
//	Langstrings und Blockkommentare könnengeschachtelt werden.
//
//	Macros werden nicht expandiert.
//	Preprozessor-Direktiven werden nicht ausgeführt.
//
//	Die abgetrennten Token werden mit den Methoden des TokenizeInterface übergeben
//		und von diesem z.B. in Worte gewandelt und an einen Array gehängt.
//		Dabei werden Zeiger in den Originaltext übergeben, bei Strings inkl. Quotes und Escape-Zeichen.
//
//	tokenize() wirft TokenizeError, wenn ein Blockkommentar oder ein String nicht terminiert ist.
//		Bei Kurzstrings zeigt die enthaltene pos auf das unerwartete nl oder null,
//		bei Blockkommentar und Langstring auf den Blockstarter.
//
//	tokenize() erkennt
//	• Strings: ".." '..' `..` und «..»
//	• Bezeichner: ['_' <unicodeletter>] ['_' <unicodeletter> 0-9]*
//	• Zahlen:
//		[0-9] [0-9 a-z A-Z]*
//		[0-9]+ '.' [0-9]* [a-z A-Z]*
//		[0-9]+ '.' [0-9]* [eE] [+-]? [0-9]+ [a-z A-Z]*
//	• Operatoren und Sonderzeichen:
//		^!$%&/()=?+*#-.:,;<>@{}[]|
//	• Mehrzeichenoperatoren:
//		++ -- == != >= <= ≤ ≥ << >> && || -> /% := += -= *= /= %= >>= <<= &&= ||=
//
//	Ketten von Sonderzeichen werden von links nach rechts gebrochen, "+++" wird zu "++" plus "+".
//
//	Zahlen werden nicht ausgewertet. Es wird nicht garantiert, dass sie syntaktisch korrekt sind!
//	Vorzeichen werden separat als Operator '+'  bzw. '-' gespeichert.


namespace kio::Vcc
{

static Array<uint16> tokenized_source;

constexpr char long_string_opener = 0xAB; // '«'
constexpr char long_string_closer = 0xBB; // '«'


static uint32 normalize_linebreaks(ptr q) noexcept
{
	// replace non-unix linebreaks with simple '\n'
	// returns number of bytes removed for DOS linebreaks

	static const char nl = '\n';
	static const char cr = '\r';

	ptr z = strchr(q, cr);
	if (z == nullptr) return 0;

	if (z > q) z--; // prev char might be nl
	for (q = z;;)
	{
		char c;
		while (uchar(c = *z++ = *q++) > 13) {}

		if (c == 0) return uint32(q - z);
		if (c == nl || c == cr)
		{
			if (c + *q == cr + nl) q++;
			*(z - 1) = nl;
		}
	}
}

static cptr skip_linecomment(cptr q) noexcept
{
	// skip to end of line
	// stop at endofline or null
	// also works with DOS line ends

	while (*q && *q != '\n') q++;
	return q;
}

// clang-format off
static_assert([]{
	static cstr s1 = "12 \\t4€ Af3'456";
	static cstr s2 = "/2 \\t4€ A+3\n//456'";
	static cstr s3 = "//\t\\t4€ A*3\\\n456'";
	static cstr s4 = "12 \\t4€ //3\r\n//456'";
	assert(skip_linecomment(s1) == s1+17);
	assert(skip_linecomment(s2) == s2+13);
	assert(skip_linecomment(s3) == s3+14);
	assert(skip_linecomment(s4) == s4+14);
	return 1;
});
// clang-format on

static cptr skip_string(cptr q) throws
{
	// skip beyond next delim
	// throw at nl or null
	// note: '\' before line end works with "\r\n" DOS line ends:
	//		 caller must also handle this when unescaping the string!

	char delim = *q++;
	assert(delim == '"' || delim == '`' || delim == '\'');

	if (*q == delim && *(q + 1) == delim) return q + 2; // accept ''' and """ as 1-char strings

	for (char c; (c = *q++) && c != '\n';)
	{
		if (c == delim) return q;
		if (c == '\\')
		{
			if (*q == '\r' && *(q + 1) == '\n') q++;
			if (*q) q++;
		}
	}
	throw "unterminated string literal";
}

// clang-format off
static_assert([]{
	static cstr s1 = "'12 \\t4€ Af3'456";
	static cstr s2 = "'12 \t4€ A+3\n456'";
	static cstr s3 = "'12 \"t\\'€ A*3";
	assert(skip_string(s1) == s1+15);
	try{ skip_string(s2); IERR(); } catch(cstr) {}
	try{ skip_string(s3); IERR(); } catch(cstr) {}
	return 1;
});
// clang-format on

static cptr skip_longstring(cptr q) throws
{
	// skip beyond next '»'
	// skip over nl
	// throw at null
	// skip recursively over '«' .. '»' (must be balanced)
	// note: "\r\n" DOS line ends are preserved
	//		 and caller must handle these when unescaping the string!

	assert(*q == long_string_opener);
	q += 1;

	while (char c = *q++)
	{
		if (c == '\\') q += !!*q;
		else if (c == long_string_closer) return q; // '»'
		else if (c == long_string_opener) q = skip_longstring(q - 1);
	}
	throw "unterminated string literal";
}

// clang-format off
static_assert([]{
	static cstr s1 = "«12 \\t4€ A¢3'456»xx";
	static cstr s2 = "«12 \t4€ A+3\n456'»»";
	static cstr s3 = "«12 \"t4«€» ««»»A*3»\n";
	static cstr s4 = "«12 \"t4«€» A*3\n";
	static cstr s5 = "«12 \"t4«» A*3\"";
	static cstr s6 = "«123\\";
	assert(skip_longstring(s1) == s1+22);
	assert(skip_longstring(s2) == s2+21);
	assert(skip_longstring(s3) == s3+29);
	try{skip_longstring(s4); IERR();}catch(cstr){}
	try{skip_longstring(s5); IERR();}catch(cstr){}
	try{skip_longstring(s6); IERR();}catch(cstr){}
	return 1;
});
// clang-format on

static cptr skip_blockcomment(cptr q) throws
{
	// skip beyond next "*/"
	// skip over nl
	// throw at null

	// nested block comments are detected and skipped. "/*" .. "*/" must be balanced.
	// long strings are detected and skipped. '«' .. '»' must be balanced. spurious '»' are ignored.
	// line comments "//" are detected and skipped.

	// contained short strings should be delimited.
	// '*/' at the end of lines with unbalanced short string delimiters are recognized!
	//		-> natural text with apostrophs should work
	//		while commented-out source is expected to contain only balanced strings.

	assert(*q == '/' && *(q + 1) == '*');

	q += 2;
	while (char c = *q++)
	{
		if (c > '/') continue;

		switch (c)
		{
		case '*':
			if (*q == '/') return q + 1; // closing '*/' found
			continue;

		case '/':
			if (*q == '*') q = skip_blockcomment(q - 1);
			else if (*q == '/') q = skip_linecomment(q);
			continue;

		case '\'':
		case '"':
		case '`':
			try
			{
				q = skip_string(q - 1);
			} // skip string (may contain '*/')
			catch (cstr)
			{} // else just skip unbalanced quote
			continue;

		case long_string_opener: //
			q = skip_longstring(q - 1);
			continue;

		default: continue;
		}
	}

	throw "unterminated block comment";
}

// clang-format off
static_assert([]{
	{static cstr s = "/*123 \t \nxx**//zz";		assert(skip_blockcomment(s) == s+14);}
	{static cstr s = "/*12'*/'; // xyz\n\t*/ ";	assert(skip_blockcomment(s) == s+20);}
	{static cstr s = "/*/*foo*/«/*\n»*/;";		assert(skip_blockcomment(s) == s+18);}
	{static cstr s = "/*\n//foo*/\n//«\n*/x";	assert(skip_blockcomment(s) == s+18);}
	{static cstr s = "/*it's a lie! */\n";		assert(skip_blockcomment(s) == s+16);}
	{static cstr s = "/*foo\n//*/";				try{skip_blockcomment(s);IERR();}catch(cstr){}}
	{static cstr s = "/*foo «*/";				try{skip_blockcomment(s);IERR();}catch(cstr){}}
	{static cstr s = "/*foo \\";				try{skip_blockcomment(s);IERR();}catch(cstr){}}
	return 1;
});
// clang-format om

static cptr skip_spaces (cptr q) noexcept
{
	// skip spaces
	// skip backslash + endofline
	// stop at endofline, nonspace or null
	// works with "\r\n" DOS line ends

	for(char c; (c = *q++); )
	{
		if(uchar(c) <= ' ') { if(c != '\n') continue; else break; }
		if(c == '\\' && *q == '\n') { q++; continue; }
		if(c == '\\' && *q == '\r' && *(q+1) == '\n') { q+=2; continue; }
		if(c == '/'  && *q == '/') { return skip_linecomment(q+1); }
		else break;
	}
	return q-1;
}

// clang-format off
static_assert([]{
	static cstr s1 = "12 \\t4€ Af3";
	static cstr s2 = "\t2";
	static cstr s3 = "  \t\\\n\t \\\r\n \t \\\r\n  \r\nx";
	static cstr s4 = "\\\t 12\nx";
	static cstr s5 = "\t";
	static cstr s6 = "\t// foobar «....x\nxx";
	assert(skip_spaces(s1) == s1);
	assert(skip_spaces(s2) == s2+1);
	assert(skip_spaces(s3) == s3+19);
	assert(skip_spaces(s4) == s4);
	assert(skip_spaces(s5) == s5+1);
	assert(skip_spaces(s6) == s6+18);
	return 1;
});
// clang-format on

static cptr skip_identifier(cptr q) noexcept
{
	// Identifier: ['_' <letter>] ['_' <letter> 0-9]*

	assert(is_letter(*q) || *q == '_');

	while (is_letter(*++q) || *q == '_' || is_decimal_digit(*q)) {}
	return q;
}

// clang-format off
static_assert([]{
	{static cstr s = "L23L+";		assert(skip_identifier(s) == s+4);}
	{static cstr s = "_a65qh.f";	assert(skip_identifier(s) == s+6);}
	{static cstr s = "é1Ä€";		assert(skip_identifier(s) == s+5);}
	{static cstr s = "кирлица+";	assert(skip_identifier(s) == s+14);}
	{static cstr s = "an_0n«»";		assert(skip_identifier(s) == s+5);}
	{static cstr s = "L23L\n";		assert(skip_identifier(s) == s+4);}
	{static cstr s = "L23L";		assert(skip_identifier(s) == s+4);}
return 1;
});
// clang-format on

inline cptr skip_decimals(cptr p) noexcept
{
	while (is_decimal_digit(*p)) { p++; }
	return p;
}

static cptr skip_number(cptr q) noexcept
{
	// Number:
	//	'0x' [0-9,a-f,A-F]+
	//	'0b' [01]+
	// 	[0-9]+
	// 	[0-9]+ '.' [0-9]+
	// 	[0-9]+ '.' [0-9]+ [eE] [+-]? [0-9]+
	// 	[0-9]+            [eE] [+-]? [0-9]+

	if (*q == '0')
	{
		if ((*(q + 1) | 0x20) == 'x' && is_hex_digit(*(q + 2))) // hex number
		{
			q += 3;
			while (is_hex_digit(*q)) { q++; }
			return q;
		}
		if ((*(q + 1) | 0x20) == 'b' && is_bin_digit(*(q + 2))) // bin number
		{
			q += 3;
			while (is_bin_digit(*q)) { q++; }
			return q;
		}
	}

	if (*q == '+' || *q == '-') q++; // skip sign

	assert(is_decimal_digit(*q));
	q = skip_decimals(q + 1); // skip manissa or integer number

	if (*q == '.' && is_decimal_digit(*(q + 1))) // decimal dot: fractional part of floating point number
	{											 // test is_dec_digit() wg. member functions, e.g. 123.lo()
		q = skip_decimals(q + 2);
	}

	if ((*q | 0x20) == 'e') // exponent
	{
		cptr q0 = q++;
		if (*q == '+' || *q == '-') q++;
		if (no_dec_digit(*q)) return q0;
		q = skip_decimals(q);
	}

	if (*q == 's' || *q == 'l') q++; // size specifier

	return q;
}

// clang-format off
static_assert([]{
	{static cstr s = "123L+";		assert(skip_number(s) == s+3);}
	{static cstr s = "0xA23L+";		assert(skip_number(s) == s+5);}
	{static cstr s = "0A23h0";		assert(skip_number(s) == s+1);}
	{static cstr s = "1e65qh.f";	assert(skip_number(s) == s+4);}
	{static cstr s = "12.34e+5s.";	assert(skip_number(s) == s+9);}
	{static cstr s = "123L+";		assert(skip_number(s) == s+3);}
	{static cstr s = "123L\n";		assert(skip_number(s) == s+3);}
	{static cstr s = "123L";		assert(skip_number(s) == s+3);}
	{static cstr s = "12.L";		assert(skip_number(s) == s+2);}
	{static cstr s = "12e34";		assert(skip_number(s) == s+5);}
	{static cstr s = "12.0e";		assert(skip_number(s) == s+4);}
	return 1;
});
// clang-format on

static cptr skip_operator(cptr q) noexcept
{
	// • Operatoren und Sonderzeichen:
	// 	^!$%&/()=?+*#-.:,;<>@{}[]|
	// • Mehrzeichenoperatoren:
	// 	≤ ≥ ++ -- == != >= <= << >> && || -> /% := += -= *= /= %= >>= <<= &&= ||=

	char c1 = *q;
	if (strchr("+-*/%><:=!&|^", c1)) // potential 2- and 3-char operators
	{
		char		c2	= *(q + 1);
		static char o[] = "<<=>>=&&=||=++ -- == != >= <= !! -> /% := += -= *= /= %= &= |= ^= ";

		for (uint i = 0; i < NELEM(o) - 1; i += 3)
		{
			if (c1 == o[i] && c2 == o[i + 1])
			{
				char c3 = *(q + 2);
				return c3 != ' ' && c3 == o[i + 2] ? q + 3 : q + 2; // 2 or 3 char operator
			}
		}
	}

	// single unicode char, operator or special character, maybe ≥ or ≤
	return ++q; // utf8::nextchar(q);
}

// clang-format off
static_assert([]{
	{static cstr s = ">=";		assert(skip_operator(s) == s+2);}
	{static cstr s = ">> ";		assert(skip_operator(s) == s+2);}
	{static cstr s = ">>=";		assert(skip_operator(s) == s+3);}
	{static cstr s = "<>>";		assert(skip_operator(s) == s+1);}
	{static cstr s = "!a";		assert(skip_operator(s) == s+1);}
	{static cstr s = "+++";		assert(skip_operator(s) == s+2);}
	{static cstr s = "+--";		assert(skip_operator(s) == s+1);}
	{static cstr s = "%\n";		assert(skip_operator(s) == s+1);}
	{static cstr s = "%==\n";	assert(skip_operator(s) == s+2);}
//	{static cstr s = "¢";		assert(skip_operator(s) == s+2);}
//	{static cstr s = "≥";		assert(skip_operator(s) == s+3);}
	{static cstr s = "&&=||=";	assert(skip_operator(s) == s+3);}
	{static cstr s = "&&||=";	assert(skip_operator(s) == s+2);}
	{static cstr s = "&||=";	assert(skip_operator(s) == s+1);}
	return 1;
});
// clang-format on


static void append_number(uint32 n)
{
	tokenized_source.append(t_INT);
	tokenized_source.append(uint16(n));
	tokenized_source.append(n >> 16);
}

static void append_number(uint64 n)
{
	tokenized_source.append(t_LONG);
	tokenized_source.append(uint16(n));
	tokenized_source.append(uint16(n >> 16));
	tokenized_source.append(uint16(n >> 32));
	tokenized_source.append(uint16(n >> 48));
}

static void append_number(float n)
{
	tokenized_source.append(t_INT);
	tokenized_source.append(reinterpret_cast<uint16*>(&n)[0]);
	tokenized_source.append(reinterpret_cast<uint16*>(&n)[1]);
}

static void append_string(cstr s)
{
	s = newcopy(s); // for the time being
	tokenized_source.append(t_STR);
	tokenized_source.append(reinterpret_cast<uint16*>(&s)[0]);
	tokenized_source.append(reinterpret_cast<uint16*>(&s)[1]);

	if constexpr (sizeof(ptr) > 4) // for testing
	{
		tokenized_source.append(reinterpret_cast<uint16*>(&s)[2]);
		tokenized_source.append(reinterpret_cast<uint16*>(&s)[3]);
	}
}

static void parse_identifier(cptr a, cptr e)
{
	// identifier ID 'NameID' is retrieved from names[]
	// for names, operators, separators, etc.

	char c	   = *e;
	*ptr(e)	   = 0;
	uint16 idf = names.add(a);
	*ptr(e)	   = c;
	tokenized_source.append(idf);
}

static void parse_base256_number(cptr a, cptr e) throws // cstr
{
	assert(*a == '\''); // base-256 number

	cstr s	 = unescapedstr(substr(a + 1, e - 1));
	int	 len = e - a - 2;

	if (len == 1) // single character
	{
		tokenized_source.append(t_CHAR);
		tokenized_source.append(uchar(*s));
	}
	else // multi char
	{
		if (len < 1) throw "base-256 literal: min. 1 character required";
		if (len > 8) throw "base-256 literal: max. 8 characters allowed";

		if (len <= 4)
		{
			uint32 n = 0;
			for (int i = 0; i < len; i++) n = n * 256 + uchar(*s);
			append_number(n);
		}
		else
		{
			uint64 n = 0;
			for (int i = 0; i < len; i++) n = n * 256 + uchar(*s);
			append_number(n);
		}
	}
}

static void parse_number(cptr a, cptr e) throws // cstr
{
	// parse number literal and return Value object
	//
	//	'0x' [0-9,a-f,A-F]+
	//	'0b' [01]+
	// 	[+-]? [0-9]+
	// 	[+-]? [0-9]+ '.' [0-9]*
	// 	[+-]? [0-9]+ '.' [0-9]* [eE] [+-]? [0-9]+

	char c	= *e;
	*ptr(e) = 0; // stopper, must be undone
	ptr z;
	errno = 0;

	if (*a == '0' && (*(a + 1) | 0x20) == 'x') // 0x1234..
	{
		uint64 value = strtoull(a + 2, &z, 16);
		*ptr(e)		 = c;

		if (e - a <= 2 + 8) append_number(uint32(value));
		else append_number(value);
	}
	else if (*a == '0' && (*(a + 1) | 0x20) == 'b') // 0b0101..
	{
		uint64 value = strtoull(a + 2, &z, 2);
		*ptr(e)		 = c;

		if (e - a <= 2 + 32) append_number(uint32(value));
		else append_number(value);
	}
	else if (skip_decimals(a) == e) // 1234..
	{
		uint64 value = strtoull(a, &z, 10);
		*ptr(e)		 = c;

		if (int32(value) == int64(value)) append_number(uint32(value));
		else append_number(value);
	}
	else if ((*a == '+' || *a == '-') && skip_decimals(a + 1) == e) // ±1234..
	{
		int64 value = strtoll(a, &z, 10);
		*ptr(e)		= c;

		if (int32(value) == int64(value)) append_number(uint32(value));
		else append_number(uint64(value));
	}
	else // float
	{
		float value = std::strtof(a, &z);
		*ptr(e)		= c;

		append_number(value);
	}

	if (errno) throw strerror(errno); // EINVAL, ERANGE
	assert(z == e);
	assert(*e == c);
}

static void parse_string(cptr a, cptr e) throws // cstr
{
	// parse string literal and return Value object
	// cstring is unquoted, unescaped and utf8-encoded
	// DOS linebreaks in long strings are normalized to '\n'

	str s = substr(a + 1, e - 1);
	if (*a == long_string_opener) normalize_linebreaks(s);
	append_string(unescapedstr(s));
}


Array<uint16>&& tokenize(cstr source) throws
{
	// tokenize source[offs++] --> words[]
	// skips:  initial BOM and SHEBANG
	//         white space, line and block comments
	// stores: identifiers, operators etc., numbers and text values in words[]
	//         linebreaks (except if inside string or block comment)
	// stores: errors in errors[]
	// throws: if too many errors

	//Vcc::source = source;
	cptr a, q = source;
	tokenized_source.purge();
	uint		   num_errors = 0;
	uint		   spos;
	constexpr uint max_errors = 10;

	// skip BOM:
	// note: this indicates an utf-8 source, but we interpret the source as latin-1 !
	if (*q == char(0xEF) && *(q + 1) == char(0xBB) && *(q + 2) == char(0xBF)) q += 3;

	// skip SHEBANG:
	if (*q == '#' && *(q + 1) == '!')
	{
		q = strchr(q, '\n');
		if (!q) return std::move(tokenized_source);
	}

	// DOIT:
	while (num_errors <= max_errors)
	{
		try
		{
			for (a = skip_spaces(q); char c = *a; a = skip_spaces(q))
			{
				spos = uint32(a - source);

				if (is_letter(*a) || c == '_')
				{
					q = skip_identifier(a);
					parse_identifier(a, q);
				}
				else if (is_decimal_digit(c))
				{
				num:
					q = skip_number(a);
					if (*q == 's' || *q == 'l') q++;
					parse_number(a, q); // throws
				}
				else if (c == '"')
				{
					q = skip_string(a); // throws
					parse_string(a, q); // throws
				}
				else if (c == '\'')
				{
					q = skip_string(a);			// throws
					parse_base256_number(a, q); // throws
				}
				else if (c == long_string_opener) // '«'
				{
					q = skip_longstring(a); // throws
					parse_string(a, q);
				}
				else if (c == '/' && *(a + 1) == '*')
				{
					q = skip_blockcomment(a); // throws
					continue;
				}
				else if ((c == '+' || c == '-') && is_decimal_digit(*(a + 1)))
				{
					// try to use '+' and '-' as numeric sign:
					//	it is important to handle numeric signs preceding unsigned values
					//	which grow in size when they become signed
					//	e.g. 40000 = uint16  -->  +40000 = int32
					//	because Value::operator+(int) will not grow size beyond size of dflt. int
					//	but truncate the value instead, to mimic what compiled code does.

					// test for number sign:
					// nicht nach nl			 ((ill. for both sign and oper))
					// nicht nach ++ --
					// nach operator
					// nach ({[,;~!?:
					// nicht nach string, number or other idf
					// nicht nach )}]		note: ')' könnte cast sein

					uint i = tokenized_source.count();
					assert(i != 0);
					uint16 idf = tokenized_source[i - 1];
					// do { idf = tokenized_source[--i]; } while (idf == tNL);

					if (idf <= t_STR) goto op;				   // after literal number or string
					if (idf == tINCR || idf == tDECR) goto op; // must be postfixes: ++ival is not possible
					if (idf <= tEKauf) goto num;			   // after operator and after ( { [
					//if(idf==tRKauf || idf==tGKauf || idf==tEKauf) goto num;
					if (idf == tKOMMA /* || idf==tSEMIK */) goto num;
					goto op;
				}
				else
				{
				op:
					q = skip_operator(a);
					parse_identifier(a, q);
				}
			}
			return std::move(tokenized_source); // ok
		}
		catch (cstr msg)
		{
			// TODO row,col
			// TODO print to Device or File
			printf("%s at spos %u\n", msg, spos);
		}
	}

	// too many errors:
	throw "too many errors";
}


} // namespace kio::Vcc


/*
































*/
