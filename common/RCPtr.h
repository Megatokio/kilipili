// Copyright (c) 2015 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include <type_traits>


namespace kio
{


template<class T>
class RCPtr
{
	template<class TT>
	friend class RCArray;
	template<class T1, class T2>
	friend class RCHashMap;

protected:
	T* p;

private:
	void retain() const noexcept
	{
		if (p) p->retain();
	}
	void release() const noexcept
	{
		if (p) p->release();
	}

public:
	RCPtr() noexcept : p(nullptr) {}
	RCPtr(T* p) noexcept : p(p) { retain(); }
	RCPtr(const RCPtr& q) noexcept : p(q.p) { retain(); }
	RCPtr(RCPtr&& q) noexcept : p(q.p) { q.p = nullptr; }
	~RCPtr() noexcept { release(); }

	RCPtr& operator=(RCPtr&& q) noexcept
	{
		std::swap(p, q.p);
		return *this;
	}
	RCPtr& operator=(const RCPtr& q) noexcept
	{
		q.retain();
		release();
		p = q.p;
		return *this;
	}
	RCPtr& operator=(T* q) noexcept
	{
		if (q) q->retain();
		release();
		p = q;
		return *this;
	}

#define subclass_only typename std::enable_if_t<std::is_base_of<T, T2>::value, int> = 1
#ifdef _GCC
	template<typename T2, subclass_only>
	friend class kio::RCPtr;
#else
	template<typename T2>
	friend class kio::RCPtr;
#endif
	template<typename T2, subclass_only>
	RCPtr& operator=(RCPtr<T2>&& q) noexcept
	{
		q.retain();
		release();
		p = q.ptr();
		return *this;
	}
	template<typename T2, subclass_only>
	RCPtr& operator=(const RCPtr<T2>& q) noexcept
	{
		q.retain();
		release();
		p = q.ptr();
		return *this;
	}
	template<typename T2, subclass_only>
	RCPtr(RCPtr<T2>& q) noexcept : p(q.ptr())
	{
		retain();
	}
	template<typename T2, subclass_only>
	RCPtr(RCPtr<T2>&& q) noexcept : p(q.ptr())
	{
		retain();
	}
#undef subclass_only

	// see https://stackoverflow.com/questions/11562/how-to-overload-stdswap
	static void swap(RCPtr<T>& a, RCPtr<T>& b) noexcept { std::swap(a.p, b.p); }

	T* operator->() const noexcept { return p; }
	T& operator*() const noexcept { return *p; }
	T* ptr() const noexcept { return p; }
	T& ref() const noexcept
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

	unsigned int refcnt() const noexcept { return p ? p->refcnt() : 0; }

	template<typename IDX>
	auto& operator[](IDX i) const
	{
		assert(p);
		return p->operator[](i);
	}

	bool is(const T* b) const noexcept { return p == b; }
	bool isnot(const T* b) const noexcept { return p != b; }
		 operator bool() const noexcept { return p != nullptr; }
};

} // namespace kio


/*

















*/