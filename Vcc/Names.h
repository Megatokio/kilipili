// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Array.h"
#include "algorithms.h"
#include "idf_ids.h"
#include "standard_types.h"
#include "tempmem.h"

namespace kio::Vcc
{

struct Names : public HashMap<cstr, IdfID>
{
	using super = HashMap<cstr, IdfID>;

	static constexpr cstr idfs[] = {
#define M(ID, STR) STR
#include "idf_id.h"
	};

	Names() {}
	~Names();

	Array<cstr> added_names;

	void purge()
	{
		super::purge();
		for (uint i = 0; i < added_names.count(); i++) delete[] added_names[i];
		added_names.purge();
	}

	void init()
	{
		purge();
		super::grow(512);

		for (uint i = 0; i < NELEM(idfs); i++) { _add(idfs[i]); }
		assert(operator[]("dup") == tDUP);
	}

	IdfID add(cstr s)
	{
		IdfID idf = super::get(s, IdfID(super::count()));
		if (idf == super::count())
		{
			s = newcopy(s);
			added_names.append(s);
			super::add(s, idf);
		}
		return idf;
	}

	using super::operator[];
	cstr		 operator[](IdfID id) { return id < NELEM(idfs) ? idfs[id] : added_names[id - NELEM(idfs)]; }

private:
	void _add(cstr s)
	{
		IdfID n = IdfID(count());
		assert(n == count()); // no doublettes!
		super::add(s, n);
	}
};


} // namespace kio::Vcc
