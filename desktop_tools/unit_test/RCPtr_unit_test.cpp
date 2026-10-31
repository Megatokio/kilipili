// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "RCPtr.h"
#include "doctest.h"

namespace kilipili
{

TEST_CASE("QspiFlash: erase sectors")
{
	struct SubClass : public RCObject
	{
		int	 _padding;
		void lock() {}
		void unlock() {}
	};

	RCObject*		   o  = new RCObject;
	SubClass*		   s  = new SubClass;
	volatile SubClass* vs = s;

	RCPtr crp(o);
	RCPtr crps(s);
	RCPtr vrcps(vs);

	NVPtr nvp4(s);
	NVPtr nvp(vs);

	MTPtr mtp2(o);
	MTPtr mtps2(s);
	MTPtr mtps3(vs);


	MTPtr mtp(crp);
	MTPtr mtps(crps);

	crp = o;
	crp = crps;
	mtp = s;
	mtp = mtps;

	MTPtr vmtps(vrcps);

	NVPtr nvp2(vrcps);
	NVPtr nvp3(vmtps);

	CHECK_FALSE(0);
}

} // namespace kilipili
