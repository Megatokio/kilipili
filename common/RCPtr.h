// Copyright (c) 2015 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "cdefs.h"
#include "glue.h"
#include "no_copy_move.h"
#include <type_traits>
#include <utility>

/*	template class RCPtr<> is a reference counting smart pointer.
	Management of the objects' reference count is thread and interrupt save.
	Access to the RCPtr itself is not thread safe.

	template class MTPtr<> is a RCPtr which itself is thread safe.
	Use this if the pointer itself can be modified while it is accessed by other threads.

	template class NVPtr<> is a RCPtr which locks a volatile object.
	It is used temporarily to access objects which have the attribute `volatile`
	to mark that they can change state not under control of the current thread.
	You can make your program thread safe by storing all objects which are accessible
	by multiple threads in RCPtr<volatile T> and forcing yourself to store the RCPtr into a
	NVPtr before accessing them. (you may mark uncritical member functions as `volatile` then.)
	
	template class RCObject<> is a base class for objects managed by a RCPtr.
	class RCObject is virtual.

	You don't need to use base class RCObject, you can also provide the
	reference count `rc` in your class which may be any variant of `int`.
	Your class does not need to be virtual, but if it has subclasses it better is.

	The RCPtr is made thread safe by using a spinlock.
	The rc is not a std::atomic<int> because the cortex M0+ has no atomic incr and decr
	and implements atomic incr and decr with a spinlock anyway. Additionally the MTPtr
	needs a mutex (a std::atomic<T*> is not enough) so the MTPtr needs a spinlock anyway too.
	To reduce the number of lock_spinlock() calls in any MTPtr member function we use the
	MTPtr's spinlock for both. This spills over to the RCPtr which now has to use it too.
*/

namespace kio
{

// It is an error to pass a RCPtr<subclass> in assignments if T has no virtual destructor
// because if the RCPtr deletes the object it will then not call the full destructor.
// But we don't check this because it is too painful: somehow classes with RCPtr<T> members
// cannot be defined without the compiler having seen the actual class definition of T
// which means we could no longer just declare a class in a header file.
//
// NOTE: std::is_base_of_v<T, T2> also covers the base class itself!
// NOTE: but they are not recognized as standard copy constructor etc. ... :-(
//
#define subclass_only typename = std::enable_if_t<std::is_base_of_v<T, T2>>
//#define subclass_only typename = std::enable_if_t < std::is_base_of_v<T, T2> && std::has_virtual_destructor_v < T >>
#define baseclass_only typename = std::enable_if_t<std::is_base_of_v<T2, T>>


// _____________________________________________________________________
//
class RCObject
{
public:
	alignas(4) mutable int16_t rc = 0;
	uint16_t rc_magic			  = 37465;
	virtual ~RCObject() noexcept
	{
		assert(rc == 0);
		assert(rc_magic == 37465);
		if constexpr (debug) rc_magic = 0;
	}
};


// _____________________________________________________________________
//
template<class T>
class RCPtr
{
	template<class TT>
	friend class RCArray;
	template<class T1, class T2>
	friend class RCHashMap;
	template<class TT>
	friend class MTPtr;
	template<class TT>
	friend class NVPtr;
	template<typename T2>
	friend class kio::RCPtr;

	using nvptr = typename std::remove_volatile_t<T>*;

	T* p;

private:
	static void retain(T* p) noexcept
	{
		if (!p) return;
		kilipili_lock_spinlock();
		++const_cast<nvptr>(p)->rc;
		kilipili_unlock_spinlock();
	}
	static void release(T* p) noexcept
	{
		if (!p) return;
		kilipili_lock_spinlock();
		auto n = --const_cast<nvptr>(p)->rc;
		kilipili_unlock_spinlock();
		if (n == 0) delete p;
	}
	static void retain_release(T* q, T* p) noexcept
	{
		kilipili_lock_spinlock();
		if (q) ++const_cast<nvptr>(q)->rc;
		bool f = p && --const_cast<nvptr>(p)->rc == 0;
		kilipili_unlock_spinlock();
		if (f) delete p;
	}

public:
	RCPtr() noexcept : p(nullptr) {}
	RCPtr(std::nullptr_t) noexcept : p(nullptr) {}
	RCPtr(T* q) noexcept : p(q) { retain(p); }
	~RCPtr() noexcept { release(p); }

	RCPtr(const RCPtr& q) noexcept : p(q.p) { retain(p); }
	RCPtr(RCPtr&& q) noexcept : p(q.p) { q.p = nullptr; }
	template<typename T2, subclass_only>
	RCPtr(const RCPtr<T2>& q) noexcept : p(q.p)
	{
		retain(p);
	}
	template<typename T2, subclass_only>
	RCPtr(RCPtr<T2>&& q) noexcept : p(q.p)
	{
		q.p = nullptr;
	}

