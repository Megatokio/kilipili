// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "glue.h"


#if defined KILIPILI_SPINLOCK_NUMBER

static struct OnInit
{
	OnInit()
	{
		spin_lock_claim(KILIPILI_SPINLOCK_NUMBER);
		spin_lock_init(KILIPILI_SPINLOCK_NUMBER);
	}
} init;

#endif


// testing
// TODO make a unit test

#include "RCPtr.h"

namespace kio
{

struct SubClass : public RCObject
{
	void lock() {}
	void unlock() {}
};

void foo()
{
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
}

} // namespace kio
