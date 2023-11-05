#pragma once
// Copyright (c) 2018 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include <type_traits>

namespace kio
{

// select type T1 or T2:
template<bool, typename T1, typename T2>
struct select_type
{
	typedef T2 type;
};
template<typename T1, typename T2>
struct select_type<true, T1, T2>
{
	typedef T1 type;
};


template<typename T>
struct has_oper_star
{
	// test whether type T has member function operator*():
	// has_oper_star<T>::value = true|false

	struct Foo
	{};
	template<typename C>
	static Foo test(...);
	template<typename C>
	static decltype(*std::declval<C>()) test(int);

	static constexpr bool value = !std::is_same<Foo, decltype(test<T>(99))>::value;
};

static_assert(has_oper_star<int*>::value, "");
static_assert(!has_oper_star<int>::value, "");
static_assert(!has_oper_star<int&>::value, "");

}; // namespace kio