	RCPtr& operator=(T* q) noexcept
	{
		retain_release(q, p);
		p = q;
		return *this;
	}
	RCPtr& operator=(std::nullptr_t) noexcept
	{
		release(p);
		p = nullptr;
		return *this;
	}
	RCPtr& operator=(const RCPtr& q) noexcept { return operator=(q.p); }
	RCPtr& operator=(RCPtr&& q) noexcept
	{
		release(p);
		p	= q.p;
		q.p = nullptr;
		return *this;
	}
	template<typename T2, subclass_only>
	RCPtr& operator=(const RCPtr<T2>& q) noexcept
	{
		return operator=(q.p);
	}
	template<typename T2, subclass_only>
	RCPtr& operator=(RCPtr<T2>&& q) noexcept
	{
		release(p);
		p	= q.p;
		q.p = nullptr;
		return *this;
	}

	// see https://stackoverflow.com/questions/11562/how-to-overload-stdswap
	friend void swap(RCPtr& a, RCPtr& b) noexcept { std::swap(a.p, b.p); }

	int refcnt() const noexcept { return p ? p->rc : 0; }
	T*	ptr() const noexcept { return p; }
	T*	operator->() const noexcept { return p; }
	T&	operator*() const noexcept
	{
		assert(p != nullptr);
		return *p;
	}
	operator T&() const noexcept
	{
		assert(p != nullptr);
		return *p;
	}
	operator T*() const noexcept { return p; }

	template<typename IDX>
	auto& operator[](IDX i) const
	{
		assert(p);
		return (*p)[i];
	}
};


/* _____________________________________________________________________
	The MTPtr can be constructed from T*, RCPtr<T> and other MTPtr<T>.
	The pointer can only be yielded as a protected RCPtr<T> or another MTPtr<T>.
	It cannot be yielded as a ptr or ref because these can be invalid the moment they are returned.
	For the same reason operators like -> and * are not provided.

	Use it like this:

		RCPtr(some_mtptr)->dosomething();
	
	or

		RCPtr p(some_mtptr);
		p->dothis();
		p->dothat();

	If you need to lock the object before manipulating it:

		NVPtr(some_mtptr)->dosomething();
*/
template<class T>
class MTPtr
{
	template<class TT>
	friend class MTPtr;
	template<class TT>
	friend class NVPtr;
	template<typename T2>
	friend class kio::MTPtr;

	using nvptr = typename std::remove_volatile<T>::type*;

	T* p;

	void copy(T* const& q) noexcept
	{
		kilipili_lock_spinlock();
		T* old = p;
		p	   = q;
		if (q) ++const_cast<nvptr>(q)->rc;
		bool f = old && --const_cast<nvptr>(old)->rc == 0;
		kilipili_unlock_spinlock();
		if (f) delete old;
	}
	void move(T*& q) noexcept
	{
		kilipili_lock_spinlock();
		T* old = p;
		p	   = q;
		q	   = nullptr;
		bool f = old && --const_cast<nvptr>(old)->rc == 0;
		kilipili_unlock_spinlock();
		if (f) delete old;
	}

public:
	MTPtr() noexcept : p(nullptr) {}
	MTPtr(std::nullptr_t) noexcept : p(nullptr) {}
	MTPtr(T* q) noexcept : p(q)
	{
		if (!p) return;
		kilipili_lock_spinlock();
		++const_cast<nvptr>(p)->rc;
		kilipili_unlock_spinlock();
	}
	~MTPtr() noexcept
	{
		if (!p) return;
		kilipili_lock_spinlock();
		auto n = --const_cast<nvptr>(p)->rc;
		kilipili_unlock_spinlock();
		if (n == 0) delete p;
	}

	MTPtr(const MTPtr& q) noexcept
	{
		kilipili_lock_spinlock();
		p = q.p;
		if (p) ++const_cast<nvptr>(p)->rc;
		kilipili_unlock_spinlock();
	}
	MTPtr(MTPtr&& q) noexcept : p(q.p) // assuming void source has no second ref
	{
		q.p = nullptr;
	}

	template<typename T2, subclass_only>
	MTPtr(const MTPtr<T2>& q) noexcept
	{
		kilipili_lock_spinlock();
		p = q.p;
		if (p) ++const_cast<nvptr>(p)->rc;
		kilipili_unlock_spinlock();
	}
	template<typename T2, subclass_only>
	MTPtr(const RCPtr<T2>& q) noexcept : MTPtr(q.p)
	{}
	template<typename T2, subclass_only>
	MTPtr(MTPtr<T2>&& q) noexcept : p(q.p) // assuming void source has no second ref
	{
		q.p = nullptr;
	}
	template<typename T2, subclass_only>
	MTPtr(RCPtr<T2>&& q) noexcept : p(q.p)
	{
		q.p = nullptr;
	}

