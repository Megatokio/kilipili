// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include <hardware/sync.h>

/*
	BucketList
	==========

	The BucketList is a queue for sending data to another thread, another core or an
	interrupt service routine, or for sending data in both directions.

	Application:
	- Sending scanlines to the video output or send/receive data from audio hardware.
	- Data exchange between cores.

   @tparam T    Bucket data type, must have a default constructor.
   @tparam SZ   Queue size, must be a power of 2.
				2 buckets are minimum for uni-directional data flow if the sender is fast enough.
				4 buckets are minimum for bi-directional data transfer.

	The BucketList is lock-free, assuming there is only one peer working on each side.
	The BucketList does not provide the synchronization needed if either side can be accessed by
	multiple threads/cores/interrupts simultaneously. Then locking must be provided by the caller.

	The buckets in the list may need to be initialized before the list can be used.
	Buckets are never actually removed from the list, as the buckets are *in* the list (no pointers).

	Data reads must test whether a bucket is available.
	Data writes (returning a formerly read bucket) never stall.
		They are always possible any time by this design.
	Buckets can only be returned in the same order as read.


	HowTo:
	------

	One side is called the low_side, one side is called the high_side.
	The low_side is assumed to be the controlling side e.g. if the queue is reset.
	Therefore in a queue to and from an __isr the application is the low_side
	and the __isr is high_side.

	The BucketList is instantiated with the uphill queue empty and the downhill queue *full* !
	If the downhill queue just conveys empty buckets, then this is no problem, but otherwise
	the caller must be aware of this and clear them before releasing the peer.

	get() does not advance the queue, it is just the peer who read the bucket
		who knows that it now possesses the bucket and must return it.
		Thus get() does not provide leasing more than one bucket.

	push() will actually advance the queue.

	get(idx) get's more buckets in advance, but they still must be push()ed in sequence.
		get(idx) is only needed if you want to prepare data in parallel on multiple cores/threads.


	Example: Scanvideo
	------------------

	video controller:
	- wait until ls_avail()
	- ls_get()
	- fill in scanline data
	- ls_push()

	scanline __isr():
	- test hs_avail()
	- hs_get()	 or use empty scanline
	- send scanline data
	- hs_push()	 if it wasn't the empty scanline
*/


namespace kipili
{

template<typename T, uint SZ, typename UINT = uint>
class BucketList
{
public:
	static constexpr UINT SIZE = SZ;
	static constexpr UINT MASK = SIZE - 1;
	static_assert(SIZE >= 2 && (SIZE & MASK) == 0, "SIZE must be 2^N");

	T	 buckets[SIZE]; // T must have default constructor
	UINT lsi = 0;		// low_side  r/w index  ("the program")
	UINT hsi = 0;		// high_side r/w index  ("the __isr")

	BucketList() = default;


	// Are there buckets available for the high_side?
	// note: if the low_side calls hs_avail() it might want to discount one bucket
	//		 which the high_side probably currently works on.
	UINT hs_avail() const noexcept
	{
		__dmb();
		return lsi - hsi;
	}

	// Get next bucket on the high_side:
	// note: this actually doesn't advance the queue.
	//		 To advance the queue you must push the previously read bucket.
	T& hs_get() noexcept
	{
		assert(hs_avail());
		return buckets[hsi & MASK];
	}
	T& hs_get(int i) noexcept
	{
		assert(hs_avail() > i);
		return buckets[(hsi + i) & MASK];
	}

	// Write (push back) bucket on the high_side:
	void hs_push() noexcept { hsi++; /*__sev();*/ }
	void hs_push(const T& t) noexcept
	{
		assert(&t == &buckets[hsi & MASK]);
		hs_push();
	}


	// Are there buckets available for the low_side?
	// note: if the high_side calls ls_avail() it might want to discount one bucket
	//		 which the low_side probably currently works on.
	UINT ls_avail() const noexcept { return SIZE - hs_avail(); }

	// Get next bucket on the low_side:
	// note: this actually doesn't advance the queue.
	//		 To advance the queue you must push the previously read bucket.
	T& ls_get() noexcept
	{
		assert(ls_avail());
		return buckets[lsi & MASK];
	}
	T& ls_get(uint i) noexcept
	{
		assert(ls_avail() > i);
		return buckets[(lsi + i) & MASK];
	}

	// Write (push back) bucket on the low_side:
	void ls_push() noexcept { lsi++; /*__sev();*/ }
	void ls_push(const T& t) noexcept
	{
		assert(&t == &buckets[lsi & MASK]);
		ls_push();
	}


	//	// call only on low_side while high_side is suspended:
	//	void ls_clear_uphill()   noexcept { lsi = hsi; }
	//	void ls_clear_downhill() noexcept { lsi = hsi - SIZE; }
	//
	//	// call only on low_side while high_side is running:
	//	void ls_drain_uphill()   noexcept { while(hs_avail()) __wfe(); }
	//	void ls_drain_downhill() noexcept { while(ls_avail()) ls_push(); }
};

} // namespace kipili
