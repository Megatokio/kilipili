// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Array.h"
#include "Type.h"
#include "no_copy_move.h"
#include "standard_types.h"
#include "string.h"

namespace kio::Vcc
{

// proc info:

enum SigID : uint16 { SigNotFound = 0xffffu };

struct Signature
{
	NO_COPY(Signature);

	Type  rtype = VOID;
	Type* args	= nullptr;
	uint  argc	= 0;

	Signature() = default;
	Signature(Signature&& q) : args(q.args), argc(q.argc) { q.args = nullptr; }
	Signature& operator=(Signature&&) = delete;

	Signature(Type rt, const Type* args, uint cnt) : Signature(rt, cnt)
	{
		memcpy(this->args, args, cnt * sizeof(*args));
	}

	Signature(Type rt, Type* args_wzero) : rtype(rt)
	{
		while (args_wzero[argc]) argc++;
		args = new Type[argc];
		memcpy(args, args_wzero, (argc + 1) * sizeof(*args_wzero));
	}

	Signature(Type rt, Type arg1) : Signature(rt, 1) { args[0] = arg1; }
	Signature(Type rt, Type arg1, Type arg2) : Signature(rt, 2)
	{
		args[0] = arg1;
		args[1] = arg2;
	}

	~Signature() { delete[] args; }
	Signature clone() const { return Signature(rtype, args, argc); }

	bool operator==(const Signature& q) const
	{
		if (rtype != q.rtype) return false;
		if (argc != q.argc) return false;
		for (uint i = 0; i < argc; i++)
			if (args[i] != q.args[i]) return false;
		return true;
	}

private:
	Signature(Type rt, uint cnt) : rtype(rt), argc(cnt) { args = new Type[cnt]; }
};


struct Signatures : public Array<Signature>
{
	using super = Array<Signature>;

	Signatures() { grow(0, 40); }

	SigID add(Signature&& s)
	{
		SigID id = find(s);
		if (id != SigNotFound) return id;
		super::append(std::move(s));
		return SigID(count() - 1);
	}
	SigID add(const Signature& s)
	{
		SigID id = find(s);
		if (id != SigNotFound) return id;
		super::append(s.clone());
		return SigID(count() - 1);
	}

	const Signature& operator[](SigID id) const { return super::operator[](id); }
	SigID			 operator[](const Signature& s) const { return SigID(indexof(s)); }
	SigID			 find(const Signature& s) { return SigID(indexof(s)); }
};


} // namespace kio::Vcc