	MTPtr& operator=(T* q) noexcept
	{
		copy(q);
		return *this;
	}
	MTPtr& operator=(const MTPtr& q) noexcept
	{
		copy(q.p);
		return *this;
	}
	template<typename T2, subclass_only>
	MTPtr& operator=(const MTPtr<T2>& q) noexcept
	{
		copy(q.p);
		return *this;
	}
	template<typename T2, subclass_only>
	MTPtr& operator=(const RCPtr<T2>& q) noexcept
	{
		copy(q.p);
		return *this;
	}
	MTPtr& operator=(MTPtr&& q) noexcept
	{
		move(q.p);
		return *this;
	}
	template<typename T2, subclass_only>
	MTPtr& operator=(MTPtr<T2>&& q) noexcept
	{
		move(q.p);
		return *this;
	}
	template<typename T2, subclass_only>
	MTPtr& operator=(RCPtr<T2>&& q) noexcept
	{
		move(q.p);
		return *this;
	}

	template<typename T2, baseclass_only>
	operator RCPtr<T2>() noexcept // yield a RCPtr
	{
		RCPtr<T2> rcptr;
		kilipili_lock_spinlock();
		rcptr->p = p;
		if (p) ++const_cast<nvptr>(p)->rc;
		kilipili_unlock_spinlock();
		return rcptr;
	}

	// see https://stackoverflow.com/questions/11562/how-to-overload-stdswap
	friend void swap(MTPtr& a, MTPtr& b) noexcept
	{
		kilipili_lock_spinlock();
		std::swap(a.p, b.p);
		kilipili_unlock_spinlock();
	}
};


/* _____________________________________________________________________
	NVPtr<T> and locks a volatile object until the NVPtr is destroyed or reassigned
	NVPtr<RCPtr<volatile T>> retains and locks the object.

	operator->() provides access to the non-volatile object.
	instantiation should normally succeed with automatic type deduction.

	class T must provide:
		- void lock() 
		- void unlock()
*/
template<class T>
class NVPtr
{
	T* p;

public:
	NO_COPY_MOVE(NVPtr);
	NVPtr() = delete;
	NVPtr(volatile T* q) : p(const_cast<T*>(q))
	{
		if (p) p->lock();
	}
	~NVPtr()
	{
		if (p) p->unlock();
	}

	NVPtr& operator=(volatile T* q)
	{
		if (p) p->unlock();
		p = const_cast<T*>(q);
		if (p) p->lock();
	}

	int refcnt() const noexcept { return p ? p->rc : 0; }
	T*	ptr() const { return p; }
	T*	operator->() const { return p; }
		operator T*() const { return p; }

	template<typename IDX>
	auto& operator[](IDX i) const
	{
		assert(p);
		return (*p)[i];
	}
};

template<class T>
class NVPtr<RCPtr<volatile T>>
{
	T* p; // will be retained!

	inline void unlock()
	{
		if (!p) return;
		p->unlock();
		kilipili_lock_spinlock();
		auto n = --p->rc;
		kilipili_unlock_spinlock();
		if (n == 0) delete p;
	}
	inline void lock()
	{
		if (!p) return;
		kilipili_lock_spinlock();
		++p->rc;
		kilipili_unlock_spinlock();
		p->lock();
	}
	void copy(volatile T*& q)
	{
		T* old = p;
		if (old) old->unlock();
		kilipili_lock_spinlock();
		p = const_cast<T*>(q);
		if (p) ++p->rc;
		bool f = old && --old->rc == 0;
		kilipili_unlock_spinlock();
		if (f) delete old;
		if (p) p->lock();
	}

public:
	NO_COPY_MOVE(NVPtr);
	NVPtr() = delete;
	~NVPtr() { unlock(); }

	template<typename T2, subclass_only>
	NVPtr(const RCPtr<volatile T2>& q) : p(const_cast<T2*>(q.p))
	{
		lock();
	}
	template<typename T2, subclass_only>
	NVPtr(const MTPtr<volatile T2>& q)
	{
		kilipili_lock_spinlock();
		p = const_cast<T2*>(q.p);
		if (p) ++p->rc;
		kilipili_unlock_spinlock();
		if (p) p->lock();
	}

	template<typename T2, subclass_only>
	NVPtr& operator=(const RCPtr<volatile T2>& q)
	{
		copy(q.p);
		return *this;
	}

	template<typename T2, subclass_only>
	NVPtr& operator=(const MTPtr<volatile T2>& q)
	{
		copy(q.p);
		return *this;
	}

	int refcnt() const noexcept { return p ? p->rc : 0; }
	T*	ptr() const { return p; }
	T*	operator->() const { return p; }
		operator T*() const { return p; }

	template<typename IDX>
	auto& operator[](IDX i) const
	{
		assert(p);
		return (*p)[i];
	}
};


// _____________________________________________________________________
// deduction guides:

template<typename T>
RCPtr(T*) -> RCPtr<T>;

template<typename T>
MTPtr(RCPtr<T>) -> MTPtr<T>;

template<typename T>
NVPtr(volatile T*) -> NVPtr<T>;

template<typename T>
NVPtr(const RCPtr<volatile T>&) -> NVPtr<RCPtr<volatile T>>;

template<typename T>
NVPtr(const MTPtr<volatile T>&) -> NVPtr<RCPtr<volatile T>>;

#undef subclass_only

} // namespace kio


/*



































*/
